cmake_minimum_required(VERSION 3.17)

# Project name
project(alexis)

add_library(alexis SHARED test.cpp)

target_include_directories(alexis PUBLIC ./)