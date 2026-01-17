file(GLOB_RECURSE GAME_SOURCES "*.h" "*.c" "*.cpp")

add_executable(game ${GAME_SOURCES})

target_include_directories(game PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(game PRIVATE GIT_DESCRIBE="${GIT_DESCRIBE_VERSION}")

target_link_libraries(game 
	common 
	libgame 
	libpoly 
	libsql 
	libthecore 
	liblua

	# external
	mariadbclient
	cryptopp-static
)

if (WIN32)
    target_link_libraries(game ws2_32)
else()
    target_link_libraries(game pthread)
    if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(game md)
    endif()
endif()
