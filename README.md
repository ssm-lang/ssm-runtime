# The Sparse Synchronous Model Runtime

The Sparse Synchronous Model (SSM) is a deterministic real-time execution technique that allows explicit, precise timing control.

## Documentation

The API documentation for this library maybe found [here](http://ssm-lang.github.io/ssm-runtime).

The operation of this library was first described in:

> Stephen A. Edwards and John Hui.
> The Sparse Synchronous Model.
> In Forum on Specification and Design Languages (FDL),
> Kiel, Germany, September 2020.
> http://www.cs.columbia.edu/~sedwards/papers/edwards2020sparse.pdf

## Dependencies

The core runtime itself is written in C99, and does not have any dependencies.
This includes includes the scheduler and memory manager.

Platform-specific bindings are provided via the [PlatformIO Core (CLI)](https://platformio.org) toolchain manager;
see their [installation instructions](https://docs.platformio.org/en/latest/core/installation.html).
On Linux, make sure to install the `99-platformio-udev.rules`.

## Examples

See the [examples](examples/) directory.
