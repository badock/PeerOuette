cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(GameClientSDL)
set(CMAKE_CXX_STANDARD 14)

find_package(Protobuf CONFIG REQUIRED)

find_package(gRPC CONFIG REQUIRED)

message(STATUS "Using protobuf ${protobuf_VERSION}")
message(STATUS "Using gRPC ${gRPC_VERSION}")

##########################################################
# Compile protos/route_guide.proto file
##########################################################

# Proto file
get_filename_component(hw_proto "${CMAKE_CURRENT_SOURCE_DIR}/protos/route_guide.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/protos/route_guide.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/protos/route_guide.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_SOURCE_DIR}/protos/route_guide.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/protos/route_guide.grpc.pb.h")

find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin)
find_program(PROTOC_PATH NAMES protoc)

add_custom_command(
		OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
		COMMAND ${PROTOC_PATH}
		ARGS --grpc_out=${CMAKE_CURRENT_SOURCE_DIR}/protos
		--cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/protos
		-I ${CMAKE_CURRENT_SOURCE_DIR}/protos
		--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
		${hw_proto}
		DEPENDS "${hw_proto}"
)

##########################################################
# Compile Peerouette
##########################################################

if(WIN32)
    add_executable(GameClientSDL
			src/bin/peerouette.cpp
            src/streaming/streaming.cpp
            src/streaming/streaming.h
            src/log/log.cpp
            src/log/log.h
            src/queue/queue.cpp
            src/queue/queue.h
            src/dxcapture/capture.cpp
            src/dxcapture/capture.h
            protos/route_guide.grpc.pb.cc
            protos/route_guide.grpc.pb.h
            protos/route_guide.pb.cc
            protos/route_guide.pb.h
			src/network/network.cpp
			src/network/network.h
            src/codec/codec.cpp
            src/codec/codec.h
			src/extractors/extractors.cpp
			src/extractors/extractors.h
			src/output/output.cpp
			src/output/output.h
            src/codec/codec.cpp
            src/codec/codec.h
            src/texture_converter/TextureConverter.cpp
            src/texture_converter/TextureConverter.h
			src/texture_converter/shaders/Pixel.hlsl
			src/texture_converter/shaders/Vertex.hlsl
			src/texture_converter/shaders/RGBtoYUV.hlsl src/inputs/inputs.cpp src/inputs/inputs.h)

			set(PIXELSHADER_FILES
			  src/texture_converter/shaders/Pixel.hlsl
			  src/texture_converter/shaders/RGBtoYUV.hlsl
			)

			set(VERTEXSHADER_FILES
			  src/I420effect/I420Effect_VS.hlsl
			  src/texture_converter/shaders/Vertex.hlsl
			)

			set(CONTENT_FILES ${PIXELSHADER_FILES} ${VERTEXSHADER_FILES})


else()
    add_executable(GameClientSDL
			src/bin/peerouette.cpp
            src/streaming/streaming.cpp
            src/streaming/streaming.h
            src/log/log.cpp
            src/log/log.h
			protos/route_guide.grpc.pb.cc
			protos/route_guide.grpc.pb.h
			protos/route_guide.pb.cc
			protos/route_guide.pb.h
			src/network/network.cpp
			src/network/network.h
			src/extractors/extractors.cpp
			src/extractors/extractors.h
			src/output/output.cpp
			src/output/output.h
            src/codec/codec.cpp
            src/codec/codec.h
            src/queue/queue.cpp
            src/queue/queue.h src/extractors/extractors.cpp src/extractors/extractors.h src/output/output.cpp src/output/output.h src/inputs/inputs.cpp src/inputs/inputs.h)
endif()
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

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

include_directories(".")
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIR})

if(WIN32)
    find_package (DirectX)
    if(DirectX11_FOUND)
        message("Found DirectX libraries")
        message("DirectX11_INCLUDE_DIRS = ${DirectX11_INCLUDE_DIRS} ")
        message("DirectX11_LIBRARIES = ${DirectX11_LIBRARIES} ")
    else()
        message(FATAL_ERROR "DirectX11 not found")
    endif()
    INCLUDE_DIRECTORIES(${DirectX11_INCLUDE_DIRS})
endif()

if(WIN32)

	set(RESOURCE_FILES
	  ${CONTENT_FILES} ${DEBUG_CONTENT_FILES} ${RELEASE_CONTENT_FILES}
	#  Direct3DApp1/Direct3DApp1_TemporaryKey.pfx)
	)
	set_property(SOURCE ${CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
	set_property(SOURCE ${DEBUG_CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT $<CONFIG:Debug>)
	set_property(SOURCE ${RELEASE_CONTENT_FILES} PROPERTY
	  VS_DEPLOYMENT_CONTENT $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>)

	set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_TYPE Pixel)
	set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_ENTRYPOINT main)
	set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_VARIABLE_NAME "%(Filename)_PX_main")
	set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_MODEL 4.0_level_9_3)
	set_property(SOURCE ${PIXELSHADER_FILES} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "$(OutDir)%(Filename).h")

	set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_TYPE Vertex)
	set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_ENTRYPOINT main)
	set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_VARIABLE_NAME "%(Filename)_VX_main")
	set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_MODEL 4.0_level_9_3)
	set_property(SOURCE ${VERTEXSHADER_FILES} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "$(OutDir)%(Filename).h")

	source_group("Source Files" FILES ${SOURCE_FILES})
	source_group("Header Files" FILES ${HEADER_FILES})
	source_group("Resource Files" FILES ${RESOURCE_FILES})
    message(" HEADER: ${HEADER_FILES}")

endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    add_definitions(-D_WIN32_WINNT=0x600)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${SDL2_LIBRARY} ${DirectX11_LIBRARIES} gRPC::grpc gRPC::grpc++_unsecure protobuf::libprotobuf)
else()
	if(APPLE)
		target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${SDL2_LIBRARY} gRPC::grpc gRPC::grpc++_unsecure protobuf::libprotobuf "-framework AudioToolbox" "-framework AudioToolbox")
	elseif(UNIX)
		target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${SDL2_LIBRARY} gRPC::grpc gRPC::grpc++_unsecure protobuf::libprotobuf)
	endif()
endif()

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

#add_dependencies(GameClientSDL CopyGeneratedFiles)
