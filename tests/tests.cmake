

file(GLOB 01_basic-srcs tests/01_basic/*.c)
file(GLOB 02_obj_ref-srcs tests/02_obj_ref/*.c)


add_executable(01_basic ${01_basic-srcs})
target_link_libraries(01_basic PRIVATE atla)

add_executable(02_obj_ref ${02_obj_ref-srcs})
target_link_libraries(02_obj_ref PRIVATE atla)
