# PracticaRobot.cmake - Funcion para el CMakeLists.txt del ALUMNO (cuando se trae este repositorio con FetchContent).
#
#   include(FetchContent)
#   FetchContent_Declare(RobotSimulator GIT_REPOSITORY https://github.com/jmguerreroh/RobotSimulator.git GIT_TAG main)
#   FetchContent_MakeAvailable(RobotSimulator)
#   robot_practica()
#
# robot_practica():
#   1. La primera vez COPIA a la carpeta del alumno una plantilla de main.cpp (si ya existe, NO la toca).
#   2. Crea el ejecutable MiPrograma con ese main.cpp y lo enlaza con el simulador.
# Argumentos opcionales:  NOMBRE <ejecutable>   FUENTES <a.cpp b.cpp ...> (ademas de main.cpp)
function(robot_practica)
  cmake_parse_arguments(A "" "NOMBRE" "FUENTES" ${ARGN})
  if(NOT A_NOMBRE)
    set(A_NOMBRE MiPrograma)
  endif()
  get_filename_component(raiz "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
  set(dir "${CMAKE_SOURCE_DIR}")  # carpeta del proyecto del alumno

  if(NOT EXISTS "${dir}/main.cpp")
    file(COPY "${raiz}/alumno/main.cpp" DESTINATION "${dir}")
    message(STATUS "Plantilla creada en tu carpeta: main.cpp")
  endif()

  add_executable(${A_NOMBRE} "${dir}/main.cpp" ${A_FUENTES})
  target_link_libraries(${A_NOMBRE} PRIVATE RobotSimulator)
endfunction()
