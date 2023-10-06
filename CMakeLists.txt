cmake_minimum_required(VERSION 3.0.0)
project(Dax)

# Onut
option(ONUT_USE_SDL "" OFF)
add_subdirectory(./thirdparty/onut onut)

# Tool
add_subdirectory(tool)

# Game
add_subdirectory(game)
