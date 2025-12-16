cmake_minimum_required(VERSION 3.25)

# Enable automatic DLL copying
cmake_policy(SET CMP0087 NEW)

project(wav-audio-player
    VERSION 1.0.0
    DESCRIPTION "A simple WAV audio player using Minimal Audio Engine"
    LANGUAGES CXX
)

find_package(minimal-audio-engine REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

# Installation settings
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS wav-audio-player
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install runtime dependencies (DLLs) on Windows
if(WIN32)
    # Install all DLLs from vcpkg bin directory
    file(GLOB VCPKG_DLLS "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/*.dll")
    install(
        FILES ${VCPKG_DLLS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
    
    # Also try to install target runtime DLLs (for any non-vcpkg dependencies)
    install(
        FILES
            $<TARGET_RUNTIME_DLLS:wav-audio-player>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()
