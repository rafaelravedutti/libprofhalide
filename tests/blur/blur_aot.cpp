#include "HalideBuffer.h"
//#include "halide_image_io.h"
#include "blur_aot.h"
/* ... */
#include <stdio.h>

using namespace Halide;
//using namespace Halide::Tools;

int main(int argc, const char **argv) {
  //Halide::Runtime::Buffer<float> input = Tools::load_and_convert_image("input.png");
  Halide::Runtime::Buffer<float> input(3840, 2160, 1);
  //Halide::Runtime::Buffer<float> input(10240, 4320, 1);
  //Halide::Runtime::Buffer<float> input(10112, 10112, 1);
  Halide::Runtime::Buffer<float> output(input.width() - 2, input.height() - 2, input.channels());
  int error;

  halide_set_num_threads(4);

  output.set_min(1, 1);

//  for(int i = 0; i < 31; ++i) {
    for(int c = 0; c < input.channels(); ++c) {
      for(int y = 0; y < input.height(); ++y) {
        for(int x = 0; x < input.width(); ++x) {
          input(x, y, c) = rand();
        }
      }
    }

    if((error = blur_y(input, output)) != 0) {
      fprintf(stderr, "Halide returned an error: %d\n", error);
    }
//  }

  return 0;
}
