function(install_actroot_component name headers)
  ##install target
  install(TARGETS ${name} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  ##pcm file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${name}_rdict.pcm DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
  ##rootmap file
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.rootmap DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib)
  ## headers
  install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endfunction()
