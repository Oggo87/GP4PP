#pragma once
#include "IniLib/IniLib.h"

using namespace IniLib;

namespace CockpitVisor
{

	void LoadSettings(IniFile iniSettings, std::string basePath);

	void ApplyPatches();
};

