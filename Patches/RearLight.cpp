#include "RearLight.h"
#include "../Utils/Helpers.h"
#include "../Utils/CarDynamicData.h"
#include "../Utils/PitLimiter.h"
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
	short brakeThreshold = 5; //%
	bool pitLimiterLight = true;

	//Target Addresses
	DWORD rearLightFunctionAddress = 0x005108f4;

	//Jump Back Addresses
	DWORD rearLightFunctionJumpBackAddress = 0x005108f9;

	//Function Addresses
	DWORD isTrackWetFunc = 0x00510778;

	//Parameters
	unsigned short maxBrakeValue = 0x4000;

	//Data Variables
	unsigned int trackWet = 0;
	unsigned int rearLightOn = 0;

	unsigned int carIndex = 0;

	DWORD carDynDataAddress = 0;

	CarDynamicData *carDynData;

	bool RearLight::wetWeatherBlinking = false;
	int RearLight::wetWeatherPeriodMs = 250;

	bool RearLight::pitLimiterBlinking = true;
	int RearLight::pitLimiterPeriodMs = 500;


	bool pitLimiterOn = false;

	//static Random Number Generator for random time offsets
	std::mt19937 Blinker::rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));

	array<RearLight, 22> rearLights;

	int calcRearLightState()
	{
		carDynData = MemUtils::addressToPtr<CarDynamicData>(carDynDataAddress);

		if (brakeLight && (carDynData->brake > maxBrakeValue))
		{
			rearLights[carIndex].setState(BRAKE);
		}
		else if(pitLimiterLight && PitLimiter::updateSatus(carDynData))
		{
			rearLights[carIndex].setState(PIT_LIMITER);
		}
		//else if (carDynData->flags_0x7B.energyHarvestingERS) // Charging
		//{
		//	rearLights[carIndex].setState(CHARGE);
		//}
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
		//Utility string builder
		std::ostringstream messageBuilder;

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

		if (brakeLight)
		{
			try
			{
				brakeThreshold = iniSettings["RearLight"]["BrakeThreshold"].getAs<short>();
			}
			catch (exception ex) {}

			messageBuilder.str(string());

			messageBuilder << "Rear Light - Brake Threshold : " << brakeThreshold << " %";

			if (brakeThreshold > 100)
			{
				messageBuilder << " (Clamped to 100 %)";
				brakeThreshold = 100;
			}
			else if (brakeThreshold < 0)
			{
				messageBuilder << " (Clamped to 0 %)";
				brakeThreshold = 0;
			}

			OutputGP4PPDebugString(messageBuilder.str());
		}

		try
		{
			pitLimiterLight = iniSettings["RearLight"]["PitLimiter"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Rear Light - Pit Limiter : " + string(pitLimiterLight ? "Enabled" : "Disabled"));

		if (pitLimiterLight)
		{
			//Pit Limiter rear light blinking
			try
			{
				RearLight::pitLimiterBlinking = iniSettings["RearLight"]["PitLimiterBlinking"].getAs<bool>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Rear Light - Pit Limiter Blinking : " + string(RearLight::pitLimiterBlinking ? "Enabled" : "Disabled"));

			if (RearLight::pitLimiterBlinking)
			{
				try
				{
					RearLight::pitLimiterPeriodMs = iniSettings["RearLight"]["PitLimiterPeriod"].getAs<int>();
				}
				catch (exception ex) {}

				OutputGP4PPDebugString("Rear Light - Pit Limiter Period : " + to_string(RearLight::pitLimiterPeriodMs) + " ms");
			}
		}
	}

	void ApplyPatches()
	{
		MemUtils::rerouteFunction(rearLightFunctionAddress, PtrToUlong(rearLightFunction), VAR_NAME(rearLightFunction));
	}
}