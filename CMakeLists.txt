cmake_minimum_required(VERSION 3.16)

option(STATIC_RUNTIME "Tie runtime libraries to program, so it does not depend on dlls in path or beside it" TRUE) # pro: binary independent of dlls, so e.g. no MSVC runtime libraries prerequisites for users - con: if runtime library contains errors, program has to be recompiled instead of being able to replace the DLL
if(STATIC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

project(ICR2FFB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

include(cmake/RecursiveDirList.cmake)
RECURSIVE_DIR_LIST(INCLUDE_DIRS src)
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PROJECT_NAME} PUBLIC "-static")
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus /Zc:inline /Zc:preprocessor) # C++ standard compliance
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4514 /wd4626 /wd4668 /wd4710 /wd4711 /wd4820 /wd5027 /wd5039 /wd5045 /wd5105) # tested on MSVC 2019
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4127 /wd4265 /wd4350 /wd4512 /wd4625 /wd4571) # tested on MSVC 2013
    target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Weverything
            -Wno-padded
            -Wno-covered-switch-default
            -Wno-zero-as-null-pointer-constant # C++98 has no nullptr, therefore NULL style
            -Wno-c++98-compat # "solved" but still emitted
        )
        if(CMAKE_CXX_STANDARD STREQUAL "98")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -Wno-c++11-extensions
                -Wno-suggest-override
                -Wno-suggest-destructor-override
            )
        endif()
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE DIRECTINPUT_VERSION=0x0800 _WIN32_WINNT=0x501) # _WIN32_WINNT_WINXP
target_link_libraries(${PROJECT_NAME} PRIVATE dinput8 winmm)
set(EXE_EXTENSION .exe)
set(PROJECT_BINARY_NAME ${PROJECT_NAME}${EXE_EXTENSION})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_BINARY_NAME} DESTINATION .) # instead of TARGETS because with MinGW cross-compile its fails with "file INSTALL cannot find ${PROJECT_BINARY_NAME}" as its missing the .exe extension
# copy ini file - this may be outdated if a setting is changed in FFBConfig, so consider removing this and let the first program run generate it, directing the user appropiately
# or adding an install step that does does this instead
install(CODE 
     "
     set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffb.ini)
     set(DEST_DIR ${CMAKE_INSTALL_PREFIX}/ffb.ini)
     if(${SRC_DIR} IS_NEWER_THAN ${DEST_DIR})
         file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ffb.ini DESTINATION ${CMAKE_INSTALL_PREFIX})
         message(STATUS \"ffb.ini copied\")
     else()
         message(STATUS \"ffb.ini kept\")
     endif()
    ")
install(FILES LICENSE README.md DESTINATION .)

# "determine" compiler
get_filename_component(TOOLCHAIN_ROOT ${CMAKE_CXX_COMPILER} DIRECTORY)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # MinGW GCC - tested with GCC 10
    string(FIND ${CMAKE_CXX_COMPILER} "i686" STR_POS)
    if(${STR_POS} EQUAL -1)
        set(TARGET_PLATFORM "64bit")
    else()
        set(TARGET_PLATFORM "32bit")
    endif()

    find_package(Threads)
    if((NOT CMAKE_HAVE_LIBC_PTHREAD EQUAL 1))
        set(MINGW_STDTHREADS_GENERATE_STDHEADERS ON CACHE BOOL "MinGW GCC std::thread and std::mutex implementation" FORCE)
        add_subdirectory(thirdparty/mingw-std-threads)
        target_link_libraries(${PROJECT_NAME} PRIVATE mingw_stdthreads)
        if(TARGET_PLATFORM STREQUAL "64bit")
            set(MINGW_LIBRARIES "${TOOLCHAIN_ROOT}/../lib/gcc/x86_64-w64-mingw32/10-win32")
        else()
            set(MINGW_LIBRARIES "${TOOLCHAIN_ROOT}/../lib/gcc/i686-w64-mingw32/10-win32")
        endif()
    else()
        set(MINGW_STDTHREADS_GENERATE_STDHEADERS OFF CACHE BOOL "MinGW GCC std::thread implementation" FORCE)
        set(MINGW_LIBRARIES "${TOOLCHAIN_ROOT}/../bin")
    endif()
    if(NOT STATIC_RUNTIME)
        if(TARGET_PLATFORM STREQUAL "64bit")
            install(FILES "${MINGW_LIBRARIES}/libgcc_s_seh-1.dll"
                    DESTINATION .
                    COMPONENT application)
        else()
            install(FILES "${MINGW_LIBRARIES}/libgcc_s_dw2-1.dll"
                    DESTINATION .
                    COMPONENT application)
        endif()
        install(FILES "${MINGW_LIBRARIES}/libstdc++-6.dll"
                DESTINATION .
                COMPONENT application)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # MinGW Clang
    string(FIND ${CMAKE_CXX_COMPILER} "i686" STR_POS)
    if(${STR_POS} EQUAL -1)
        set(TARGET_PLATFORM "64bit")
    else()
        set(TARGET_PLATFORM "32bit")
    endif()
    if(NOT STATIC_RUNTIME)
        if(TARGET_PLATFORM STREQUAL "64bit")
            set(MINGW_LIBRARIES "${TOOLCHAIN_ROOT}/../x86_64-w64-mingw32/bin")
        else()
            set(MINGW_LIBRARIES "${TOOLCHAIN_ROOT}/../i686-w64-mingw32/bin")
        endif()
        install(FILES "${MINGW_LIBRARIES}/libunwind.dll" "${MINGW_LIBRARIES}/libc++.dll"
        DESTINATION .
        COMPONENT application)
    endif()
elseif(MSVC)
    string(FIND ${CMAKE_CXX_COMPILER} "x64" STR_POS)
    if(${STR_POS} EQUAL -1)
        set(TARGET_PLATFORM "32bit")
    else()
        set(TARGET_PLATFORM "64bit")
    endif()
endif()

if(TARGET_PLATFORM STREQUAL "64bit")
    message(STATUS "64bit build determined")
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE)
else()
    message(STATUS "32bit build determined")
endif()

# PDB debug symbols with MSVC and MinGW Clang
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(IS_DEBUGABLE_BUILD TRUE)
else()
    set(IS_DEBUGABLE_BUILD FALSE)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND IS_DEBUGABLE_BUILD) # MinGW Clang
    target_compile_options(${PROJECT_NAME} PUBLIC -gcodeview)
    target_link_options(${PROJECT_NAME} PUBLIC -Wl,--pdb=)
endif()
if(MSVC AND IS_DEBUGABLE_BUILD)
    target_compile_options(${PROJECT_NAME} PUBLIC /Zi)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF")
endif()
if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR MSVC) AND IS_DEBUGABLE_BUILD)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pdb
            DESTINATION .
            COMPONENT debug)
endif()
