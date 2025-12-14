#pragma once
#include "../IniLib/IniLib.h"

using namespace IniLib;

namespace Assets
{

	void LoadSettings(IniFile iniSettings);

	void DefaultSettings();

	void ApplyPatches();
};

