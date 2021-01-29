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
    Buffer<float> output(input.width() - 2, input.height() - 2, input.channels());
    Func blur_x, blur_y;
    Var x, y, c, xi, yi;
    string schedule;

    blur_x(x, y, c) = (input(x - 1, y, c) + input(x, y, c) + input(x + 1, y, c)) / 3.0f;
    blur_y(x, y, c) = (blur_x(x, y - 1, c) + blur_x(x, y, c) + blur_x(x, y + 1, c)) / 3.0f;

#if SCHEDULE == 1
    /* Breadth-first */
    schedule = "breadth_first";
    blur_x.compute_root().vectorize(x, 8);
    blur_y.vectorize(x, 8);

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
    blur_y.vectorize(x, 8);

    #ifdef PARALLEL
    blur_y.parallel(y);
    #endif

    #ifdef PROFILE
    blur_x.profile(PROFILE_PRODUCTION, true, true);
    blur_y.profile(PROFILE_PRODUCTION, true, true);
    #endif

#elif SCHEDULE == 3
    /* Sliding window */
    schedule = "sliding_window";
    blur_x.store_at(blur_y, c).compute_at(blur_y, x).vectorize(x, 8);
    blur_y.vectorize(x, 8);

    #ifdef PARALLEL
    blur_y.parallel(y);
    #endif

    #ifdef PROFILE
    profile_at(blur_y, c, true);
    #endif

#elif SCHEDULE == 4
    /* Tile (block dimension = 32x32) */
    schedule = "tile_32x32";
    blur_x.compute_at(blur_y, x).vectorize(x, 8);
    blur_y.tile(x, y, xi, yi, 32, 32).vectorize(xi, 8);

    #ifdef PARALLEL
    blur_y.parallel(y);
    #endif

    #ifdef PROFILE
    blur_x.profile(PROFILE_PRODUCTION, true, true);
    blur_y.profile(PROFILE_PRODUCTION, true, true);
    #endif

#else
    schedule = "invalid";
    #error "Invalid schedule!"
#endif

    output.set_min(1, 1);

#ifdef COMPILE_AOT
#ifdef PROFILE
    #ifndef PARALLEL
    blur_y.compile_to_lowered_stmt("html/blur_" + schedule + "_serial.html", {input}, HTML);
    blur_y.compile_to_lowered_stmt("txt/blur_" + schedule + "_serial.txt", {input}, Text);
    #else
    blur_y.compile_to_lowered_stmt("html/blur_" + schedule + "_parallel.html", {input}, HTML);
    blur_y.compile_to_lowered_stmt("txt/blur_" + schedule + "_parallel.txt", {input}, Text);
    #endif
#endif
    blur_y.compile_to_static_library("blur_aot", {input}, "blur_y");
#else
    blur_y.realize(output);
#endif

  return 0;
}
