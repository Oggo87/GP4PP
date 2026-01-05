#include "RearLight.h"
#include "../Utils/Helpers.h"
#include "../Utils/CarDynamicData.h"
#include "../GP4MemLib/GP4MemLib.h"
#include<array>

using namespace GP4MemLib;
using namespace GP4PP;
using namespace std;
using namespace chrono;

namespace RearLight
{
	//General Settings
	bool wetWeatherLight = true;
	bool brakeLight = false;

	//Target Addresses
	DWORD rearLightFunctionAddress = 0x005108f4;

	//Jump Back Addresses
	DWORD rearLightFunctionJumpBackAddress = 0x005108f9;

	//Function Addresses
	DWORD isTrackWetFunc = 0x00510778;

	//Data Variables
	unsigned int trackWet = 0;
	unsigned int rearLightOn = 0;

	unsigned int carIndex = 0;

	DWORD carDynDataAddress = 0;

	CarDynamicData *carDynData;

	bool RearLight::wetWeatherBlinking = false;
	int RearLight::wetWeatherPeriodMs = 250;

	//static Random Number Generator for random time offsets
	std::mt19937 Blinker::rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));

	array<RearLight, 22> rearLights;

	int calcRearLightState()
	{
		carDynData = MemUtils::addressToPtr<CarDynamicData>(carDynDataAddress);

		if (brakeLight && carDynData->brake_0x24B > 0x10)
		{
			rearLights[carIndex].setState(BRAKE);
		}
		else if (wetWeatherLight && trackWet)
		{
			rearLights[carIndex].setState(WET);
		}
		else
		{
			rearLights[carIndex].setState(OFF);
		}

		return rearLights[carIndex].update();
	}

	__declspec(naked) void rearLightFunction()
	{
		RegUtils::saveVolatileRegisters();

		__asm mov carIndex, EAX

		__asm mov carDynDataAddress, ESI

		__asm call isTrackWetFunc

		__asm mov trackWet, EAX

		rearLightOn = calcRearLightState();

		RegUtils::restoreVolatileRegisters();

		__asm mov EAX, rearLightOn

		__asm jmp rearLightFunctionJumpBackAddress //jump back into regular flow
	}

	void LoadSettings(IniFile iniSettings)
	{
		//Wet weather rear light
		try
		{
			wetWeatherLight = iniSettings["RearLight"]["WetWeather"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Rear Light - Wet Weather : " + string(wetWeatherLight ? "Enabled" : "Disabled"));

		if (wetWeatherLight)
		{
			//Wet weather rear light blinking
			try
			{
				RearLight::wetWeatherBlinking = iniSettings["RearLight"]["WetWeatherBlinking"].getAs<bool>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Rear Light - Wet Weather Blinking : " + string(RearLight::wetWeatherBlinking ? "Enabled" : "Disabled"));

			if (RearLight::wetWeatherBlinking)
			{
				try
				{
					RearLight::wetWeatherPeriodMs = iniSettings["RearLight"]["WetWeatherPeriod"].getAs<int>();
				}
				catch (exception ex) {}

				OutputGP4PPDebugString("Rear Light - Wet Weather Period : " + to_string(RearLight::wetWeatherPeriodMs) + " ms");
			}
		}

		//Brake rear light
		try
		{
			brakeLight = iniSettings["RearLight"]["Brake"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Rear Light - Brake : " + string(brakeLight ? "Enabled" : "Disabled"));
	}

	void ApplyPatches()
	{
		MemUtils::rerouteFunction(rearLightFunctionAddress, PtrToUlong(rearLightFunction), VAR_NAME(rearLightFunction));
	}
}