#include "Config.h"

#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <filesystem>
#include <fstream>
#include <string>

constexpr char defaultConfigFilePath[] = "settings.json";

using namespace Sparkle;
bool Config::load()
{
	return load(defaultConfigFilePath);
}

bool Config::load(std::string cfgPath)
{
	std::ifstream cfs(cfgPath);
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
		if (cmap.HasMember("levelPath")) {
			levelPath = std::string(cmap["levelPath"].GetString());
		}
		if (cmap.HasMember("vkValidation")) {
			validationLayers = cmap["vkValidation"].GetBool();
		}

		return true;
	}
	return false;
}

bool Config::store()
{
	return store(defaultConfigFilePath);
}

bool Config::store(std::string cfgPath)
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
	writer.Key("level");
	writer.String(levelPath);
	writer.Key("vkValidation");
	writer.Bool(validationLayers);

	std::ofstream ofs(cfgPath);
	ofs << sb.GetString();
	ofs.flush();
	ofs.close();

	return true;
}