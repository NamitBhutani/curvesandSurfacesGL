cmake_minimum_required(VERSION 3.10)
project(SceneGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OpenGL preference to GLVND
set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_library(glad STATIC
    glad/glad.c)
target_include_directories(glad PRIVATE
    glad/include)

# Add your source files
add_executable(${PROJECT_NAME}
    main.cpp
    camera.h
    shader.h
    mesh.h
    model.h
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    glad/include
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    $ENV{GLM_INCLUDE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS})

# Copy model & shader files to build directory
file(GLOB MODEL_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*.off"
)
file(GLOB SHADER_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)
file(
    GLOB OBJECT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/build/*"
)
file(COPY ${MODEL_FILES} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${OBJECT_FOLDER} DESTINATION ${CMAKE_BINARY_DIR})

# Set the OpenGL policy to remove warning
cmake_policy(SET CMP0072 NEW)