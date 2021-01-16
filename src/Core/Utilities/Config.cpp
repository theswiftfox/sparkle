#include "Config.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <filesystem>
#include <fstream>
#include <string>

const std::string configFilePath = "settings.json";

using namespace sparkle;
bool Config::load()
{
	std::ifstream cfs(configFilePath);
	if (cfs.is_open()) {
		rapidjson::IStreamWrapper csw(cfs);

		rapidjson::Document cmap;
		cmap.ParseStream(csw);

		cfs.close();

		if (cmap.HasMember("resolution")) {
			const auto& res = cmap["resolution"];
			if (res.HasMember("width") && res.HasMember("height")) {
				resolution.width = res["width"].GetUint();
				resolution.height = res["height"].GetUint();
			}
		}
		if (cmap.HasMember("windowMode")) {
			windowMode = (WindowMode)cmap["windowMode"].GetUint();
		}

		return true;
	}
	return false;
}

bool Config::store()
{
	rapidjson::StringBuffer sb;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

	writer.StartObject();
	writer.Key("resolution");
	writer.StartObject();
	writer.Key("width");
	writer.Uint(resolution.width);
	writer.Key("height");
	writer.Uint(resolution.height);
	writer.EndObject(2);
	writer.Key("windowMode");
	writer.Uint((uint32_t)windowMode);
	writer.EndObject(2);

	std::ofstream ofs("settings.json");
	ofs << sb.GetString();
	ofs.flush();
	ofs.close();

	return true;
}