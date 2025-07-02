cmake_minimum_required(VERSION 3.15)
project(Triangle2DDemo)

# 0) Where is your source?
# If main.cpp lives in demo/Triangle2D:
add_executable(Triangle2DDemo main.cpp)

# Link against Conan-provided targets:
target_link_libraries(Triangle2DDemo PRIVATE
    Boost::boost
    glm::glm
    range-v3::range-v3
    glfw
    Vulkan::Vulkan
)

# 1) Gather your shaders
file(GLOB SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)

# 2) Prepare outputs and ensure folder exists
set(SPIRV_SHADERS "")
set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${SPIRV_DIR}")

foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUT "${SPIRV_DIR}/${FILE_NAME}.spv")
    list(APPEND SPIRV_SHADERS ${OUT})
    add_custom_command(
        OUTPUT ${OUT}
        COMMAND glslangValidator -V ${SHADER} -o ${OUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${FILE_NAME} to SPIR-V"
    )
endforeach()

# 3) Make a Shaders target, then make the EXE depend on it
add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADERS})
add_dependencies(Triangle2DDemo Shaders)

# 4) After build, copy the original .vert/.frag next to your EXE
add_custom_command(TARGET Triangle2DDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
      $<TARGET_FILE_DIR:Triangle2DDemo>/shaders
)
