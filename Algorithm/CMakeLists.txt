include_directories(./Actions/)

add_actlibrary(NAME ActAlgorithm LINK ActData ActPhysics HEADERS ./Actions/*.h SOURCES ./Actions/*.cxx)
