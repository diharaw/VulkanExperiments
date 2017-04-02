cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB_RECURSE HELLO_VULKAN_SOURCE  *.cpp *.h *.c)

add_executable(1-hello-vulkan ${HELLO_VULKAN_SOURCE})

set_target_properties( 1-hello-vulkan
    				           PROPERTIES
    				           ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/1-hello-vulkan"
    				           LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/1-hello-vulkan"
    				           RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/1-hello-vulkan" )

target_link_libraries(1-hello-vulkan ${VULKAN_STATIC_LIBRARY})
target_link_libraries(1-hello-vulkan glfw)