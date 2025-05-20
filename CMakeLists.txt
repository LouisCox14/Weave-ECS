cmake_minimum_required(VERSION 3.12)
project(ECSComparison CXX)

# Option to choose the ECS backend
set(ECS_BACKEND "Archetype" CACHE STRING "Choose ECS backend: SparseSet or Archetype")
set_property(CACHE ECS_BACKEND PROPERTY STRINGS SparseSet Archetype)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory based on ECS_BACKEND
if(ECS_BACKEND STREQUAL "SparseSet")
    set(SRC_DIR "${CMAKE_SOURCE_DIR}/src/SparseSet")
elseif(ECS_BACKEND STREQUAL "Archetype")
    set(SRC_DIR "${CMAKE_SOURCE_DIR}/src/Archetype")
else()
    message(FATAL_ERROR "Invalid ECS_BACKEND value: ${ECS_BACKEND}")
endif()

# Gather .cpp files from the selected ECS source directory
file(GLOB_RECURSE ECS_SRC_FILES "${SRC_DIR}/*.cpp")

# Always gather .cpp files from src/Utilities
file(GLOB_RECURSE UTIL_SRC_FILES "${CMAKE_SOURCE_DIR}/src/Utilities/*.cpp")

# Combine all source files
set(SRC_FILES ${ECS_SRC_FILES} ${UTIL_SRC_FILES})

# Add library
add_library(ECSComparison STATIC ${SRC_FILES})

# Include directories
target_include_directories(ECSComparison PRIVATE "${SRC_DIR}" "${CMAKE_SOURCE_DIR}/src/Utilities")