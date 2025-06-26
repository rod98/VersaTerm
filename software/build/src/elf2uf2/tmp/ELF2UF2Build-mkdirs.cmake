# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/penguin/Programming/versaterm/VersaTerm/software/lib/pico-sdk/tools/elf2uf2")
  file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/lib/pico-sdk/tools/elf2uf2")
endif()
file(MAKE_DIRECTORY
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/elf2uf2"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/tmp"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/src/ELF2UF2Build-stamp"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/src"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/src/ELF2UF2Build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/src/ELF2UF2Build-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/build/src/elf2uf2/src/ELF2UF2Build-stamp${cfgdir}") # cfgdir has leading slash
endif()
