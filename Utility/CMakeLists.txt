#1-> actroot executable
add_executable(actroot actroot.cxx)
target_link_libraries(actroot ${ROOT_LIBRARIES} ActCore)
install_executable(actroot)

#2-> actplot executable
add_executable(actplot actplot.cxx)
target_link_libraries(actplot ${ROOT_LIBRARIES} ActCore)
install_executable(actplot)
