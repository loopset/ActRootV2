function(install_headers_library NAME HEADERS)
  #install target
  install(TARGETS ${NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  # #headers
  install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endfunction()

function(install_executable NAME)
  #install executable to bin dir
  install(TARGETS ${NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endfunction()

function(install_root_dict NAME)
  #dictionary
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/G__${NAME}.cxx DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  #pcm file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}_rdict.pcm DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
  #rootmap file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.rootmap DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
endfunction()

function(add_actlibrary)
  # parse arguments
  cmake_parse_arguments(
    PARSED_ARGS #prefix
    "" #boolean arguments
    "NAME" # list of mono-valued args
    "LINK" # list of multi-valued args
    ${ARGN} #number of arguments
  )
  if(NOT PARSED_ARGS_NAME)
    message(FATAL_ERROR "add_actlibrary() : you must provide a library name")
  endif()

  # get sources
  file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)
  # get headers
  file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h)

  # LinkDef and dict generation
  set(LINKDEF_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${PARSED_ARGS_NAME}LinkDef.hh)
  set(DICTIONARY_NAME G__${PARSED_ARGS_NAME})
  ROOT_GENERATE_DICTIONARY(${DICTIONARY_NAME} ${headers} LINKDEF  ${LINKDEF_LOCATION})

  # add library
  add_library(${PARSED_ARGS_NAME} SHARED ${sources} ${headers} ${DICTIONARY_NAME}.cxx)
  # add headers so g++ knows where to locate them
  target_include_directories(${PARSED_ARGS_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
  # link to ROOT
  # and other ActRoot components
  target_link_libraries(${PARSED_ARGS_NAME} ${ROOT_LIBRARIES} ${PARSED_ARGS_LINK})
  # install headers and library
  install_headers_library(${PARSED_ARGS_NAME} "${headers}")
  # and finally root components
  install_root_dict(${PARSED_ARGS_NAME})
endfunction()
