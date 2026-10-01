cmake_minimum_required(VERSION 3.16)
project(RobotSimulator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilacion" FORCE)
endif()

# Solo se compilan ejemplos/tests cuando este es el proyecto principal.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(ROBOTSIM_IS_TOP_LEVEL ON)
else()
  set(ROBOTSIM_IS_TOP_LEVEL OFF)
endif()
option(ROBOTSIM_BUILD_EXAMPLES "Compilar los ejemplos" ${ROBOTSIM_IS_TOP_LEVEL})
option(ROBOTSIM_BUILD_TESTS "Compilar los tests" ${ROBOTSIM_IS_TOP_LEVEL})

include(${CMAKE_CURRENT_LIST_DIR}/cmake/PracticaRobot.cmake)  # define robot_practica() para el CMakeLists del alumno

find_package(Threads REQUIRED)

# ---- GLFW: el del sistema si existe; si no, se descarga y compila ------------
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "GLFW no encontrado en el sistema: descargandolo con FetchContent")
  include(FetchContent)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4)
  FetchContent_MakeAvailable(glfw)
endif()

# ---- GLM (solo cabeceras) y GLAD (OpenGL 3.3 Core), incluidos en third_party --
add_library(robotsim_glm INTERFACE)
target_include_directories(robotsim_glm SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)

add_library(robotsim_glad STATIC third_party/glad/src/glad.c)
target_include_directories(robotsim_glad SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include)
target_link_libraries(robotsim_glad PUBLIC ${CMAKE_DL_LIBS})
set_target_properties(robotsim_glad PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Shaders embebidos: shaders/*.vert|frag -> cabecera generada -----------
# Asi la biblioteca no depende de rutas de ficheros en tiempo de ejecucion.
set(ROBOTSIM_SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic.vert ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic.frag)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ROBOTSIM_SHADER_FILES})
file(READ shaders/basic.vert BASIC_VERT_SOURCE)
file(READ shaders/basic.frag BASIC_FRAG_SOURCE)
set(ROBOTSIM_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(cmake/shaders_embedded.h.in ${ROBOTSIM_GENERATED_DIR}/shaders_embedded.h @ONLY)

# ---- Biblioteca -----------------------------------------------------------
add_library(RobotSimulator STATIC
  src/Simulador.cpp
  src/Board.cpp
  src/Camera.cpp
  src/Mesh.cpp
  src/RenderLoop.cpp
  src/Renderer.cpp
  src/SessionLog.cpp
  src/Sha256.cpp
  src/Shader.cpp
  src/Window.cpp)
add_library(RobotSimulator::RobotSimulator ALIAS RobotSimulator)

target_include_directories(RobotSimulator
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${ROBOTSIM_GENERATED_DIR})
target_link_libraries(RobotSimulator PRIVATE glfw robotsim_glad robotsim_glm Threads::Threads)

if(MSVC)
  target_compile_options(RobotSimulator PRIVATE /W4)
else()
  target_compile_options(RobotSimulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ROBOTSIM_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ROBOTSIM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
