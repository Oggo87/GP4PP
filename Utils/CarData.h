#pragma once
#include "CarDataFlags.h"
#include <cstdint>

#pragma pack(push,1)

typedef struct Car
{
    /* 0x000 */ int32_t  posX;
    /* 0x004 */ int32_t  posY;
    /* 0x008 */ int32_t  posZ;
    /* 0x00C */ int32_t  angleZ;
    /* 0x010 */ int32_t  angleX;
    /* 0x014 */ int32_t  angleY;
    /* 0x018 */ uint16_t speedAngleZ;
    /* 0x01A */ uint8_t  field_0x01A;
    /* 0x01B */ uint8_t  field_0x01B;
    /* 0x01C */ struct Seg * pSeg;

    /* 0x020 */ uint32_t timeLastSplit1;
    /* 0x024 */ uint32_t timeLastSplit2;
    /* 0x028 */ uint32_t timeLast;
    /* 0x02C */ uint32_t timeBestSplit1;
    /* 0x030 */ uint32_t timeBestSplit2;
    /* 0x034 */ uint32_t timeBest;
    /* 0x038 */ uint32_t timePrevBest;

    /* 0x03C */ int32_t  steer;

    /* 0x040 */ uint32_t invisibleBits;
    /* 0x044 */ uint32_t prevInvisibleBits;
    /* 0x048 */ uint32_t field_0x048;
    /* 0x04C */ uint32_t timerLeds;
    /* 0x050 */ uint32_t timer_0x050;
    /* 0x054 */ uint32_t timerBlackFlag;

    /* 0x058 */ uint16_t extCameraPosX;
    /* 0x05A */ uint16_t extCameraPosZ;
    /* 0x05C */ uint16_t extCameraPosY;
    /* 0x05E */ uint16_t forceFeedback;

    /* 0x060 */ uint16_t fuelLaps;
    /* 0x062 */ uint16_t revs;
    /* 0x064 */ uint16_t carHeight;
    /* 0x066 */ uint16_t segNrDist_0x066;
    /* 0x068 */ uint16_t segNr;
    /* 0x06A */ uint16_t steerRaw;

    /* 0x06C */ uint8_t  brakeRaw;
    /* 0x06D */ uint8_t  clutchRaw;
    /* 0x06E */ uint8_t  throttleRaw;
    /* 0x06F */ CarDigitalControlFlags  digitalControl;

    /* 0x070 */ uint8_t  curLap;
    /* 0x071 */ CarFlags71  flags_0x71;
    /* 0x072 */ uint8_t  gear;
    /* 0x073 */ uint8_t  teamNr;
    /* 0x074 */ uint8_t  split;
    /* 0x075 */ uint8_t  failureWheel;
    /* 0x076 */ uint8_t  field_0x076;
    /* 0x077 */ uint8_t  field_0x077;

    /* 0x078 */ CarDrivingAidsFlags  drivingAids;
    /* 0x079 */ uint8_t  field_0x079;
    /* 0x07A */ CarFlags7A  flags_0x7A;
    /* 0x07B */ CarEngineFlags  flags_0x7B;
    /* 0x07C */ uint8_t  id;
    /* 0x07D */ uint8_t  trackOrderIndex;
    /* 0x07E */ uint8_t  field_0x07E;
    /* 0x07F */ uint8_t  tyreType;

    /* 0x080 */ CarDamageFlags1  flagsDamage1;
    /* 0x081 */ CarDamageFlags2  flagsDamage2;
    /* 0x082 */ CarLedsFlags  flagsLeds;
    /* 0x083 */ CarRaceFlagsFlags  flagsRaceFlags;
    /* 0x084 */ uint8_t  failureType;
    /* 0x085 */ uint8_t  warning;
    /* 0x086 */ uint8_t  field_0x086;
    /* 0x087 */ uint8_t  field_0x087;

    /* 0x088 */ uint8_t  lcdRenderFlags_0x88;
    /* 0x089 */ uint8_t  lcdRenderFlags_0x89;
    /* 0x08A */ uint8_t  lcdRenderFlags_0x8A;
    /* 0x08B */ uint8_t  lcdRenderFlags_0x8B;
    /* 0x08C */ uint8_t  field_0x08C;
    /* 0x08D */ uint8_t  pitFlags_0x8D;
    /* 0x08E */ uint8_t  field_0x08E;
    /* 0x08F */ uint8_t  field_0x08F;

    /* 0x090 */ uint16_t segPosX;
    /* 0x092 */ uint16_t segPosY;
    /* 0x094 */ uint16_t field_0x094;
    /* 0x096 */ uint16_t segDist;

    /* 0x098 */ int32_t  speed;
    /* 0x09C */ uint16_t flags_0x9C;
    /* 0x09E */ uint8_t  failureLapNr;
    /* 0x09F */ uint8_t  failureSeg;

    /* 0x0A0 */ uint16_t field_0x0A0;
    /* 0x0A2 */ uint16_t field_0x0A2;
    /* 0x0A4 */ int32_t  speedX;
    /* 0x0A8 */ int32_t  speedY;
    /* 0x0AC */ int32_t  field_0x0AC;

    /* 0x0B0 */ uint16_t field_0x0B0;
    /* 0x0B2 */ uint16_t weight;
    /* 0x0B4 */ uint16_t steerAfterSH;
    /* 0x0B6 */ uint16_t field_0x0B6;
    /* 0x0B8 */ uint16_t field_0x0B8;
    /* 0x0BA */ uint16_t field_0x0BA;
    /* 0x0BC */ uint32_t field_0x0BC;

    /* 0x0C0 */ uint32_t timeAbsLast;
    /* 0x0C4 */ uint16_t field_0x0C4;
    /* 0x0C6 */ uint16_t field_0x0C6;
    /* 0x0C8 */ uint16_t field_0x0C8;
    /* 0x0CA */ uint16_t field_0x0CA;
    /* 0x0CC */ uint8_t  flagsCC;
    /* 0x0CD */ uint8_t  flagsCD;
    /* 0x0CE */ uint8_t  flagsCE;
    /* 0x0CF */ uint8_t  ctrlFlagsCF;

    /* 0x0D0 */ uint32_t field_0x0D0;
    /* 0x0D4 */ uint16_t field_0x0D4;
    /* 0x0D6 */ uint16_t weightFactor;
    /* 0x0D8 */ uint16_t ccSpeed;
    /* 0x0DA */ uint16_t field_0x0DA;
    /* 0x0DC */ uint16_t field_0x0DC;
    /* 0x0DE */ uint16_t field_0x0DE;

    /* 0x0E0 */ uint16_t field_0x0E0;
    /* 0x0E2 */ uint16_t numFeetIntoLap;
    /* 0x0E4 */ uint8_t  pitSeq;
    /* 0x0E5 */ uint8_t  flagsE5;
    /* 0x0E6 */ uint8_t  analogControl;
    /* 0x0E7 */ uint8_t  flagsE7;

    /* 0x0E8 */ uint16_t field_0x0E8;
    /* 0x0EA */ uint16_t field_0x0EA;
    /* 0x0EC */ uint16_t field_0x0EC;
    /* 0x0EE */ uint16_t throttle;

    /* 0x0F0 */ uint32_t timerPitSeq;
    /* 0x0F4 */ uint16_t field_0x0F4;
    /* 0x0F6 */ uint16_t power;
    /* 0x0F8 */ uint32_t drivenDistInSegs;

    /* 0x0FC */ uint8_t  field_0x0FC;
    /* 0x0FD */ uint8_t  teamIndex_0x0FD;
    /* 0x0FE */ uint8_t  flags_0x0FE;
    /* 0x0FF */ uint8_t  flags_0x0FF;

    /* 0x100 */ uint16_t field_0x100;
    /* 0x102 */ uint16_t field_0x102;
    /* 0x104 */ uint32_t setupDiff_0x104;
    /* 0x108 */ uint16_t field_0x108;
    /* 0x10A */ uint16_t field_0x10A;
    /* 0x10C */ uint16_t field_0x10C;
    /* 0x10E */ uint16_t grip;

    /* 0x110 */ uint32_t pCar_0x110;
    /* 0x114 */ uint32_t pCar_0x114;
    /* 0x118 */ uint32_t pCar_0x118;

    /* 0x11C */ uint16_t field_0x11C;
    /* 0x11E */ uint16_t field_0x11E;
    /* 0x120 */ uint8_t  field_0x120;
    /* 0x121 */ uint8_t  gear_0x121;
    /* 0x122 */ uint8_t  flags_0x122;
    /* 0x123 */ uint8_t  flags_0x123;

    /* 0x124 */ uint32_t pCar_0x124;
    /* 0x128 */ uint32_t pCar_0x128;
    /* 0x12C */ uint32_t pCar_0x12C;
    /* 0x130 */ uint32_t pCar_0x130;

    /* 0x134 */ uint8_t  flags_0x134;
    /* 0x135 */ uint8_t  flags_0x135;
    /* 0x136 */ uint8_t  numPitstopsDone;
    /* 0x137 */ uint8_t  flags_0x137;

    /* 0x138 */ uint16_t field_0x138;
    /* 0x13A */ uint16_t field_0x13A;
    /* 0x13C */ uint16_t field_0x13C;
    /* 0x13E */ uint16_t field_0x13E;
    /* 0x140 */ uint16_t field_0x140;
    /* 0x142 */ uint16_t field_0x142;
    /* 0x144 */ uint16_t field_0x144;
    /* 0x146 */ uint16_t field_0x146;

    /* 0x148 */ uint32_t field_0x148;
    /* 0x14C */ uint32_t field_0x14C;
    /* 0x150 */ uint32_t field_0x150;
    /* 0x154 */ uint32_t field_0x154;
    /* 0x158 */ uint32_t field_0x158;
    /* 0x15C */ uint32_t field_0x15C;

    /* 0x160 */ uint8_t  tyreSetRel_0x160;
    /* 0x161 */ uint8_t  flags_0x161;
    /* 0x162 */ uint8_t  flags_0x162;
    /* 0x163 */ uint8_t  flags_0x163;

    /* 0x164 */ uint32_t downforceRearWing;
    /* 0x168 */ uint16_t field_0x168;
    /* 0x16A */ uint16_t downforceFrontWing;
    /* 0x16C */ uint32_t field_0x16C;
    /* 0x170 */ uint16_t field_0x170;
    /* 0x172 */ uint8_t  differential;
    /* 0x173 */ uint8_t  ffScaling;
    /* 0x174 */ uint16_t field_0x174;
    /* 0x176 */ uint16_t field_0x176;

    /* 0x178 */ uint8_t  steerHelp;
    /* 0x179 */ uint8_t  flags_0x179;
    /* 0x17A */ uint8_t  steerLSZ;
    /* 0x17B */ uint8_t  throttleLSZ;
    /* 0x17C */ uint32_t time_0x17C;
    /* 0x180 */ uint16_t field_0x180;
    /* 0x182 */ uint8_t  field_0x182;
    /* 0x183 */ uint8_t  field_0x183;
    /* 0x184 */ uint16_t field_0x184;
    /* 0x186 */ uint16_t field_0x186;

    /* 0x188 */ WheelsD  wheelsD_0x188;

    /* 0x198 */ uint8_t  plannedNumPitstops;
    /* 0x199 */ uint8_t  plannedPitStop1_0x199;
    /* 0x19A */ uint8_t  plannedPitStop2_0x19A;
    /* 0x19B */ uint8_t  plannedPitStop3_0x19B;
    /* 0x19C */ uint32_t fuel;
    /* 0x1A0 */ uint32_t field_0x1A0;
    /* 0x1A4 */ uint32_t field_0x1A4;

    /* 0x1A8 */ uint8_t  field_0x1A8;
    /* 0x1A9 */ uint8_t  field_0x1A9;
    /* 0x1AA */ uint8_t  field_0x1AA;
    /* 0x1AB */ uint8_t  field_0x1AB;
    /* 0x1AC */ uint16_t field_0x1AC;
    /* 0x1AE */ uint16_t field_0x1AE;

    /* 0x1B0 */ uint8_t  field_0x1B0;
    /* 0x1B1 */ uint8_t  field_0x1B1;
    /* 0x1B2 */ uint8_t  field_0x1B2;
    /* 0x1B3 */ uint8_t  field_0x1B3;
    /* 0x1B4 */ uint16_t field_0x1B4;
    /* 0x1B6 */ uint16_t field_0x1B6;
    /* 0x1B8 */ uint32_t pSeg_0x1B8;
    /* 0x1BC */ uint16_t carOS_0x1BC;
    /* 0x1BE */ uint16_t field_0x1BE;
    /* 0x1C0 */ uint16_t field_0x1C0;
    /* 0x1C2 */ uint16_t field_0x1C2;
    /* 0x1C4 */ uint8_t  field_0x1C4;
    /* 0x1C5 */ uint8_t  field_0x1C5;

    /* 0x1C6 */ uint8_t  gearRatios[7];
    /* 0x1CD */ uint8_t  numGears;
    /* 0x1CE */ uint8_t  field_0x1CE;
    /* 0x1CF */ uint8_t  field_0x1CF;

    /* 0x1D0 */ uint8_t  maxLock;
    /* 0x1D1 */ uint8_t  steerRWS;
    /* 0x1D2 */ uint8_t  brakeLSZ;
    /* 0x1D3 */ uint8_t  clutchLSZ;
    /* 0x1D4 */ uint32_t time1D4;
    /* 0x1D8 */ uint32_t timerPitSeq3;
    /* 0x1DC */ uint32_t timerPitSeq2;
    /* 0x1E0 */ uint16_t field_0x1E0;
    /* 0x1E2 */ uint16_t brakeBias;
    /* 0x1E4 */ uint32_t timerFailure;
    /* 0x1E8 */ uint32_t timer1E8;
    /* 0x1EC */ uint32_t timer1EC;
    /* 0x1F0 */ uint32_t timer1F0;

    /* 0x1F4 */ uint8_t  field_0x1F4;
    /* 0x1F5 */ uint8_t  flags1F5;
    /* 0x1F6 */ uint8_t  field_0x1F6;
    /* 0x1F7 */ uint8_t  flags1F7;
    /* 0x1F8 */ uint8_t  field_0x1F8;
    /* 0x1F9 */ uint8_t  field_0x1F9;
    /* 0x1FA */ uint8_t  field_0x1FA;
    /* 0x1FB */ uint8_t  field_0x1FB;
    /* 0x1FC */ uint32_t timer1FC;

    /* 0x200 */ uint8_t  field_0x200;
    /* 0x201 */ uint8_t  index_0x201;
    /* 0x202 */ uint8_t  carID_0x202;
    /* 0x203 */ uint8_t  flags_0x203;
    /* 0x204 */ uint16_t field_0x204;
    /* 0x206 */ uint16_t field_0x206;
    /* 0x208 */ uint16_t field_0x208;
    /* 0x20A */ uint16_t field_0x20A;
    /* 0x20C */ uint8_t  field_0x20C;
    /* 0x20D */ uint8_t  field_0x20D;
    /* 0x20E */ uint8_t  field_0x20E;
    /* 0x20F */ uint8_t  field_0x20F;

    /* 0x210 */ WheelsB  wheelsB_0x210;
    /* 0x214 */ WheelsB  wheelsB_0x214;
    /* 0x218 */ WheelsB  wheelsB_0x218;

    /* 0x21C */ uint16_t field_0x21C;
    /* 0x21E */ uint16_t field_0x21E;
    /* 0x220 */ uint32_t timer220;
    /* 0x224 */ uint16_t fuelLaps_0x224;
    /* 0x226 */ uint8_t  tyreSetSelLapsDone;
    /* 0x227 */ uint8_t  rearWing_0x227;
    /* 0x228 */ uint32_t field_0x228;
    /* 0x22C */ uint32_t field_0x22C;

    /* 0x230 */ uint8_t  flags_0x230;
    /* 0x231 */ uint8_t  flags_0x231;
    /* 0x232 */ uint8_t  field_0x232;
    /* 0x233 */ uint8_t  field_0x233;
    /* 0x234 */ uint8_t  field_0x234;
    /* 0x235 */ uint8_t  counter_0x235;
    /* 0x236 */ uint8_t  field_0x236;
    /* 0x237 */ uint8_t  field_0x237;
    /* 0x238 */ uint16_t field_0x238;
    /* 0x23A */ uint16_t field_0x23A;
    /* 0x23C */ uint16_t field_0x23C;
    /* 0x23E */ uint16_t field_0x23E;
    /* 0x240 */ uint32_t field_0x240;
    /* 0x244 */ uint32_t engineTemperature;

    /* 0x248 */ uint8_t  field_0x248;
    /* 0x249 */ uint8_t  field_0x249;
    /* 0x24A */ uint8_t  brake_0x24A;
    /* 0x24B */ uint8_t  field_0x24B;
    /* 0x24C */ uint16_t field_0x24C;
    /* 0x24E */ uint16_t field_0x24E;
    /* 0x250 */ uint32_t timer250;

    /* 0x254 */ uint8_t  field_0x254;
    /* 0x255 */ uint8_t  field_0x255;
    /* 0x256 */ uint8_t  field_0x256;
    /* 0x257 */ uint8_t  field_0x257;
    /* 0x258 */ uint8_t  field_0x258;
    /* 0x259 */ uint8_t  field_0x259;
    /* 0x25A */ uint8_t  field_0x25A;
    /* 0x25B */ uint8_t  field_0x25B;
    /* 0x25C */ uint8_t  field_0x25C;
    /* 0x25D */ uint8_t  field_0x25D;
    /* 0x25E */ uint8_t  field_0x25E;
    /* 0x25F */ uint8_t  field_0x25F;
    /* 0x260 */ uint32_t timer260;
    /* 0x264 */ uint16_t revsRel_0x264;
    /* 0x266 */ uint8_t  diffDec;
    /* 0x267 */ uint8_t  diffAcc;
    /* 0x268 */ uint8_t  field_0x268;
    /* 0x269 */ uint8_t  field_0x269;
    /* 0x26A */ uint8_t  frontWing_0x26A;
    /* 0x26B */ uint8_t  field_0x26B;
    /* 0x26C */ uint32_t field_0x26C;
    /* 0x270 */ uint16_t field_0x270;
    /* 0x272 */ uint8_t  field_0x272;
    /* 0x273 */ uint8_t  field_0x273;
    /* 0x274 */ uint32_t timer274;
    /* 0x278 */ uint16_t field_0x278;
    /* 0x27A */ uint16_t field_0x27A;
    /* 0x27C */ uint16_t field_0x27C;
    /* 0x27E */ uint16_t field_0x27E;
    /* 0x280 */ uint16_t field_0x280;
    /* 0x282 */ uint16_t field_0x282;
    /* 0x284 */ uint32_t field_0x284;
    /* 0x288 */ uint32_t field_0x288;
    /* 0x28C */ uint32_t field_0x28C;

    /* 0x290 */ uint8_t  field_0x290;
    /* 0x291 */ uint8_t  field_0x291;
    /* 0x292 */ uint8_t  field_0x292;
    /* 0x293 */ uint8_t  field_0x293;
    /* 0x294 */ uint8_t  field_0x294;
    /* 0x295 */ uint8_t  field_0x295;
    /* 0x296 */ uint8_t  field_0x296;
    /* 0x297 */ uint8_t  field_0x297;
    /* 0x298 */ uint8_t  field_0x298;
    /* 0x299 */ uint8_t  field_0x299;
    /* 0x29A */ uint8_t  field_0x29A;
    /* 0x29B */ uint8_t  field_0x29B;
    /* 0x29C */ uint8_t  field_0x29C;
    /* 0x29D */ uint8_t  field_0x29D;
    /* 0x29E */ uint8_t  field_0x29E;
    /* 0x29F */ uint8_t  field_0x29F;
    /* 0x2A0 */ uint32_t timer2A0;
    /* 0x2A4 */ uint8_t  field_0x2A4;
    /* 0x2A5 */ uint8_t  field_0x2A5;
    /* 0x2A6 */ uint8_t  field_0x2A6;
    /* 0x2A7 */ uint8_t  field_0x2A7;
    /* 0x2A8 */ uint8_t  field_0x2A8;
    /* 0x2A9 */ uint8_t  field_0x2A9;
    /* 0x2AA */ uint8_t  field_0x2AA;
    /* 0x2AB */ uint8_t  field_0x2AB;
    /* 0x2AC */ uint32_t arTimer_0x2AC[3];
    /* 0x2B8 */ uint8_t  field_0x2B8[3];
    /* 0x2BB */ uint8_t  field_0x2BB;
    /* 0x2BC */ uint8_t  field_0x2BC;
    /* 0x2BD */ uint8_t  field_0x2BD;
    /* 0x2BE */ uint8_t  field_0x2BE;
    /* 0x2BF */ uint8_t  field_0x2BF;
    /* 0x2C0 */ uint8_t  field_0x2C0;
    /* 0x2C1 */ uint8_t  field_0x2C1;
    /* 0x2C2 */ uint8_t  field_0x2C2;
    /* 0x2C3 */ uint8_t  field_0x2C3;
    /* 0x2C4 */ uint8_t  field_0x2C4;
    /* 0x2C5 */ uint8_t  field_0x2C5;
    /* 0x2C6 */ uint8_t  field_0x2C6;
    /* 0x2C7 */ uint8_t  field_0x2C7;
    /* 0x2C8 */ uint8_t  field_0x2C8;
    /* 0x2C9 */ uint8_t  field_0x2C9;
    /* 0x2CA */ uint8_t  field_0x2CA;
    /* 0x2CB */ uint8_t  field_0x2CB;

    /* 0x2CC */ WheelsD  wddSprings;
    /* 0x2DC */ WheelsD  wddRideHeight;
    /* 0x2EC */ WheelsD  field_0x2EC;
    /* 0x2FC */ WheelsD  wddActualRideHeight;
    /* 0x30C */ WheelsD  field_0x30C;
    /* 0x31C */ WheelsD  field_0x31C;
    /* 0x32C */ WheelsD  wddSuspTravel;
    /* 0x33C */ WheelsD  field_0x33C;
    /* 0x34C */ WheelsD  field_0x34C;
    /* 0x35C */ WheelsD  adPackers;

    /* 0x36C */ uint32_t field_0x36C;
    /* 0x370 */ uint32_t field_0x370;
    /* 0x374 */ uint32_t field_0x374;
    /* 0x378 */ uint32_t field_0x378;
    /* 0x37C */ WheelsD  field_0x37C;
    /* 0x38C */ WheelsD  field_0x38C;
    /* 0x39C */ WheelsD  wddWheelSpeed;
    /* 0x3AC */ WheelsD  wddSlowRebound;
    /* 0x3BC */ WheelsD  wddFastRebound;
    /* 0x3CC */ WheelsD  wddSlowBump;
    /* 0x3DC */ WheelsD  wddFastBump;
    /* 0x3EC */ WheelsD  wddSlowRebound2;
    /* 0x3FC */ WheelsD  wddSlowBump2;

    /* 0x40C */ uint32_t antiRollbarRear;
    /* 0x410 */ uint32_t antiRollbarFront;
    /* 0x414 */ uint8_t  flags_0x414;
    /* 0x415 */ uint8_t  racePos;
    /* 0x416 */ uint16_t segDist_0x416;
    /* 0x418 */ uint16_t field_0x418;
    /* 0x41A */ uint8_t  controlMethodType;
    /* 0x41B */ uint8_t  field_0x41B;

} Car;

static_assert(sizeof(Car) == 0x41C, "Car struct size mismatch");

// Seg struct unchanged except for member name format
typedef struct Seg
{
    /* 0x000 */ uint16_t angleZ;
    /* 0x002 */ uint16_t angleXChase;
    /* 0x004 */ uint16_t posX;
    /* 0x006 */ uint16_t posZ;
    /* 0x008 */ uint16_t posY;
    /* 0x00A */ uint16_t angleA;
    /* 0x00C */ uint16_t rightSideX;
    /* 0x00E */ uint16_t rightSideY;
    /* 0x010 */ uint8_t  flags10;
    /* 0x011 */ uint8_t  extraSideX;
    /* 0x012 */ uint8_t  flags12;
    /* 0x013 */ uint8_t  extraSideY;
    /* 0x014 */ uint16_t angleZChangeMulHalfPI;
    /* 0x016 */ uint16_t ccLine;
    /* 0x018 */ uint16_t ccLineRAngle;
    /* 0x01A */ uint16_t nr;
    /* 0x01C */ uint8_t  ccLineDraw_0x1C;
    /* 0x01D */ uint8_t  ccLineDraw;
    /* 0x01E */ uint16_t objectID;
    /* 0x020 */ uint8_t  field_0x020;
    /* 0x021 */ uint8_t  finePosXY;
    /* 0x022 */ uint8_t  field_0x022;
    /* 0x023 */ uint8_t  field_0x023;
    /* 0x024 */ uint8_t  flags_0x024;
    /* 0x025 */ uint8_t  field_0x025;
    /* 0x026 */ uint16_t angleY;
    /* 0x028 */ uint8_t  field_0x028;
    /* 0x029 */ uint8_t  field_0x029;
    /* 0x02A */ uint8_t  field_0x02A;
    /* 0x02B */ uint8_t  swivelArmIndex;
    /* 0x02C */ uint32_t flagsTexture;
    /* 0x030 */ uint32_t flags30;
    /* 0x034 */ uint32_t flags34;
    /* 0x038 */ uint32_t flags38;
    /* 0x03C */ uint32_t flags3C;
    /* 0x040 */ uint32_t flags40;
    /* 0x044 */ uint32_t pSegPitlane;
    /* 0x048 */ uint32_t pitFlags;
    /* 0x04C */ uint16_t leftSideX;
    /* 0x04E */ uint16_t leftSideY;
    /* 0x050 */ uint8_t  field_0x050;
    /* 0x051 */ uint8_t  flags51;
    /* 0x052 */ uint16_t field_0x052;
    /* 0x054 */ uint16_t field_0x054;
    /* 0x056 */ uint16_t texture56;
    /* 0x058 */ uint8_t  field_0x058;
    /* 0x059 */ uint8_t  field_0x059;
    /* 0x05A */ uint8_t  field_0x05A;
    /* 0x05B */ uint8_t  field_0x05B;
    /* 0x05C */ uint8_t  field_0x05C;
    /* 0x05D */ uint8_t  field_0x05D;
    /* 0x05E */ uint8_t  field_0x05E;
    /* 0x05F */ uint8_t  field_0x05F;
    /* 0x060 */ uint16_t widthLeft;
    /* 0x062 */ uint16_t widthRight;
    /* 0x064 */ uint8_t  flagsSplit;
    /* 0x065 */ uint8_t  bumpiness;
    /* 0x066 */ uint8_t  field_0x066;
    /* 0x067 */ uint8_t  field_0x067;
    /* 0x068 */ uint8_t  speedLimitBlackFlagLeft;
    /* 0x069 */ uint8_t  speedLimitBlackFlagRight;
    /* 0x06A */ uint8_t  field_0x06A;
    /* 0x06B */ uint8_t  field_0x06B;
    /* 0x06C */ uint16_t angle_0x6C;
    /* 0x06E */ uint16_t angle_0x6E;
    /* 0x070 */ uint8_t  flags_0x70;
    /* 0x071 */ uint8_t  cclineSegCounter;
    /* 0x072 */ uint8_t  field_0x072;
    /* 0x073 */ uint8_t  field_0x073;
    /* 0x074 */ uint16_t field_0x074;
    /* 0x076 */ uint16_t field_0x076;
    /* 0x078 */ uint16_t sceneryStabilizingFlags;
    /* 0x07A */ uint16_t tyreMarkIndex;
    /* 0x07C */ uint8_t  pitSegIndexStart;
    /* 0x07D */ uint8_t  pitSegIndexEnd;
    /* 0x07E */ uint16_t field_0x07E;
    /* 0x080 */ uint16_t field_0x080;
    /* 0x082 */ uint8_t  field_0x082;
    /* 0x083 */ uint8_t  field_0x083;
    /* 0x084 */ uint8_t  field_0x084;
    /* 0x085 */ uint8_t  field_0x085;
    /* 0x086 */ uint8_t  field_0x086;
    /* 0x087 */ uint8_t  field_0x087;
    /* 0x088 */ uint16_t field_0x088;
    /* 0x08A */ uint16_t field_0x08A;
    /* 0x08C */ uint8_t  field_0x08C;
    /* 0x08D */ uint8_t  field_0x08D;
    /* 0x08E */ uint16_t field_0x08E;
    /* 0x090 */ uint16_t shadowLength;
    /* 0x092 */ uint8_t  pitlaneJoinFlags;
    /* 0x093 */ uint8_t  field_0x093;
    /* 0x094 */ uint16_t field_0x094;
    /* 0x096 */ uint16_t field_0x096;
    /* 0x098 */ uint16_t field_0x098;
    /* 0x09A */ uint16_t field_0x09A;
    /* 0x09C */ uint16_t field_0x09C;
    /* 0x09E */ uint8_t  field_0x09E;
    /* 0x09F */ uint8_t  flags_0x09F;
    /* 0x0A0 */ uint32_t field_0x0A0;
    /* 0x0A4 */ uint16_t field_0x0A4;
    /* 0x0A6 */ uint16_t field_0x0A6;
    /* 0x0A8 */ uint8_t  flags_0xA8;
    /* 0x0A9 */ uint8_t  field_0x0A9;
    /* 0x0AA */ uint8_t  kerbProfileLeft;
    /* 0x0AB */ uint8_t  kerbProfileRight;
    /* 0x0AC */ uint32_t shadowRel_0xAC;
    /* 0x0B0 */ uint16_t field_0x0B0;
    /* 0x0B2 */ uint16_t field_0x0B2;
    /* 0x0B4 */ uint8_t  field_0x0B4;
    /* 0x0B5 */ uint8_t  field_0x0B5;
    /* 0x0B6 */ uint8_t  field_0x0B6;
    /* 0x0B7 */ uint8_t  field_0x0B7;
    /* 0x0B8 */ uint16_t field_0x0B8;
    /* 0x0BA */ uint16_t field_0x0BA;
    /* 0x0BC */ uint16_t field_0x0BC;
    /* 0x0BE */ uint16_t field_0x0BE;
    /* 0x0C0 */ uint16_t field_0x0C0;
    /* 0x0C2 */ uint16_t field_0x0C2;
    /* 0x0C4 */ uint16_t field_0x0C4;
    /* 0x0C6 */ uint16_t field_0x0C6;
    /* 0x0C8 */ uint16_t field_0x0C8;
    /* 0x0CA */ uint16_t field_0x0CA;
    /* 0x0CC */ uint16_t angleDelta;
    /* 0x0CE */ uint16_t field_0x0CE;
    /* 0x0D0 */ uint8_t  field_0x0D0;
    /* 0x0D1 */ uint8_t  field_0x0D1;
    /* 0x0D2 */ uint16_t field_0x0D2;
    /* 0x0D4 */ uint16_t field_0x0D4;
    /* 0x0D6 */ uint16_t field_0x0D6;
    /* 0x0D8 */ uint16_t field_0x0D8;
    /* 0x0DA */ uint16_t field_0x0DA;
    /* 0x0DC */ uint16_t field_0x0DC;
    /* 0x0DE */ uint16_t field_0x0DE;
    /* 0x0E0 */ uint8_t  field_0x0E0;
    /* 0x0E1 */ uint8_t  field_0x0E1;
    /* 0x0E2 */ uint16_t field_0x0E2;
    /* 0x0E4 */ uint32_t angleZ;
    /* 0x0E8 */ uint32_t angleX;
    /* 0x0EC */ uint32_t angleZ;
    /* 0x0F0 */ uint32_t posX_tc0xf6;
    /* 0x0F4 */ uint32_t posZ_tc0xf6;
    /* 0x0F8 */ uint32_t posY_tc0xf6;
    /* 0x0FC */ uint16_t field_0x0FC;
    /* 0x0FE */ uint16_t field_0x0FE;
    /* 0x100 */ uint32_t field_0x100;
    /* 0x104 */ uint32_t field_0x104;
    /* 0x108 */ uint16_t field_0x108;
    /* 0x10A */ uint16_t field_0x10A;
    /* 0x10C */ uint32_t field_0x10C;
    /* 0x110 */ uint32_t field_0x110;
    /* 0x114 */ uint16_t field_0x114;
    /* 0x116 */ uint16_t field_0x116;
    /* 0x118 */ uint32_t field_0x118;
    /* 0x11C */ uint32_t field_0x11C;
    /* 0x120 */ uint32_t field_0x120;
    /* 0x124 */ uint32_t field_0x124;
    /* 0x128 */ uint16_t field_0x128;
    /* 0x12A */ uint16_t field_0x12A;
    /* 0x12C */ uint16_t field_0x12C;
    /* 0x12E */ uint16_t field_0x12E;
    /* 0x130 */ uint16_t field_0x130;
    /* 0x132 */ uint16_t field_0x132;
    /* 0x134 */ uint8_t  field_0x134;
    /* 0x135 */ uint8_t  field_0x135;
    /* 0x136 */ uint8_t  leftVergeUnk1;
    /* 0x137 */ uint8_t  rightVergeUnk1;
    /* 0x138 */ uint16_t field_0x138;
    /* 0x13A */ uint8_t  field_0x13A;
    /* 0x13B */ uint8_t  field_0x13B;
    /* 0x13C */ uint8_t  field_0x13C;
    /* 0x13D */ uint8_t  field_0x13D;
    /* 0x13E */ uint16_t field_0x13E;
    /* 0x140 */ uint8_t  field_0x140;
    /* 0x141 */ uint8_t  field_0x141;
    /* 0x142 */ uint8_t  field_0x142;
    /* 0x143 */ uint8_t  field_0x143;
    /* 0x144 */ uint16_t leftVergeWidth;
    /* 0x146 */ uint16_t leftVergeExternalHeight;
    /* 0x148 */ uint16_t leftVergeUnk2;
    /* 0x14A */ uint16_t rightVergeWidth;
    /* 0x14C */ uint16_t rightVergeExternalHeight;
    /* 0x14E */ uint16_t rightVergeUnk2;
    /* 0x150 */ uint32_t field_0x150;
    /* 0x154 */ uint8_t  field_0x154;
    /* 0x155 */ uint8_t  field_0x155;
    /* 0x156 */ uint8_t  field_0x156;
    /* 0x157 */ uint8_t  field_0x157;
    /* 0x158 */ uint8_t  field_0x158;
    /* 0x159 */ uint8_t  field_0x159;
    /* 0x15A */ uint8_t  field_0x15A;
    /* 0x15B */ uint8_t  field_0x15B;
    /* 0x15C */ uint8_t  field_0x15C;
    /* 0x15D */ uint8_t  field_0x15D;
    /* 0x15E */ uint8_t  field_0x15E;
    /* 0x15F */ uint8_t  field_0x15F;

} Seg;

static_assert(sizeof(Seg) == 0x160, "Seg struct size mismatch");

#pragma pack(pop)
