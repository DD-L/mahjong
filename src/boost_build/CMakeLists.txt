add_definitions (
    "-Wno-unknown-pragmas -Wno-unused-function"
)

aux_source_directory (. DIR_SRC)
add_library (boost_build_essential STATIC ${DIR_SRC})
