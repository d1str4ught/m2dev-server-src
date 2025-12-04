file(GLOB_RECURSE DB_SOURCES "*.h" "*.cpp")

add_executable(db ${DB_SOURCES})

target_compile_definitions(db PRIVATE GIT_DESCRIBE="${GIT_DESCRIBE_VERSION}")

target_link_libraries(db 
	common
	libgame 
	libpoly 
	libsql 
	libthecore

	# external
	#mariadbclient
)

if (USE_SIL AND NOT WIN32)
	target_link_libraries(db ${MARIADB_LIB} ${SSL_LIB} ${CRYPTO_LIB} ${ZLIB_LIB})
else()
	target_link_libraries(db mariadbclient)
endif()

if (WIN32)
	target_link_libraries(db ws2_32)
else()
	target_link_libraries(db pthread md)
endif()
