cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(sparkle-engine VERSION 0.1 LANGUAGES CXX)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (NOT MSVC)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		add_compile_options("-stdlib=libstdc++")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fms-extensions")
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
set(PREV_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
set(CC_TARGETS_AMD64_SH ON CACHE BOOL "" FORCE)
set(SHARED_LIBRARY_SH ON CACHE BOOL "" FORCE)
set(STATIC_LIBRARY_SH OFF CACHE BOOL "" FORCE)
set(LIBS_ONLY_BUILD_SH ON CACHE BOOL "" FORCE)
set(NO_ICU_SH ON CACHE BOOL "" FORCE)
add_subdirectory(3rdParty/ChakraCore)

set(CMAKE_BUILD_TYPE ${PREV_BUILD_TYPE})
else()
	# TODO: include msbuild step here instead of prebuilt libs?
endif()

# on windows set the environment variable ASSIMP_ROOT_DIR to the folder containing assimp bin and include folders
find_package(assimp REQUIRED)

include_directories("${CMAKE_SOURCE_DIR}/src/include")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/gli")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/glm")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/glfw/include")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/simpleini")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/stb")
include_directories("${CMAKE_SOURCE_DIR}/3rdParty/ChakraCore/lib/")
include_directories("${Vulkan_INCLUDE_DIR}")
include_directories("${ASSIMP_INCLUDE_DIR}")

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "includedir='${dir}'")
# endforeach()

set(SRC
	src/Application.cpp
	src/AppSettings.cpp
	src/Camera.cpp
	src/FileReader.cpp
	src/Geometry.cpp
	src/main.cpp
	src/GraphicsPipeline.cpp
	src/InputController.cpp
	src/Lights.cpp
	src/Material.cpp
	src/RenderBackend.cpp
	src/Shader.cpp
	src/Texture.cpp
	src/UI.cpp

	3rdParty/imgui/imgui.cpp
	3rdParty/imgui/imgui_draw.cpp
	3rdParty/imgui/imgui_demo.cpp
	3rdParty/imgui/imgui_widgets.cpp
)

set(HDR
	src/include/Application.h
	src/include/AppSettings.h
	src/include/Camera.h
	src/include/FileReader.h
	src/include/Geometry.h
	src/include/GraphicsPipeline.h
	src/include/InputController.h
	src/include/Lights.h
	src/include/Material.h
	src/include/RenderBackend.h
	src/include/Shader.h
	src/include/Texture.h
	src/include/UI.h
)

set(SHADERS
	shaders/scene.frag
	shaders/scene.vert
	shaders/ui.frag
	shaders/ui.vert
	shaders/deferred.vert.hlsl
	shaders/deferred.frag.hlsl
)

set(ASSETS
	assets/settings.ini
	assets/materials/default/diff.png
	assets/materials/default/spec.png
)

add_executable(${PROJECT_NAME} ${SRC} ${HDR})
set(LINKLIBRARIES ${Vulkan_LIBRARIES} glfw ${ASSIMP_LIBRARY_RELEASE})
if (MSVC) 
target_link_libraries(${PROJECT_NAME} ${LINKLIBRARIES} ${CMAKE_SOURCE_DIR}/3rdParty/prebuilt/ChakraCore/x64_release/ChakraCore.lib)
else()
target_link_libraries(${PROJECT_NAME} ${LINKLIBRARIES} ChakraCore stdc++fs)
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
