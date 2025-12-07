#include "CockpitVisor.h"
#include "Helpers.h"
#include "GP4MemLib/GP4MemLib.h"
#include "IniLib/IniLib.h"

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

	//Jump Back Addresses
	DWORD cockpitVisorJumpBackAddress = 0x004858fe;
	DWORD cockpitVisorJumpBackAddress2 = 0x00488a0b;

	//Vars and Data Addresses
	DWORD cockpitMesh = 0x00a4d5ec;

	//Function Addresses
	DWORD GetObjectIndexByName = 0x00457200;
	DWORD GetShader = 0x0045ee70;
	DWORD ApplyShaderToObject = 0x00457260;
	DWORD SetCockpitMirrorShaderParameters = 0x00485080;

	//Cockpit Glass Object Variables
	string visorObjectName = "Visor";
	int visorObjectIndex = -1;
	std::vector<short> visorBytes = { 128, 128, 128, 0 };
	DWORD visorColour = 0x808080;
	float transparencyMultiplier = 0.5;

	DWORD CGP4Car = NULL;


	void SetCockpitVisorShaderParameters()
	{
		//Get cockpit mesh
		byte* cockpitMeshStruct = GP4MemLib::MemUtils::addressToValue<byte*>(cockpitMesh);

		//Get number of cockpit objects
		int* numCockpitObjects = (int*)(cockpitMeshStruct + 0x1b0);

		if (visorObjectIndex > -1 && visorObjectIndex < *numCockpitObjects)
		{
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
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[0], &visorColour, 4);

			//Multiplier
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[1], &transparencyMultiplier, 4);

			//Unknown
			arrayShaderData[*visorObjectIndex].params[2] = 0xffffff;

			//Multiplier
			std::memcpy(&arrayShaderData[*visorObjectIndex].params[3], &transparencyMultiplier, 4);

			//Unknown
			arrayShaderData[*visorObjectIndex].params[4] = GP4MemLib::MemUtils::addressToValue<DWORD>(0x00644274);

			//Unknown, possibly reflections
			if (CGP4Car != NULL)
			{
				float val1 = atan2f(GP4MemLib::MemUtils::addressToValue<float>(CGP4Car + 0x354), GP4MemLib::MemUtils::addressToValue<float>(CGP4Car + 0x35c));
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

		SetCockpitVisorShaderParameters();

	noObjectFound:

		__asm call SetCockpitMirrorShaderParameters //original function call

		__asm jmp cockpitVisorJumpBackAddress //jump back into regular flow
	}

	__declspec(naked) void pitcrewFunc2()
	{
		__asm {
			mov CGP4Car, EBP
		}

		SetCockpitVisorShaderParameters();

		__asm call SetCockpitMirrorShaderParameters //original function call

		__asm jmp cockpitVisorJumpBackAddress2 //jump back into regular flow
	}


	void LoadSettings(IniFile iniSettings)
	{

		// Cockpit Visor
		try
		{
			cockpitVisor = iniSettings["Settings"]["CockpitVisor"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Cockpit Visor : " + string(cockpitVisor ? "Enabled" : "Disabled"));

		if (cockpitVisor)
		{
			try
			{
				visorObjectName = iniSettings["CockpitVisor"]["ObjectName"].getString();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Cockpit Visor Object Name: " + visorObjectName);

			try
			{
				visorBytes = iniSettings["CockpitVisor"]["Colour"].getVectorAs<short>();
			}
			catch (exception ex) {}

			while (visorBytes.size() < 4)
			{
				visorBytes.push_back(0);
			}

			visorColour = 0;

			for (unsigned int i = 0; i < visorBytes.size(); i++)
			{
				if (visorBytes[i] > 255)
				{
					visorBytes[i] = 255;
				}
				visorColour += (visorBytes[i] << i * 8);
			}

			OutputGP4PPDebugString("Cockpit Visor Colour: B: " + to_string(visorBytes[0]) + " G: " + to_string(visorBytes[1]) + " R: " + to_string(visorBytes[2]) + " A: " + to_string(visorBytes[3]));

			try
			{
				transparencyMultiplier = iniSettings["CockpitVisor"]["Transparency"].getAs<float>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Cockpit Visor Transparency Multiplier: " + to_string(transparencyMultiplier));
		}
	}

	void ApplyPatches()
	{
		//Re-route for cockpit visor
		MemUtils::rerouteFunction(cockpitVisorStartAddress, PtrToUlong(cockpitVisorFunc), VAR_NAME(cockpitVisorFunc));

		//Re-route for cockpit visor 2
		MemUtils::rerouteFunction(cockpitVisorStartAddress2, PtrToUlong(pitcrewFunc2), VAR_NAME(pitcrewFunc2));
	}
};
