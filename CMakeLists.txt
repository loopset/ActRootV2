#-----ActRoot-----

# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR) #minimum version of CMAKE required
project(ActRoot)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
# stablish c++ version
set(CMAKE_CXX_STANDARD 17) # same as ROOT built for Ubuntu
set(CMAKE_CXX_STANDARD_REQUIRED TRUE) # force require this standard
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_MESSAGE LAZY) # print only updates of installed files

# read Geant4 options
option(BUILD_GEANT4 "Build ActGeant4 library" OFF) #disabled by default
#ROOT
# Gui component is of paramount importance to Event viewers
find_package(ROOT 6.20 CONFIG REQUIRED COMPONENTS Gui MathCore Physics GenVector)
if(BUILD_GEANT4)
    find_package(Geant4 CONFIG REQUIRED ui_all vis_all)
endif()
#automatically loads ROOT_INCLUDE_DIRS and ROOT_LIBRARY_DIR (BUT NOT LIBRARIES, they must be added manually in target_link_libs)
include(${ROOT_USE_FILE})
include(Scripts.cmake)

# set install dir
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

# include all the includes!
include_directories(Core/inc)
include_directories(Detectors/inc)
# include_directories(Utility/inc)
include_directories(Physics/inc)
# include_directories(Simulation/inc)
include_directories(Algorithm/inc)
if(BUILD_GEANT4)
    include_directories(Geant4/inc)
endif()

#add subdirs CMakeLists.txt
add_subdirectory(Core)
# add_subdirectory(Detectors)
# add_subdirectory(Utility)
add_subdirectory(Physics)
# add_subdirectory(Simulation)
add_subdirectory(Algorithm)
if(BUILD_GEANT4)
    add_subdirectory(Geant4)
endif()
