cmake_minimum_required(VERSION 3.18)

project(S5P)
set(CMAKE_CXX_STANDARD 14)

# work for gdb
option(DEBUG_MODE "Enable debug mode" OFF)

if(DEBUG_MODE)
    message("Debug mode is enabled.")
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2 -ggdb")
else()
    message("Debug mode is disabled.")
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
FIND_PACKAGE(OpenMP REQUIRED)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(EXECUTABLE_OUTPUT_PATH "./bin/")


file(GLOB SOURCES "*.cpp")
add_executable(main ${SOURCES})

target_link_libraries(main PRIVATE Threads::Threads)
