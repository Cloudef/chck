# Chck
[![buildhck status](http://build.cloudef.pw/build/chck/master/linux x86_64/current/status.svg)](http://build.cloudef.pw/build/chck/master/linux%20x86_64)

Collection of C utilities taken and cleaned up from my other projects

## Building

    mkdir target && cd target                # - create build target directory
    cmake -DCMAKE_BUILD_TYPE=Upstream ..     # - run CMake
    make                                     # - compile

## Running tests

    cd target                                # - cd to your target directory
    make test                                # - run tests

## Installing

The utilites are small and mainly meant to be copied to your project.
However libraries for each component is built with the cmake.

## Some rules what goes here

* Each utility should be independant.
   * Though, it's ok to include macros.h and overflow.h
* Each utility should contain tests.
* Each utility should have use more than once.

## License

zlib, see LICENSE file for more information
