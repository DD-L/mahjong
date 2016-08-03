aux_source_directory (. DIR_SRC)
file (GLOB HEADERS
    ./*.h
)
add_executable (client ${DIR_SRC} ${HEADERS})
# make install -- build/bin
install (TARGETS client DESTINATION ${PROJECT_BINARY_DIR}/bin)

# gcc -l
target_link_libraries ( client 
# 注意顺序
#    engine
    boost_build_essential   
    pthread
)

if (WIN32 OR CYGWIN)
    add_definitions (
        -D__USE_W32_SOCKETS
    )
    target_link_libraries ( client
        ws2_32
        #-lws2_32
    )
endif ()

if (UNIX)
    target_link_libraries ( client
        rt
    )
endif ()

