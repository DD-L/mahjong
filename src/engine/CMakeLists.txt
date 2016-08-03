add_definitions (
    "-Wno-unused-local-typedefs"    
) 
aux_source_directory (. DIR_SRC)
file (GLOB HEADERS
    ./*.h
)
add_library (engine STATIC ${DIR_SRC} ${HEADERS})
