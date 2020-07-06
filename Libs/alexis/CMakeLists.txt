cmake_minimum_required(VERSION 3.17)

# Project name
project(alexis)

add_library(alexis SHARED test.cpp "Precompiled.h" "Precompiled.cpp" "Utils/Timer.h" "Utils/Timer.cpp" "Core/Application.h" "Core/Application.cpp" "Utils/D3DUtils.h")
target_link_libraries(alexis PRIVATE "d3d12.lib" "dxgi.lib")

target_precompile_headers(alexis PUBLIC "Precompiled.h")
target_include_directories(alexis PUBLIC ./)