cmake_minimum_required(VERSION 3.9)
project(GameClientSDL)

set(CMAKE_CXX_STANDARD 11)

add_executable(GameClientSDL main.cpp video/ffmpeg.cpp video/ffmpeg.h)