cmake_minimum_required(VERSION 3.20)
project(AsteroidRush VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/FetchRaylib.cmake)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(asteroid_rush ${SOURCES})

target_include_directories(asteroid_rush PRIVATE src)
target_link_libraries(asteroid_rush PRIVATE raylib)

if(WIN32)
    target_link_libraries(asteroid_rush PRIVATE winmm)
endif()
