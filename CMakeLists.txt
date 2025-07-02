cmake_minimum_required(VERSION 3.16)
project(vkeng CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall)

find_package(Boost REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(range-v3 REQUIRED)
find_package(Vulkan REQUIRED)
# find_package(libpng REQUIRED)

include_directories(include)
add_subdirectory(demo)

add_library(vkeng INTERFACE)
target_include_directories(vkeng
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# installs
install(TARGETS vkeng
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)

