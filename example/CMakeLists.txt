cmake_minimum_required(VERSION 3.1.0)
project(RDUINO_EXAMPLES VERSION 0.0.0)

#define names and files
set(APP_EXE_NAME rduino_gpio)

#include packages
find_library(RDUINO_LIB rduino REQUIRED)
set(LIBS_TO_LINK ${RDUINO_LIB})

#generate lists of all source and header files
file(GLOB HEADERS "include/*.h")

#compile a library
add_executable(${APP_EXE_NAME} gpio_test.c ${HEADERS})

#include headers and libs
target_include_directories(${APP_EXE_NAME} PUBLIC include)
target_link_libraries(${APP_EXE_NAME} PRIVATE ${LIBS_TO_LINK})

