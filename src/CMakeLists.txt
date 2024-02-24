cmake_minimum_required(VERSION 3.8)

project(engine CXX)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/graphics/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/graphics/Error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/graphics/Instance.cpp)

add_library(graphics SHARED ${SOURCES})
add_library(engine::graphics ALIAS graphics)

target_include_directories(graphics PRIVATE Vulkan::Headers  ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(graphics PRIVATE SDL3::SDL3 Vulkan::Vulkan vk-bootstrap::vk-bootstrap)
set_property(TARGET graphics PROPERTY CXX_STANDARD 23)
