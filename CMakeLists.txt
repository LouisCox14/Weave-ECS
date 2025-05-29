cmake_minimum_required(VERSION 3.12)
project(WeaveECS CXX)

# Option to choose the ECS backend
set(ECS_BACKEND "Archetype" CACHE STRING "Choose ECS backend: SparseSet or Archetype")
set_property(CACHE ECS_BACKEND PROPERTY STRINGS SparseSet Archetype)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set base source directories
set(BASE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(UTILITIES_DIR "${BASE_SRC_DIR}/Utilities")

# Set backend source directory
if(ECS_BACKEND STREQUAL "SparseSet")
    set(BACKEND_SRC_DIR "${BASE_SRC_DIR}/SparseSet")
elseif(ECS_BACKEND STREQUAL "Archetype")
    set(BACKEND_SRC_DIR "${BASE_SRC_DIR}/Archetype")
else()
    message(FATAL_ERROR "Invalid ECS_BACKEND value: ${ECS_BACKEND}")
endif()

# Gather .cpp files directly in /src (non-recursive)
file(GLOB SRC_ROOT_FILES "${BASE_SRC_DIR}/*.cpp" "${BASE_SRC_DIR}/*.c")

# Gather backend-specific files (recursive)
file(GLOB_RECURSE BACKEND_SRC_FILES "${BACKEND_SRC_DIR}/*.cpp")

# Gather utility files (recursive)
file(GLOB_RECURSE UTILITIES_SRC_FILES "${UTILITIES_DIR}/*.cpp")

# Combine all source files
set(ALL_SRC_FILES
    ${SRC_ROOT_FILES}
    ${BACKEND_SRC_FILES}
    ${UTILITIES_SRC_FILES}
)

# Add library
add_library(WeaveECS STATIC ${ALL_SRC_FILES})

# Include directories
target_include_directories(WeaveECS PUBLIC
    "${BASE_SRC_DIR}"
    "${BACKEND_SRC_DIR}"
    "${UTILITIES_DIR}"
)
