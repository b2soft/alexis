cmake_minimum_required(VERSION 3.17)

# Project name
project(Game)

# Set Project standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(../Libs libs)

# Add the executable
add_executable(Game main.cpp)

target_link_libraries(Game PRIVATE alexis)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)

install(TARGETS Game CONFIGURATIONS Debug RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}../../Bin/Debug)
install(TARGETS alexis CONFIGURATIONS Debug RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}../../Bin/Debug)

install(TARGETS Game CONFIGURATIONS Release RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}../../Bin/Release)
install(TARGETS alexis CONFIGURATIONS Release RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}../../Bin/Release)