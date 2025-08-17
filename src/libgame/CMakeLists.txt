file(GLOB_RECURSE LIBGAME_SOURCES "*.h" "*.cpp")

add_library(libgame STATIC ${LIBGAME_SOURCES})

target_link_libraries(libgame 
	common
)
