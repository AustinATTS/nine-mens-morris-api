cmake_minimum_required(VERSION 3.16)
project(NineMensMorrisAPI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cc")

add_executable(MuehleBridge ${CORE_SOURCES})

target_include_directories(MuehleBridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)