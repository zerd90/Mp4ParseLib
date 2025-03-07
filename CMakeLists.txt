# 最低CMake版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Mp4ParseLib)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
else()
	set(INCLUDE_DIRECTORIES
		${INCLUDE_DIRECTORIES}
		${PROJECT_SOURCE_DIR}/include
		PARENT_SCOPE
	)
endif()

option(BUILD_SAMPLES "Build MP4 Parse Sample Programs" OFF)

file(GLOB SRC_LIST
	"src/*.cpp"
)

if(WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# 头文件路径
include_directories(
	${INCLUDE_DIRECTORIES}
	"include"
)

add_library(${PROJECT_NAME} ${SRC_LIST})

if(BUILD_SAMPLES)
	add_subdirectory(samples)
endif()