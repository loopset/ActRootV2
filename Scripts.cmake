function(install_actroot_component fullname name headers)
  ##install target
  install(TARGETS ${fullname} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  ##dictionary
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/G__${fullname}.cxx DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  ##pcm file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${fullname}_rdict.pcm DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
  ##rootmap file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${fullname}.rootmap DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
  ## headers
  install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endfunction()
