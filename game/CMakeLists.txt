cmake_minimum_required(VERSION 3.0.0)

# Project name
project(Daxanadu)

# Define LINUX
if (UNIX AND NOT APPLE)
    set(LINUX 1)
endif()

# Setup modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Some compiler flags
set(CMAKE_CXX_STANDARD 17)
if (MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP /FS /std:c++latest")
endif()

# Define _DEBUG
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
endif()

#justwindowsthings
if (WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Project files
file(GLOB srcfiles ./src/*.*)
file(GLOB apfiles ./src/Archipelago/*.*)
file(GLOB hardwarefiles ./src/Hardware/*.*)
file(GLOB inputsfiles ./src/Inputs/*.*)
list(APPEND srcfiles ../thirdparty/MCS6502/MCS6502.c)
list(APPEND srcfiles ../thirdparty/MCS6502/MCS6502.h)
list(APPEND includes PUBLIC
    ./src/
    ./src/Archipelago/
    ./src/Hardware/
    ./src/Inputs/
    ../thirdparty/MCS6502/
)

# Onut
option(ASSIMP_BUILD_ASSIMP_TOOLS "" OFF)
option(ASSIMP_BUILD_TESTS "" OFF)
list(APPEND libs PUBLIC libonut)
list(APPEND includes PUBLIC ../thirdparty/onut/include/)

# APCpp
option(USE_ZLIB "" OFF)
option(ENABLE_PROGRAMS "" OFF)
add_subdirectory(../thirdparty/APCpp APCpp)
list(APPEND libs PUBLIC APCpp)
list(APPEND includes PUBLIC ../thirdparty/APCpp/)

# Source groups for Visual Studio
source_group("thirdparty" FILES ${srcthirdparty})
source_group("src" FILES ${srcfiles})
source_group("Archipelago" FILES ${apfiles})
source_group("Hardware" FILES ${hardwarefiles})
source_group("Inputs" FILES ${inputsfiles})

# .exe
add_executable(${PROJECT_NAME} ${srcfiles} ${srcthirdparty} ${apfiles} ${hardwarefiles} ${inputsfiles})

# Work dir
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

# Lib/Headers
target_include_directories(${PROJECT_NAME} PUBLIC ${includes})
target_link_libraries(${PROJECT_NAME} PUBLIC ${libs})
