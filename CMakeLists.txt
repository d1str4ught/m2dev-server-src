cmake_minimum_required(VERSION 3.15)
project(m2dev-server-src)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ASan support
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Custom options
option(USE_LEGDEPS "Use preincluded legacy dependencies (vendor/legacy instead of git submodules installation via depinit)" ON)
option(USE_SIL "Use the system's installed libraries (must exist locally)" OFF)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/buildtool"
)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_install")

if(WIN32)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	add_compile_options(
		$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>
		$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:preprocessor>
	)
	
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
  $<$<PLATFORM_ID:Linux>:OS_LINUX>
  $<$<PLATFORM_ID:Darwin>:OS_MACOS>
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Derive a version string from git for the db target
find_package(Git QUIET)

if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} describe --always --dirty --tags
		OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
endif()

# ====================================================================
# [START] Dependency Initialization & Configuration Logic
# ====================================================================

# --- A. System Installed Libraries (SIL) ---
if (USE_SIL AND NOT WIN32)
	message(STATUS "Configuring to use System Installed Libraries (SIL).")
	
	# 1. Include Paths (Updated to reflect specific directory locations)
	
	# PCG-CPP (Homeless headers, directly in /usr/local/include)
	include_directories("/usr/local/include") 
	
	# Cryptopp
	include_directories("/usr/local/include/cryptopp") 
	
	# MariaDB
	include_directories("/usr/local/include/mysql")
	
	# Spdlog
	include_directories("/usr/local/include/spdlog") 
	
	# STB
	include_directories("/usr/local/include/stb") 

	# 2. Find and Link Libraries (Finds the .a or .so files)
	
	# Cryptopp (Static: libcryptopp.a)
	find_library(CRYPTOPP_LIB NAMES cryptopp HINTS /usr/local/lib)
	
	# MariaDB (Static: libmariadbclient.a)
	find_library(MARIADB_LIB NAMES mariadbclient HINTS /usr/local/lib/mysql) # Explicit path due to nesting
	
	# Spdlog (Dynamic: libspdlog.so)
	find_library(SPDLOG_LIB NAMES spdlog HINTS /usr/local/lib)

	# Dependency libs for MariaDB
	find_library(SSL_LIB NAMES ssl HINTS /usr/local/lib)
	find_library(CRYPTO_LIB NAMES crypto HINTS /usr/local/lib)
	find_library(ZLIB_LIB NAMES z zlib HINTS /usr/local/lib)
	find_library(FMT_LIB NAMES fmt HINTS /usr/local/lib)

	# 3. Aggregate all found system libraries for later linking
	set(SYSTEM_LIBS 
		${CRYPTOPP_LIB} 	# cd /usr/ports/security/cryptopp && make install clean
		${MARIADB_LIB} 	# cd /usr/ports/databases/mariadbXX-client (or mariadb-connector-c) && make install clean
		${SPDLOG_LIB}	# cd /usr/ports/devel/spdlog && make install clean
		
		# --- ADDED NECESSARY LIBS ---
		${SSL_LIB} 	# cd /usr/ports/security/openssl && make install clean
		${CRYPTO_LIB} 	# cd /usr/ports/security/openssl && make install clean
		${ZLIB_LIB} 	# cd /usr/ports/archivers/libzip && make install clean
		${FMT_LIB} 	# cd /usr/ports/devel/libfmt && make install clean
		# You may need to add system dependencies (e.g., ZLIB, PTHREAD, etc.) here!
	)
	
	if (NOT CRYPTOPP_LIB OR NOT MARIADB_LIB OR NOT SPDLOG_LIB OR 
		NOT SSL_LIB OR NOT CRYPTO_LIB OR NOT ZLIB_LIB OR NOT FMT_LIB)
		
		message(STATUS "--- Mandatory Library Check FAILED ---")
		message(STATUS "CRYPTOPP_LIB: ${CRYPTOPP_LIB}")
		message(STATUS "MARIADB_LIB:  ${MARIADB_LIB}")
		message(STATUS "SPDLOG_LIB:   ${SPDLOG_LIB}")
		message(STATUS "SSL_LIB:      ${SSL_LIB}")
		message(STATUS "CRYPTO_LIB:   ${CRYPTO_LIB}")
		message(STATUS "ZLIB_LIB:     ${ZLIB_LIB}")
		message(STATUS "FMT_LIB:      ${FMT_LIB}")
		message(STATUS "------------------------------------")
		
		# Check again and terminate only if a critical library is missing
		if(NOT CRYPTOPP_LIB OR NOT MARIADB_LIB OR NOT SPDLOG_LIB OR NOT SSL_LIB OR NOT CRYPTO_LIB)
			message(FATAL_ERROR "FATAL: One of the five core libraries (Cryptopp, MariaDB, Spdlog, SSL, Crypto) was not found.")
		endif()
		
	endif()
# --- B. Vendor Dependencies (LEGACY or MODERN) ---
else()
	if (NOT USE_LEGDEPS)
		if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
			# 1. Locate the Python interpreter (Required to run depinit.py)
			find_package(Python3 REQUIRED COMPONENTS Interpreter)

			# 2. Execute the dependency initialization script
			message(STATUS "Running server dependency initialization script (depinit.py)...")

			execute_process(
				COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/depinit.py"
				# Ensure execution happens in the project root for Git commands
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" 
				RESULT_VARIABLE DEP_INIT_RESULT
				OUTPUT_VARIABLE DEP_INIT_OUTPUT
				ERROR_VARIABLE DEP_INIT_OUTPUT
			)

			# 3. Print the script output
			message(STATUS "${DEP_INIT_OUTPUT}")

			# 4. Check the return code and halt if the script failed
			if (DEP_INIT_RESULT)
				message(FATAL_ERROR "Server dependency initialization script failed! See messages above.")
			endif()

			message(STATUS "All server dependencies are ready.")
		endif()
	endif()
endif()

# ====================================================================
# [END] Dependency Initialization & Configuration Logic
# ====================================================================

if(NOT GIT_DESCRIBE_VERSION)
	if(GIT_FOUND)
		execute_process(
			COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
			OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
	else()
		set(GIT_DESCRIBE_VERSION "unknown")
	endif()
endif()

if(NOT GIT_DESCRIBE_VERSION)
	set(GIT_DESCRIBE_VERSION "unknown")
endif()

# ASan flags
if(ENABLE_ASAN)
	if(MSVC)
		add_compile_options(/fsanitize=address)
		add_link_options(/fsanitize=address)
		add_definitions(-D_DISABLE_VECTOR_ANNOTATION)
		add_definitions(-D_DISABLE_STRING_ANNOTATION)
	else()
		add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
		add_link_options(-fsanitize=address)
	endif()
endif()

add_definitions(-DNOMINMAX)
add_definitions(-DWIN32_LEAN_AND_MEAN)

include_directories(src)

if (NOT USE_SIL)
	if (USE_LEGDEPS)
		include_directories("include/legacy")
	else()
		include_directories("include/latest")
	endif()

	# MariDB includes
	if (NOT USE_LEGDEPS)
		include_directories(${CMAKE_BINARY_DIR}/vendor/latest/mariadb-connector-c/include)
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/latest/mariadb-connector-c/include)
	else()
		include_directories(${CMAKE_BINARY_DIR}/vendor/legacy/mariadb-connector-c-3.4.5/include)
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/legacy/mariadb-connector-c-3.4.5/include)
	endif()

	# For MSVC we must ensure Crypto++ is built as static-only and avoid its asm duplicates.
	# This forces the vendor/cryptopp subproject to:
	#  - disable asm (avoids x64dll.asm duplicate symbols on MSVC toolchains),
	#  - build only the static library (prevents building a separate shared cryptopp lib).
	if (NOT USE_LEGDEPS)
		if (WIN32 AND MSVC)
		  # Disable Crypto++ assembly on MSVC (avoids duplicate symbol issues)
		  set(DISABLE_ASM ON CACHE BOOL "Disable Crypto++ ASM on MSVC to avoid duplicate symbols" FORCE)

		  # Force Crypto++ to build static-only (no cryptopp shared target)
		  set(BUILD_SHARED OFF CACHE BOOL "Force Crypto++ shared build OFF on MSVC" FORCE)
		  set(BUILD_STATIC ON CACHE BOOL "Force Crypto++ static build ON on MSVC" FORCE)
		endif()
	endif()
endif()

add_subdirectory(src)

if (NOT USE_SIL)
	if (USE_LEGDEPS)
		add_subdirectory("vendor/legacy")
	else()
		add_subdirectory("vendor/latest")
	endif()
endif()
