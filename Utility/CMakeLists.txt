#1-> actroot executable
add_executable(actroot actroot.cxx)
target_link_libraries(actroot ${ROOT_LIBRARIES} ActCore)
install_executable(actroot)
