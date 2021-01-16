cmake_minimum_required(VERSION 3.11)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(sparkle-engine VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

### CONAN
# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(
    BUILD_TYPE "Release"
    CONANFILE conanfile.txt  # or relative build/conanfile.txt
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

set(CMAKE_INSTALL_PREFIX "./install")
cmake_policy(SET CMP0076 NEW)

### GLFW
find_package(glfw3 REQUIRED)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)
FetchContent_Declare(
	gltf
	GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
)

find_package(Vulkan REQUIRED)

set(LINK_LIBRARIES
	glfw::glfw
	${Vulkan_LIBRARIES}
)

# Sparkle Engine
add_executable(${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
add_subdirectory(src/Core)
add_subdirectory(src/Import)

### INCLUDE
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/glm-src)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/rapidjson/include)
# Additional Include Directories
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/external")
target_include_directories(${PROJECT_NAME} PRIVATE "${Vulkan_INCLUDE_DIR}")

set(LINKLIBRARIES ${LINK_LIBRARIES})