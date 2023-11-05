cmake_minimum_required(VERSION 3.22)

project(libatla)

# TODO: figure out how to hide this from other projects
#add_subdirectory(external/libsmd)

include(external/lua-5.2.4/lua.cmake)
include(src/atla.cmake)

# add_executable(atlalc ./src/atlalc/atlalc.c)
# target_link_libraries(atlalc lua)

# todo: tests
# add_custom_command(
#   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic/atla.gen.c
#   COMMAND atlalc -o ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic ../test/01_basic/basic.lua
#   DEPENDS atlalc ./test/01_basic/basic.lua)
# add_executable(01_basic ./test/01_basic/main.c ${CMAKE_CURRENT_BINARY_DIR}/test/01_basic/atla.gen.c)
# target_include_directories(01_basic 
#   PRIVATE ./include)
# 
#   add_custom_command(
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test/02_split/atla.gen.c
#     COMMAND atlalc -o ${CMAKE_CURRENT_BINARY_DIR}/test/02_split ../test/02_split/foo.lua ../test/02_split/vec2.lua ../test/02_split/vec3.lua
#     DEPENDS atlalc ./test/02_split/foo.lua ./test/02_split/vec2.lua ./test/02_split/vec3.lua)
#   add_executable(02_split ./test/02_split/main.c ${CMAKE_CURRENT_BINARY_DIR}/test/02_split/atla.gen.c)
#   target_include_directories(02_split 
#     PRIVATE ./include)