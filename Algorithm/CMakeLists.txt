include_directories(./Actions/)

add_actlibrary(NAME ActAlgorithm LINK ActPhysics HEADERS ./Actions/*.h SOURCES ./Actions/*.cxx)
