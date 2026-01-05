/*
 * This header is derived from original work by René 'SDI' Smit - https://www.sdidit.nl/
 *
 * Contributions by Paulo 'Prblanco' Blanco - https://f1virtual.blogspot.com/
 *
 * Modifications have been made by Diego 'Öggo' Noriega.
 */


#pragma once
#include <cstdint>

#pragma pack(push,1)

// offset 0x06F - CarDynamicData::digitalControl
struct CarDigitalControlFlags {
    uint8_t throttle : 1; // bit 0 (1)  - throttle pressed
    uint8_t brake : 1; // bit 1 (2)  - brake pressed
    uint8_t steerLeft : 1; // bit 2 (4)  - steering left
    uint8_t steerRight : 1; // bit 3 (8)  - steering right
    uint8_t gearChange : 1; // bit 4 (0x10) - gear change active
    uint8_t reserved5 : 2;
    uint8_t downshift : 1; // bit 7 (0x80) - using downshift
};

// offset 0x071 - CarDynamicData::flags_0x071
struct CarFlags71 {
    uint8_t reserved0 : 7;
    uint8_t pitLight : 1; // bit 7 (0x80) - pit light enabled
};

// offset 0x078 - CarDynamicData::drivingAids
struct CarDrivingAidsFlags {
	uint8_t autoBrakes : 1; // bit 0 (1) - auto brakes
	uint8_t autoGears : 1; // bit 1 (2) - auto gears
	uint8_t autoRightCar : 1; // bit 2 (4) - auto right car
	uint8_t indestructible : 1; // bit 3 (8) - indestructible
	uint8_t tracionControl : 1; // bit 4 (0x10) - traction control
	uint8_t steeringHelp : 1; // bit 5 (0x20) - steering help
	uint8_t launchControl : 1; // bit 6 (0x40) - launch control
};

// offset 0x7A - CarDynamicData::flags_0x7A
struct CarFlags7A {
	uint8_t reserved0 : 4;
    uint8_t noDriver : 1; // bit 5 (0x20) - no driver in car
	uint8_t stopCar : 1; // bit 6 (0x40) - stop car
	uint8_t showCCFuel : 1; // bit 7 (0x80) - show CC fuel - unclear
};

// offset 0x7B - CarDynamicData::flags_0x7B
struct CarEngineFlags {
    uint8_t reserved0 : 1;
	uint8_t engineRunning : 1; // bit 1 (2) - engine running
	uint8_t reserved2 : 1;
	uint8_t onPitStop : 1; // bit 3 (8) - on pit stop
	uint8_t onPitOptionsScreen : 1; // bit 4 (0x10) - on pit options screen - unclear
    uint8_t reserved5 : 1;
	uint8_t onPits : 1; // bit 6 (0x40) - on pits
	uint8_t reserved7 : 1;
};

// offset 0x80 - CarDynamicData::flagsDamage1
struct CarDamageFlags1 {
	uint8_t plankRear : 1; // bit 0 (1) - rear plank
	uint8_t plankFront : 1; // bit 1 (2) - front plank
	uint8_t wheelFrontRight : 1; // bit 2 (4) - front right wheel
	uint8_t wheelFrontLeft : 1; // bit 3 (8) - front left wheel
	uint8_t wheelRearRight : 1; // bit 4 (0x10) - rear right wheel
	uint8_t wheelRearLeft : 1; // bit 5 (0x20) - rear left wheel
	uint8_t wingRear : 1; // bit 6 (0x40) - rear wing
	uint8_t wingFront : 1; // bit 7 (0x80) - front wing
};

// offset 0x81 - CarDynamicData::flagsDamage2
struct CarDamageFlags2 {
    uint8_t reserved : 4;
	uint8_t blackFlag : 1; // bit 4 (0x10) - black flag according to Prblanco
	uint8_t wingDamage : 1; // bit 5 (0x20) - wing damage
	uint8_t plankRearRed : 1; // bit 6 (0x40) - rear plank red
	uint8_t plankFrontRed : 1; // bit 7 (0x80) - front plank red
};

// offset 0x82 - CarDynamicData::flagsLeds
struct CarLedsFlags {
    uint8_t displayNow : 1; // bit 0 (1)
    uint8_t driverName : 1; // bit 2 (4)
    uint8_t split : 2; // bits 3–4 (8/0x10)
    uint8_t warning : 1; // bit 5 (0x20)
    uint8_t failure : 1; // bit 6 (0x40)
	uint8_t logging : 1; // bit 7 (0x80) - start logging
};

// offset 0x83 - CarDynamicData::flagsRaceFlags
struct CarRaceFlagsFlags {
	uint8_t reserved0 : 2;
    uint8_t blackFlag : 1; // bit 2 (4)
	uint8_t reserved3 : 1;
    uint8_t yellowFlag : 1; // bit 4 (0x10)
    uint8_t reserved6 : 3;
};

// offset 0x86 - CarDynamicData::flags_0x086
struct CarFlags86 {
	uint8_t relatedRearWingDamage : 1; // bit 0 (1) - related to rear wing damage
	uint8_t relatedFrontWingDamage : 1; // bit 1 (2) - related to front wing damage
    uint8_t reserved2 : 6;
};

// offset 0x88 - CarDynamicData::lcdRenderFlags_0x088
// LCD render selection flags (main info line)
struct CarLCDRenderFlags88 {
    uint8_t showSpeed : 1; // bit 0 (0x01) - speed
    uint8_t showPosition : 1; // bit 1 (0x02) - position
    uint8_t showNumRunners : 1; // bit 2 (0x04) - number of runners
    uint8_t showRaceLaps : 1; // bit 3 (0x08) - number of race laps
    uint8_t showSplit : 1; // bit 4 (0x10) - split? (uncertain)
    uint8_t showPrevBestTime : 1; // bit 5 (0x20) - previous best lap time
    uint8_t showLapsDone : 1; // bit 6 (0x40) - laps completed
    uint8_t reserved : 1; // bit 7
};

// offset 0x89 - CarDynamicData::lcdRenderFlags_0x089
// LCD render selection flags (timing / gap info)
struct CarLCDRenderFlags89 {
    uint8_t showCarNumber : 1; // bit 0 (0x01) - car number
    uint8_t showLapTimeCurrent : 1; // bit 1 (0x02) - current lap time
    uint8_t showLapTimeInverse : 1; // bit 2 (0x04) - inverted current lap time
    uint8_t showGap : 1; // bit 3 (0x08) - gap to car ahead/behind
    uint8_t showLapTime : 1; // bit 4 (0x10) - lap time
    uint8_t showFuelLapsPitMenu : 1; // bit 5 (0x20) - fuel laps (pit menu)
    uint8_t showTyreSetLapsDone : 1; // bit 6 (0x40) - tyre set laps completed
    uint8_t showFuelOrEngTemp : 1; // bit 7 (0x80) - fuel laps / engine temp
};

// offset 0x8A - CarDynamicData::lcdRenderFlags_0x08A
// LCD render selection flags (warnings / menus)
struct CarLCDRenderFlags8A {
    uint8_t showWarning : 1; // bit 0 (0x01) - warning message
    uint8_t showFailure : 1; // bit 1 (0x02) - failure message
    uint8_t reserved2 : 1;
    uint8_t showRacePitMenu : 1; // bit 3 (0x08) - race pit menu
    uint8_t showBestLapTime : 1; // bit 4 (0x10) - best lap time
    uint8_t showSplitDiff : 1; // bit 5 (0x20) - split difference
    uint8_t reserved6 : 1;
    uint8_t showTyreSelMenu : 1; // bit 7 (0x80) - tyre selection menu
};

// offset 0x8B - CarDynamicData::lcdRenderFlags_0x08B
// LCD render selection flags (race progress info)
struct CarLCDRenderFlags8B {
    uint8_t reserved0 : 4;
    uint8_t showFuelLapsMenu : 1; // bit 4 (0x10) - fuel laps menu
    uint8_t showTimeRemaining : 1; // bit 5 (0x20) - time remaining
    uint8_t reserved6 : 1;
    uint8_t showLapsToGo : 1; // bit 7 (0x80) - laps remaining
};

// offset 0x8D - CarDynamicData::flagsPit
struct CarPitFlags {
    uint8_t addExtraWeight : 1; // bit 0 (1)
    uint8_t inPitLane : 1; // bit 1 (2)
    uint8_t sessionOver : 1; // bit 2 (4)
	uint8_t reserved3 : 1;
    uint8_t hadFailure : 1; // bit 4 (0x10)
    uint8_t movedInMenu : 1; // bit 5 (0x20)
	uint8_t reserved6 : 1;
    uint8_t notInPitBox : 1; // bit 7 (0x80)
};

// offset 0xCE - CarDynamicData::flags_0x0CE
// Engine / pit-related trouble flags
struct CarFlagsCE {
	uint8_t outOfFuel : 1; // bit 0 (0x01) - out of fuel
    uint8_t suspensionTransm : 1; // bit 1 (0x02) - suspension / transmission trouble
    uint8_t reserved2 : 1;
    uint8_t waterOilEngine : 1; // bit 3 (0x08) - water / oil / engine trouble
	uint8_t reserved4 : 1; // bit 4 (0x10) - always on for player car?
    uint8_t pitCrewReady : 1; // bit 5 (0x20) - pit crew ready?
	uint8_t wallCollision : 1; // bit 6 (0x40) - wall collision
    uint8_t reserved7 : 1;
};

// offset 0xCF - CarDynamicData::flagsCtrl_0x0CF
struct CarCtrlFlags {
	uint8_t reserved0 : 3;
    uint8_t forceFeedback : 1; // bit 3 (8)
    uint8_t advancedSetup : 1; // bit 4 (0x10)
    uint8_t separateShift : 1; // bit 5 (0x20)
    uint8_t sevenGears : 1; // bit 6 (0x40)
    uint8_t reserved7 : 1;
};

// offset 0xE4 - CarDynamicData::pitSeq
// Pit stop state machine
enum CarPitSequence : uint8_t {
    PITSEQ_ON_TRACK = 0, // on track
    PITSEQ_CROSSED_PIT_SF = 1, // crossed pit start/finish
    PITSEQ_ON_JACKS = 2, // car on jacks
    PITSEQ_PIT_MENU = 3, // pit menu active
    PITSEQ_WANT_LEAVE_PIT = 4, // driver wants to leave pit
    PITSEQ_OFF_JACKS = 5, // car off jacks
    PITSEQ_JACKS_AWAY = 6, // jacks removed
    PITSEQ_LOLLIPOP_AWAY = 7, // lollipop man away
    PITSEQ_DRIVING_TO_EXIT = 8  // driving to pit exit
};

// offset 0xE5 - CarDynamicData::flags_0x0E5
// Steering / CC / trouble indicators
struct CarFlagsE5 {
    uint8_t steerRelative : 1; // bit 0 (0x01) - steering relative mode
    uint8_t reserved1 : 1;
    uint8_t CC : 1; // bit 2 (0x04) - CC
    uint8_t reserved3 : 1;
    uint8_t troubleFlags : 4; // bits 4–7 - related to trouble/failure states
};

// offset 0xE6 - CarDynamicData::analogControl
// Analog input modes and device availability
struct CarAnalogControlFlags {
    uint8_t clutchMode : 1; // bit 0 (0x01) - clutch mode
    uint8_t brakeMode : 1; // bit 1 (0x02) - brake mode
    uint8_t accelMode : 1; // bit 2 (0x04) - accelerator mode
    uint8_t steerMode : 1; // bit 3 (0x08) - steering mode
    uint8_t clutchDevice : 1; // bit 4 (0x10) - clutch device
    uint8_t brakeDevice : 1; // bit 5 (0x20) - brake device
    uint8_t throttleDevice : 1; // bit 6 (0x40) - throttle device
    uint8_t steerDevice : 1; // bit 7 (0x80) - steering device
};


// offset 0xFE - CarDynamicData::flags_0x0FE
struct CarFlagsFE {
    uint8_t reserved0 : 2;
	uint8_t pitLight : 1; // bit 2 (0x04) - pit light
	uint8_t reserved2 : 5;
};

// offset 0x122 - CarDynamicData::flags_0x122
struct CarFlags122 {
    uint8_t reserved0 : 1;
    uint8_t blueFlag : 1; // bit 1 (0x2) - blue flag
    uint8_t reserved2 : 6;
};

// offset 0x161 - CarDynamicData::flags_0x161
struct CarFlags161 {
    uint8_t reserved0 : 2;
    uint8_t willGetFailure : 1; // bit 2 (0x04) - will get failure
    uint8_t reserved3 : 1;
    uint8_t split2Checked : 1; // bit 4 (0x10) - split 2 checked
    uint8_t split1Checked : 1; // bit 5 (0x20) - split 1 checked
    uint8_t outOfFuel : 1; // bit 6 (0x40) - out of fuel
    uint8_t reserved7 : 1;
};

// offset 0x162 - CarDynamicData::flags_0x162
struct CarFlags162 {
    uint8_t reserved0 : 4;
    uint8_t timesDq : 1; // bit 4 (0x10) - times dq
    uint8_t reserved5 : 3;
};

// offset 0x203 - CarDynamicData::flags_0x203
struct CarFlags203 {
    uint8_t reserved0 : 4;
    uint8_t relToDamage : 1; // bit 4 (0x10) - related to damage
    uint8_t reserved5 : 3;
};

// offset 0x230 - CarDynamicData::flags_0x230
struct CarFlags230 {
    uint8_t reserved0 : 7;
    uint8_t carOnJacks : 1; // bit 7 (0x80) - car on jacks
};

// offset 0x231 - CarDynamicData::flags_0x231
struct CarFlags231 {
    uint8_t reserved0 : 5;
    uint8_t pitLightOff : 1; // bit 5 (0x20) - pit light turned off
    uint8_t reserved6 : 2;
};


// Now the main CarDynamicData struct with member names converted
typedef uint32_t WheelsD[4];  // 16 bytes
typedef uint8_t  WheelsB[4];  // 4 bytes

// offset 0x12 - TrackSector::flags_0x012
struct SegFlags12 {
	uint8_t endOfLap : 1; // bit 0 (1) - end of lap
	uint8_t waveFinishFlag : 1; // bit 1 (2) - wave finish flag
	uint8_t reserved5 : 6;
};

// offset 0x48 - TrackSector::pitFlags
struct SegPitFlags {
	uint32_t pitEntry : 1; // bit 0 (1) - pit entry
	uint32_t pitExit : 1; // bit 1 (2) - pit exit
	uint32_t kerbTypeSel : 1; // bit 2 (4) - kerb type selection
	uint32_t reserved3 : 1;
	uint32_t bridgedRightFence : 1; // bit 4 (0x10) - bridged right fence (off at start and end of pitlane)
	uint32_t bridgedLeftFence : 1; // bit 5 (0x20) - bridged left fence (off at start and end of pitlane)
	uint32_t reserved6 : 2;
	uint32_t correctRightYellowPitLine : 1; // bit 8 (0x100) - correct right yellow pit line
	uint32_t correctLeftYellowPitLine : 1; // bit 9 (0x200) - correct left yellow pit line
	uint32_t reserved : 22;
};

//00000051 flags_51        db ? ; 2 = black flag area right 4 = left

// offset 0x51 - TrackSector::flags_0x051
struct SegFlags51 {
	uint8_t reserved0 : 1;
	uint8_t blackFlagAreaRight : 1; // bit 1 (2) - black flag area right
	uint8_t blackFlagAreaLeft : 1; // bit 2 (4) - black flag area left
	uint8_t reserved2 : 5;
};

// offset 0x64 - TrackSector::flagsSplit
struct SegFlagsSplit {
    uint8_t split1 : 1; // bit 0 (1) - split 1
    uint8_t split2 : 1; // bit 1 (2) - split 2
    uint8_t reserved2 : 1;
    uint8_t reserved3 : 1;
    uint8_t setByTCd2 : 1; // bit 4 (0x10) - set by TC0xd2
    uint8_t setByTCdc : 1; // bit 5 (0x20) - set by TC0xdc
    uint8_t setByTCdd : 1; // bit 6 (0x40) - set by TC0xdd
    uint8_t setByTrackTable : 1; // bit 7 (0x80) - set by track table
};

// offset 0x70 - TrackSector::flags_0x070
struct SegFlags70 {
    uint8_t reserved0 : 1;
    uint8_t designateDryArea : 1; // bit 1 (2) - designate dry area
    uint8_t reserved2 : 6;
};

#pragma pack(pop)