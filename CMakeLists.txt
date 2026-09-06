cmake_minimum_required(VERSION 3.11)
# agent: codex | 2026-09-06 | simplecraft scaffold | cb4fc0
project(simplecraft C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(RAYLIB_DIR /home/x/code/raylib)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "build type" FORCE)
endif()

if (PLATFORM STREQUAL "Web")
  set(GRAPHICS GRAPHICS_API_OPENGL_ES3 CACHE STRING "raylib graphics API" FORCE)
endif()

add_subdirectory(${RAYLIB_DIR} ${CMAKE_BINARY_DIR}/raylib EXCLUDE_FROM_ALL)
add_executable(${PROJECT_NAME} src/main.c src/app.c src/cli.c src/tynbox.c)
target_link_libraries(${PROJECT_NAME} raylib m)

if (PLATFORM STREQUAL "Web")
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
  target_link_options(${PROJECT_NAME} PRIVATE
    --shell-file ${CMAKE_SOURCE_DIR}/shell.html
    --preload-file ${CMAKE_SOURCE_DIR}/res@/res
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sINVOKE_RUN=1)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s FULL_ES3=1 -s WASM=1 -s ASYNCIFY")
else()
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
