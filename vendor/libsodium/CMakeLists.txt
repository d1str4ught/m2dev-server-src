# libsodium CMake wrapper
# This file should be placed in vendor/libsodium/ alongside the libsodium source
# After running: git submodule add https://github.com/jedisct1/libsodium.git libsodium

cmake_minimum_required(VERSION 3.16)

# Check if libsodium source exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/libsodium/include/sodium.h")
    message(FATAL_ERROR
        "libsodium source not found. Please run:\n"
        "  cd vendor/libsodium\n"
        "  git submodule add https://github.com/jedisct1/libsodium.git .\n"
        "Or download libsodium and extract to vendor/libsodium/"
    )
endif()

project(sodium C)

set(SODIUM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/libsodium")

# Generate version.h from version.h.in
set(VERSION "1.0.20")
set(SODIUM_LIBRARY_VERSION_MAJOR 26)
set(SODIUM_LIBRARY_VERSION_MINOR 2)
set(SODIUM_LIBRARY_MINIMAL_DEF "")
configure_file(
    "${SODIUM_SRC_DIR}/include/sodium/version.h.in"
    "${SODIUM_SRC_DIR}/include/sodium/version.h"
)

# Collect all source files
file(GLOB_RECURSE SODIUM_SOURCES
    "${SODIUM_SRC_DIR}/crypto_aead/*.c"
    "${SODIUM_SRC_DIR}/crypto_auth/*.c"
    "${SODIUM_SRC_DIR}/crypto_box/*.c"
    "${SODIUM_SRC_DIR}/crypto_core/*.c"
    "${SODIUM_SRC_DIR}/crypto_generichash/*.c"
    "${SODIUM_SRC_DIR}/crypto_hash/*.c"
    "${SODIUM_SRC_DIR}/crypto_kdf/*.c"
    "${SODIUM_SRC_DIR}/crypto_kx/*.c"
    "${SODIUM_SRC_DIR}/crypto_onetimeauth/*.c"
    "${SODIUM_SRC_DIR}/crypto_pwhash/*.c"
    "${SODIUM_SRC_DIR}/crypto_scalarmult/*.c"
    "${SODIUM_SRC_DIR}/crypto_secretbox/*.c"
    "${SODIUM_SRC_DIR}/crypto_secretstream/*.c"
    "${SODIUM_SRC_DIR}/crypto_shorthash/*.c"
    "${SODIUM_SRC_DIR}/crypto_sign/*.c"
    "${SODIUM_SRC_DIR}/crypto_stream/*.c"
    "${SODIUM_SRC_DIR}/crypto_verify/*.c"
    "${SODIUM_SRC_DIR}/randombytes/*.c"
    "${SODIUM_SRC_DIR}/sodium/*.c"
)

# Create static library
add_library(sodium STATIC ${SODIUM_SOURCES})

# Include directories
target_include_directories(sodium PUBLIC
    "${SODIUM_SRC_DIR}/include"
    "${SODIUM_SRC_DIR}/include/sodium"
)

target_include_directories(sodium PRIVATE
    "${SODIUM_SRC_DIR}"
    "${SODIUM_SRC_DIR}/crypto_onetimeauth/poly1305"
    "${SODIUM_SRC_DIR}/crypto_onetimeauth/poly1305/donna"
    "${SODIUM_SRC_DIR}/crypto_stream/chacha20"
    "${SODIUM_SRC_DIR}/crypto_stream/chacha20/ref"
    "${SODIUM_SRC_DIR}/crypto_stream/salsa20"
    "${SODIUM_SRC_DIR}/crypto_stream/salsa20/ref"
)

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(sodium PRIVATE
        SODIUM_STATIC
        SODIUM_EXPORT=
        _CRT_SECURE_NO_WARNINGS
        inline=__inline
    )
else()
    target_compile_definitions(sodium PRIVATE
        SODIUM_STATIC
        SODIUM_EXPORT=
        HAVE_INLINE
    )
endif()

# Export for consumers
target_compile_definitions(sodium PUBLIC SODIUM_STATIC)
