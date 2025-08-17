project(liblua_static)

set(LUA_SOURCES
	lapi.c
	lcode.c
	ldebug.c
	ldo.c
	ldump.c
	lfunc.c
	lgc.c
	llex.c
	lmem.c
	lobject.c
	lopcodes.c
	lparser.c
	lstate.c
	lstring.c
	ltable.c
	ltests.c
	ltm.c
	lundump.c
	lvm.c
	lzio.c
	lib/lauxlib.c
	lib/lbaselib.c
	lib/ldblib.c
	lib/liolib.c
	lib/lmathlib.c
	lib/loadlib.c
	lib/lstrlib.c
	lib/ltablib.c
)

add_library(${PROJECT_NAME} STATIC ${LUA_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../include .)