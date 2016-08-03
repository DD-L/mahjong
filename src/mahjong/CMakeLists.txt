aux_source_directory (. DIR_SRC)
file (GLOB HEADERS
    ./*.h
)
add_executable (mahjong ${DIR_SRC} ${HEADERS})
# make install -- build/bin
install (TARGETS mahjong DESTINATION ${PROJECT_BINARY_DIR}/bin)
file(COPY ${CMAKE_SOURCE_DIR}/src/engine/server-config.json 
    DESTINATION ${PROJECT_BINARY_DIR}/bin)

# gcc -l
target_link_libraries ( mahjong 
# 注意顺序
    engine
    log_lib
    boost_build_essential   
    pthread
)

if (WIN32 OR CYGWIN)
    add_definitions (
        -D__USE_W32_SOCKETS
    )
    target_link_libraries ( mahjong
        ws2_32
        #-lws2_32
    )
endif ()

if (UNIX)
    target_link_libraries ( mahjong
        rt
    )
endif ()

