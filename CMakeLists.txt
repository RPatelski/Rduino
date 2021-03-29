cmake_minimum_required(VERSION 3.1.0)
project(RDUINO VERSION 0.0.0)

#define names and files
set(APP_LIB_NAME rduino)

#include packages
find_package(Threads REQUIRED)
find_library(GPIOD_LIB gpiod REQUIRED)
set(LIBS_TO_LINK Threads::Threads ${GPIOD_LIB})

#generate lists of all source and header files
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

#compile a library
add_library(${APP_LIB_NAME} SHARED ${SOURCES} ${HEADERS})

#include headers and libs
target_include_directories(${APP_LIB_NAME} PRIVATE include)
target_link_libraries(${APP_LIB_NAME} PRIVATE ${LIBS_TO_LINK} )

#set properties
set_target_properties(${APP_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${APP_LIB_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS ${APP_LIB_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

