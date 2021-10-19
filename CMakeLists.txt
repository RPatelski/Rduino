cmake_minimum_required(VERSION 3.1.0)
project(RDUINO VERSION 0.0.1)

#define names and files
set(APP_LIB_NAME Rduino)

#include packages
find_package(Threads REQUIRED)
find_library(GPIOD_LIB gpiod REQUIRED)
set(LIBS_TO_LINK Threads::Threads ${GPIOD_LIB})

set(INCLUDE_DEST "include/rduino")
set(LIBRARY_DEST "lib/rduino")

#generate lists of all source and header files
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

#compile a library
add_library(${APP_LIB_NAME} SHARED ${SOURCES} ${HEADERS})

#include headers and libs
target_include_directories(${APP_LIB_NAME} PUBLIC 
$<BUILD_INTERFACE:${MY_LIBRARY_SOURCE_DIR}/include>
$<INSTALL_INTERFACE:${include_dest}>)

target_link_libraries(${APP_LIB_NAME} PRIVATE ${LIBS_TO_LINK})

#set properties
#set_target_properties(${APP_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
#set_target_properties(${APP_LIB_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS ${APP_LIB_NAME} DESTINATION ${LIBRARY_DEST})
install(FILES ${HEADERS} DESTINATION ${INCLUDE_DEST})

install(TARGETS ${APP_LIB_NAME} EXPORT ${APP_LIB_NAME} DESTINATION ${LIBRARY_DEST})
install(EXPORT ${APP_LIB_NAME} NAMESPACE ${APP_LIB_NAME}:: DESTINATION ${LIBRARY_DEST})
install(FILES rduino-config.cmake DESTINATION ${LIBRARY_DEST})


