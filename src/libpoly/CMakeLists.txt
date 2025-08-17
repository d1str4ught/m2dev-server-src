file(GLOB_RECURSE LIBPOLY_SOURCES "*.h" "*.cpp")

add_library(libpoly STATIC ${LIBPOLY_SOURCES})

target_link_libraries(libpoly 
	common
)
