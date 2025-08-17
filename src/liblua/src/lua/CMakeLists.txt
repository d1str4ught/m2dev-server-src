set(LUA_INTERPRETER_SOURCES
	lua.c
)

if (BUILD_INTERPRETER)
	add_executable(lua ${LUA_INTERPRETER_SOURCES})
	
	target_link_libraries(lua liblua_static)
endif()