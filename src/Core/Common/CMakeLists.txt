target_sources(sparkle-engine
	PUBLIC
		Camera.h
		Camera.cpp
		Lights.h
		Lights.cpp
		Material.h
		Material.cpp
		Texture.h
		Texture.cpp
		Scene/Geometry.h
		Scene/Geometry.cpp		
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Scene)