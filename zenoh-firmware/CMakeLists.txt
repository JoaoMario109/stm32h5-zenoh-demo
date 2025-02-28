cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME zenoh-firmware)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C/C++ languages
enable_language(C ASM CXX)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

get_target_property(CUBE_MODULE_INCLUDES stm32cubemx INTERFACE_INCLUDE_DIRECTORIES)
if(NOT CUBE_MODULE_INCLUDES)
    set(CUBE_MODULE_INCLUDES "")
endif()

# Third party libraries fetch
include(FetchContent)

#  - FreeRTOS-Plus-TCP
FetchContent_Declare(freertos_plus_tcp
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git
  GIT_TAG        main
  GIT_SUBMODULES ""
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc"
)
target_compile_options(freertos_config INTERFACE -Wno-error)

set(CMAKE_SYSTEM_NAME "Generic" CACHE STRING "" FORCE)
set(FREERTOS_PLUS_TCP_COMPILER "GCC" CACHE STRING "" FORCE)
set(FREERTOS_PLUS_TCP_NETWORK_IF "STM32" CACHE STRING "" FORCE)
set(FREERTOS_PLUS_TCP_BUFFER_ALLOCATION "2" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(freertos_plus_tcp)

# Ad CubeMX is the one providing the FreeRTOS kernel and configuration, we alias the target
add_library(freertos_kernel ALIAS stm32cubemx)

target_compile_definitions(freertos_plus_tcp PRIVATE STM32H563xx)
target_include_directories(freertos_plus_tcp PRIVATE ${CUBE_MODULE_INCLUDES})

#  - Zenoh Pico
FetchContent_Declare(zenohpico
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-pico.git
  GIT_TAG        main
  GIT_SUBMODULES ""
)

# Building for FreeRTOS
set(WITH_FREERTOS_PLUS_TCP "ON" CACHE BOOL "" FORCE)

# We want the static library
set(PACKAGING "OFF" CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS "OFF" CACHE BOOL "" FORCE)

# No threads
set(CHECK_THREADS 0 FORCE)
set(THREADS_PREFER_PTHREAD_FLAG "OFF" CACHE BOOL "" FORCE)

# No tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Not supported right now
set(Z_FEATURE_LINK_UDP_MULTICAST 0 CACHE BOOL "" FORCE)
set(Z_FEATURE_MULTI_THREAD 0 CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zenohpico)

include("${zenohpico_SOURCE_DIR}/cmake/helpers.cmake")

configure_include_project(ZENOHPICO zenohpico zenohpico::lib "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

# Edit the Zenoh Pico CMakeLists.txt to remove any set(CHECK_THREADS ON) line to set(CHECK_THREADS 0)
set(ZENOH_PICO_CMAKE "${zenohpico_SOURCE_DIR}/CMakeLists.txt")
file(READ "${ZENOH_PICO_CMAKE}" ZENOH_PICO_CMAKE_CONTENTS)
string(REPLACE "set(CHECK_THREADS \"ON\")" "set(CHECK_THREADS 0)" ZENOH_PICO_CMAKE_CONTENTS "${ZENOH_PICO_CMAKE_CONTENTS}")
file(WRITE "${ZENOH_PICO_CMAKE}" "${ZENOH_PICO_CMAKE_CONTENTS}")
message(STATUS "Patched Zenoh Pico: Disabled CHECK_THREADS in CMakeLists.txt")

target_link_libraries(zenohpico_static
  freertos_kernel
  freertos_plus_tcp
)

target_include_directories(stm32cubemx INTERFACE
  "${freertos_plus_tcp_SOURCE_DIR}/source/include/"
  "${zenohpico_SOURCE_DIR}/include/"
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    freertos_plus_tcp
    zenohpico_static
)

# Create binary and hex files
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating binary and hex file"
)
