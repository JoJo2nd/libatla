cmake_minimum_required(VERSION 3.22)

project(Atla)

add_library(lua 
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

add_executable(atlalc ./src/atlalc/atlalc.c)
target_link_libraries(atlalc lua)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic/atla.gen.c
  COMMAND atlalc -o ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic ../test/01_basic/basic.lua
  DEPENDS atlalc ./test/01_basic/basic.lua)
add_executable(01_basic ./test/01_basic/main.c ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic/atla.gen.c)
target_include_directories(01_basic 
  PRIVATE ./include)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test/02_split/atla.gen.c
    COMMAND atlalc -o ${CMAKE_CURRENT_BINARY_DIR}/test/02_split ../test/02_split/foo.lua ../test/02_split/vec2.lua ../test/02_split/vec3.lua
    DEPENDS atlalc ./test/02_split/foo.lua ./test/02_split/vec2.lua ./test/02_split/vec3.lua)
  add_executable(02_split ./test/02_split/main.c ${CMAKE_CURRENT_BINARY_DIR}/test/02_split/atla.gen.c)
  target_include_directories(02_split 
    PRIVATE ./include)