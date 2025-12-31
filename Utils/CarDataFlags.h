#pragma once
#include <cstdint>

#pragma pack(push,1)

// offset 0x06F - Car::digitalControl
struct CarDigitalControlFlags {
    uint8_t throttle : 1; // bit 0 (1)  - throttle pressed
    uint8_t brake : 1; // bit 1 (2)  - brake pressed
    uint8_t steerLeft : 1; // bit 2 (4)  - steering left
    uint8_t steerRight : 1; // bit 3 (8)  - steering right
    uint8_t gearChange : 1; // bit 4 (0x10) - gear change active
    uint8_t reserved5 : 2;
    uint8_t downshift : 1; // bit 7 (0x80) - using downshift
};

// offset 0x071 - Car::flags_0x071
struct CarFlags71 {
    uint8_t reserved0 : 7;
    uint8_t pitLight : 1; // bit 7 (0x80) - pit light enabled
};

// offset 0x078 - Car::drivingAids
struct CarDrivingAidsFlags {
    uint8_t autoBrakes : 1; // bit 0 (1)
    uint8_t reserved1 : 2;
    uint8_t indestructible : 1; // bit 3 (8)
    uint8_t reserved4 : 1;
    uint8_t steeringHelp : 1; // bit 5 (0x20) - SH
    uint8_t reserved6 : 2;
};

// offset 0x7A - Car::flags_0x7A
struct CarFlags7A {
	uint8_t reserved0 : 4;
    uint8_t noDriver : 1; // bit 5 (0x20) - no driver in car
    uint8_t stopCar : 1; // bit 6 (0x40)
    uint8_t showCCFuel : 1; // bit 7 (0x80)
};

// offset 0x7B - Car::flags_0x7B
struct CarEngineFlags {
    //uint8_t engineStopped : 1; // bit 0 (1)
    //uint8_t reserved : 7;
    uint8_t reserved0 : 1;
	uint8_t engineRunning : 1; // bit 1 (2)
	uint8_t reserved2 : 1;
	uint8_t onPitStop : 1; // bit 3 (8)
	uint8_t onPitOptionsScreen : 1; // bit 4 (0x10) - unclear
    uint8_t reserved5 : 1;
	uint8_t onPits : 1; // bit 6 (0x40)
	uint8_t reserved7 : 1;
};

// offset 0x80 - Car::flagsDamage1
struct CarDamageFlags1 {
    uint8_t plankRear : 1; // bit 0 (1)
    uint8_t plankFront : 1; // bit 1 (2)
    uint8_t wheelFrontRight : 1; // bit 2 (4)
    uint8_t wheelFrontLeft : 1; // bit 3 (8)
    uint8_t wheelRearRight : 1; // bit 4 (0x10)
    uint8_t wheelRearLeft : 1; // bit 5 (0x20)
    uint8_t wingRear: 1; // bit 6 (0x40)
    uint8_t wingFront : 1; // bit 7 (0x80)
};

// offset 0x81 - Car::flagsDamage2
struct CarDamageFlags2 {
    uint8_t reserved : 5;
    uint8_t wingDamage : 1; // bit 5 (0x20)
    uint8_t plankRearRed : 1; // bit 6 (0x40)
    uint8_t plankFrontRed : 1; // bit 7 (0x80)
};

// offset 0x82 - Car::flagsLeds
struct CarLedsFlags {
    uint8_t displayNow : 1; // bit 0 (1)
    uint8_t driverName : 1; // bit 2 (4)
    uint8_t split : 2; // bits 3–4 (8/0x10)
    uint8_t warning : 1; // bit 5 (0x20)
    uint8_t failure : 1; // bit 6 (0x40)
	uint8_t logging : 1; // bit 7 (0x80) - start logging
};

// offset 0x83 - Car::flagsRaceFlags
struct CarRaceFlagsFlags {
	uint8_t reserved0 : 2;
    uint8_t blackFlag : 1; // bit 2 (4)
	uint8_t reserved3 : 1;
    uint8_t yellowFlag : 1; // bit 4 (0x10)
    uint8_t reserved6 : 3;
};




// bLCDRenderFlags_88 (_0x88): 1=speed,2=pos,4=#runners,8=#racelaps,_0x10=split,_0x20=timeprevbest,_0x40=#lapsdone
union LCDRenderFlags88 {
    uint8_t value;
    struct {
        uint8_t speed : 1;         // 0x01
        uint8_t pos : 1;           // 0x02
        uint8_t runners : 1;       // 0x04
        uint8_t racelaps : 1;      // 0x08
        uint8_t split : 1;         // 0x10
        uint8_t timePrevBest : 1;  // 0x20
        uint8_t lapsDone : 1;      // 0x40
        uint8_t _res : 1;          // 0x80
    } bits;
};

// bLCDRenderFlags_89 (_0x89): 1=car nr,2=laptime cur,4=laptime curinv,8=gap,_0x10=lap time,_0x20=fuel laps pit menu,_0x40=tyreset laps done,_0x80=fuel laps/eng.temp
union LCDRenderFlags89 {
    uint8_t value;
    struct {
        uint8_t carNr : 1;         // 0x01
        uint8_t lapTimeCur : 1;    // 0x02
        uint8_t lapTimeCurInv : 1; // 0x04
        uint8_t gap : 1;           // 0x08
        uint8_t lapTime : 1;       // 0x10
        uint8_t fuelLapsPitMenu : 1; // 0x20
        uint8_t tyreSetLapsDone : 1; // 0x40
        uint8_t fuelLapsEngTemp : 1; // 0x80
    } bits;
};

// bLCDRenderFlags_8A (_0x8A): 1=warning,2=failure,8=race pit menu,_0x10=laptime best,_0x20=split diff,_0x80=tyresel menu
union LCDRenderFlags8A {
    uint8_t value;
    struct {
        uint8_t warning : 1;       // 0x01
        uint8_t failure : 1;       // 0x02
        uint8_t _res1 : 1;         // 0x04
        uint8_t racePitMenu : 1;   // 0x08
        uint8_t lapTimeBest : 1;   // 0x10
        uint8_t splitDiff : 1;     // 0x20
        uint8_t _res2 : 1;         // 0x40
        uint8_t tyreSelMenu : 1;   // 0x80
    } bits;
};

// bLCDRenderFlags_8B (_0x8B): 0x10=fuel laps menu,_0x20=time remaining,_0x80=laps to go
union LCDRenderFlags8B {
    uint8_t value;
    struct {
        uint8_t _res0 : 4;
        uint8_t fuelLapsMenu : 1;  // 0x10
        uint8_t timeRemaining : 1; // 0x20
        uint8_t _res1 : 1;
        uint8_t lapsToGo : 1;      // 0x80
    } bits;
};

// flags_7A (_0x7A): 0x20=no driver in car,_0x40=stop car,_0x80=show cc fuel?
union Flags7A {
    uint8_t value;
    struct {
        uint8_t _res0 : 5;
        uint8_t noDriverInCar : 1; // 0x20
        uint8_t stopCar : 1;       // 0x40
        uint8_t showCCFuel : 1;    // 0x80
    } bits;
};

// flags_8D (_0x8D): 0x01=add extra weight?,_0x02=in pitlane,_0x04=session over,_0x10=had failure,_0x20=moved in pit menu,_0x80=not yet in pitbox
union PitFlags8D {
    uint8_t value;
    struct {
        uint8_t addExtraWeight : 1; // 0x01
        uint8_t inPitlane : 1;      // 0x02
        uint8_t sessionOver : 1;    // 0x04
        uint8_t _res1 : 1;          // 0x08
        uint8_t hadFailure : 1;     // 0x10
        uint8_t movedInPitMenu : 1; // 0x20
        uint8_t _res2 : 1;          // 0x40
        uint8_t notYetInPitbox : 1; // 0x80
    } bits;
};

// flags_CE (_0xCE): examples: 0x02=susp/transm,_0x08=water/oil/engine,_0x20=pit crew ready
union FlagsCE {
    uint8_t value;
    struct {
        uint8_t _res0 : 1;
        uint8_t suspTransm : 1; // 0x02
        uint8_t _res1 : 1;
        uint8_t waterOilEngine : 1; // 0x08
        uint8_t _res2 : 1;
        uint8_t pitCrewReady : 1; // 0x20
        uint8_t _res3 : 1;
        uint8_t _res4 : 1;
    } bits;
};

// flagsCtrl_CF (_0xCF): 0x08=FF,_0x10=use adv setup,_0x20=sep. shift,_0x40=7 gears
union CtrlFlagsCF {
    uint8_t value;
    struct {
        uint8_t _res0 : 3;
        uint8_t FF : 1;           // 0x08
        uint8_t useAdvSetup : 1;  // 0x10
        uint8_t sepShift : 1;     // 0x20
        uint8_t sevenGears : 1;   // 0x40
        uint8_t _res1 : 1;
    } bits;
};

// flags_E5 (_0xE5): 1=steer rel., 4=CC, rest=related to trouble (approx)
union FlagsE5 {
    uint8_t value;
    struct {
        uint8_t steerRel : 1; // 0x01
        uint8_t _res1 : 1;
        uint8_t CC : 1;       // 0x04
        uint8_t _res2 : 5;
    } bits;
};

// flags_71 (_0x71): bit 0x80 indicated pitlight in comments (map into single-bit wrapper)
union Flags71 {
    uint8_t value;
    struct {
        uint8_t _res0 : 7;
        uint8_t pitLight : 1; // 0x80
    } bits;
};

// flags_FE (_0xFE): comment mentions 0x04=pitlight
union FlagsFE {
    uint8_t value;
    struct {
        uint8_t _res0 : 2;
        uint8_t pitLight : 1; // 0x04
        uint8_t _res1 : 5;
    } bits;
};

// flags_230 (_0x230): comment mentions 0x80=car on jacks
union Flags230 {
    uint8_t value;
    struct {
        uint8_t _res0 : 7;
        uint8_t carOnJacks : 1; // 0x80
    } bits;
};

// Now the main Car struct with member names converted
typedef uint32_t WheelsD[4];  // 16 bytes
typedef uint8_t  WheelsB[4];  // 4 bytes

#pragma pack(pop)