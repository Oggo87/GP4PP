#include "HelmetVisor.h"
#include "../Utils/CGP4Glass.h"
#include "../IniLib/IniLib.h"
#include "../GP4MemLib/GP4MemLib.h"

using namespace std;
using namespace GP4MemLib;
using namespace IniLib;
using namespace GP4PP;

namespace HelmetVisor
{
	//Vars and Data Addresses
	DWORD helmetColoursBaseAddress = 0x00644030;

	//Storage variables
	bool individualHelmetVisorColours = true;
	std::vector<CGP4Glass> helmetVisorGlassData;

	DWORD colour = 0x00000000;

	void LoadSettings(IniFile iniSettings, std::string basePath)
	{
		CGP4Glass defaultGlass, glass;
		string iniFileName = "VisorColours.ini";
		IniFile iniVisorSettings;

		defaultGlass.colour = { 0, 0, 0, 0 };
		defaultGlass.transparencyMultiplier = 0.5;

		glass = defaultGlass;

		try
		{
			individualHelmetVisorColours = iniSettings["HelmetVisor"]["Individual"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual Helmet Visor Colours: " + string(individualHelmetVisorColours ? "Enabled" : "Disabled"));

		defaultGlass.LoadGlassSettings(iniSettings, "Helmet Visor", "HelmetVisor");

		if (individualHelmetVisorColours)
		{
			try 
			{
				iniFileName = iniSettings["HelmetVisor"]["Ini"].getString();
			}
			catch (exception ex) {}

			if (iniVisorSettings.load(basePath + iniFileName))
			{
				OutputGP4PPDebugString("Loaded Helmet Visor INI: " + iniFileName);
			}
			else
			{
				individualHelmetVisorColours = false;

				OutputGP4PPDebugString("Failed to open Helmet Visor INI: " + iniFileName);
			}
		}
		
		for (int i = 1; i <= 22; i++)
		{
			glass = defaultGlass;

			if(individualHelmetVisorColours)
			{

				glass.LoadGlassSettings(iniVisorSettings, "Helmet Visor " + to_string(i), "HelmetVisor" + to_string(i));
			}

			helmetVisorGlassData.push_back(glass);
		}
	}

	void ApplyPatches()
	{
		for (int i = 0; i < 22; i++)
		{
			//Patch Helmet Visor Colours
			colour = helmetVisorGlassData[i].colour;
			MemUtils::patchAddress((LPVOID)(helmetColoursBaseAddress + i * 0x8), MemUtils::toBytes<DWORD>(colour), sizeof(DWORD));

			//Patch Helmet Visor Transparency
			MemUtils::patchAddress((LPVOID)(helmetColoursBaseAddress + 0x4 + i * 0x8), MemUtils::toBytes<float>(helmetVisorGlassData[i].transparencyMultiplier), sizeof(float));
		}
	}
}