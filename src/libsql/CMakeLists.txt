file(GLOB_RECURSE LIBSQL_SOURCES "*.h" "*.cpp")

add_library(libsql STATIC ${LIBSQL_SOURCES})
target_precompile_headers(libsql PRIVATE stdafx.h)

target_link_libraries(libsql 
	common
)
