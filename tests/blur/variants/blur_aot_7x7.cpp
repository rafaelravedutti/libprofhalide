#include "HalideBuffer.h"
//#include "halide_image_io.h"
#include "blur_aot.h"
/* ... */
#include <stdio.h>

using namespace Halide;
//using namespace Halide::Tools;

#if !defined IMAGE_WIDTH || !defined IMAGE_HEIGHT || !defined IMAGE_CHANNELS
    #error "You must define the image dimensions!"
#endif

int main(int argc, const char **argv) {
    Halide::Runtime::Buffer<float> input(IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_CHANNELS);
    Halide::Runtime::Buffer<float> output(input.width(), input.height(), input.channels());
    int error;

    for(int c = 0; c < input.channels(); ++c) {
        for(int y = 0; y < input.height(); ++y) {
            for(int x = 0; x < input.width(); ++x) {
                input(x, y, c) = rand();
            }
        }
    }

    if((error = blur_x(input, output)) != 0) {
        fprintf(stderr, "Halide returned an error: %d\n", error);
    }

    return 0;
}
