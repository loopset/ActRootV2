set(LIB_NAME ActPhysics)

file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# Build dictionary
set(LINKDEF_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ActPhysicsLinkDef.h)
set(DICTIONARY_NAME G__${LIB_NAME})
ROOT_GENERATE_DICTIONARY(${DICTIONARY_NAME} ${headers} LINKDEF  ${LINKDEF_LOCATION})

# add library
add_library(${LIB_NAME} SHARED ${sources} ${headers} ${DICTIONARY_NAME}.cxx)
## add headers so g++ knows where to locate them
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# link to ROOT
target_link_libraries(${LIB_NAME} ${ROOT_LIBRARIES})
# install
install_actroot_component(${LIB_NAME} "${headers}")
