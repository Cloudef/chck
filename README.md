# Chck

Collection of C utilities taken and cleaned up from my other projects

## Building

   mkdir target &&  cd target
   cmake -DCMAKE_INSTALL_PREFIX=build ..
   make

## Running tests

   cd target
   make test

## Installing

   No systemwide install yet provided.
   The utilites are small and mainly meant to be copied to your project.
   However library containing all the utilities is built with the cmake.

## Some rules what goes here

   1. Each utility should be independant.
   2. Each utility should contain tests.
   3. Each utility should have use more than once.

## License

   zlib, see LICENSE file for more information
