# lib name
set(FULL_NAME ActGeant4)

# set files to compile
file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hh)

# LinkDef and dict generation
# set(LINKDEF_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${FULL_NAME}LinkDef.hh)
# set(DICTIONARY_NAME G__${FULL_NAME})
# ROOT_GENERATE_DICTIONARY(${DICTIONARY_NAME} ${headers} LINKDEF  ${LINKDEF_LOCATION})

# build executable
add_executable(${FULL_NAME} ${FULL_NAME}.cc ${sources} ${headers})
target_link_libraries(${FULL_NAME} ${Geant4_LIBRARIES})
# install it in bin
install(TARGETS ${FULL_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# copy macros
set(MACROS
  gui.mac
  init_vis.mac
  vis.mac
  )

foreach(_macro ${MACROS})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/macros/${_macro}
    ${CMAKE_INSTALL_PREFIX}/bin/${_macro}
    COPYONLY
    )
endforeach()
