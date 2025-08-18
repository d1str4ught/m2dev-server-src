cmake_minimum_required(VERSION 3.15)
project(m2dev-server-src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/buildtool"
)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_install")

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

# MariDB includes
include_directories(${CMAKE_BINARY_DIR}/vendor/mariadb-connector-c-3.4.5/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/mariadb-connector-c-3.4.5/include)

add_subdirectory(src)

## vendor setup

## MariaDB-Connector-C
set(INSTALL_PLUGINDIR "${CMAKE_BINARY_DIR}/mariadb_plugins")
set(WITH_CURL OFF CACHE BOOL "")
set(WITH_UNIT_TESTS OFF CACHE BOOL "")
set(CLIENT_PLUGIN_DIALOG STATIC CACHE STRING "")
set(CLIENT_PLUGIN_CLIENT_ED25519 STATIC CACHE STRING "")
set(CLIENT_PLUGIN_CACHING_SHA2_PASSWORD STATIC CACHE STRING "")
set(CLIENT_PLUGIN_SHA256_PASSWORD STATIC CACHE STRING "")
set(CLIENT_PLUGIN_PARSE_CLIENT STATIC CACHE STRING "")
set(CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT OFF CACHE STRING "")
set(CLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD OFF CACHE STRING "")
set(CLIENT_PLUGIN_PVIO_SHMEM STATIC CACHE STRING "")

add_subdirectory(vendor)
