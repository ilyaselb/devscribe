cmake_minimum_required(VERSION 3.15)
project(DevScribe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Add your headers here just so they show up in IDEs (VS/CLion)
add_executable(DevScribe
    src/main.cpp
    src/Note.hpp       # Added
    src/NoteManager.hpp # Added
    ${IMGUI_SOURCES}
)

target_include_directories(DevScribe PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    src               # Add src to include path
)

target_link_libraries(DevScribe
    OpenGL::GL
    glfw
)

# Windows specific linking
if(WIN32)
    target_link_libraries(DevScribe opengl32)
endif()