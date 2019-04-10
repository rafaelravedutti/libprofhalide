#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "interpolate_aot.h"
/* ... */
#include <stdio.h>

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, const char **argv) {
  Halide::Runtime::Buffer<float> in_png = Tools::load_and_convert_image("input.png");
  Halide::Runtime::Buffer<float> out(in_png.width(), in_png.height(), 3);
  int error;

  if((error = normalize(in_png, out)) != 0) {
    fprintf(stderr, "Halide returned an error: %d\n", error);
  }

  Tools::convert_and_save_image(out, "output.png");

  return 0;
}
