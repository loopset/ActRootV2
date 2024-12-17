include_directories(./Actions/)

add_actlibrary(NAME ActAlgorithm LINK ActData HEADERS ./Actions/*.h SOURCES ./Actions/*.cxx)
