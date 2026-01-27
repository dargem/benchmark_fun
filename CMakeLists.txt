cmake_minimum_required(VERSION 3.16)

project(prefetcher_benchmarks LANGUAGES CXX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
list(APPEND SOURCES "main.cpp")

set(ALL_DIRS "")
foreach(FILE ${SOURCES})
    get_filename_component(DIR ${FILE} DIRECTORY)
    list(APPEND ALL_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES ALL_DIRS)

add_executable(prefetcher_benchmarks
    ${SOURCES}
)

target_include_directories(prefetcher_benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ALL_DIRS}
)

target_compile_features(prefetcher_benchmarks PRIVATE cxx_std_17)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(prefetcher_benchmarks PRIVATE
        -O3
        -march=native
        -mtune=native
        -funroll-loops
    )

    target_link_options(prefetcher_benchmarks PRIVATE
        -flto
    )
endif()
