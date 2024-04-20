add_library(sge-lib OBJECT
    Constants.hpp
    Debug.hpp
    Realm.hpp
    Renderer.cpp
    Renderer.hpp

    game/Actor.cpp
    game/Actor.hpp
    game/Game.cpp
    game/Game.hpp
    game/Input.cpp
    game/Input.hpp
    game/Scene.cpp
    game/Scene.hpp

    net/Client.cpp
    net/Client.hpp
    net/Host.cpp
    net/Host.hpp
    net/Messages.hpp
    net/MessageSocket.hpp
    net/Protocol.cpp
    net/Protocol.hpp
    net/Replicator.cpp
    net/Replicator.hpp

    physics/Collision.cpp
    physics/Collision.hpp
    physics/Raycast.hpp
    physics/Rigidbody.cpp
    physics/Rigidbody.hpp
    physics/World.cpp
    physics/World.hpp

    render/RenderQueue.cpp
    render/RenderQueue.hpp
    render/Text.cpp
    render/Text.hpp

    resources/Configs.cpp
    resources/Configs.hpp
    resources/Deserialize.cpp
    resources/Deserialize.hpp
    resources/Resources.cpp
    resources/Resources.hpp
    
    scripting/ActorTemplate.cpp
    scripting/ActorTemplate.hpp
    scripting/Component.cpp
    scripting/Component.hpp
    scripting/ComponentContainer.cpp
    scripting/ComponentContainer.hpp
    scripting/EventSub.cpp
    scripting/EventSub.hpp
    scripting/Invoke.hpp
    scripting/Libs.cpp
    scripting/Libs.hpp
    scripting/LuaInterface.hpp
    scripting/LuaValue.hpp
    scripting/Scripting.cpp
    scripting/Scripting.hpp
    scripting/Tracing.cpp
    scripting/Tracing.hpp

    scripting/components/CppComponent.cpp
    scripting/components/CppComponent.hpp
    scripting/components/InterpTransform.cpp
    scripting/components/InterpTransform.hpp
    scripting/components/LuaComponent.hpp
    scripting/components/LuaComponent.cpp
    scripting/components/Transform.cpp
    scripting/components/Transform.hpp

    util/AsyncLock.cpp
    util/AsyncLock.hpp
    util/AsyncSpscQueue.hpp
    util/B2Ptr.hpp
    util/FPS.cpp
    util/FPS.hpp
    util/Rect.hpp
    util/SDLPtr.hpp
)

target_include_directories(sge-lib PUBLIC 
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/lib/glm"
    "${CMAKE_SOURCE_DIR}/lib/rapidjson/include"
    "${CMAKE_SOURCE_DIR}/lib/lua"
    "${CMAKE_SOURCE_DIR}/lib/LuaBridge/Source"
    "${CMAKE_SOURCE_DIR}/lib/box2d/include"
    "${Boost_INCLUDE_DIRS}"
)

if(SGE_USE_PRECOMPILED_HEADER)
    message(STATUS "Building with precompiled header")
    target_precompile_headers(sge-lib PRIVATE PrecompiledHeader.hpp)
else()
    message(STATUS "Building without precompiled header")
endif()

###########################
# Compilation definitions #
###########################

target_compile_definitions(sge-lib PUBLIC LUABRIDGE_CXX17=1)

if(FPS_STATS)
    target_compile_definitions(sge-lib PUBLIC TRACK_FPS)
endif()

if(RECORDING_ENABLED)
    target_compile_definitions(sge-lib PUBLIC ENABLE_RECORDING_MODE)
endif()

if(SCRIPT_TRACING)
    target_compile_definitions(sge-lib PUBLIC TRACING)
endif()

if (NET_DEBUG)
    target_compile_definitions(sge-lib PUBLIC NET_DEBUG)
endif()

if(MSVC)
    target_compile_options(sge-lib PUBLIC /wd4996)
else()
    target_compile_options(sge-lib PUBLIC 
        -Wno-deprecated
        -Wno-deprecated-declarations
    )
endif()

########################
# Link to core library #
########################

target_link_libraries(sge-lib PRIVATE 
    lua
    box2d
)

target_link_libraries(sge-lib PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
)

target_link_libraries(sge-lib PUBLIC
    ${Boost_LIBRARIES}
    msgpack-cxx
    rva
)

######################
# Create executables #
######################

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR LANGUAGES CXX)
    if(LTO_SUPPORTED)
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(STATUS "IPO / LTO not supported: <${LTO_ERROR}>")
    endif()
endif()

if(BUILD_CLIENT)
    add_executable(sge-client
        Realm.cpp

        client/AudioPlayer.cpp
        client/AudioPlayer.hpp    
        client/Client.cpp
        client/Client.hpp
        client/ClientInterface.cpp
        client/ClientInterface.hpp
        client/main.cpp
    )
    target_link_libraries(sge-client PUBLIC sge-lib)

    if(SGE_USE_PRECOMPILED_HEADER)
        target_precompile_headers(sge-client REUSE_FROM sge-lib)
    endif()

    if(LTO_SUPPORTED)
        set_property(TARGET sge-client PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)    
    endif()
endif()

if(BUILD_SERVER)
    add_executable(sge-server
        Realm.cpp

        server/Server.cpp
        server/Server.hpp
        server/ServerInterface.cpp
        server/ServerInterface.hpp
        server/main.cpp
    )
    target_compile_definitions(sge-server PUBLIC SGE_SERVER)
    target_link_libraries(sge-server PUBLIC sge-lib)

    if(SGE_USE_PRECOMPILED_HEADER)
        target_precompile_headers(sge-server REUSE_FROM sge-lib)
    endif()

    if(LTO_SUPPORTED)
        set_property(TARGET sge-server PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
