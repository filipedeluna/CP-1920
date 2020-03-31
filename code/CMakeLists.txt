cmake_minimum_required(VERSION 3.15)
project(main C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=gnu11")
set(CMAKE_C_FLAGS_DEBUG "-g")

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_C_FLAGS_RELEASE "-O3")

find_package(OpenMP)

include_directories(.)

add_executable(main
        src/args.c
        src/args.h
        src/debug.c
        src/debug.h
        src/main.c
        src/patterns.c
        src/patterns.h
        src/unit.c
        src/unit.h)

target_link_libraries(main PUBLIC OpenMP::OpenMP_C)
