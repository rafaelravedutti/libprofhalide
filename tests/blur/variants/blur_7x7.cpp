/* Halide library */
#include "Halide.h"
//#include "halide_image_io.h"
/* ... */
#include <iostream>
#include <string>

using namespace Halide;
//using namespace Halide::Tools;

using std::vector;
using std::string;

#if !defined SCHEDULE
    #error "You must define a schedule!"
#endif

#if !defined IMAGE_WIDTH || !defined IMAGE_HEIGHT || !defined IMAGE_CHANNELS
    #error "You must define the image dimensions!"
#endif

int main(int argc, const char **argv) {
    Buffer<float> input(IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_CHANNELS);
    Buffer<float> output(input.width(), input.height(), input.channels());
    Func in_bounded, kernel, blur_x, blur_y;
    Var x, y, c, xi, yi;
    string schedule;
    float sigma = 1.5f;

    in_bounded = BoundaryConditions::repeat_edge(input);
    kernel(x) = exp(-x * x / (2 * sigma * sigma)) / (sqrtf(2 * M_PI) * sigma);

    blur_y(x, y, c) = (kernel(0) * in_bounded(x, y, c) +
                       kernel(1) * (in_bounded(x, y - 1, c) + in_bounded(x, y + 1, c)) +
                       kernel(2) * (in_bounded(x, y - 2, c) + in_bounded(x, y + 2, c)) +
                       kernel(3) * (in_bounded(x, y - 3, c) + in_bounded(x, y + 3, c)));

    blur_x(x, y, c) = (kernel(0) * blur_y(x, y, c) +
                       kernel(1) * (blur_y(x - 1, y, c) + blur_y(x + 1, y, c)) +
                       kernel(2) * (blur_y(x - 2, y, c) + blur_y(x + 2, y, c)) +
                       kernel(3) * (blur_y(x - 3, y, c) + blur_y(x + 3, y, c)));

#if SCHEDULE == 1
    /* Breadth-first */
    schedule = "breadth_first";
    blur_x.vectorize(x, 8);
    blur_y.compute_root().vectorize(x, 8);

    #ifdef PARALLEL
    blur_x.parallel(y);
    blur_y.parallel(y);
    #endif

    #ifdef PROFILE
    blur_x.profile(PROFILE_PRODUCTION, true, true);
    blur_y.profile(PROFILE_PRODUCTION, true, true);
    #endif

#elif SCHEDULE == 2
    /* Full-fusion */
    schedule = "full_fusion";
    blur_x.vectorize(x, 8);

    #ifdef PARALLEL
    blur_x.parallel(y);
    #endif

    #ifdef PROFILE
    blur_x.profile(PROFILE_PRODUCTION, true, true);
    blur_y.profile(PROFILE_PRODUCTION, true, true);
    #endif

#elif SCHEDULE == 3
    /* Sliding window */
    schedule = "sliding_window";
    blur_x.vectorize(x, 8);
    blur_y.store_at(blur_x, c).compute_at(blur_x, x).vectorize(x, 8);

    #ifdef PARALLEL
    blur_x.parallel(y);
    #endif

    #ifdef PROFILE
    profile_at(blur_x, c, true);
    #endif

#elif SCHEDULE == 4
    /* Tile (block dimension = 32x32) */
    schedule = "tile_32x32";
    blur_x.tile(x, y, xi, yi, 32, 32).vectorize(xi, 8);
    blur_y.compute_at(blur_x, x).vectorize(x, 8);

    #ifdef PARALLEL
    blur_x.parallel(y);
    #endif

    #ifdef PROFILE
    blur_x.profile(PROFILE_PRODUCTION, true, true);
    blur_y.profile(PROFILE_PRODUCTION, true, true);
    #endif
#else
    schedule = "invalid";
    #error "Invalid schedule!"
#endif

    //output.set_min(1, 1);

#ifdef COMPILE_AOT
#ifdef PROFILE
    #ifndef PARALLEL
    blur_x.compile_to_lowered_stmt("html/blur_" + schedule + "_serial.html", {input}, HTML);
    blur_x.compile_to_lowered_stmt("txt/blur_" + schedule + "_serial.txt", {input}, Text);
    #else
    blur_x.compile_to_lowered_stmt("html/blur_" + schedule + "_parallel.html", {input}, HTML);
    blur_x.compile_to_lowered_stmt("txt/blur_" + schedule + "_parallel.txt", {input}, Text);
    #endif
#endif
    blur_x.compile_to_static_library("blur_aot", {input}, "blur_x");
#else
    blur_x.realize(output);
#endif

  return 0;
}
