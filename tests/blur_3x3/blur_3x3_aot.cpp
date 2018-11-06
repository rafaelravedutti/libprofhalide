#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "blur_3x3_aot.h"
/* ... */
#include <stdio.h>

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, const char **argv) {
  Halide::Runtime::Buffer<uint8_t> input = Tools::load_image("input.png");
  Halide::Runtime::Buffer<uint8_t> output(input.width() - 2, input.height() - 2, input.channels());
  int error;

  output.set_min(1, 1);

  if((error = blur_y(input, output)) != 0) {
    fprintf(stderr, "Halide returned an error: %d\n", error);
  }

  return 0;
}
