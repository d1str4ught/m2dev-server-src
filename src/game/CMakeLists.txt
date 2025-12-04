file(GLOB_RECURSE GAME_SOURCES "*.h" "*.c" "*.cpp")

add_executable(game ${GAME_SOURCES})

target_compile_definitions(game PRIVATE GIT_DESCRIBE="${GIT_DESCRIBE_VERSION}")

target_link_libraries(game 
	common 
	libgame 
	libpoly 
	libsql 
	libthecore 
	liblua

	# external
	#mariadbclient
	#cryptopp-static
)

if (USE_SIL AND NOT WIN32)
	target_link_libraries(game ${MARIADB_LIB} ${CRYPTOPP_LIB} ${SSL_LIB} ${CRYPTO_LIB})
else()
	target_link_libraries(game
		mariadbclient
		cryptopp-static
	)
endif()

if (WIN32)
	target_link_libraries(game ws2_32)
else()
	target_link_libraries(game pthread md)
endif()
