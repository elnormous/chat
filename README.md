# Chat

Simple chat client and server.

# Installation

[Boost C++ libraries](https://www.boost.org/) are needed to compile the project. Before compiling the project you have to initialize git submodules:

```shell
git submodule update --init
```

The project can be built with Xcode or generated with CMake. To generate a Makefile with CMake, execute the following commands in your shell:

```shell
mkdir build
cd build
cmake ..
make
```