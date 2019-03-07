target_sources(sparkle-engine
	PUBLIC
		RenderBackend.h
		RenderBackend.cpp
		Common/Shader.h
		Common/Shader.cpp
		Common/SparkleTypes.h
		Common/VulkanInitializers.h
		Compute/ComputePipeline.h
		Compute/ComputePipeline.cpp
		Draw/GraphicsPipeline.h
		Draw/GraphicsPipeline.cpp
		Draw/UI.h
		Draw/UI.cpp
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Common)
target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Compute)
target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Draw)