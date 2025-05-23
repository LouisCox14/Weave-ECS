cmake_minimum_required(VERSION 3.12)
project(WeaveECS CXX)

# Option to choose the ECS backend
set(ECS_BACKEND "Archetype" CACHE STRING "Choose ECS backend: SparseSet or Archetype")
set_property(CACHE ECS_BACKEND PROPERTY STRINGS SparseSet Archetype)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory based on ECS_BACKEND
if(ECS_BACKEND STREQUAL "SparseSet")
    set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/SparseSet")
elseif(ECS_BACKEND STREQUAL "Archetype")
    set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Archetype")
else()
    message(FATAL_ERROR "Invalid ECS_BACKEND value: ${ECS_BACKEND}")
endif()

# Gather .cpp files from the selected ECS source directory
file(GLOB_RECURSE ECS_SRC_FILES "${SRC_DIR}/*.cpp")

# Add library
add_library(WeaveECS STATIC ${ECS_SRC_FILES})

# Include directories
target_include_directories(WeaveECS PUBLIC "${SRC_DIR}")