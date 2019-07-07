#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "interpolate_aot.h"
/* ... */
#include <stdio.h>

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, const char **argv) {
  //Halide::Runtime::Buffer<float> input = Tools::load_and_convert_image("input.png");
  //Halide::Runtime::Buffer<float> input(3840, 2160, 4);
  Halide::Runtime::Buffer<float> input(10240, 4320, 4);
  Halide::Runtime::Buffer<float> out(input.width(), input.height(), 3);
  int error;

  halide_set_num_threads(4);

  for(int c = 0; c < input.channels(); ++c) {
    for(int y = 0; y < input.height(); ++y) {
      for(int x = 0; x < input.width(); ++x) {
        input(x, y, c) = rand();
      }
    }
  }

  if((error = normalize(input, out)) != 0) {
    fprintf(stderr, "Halide returned an error: %d\n", error);
  }

  return 0;
}
