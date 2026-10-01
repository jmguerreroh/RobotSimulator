# Prueba de integracion: el log escrito por C++ lo verifica el script de Python del profesor, y tambien
# detecta las modificaciones. Uso: cmake -DPYTHON=.. -DVERIFICADOR=.. -DLOG=.. -P verificar_log.cmake
function(ejecutar esperado)
  execute_process(COMMAND ${PYTHON} ${VERIFICADOR} ${ARGN} RESULT_VARIABLE r OUTPUT_VARIABLE salida ERROR_VARIABLE salida)
  if(NOT r EQUAL ${esperado})
    message(FATAL_ERROR "verificar_log.py ${ARGN}: esperaba codigo ${esperado} y devolvio ${r}\n${salida}")
  endif()
endfunction()

file(READ ${LOG} contenido)
string(REGEX MATCH "checksum: ([0-9a-f]+)" _ "${contenido}")
set(checksum ${CMAKE_MATCH_1})

ejecutar(0 ${LOG})                      # original
ejecutar(0 ${LOG} ${checksum})          # checksum indicado coincide
ejecutar(1 ${LOG} 0000000000000000000000000000000000000000000000000000000000000000)

# 1) cambiar el autor, 2) cambiar el estado final, 3) cambiar el checksum, 4) truncar el fichero
string(REPLACE "autor: Ana" "autor: Eva" t1 "${contenido}")
if(t1 STREQUAL contenido)
  message(FATAL_ERROR "el test no ha modificado el autor")
endif()
file(WRITE ${LOG}.t1 "${t1}")
ejecutar(1 ${LOG}.t1)

string(REPLACE "---\n.*-\n..R" "---\n.*-\n.R." t2 "${contenido}")
if(t2 STREQUAL contenido)
  message(FATAL_ERROR "el test no ha modificado el estado final")
endif()
file(WRITE ${LOG}.t2 "${t2}")
ejecutar(1 ${LOG}.t2)

string(SUBSTRING "${checksum}" 63 1 ultimo)
if(ultimo STREQUAL "0")
  set(nuevo 1)
else()
  set(nuevo 0)
endif()
string(SUBSTRING "${checksum}" 0 63 prefijo)
string(REPLACE "checksum: ${checksum}" "checksum: ${prefijo}${nuevo}" t3 "${contenido}")
if(t3 STREQUAL contenido)
  message(FATAL_ERROR "el test no ha modificado el checksum")
endif()
file(WRITE ${LOG}.t3 "${t3}")
ejecutar(1 ${LOG}.t3)

string(LENGTH "${contenido}" largo)
math(EXPR corto "${largo} - 30")
string(SUBSTRING "${contenido}" 0 ${corto} t4)
file(WRITE ${LOG}.t4 "${t4}")
ejecutar(1 ${LOG}.t4)

ejecutar(2 ${LOG}.no_existe)
message(STATUS "verificador de logs: todo correcto")
