cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()
project(sparkle-engine VERSION 0.1 LANGUAGES CXX)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (NOT MSVC)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libstdc++ -fms-extensions")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
	endif()
endif()

find_package(Vulkan REQUIRED)

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found!")
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdParty/glfw)

# chakraCore
if(NOT MSVC)
	if (BUILD_CHAKRA_CORE)
		set(PREV_BUILD_TYPE ${CMAKE_BUILD_TYPE})
		set(CMAKE_BUILD_TYPE Release)
		set(CC_TARGETS_AMD64_SH ON CACHE BOOL "" FORCE)
		set(SHARED_LIBRARY_SH ON CACHE BOOL "" FORCE)
		set(STATIC_LIBRARY_SH OFF CACHE BOOL "" FORCE)
		set(LIBS_ONLY_BUILD_SH ON CACHE BOOL "" FORCE)
		set(NO_ICU_SH ON CACHE BOOL "" FORCE)
		add_subdirectory(3rdParty/ChakraCore)

		set(CMAKE_BUILD_TYPE ${PREV_BUILD_TYPE})

		set(CHAKRA_LIB ChakraCore)
	else()
		set(CHAKRA_LIB ${CMAKE_SOURCE_DIR}/3rdParty/prebuilt/ChakraCore/x64_release/libChakraCore.so)
	endif()
else()
	set(CHAKRA_LIB ${CMAKE_SOURCE_DIR}/3rdParty/prebuilt/ChakraCore/x64_release/ChakraCore.lib)
endif()

# on windows set the environment variable ASSIMP_ROOT_DIR to the folder containing assimp bin and include folders
find_package(assimp REQUIRED)

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "includedir='${dir}'")
# endforeach()


set(SOURCE_LIST
	src/main.cpp

	3rdParty/imgui/imgui.cpp
	3rdParty/imgui/imgui_draw.cpp
	3rdParty/imgui/imgui_widgets.cpp
)

# Sparkle Engine
add_executable(${PROJECT_NAME} ${SOURCE_LIST})
add_subdirectory(src/Core)
add_subdirectory(src/Import)

set(SHADERS
	shaders/ui.frag
	shaders/ui.vert
	shaders/deferred.vert.hlsl
	shaders/deferred.frag.hlsl
	shaders/MRT.vert.hlsl
	shaders/MRT.frag.hlsl
	shaders/cull.comp
)

set(ASSETS
	assets/settings.ini
	assets/materials/default/diff.png
	assets/materials/default/spec.png
)

# Additional Include Directories
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/gli")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/glm")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/glfw/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/simpleini")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/stb")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/ChakraCore/lib/")
target_include_directories(${PROJECT_NAME} PRIVATE "${Vulkan_INCLUDE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${ASSIMP_INCLUDE_DIR}")

set(LINKLIBRARIES ${Vulkan_LIBRARIES} glfw ${ASSIMP_LIBRARY_RELEASE})
if (MSVC) 
	target_link_libraries(${PROJECT_NAME} ${LINKLIBRARIES} ${CHAKRA_LIB})
else()
	target_link_libraries(${PROJECT_NAME} ${LINKLIBRARIES} ${CHAKRA_LIB} stdc++fs)
endif()

if (WIN32)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSIMP_ROOT_DIR}/bin/assimp-${ASSIMP_MSVC_VERSION}-mt.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/3rdParty/prebuilt/ChakraCore/x64_release/ChakraCore.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
endif()

foreach(SHADER ${SHADERS})
	get_filename_component(FILE_NAME ${SHADER} NAME)
	set(output_file ${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv)
	set(compiled_shaders ${compiled_shaders} ${output_file})

	add_custom_command(
		OUTPUT ${output_file}
		COMMAND ${GLSLANG_VALIDATOR} -e main -w -V ${CMAKE_SOURCE_DIR}/${SHADER} -o ${output_file}
		DEPENDS ${CMAKE_SOURCE_DIR}/${SHADER}
		COMMENT "Compiling shader ${output_file}"
	)
endforeach()
	
add_custom_target(shaders-${PROJECT_NAME} ALL DEPENDS ${compiled_shaders})
add_custom_command(TARGET shaders-${PROJECT_NAME} COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders COMMENT "Create shader bin dir")

add_dependencies(${PROJECT_NAME} shaders-${PROJECT_NAME})


add_custom_target(assets-${PROJECT_NAME} ALL DEPENDS ${OUT_ASSETS})
foreach(ASSET ${ASSETS})
	set(ASSET_O ${CMAKE_CURRENT_BINARY_DIR}/${ASSET})
	set(OUT_ASSETS ${OUT_ASSETS} )
	add_custom_command(
		TARGET assets-${PROJECT_NAME} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/${ASSET} ${ASSET_O}
	)
endforeach()
add_dependencies(${PROJECT_NAME} assets-${PROJECT_NAME})
