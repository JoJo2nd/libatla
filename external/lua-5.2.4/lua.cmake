
file(GLOB lua-srcs 
external/lua-5.2.4/src/lapi.c
external/lua-5.2.4/src/lcode.c
external/lua-5.2.4/src/lctype.c
external/lua-5.2.4/src/ldebug.c
external/lua-5.2.4/src/ldump.c
external/lua-5.2.4/src/ldo.c
external/lua-5.2.4/src/lfunc.c
external/lua-5.2.4/src/lgc.c
external/lua-5.2.4/src/llex.c
external/lua-5.2.4/src/lmem.c
external/lua-5.2.4/src/lobject.c
external/lua-5.2.4/src/lopcodes.c
external/lua-5.2.4/src/lparser.c
external/lua-5.2.4/src/lstate.c
external/lua-5.2.4/src/lstring.c
external/lua-5.2.4/src/ltable.c
external/lua-5.2.4/src/ltm.c
external/lua-5.2.4/src/lundump.c
external/lua-5.2.4/src/lvm.c
external/lua-5.2.4/src/lzio.c
external/lua-5.2.4/src/lauxlib.c
external/lua-5.2.4/src/lbaselib.c
external/lua-5.2.4/src/lbitlib.c
external/lua-5.2.4/src/lcorolib.c
external/lua-5.2.4/src/ldblib.c
external/lua-5.2.4/src/liolib.c
external/lua-5.2.4/src/lmathlib.c
external/lua-5.2.4/src/loslib.c
external/lua-5.2.4/src/lstrlib.c
external/lua-5.2.4/src/ltablib.c
external/lua-5.2.4/src/loadlib.c
external/lua-5.2.4/src/linit.c)

add_library(lua STATIC ${lua-srcs})

target_include_directories(lua INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
