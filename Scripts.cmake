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
    set(options OPTS)
    set(oneValue NAME)
    set(multiValue LINK HEADERS SOURCES)
    cmake_parse_arguments(PARSED_ARGS "${options}" "${oneValue}"
                           "${multiValue}" ${ARGN})
    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "add_actlibrary() : you must provide a library name")
    endif()

    # get sources
    file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx ${PARSED_ARGS_SOURCES})
    # get headers
    file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h ${PARSED_ARGS_HEADERS})

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

function(add_userlibrary)
    # parse arguments
    set(options OPTS)
    set(oneValue NAME)
    set(multiValue LINK SOURCES)
    cmake_parse_arguments(PARSED_ARGS "${options}" "${oneValue}"
                           "${multiValue}" ${ARGN})
    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "add_actlibrary() : you must provide a library name")
    endif()

    set(CMAKE_CXX_STANDARD 17)

    # ROOT
    find_package(ROOT 6.20 CONFIG REQUIRED COMPONENTS MathCore MathMore Physics GenVector)
    include(${ROOT_USE_FILE})

    # ActRoot
    set(ACTROOT_INSTALL $ENV{ACTROOT}/install)
    include_directories(${ACTROOT_INSTALL}/include)
    link_directories(${ACTROOT_INSTALL}/lib)

    add_library(${PARSED_ARGS_NAME} SHARED ${PARSED_ARGS_SOURCES})
    target_link_libraries(${PARSED_ARGS_NAME} PRIVATE ${ROOT_LIBRARIES} ${PARSED_ARGS_LINK})
endfunction()
