target_sources(sparkle-engine
	PUBLIC
		Config.h
		COnfig.cpp
		FileReader.h
		FileReader.cpp
		Util.h
		Util.cpp
)

target_include_directories(sparkle-engine PRIVATE ${CMAKE_CURRENT_LIST_DIR})