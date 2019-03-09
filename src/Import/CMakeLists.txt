target_sources(sparkle-engine
	PUBLIC
		AssimpLoader.h
		AssimpLoader.cpp
		GltfLoader.h
		GltfLoader.cpp
		SceneLoader.h
		SceneLoader.cpp
)

target_include_directories(sparkle-engine PUBLIC ${CMAKE_CURRENT_LIST_DIR})