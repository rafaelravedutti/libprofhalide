# libprofhalide: A wrapper library to count performance events on Halide generated kernels

This library works together with the perfctr module in Halide, therefore you need to use our fork from Halide. For now, interfaces for PAPI and Likwid are available, but more interfaces can be implemented easily.

## Cloning and building our fork from Halide

Clone and compile the fork from Halide with the required adjustments. In order to compile it, **llvm-config** must point to a LLVM build that is at least version 10:
```bash
git clone https://github.com/rafaelravedutti/Halide.git
cd Halide
make -j4 # number of jobs to compile in parallel
```

## Cloning and building libprofhalide

Clone the libprofhalide and before building it, set the proper paths for PAPI and Likwid in the Makefile. If any of these is not available it is possible to simply remove the corresponding library from the rule **all**. When done, execute the **make** command.
```bash
git clone https://github.com/rafaelravedutti/libprofhalide.git
cd libprofhalide
# Adjust PAPI and Likwid paths in Makefile
make -j4 # number of jobs to compile in parallel
```

## Using the blur example as a test

To use the blur examples presented in our paper, just cd into **tests/blur** in this repository, and in the **Makefile** set the proper paths for Halide (**HALIDE\_PATH**), Likwid (**LIKWID\_PATH**) and PAPI (**PAPI\_PATH**) --- do not run **make** here, the test script runs it. Include the proper Halide bin path and libprofhalide path in the **source.me** file, and configure the **gen\_tests.sh** script, to finally run it:
```bash
cd tests/blur
# Set proper paths for Halide, Likwid and PAPI in Makefile
# Include Halide bin path and libprofhalide path in source.me
bash gen_tests.sh # Run the tests
```
