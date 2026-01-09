#pragma once
#include "Helpers.h"
#include "CarDynamicData.h"
#include "..\GP4MemLib\GP4MemLib.h"
#include <map>

namespace GP4PP
{
	typedef std::map<CarDynamicData*, CarPitLaneState> PitLaneStates;

	class PitLimiter {

		PitLimiter() = delete;

		static bool tweaker;

		static const DWORD tweakerPitLimiterStateAddress = 0x40029658;

		static PitLaneStates pitLaneStates;

	public:

		/*
		*  Updates the pit limiter status and returns the new value
		*/
		static bool updateSatus(CarDynamicData* carDynData)
		{
			bool isPlayerCar = (carDynData->id >= 0x80);

			//if (/*tweaker && */isPlayerCar)
			if (GP4PP::isTweakerEnabled() && isPlayerCar)
			{
				
				// Get the car index (0-based)
				// Bit 7 is set for player car, so mask it out
				byte carId = (carDynData->id & 0x7F) - 1;

				// Get the pit limiter state from Tweaker memory
				// Each bit represents the pit limiter state for a car (1 = on, 0 = off)
				// Shift the bits to get the state for the specific car
				bool tweakerPitLimiterState = GP4MemLib::MemUtils::addressToValue<DWORD>(tweakerPitLimiterStateAddress) & (1 << carId);

				return tweakerPitLimiterState;
			}

			CarPitLaneState currentPitLaneState = pitLaneStates[carDynData];
			CarPitLaneState newPitLaneState = static_cast<CarPitLaneState>(carDynData->pitLaneState & 7);

			if ((currentPitLaneState != newPitLaneState) && // state changed
				((carDynData->pitSeq == CarPitSequence::PITSEQ_PIT_MENU) || // always update when in pit menu (force initial state to IN_PITLANE)
				(newPitLaneState != CarPitLaneState::PITSTATE_IN_PITLANE) || // always update when not in pitlane
				(currentPitLaneState != CarPitLaneState::PITSTATE_ON_TRACK))) // only update from "IN_PITLANE" if not coming from "ON_TRACK"
																				// (states go 0->1->2->1 when coming into pitlane)
			{
				pitLaneStates[carDynData] = newPitLaneState;

#ifdef DEBUG
				std::string debugCurrentStateStr = std::to_string(static_cast<int>(currentPitLaneState));
				std::string debugNewStateStr = std::to_string(static_cast<int>(newPitLaneState));

				switch (currentPitLaneState)
				{
				case CarPitLaneState::PITSTATE_ON_TRACK:
					debugCurrentStateStr += " - PITSTATE_ON_TRACK";
					break;
				case CarPitLaneState::PITSTATE_IN_PITLANE:
					debugCurrentStateStr += " - PITSTATE_IN_PITLANE";
					break;
				case CarPitLaneState::PITSTATE_ENTERING_PITS:
					debugCurrentStateStr += " - PITSTATE_ENTERING_PITS";
					break;
				case CarPitLaneState::PITSTATE_LEAVING_PITS:
					debugCurrentStateStr += " - PITSTATE_LEAVING_PITS";
					break;
				case CarPitLaneState::PITSTATE_UNKNOWN:
					debugCurrentStateStr += " - PITSTATE_UNKNOWN";
					break;
				default:
					break;
				}

				switch (newPitLaneState)
				{
				case CarPitLaneState::PITSTATE_ON_TRACK:
					debugNewStateStr += " - PITSTATE_ON_TRACK";
					break;
				case CarPitLaneState::PITSTATE_IN_PITLANE:
					debugNewStateStr += " - PITSTATE_IN_PITLANE";
					break;
				case CarPitLaneState::PITSTATE_ENTERING_PITS:
					debugNewStateStr += " - PITSTATE_ENTERING_PITS";
					break;
				case CarPitLaneState::PITSTATE_LEAVING_PITS:
					debugNewStateStr += " - PITSTATE_LEAVING_PITS";
					break;
				case CarPitLaneState::PITSTATE_UNKNOWN:
					debugNewStateStr += " - PITSTATE_UNKNOWN";
					break;
				default:
					break;
				}
				GP4PP::OutputGP4PPDebugString("Car " + std::to_string(static_cast<int>(carDynData->id)) + " status switched from " + debugCurrentStateStr + " to " + debugNewStateStr);
#endif // DEBUG
			}

			// Return true only if state is IN_PITLANE
			if(pitLaneStates[carDynData] == CarPitLaneState::PITSTATE_IN_PITLANE)
			{
				return true;
			}

			return false;

		};


	};

	//Static Definitions
	bool PitLimiter::tweaker = GP4PP::isTweakerEnabled();
	PitLaneStates PitLimiter::pitLaneStates;
}