target_sources(sparkle-engine
	PUBLIC
		Application.h
		Application.cpp
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(Common)
add_subdirectory(Input)
add_subdirectory(Utilities)
add_subdirectory(VkRenderer)