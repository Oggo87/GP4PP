#include "CockpitVisor.h"
#include "../Utils/Helpers.h"
#include "../Utils/BGRA.h"
#include "../Utils/CGP4Glass.h"
#include "../GP4MemLib/GP4MemLib.h"
#include "../IniLib/IniLib.h"

using namespace std;
using namespace GP4MemLib;
using namespace IniLib;
using namespace GP4PP;

namespace CockpitVisor
{
	// Settings variables
	bool cockpitVisor = false;

	//Target Addresses
	DWORD cockpitVisorStartAddress = 0x004858f9;
	DWORD cockpitVisorStartAddress2 = 0x00487a61;
	DWORD zCockpitVisorStartAddress = 0x00487e9c;

	//Jump Back Addresses
	DWORD cockpitVisorJumpBackAddress = 0x004858fe;
	DWORD cockpitVisorJumpBackAddress2 = 0x00488a0b;
	DWORD zCockpitVisorJumpBackAddress = 0x00487ea9;

	//Vars and Data Addresses
	DWORD cockpitMesh = 0x00a4d5ec;

	//Function Addresses
	DWORD GetObjectIndexByName = 0x00457200;
	DWORD GetShader = 0x0045ee70;
	DWORD ApplyShaderToObject = 0x00457260;
	DWORD SetCockpitMirrorShaderParameters = 0x00485080;

	//Cockpit Glass Object Variables
	string visorObjectName = "Visor";
	bool individualVisorColours = false;
	int visorObjectIndex = -1;
	std::vector<CGP4Glass> visorGlassData;

	//Z Cockpit Glass Object Variables
	bool individualZCockpitVisorColours = false;
	std::vector<CGP4Glass> zGlassData;

	//Storage variables
	DWORD cGP4CarAddress = NULL;
	DWORD zShaderParams = NULL;

	DWORD colour = 0x00000000;
	float transparencyMultiplier = 0.5;

	int driverIndex = 0;

	void SetCockpitVisorShaderParameters()
	{
		//Get cockpit mesh
		byte* cockpitMeshStruct = GP4MemLib::MemUtils::addressToValue<byte*>(cockpitMesh);

		//Get number of cockpit objects
		int* numCockpitObjects = (int*)(cockpitMeshStruct + 0x1b0);

		if (visorObjectIndex > -1 && visorObjectIndex < *numCockpitObjects)
		{
			if (cGP4CarAddress != NULL && individualVisorColours)
			{
				driverIndex = GP4MemLib::MemUtils::addressToValue<int>(cGP4CarAddress + 0x1C);
			}
			else
			{
				driverIndex = 0;
			}

			//Get array of mesh objects
			byte* arrMeshObjects = cockpitMeshStruct + 0x1b4;

			//Get pointer to visor object
			DWORD* visorObject = (DWORD*)(*(DWORD*)arrMeshObjects + visorObjectIndex * 0x34);

			//Get Index of visor object
			int* visorObjectIndex = (int*)(&visorObject[3]);

			//Get visor object data
			DWORD** visorObjectData = (DWORD**)(visorObject[2]);

			//Get Array of Shader Data
			CGP4MeshShaderData* arrayShaderData = (CGP4MeshShaderData*)visorObjectData[5];

			//Set Shader Parameters
			//Colour
			colour = visorGlassData[driverIndex].colour;
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[0], &colour, 4);

			//Multiplier
			transparencyMultiplier = visorGlassData[driverIndex].transparencyMultiplier;
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[1], &transparencyMultiplier, 4);

			//Unknown
			arrayShaderData[*visorObjectIndex].params[2] = 0xffffff;

			//Multiplier
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[3], &transparencyMultiplier, 4);

			//Unknown
			arrayShaderData[*visorObjectIndex].params[4] = GP4MemLib::MemUtils::addressToValue<DWORD>(0x00644274);

			//Unknown, possibly reflections
			if (cGP4CarAddress != NULL)
			{
				float val1 = atan2f(GP4MemLib::MemUtils::addressToValue<float>(cGP4CarAddress + 0x354), GP4MemLib::MemUtils::addressToValue<float>(cGP4CarAddress + 0x35c));
				float val2 = atan2f(GP4MemLib::MemUtils::addressToValue<float>(0x00a53294), GP4MemLib::MemUtils::addressToValue<float>(0x00a5329c));

				float val = (val2 + 3.1415927f + val1 + 3.1415927f) * 0.5f * 0.03183099f;

				std::memcpy(&arrayShaderData[*visorObjectIndex].params[5], &val, 4);
			}
			else
			{
				arrayShaderData[*visorObjectIndex].params[5] = 0;
			}

		}
	}

	__declspec(naked) void cockpitVisorFunc()
	{
		__asm {
			//set mesh
			mov ECX, dword ptr[EDI + ESI * 0x4 + 0x4]

			//set name of object to be visor
			//push visorObjectName
			lea EBP, visorObjectName
			push EBP

			//get index of visor object, it will be in EAX
			call GetObjectIndexByName

			//save index in EBP
			mov EBP, EAX

			//skip if object not found
			cmp EBP, -1

			//save index into variable
			mov visorObjectIndex, EBP

			jz noObjectFound

			//if found
			//set flag
			push 0x1

			//set object index
			push EBP

			//get the shader (it will be in EAX)
			push 0x11

			call GetShader

			//set mesh (this)
			mov ECX, dword ptr[EDI + ESI * 0x4 + 0x4]

			//advance stack
			add ESP, 0x4

			//set shader
			push EAX

			//apply shader
			call ApplyShaderToObject
		}

		cGP4CarAddress = NULL;

		SetCockpitVisorShaderParameters();

	noObjectFound:

		__asm call SetCockpitMirrorShaderParameters //original function call

		__asm jmp cockpitVisorJumpBackAddress //jump back into regular flow
	}

	__declspec(naked) void cockpitVisorFunc2()
	{
		__asm {
			mov cGP4CarAddress, EBP
		}

		SetCockpitVisorShaderParameters();

		__asm call SetCockpitMirrorShaderParameters //original function call

		__asm jmp cockpitVisorJumpBackAddress2 //jump back into regular flow
	}

	__declspec(naked) void zCockpitVisorFunc()
	{
		RegUtils::saveVolatileRegisters();

		__asm mov cGP4CarAddress, EBP

		__asm mov zShaderParams, EAX

		if (individualZCockpitVisorColours)
		{
			driverIndex = GP4MemLib::MemUtils::addressToValue<int>(cGP4CarAddress + 0x1C);
		}
		else
		{
			driverIndex = 0;
		}

		//Set Shader Parameters
		//Colour
		colour = zGlassData[driverIndex].colour;
		MemUtils::patchAddress((LPVOID)(zShaderParams), MemUtils::toBytes<DWORD>(colour), sizeof(DWORD));

		//MemUtils::patchAddress((LPVOID)(zShaderParams), (BYTE*)(&(zGlassData[driverIndex].colour)), 4);

		//Multiplier
		MemUtils::patchAddress((LPVOID)(zShaderParams + 4), MemUtils::toBytes<float>(zGlassData[driverIndex].transparencyMultiplier), sizeof(float));

		RegUtils::restoreVolatileRegisters();

		__asm jmp zCockpitVisorJumpBackAddress //jump back into regular flow
	}

	void LoadSettings(IniFile iniSettings, string basePath)
	{

		CGP4Glass defaultGlass, glass;
		string iniFileName = "VisorColours.ini";
		IniFile iniVisorSettings;

		// Cockpit Visor
		try
		{
			cockpitVisor = iniSettings["Settings"]["CockpitVisor"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Cockpit Visor : " + string(cockpitVisor ? "Enabled" : "Disabled"));

		if (cockpitVisor)
		{
			defaultGlass.colour = { 128, 128, 128, 0 };
			defaultGlass.transparencyMultiplier = 0.5f;

			try
			{
				visorObjectName = iniSettings["CockpitVisor"]["ObjectName"].getString();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Cockpit Visor Object Name: " + visorObjectName);

			try
			{
				individualVisorColours = iniSettings["CockpitVisor"]["Individual"].getAs<bool>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Individual Cockpit Visor Colours: " + string(individualVisorColours ? "Enabled" : "Disabled"));

			defaultGlass.LoadGlassSettings(iniSettings, "Cockpit Visor", "CockpitVisor");

			if (individualVisorColours)
			{
				try
				{
					iniFileName = iniSettings["CockpitVisor"]["Ini"].getString();
				}
				catch (exception ex) {}

				if (iniVisorSettings.load(basePath + iniFileName))
				{
					OutputGP4PPDebugString("Loaded Cockpit Visor INI: " + iniFileName);

					for (int i = 1; i <= 22; i++)
					{
						glass = defaultGlass;

						glass.LoadGlassSettings(iniVisorSettings, "Cockpit Visor " + to_string(i), "CockpitVisor" + to_string(i));

						visorGlassData.push_back(glass);
					}
				}
				else
				{
					individualVisorColours = false;

					OutputGP4PPDebugString("Failed to open Cockpit Visor INI: " + iniFileName);
				}
			}
			else
			{

				visorGlassData.push_back(defaultGlass);
			}

		}

		// Z Cockpit Visor
		defaultGlass.colour = { 128, 128, 128, 0 };
		defaultGlass.transparencyMultiplier = 0.3f;

		try
		{
			individualZCockpitVisorColours = iniSettings["ZCockpitVisor"]["Individual"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual Z Cockpit Visor Colours: " + string(individualZCockpitVisorColours ? "Enabled" : "Disabled"));

		defaultGlass.LoadGlassSettings(iniSettings, "Z Cockpit Visor", "ZCockpitVisor");

		if (individualZCockpitVisorColours)
		{
			try
			{
				iniFileName = iniSettings["ZCockpitVisor"]["Ini"].getString();
			}
			catch (exception ex) {}

			if (iniVisorSettings.load(basePath + iniFileName))
			{
				OutputGP4PPDebugString("Loaded Cockpit Visor INI: " + iniFileName);

				for (int i = 1; i <= 22; i++)
				{
					glass = defaultGlass;

					glass.LoadGlassSettings(iniVisorSettings, "Z Cockpit Visor " + to_string(i), "ZCockpitVisor" + to_string(i));

					zGlassData.push_back(glass);
				}
			}
			else
			{
				individualVisorColours = false;

				OutputGP4PPDebugString("Failed to open Cockpit Visor INI: " + iniFileName);
			}

		}
		if (!individualZCockpitVisorColours)
		{
			zGlassData.push_back(defaultGlass);
		}

	}

	void ApplyPatches()
	{
		//Re-route for cockpit visor
		MemUtils::rerouteFunction(cockpitVisorStartAddress, PtrToUlong(cockpitVisorFunc), VAR_NAME(cockpitVisorFunc));

		//Re-route for cockpit visor 2
		MemUtils::rerouteFunction(cockpitVisorStartAddress2, PtrToUlong(cockpitVisorFunc2), VAR_NAME(cockpitVisorFunc2));

		//Re-route for z-cockpit visor
		MemUtils::rerouteFunction(zCockpitVisorStartAddress, PtrToUlong(zCockpitVisorFunc), VAR_NAME(zCockpitVisorFunc));
	}
};
