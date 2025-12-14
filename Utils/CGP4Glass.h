#pragma once
#include "../Utils/Helpers.h"
#include "../Utils/BGRA.h"
#include "../IniLib/IniLib.h"

using namespace std;

namespace GP4PP
{
	struct CGP4Glass
	{
		BGRA colour = { 128, 128, 128, 0 };
		float transparencyMultiplier = 0.5;

		void LoadGlassSettings(IniLib::IniFile iniVisorSettings, string name, string sectionName)
		{

			try
			{
				colour = iniVisorSettings[sectionName]["Colour"].getVectorAs<short>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString(name + " Colour: B: " + to_string(colour.getB()) + " G: " + to_string(colour.getG()) + " R: " + to_string(colour.getR()) + " A: " + to_string(colour.getA()));

			try
			{
				transparencyMultiplier = iniVisorSettings[sectionName]["Transparency"].getAs<float>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString(name + " Transparency Multiplier: " + to_string(transparencyMultiplier));
		}
	};
}

