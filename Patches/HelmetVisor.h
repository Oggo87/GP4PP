#pragma once
#include "../IniLib/IniLib.h"

using namespace IniLib;

namespace HelmetVisor
{

	void LoadSettings(IniFile iniSettings, std::string basePath);

	void ApplyPatches();
};

