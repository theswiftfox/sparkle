target_sources(sparkle-engine
	PUBLIC
		AppSettings.h
		AppSettings.cpp
		FileReader.h
		FileReader.cpp
		Util.h
		Util.cpp
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})