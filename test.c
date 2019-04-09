#include "perf.h"

#define N   7000

int main(int argc, const char *argv[]) {
  float A[N], B[N], C[N];
  unsigned int i;

  for(i = 0; i < N; ++i) {
    B[i] = (float)(i * 2);
    C[i] = (float)(i * 3);
  }

  perf_descriptor_start(0);

  for(i = 0; i < N; ++i) {
    A[i] = B[i] + C[i];
  }

  perf_descriptor_stop(0);
}
