# Los tests de Board y Camera usan cabeceras internas (src/) y no abren ventana.
foreach(name test_board test_camera test_simulador_api)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE RobotSimulator robotsim_glm)
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
