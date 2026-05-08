set(EXECUTABLE_NAME ${PROJECT_NAME})

if (ANDROID)
	# The SDL java code is hardcoded to load libmain.so on android, so we need to change EXECUTABLE_NAME
	set(EXECUTABLE_NAME main)
	add_library(${EXECUTABLE_NAME} SHARED)
else()
	add_executable(${EXECUTABLE_NAME})
endif()


target_sources(${EXECUTABLE_NAME} PRIVATE
        components/aicontroller.h
        components/collision.h
        components/damage.h
        components/health.h
        components/owner.h
        components/playerclass.cpp
        components/playerclass.h
        components/damageaccumulator.h
        components/floatingnumber.h
        components/chainlightningdata.h
        components/fragmentdata.h
        components/spriteeffect.h
        components/whirlwindobjectdata.h
        components/playerindex.h
        components/playerinput.h
        components/playerxp.h
        components/radius.h
        components/timewarpdata.h
        components/render2d.cpp
        components/render2d.h
        components/debugrigidbody2d.cpp
        components/debugrigidbody2d.h
        components/rigidbody2d.h
        components/sprite.h
        components/type.h
        components/weapon.h
        configuration.h
        ecs.h
        entities/classes/baseclass.h
        entities/classes/berserker.cpp
        entities/classes/berserker.h
        entities/classes/bouncer.h
        entities/classes/trapper.h
        entities/classes/wizard.cpp
        entities/classes/wizard.h
        entities/fireball.cpp
        entities/fireball.h
        entities/whirlwindobject.cpp
        entities/whirlwindobject.h
        entities/lizard.cpp
        entities/lizard.h
        entities/physicsobject.h
        entities/gate.cpp
        entities/gate.h
        entities/puzzleblock.cpp
        entities/puzzleblock.h
        entities/mirrorblock.cpp
        entities/mirrorblock.h
        entities/pressureplate.cpp
        entities/pressureplate.h
        entities/roomsensor.cpp
        entities/roomsensor.h
        entities/player.cpp
        entities/player.h
        entities/spawner.h
        entities/fragment.cpp
        entities/fragment.h
        entities/timewarp.cpp
        entities/timewarp.h
        main.cpp
        main.h
        map/dungeon.h
        map/levelmanager.cpp
        map/levelmanager.h
        map/newdungeon.cpp
        map/newdungeon.h
        map/tile.h
        map/room.h
        lobbyconfig.h
        states/game/game.cpp
        states/game/game.h
        states/lobby/lobby.cpp
        states/lobby/lobby.h
#        states/intro/intro.cpp
#        states/intro/intro.h
        states/menu/menu.cpp
        states/menu/menu.h
        states/menu/menustructure.h
        states/test/box2dobjects.cpp
        states/test/box2dobjects.h
        states/test/test.cpp
        states/test/test.h
        ui/ui.cpp
        ui/ui.h
        world/world.cpp
        world/world.h


        extra/LightLayer.cpp
        extra/LightLayer.h
)

if (CMAKE_BUILD_TYPE STREQUAL Release)
    if (EMSCRIPTEN)
        set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS "-pthread -sUSE_PTHREADS  -s PTHREAD_POOL_SIZE=2 -DMA_ENABLE_AUDIO_WORKLETS -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -sASYNCIFY -s FORCE_FILESYSTEM=1 -s TOTAL_MEMORY=67108864 -sUSE_SDL=3 -sUSE_SDL_MIXER=3 -sSTACK_SIZE=8MB -s DEMANGLE_SUPPORT=1 -s GL_ENABLE_GET_PROC_ADDRESS -s EXPORTED_FUNCTIONS=\"[_malloc,_main]\" --shell-file ${CMAKE_SOURCE_DIR}/assets/shell.html --preload-file ${CMAKE_SOURCE_DIR}/assets/@assets --bind")
        set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so taht you can directly open it.

        set(OUTPUT_DIR "../../../bin/Emscripten")
    else()
        set(OUTPUT_DIR "../../../bin/Release")
    endif()

else()
    set(OUTPUT_DIR "../../../bin/Debug")
endif()


if (NOT EMSCRIPTEN)
    file(COPY ../assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../../../bin/${CMAKE_BUILD_TYPE})
endif ()


set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        OUTPUT_NAME ${EXECUTABLE_NAME}
        ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EXECUTABLE_NAME})

target_include_directories(${EXECUTABLE_NAME} PRIVATE .)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE libs)
if(MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /W3)
else()
    target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall)
endif()
target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_23)