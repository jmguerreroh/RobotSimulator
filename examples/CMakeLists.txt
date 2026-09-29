# Cada ejemplo es un ejecutable que solo enlaza con RobotSimulator.
foreach(example ejemplo_basico ejemplo_movimiento ejemplo_no_bloqueante)
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE RobotSimulator)
endforeach()
