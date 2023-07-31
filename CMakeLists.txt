#-----ActRoot-----

# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR) #minimum version of CMAKE required
project(ActRoot)

# stablish c++ version
set(CMAKE_CXX_STANDARD 17) # same as ROOT built for Ubuntu
set(CMAKE_CXX_STANDARD_REQUIRED TRUE) # force require this standard
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
##ROOT
find_package(ROOT 6.20 CONFIG REQUIRED COMPONENTS MathCore Physics)
include(${ROOT_USE_FILE})##automatically loads ROOT_INCLUDE_DIRS and ROOT_LIBRARY_DIR (BUT NOT LIBRARIES, they must be added manually in target_link_libs)
include(Scripts.cmake)

# set install dir
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

# include all the includes!
include_directories(".")

##add subdirs
add_subdirectory(Core)
add_subdirectory(Physics)
