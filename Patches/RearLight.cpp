#include "RearLight.h"
#include "../Utils/Helpers.h"
#include "../GP4MemLib/GP4MemLib.h"
#include<chrono>
#include<array>
#include<random>

using namespace GP4MemLib;
using namespace GP4PP;
using namespace std;
using namespace chrono;

namespace RearLight
{
	//Target Addresses
	DWORD rearLightFunctionAddress = 0x005108f4;

	//Jump Back Addresses
	DWORD rearLightFunctionJumpBackAddress = 0x005108f9;

	//Data Variables
	unsigned int rearLightOn = 0;
	unsigned int accumulator = 0;

	unsigned int carIndex = 0;

	int periodMs = 500; //blink period in milliseconds

	auto lastTime = steady_clock::now();

	struct RearLightState
	{
		// lastTime is set to now + random offset so different cars blink out of phase
		steady_clock::time_point lastTime;
		bool lightOn = false;

		RearLightState()
		{
			auto now = steady_clock::now();

			// static RNG so we don't reseed on each construction
			static mt19937 rng(static_cast<unsigned>(steady_clock::now().time_since_epoch().count()));
			uniform_int_distribution<int> dist(0, periodMs);

			lastTime = now + milliseconds(dist(rng));
			lightOn = false;
		}
	};

	array<RearLightState, 22> rearLightStates;

	void calcRearLightState()
	{

		auto currentTime = steady_clock::now();

		auto elapsed = duration_cast<milliseconds>(currentTime - rearLightStates[carIndex].lastTime).count();

		if (elapsed >= periodMs)
		{

			if (rearLightStates[carIndex].lightOn)
			{
				rearLightStates[carIndex].lightOn = 0;
			}
			else
			{
				rearLightStates[carIndex].lightOn = 1;
			}

			rearLightStates[carIndex].lastTime += milliseconds(periodMs);
		}
	}

	__declspec(naked) void rearLightFunction()
	{
		RegUtils::saveVolatileRegisters();

		__asm mov carIndex, EAX

		calcRearLightState();

		rearLightOn = rearLightStates[carIndex].lightOn;

		RegUtils::restoreVolatileRegisters();

		__asm mov EAX, rearLightOn

		__asm jmp rearLightFunctionJumpBackAddress //jump back into regular flow
	}

	void LoadSettings(IniFile iniSettings)
	{
	}

	void ApplyPatches()
	{
		MemUtils::rerouteFunction(rearLightFunctionAddress, PtrToUlong(rearLightFunction), VAR_NAME(rearLightFunction));
	}
}