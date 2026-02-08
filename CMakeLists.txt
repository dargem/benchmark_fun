# To build
# cmake -S . -B build
# cmake --build build
# run it from inside ./build/etc

cmake_minimum_required(VERSION 3.16)

project(benchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
list(APPEND SOURCES "main.cpp")

set(ALL_DIRS "")
foreach(FILE ${SOURCES})
    get_filename_component(DIR ${FILE} DIRECTORY)
    list(APPEND ALL_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES ALL_DIRS)

add_executable(benchmarks
    ${SOURCES}
)

target_include_directories(benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ALL_DIRS}
)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED)
target_link_libraries(benchmarks PRIVATE Boost::boost)

target_compile_features(benchmarks PRIVATE cxx_std_20)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(benchmarks PRIVATE
        -O3
        -march=native
        -mtune=native
        -funroll-loops
    )

    target_link_options(benchmarks PRIVATE
        -flto
    )
endif()
