/* Halide library */
#include "Halide.h"
#include "halide_image_io.h"
/* ... */
#include <stdio.h>

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, const char **argv) {
  Buffer<uint8_t> input = Tools::load_image("input.png");
  Buffer<uint8_t> output(input.width() - 2, input.height() - 2, input.channels());
  Halide::Func blur_x, blur_y;
  Var x, y, c, xi, yi;

  blur_x(x, y, c) = (input(x - 1, y, c) + input(x, y, c) + input(x + 1, y, c)) / 3;
  blur_y(x, y, c) = (blur_x(x, y - 1, c) + blur_x(x, y, c) + blur_x(x, y + 1, c)) / 3;

  //blur_y.tile(x, y, xi, yi, 32, 16);
  //blur_x.compute_at(blur_y, y);

  blur_y.tile(x, y, xi, yi, 256, 32).vectorize(xi, 8).parallel(y);
  blur_x.compute_at(blur_y, x).vectorize(x, 8);

  //blur_x.compute_root();

  output.set_min(1, 1);

  blur_x.profile();
  //blur_y.profile();

#ifdef COMPILE_AOT

  blur_y.compile_to_lowered_stmt("blur.html", {input}, HTML);
  blur_y.compile_to_static_library("blur_3x3_aot", {input}, "blur_y");

#else

  blur_y.realize(output);

#endif

  return 0;
}
