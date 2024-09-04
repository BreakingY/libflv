cmake_minimum_required(VERSION 3.0)
project (FLVPROJ)

include_directories(./include ./source)
aux_source_directory(./source SRC)

link_directories(/usr/local/lib /usr/lib)
set(EXECUTABLE_OUTPUT_PATH ./)
add_compile_options(-g) 
set(CMAKE_BUILD_TYPE Debug) # Release
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./)

add_library(flv SHARED ${SRC})
# target_link_libraries(flv pthread)

# test
add_executable(flv_test ./test/flv_test.c)
target_link_libraries(flv_test flv)