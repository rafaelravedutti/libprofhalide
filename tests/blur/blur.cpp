/* Halide library */
#include "Halide.h"
#include "halide_image_io.h"
/* ... */
#include <iostream>

using namespace Halide;
using namespace Halide::Tools;

using std::vector;

int main(int argc, const char **argv) {
  Buffer<float> input(10112, 10112, 1);
  //Buffer<float> input = Tools::load_and_convert_image("input.png");
  Buffer<float> output(input.width() - 2, input.height() - 2, input.channels());
  Func blur_x, blur_y;
  Var x, y, c, xi, yi;

  blur_x(x, y, c) = (input(x - 1, y, c) + input(x, y, c) + input(x + 1, y, c)) / 3.0f;
  blur_y(x, y, c) = (blur_x(x, y - 1, c) + blur_x(x, y, c) + blur_x(x, y + 1, c)) / 3.0f;

  //blur_y.tile(x, y, xi, yi, 32, 32);
  //blur_x.compute_at(blur_y, x);

  //blur_y.tile(x, y, xi, yi, 256, 32).vectorize(xi, 8).parallel(y);
  //blur_y.tile(x, y, xi, yi, 256, 32).vectorize(xi, 8);
  //blur_x.compute_at(blur_y, x).vectorize(x, 8);

  //blur_x.store_at(blur_y, c).compute_at(blur_y, y);

  //blur_x.compute_root();

  blur_y.profile(PROFILE_PRODUCTION, true, true);

  output.set_min(1, 1);

#ifdef COMPILE_AOT

  blur_y.compile_to_lowered_stmt("blur.html", {input}, HTML);
  blur_y.compile_to_static_library("blur_aot", {input}, "blur_y");

#else

  blur_y.realize(output);

#endif

  return 0;
}
