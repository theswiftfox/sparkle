target_sources(sparkle-engine
	PUBLIC
		Application.h
		Application.cpp
		ChakraJS/SparkleChakraCore.h
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ChakraJS)

add_subdirectory(Common)
add_subdirectory(Input)
add_subdirectory(Utilities)
add_subdirectory(VkRenderer)