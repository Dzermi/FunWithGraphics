cmake_minimum_required(VERSION 3.15)
project(Grafika)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

# 1. SFML
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)

# 2. Dear ImGui (samo jądro)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9
)
FetchContent_MakeAvailable(imgui)

# 3. ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Katalog ze zrodlami ImGui" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Wylacz find_package dla SFML" FORCE)

FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(ImGui-SFML)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# 4. OpenGL
find_package(OpenGL REQUIRED)

# 5. Plik wykonywalny
add_executable(Grafika src/main.cpp)

target_link_libraries(Grafika PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
    OpenGL::GL
)

if(WIN32)
    add_custom_command(TARGET Grafika POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:Grafika>
        $<TARGET_FILE_DIR:Grafika>
        COMMAND_EXPAND_LISTS
    )
endif()