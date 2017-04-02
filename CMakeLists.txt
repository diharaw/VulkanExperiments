cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project("VulkanExperiments")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/external")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/external")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glfw/include")
set(GLM_INCLUDE_DIRS  "${PROJECT_SOURCE_DIR}/external/glm")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan)

include_directories("${GLFW_INCLUDE_DIRS}" 
					"${GLM_INCLUDE_DIRS}"
					"${VULKAN_INCLUDE_DIR}")

add_subdirectory(external)
add_subdirectory(src)

