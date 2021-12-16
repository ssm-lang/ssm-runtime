# The Sparse Synchronous Model Runtime

![tests](https://github.com/ssm-lang/ssm-runtime/actions/workflows/test.yml/badge.svg?branch=dev)
[![doc](https://github.com/ssm-lang/ssm-runtime/actions/workflows/doc.yml/badge.svg?branch=dev)](https://ssm-lang.github.io/ssm-runtime)
[![codecov](https://codecov.io/gh/ssm-lang/ssm-runtime/branch/dev/graph/badge.svg?token=TYCPPY7Y92)](https://codecov.io/gh/ssm-lang/ssm-runtime)

The Sparse Synchronous Model (SSM) is a deterministic real-time execution technique that allows explicit, precise timing control.

## Documentation

The operation of this library was first described in:

> Stephen A. Edwards and John Hui.
> The Sparse Synchronous Model.
> In Forum on Specification and Design Languages (FDL),
> Kiel, Germany, September 2020.
> http://www.cs.columbia.edu/~sedwards/papers/edwards2020sparse.pdf

The generated documentation for this library maybe found [here](https://ssm-lang.github.io/ssm-runtime).

## Dependencies

The core runtime, which includes the scheduler and memory manager, is written in C99, without library dependencies.
The top-level Makefile is used to compile and test this core library, and has the following dependencies:

-   Some C99-compatible compiler (`gcc` required for testing)
-   [GNU make](https://www.gnu.org/software/make/manual/html_node/index.html)
-   [GNU Bash](https://www.gnu.org/software/bash/) (required for testing)
-   [Doxygen](https://www.doxygen.nl/index.html) (required for building documentation)
-   [Graphviz](http://graphviz.org/) (required for building documentation)
-   [valgrind](https://valgrind.org/) (required for testing)
-   [gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) (required for testing)

On Ubuntu, Debian, and other Linux distributions that use aptitude, these can be installed using:

```shell
sudo apt install build-essential gcc doxygen graphviz valgrind gcov
```

Platform-specific bindings are provided via the [PlatformIO Core (CLI)](https://platformio.org) toolchain manager;
see their [installation instructions](https://docs.platformio.org/en/latest/core/installation.html).
On Linux, make sure to install the `99-platformio-udev.rules`.

Note that PlatformIO _does not use the top-level Makefile_ to build the SSM runtime.
Instead, it uses the top-level library.json manifest to compile this library as a PlatformIO package.

## Quickstart

The top-level `Makefile` can be used to build the platform-generic library alone, without platform-specific bindings.
To build just the library, just run:

```shell
make # `lib' is the default target
```

All build artifacts, including `libssm.a`, are placed in the `build` directory.

To run the included test suite, run the included test script:

```shell
./runtests.sh
```

To build and run any individual example in the `examples/` directory, e.g., `examples/fib.c`:

```shell
make build/fib
./build/fib 5
```
