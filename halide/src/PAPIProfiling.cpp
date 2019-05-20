#include <algorithm>
#include <map>
#include <string>
#include <limits>

#include "PAPIProfiling.h"
#include "CodeGen_Internal.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Simplify.h"
#include "Substitute.h"
#include "Util.h"

namespace Halide {
namespace Internal {

using std::map;
using std::pair;
using std::string;
using std::tuple;
using std::vector;

vector<tuple<string, int, bool, string>> papi_profiler_defs;

class InjectPAPIProfiling : public IRMutator2 {
public:
    map<string, int> indices;   // maps from func name -> index in buffer.
    map<int, int> parent_prod;   // maps from func int -> parent func id in production level.
    map<int, int> parent_cons;   // maps from func int -> parent func id in consumption level.
    map<int, int> show_threads_prod;   // maps from func int -> show threads in profiler in production level.
    map<int, int> show_threads_cons;   // maps from func int -> show threads in profiler in consumption level.

    vector<int> stack; // What produce nodes are we currently inside of.
    vector<pair<bool, bool>> profile_stack; // Produce node we are inside of must be profiled?

    string pipeline_name;

    InjectPAPIProfiling(const string &pipeline_name) : pipeline_name(pipeline_name) {
        indices["overhead"] = 0;
        stack.push_back(0);
    }

    map<int, uint64_t> func_childrens; // map from func_id -> number of producer-consumer childrens
    map<int, uint64_t> func_stack_current; // map from func id -> current stack allocation
    map<int, uint64_t> func_stack_peak; // map from func id -> peak stack allocation

private:
    using IRMutator2::visit;

    struct AllocSize {
        bool on_stack;
        Expr size;
    };

    Scope<AllocSize> func_alloc_sizes;

    bool profiling_memory = true;

    // Strip down the tuple name, e.g. f.0 into f
    string normalize_name(const string &name) {
        vector<string> v = split_string(name, ".");
        internal_assert(v.size() > 0);
        return v[0];
    }

    int get_func_id(const string &name) {
        string norm_name = normalize_name(name);
        int idx = -1;
        map<string, int>::iterator iter = indices.find(norm_name);
        if (iter == indices.end()) {
            idx = (int)indices.size();
            indices[norm_name] = idx;
        } else {
            idx = iter->second;
        }
        return idx;
    }

    Expr compute_allocation_size(const vector<Expr> &extents,
                                 const Expr &condition,
                                 const Type &type,
                                 const std::string &name,
                                 bool &on_stack) {
        on_stack = true;

        Expr cond = simplify(condition);
        if (is_zero(cond)) { // Condition always false
            return make_zero(UInt(64));
        }

        int32_t constant_size = Allocate::constant_allocation_size(extents, name);
        if (constant_size > 0) {
            int64_t stack_bytes = constant_size * type.bytes();
            if (can_allocation_fit_on_stack(stack_bytes)) { // Allocation on stack
                return make_const(UInt(64), stack_bytes);
            }
        }

        // Check that the allocation is not scalar (if it were scalar
        // it would have constant size).
        internal_assert(extents.size() > 0);

        on_stack = false;
        Expr size = cast<uint64_t>(extents[0]);
        for (size_t i = 1; i < extents.size(); i++) {
            size *= extents[i];
        }
        size = simplify(Select::make(condition, size * type.bytes(), make_zero(UInt(64))));
        return size;
    }

    /*
    Stmt visit(const Allocate *op) override {
        int idx = get_func_id(op->name);

        vector<Expr> new_extents;
        bool all_extents_unmodified = true;
        for (size_t i = 0; i < op->extents.size(); i++) {
            new_extents.push_back(mutate(op->extents[i]));
            all_extents_unmodified &= new_extents[i].same_as(op->extents[i]);
        }
        Expr condition = mutate(op->condition);

        bool on_stack;
        Expr size = compute_allocation_size(new_extents, condition, op->type, op->name, on_stack);
        internal_assert(size.type() == UInt(64));
        func_alloc_sizes.push(op->name, {on_stack, size});

        // compute_allocation_size() might return a zero size, if the allocation is
        // always conditionally false. remove_dead_allocations() is called after
        // inject_profiling() so this is a possible scenario.
        if (!is_zero(size) && on_stack) {
            const uint64_t *int_size = as_const_uint(size);
            internal_assert(int_size != NULL); // Stack size is always a const int
            func_stack_current[idx] += *int_size;
            func_stack_peak[idx] = std::max(func_stack_peak[idx], func_stack_current[idx]);
            debug(3) << "  Allocation on stack: " << op->name << "(" << size << ") in pipeline " << pipeline_name
                     << "; current: " << func_stack_current[idx] << "; peak: " << func_stack_peak[idx] << "\n";
        }

        Stmt body = mutate(op->body);
        Expr new_expr;
        Stmt stmt;
        if (op->new_expr.defined()) {
            new_expr = mutate(op->new_expr);
        }
        if (all_extents_unmodified &&
            body.same_as(op->body) &&
            condition.same_as(op->condition) &&
            new_expr.same_as(op->new_expr)) {
            stmt = op;
        } else {
            stmt = Allocate::make(op->name, op->type, op->memory_type,
                                  new_extents, condition, body, new_expr, op->free_function);
        }

        if (!is_zero(size) && !on_stack && profiling_memory) {
            Expr profiler_pipeline_state = Variable::make(Handle(), "profiler_pipeline_state");
            debug(3) << "  Allocation on heap: " << op->name << "(" << size << ") in pipeline " << pipeline_name << "\n";
            Expr set_task = Call::make(Int(32), "halide_papi_memory_allocate",
                                       {profiler_pipeline_state, idx, size}, Call::Extern);
            stmt = Block::make(Evaluate::make(set_task), stmt);
        }
        return stmt;
    }

    Stmt visit(const Free *op) override {
        int idx = get_func_id(op->name);

        AllocSize alloc = func_alloc_sizes.get(op->name);
        internal_assert(alloc.size.type() == UInt(64));
        func_alloc_sizes.pop(op->name);

        Stmt stmt = IRMutator2::visit(op);

        if (!is_zero(alloc.size)) {
            Expr profiler_pipeline_state = Variable::make(Handle(), "profiler_pipeline_state");

            if (!alloc.on_stack) {
                if (profiling_memory) {
                    debug(3) << "  Free on heap: " << op->name << "(" << alloc.size << ") in pipeline " << pipeline_name << "\n";
                    Expr set_task = Call::make(Int(32), "halide_papi_memory_free",
                                               {profiler_pipeline_state, idx, alloc.size}, Call::Extern);
                    stmt = Block::make(Evaluate::make(set_task), stmt);
                }
            } else {
                const uint64_t *int_size = as_const_uint(alloc.size);
                internal_assert(int_size != nullptr);

                func_stack_current[idx] -= *int_size;
                debug(3) << "  Free on stack: " << op->name << "(" << alloc.size << ") in pipeline " << pipeline_name
                         << "; current: " << func_stack_current[idx] << "; peak: " << func_stack_peak[idx] << "\n";
            }
        }
        return stmt;
    }
    */

    Stmt visit(const ProducerConsumer *op) override {
        int idx;
        Stmt body, stmt;
        bool must_profile = false;
        bool must_show_threads = false;

        int parent = (stack.size() > 1) ? stack.back() : 0;
        std::string parent_name;

        for(auto& func: indices) {
          if(func.second == parent) {
            parent_name = func.first;
          }
        }

        auto it = find_if(papi_profiler_defs.begin(), papi_profiler_defs.end(),
                          [op, parent_name](std::tuple<string, int, bool, string> const &elem) {
            bool level_cond =
                (std::get<1>(elem) & PROFILE_PRODUCTION && op->is_producer) ||
                (std::get<1>(elem) & PROFILE_CONSUMPTION && !op->is_producer);

            bool parent_cond =
                (std::get<3>(elem) == "") || (std::get<3>(elem) == parent_name);

            return std::get<0>(elem) == op->name && level_cond && parent_cond;
        });

        must_profile = it != papi_profiler_defs.end() && std::get<2>(*it);
        must_show_threads = must_profile && std::get<1>(*it) & PROFILE_SHOW_THREADS;

        if(stack.size() > 1) {
            func_childrens[stack.back()]++;
        }

        if (op->is_producer) {
            idx = get_func_id(op->name);
            func_childrens[idx] = 0;

            profile_stack.push_back(std::make_pair(must_profile, must_show_threads));
            stack.push_back(idx);
            body = mutate(op->body);
            stack.pop_back();
            profile_stack.pop_back();
        } else {
            body = mutate(op->body);
            // At the beginning of the consume step, set the current task
            // back to the outer one.
            idx = stack.back();
        }

        if(!must_profile && it == papi_profiler_defs.end() && profile_stack.back().first) {
          must_profile = true;
          must_show_threads = profile_stack.back().second;
        }

        Expr profiler_token = Variable::make(Int(32), "profiler_token");
        Expr profiler_state = Variable::make(Handle(), "profiler_state");

        if(must_profile) {
            Expr enter_task, leave_task;

            if(op->is_producer && func_childrens[idx] > 0) {
                enter_task = Call::make(Int(32), "halide_papi_enter_overhead_region",
                                        {profiler_state, profiler_token, idx}, Call::Extern);

                leave_task = Call::make(Int(32), "halide_papi_leave_overhead_region",
                                        {profiler_state, profiler_token, idx}, Call::Extern);
            } else {
                enter_task = Call::make(Int(32), "halide_papi_enter_current_func",
                                        {profiler_state, profiler_token, get_func_id(op->name),
                                         make_bool(op->is_producer)}, Call::Extern);

                leave_task = Call::make(Int(32), "halide_papi_leave_current_func",
                                        {profiler_state, profiler_token, get_func_id(op->name),
                                         make_bool(op->is_producer)}, Call::Extern);
            }

            body = Block::make({Evaluate::make(enter_task), body, Evaluate::make(leave_task)});
        }

        // This call gets inlined and becomes a single store instruction.
        Expr set_task = Call::make(Int(32), "halide_papi_set_current_func",
                                   {profiler_state, profiler_token, idx}, Call::Extern);

        body = Block::make(Evaluate::make(set_task), body);

        stmt = ProducerConsumer::make(op->name, op->is_producer, body, op->must_profile);

        if(must_profile && !profile_stack.empty()) {
            int parent = stack.back();

            Expr enter_overhead = Call::make(Int(32), "halide_papi_enter_overhead_region",
                                             {profiler_state, profiler_token, parent}, Call::Extern);

            Expr leave_overhead = Call::make(Int(32), "halide_papi_leave_overhead_region",
                                             {profiler_state, profiler_token, parent}, Call::Extern);

            stmt = Block::make({Evaluate::make(leave_overhead), stmt, Evaluate::make(enter_overhead)});
        }

        auto fid = get_func_id(op->name);

        if(op->is_producer) {
          parent_prod[fid] = parent;
          show_threads_prod[fid] = must_show_threads;
        } else {
          parent_cons[fid] = parent;
          show_threads_cons[fid] = must_show_threads;
        }

        return stmt;
    }

    Stmt visit(const For *op) override {
        Stmt body = op->body;

        // The for loop indicates a device transition or a
        // parallel job launch. Decrement the number of active
        // threads outside the loop, and increment it inside the
        // body.
        bool update_active_threads = (op->device_api == DeviceAPI::Hexagon ||
                                      op->is_parallel());

        Expr state = Variable::make(Handle(), "profiler_state");

        Stmt enter_parallel_region =
            Evaluate::make(Call::make(Int(32), "halide_papi_enter_parallel_region",
                                      {state}, Call::Extern));
        Stmt leave_parallel_region =
            Evaluate::make(Call::make(Int(32), "halide_papi_leave_parallel_region",
                                      {state}, Call::Extern));

        Stmt incr_active_threads =
            Evaluate::make(Call::make(Int(32), "halide_papi_incr_active_threads",
                                      {state}, Call::Extern));
        Stmt decr_active_threads =
            Evaluate::make(Call::make(Int(32), "halide_papi_decr_active_threads",
                                      {state}, Call::Extern));

        if (update_active_threads) {
            body = Block::make({incr_active_threads, body, decr_active_threads});
        }

        // We profile by storing a token to global memory, so don't enter GPU loops
        if (op->device_api == DeviceAPI::Hexagon) {
            // TODO: This is for all offload targets that support
            // limited internal profiling, which is currently just
            // hexagon. We don't support per-func stats remotely,
            // which means we can't do memory accounting.
            bool old_profiling_memory = profiling_memory;
            profiling_memory = false;
            body = mutate(body);

            profiling_memory = old_profiling_memory;

            // Get the profiler state pointer from scratch inside the
            // kernel. There will be a separate copy of the state on
            // the DSP that the host side will periodically query.
            Expr get_state = Call::make(Handle(), "halide_papi_get_state", {}, Call::Extern);
            body = substitute("profiler_state", Variable::make(Handle(), "hvx_profiler_state"), body);
            body = LetStmt::make("hvx_profiler_state", get_state, body);
        } else if (op->device_api == DeviceAPI::None ||
                   op->device_api == DeviceAPI::Host) {
            body = mutate(body);
        } else {
            body = op->body;
        }

        Stmt stmt = For::make(op->name, op->min, op->extent, op->for_type, op->device_api, body);

        if (update_active_threads) {
            stmt = Block::make({decr_active_threads, enter_parallel_region, stmt, leave_parallel_region, incr_active_threads});
        }
        return stmt;
    }
};

Stmt inject_papi_profiling(Stmt s, string pipeline_name) {
    InjectPAPIProfiling profiling(pipeline_name);
    s = profiling.mutate(s);

    int num_funcs = (int)(profiling.indices.size());

    Expr func_names_buf = Variable::make(Handle(), "profiling_func_names");

    Expr func_parents_prod_buf = Variable::make(Handle(), "profiling_func_parents_prod");

    Expr func_parents_cons_buf = Variable::make(Handle(), "profiling_func_parents_cons");

    Expr func_threads_prod_buf = Variable::make(Handle(), "profiling_func_threads_prod");

    Expr func_threads_cons_buf = Variable::make(Handle(), "profiling_func_threads_cons");

    Expr start_profiler = Call::make(Int(32), "halide_papi_pipeline_start",
                                     {pipeline_name, num_funcs, func_names_buf,
                                      func_parents_prod_buf, func_parents_cons_buf,
                                      func_threads_prod_buf, func_threads_cons_buf}, Call::Extern);

    Expr get_state = Call::make(Handle(), "halide_papi_get_state", {}, Call::Extern);

    Expr get_pipeline_state = Call::make(Handle(), "halide_papi_get_pipeline_state", {pipeline_name}, Call::Extern);

    Expr profiler_token = Variable::make(Int(32), "profiler_token");

    Expr stop_profiler = Call::make(Int(32), Call::register_destructor,
                                    {Expr("halide_papi_pipeline_end"), get_state}, Call::Intrinsic);

    /*
    bool no_stack_alloc = profiling.func_stack_peak.empty();
    if (!no_stack_alloc) {
        Expr func_stack_peak_buf = Variable::make(Handle(), "profiling_func_stack_peak_buf");

        Expr profiler_pipeline_state = Variable::make(Handle(), "profiler_pipeline_state");
        Stmt update_stack = Evaluate::make(Call::make(Int(32), "halide_papi_stack_peak_update",
                                           {profiler_pipeline_state, func_stack_peak_buf}, Call::Extern));
        s = Block::make(update_stack, s);
    }
    */

    Expr profiler_state = Variable::make(Handle(), "profiler_state");

    Stmt incr_active_threads =
        Evaluate::make(Call::make(Int(32), "halide_papi_incr_active_threads",
                                  {profiler_state}, Call::Extern));
    Stmt decr_active_threads =
        Evaluate::make(Call::make(Int(32), "halide_papi_decr_active_threads",
                                  {profiler_state}, Call::Extern));
    s = Block::make({incr_active_threads, s, decr_active_threads});

    s = LetStmt::make("profiler_pipeline_state", get_pipeline_state, s);
    s = LetStmt::make("profiler_state", get_state, s);

    // If there was a problem starting the profiler, it will call an
    // appropriate halide error function and then return the
    // (negative) error code as the token.
    s = Block::make(AssertStmt::make(profiler_token >= 0, profiler_token), s);
    s = LetStmt::make("profiler_token", start_profiler, s);

    /*
    if (!no_stack_alloc) {
        for (int i = num_funcs-1; i >= 0; --i) {
            s = Block::make(Store::make("profiling_func_stack_peak_buf",
                                        make_const(UInt(64), profiling.func_stack_peak[i]),
                                        i, Parameter(), const_true()), s);
        }
        s = Block::make(s, Free::make("profiling_func_stack_peak_buf"));
        s = Allocate::make("profiling_func_stack_peak_buf", UInt(64),
                           MemoryType::Auto, {num_funcs}, const_true(), s);
    }
    */

    for (std::pair<string, int> p : profiling.indices) {
        s = Block::make(Store::make("profiling_func_names", p.first, p.second, Parameter(), const_true()), s);
    }

    for (std::pair<int, int> p : profiling.parent_prod) {
        s = Block::make(Store::make("profiling_func_parents_prod", p.second, p.first, Parameter(), const_true()), s);
    }

    for (std::pair<int, int> p : profiling.parent_cons) {
        s = Block::make(Store::make("profiling_func_parents_cons", p.second, p.first, Parameter(), const_true()), s);
    }

    for (std::pair<int, int> p : profiling.show_threads_prod) {
        s = Block::make(Store::make("profiling_func_threads_prod", p.second, p.first, Parameter(), const_true()), s);
    }

    for (std::pair<int, int> p : profiling.show_threads_cons) {
        s = Block::make(Store::make("profiling_func_threads_cons", p.second, p.first, Parameter(), const_true()), s);
    }

    s = Block::make(s, Free::make("profiling_func_parents_prod"));
    s = Block::make(s, Free::make("profiling_func_parents_cons"));
    s = Block::make(s, Free::make("profiling_func_threads_prod"));
    s = Block::make(s, Free::make("profiling_func_threads_cons"));
    s = Block::make(s, Free::make("profiling_func_names"));
    s = Allocate::make("profiling_func_names", Handle(),
                       MemoryType::Auto, {num_funcs}, const_true(), s);
    s = Allocate::make("profiling_func_threads_prod", Handle(),
                       MemoryType::Auto, {num_funcs}, const_true(), s);
    s = Allocate::make("profiling_func_threads_cons", Handle(),
                       MemoryType::Auto, {num_funcs}, const_true(), s);
    s = Allocate::make("profiling_func_parents_prod", Handle(),
                       MemoryType::Auto, {num_funcs}, const_true(), s);
    s = Allocate::make("profiling_func_parents_cons", Handle(),
                       MemoryType::Auto, {num_funcs}, const_true(), s);
    s = Block::make(Evaluate::make(stop_profiler), s);

    return s;
}

}
}
