cmake_minimum_required(VERSION 3.12)
project(SECYAN LANGUAGES CXX)
cmake_policy(SET CMP0077 NEW)

option(SECYAN_TEST "Build tests" OFF)
set(SECYAN_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SECYAN_BINARY_ROOT "${CMAKE_CURRENT_BINARY_DIR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Use shared boost libraries" FORCE)
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use shared boost runtime" FORCE)

find_package(Boost COMPONENTS system filesystem program_options thread REQUIRED)
# find_package(Threads REQUIRED)

set(ENABLE_RELIC ON CACHE BOOL "Build relic by default" FORCE)
set(MULTI PTHREAD CACHE STRING "Build relic with multithreading from pthread" FORCE)
set(ARITH easy CACHE STRING "Build relic without gmp" FORCE)
set(WITH "MD;DV;BN;FB;FP;EB;EC;EP" CACHE STRING "Overwrite Relic algorithms in ABY with their superset" FORCE)

find_package(ABY QUIET)
set(ENCRYPTO_UTILS_BUILD_TESTS OFF CACHE BOOL "Disable tests for the ENCRYPTO utils library" FORCE)
if (ABY_FOUND)
    message(STATUS "Found ABY")
elseif (NOT ABY_FOUND AND NOT TARGET ABY::aby)
    message("ABY was not found: add ABY subdirectory")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/ABY/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/ABY")
        execute_process(COMMAND git submodule update --init extern/ABY
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    add_subdirectory(extern/ABY)
endif ()

# relic is already compiled by ENCRYPTO_utils, which is ABY's dependency
set(RLC_LIBRARY "${SECYAN_BINARY_ROOT}/extern/ABY/lib/librelic_s.a")

file(COPY
        "${PROJECT_SOURCE_DIR}/extern/ABY/extern/ENCRYPTO_utils/extern/relic/include/"
        "${SECYAN_BINARY_ROOT}/extern/ABY/extern/ENCRYPTO_utils/extern/relic/include/relic_conf.h"
    DESTINATION
        "${PROJECT_BINARY_DIR}/include/relic"
)

# we need to set these compile flags globally to compile libOTe and its dependencies
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -pthread -maes -msse2 -msse3 -msse4.1 -mpclmul -mavx -mavx2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")

set(ENABLE_SIMPLESTOT ON CACHE BOOL "Enable Simplest OT for Base OTs" FORCE)
set(ENABLE_IKNP ON CACHE BOOL "Enable IKNP for 1-out-of-2 OTs" FORCE)
set(ENABLE_KKRT ON CACHE BOOL "Enable KKRT for OPRFs" FORCE)

find_package(libOTe QUIET)
if (libOTe_FOUND)
    message(STATUS "Found libOTe")
elseif (NOT libOTe_FOUND AND NOT TARGET libOTe::libote)
    message("libOTe was not found: add libOTe subdirectory")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/libOTe/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/libOTe")
        execute_process(COMMAND git submodule update --init --recursive extern/libOTe
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    add_subdirectory(extern/libOTe)
endif ()

add_dependencies(libOTe relic_s)

add_subdirectory(src/core)
add_subdirectory(src/example)

if(SECYAN_TEST)
    add_subdirectory(src/test)
endif(SECYAN_TEST)