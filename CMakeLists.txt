cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(mcc)

# add_compile_options(-Wall -Wextra -Wpedantic -ansi)

#For debugging
 add_compile_options(-Wall -Wextra -Wpedantic -ansi -ggdb3 -fsanitize=address)
 add_link_options(-fsanitize=address)

file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(mcc "${Sources}")

target_link_libraries(mcc -lraylib -lGL -lm -lpthread -ldl -lrt -lX11)

