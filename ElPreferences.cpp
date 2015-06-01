#include "ElPreferences.h"
#include "Utils\Macros.h"
#include <yaml-cpp\yaml.h>

#include <iostream>
#include <fstream>

const char *PreferencesFileName = "dewrito_prefs.yaml";

namespace
{
	void parsePlayerData(ElPreferences *prefs, const YAML::Node &player);
	void parseArmorData(ElPreferences *prefs, const YAML::Node &armor);
	void parseColorData(ElPreferences *prefs, const YAML::Node &colors);
	void parseVideoData(ElPreferences *prefs, const YAML::Node &video);
	void parseHostData(ElPreferences *prefs, const YAML::Node &host);
	void parseInputData(ElPreferences *prefs, const YAML::Node &input);
	uint32_t parseColor(const YAML::Node &color);
	void emitColor(YAML::Emitter &out, uint32_t color);
	uint64_t parseUid(const std::string &uid);
	std::string uidToString(uint64_t uid);
}

ElPreferences::ElPreferences()
	: playerName(""),
	helmetName("base"),
	chestName("base"),
	shouldersName("base"),
	armsName("base"),
	legsName("base"),
	accessoryName("base"),
	pelvisName("base"),
	primaryColor(0),
	secondaryColor(0),
	visorColor(0),
	lightsColor(0),
	holoColor(0),
	countdownTimer(5),
	serverPassword(""),
	serverLanMode(false),
	fov(90.f),
	rawMouse(true),
	crosshairCentered(false),
	playerUid(0)
{
	memset(&lastChanged, 0, sizeof(lastChanged));
}

bool ElPreferences::load()
{
	try
	{
		YAML::Node prefs = YAML::LoadFile(PreferencesFileName);
		if (prefs["player"])
			parsePlayerData(this, prefs["player"]);
		if (prefs["video"])
			parseVideoData(this, prefs["video"]);
		if (prefs["host"])
			parseHostData(this, prefs["host"]);
		if (prefs["input"])
			parseInputData(this, prefs["input"]);
	}
	catch (YAML::BadFile&)
	{
		return false;
	}
	catch (YAML::Exception &ex)
	{
		std::cout << "Unable to load " << PreferencesFileName << ": " << ex.mark.line << "," << ex.mark.column << " " << ex.msg << std::endl;
		return false;
	}
	return true;
}

bool ElPreferences::save()
{
	std::ofstream outFile(PreferencesFileName, std::ios::trunc);
	if (outFile.fail())
		return false;
	try
	{
		YAML::Emitter out(outFile);
		out.SetIndent(4);
		out << YAML::BeginMap;
		out << YAML::Key << "player" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << playerName;
		out << YAML::Key << "uid" << YAML::Value << uidToString(playerUid);
		out << YAML::Key << "armor" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "helmet" << YAML::Value << helmetName;
		out << YAML::Key << "chest" << YAML::Value << chestName;
		out << YAML::Key << "shoulders" << YAML::Value << shouldersName;
		out << YAML::Key << "arms" << YAML::Value << armsName;
		out << YAML::Key << "legs" << YAML::Value << legsName;
		out << YAML::Key << "accessory" << YAML::Value << accessoryName;
		out << YAML::Key << "pelvis" << YAML::Value << pelvisName;
		out << YAML::EndMap;
		out << YAML::Key << "colors" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "primary" << YAML::Value;
		emitColor(out, primaryColor);
		out << YAML::Key << "secondary" << YAML::Value;
		emitColor(out, secondaryColor);
		out << YAML::Key << "lights" << YAML::Value;
		emitColor(out, lightsColor);
		out << YAML::Key << "visor" << YAML::Value;
		emitColor(out, visorColor);
		out << YAML::Key << "holo" << YAML::Value;
		emitColor(out, holoColor);
		out << YAML::EndMap;
		out << YAML::EndMap;
		out << YAML::Key << "video" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "fov" << YAML::Value << fov;
		out << YAML::Key << "crosshairCentered" << YAML::Value << crosshairCentered;
		out << YAML::EndMap;
		out << YAML::Key << "host" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "countdown" << YAML::Value << countdownTimer;
		out << YAML::Key << "password" << YAML::Value << serverPassword;
		//out << YAML::Key << "lanMode" << YAML::Value << serverLanMode;
		out << YAML::EndMap;
		out << YAML::Key << "input" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "rawMouse" << YAML::Value << rawMouse;
		out << YAML::EndMap;
		out << YAML::EndMap;
	}
	catch (YAML::Exception&)
	{
		return false;
	}
	memset(&lastChanged, 0, sizeof(lastChanged)); // Don't trigger a change event for this
	return true;
}

bool ElPreferences::changed()
{
	// Just poll the file for changes...the official file notification APIs are too much of a hassle
	// I timed this out and it only takes about 20us on my machine, so this is probably fine
	// -shockfire
	WIN32_FILE_ATTRIBUTE_DATA attribs;
	if (!GetFileAttributesEx(PreferencesFileName, GetFileExInfoStandard, &attribs))
		return false;
	if (lastChanged.dwLowDateTime == 0 && lastChanged.dwHighDateTime == 0)
	{
		lastChanged = attribs.ftLastWriteTime;
		return false;
	}
	if (CompareFileTime(&attribs.ftLastWriteTime, &lastChanged) <= 0)
		return false;
	lastChanged = attribs.ftLastWriteTime;
	return true;
}

namespace
{
	void parsePlayerData(ElPreferences *prefs, const YAML::Node &player)
	{
		if (player["name"])
			prefs->setPlayerName(player["name"].as<std::string>());
		if (player["uid"])
			prefs->setPlayerUid(parseUid(player["uid"].as<std::string>()));
		if (player["armor"])
			parseArmorData(prefs, player["armor"]);
		if (player["colors"])
			parseColorData(prefs, player["colors"]);
	}

	void parseArmorData(ElPreferences *prefs, const YAML::Node &armor)
	{
		if (armor["helmet"])
			prefs->setHelmetName(armor["helmet"].as<std::string>());
		if (armor["chest"])
			prefs->setChestName(armor["chest"].as<std::string>());
		if (armor["shoulders"])
			prefs->setShouldersName(armor["shoulders"].as<std::string>());
		if (armor["arms"])
			prefs->setArmsName(armor["arms"].as<std::string>());
		if (armor["legs"])
			prefs->setLegsName(armor["legs"].as<std::string>());
		if (armor["accessory"])
			prefs->setAccessoryName(armor["accessory"].as<std::string>());
		if (armor["pelvis"])
			prefs->setPelvisName(armor["pelvis"].as<std::string>());
	}

	void parseColorData(ElPreferences *prefs, const YAML::Node &colors)
	{
		if (colors["primary"])
			prefs->setPrimaryColor(parseColor(colors["primary"]));
		if (colors["secondary"])
			prefs->setSecondaryColor(parseColor(colors["secondary"]));
		if (colors["visor"])
			prefs->setVisorColor(parseColor(colors["visor"]));
		if (colors["lights"])
			prefs->setLightsColor(parseColor(colors["lights"]));
		if (colors["holo"])
			prefs->setHoloColor(parseColor(colors["holo"]));
	}

	void parseVideoData(ElPreferences *prefs, const YAML::Node &video)
	{
		if (video["fov"])
			prefs->setFieldOfView(video["fov"].as<float>());

		if (video["crosshairCentered"])
			prefs->setCrosshairCentered(video["crosshairCentered"].as<bool>());
	}

	void parseHostData(ElPreferences *prefs, const YAML::Node &host)
	{
		if (host["countdown"])
			prefs->setCountdownTimer(host["countdown"].as<int>());

		if (host["password"])
			prefs->setServerPassword(host["password"].as<std::string>());

		//if (host["lanMode"])
		//	prefs->setServerLanMode(host["lanMode"].as<bool>());
	}

	void parseInputData(ElPreferences *prefs, const YAML::Node &input)
	{
		if (input["rawMouse"])
			prefs->setRawMouse(input["rawMouse"].as<bool>());
	}

	uint32_t parseColor(const YAML::Node &color)
	{
		uint8_t r = 0, g = 0, b = 0;
		if (color["r"])
			r = color["r"].as<int>();
		if (color["g"])
			g = color["g"].as<int>();
		if (color["b"])
			b = color["b"].as<int>();
		return (r << 16) | (g << 8) | b;
	}

	void emitColor(YAML::Emitter &out, uint32_t color)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "r" << YAML::Value << static_cast<int>((color >> 16) & 0xFF);
		out << YAML::Key << "g" << YAML::Value << static_cast<int>((color >> 8) & 0xFF);
		out << YAML::Key << "b" << YAML::Value << static_cast<int>(color & 0xFF);
		out << YAML::EndMap;
	}

	uint64_t parseUid(const std::string &uid)
	{
		// UIDs are just a string of hex digits
		uint64_t result = 0;
		for (size_t i = 0; i < uid.length(); i++)
		{
			result <<= 4;
			char ch = tolower(uid[i]);
			if (ch >= '0' && ch <= '9')
				result |= ch - '0';
			else if (ch >= 'a' && ch <= 'z')
				result |= ch - 'a' + 0xA;
		}
		return result;
	}

	std::string uidToString(uint64_t uid)
	{
		const char *hexChars = "0123456789abcdef";
		std::string result;
		for (int i = 0; i < 16; i++)
		{
			result += hexChars[uid >> 60];
			uid <<= 4;
		}
		return result;
	}
}