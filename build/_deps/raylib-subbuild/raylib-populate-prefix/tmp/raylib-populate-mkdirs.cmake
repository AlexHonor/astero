# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Projects/asteroid-rush/build/_deps/raylib-src"
  "C:/Projects/asteroid-rush/build/_deps/raylib-build"
  "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix"
  "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/tmp"
  "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp"
  "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/src"
  "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Projects/asteroid-rush/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
