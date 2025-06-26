# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/penguin/Programming/versaterm/VersaTerm/software/lib/pico-sdk/tools/pioasm")
  file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/lib/pico-sdk/tools/pioasm")
endif()
file(MAKE_DIRECTORY
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/pioasm"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/tmp"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/src/PioasmBuild-stamp"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/src"
  "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/penguin/Programming/versaterm/VersaTerm/software/build/lib/PicoDVI/software/libdvi/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
