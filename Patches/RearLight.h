#pragma once
#include "../IniLib/IniLib.h"
#include<chrono>
#include<random>


using namespace IniLib;

namespace RearLight
{
	enum REAR_LIGHT_STATE
	{
		OFF,
		WET,
		BRAKE,
		PIT_LIMITER,
		CHARGE

	};

	class Blinker
	{

	private:
		int periodMs; 
		std::chrono::steady_clock::time_point lastTime;
		bool lightOn;
		bool isBlinking;

		static std::mt19937 rng;

	public:

		Blinker()
		{
			periodMs = 0;
			lastTime = std::chrono::steady_clock::now();
			lightOn = false;
			isBlinking = false;
		}

		bool enable(int periodMilliseconds)
		{
			periodMs = periodMilliseconds;
			if (!isBlinking)
			{
				auto now = std::chrono::steady_clock::now();

				std::uniform_int_distribution<int> dist(0, periodMs);

				lastTime = now + std::chrono::milliseconds(dist(rng));

				isBlinking = true;
			}
			return isBlinking;
		}

		bool disable()
		{
			isBlinking = false;
			lightOn = false;
			return isBlinking;
		}

		bool tick()
		{
			if (isBlinking)
			{
				auto currentTime = std::chrono::steady_clock::now();
				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
				if (elapsed >= periodMs)
				{
					lightOn = !lightOn;
					lastTime += std::chrono::milliseconds(periodMs);
				}
				return lightOn;
			}
			return false;
		}

		bool isEnabled() const
		{
			return isBlinking;
		}

		bool getLightState() const
		{
			return lightOn;
		}

	};

	class RearLight
	{
	private:
		REAR_LIGHT_STATE currentState;
		Blinker blinker;
		bool lightOn;

	public:

		static bool wetWeatherBlinking;
		static int wetWeatherPeriodMs;
		static bool pitLimiterBlinking;
		static int pitLimiterPeriodMs;

		RearLight()
		{
			currentState = OFF;
			lightOn = false;
		}

		void setState(REAR_LIGHT_STATE newState)
		{
			if (newState != currentState)
			{

				/*
				F1 rear lights flash at different rates (Hz) to signal conditions:
				4Hz (fast) in the wet for visibility;
				2Hz (slow) for energy harvesting (ERS recovery) or Pit Limiter use; and
				2Hz for 10 secs after Safety Car/VSC;
				*/

				currentState = newState;
				switch (currentState)
				{
				case OFF:
					blinker.disable();
					lightOn = false;
					break;
				case WET:
					if (wetWeatherBlinking)
					{
						blinker.enable(wetWeatherPeriodMs);
					}
					else
					{
						blinker.disable();
						lightOn = true;
					}
					break;
				case BRAKE:
					blinker.disable();
					lightOn = true;
					break;
				case PIT_LIMITER:
					if (pitLimiterBlinking)
					{
						blinker.enable(pitLimiterPeriodMs);
					}
					else
					{
						blinker.disable();
						lightOn = true;
					}
					break;
				case CHARGE:
					blinker.enable(500); // 2Hz
					break;
				}
			}
		}

		bool update()
		{
			if (blinker.isEnabled())
			{
				lightOn = blinker.tick();
			}

			return lightOn;
		}
	};

	void LoadSettings(IniFile iniSettings);

	void ApplyPatches();
};

