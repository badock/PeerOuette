cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(GameClientSDL)
set(CMAKE_CXX_STANDARD 11)

add_executable(GameClientSDL
        main.c
        src/video/ffmpeg.cpp
        src/video/ffmpeg.h
        src/log/log.c
        src/log/log.h
        src/queue/queue.c
        src/queue/queue.h)

INCLUDE(FindPkgConfig)

if(WIN32)
  set(SDL2_DIR deps/sdl2/build/native)
  find_package(SDL2 REQUIRED)
else()
  #PKG_SEARCH_MODULE(SDL REQUIRED sdl)
endif()

find_package(SDL2 REQUIRED)

find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)
if(FFMPEG_FOUND)
    message("Found FFMPEG/LibAV libraries")
    message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIR} ")
    message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
else()
    message(FATAL_ERROR "FFMPEG not found")
endif()

include_directories(".")
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${SDL2_LIBRARY})

if(WIN32)
add_custom_target(DLLCopyBinaries
	# todo: check if debug and release folder exist
	# debug version
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/sdl2.redist/build/native/bin/Win32/dynamic/SDL2.dll ${CMAKE_BINARY_DIR}/Debug
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg.Nightly/build/native/bin/Win32/avcodec-58.dll ${CMAKE_BINARY_DIR}/Debug
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg.Nightly/build/native/bin/Win32/swscale-5.dll ${CMAKE_BINARY_DIR}/Debug
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg.Nightly/build/native/bin/Win32/avformat-58.dll ${CMAKE_BINARY_DIR}/Debug
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg.Nightly/build/native/bin/Win32/avutil-56.dll ${CMAKE_BINARY_DIR}/Debug
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg.Nightly/build/native/bin/Win32/swresample-3.dll ${CMAKE_BINARY_DIR}/Debug

)
add_dependencies(GameClientSDL DLLCopyBinaries)
endif()

