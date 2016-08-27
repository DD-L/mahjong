cmake_minimum_required (VERSION 2.8.4)
project (Engine)

# special Cygwin
#set(CMAKE_LEGACY_CYGWIN_WIN32 1)

# Set Debug/Release
set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -DENGINE_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

if (WIN32 OR CYGWIN)
    add_definitions (
        -D__USE_W32_SOCKETS
    )
endif ()


add_definitions (
# boost
    -D_REENTRANT
#    -DBOOST_ALL_NO_LIB
#    -DBOOST_SYSTEM_NO_LIB
#    -DBOOST_DATE_TIME_SOURCE

# log_lib
    -DLOG_LOCKFREE_QUEUE
#  deprecated warning, boost 
    -Wno-deprecated-declarations
)

# gcc -I
include_directories (
    ${PROJECT_SOURCE_DIR}/contrib/boost/boost_1_57_0
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/boost_build
)


# gcc -L
link_directories ( 
    ${PROJECT_BINARY_DIR}/src/boost_build 
    ${PROJECT_BINARY_DIR}/src/log 
    ${PROJECT_BINARY_DIR}/src/engine 
    ${PROJECT_BINARY_DIR}/src/mahjong 
)


# check C++11 support
include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1y)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
elseif(COMPILER_SUPPORTS_CXX1y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


# add dir - "./src"
add_subdirectory (src/boost_build)
add_subdirectory (src/log)
add_subdirectory (src/engine)
add_subdirectory (src/mahjong)
add_subdirectory (src/client)


