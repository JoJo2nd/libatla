file(GLOB atla-srcs 
    src/*.c
    include/atla/*.h)

add_library(atla STATIC ${atla-srcs})

target_include_directories(atla PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(atla PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/external/libsmd)

target_link_libraries(atla PRIVATE smd)


#if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # We're in the root, define additional targets for developers.
    include(tests/tests.cmake)
#endif()