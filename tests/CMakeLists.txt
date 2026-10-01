# Los tests de Board, Camera y registro usan cabeceras internas (src/) y no abren ventana.
foreach(name test_board test_camera test_simulador_api test_log)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE RobotSimulator robotsim_glm)
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

# test_log escribe un log real; si existe el verificador del profesor (profesor/verificar_log.py, que no se
# sube al repositorio) y hay Python 3, se comprueba que lo da por original y detecta las modificaciones.
set(ROBOTSIM_VERIFICADOR ${PROJECT_SOURCE_DIR}/profesor/verificar_log.py)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND AND EXISTS ${ROBOTSIM_VERIFICADOR})
  set(ROBOTSIM_LOG_EJEMPLO ${CMAKE_CURRENT_BINARY_DIR}/ejemplo_verificacion.log)
  add_test(NAME test_log_escribe COMMAND test_log ${ROBOTSIM_LOG_EJEMPLO})
  add_test(NAME test_log_verificador_python
    COMMAND ${CMAKE_COMMAND} -DPYTHON=${Python3_EXECUTABLE} -DVERIFICADOR=${ROBOTSIM_VERIFICADOR}
            -DLOG=${ROBOTSIM_LOG_EJEMPLO} -P ${CMAKE_CURRENT_SOURCE_DIR}/verificar_log.cmake)
  set_tests_properties(test_log_escribe PROPERTIES FIXTURES_SETUP log_ejemplo)
  set_tests_properties(test_log_verificador_python PROPERTIES FIXTURES_REQUIRED log_ejemplo)
endif()
