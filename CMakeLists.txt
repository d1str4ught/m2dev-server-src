cmake_minimum_required(VERSION 3.15)
project(m2dev-server-src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/buildtool"
)

if(WIN32)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	
	if (MSVC)
		add_compile_options(/MP)
	endif()
else()
	include_directories(${MARIADB_INCLUDE_DIR})
	
	add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-finput-charset=UTF-8>")
endif()

add_compile_definitions(
  $<$<PLATFORM_ID:Windows>:OS_WINDOWS>
  $<$<PLATFORM_ID:FreeBSD>:OS_FREEBSD>
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DNOMINMAX)

include_directories(src)
include_directories(include)

add_subdirectory(src)
