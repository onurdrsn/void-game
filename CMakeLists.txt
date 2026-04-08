cmake_minimum_required(VERSION 3.20)
project(void_game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/vmath.cpp
    src/platform.cpp
    src/renderer.cpp
    src/audio.cpp
    src/world.cpp
    src/entity.cpp
    src/game.cpp
)

add_executable(void_game ${SOURCES})
target_include_directories(void_game PRIVATE src)

# ── Platform-specific flags ──────────────────────────────────────────────────

if(WIN32)
    # MSVC
    if(MSVC)
        target_compile_options(void_game PRIVATE
            /O2 /Oi /Ot /GL /GR- /EHs-c- /W3
            /D_CRT_SECURE_NO_WARNINGS
        )
        target_link_options(void_game PRIVATE /LTCG /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
        target_link_libraries(void_game PRIVATE opengl32 gdi32 user32 kernel32 winmm)
    else()
        # MinGW
        target_compile_options(void_game PRIVATE
            -O2 -fno-exceptions -fno-rtti -fno-stack-protector
            -ffunction-sections -fdata-sections -mwindows
        )
        target_link_options(void_game PRIVATE -Wl,--gc-sections -s -mwindows)
        target_link_libraries(void_game PRIVATE opengl32 gdi32 user32 winmm)
    endif()

elseif(UNIX AND NOT APPLE)
    # Linux / Arch Linux WSL2 — PRIMARY PLATFORM
    # if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    #     target_compile_options(void_game PRIVATE
    #         -g -O0 -fno-exceptions -fno-rtti -fno-stack-protector
    #         -ffunction-sections -fdata-sections
    #         -Wall -Wextra -Wno-unused-parameter
    #     )
    #     target_link_options(void_game PRIVATE -Wl,--gc-sections)
    #     message(STATUS "Debug build: symbols included, UPX disabled")
    # else()
        target_compile_options(void_game PRIVATE
            -O2 -fno-exceptions -fno-rtti -fno-stack-protector
            -ffunction-sections -fdata-sections
            -Wall -Wextra -Wno-unused-parameter
        )
        target_link_options(void_game PRIVATE -Wl,--gc-sections -s)
    # endif()
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(void_game PRIVATE OpenGL::GL ${X11_LIBRARIES} m dl pthread)
    target_include_directories(void_game PRIVATE ${X11_INCLUDE_DIR})
endif()

# ── UPX compression post-build (only Release) ───────────────────────────────
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(UPX_EXECUTABLE upx)
    if(UPX_EXECUTABLE)
        add_custom_command(TARGET void_game POST_BUILD
            COMMAND ${UPX_EXECUTABLE} --best --lzma
                    $<TARGET_FILE:void_game>
            COMMENT "Compressing executable with UPX..."
            VERBATIM
        )
        message(STATUS "UPX found: ${UPX_EXECUTABLE} — will compress output")
    else()
        message(WARNING "UPX not found — executable will not be compressed. Run: sudo pacman -S upx")
    endif()
endif()
