cmake_minimum_required(VERSION 3.22)

project("WizardsAndLizards")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)


enable_language(CXX)

set(CMAKE_C_STANDARD 23)

set(LUMINOVEAU_BUILD_IMGUI ON)

if (EMSCRIPTEN)
    message("EMSCRIPTEN BUILD!!!")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -sUSE_PTHREADS -pthread -msimd128 -sUSE_SDL=3 -sUSE_SDL_MIXER=3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -sUSE_PTHREADS -pthread -msimd128 -sUSE_SDL=3 -sUSE_SDL_MIXER=3")
else ()
    if (MSVC)
        # MSVC: use /arch:AVX2 (equivalent to x86-64-v3 baseline)
        # Use CMAKE_CXX/C_FLAGS rather than add_compile_options so ASM_NASM is not affected.
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            string(APPEND CMAKE_CXX_FLAGS " /O2 /arch:AVX2")
            string(APPEND CMAKE_C_FLAGS   " /O2 /arch:AVX2")
            add_definitions(-DSDL_DISABLE_LOCKS)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
        else ()
            string(APPEND CMAKE_CXX_FLAGS " /arch:AVX2")
            string(APPEND CMAKE_C_FLAGS   " /arch:AVX2")
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
        endif ()
    else ()
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "-O3 -march=x86-64-v3")
            set(CMAKE_C_FLAGS "-O3 -march=x86-64-v3")
            set(CMAKE_POSITION_INDEPENDENT_CODE ON)
            add_definitions(-DSDL_DISABLE_LOCKS)
            if (UNIX)
                set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
            else()
                set(CMAKE_EXE_LINKER_FLAGS "-static -mwindows")
            endif()
        else ()
            set(CMAKE_CXX_FLAGS "-march=x86-64-v3")
            set(CMAKE_C_FLAGS "-march=x86-64-v3")
            if (UNIX)

            else()
                set(CMAKE_EXE_LINKER_FLAGS "-static")
            endif()
        endif ()
    endif ()
endif ()

add_subdirectory(libs)
add_subdirectory(src)

