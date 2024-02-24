cmake_minimum_required(VERSION 3.8)

project(graphics CXX)

find_package(Vulkan REQUIRED)

message("Using Vulkan ${Vulkan_VERSION}")

add_subdirectory(extern/SDL)
add_subdirectory(extern/vk-bootstrap)
add_subdirectory(src)

add_executable(main main.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 23)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE engine::graphics)
