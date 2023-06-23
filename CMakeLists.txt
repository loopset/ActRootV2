#-----ActRoot-----

# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR) #minimum version of CMAKE required
project(ActRoot)

# stablish c++ version
set(CMAKE_CXX_STANDARD 17) # same as ROOT built for Arch Linux
set(CMAKE_CXX_STANDARD_REQUIRED TRUE) # force require this standard
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
##ROOT
find_package(ROOT 6.20 CONFIG REQUIRED COMPONENTS GenVector MathMore)
include(${ROOT_USE_FILE})##automatically loads ROOT_INCLUDE_DIRS and ROOT_LIBRARY_DIR (BUT NOT LIBRARIES, they must be added manually in target_link_libs)

## set files to compile
file(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cxx)
file(GLOB headers ${CMAKE_SOURCE_DIR}/inc/*.h)

# ActRoot LinkDef and dict generation
set(LINKDEF_LOCATION ${CMAKE_SOURCE_DIR}/ActRootLinkDef.h)
set(DICTIONARY_NAME G__${PROJECT_NAME})
ROOT_GENERATE_DICTIONARY(${DICTIONARY_NAME} ${headers} LINKDEF  ${LINKDEF_LOCATION})

# add ActRoot library
add_library(${PROJECT_NAME} SHARED ${sources} ${headers} ${DICTIONARY_NAME}.cxx)
## add ActRoot headers so g++ knows where to locate them
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/inc)
# link to ROOT
target_link_libraries(${PROJECT_NAME} ${ROOT_LIBRARIES})
