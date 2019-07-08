/* Halide library */
#include "Halide.h"
#include "halide_image_io.h"
/* ... */
#include <iostream>
#include <string>

using namespace Halide;
using namespace Halide::Tools;

using std::vector;
using std::string;

#if !defined SCHEDULE
  #error "You must define a schedule!"
#endif

Var x, y, c, k;

// Downsample with a 1 3 3 1 filter
Func downsample(Func f) {
  using Halide::_;
  Func downx, downy;
  downx(x, y, _) = (f(2*x-1, y, _) + 3.0f * (f(2*x, y, _) + f(2*x+1, y, _)) + f(2*x+2, y, _)) / 8.0f;
  downy(x, y, _) = (downx(x, 2*y-1, _) + 3.0f * (downx(x, 2*y, _) + downx(x, 2*y+1, _)) + downx(x, 2*y+2, _)) / 8.0f;
  return downy;
}

// Upsample using bilinear interpolation
Func upsample(Func f) {
  using Halide::_;
  Func upx, upy;
  upx(x, y, _) = 0.25f * f((x/2) - 1 + 2*(x % 2), y, _) + 0.75f * f(x/2, y, _);
  upy(x, y, _) = 0.25f * upx(x, (y/2) - 1 + 2*(y % 2), _) + 0.75f * upx(x, y/2, _);
  return upy;
}

int main(int argc, const char **argv) {
  Buffer<float> input(3840, 2160, 3);
  //Buffer<float> input(10240, 4320, 3);
  //Buffer<float> input(10112, 10112, 3);
  //Buffer<float> input = Tools::load_and_convert_image("input.png");
  Buffer<float> output(input.width(), input.height(), input.channels());
  string schedule;

  const int maxJ = 10;
  const int J = 8;
  const int levels = 5;
  const float alpha = 1.0;
  const float beta = 1.0;

  Func remap;
  Expr fx = cast<float>(x) / 256.0f;
  remap(x) = alpha*fx*exp(-fx*fx/2.0f);

  // Set a boundary condition
  Func clamped = Halide::BoundaryConditions::repeat_edge(input);

  // Convert to floating point
  Func floating;
  floating(x, y, c) = clamped(x, y, c) / 65535.0f;

  // Get the luminance channel
  Func gray;
  gray(x, y) = 0.299f * floating(x, y, 0) + 0.587f * floating(x, y, 1) + 0.114f * floating(x, y, 2);

  // Make the processed Gaussian pyramid.
  Func gPyramid[maxJ];
  // Do a lookup into a lut with 256 entires per intensity level
  Expr level = k * (1.0f / (levels - 1));
  Expr idx = gray(x, y)*cast<float>(levels-1)*256.0f;
  idx = clamp(cast<int>(idx), 0, (levels-1)*256);
  gPyramid[0](x, y, k) = beta*(gray(x, y) - level) + level + remap(idx - 256*k);
  for (int j = 1; j < J; j++) {
    gPyramid[j](x, y, k) = downsample(gPyramid[j-1])(x, y, k);
  }

  // Get its laplacian pyramid
  Func lPyramid[maxJ];
  lPyramid[J-1](x, y, k) = gPyramid[J-1](x, y, k);
  for (int j = J-2; j >= 0; j--) {
    lPyramid[j](x, y, k) = gPyramid[j](x, y, k) - upsample(gPyramid[j+1])(x, y, k);
  }

  // Make the Gaussian pyramid of the input
  Func inGPyramid[maxJ];
  inGPyramid[0](x, y) = gray(x, y);
  for (int j = 1; j < J; j++) {
    inGPyramid[j](x, y) = downsample(inGPyramid[j-1])(x, y);
  }

  // Make the laplacian pyramid of the output
  Func outLPyramid[maxJ];
  for (int j = 0; j < J; j++) {
    // Split input pyramid value into integer and floating parts
    Expr level = inGPyramid[j](x, y) * cast<float>(levels-1);
    Expr li = clamp(cast<int>(level), 0, levels-2);
    Expr lf = level - cast<float>(li);
    // Linearly interpolate between the nearest processed pyramid levels
    outLPyramid[j](x, y) = (1.0f - lf) * lPyramid[j](x, y, li) + lf * lPyramid[j](x, y, li+1);
  }

  // Make the Gaussian pyramid of the output
  Func outGPyramid[maxJ];
  outGPyramid[J-1](x, y) = outLPyramid[J-1](x, y);
  for (int j = J-2; j >= 0; j--) {
    outGPyramid[j](x, y) = upsample(outGPyramid[j+1])(x, y) + outLPyramid[j](x, y);
  }

  // Reintroduce color (Connelly: use eps to avoid scaling up noise w/ apollo3.png input)
  Func color;
  float eps = 0.01f;
  color(x, y, c) = outGPyramid[0](x, y) * (floating(x, y, c)+eps) / (gray(x, y)+eps);

  // Convert back to 16-bit
  Func out;
  out(x, y, c) = cast<float>(clamp(color(x, y, c), 0.0f, 1.0f) * 65535.0f);

#ifndef PROFILE
    set_profiler_status(false);
#endif

#if SCHEDULE == 1
    /* Serial without vectorization */
    schedule = "serial";

    remap.compute_root();
    Var yo;
    out.reorder(c, x, y).split(y, yo, y, 64).parallel(yo).vectorize(x, 8);
    gray.compute_root().parallel(y, 32).vectorize(x, 8);
    for (int j = 1; j < 5; j++) {
      inGPyramid[j]
        .compute_root().parallel(y, 32).vectorize(x, 8);
      gPyramid[j]
        .compute_root().reorder_storage(x, k, y)
        .reorder(k, y).parallel(y, 8).vectorize(x, 8);
      outGPyramid[j]
        .store_at(out, yo).compute_at(out, y).fold_storage(y, 8)
        .vectorize(x, 8);
    }
    outGPyramid[0].compute_at(out, y).vectorize(x, 8);
    for (int j = 5; j < J; j++) {
      inGPyramid[j].compute_root();
      gPyramid[j].compute_root().parallel(k);
      outGPyramid[j].compute_root();
    }

  #ifdef PROFILE
    for(int j = 0; j < J; ++j) {
      gPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      inGPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      outLPyramid[j].profile(PROFILE_PRODUCTION, true, true);
    }

    out.profile(PROFILE_PRODUCTION, true, true);
  #endif

#elif SCHEDULE == 2
    /* Serial with vectorization */
    schedule = "serial_vec";

    remap.compute_root();
    Var yo;
    out.reorder(c, x, y).split(y, yo, y, 64).parallel(yo).vectorize(x, 8);
    gray.compute_root().parallel(y, 32).vectorize(x, 8);
    for (int j = 1; j < 5; j++) {
      inGPyramid[j]
        .compute_root().parallel(y, 32).vectorize(x, 8);
      gPyramid[j]
        .compute_root().reorder_storage(x, k, y)
        .reorder(k, y).parallel(y, 8).vectorize(x, 8);
      outGPyramid[j]
        .store_at(out, yo).compute_at(out, y).fold_storage(y, 8)
        .vectorize(x, 8);
    }
    outGPyramid[0].compute_at(out, y).vectorize(x, 8);
    for (int j = 5; j < J; j++) {
      inGPyramid[j].compute_root();
      gPyramid[j].compute_root().parallel(k);
      outGPyramid[j].compute_root();
    }

  #ifdef PROFILE
    for(int j = 0; j < J; ++j) {
      gPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      inGPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      outLPyramid[j].profile(PROFILE_PRODUCTION, true, true);
    }

    out.profile(PROFILE_PRODUCTION, true, true);
  #endif

#elif SCHEDULE == 3
    /* Sliding window */
    schedule = "parallel_with_vec";

    remap.compute_root();
    Var yo;
    out.reorder(c, x, y).split(y, yo, y, 64).parallel(yo).vectorize(x, 8);
    gray.compute_root().parallel(y, 32).vectorize(x, 8);
    for (int j = 1; j < 5; j++) {
      inGPyramid[j]
        .compute_root().parallel(y, 32).vectorize(x, 8);
      gPyramid[j]
        .compute_root().reorder_storage(x, k, y)
        .reorder(k, y).parallel(y, 8).vectorize(x, 8);
      outGPyramid[j]
        .store_at(out, yo).compute_at(out, y).fold_storage(y, 8)
        .vectorize(x, 8);
    }
    outGPyramid[0].compute_at(out, y).vectorize(x, 8);
    for (int j = 5; j < J; j++) {
      inGPyramid[j].compute_root();
      gPyramid[j].compute_root().parallel(k);
      outGPyramid[j].compute_root();
    }

  #ifdef PROFILE
    for(int j = 0; j < J; ++j) {
      gPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      inGPyramid[j].profile(PROFILE_PRODUCTION, true, true);
      outLPyramid[j].profile(PROFILE_PRODUCTION, true, true);
    }

    out.profile(PROFILE_PRODUCTION, true, true);
  #endif

#else
    schedule = "invalid";

    #error "Invalid schedule!"

#endif

#ifdef COMPILE_AOT

#ifdef PROFILE
  #ifndef PARALLEL
    out.compile_to_lowered_stmt("html/local_laplacian_" + schedule + "_serial.html", {input}, HTML);
  #else
    out.compile_to_lowered_stmt("html/local_laplacian_" + schedule + "_parallel.html", {input}, HTML);
  #endif
#endif

  out.compile_to_static_library("local_laplacian_aot", {input}, "out");

#else

  out.realize(output);

#endif

  return 0;
}
