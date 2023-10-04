# lib name
set(LIB_NAME Physics)
set(FULL_NAME Act${LIB_NAME})
## set files to compile
file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx)
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# LinkDef and dict generation
set(LINKDEF_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${FULL_NAME}LinkDef.hh)
set(DICTIONARY_NAME G__${FULL_NAME})
ROOT_GENERATE_DICTIONARY(${DICTIONARY_NAME} ${headers} LINKDEF  ${LINKDEF_LOCATION})

# add library
add_library(${FULL_NAME} SHARED ${sources} ${headers} ${DICTIONARY_NAME}.cxx)
## add headers so g++ knows where to locate them
target_include_directories(${FULL_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# link to ROOT
target_link_libraries(${FULL_NAME} ${ROOT_LIBRARIES})
# install
install_actroot_component(${FULL_NAME} "${headers}")
