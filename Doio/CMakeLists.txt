cmake_minimum_required(VERSION 3.20)
project(doio_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies via vcpkg ────────────────────────────────────────────────────
# Install with: vcpkg install imgui[core,dx11-binding,win32-binding] nlohmann-json
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/KeyboardConfig.cpp
    src/KeyboardEditor.cpp
)

set(HEADERS
    src/KeyboardConfig.h
    src/KeyboardEditor.h
    src/KeycodeDefs.h
)

# ── Target ───────────────────────────────────────────────────────────────────
add_executable(doio_editor WIN32 ${SOURCES} ${HEADERS})

target_include_directories(doio_editor PRIVATE src)

target_link_libraries(doio_editor PRIVATE
    imgui::imgui
    nlohmann_json::nlohmann_json
    d3d11
    dxgi
    comdlg32
    shlwapi
)

# VS2022: set the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY VS_STARTUP_PROJECT doio_editor)

# Copy sample JSON files next to the executable if they exist
foreach(f design.json me.json)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${f}")
        configure_file("${CMAKE_SOURCE_DIR}/${f}"
                       "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${f}" COPYONLY)
    endif()
endforeach()
