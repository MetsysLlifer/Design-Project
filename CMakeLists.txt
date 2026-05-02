cmake_minimum_required(VERSION 3.16)
project(DSAVisualizer C)

set(CMAKE_C_STANDARD 99)

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c")

add_executable(dsa_visualizer ${SRC_FILES})

target_include_directories(dsa_visualizer PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Prefer a CMake package (installed raylib). If not found, fall back to vendor/raylib layout.
find_package(raylib CONFIG QUIET)

if (raylib_FOUND)
    target_link_libraries(dsa_visualizer PRIVATE raylib)
else()
    if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/raylib")
        message(STATUS "Using vendored raylib in ${CMAKE_SOURCE_DIR}/vendor/raylib")
        target_include_directories(dsa_visualizer PRIVATE "${CMAKE_SOURCE_DIR}/vendor/raylib/include")

        if (WIN32)
            file(GLOB RAYLIB_LIBS "${CMAKE_SOURCE_DIR}/vendor/raylib/lib/*.lib" "${CMAKE_SOURCE_DIR}/vendor/raylib/lib/*.a")
            target_link_libraries(dsa_visualizer PRIVATE ${RAYLIB_LIBS})
        else()
            file(GLOB RAYLIB_LIBS "${CMAKE_SOURCE_DIR}/vendor/raylib/lib/*.a" "${CMAKE_SOURCE_DIR}/vendor/raylib/lib/*.so" "${CMAKE_SOURCE_DIR}/vendor/raylib/lib/*.dylib")
            target_link_libraries(dsa_visualizer PRIVATE ${RAYLIB_LIBS})
            if (APPLE)
                target_link_libraries(dsa_visualizer PRIVATE "-framework CoreVideo" "-framework IOKit" "-framework Cocoa" "-framework GLUT" "-framework OpenGL")
            endif()
        endif()
    else()
        message(WARNING "raylib not found. Set RAYLIB_DIR or place a prebuilt raylib under vendor/raylib.")
    endif()
endif()

# Allow user to override install/library locations via -DRAYLIB_DIR=...
if (DEFINED RAYLIB_DIR AND NOT raylib_FOUND)
    message(STATUS "RAYLIB_DIR provided: ${RAYLIB_DIR}")
    target_include_directories(dsa_visualizer PRIVATE "${RAYLIB_DIR}/include")
    if (WIN32)
        file(GLOB RAY_LIBS "${RAYLIB_DIR}/lib/*.lib" "${RAYLIB_DIR}/lib/*.a")
    else()
        file(GLOB RAY_LIBS "${RAYLIB_DIR}/lib/*.a" "${RAYLIB_DIR}/lib/*.so" "${RAYLIB_DIR}/lib/*.dylib")
    endif()
    if (RAY_LIBS)
        target_link_libraries(dsa_visualizer PRIVATE ${RAY_LIBS})
    endif()
endif()
