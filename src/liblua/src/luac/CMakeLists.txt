set(LUA_COMPILER_SOURCES
	luac.c
	print.c
	../lopcodes.c
)

if (BUILD_INTERPRETER)
	add_executable(luac ${LUA_COMPILER_SOURCES})
	
	target_include_directories(luac PRIVATE ..)
	target_link_libraries(luac liblua_static)
	
	target_compile_definitions(luac PRIVATE LUA_OPNAMES)
endif()