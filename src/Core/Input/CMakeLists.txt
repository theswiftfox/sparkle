target_sources(sparkle-engine
	PUBLIC
		InputController.h
		InputController.cpp
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})