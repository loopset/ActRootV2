add_actlibrary(NAME ActUtility LINK ActCore)

#1-> actroot executable
add_executable(actroot exec/actroot.cxx)
target_link_libraries(actroot ${ROOT_LIBRARIES} ActCore ActDetectors ActUtility)
install_executable(actroot)

#2-> actplot executable
add_executable(actplot exec/actplot.cxx)
target_link_libraries(actplot ${ROOT_LIBRARIES} ActCore ActDetectors ActUtility)
install_executable(actplot)
