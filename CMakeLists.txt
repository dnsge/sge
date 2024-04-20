cmake_minimum_required(VERSION 3.17...3.28)

project(SageEngine 
    VERSION 1.0
    DESCRIPTION "Proof-of-concept multipurpose 2D Game Engine"
    LANGUAGES CXX
)

option(BUILD_SERVER "Build server binary" On)
option(BUILD_CLIENT "Build client binary" On)

option(FPS_STATS "Track and print game FPS information" Off)
option(RECORDING_ENABLED "Record frames to disk" Off)
option(SCRIPT_TRACING "Trace and print scripting library calls" Off)
option(NET_DEBUG "Debug network operations" Off)

option(SGE_USE_PRECOMPILED_HEADER "Use precompiled header" On)

##########################
# Compiler Configuration #
##########################

find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)
if (SCCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    message("-- Using sccache for speeding up builds")
elseif (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message("-- Using ccache for speeding up builds")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################
# Dependencies #
################

# Find SDL packages
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Find Boost
find_package(Boost 1.84.0 REQUIRED OPTIONAL_COMPONENTS thread headers)

set(RAPIDJSON_BUILD_DOC OFF CACHE INTERNAL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)

set(LUABRIDGE_CXX17 ON CACHE INTERNAL "" FORCE)

set(BOX2D_BUILD_UNIT_TESTS OFF CACHE INTERNAL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE INTERNAL "" FORCE)

set(MSGPACK_CXX20 ON CACHE INTERNAL "" FORCE)
set(MSGPACK_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
SET(MSGPACK_USE_STD_VARIANT_ADAPTOR ON CACHE INTERNAL "" FORCE)

add_subdirectory(lib/lua)       # lua v5.4.6
add_subdirectory(lib/LuaBridge) # LuaBridge v2.8
add_subdirectory(lib/box2d)     # Box2D v2.4.1
add_subdirectory(lib/msgpack-c) # msgpack-c (C++) v6.1.1
add_subdirectory(lib/recursive-variant)

# Game engine source
add_subdirectory(src)
