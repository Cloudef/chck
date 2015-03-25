# Chck
[![buildhck status](http://cloudef.eu:9001/build/chck/master/linux%20x86_64/build-status.png)](#)

Collection of C utilities taken and cleaned up from my other projects

## Building

    mkdir target && cd target                # - create build target directory
    cmake -DCMAKE_INSTALL_PREFIX=build ..    # - run CMake, set install directory
    make                                     # - compile

## Running tests

    cd target                                # - cd to your target directory
    make test                                # - run tests

## Installing

No systemwide install yet provided.

The utilites are small and mainly meant to be copied to your project.
However library containing all the utilities is built with the cmake.

## Some rules what goes here
* Each utility should be independant.
   * Though, it's ok to include macros.h and overflow.h
* Each utility should contain tests.
* Each utility should have use more than once.

## License

zlib, see LICENSE file for more information
