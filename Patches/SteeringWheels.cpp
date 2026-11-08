#include "SteeringWheels.h"
#include "General.h"
#include "../Utils/Helpers.h"
#include "../GP4MemLib/GP4MemLib.h"
#include "../IniLib/IniLib.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace std;
using namespace GP4MemLib;
using namespace IniLib;
using namespace GP4PP;

// Per-team or per-car steering wheels, for both the external wheels drawn on every car and the
// in-cockpit steering wheel mesh, and per-team or per-car cockpit dashes. Everything runs per
// car: the asset name decides whether teammates share a steering wheel or dash or not.
//
// Concept and reverse engineering from the ZaZ GP4 Tools proof of concept (Ronald Plukker).
//
// External wheels: GP4 draws all external steering wheels in one batch with the single
// "stwheel.tga" texture. A car whose CGP4Car + 0xd0 holds a texture index instead of -1 is
// drawn separately, with that texture bound to stage 1. Per-car textures are loaded right
// after GP4 loads the vanilla one, CGP4Car + 0xd0 is set for the duration of the external
// steering wheel renderer only, and just before each separate draw a texture that is ours is
// moved from stage 1 to stage 0, where the visible steering wheel texture lives.
//
// 3D external wheels (UseCockpitMesh): the cockpit steering wheel mesh is drawn on every car that
// would get a flat external wheel, with only its static objects and the car's rev lights, flag lights, gear, water temperature, fuel,
// position, lap, lap time, speed, driver status, warning and pit lights shown, and GP4's flat wheel is
// skipped for those cars. Each car uses the cockpit steering wheel it gets in the cockpit view,
// scaled by CockpitMeshScale. The meshes are shared with the cockpit, so the placement of the
// selected one is restored after the draw.
//
// Cockpit wheel and dash: the cockpit draws the meshes at CGP4Cockpit + 0xd8c (wheel) and
// CGP4Cockpit + 0xd88 (dash). Per-car meshes are loaded right after the vanilla cockpit meshes,
// one per distinct file, and before each cockpit update the dash and wheel of the viewed car are
// selected, together with the 294 cockpit object mappings (displays, lights, buttons) that point
// into them. The mappings depend on both meshes, so there is one per dash and wheel combination.
// Everything is restored and freed before the cockpit is destroyed.
//
// The cockpit is loaded before the cars are created, so the team and car of each car slot are
// calculated the same way D3D_LoadCars assigns them, rather than read from the cars.

namespace SteeringWheels
{
	// Settings of one steering wheel or dash asset, read from its own ini section
	struct AssetSettings
	{
		string section;
		string name;
		string perTeamFileName;
		string perDriverFileName;
		bool perTeam;
		bool perDriver;
		bool autoName;
		string fileName;
		bool enabled;
	};

	// Settings variables
	AssetSettings externalSettings = { "ExternalSteeringWheels", "External Steering Wheels", "stwheel_{teamname}.tga", "stwheel_{teamname}_car{car}.tga", false, false, true, "", false };
	AssetSettings cockpitSettings = { "CockpitSteeringWheels", "Cockpit Steering Wheels", "cockpits\\{teamname}\\cockpit_wheel.gp4", "cockpits\\{teamname}_car{car}\\cockpit_wheel.gp4", false, false, true, "", false };
	AssetSettings dashSettings = { "CockpitDashes", "Cockpit Dashes", "cockpits\\{teamname}\\cockpit_dash.gp4", "cockpits\\{teamname}_car{car}\\cockpit_dash.gp4", false, false, true, "", false };
	bool useCockpitMesh = false;
	const float DEFAULT_COCKPIT_MESH_SCALE = 0.7f;
	float cockpitMeshScale = DEFAULT_COCKPIT_MESH_SCALE;

	// Constants
	const int NUM_COCKPIT_OBJECTS = 0x126;
	const int VANILLA = -1;

	// Car and team counts, read from GP4's code by readCarCounts()
	int numCars = 0;
	int numTeams = 0;

	// Compare instruction closing the car creation loop of D3D_LoadCars: cmp ebx, <number of cars>
	DWORD carLoopCompare = 0x0048d80f;
	BYTE carLoopCompareBytes[] = { 0x83, 0xfb };

	// Clamp of GetCarDynamicData to the last car physics data entry: cmp eax, <number of entries - 1>
	DWORD dynamicDataClamp = 0x00408396;
	BYTE dynamicDataClampBytes[] = { 0x83, 0xf8 };

	// Target Addresses (call instructions whose target gets replaced)
	DWORD initCarSteeringWheelsCall = 0x0048d81e;
	DWORD renderCarSteeringWheelsCall = 0x0049b94a;
	DWORD individualWheelFlushCall = 0x0048ca3d;
	DWORD cockpitLoadMeshCall = 0x00495d51;
	DWORD cockpitUpdateCall = 0x0049d346;
	DWORD cockpitDestructorCall = 0x00498074;

	// Original bytes of the call instructions above, checked before patching
	BYTE initCarSteeringWheelsBytes[] = { 0xe8, 0x9d, 0xda, 0xff, 0xff };
	BYTE renderCarSteeringWheelsBytes[] = { 0xe8, 0x41, 0xfa, 0xfe, 0xff };
	BYTE individualWheelFlushBytes[] = { 0xe8, 0xfe, 0x09, 0xfc, 0xff };
	BYTE cockpitLoadMeshBytes[] = { 0xe8, 0xfa, 0x9a, 0xfd, 0xff };
	BYTE cockpitUpdateBytes[] = { 0xe8, 0xc5, 0x35, 0xfd, 0xff };
	BYTE cockpitDestructorBytes[] = { 0xe8, 0xc7, 0x74, 0xfd, 0xff };

	// DrawCarSteeringWheels instructions replaced by a jump, for the gloves and arms of the 3D external wheels
	DWORD wheelIndexBlockAdd = 0x0048b4d3;		// add ecx, <offset of the last car's indices>, ecx = locked index buffer
	DWORD wheelVertexUnlock = 0x0048c32a;		// mov eax, [esi] - push esi - call [eax + 0x30], vertex buffer unlock
	BYTE wheelIndexBlockAddBytes[] = { 0x81, 0xc1 };
	BYTE wheelVertexUnlockBytes[] = { 0x8b, 0x06, 0x56, 0xff, 0x50, 0x30 };

	// Jump Back Addresses
	DWORD wheelIndexBlockAddJumpBack = 0x0048b4d9;
	DWORD wheelVertexUnlockJumpBack = 0x0048c330;

	// Vars and Data Addresses
	DWORD arrCars = 0x00a51fd8;
	DWORD arrTeamNames = 0x00643ba8;
	DWORD quickLapsMode = 0x007c2e75;			// byte, non-zero in quick laps
	DWORD quickLapsCarTeams = 0x007c2b24;		// byte per car slot, team chosen in quick laps
	DWORD currentView = 0x00645950;				// 1-based
	DWORD viewCameraCar = 0x00a5110c;			// car index per view, indexed by currentView - 1
	DWORD carsFolder = 0x00644ee4;				// "CARS\"
	DWORD stWheelTextureBasePath = 0x00644ef8;	// ".\cars\"
	DWORD stWheelTextureIndex = 0x00644bbc;
	DWORD stWheelVertexBuffer = 0x00a4d698;
	DWORD cockpitObjectNames = 0x006412f8;
	DWORD cockpitWheelShaderPush = 0x0046faf2;	// push <shader id>, patched by StWheelAdvancedCarShader
	DWORD cockpitDashShaderPush = 0x0046f9bc;	// push <shader id>
	DWORD cockpitPointer = 0x00a4fcc8;			// CGP4Cockpit*
	DWORD lodManagerPointer = 0x00a4f55c;
	DWORD stWheelTeamOffsets = 0x006449f0;		// y, z per team
	DWORD stWheelY = 0x00644bf8;
	DWORD stWheelZ = 0x00644bf4;
	DWORD stWheelMaxDistance = 0x005f09a8;		// 80.0
	DWORD cockpitWheelTilt = 0x006418b8;
	DWORD carDynamicDataArray = 0x0066fef0;		// car physics data, one entry per car, not in car order
	DWORD raceClock = 0x0066d814;				// g_raceClockMs
	DWORD sessionType = 0x007c2ea9;				// byte, SESSION_RACE in a race
	DWORD metricUnits = 0x007ad8ac;				// word, negative = km/h
	DWORD playerDynamicData = 0x0066d820;		// car physics data of the player, its fuel is the only one shown in a race
	DWORD driverStatusData = 0x0066d828;		// car physics data that also gets the driver status light when driverStatusEnabled is set
	DWORD driverStatusEnabled = 0x007aafd4;		// byte
	DWORD bestLapOrder = 0x00675958;			// car ids by best lap, position outside a race
	DWORD numBestLapOrder = 0x0066d8ec;			// dword
	DWORD stWheelCarsDrawn = 0x00a4d6a0;		// dword, cars written to the external wheel buffers this frame

	// Function Addresses
	DWORD InitCarSteeringWheels = 0x0048b2c0;
	DWORD RenderCarSteeringWheels = 0x0048b390;
	DWORD DeviceFlushState = 0x0044d440;
	DWORD CockpitLoadMesh = 0x0046f850;
	DWORD CockpitUpdate = 0x00470910;
	DWORD CockpitDestructor = 0x0046f540;
	DWORD GetTextureIndex = 0x00450710;
	DWORD BackupTextureBasePath = 0x0044f5c0;
	DWORD SetTextureBasePath = 0x0044f4f0;
	DWORD RestoreTextureBasePath = 0x0044f5e0;
	DWORD AllocateMesh = 0x00450cb0;
	DWORD RemoveMesh = 0x00450e90;
	DWORD LoadGP4MeshFile = 0x004569d0;
	DWORD PrepareMesh = 0x00457810;
	DWORD GetObjectIndexByName = 0x00457200;
	DWORD SetObjectVisible = 0x00457660;
	DWORD ApplyShaderToObject = 0x00457260;
	DWORD GetShader = 0x0045ee70;
	DWORD AssetFileExists = 0x0046af40;
	DWORD MeshSetTransform = 0x004578c0;
	DWORD MeshUpdateTransforms = 0x00457e70;
	DWORD CockpitSetMeshTransforms = 0x0046fdc0;
	DWORD DeviceGetRenderState = 0x0044d3b0;
	DWORD DeviceQueueRenderState = 0x0044d290;

	// Function Pointer Types
	typedef void(__cdecl* InitCarSteeringWheelsType)(BYTE*);
	typedef void(__cdecl* RenderCarSteeringWheelsType)(BYTE*, DWORD);
	typedef void(__thiscall* DeviceFlushStateType)(BYTE*);
	typedef void(__thiscall* CockpitLoadMeshType)(BYTE*, BYTE*);
	typedef void(__thiscall* CockpitUpdateType)(BYTE*, DWORD, int);
	typedef void(__thiscall* CockpitDestructorType)(BYTE*);
	typedef int(__thiscall* GetTextureIndexType)(BYTE*, const char*, int, int, int, int, int, int);
	typedef void(__thiscall* BackupTextureBasePathType)(BYTE*);
	typedef void(__thiscall* SetTextureBasePathType)(BYTE*, const char*);
	typedef void(__thiscall* RestoreTextureBasePathType)(BYTE*);
	typedef DWORD(__thiscall* AllocateMeshType)(BYTE*, int, int);
	typedef void(__thiscall* RemoveMeshType)(BYTE*, DWORD);
	typedef bool(__thiscall* LoadGP4MeshFileType)(BYTE*, const char*, int);
	typedef void(__thiscall* PrepareMeshType)(BYTE*, int, int);
	typedef DWORD(__thiscall* GetObjectIndexByNameType)(BYTE*, const char*);
	typedef void(__thiscall* SetObjectVisibleType)(BYTE*, int, int);
	typedef void(__thiscall* ApplyShaderToObjectType)(BYTE*, BYTE*, int, int);
	typedef BYTE* (__cdecl* GetShaderType)(int);
	typedef int(__stdcall* AssetFileExistsType)(char*, int);
	typedef void(__thiscall* MeshSetTransformType)(BYTE*, float*, int);
	typedef void(__thiscall* MeshUpdateTransformsType)(BYTE*, int, char);
	typedef void(__thiscall* CockpitSetMeshTransformsType)(BYTE*, BYTE*, BYTE*);
	typedef void(__thiscall* MeshDrawType)(BYTE*, int, void*, char);
	typedef long(__stdcall* D3DSetTransformType)(void*, DWORD, float*);
	typedef int(__thiscall* DeviceGetRenderStateType)(BYTE*, DWORD, DWORD*);
	typedef int(__thiscall* DeviceQueueRenderStateType)(BYTE*, DWORD, DWORD);

	// Function Pointers
	InitCarSteeringWheelsType InitCarSteeringWheelsFunc = (InitCarSteeringWheelsType)InitCarSteeringWheels;
	RenderCarSteeringWheelsType RenderCarSteeringWheelsFunc = (RenderCarSteeringWheelsType)RenderCarSteeringWheels;
	DeviceFlushStateType DeviceFlushStateFunc = (DeviceFlushStateType)DeviceFlushState;
	CockpitLoadMeshType CockpitLoadMeshFunc = (CockpitLoadMeshType)CockpitLoadMesh;
	CockpitUpdateType CockpitUpdateFunc = (CockpitUpdateType)CockpitUpdate;
	CockpitDestructorType CockpitDestructorFunc = (CockpitDestructorType)CockpitDestructor;
	GetTextureIndexType GetTextureIndexFunc = (GetTextureIndexType)GetTextureIndex;
	BackupTextureBasePathType BackupTextureBasePathFunc = (BackupTextureBasePathType)BackupTextureBasePath;
	SetTextureBasePathType SetTextureBasePathFunc = (SetTextureBasePathType)SetTextureBasePath;
	RestoreTextureBasePathType RestoreTextureBasePathFunc = (RestoreTextureBasePathType)RestoreTextureBasePath;
	AllocateMeshType AllocateMeshFunc = (AllocateMeshType)AllocateMesh;
	RemoveMeshType RemoveMeshFunc = (RemoveMeshType)RemoveMesh;
	LoadGP4MeshFileType LoadGP4MeshFileFunc = (LoadGP4MeshFileType)LoadGP4MeshFile;
	PrepareMeshType PrepareMeshFunc = (PrepareMeshType)PrepareMesh;
	GetObjectIndexByNameType GetObjectIndexByNameFunc = (GetObjectIndexByNameType)GetObjectIndexByName;
	SetObjectVisibleType SetObjectVisibleFunc = (SetObjectVisibleType)SetObjectVisible;
	ApplyShaderToObjectType ApplyShaderToObjectFunc = (ApplyShaderToObjectType)ApplyShaderToObject;
	GetShaderType GetShaderFunc = (GetShaderType)GetShader;
	AssetFileExistsType AssetFileExistsFunc = (AssetFileExistsType)AssetFileExists;
	MeshSetTransformType MeshSetTransformFunc = (MeshSetTransformType)MeshSetTransform;
	MeshUpdateTransformsType MeshUpdateTransformsFunc = (MeshUpdateTransformsType)MeshUpdateTransforms;
	CockpitSetMeshTransformsType CockpitSetMeshTransformsFunc = (CockpitSetMeshTransformsType)CockpitSetMeshTransforms;
	DeviceGetRenderStateType DeviceGetRenderStateFunc = (DeviceGetRenderStateType)DeviceGetRenderState;
	DeviceQueueRenderStateType DeviceQueueRenderStateFunc = (DeviceQueueRenderStateType)DeviceQueueRenderState;

	// Structure offsets
	const DWORD CAR_TEAM_INDEX = 0x20;
	const DWORD CAR_TEAM_CAR = 0x24;			// 0 or 1, first or second car of the team
	const DWORD CAR_WHEEL_TEXTURE = 0xd0;
	const DWORD CAR_MATRIX = 0x334;
	const DWORD CAR_WHEEL_VISIBLE = 0x814;		// byte, external steering wheel drawn when non-zero
	const DWORD CAR_STEERING_ANGLE = 0x4a28;
	const DWORD CAR_REV_LIGHTS = 0x4a2c;		// byte, 1 + number of rev lights lit

	const DWORD DYNAMIC_DATA_SIZE = 0x41c;
	const DWORD DYNAMIC_GEAR = 0x72;			// signed byte, -1 = reverse, 0 = neutral
	const DWORD DYNAMIC_ID = 0x7c;				// byte, car index + 1 in the low 6 bits
	const DWORD DYNAMIC_SPLIT_TIMES = 0x20;		// dword ms each: split 1, split 2, lap
	const DWORD DYNAMIC_LAST_SPLIT = 0x74;		// byte, index into DYNAMIC_SPLIT_TIMES of the last timing line crossed
	const DWORD DYNAMIC_LED_FLAGS = 0x82;		// LCD message queue bits (UpdateCarLcdMessageQueue)
	const DWORD DYNAMIC_RACE_FLAGS = 0x83;
	const DWORD DYNAMIC_PIT_FLAGS = 0x8d;
	const DWORD DYNAMIC_FLAGS_122 = 0x122;
	const DWORD DYNAMIC_FUEL_LAPS = 0x60;		// word, laps of fuel in 1/64 laps
	const DWORD DYNAMIC_CURRENT_LAP = 0x70;		// byte
	const DWORD DYNAMIC_STATE = 0x7a;			// byte, STATE_NOT_ACTIVE
	const DWORD DYNAMIC_DAMAGE = 0x80;			// word, one bit per warning light
	const DWORD DYNAMIC_LCD_FLAGS_8B = 0x8b;
	const DWORD DYNAMIC_SPEED = 0x98;			// signed dword
	const DWORD DYNAMIC_LAP_START = 0xc0;		// dword, race clock at the start of the lap, LAP_NOT_STARTED when none
	const DWORD DYNAMIC_PIT_LIGHT_FLAGS = 0xfe;
	const DWORD DYNAMIC_MENU_FUEL_LAPS = 0x224;	// word, shown instead of DYNAMIC_FUEL_LAPS with LCD_FLAGS_8B_MENU_FUEL
	const DWORD DYNAMIC_ENGINE_TEMPERATURE = 0x244;	// dword, degrees in the high word
	const DWORD DYNAMIC_RACE_POSITION = 0x415;	// byte, (position - 1) * 2
	const BYTE STATE_NOT_ACTIVE = 0x80;
	const BYTE LCD_FLAGS_8B_MENU_FUEL = 0x10;
	const BYTE PIT_LIGHT = 0x04;
	const BYTE SESSION_RACE = 0x80;
	const DWORD SPEED_FACTOR_MPH = 0x2ba;
	const DWORD SPEED_FACTOR_KMH = 0x464;
	const DWORD MAX_ENGINE_TEMPERATURE = 150;
	const DWORD MAX_FUEL_LAPS = 99;
	const DWORD LAP_NOT_STARTED = 0xc0000000;
	const DWORD MAX_SPLIT_TIME = 600000;		// ms, "------" from 10 minutes
	const int MAX_LAP_CLOCK = 598999;			// ms, the running clock stops at 9:58: LcdWriteCurrentLapTime only writes below 9:59
	const BYTE NUM_SPLIT_TIMES = 3;
	const BYTE LED_FLAGS_TIME = 0x08;			// lap or split time message pending or on screen: the time is held
	const BYTE LED_FLAGS_WARNING = 0x20;		// warning message (the CarLedsFlags bitfield names are off from bit 2 on)
	const BYTE RACE_FLAGS_YELLOW = 0x10;
	const BYTE PIT_FLAGS_FAILURE = 0x10;
	const BYTE FLAGS_122_BLUE = 0x02;
	const DWORD FLAG_FLASH_PERIOD = 0x7d;		// game clock ticks

	const DWORD LOD_NUM_ENTRIES = 0x3c48;
	const DWORD LOD_ENTRIES = 0x3c4c;
	const DWORD LOD_ENTRY_SIZE = 0x78;
	const DWORD LOD_ENTRY_CAR = 0x00;
	const DWORD LOD_ENTRY_DISTANCE = 0x04;
	const DWORD LOD_ENTRY_VISIBLE = 0x74;

	// Flat external wheel indices per car: 7 quads of 6 indices, wheel front, back, rev lights, gloves, arms
	const int WHEEL_BLOCK_INDICES = 42;
	const int WHEEL_ONLY_INDICES = 18;			// wheel front, back and rev lights, the gloves and arms follow

	const DWORD DEVICE_MANAGER = 0x04;
	const DWORD DEVICE_D3D_DEVICE = 0x08;
	const DWORD DEVICE_WORLD_MATRIX = 0x2018;
	const DWORD DEVICE_CURRENT_SHADER = 0x24a0;
	const DWORD D3D_SET_TRANSFORM = 0x94;		// IDirect3DDevice8::SetTransform vtable offset
	const DWORD D3DTS_WORLD = 0x100;
	const DWORD D3DRS_NORMALIZENORMALS = 0x8f;
	const DWORD DEVICE_STAGE0_TEXTURE = 0x1f7c;
	const DWORD DEVICE_STAGE1_TEXTURE = 0x1f80;
	const DWORD DEVICE_STAGE0_DIRTY = 0x1f9c;
	const DWORD DEVICE_STAGE1_DIRTY = 0x1f9d;
	const DWORD DEVICE_TEXTURES_DIRTY = 0x1fa4;

	const DWORD MANAGER_NUM_TEXTURES = 0x08;
	const DWORD MANAGER_TEXTURE_RESIZE = 0x18;
	const DWORD MANAGER_TEXTURES = 0x24;
	const DWORD MANAGER_MESHES = 0x2c;
	const DWORD MANAGER_NUM_MIPMAPS = 0x8ec;
	const DWORD TEXTURE_SIZE = 0x4c;
	const DWORD TEXTURE_D3D_TEXTURE = 0x38;

	const DWORD COCKPIT_DEVICE = 0x4c;
	const DWORD COCKPIT_MESH_SELECTORS = 0x98;
	const DWORD COCKPIT_OBJECT_INDICES = 0x530;
	const DWORD COCKPIT_DASH_MESH = 0xd88;
	const DWORD COCKPIT_WHEEL_MESH = 0xd8c;
	const DWORD COCKPIT_WORLD_MATRIX = 0xcbc;
	const DWORD COCKPIT_ROTATION_MATRIX = 0xcfc;

	// Cockpit object slots shown when nothing is lit: steering_wheel, LED01_off to LED05_off, Warning_off,
	// and the Water_T_on, Fuel_F_on and Lap_L_on labels that CGP4Cockpit::ResetDisplayLabels turns on for good.
	// The other *_off objects (aids, rev lights, flags, pit, opposition spread) are never shown by the cockpit
	const int COCKPIT_STATIC_OBJECTS[] = { 0x000, 0x107, 0x109, 0x10b, 0x10d, 0x10f, 0x113, 0x001, 0x05c, 0x08f };

	// Per-car displays, slots as CGP4Cockpit::UpdateDisplayObjects and its setters use them. Digit n of a
	// display is its _0 slot + n
	const int COCKPIT_WATER_A_1 = 0x03;
	const int COCKPIT_WATER_B_0 = 0x04;
	const int COCKPIT_WATER_C_0 = 0x0e;
	const int COCKPIT_FUEL_POINT = 0x5e;
	const int COCKPIT_FUEL_F_MINUS = 0x5f;
	const int COCKPIT_FUEL_A_0 = 0x60;
	const int COCKPIT_FUEL_A_MINUS = 0x6a;
	const int COCKPIT_FUEL_B_0 = 0x6b;
	const int COCKPIT_FUEL_B_MINUS = 0x75;
	const int COCKPIT_FUEL_C_0 = 0x76;
	const int COCKPIT_FUEL_C_MINUS = 0x80;
	const int COCKPIT_POSITION_P = 0x81;
	const int COCKPIT_POSITION_A_0 = 0x82;		// 0 to 2 only
	const int COCKPIT_POSITION_B_0 = 0x85;
	const int COCKPIT_LAP_A_0 = 0x92;
	const int COCKPIT_LAP_B_0 = 0x9c;
	const int COCKPIT_SPEED_A_0 = 0xe2;			// 0 to 3 only
	const int COCKPIT_SPEED_B_0 = 0xe6;
	const int COCKPIT_SPEED_C_0 = 0xf0;
	const int COCKPIT_DRIVER_STATUS_OFF = 0x110;
	const int COCKPIT_DRIVER_STATUS_RED = 0x111;
	const int COCKPIT_PIT_1 = 0x121;			// own pit light
	const int COCKPIT_PIT_2 = 0x122;			// team-mate's pit light, in a race
	const int COCKPIT_LAP_TIME_COLON = 0x18;
	const int COCKPIT_LAP_TIME_POINT = 0x19;
	const int COCKPIT_LAP_TIME_A_0 = 0x1a;		// A to F: _0 to _9 then _minus, 11 slots each
	const int COCKPIT_LAP_TIME_DIGIT_SLOTS = 11;
	const int COCKPIT_LAP_TIME_MINUS = 10;		// slot of _minus within a digit
	const int NUM_LAP_TIME_DIGITS = 6;			// M:SS.mmm

	// Warning light of each DYNAMIC_DAMAGE bit, as CGP4Cockpit::SetWarningLights, -1 = none
	const int COCKPIT_WARNINGS[16] = { 0x11b, 0x11a, 0x115, 0x114, 0x116, 0x117, 0x11f, 0x11e, -1, -1, -1, -1, -1, -1, 0x11d, 0x11c };

	// Slots driven per car, by display, first and last
	struct DisplayGroup
	{
		const char* name;
		int first;
		int last;
	};

	const DisplayGroup COCKPIT_DISPLAY_GROUPS[] = {
		{ "water", 0x003, 0x017 },
		{ "lap time", 0x018, 0x05b },
		{ "fuel", 0x05e, 0x080 },
		{ "position", 0x081, 0x08e },
		{ "lap", 0x092, 0x0a5 },
		{ "speed", 0x0e2, 0x0f9 },
		{ "driver status", 0x110, 0x111 },
		{ "warning", 0x114, 0x11f },
		{ "pit", 0x121, 0x122 },
	};

	// Cockpit object slots of the rev lights: REVn_on at REV1_ON + 2 * (n - 1)
	const int NUM_REV_LIGHTS = 5;
	const int COCKPIT_LEFT_REV1_ON = 0xb9;
	const int COCKPIT_RIGHT_REV1_ON = 0xc3;

	// Flag lights: flagn_on at FLAG1_ON + 2 * (n - 1)
	const int NUM_FLAG_LIGHTS = 3;
	const int COCKPIT_LEFT_FLAG1_ON = 0xcd;
	const int COCKPIT_RIGHT_FLAG1_ON = 0xd3;

	// Gear_R, Gear_N, Gear_1 to Gear_7
	const int NUM_GEARS = 9;
	const int COCKPIT_GEAR_R = 0xd9;

	const BYTE COCKPIT_LIGHT_MAP_SHADER = 0x13;
	const DWORD SHADER_LIGHT_MAP = 0x60;
	const DWORD MESH_DRAW = 0x10;				// CGP4Mesh vtable offset

	const DWORD MESH_NUM_OBJECTS = 0x1b0;
	const DWORD MESH_OBJECTS = 0x1b4;
	const DWORD MESH_PREPARED = 0x1f8;
	const DWORD MESH_PARENT_OBJECT = 0x1fc;
	const DWORD MESH_OBJECT_SIZE = 0x34;
	const DWORD MESH_OBJECT_DATA = 0x08;
	const DWORD MESH_OBJECT_INDEX = 0x0c;
	const DWORD OBJECT_DATA_FLAGS = 0x00;
	const DWORD OBJECT_DATA_TRANSFORMS = 0x08;
	const DWORD OBJECT_FLAGS_SIZE = 0x14;
	const DWORD OBJECT_TRANSFORM_SIZE = 0x94;
	const DWORD OBJECT_TRANSFORM_MATRIX = 0x4c;
	const DWORD OBJECT_TRANSFORM_PARENT = 0x8c;
	const DWORD MAX_MESH_OBJECTS = 0x10000;

	// Cockpit object mapping: which mesh (0 = dash, 1 = wheel) and which object index
	struct CockpitMapping
	{
		int selectors[NUM_COCKPIT_OBJECTS];
		int indices[NUM_COCKPIT_OBJECTS];
	};

	// One cockpit steering wheel or dash file, shared by every car whose asset name resolves to it
	struct CockpitMesh
	{
		bool loaded;	// false when the file was not found or failed to load
		BYTE* mesh;
		DWORD meshIndex;
		string fileName;
	};

	// Dash and steering wheel combination used by at least one car, with the object mappings into both
	struct CockpitSet
	{
		int dash;		// index into cockpitDashMeshes, or VANILLA
		int wheel;		// index into cockpitWheelMeshes, or VANILLA
		CockpitMapping mapping;
	};

	// Team and car of a car slot
	struct CarSlot
	{
		int team;
		int car;		// 0 or 1
	};

	// Objects of a cockpit steering wheel mesh shown on the external wheels, -1 when the mesh does not have them
	struct ExternalWheelObjects
	{
		vector<int> statics;
		int revOn[2][NUM_REV_LIGHTS];		// left, right
		int flagOn[2][NUM_FLAG_LIGHTS];		// left, right
		int gears[NUM_GEARS];
		vector<pair<int, int>> displays;		// cockpit slot and mesh object of each per-car display object found
	};

	// Row-major 4x4 matrix, same layout as D3DMATRIX
	struct Matrix
	{
		float m[4][4];
	};

	// External wheels storage variables
	bool externalPatched = false;
	vector<int> carTextureIndex;
	bool insideWheelRender = false;
	bool firstDrawLogged = false;
	vector<bool> promotionLogged;
	vector<BYTE*> changedCars;
	vector<int> oldTextures;

	// 3D external wheels storage variables
	vector<BYTE*> meshCars;				// cars drawn with the cockpit mesh, flat wheel hidden for the original call
	vector<BYTE> oldWheelVisible;
	int numFrameMeshCars = 0;			// meshCars entries of the current external draw
	int frameSkipCar = -1;				// car not drawn by the current external draw, -1 = none
	bool glovesPatched = false;			// GP4 keeps drawing the gloves and arms of the 3D wheel cars
	int numWheelBlocks = 0;				// cars that fit in the flat wheel buffers, read from GP4's code by readWheelBlockCount()
	DWORD wheelIndexBlockOffset = 0;	// bytes from the start of the index buffer to the last car's indices
	WORD* wheelIndexBuffer = NULL;		// locked index buffer of the current external draw
	vector<int> hiddenWheelBlocks;		// blocks of the 3D wheel cars in the current external draw
	bool wheelBlocksLogged = false;
	map<BYTE*, ExternalWheelObjects> externalWheelObjects;	// per cockpit wheel mesh
	vector<BYTE*> drawnMeshes;			// meshes whose objects were changed by the current external draw
	int numDynamicData = 0;				// read from GP4's code by readDynamicDataCount(), 0 = no gears and flags
	vector<BYTE*> carDynamicData;		// car physics data per car index, NULL when not found
	bool firstMeshDrawLogged = false;

	// Cockpit wheels and dashes storage variables
	bool cockpitPatched = false;
	BYTE* cockpitObject = NULL;
	BYTE* vanillaDashMesh = NULL;
	BYTE* vanillaWheelMesh = NULL;
	CockpitMapping vanillaMapping;
	vector<CockpitMesh> cockpitDashMeshes;
	vector<CockpitMesh> cockpitWheelMeshes;
	vector<CockpitSet> cockpitSets;
	vector<int> carCockpitSet;			// index into cockpitSets per car, or VANILLA
	int selectedSet = VANILLA;
	vector<bool> selectionLogged;

	template <typename T>
	T& field(BYTE* base, DWORD offset)
	{
		return *reinterpret_cast<T*>(base + offset);
	}

	BYTE* carAt(int carIndex)
	{
		return MemUtils::addressToValue<BYTE*>(arrCars + carIndex * sizeof(DWORD));
	}

	string teamName(int team)
	{
		return string(MemUtils::addressToPtr<char>(arrTeamNames + team * 0x20));
	}

	CarSlot carSlot(int carIndex)
	{
		// Same assignment as D3D_LoadCars
		int slot = carIndex;

		if (MemUtils::addressToValue<BYTE>(quickLapsMode) != 0)
		{
			slot = (carIndex & 1) + MemUtils::addressToValue<BYTE>(quickLapsCarTeams + carIndex) * 2;
		}

		CarSlot carSlot = { slot / 2, slot & 1 };

		return carSlot;
	}

	bool validTeam(const CarSlot& slot)
	{
		return slot.team >= 0 && slot.team < numTeams;
	}

	string carName(int carIndex)
	{
		CarSlot slot = carSlot(carIndex);

		if (!validTeam(slot))
		{
			return "car index " + to_string(carIndex + 1);
		}

		return teamName(slot.team) + " car " + to_string(slot.car + 1);
	}

	string carFileName(const string& pattern, int carIndex)
	{
		CarSlot slot = carSlot(carIndex);

		map<string, string> variables;
		variables["teamname"] = teamName(slot.team);
		variables["team"] = to_string(slot.team + 1);
		variables["car"] = to_string(slot.car + 1);
		variables["driverindex"] = to_string(carIndex + 1);

		return replaceVariables(pattern, variables);
	}

	bool assetFileExists(const string& fileName)
	{
		char fileNameBuffer[MAX_PATH];

		strcpy_s(fileNameBuffer, fileName.c_str());

		// GP4 returns 0 when the asset is found, both in cars.wad and on disk
		return AssetFileExistsFunc(fileNameBuffer, 0) == 0;
	}

	string carsFolderPath(const string& fileName)
	{
		return string(MemUtils::addressToPtr<char>(carsFolder)) + fileName;
	}

	string texFileName(string fileName)
	{
		// Same rule as GP4's texture loader (GetTextureIndex): everything from the last '.'
		// is replaced by .tex, or .tex is appended when there is no '.'
		size_t extensionPos = fileName.rfind('.');

		if (extensionPos != string::npos)
		{
			fileName.erase(extensionPos);
		}

		return fileName + ".tex";
	}

	bool checkCallBytes(DWORD callAddress, const BYTE expected[5], const string& name)
	{
		if (memcmp(MemUtils::addressToPtr<BYTE>(callAddress), expected, 5) == 0)
		{
			return true;
		}

		OutputGP4PPDebugString("Steering Wheels: unexpected bytes at " + MemUtils::dwordToString(callAddress) + " (" + name + "), patch not applied");

		return false;
	}

	bool readCarCounts()
	{
		BYTE* compare = MemUtils::addressToPtr<BYTE>(carLoopCompare);

		if (memcmp(compare, carLoopCompareBytes, sizeof(carLoopCompareBytes)) != 0)
		{
			OutputGP4PPDebugString("Steering Wheels: unexpected bytes at " + MemUtils::dwordToString(carLoopCompare) + " (D3D_LoadCars car loop), patch not applied");
			return false;
		}

		// The operand is a sign-extended byte
		int count = static_cast<signed char>(compare[sizeof(carLoopCompareBytes)]);

		if (count <= 0 || count % 2 != 0)
		{
			OutputGP4PPDebugString("Steering Wheels: unexpected car count " + to_string(count) + ", patch not applied");
			return false;
		}

		numCars = count;
		numTeams = numCars / 2;

		carTextureIndex.assign(numCars, -1);
		promotionLogged.assign(numCars, false);
		changedCars.resize(numCars);
		oldTextures.resize(numCars);
		meshCars.resize(numCars);
		oldWheelVisible.resize(numCars);
		carCockpitSet.assign(numCars, VANILLA);
		selectionLogged.assign(numCars, false);

		OutputGP4PPDebugString("Steering Wheels: " + to_string(numCars) + " cars, " + to_string(numTeams) + " teams");

		return true;
	}

	bool readDynamicDataCount()
	{
		BYTE* clamp = MemUtils::addressToPtr<BYTE>(dynamicDataClamp);

		if (memcmp(clamp, dynamicDataClampBytes, sizeof(dynamicDataClampBytes)) != 0)
		{
			OutputGP4PPDebugString("Steering Wheels: unexpected bytes at " + MemUtils::dwordToString(dynamicDataClamp) + " (GetCarDynamicData), no gears and flags on the 3D external steering wheels");
			return false;
		}

		// The operand is a sign-extended byte, the index of the last entry
		int count = static_cast<signed char>(clamp[sizeof(dynamicDataClampBytes)]) + 1;

		if (count < numCars)
		{
			OutputGP4PPDebugString("Steering Wheels: " + to_string(count) + " car physics data entries for " + to_string(numCars) + " cars, no gears and flags on the 3D external steering wheels");
			return false;
		}

		numDynamicData = count;
		carDynamicData.assign(numCars, NULL);

		OutputGP4PPDebugString("Steering Wheels: " + to_string(numDynamicData) + " car physics data entries");

		return true;
	}

	bool readWheelBlockCount()
	{
		BYTE* add = MemUtils::addressToPtr<BYTE>(wheelIndexBlockAdd);

		if (memcmp(add, wheelIndexBlockAddBytes, sizeof(wheelIndexBlockAddBytes)) != 0
			|| memcmp(MemUtils::addressToPtr<BYTE>(wheelVertexUnlock), wheelVertexUnlockBytes, sizeof(wheelVertexUnlockBytes)) != 0)
		{
			OutputGP4PPDebugString("Steering Wheels: unexpected bytes in DrawCarSteeringWheels, no gloves and arms on the 3D external steering wheels");
			return false;
		}

		// The operand is the byte offset of the last car's indices, the cars without a light map fill from the start
		DWORD offset = *reinterpret_cast<DWORD*>(add + sizeof(wheelIndexBlockAddBytes));
		DWORD blockSize = WHEEL_BLOCK_INDICES * sizeof(WORD);

		if (offset == 0 || offset % blockSize != 0 || offset / blockSize + 1 < (DWORD)numCars)
		{
			OutputGP4PPDebugString("Steering Wheels: unexpected index buffer size " + MemUtils::dwordToString(offset) + " in DrawCarSteeringWheels, no gloves and arms on the 3D external steering wheels");
			return false;
		}

		wheelIndexBlockOffset = offset;
		numWheelBlocks = offset / blockSize + 1;
		hiddenWheelBlocks.resize(numWheelBlocks);

		OutputGP4PPDebugString("Steering Wheels: " + to_string(numWheelBlocks) + " cars in the external wheel buffers");

		return true;
	}

	//
	// External steering wheels
	//

	DWORD textureD3DPointer(BYTE* manager, int textureIndex)
	{
		if (manager == NULL || textureIndex < 0 || (DWORD)textureIndex >= field<DWORD>(manager, MANAGER_NUM_TEXTURES))
		{
			return 0;
		}

		BYTE* textures = field<BYTE*>(manager, MANAGER_TEXTURES);

		if (textures == NULL)
		{
			return 0;
		}

		return field<DWORD>(textures, textureIndex * TEXTURE_SIZE + TEXTURE_D3D_TEXTURE);
	}

	void checkCarSlot(int carIndex, const CarSlot& slot)
	{
		// The cars exist by now: confirm they match the calculated team and car
		BYTE* car = carAt(carIndex);

		if (car == NULL)
		{
			return;
		}

		int carTeam = field<int>(car, CAR_TEAM_INDEX);
		int carInTeam = field<int>(car, CAR_TEAM_CAR);

		if (carTeam != slot.team || carInTeam != slot.car)
		{
			OutputGP4PPDebugString("Steering Wheels: car index " + to_string(carIndex + 1) + " calculated as team " + to_string(slot.team + 1) + " car " + to_string(slot.car + 1) + ", but GP4 has team " + to_string(carTeam + 1) + " car " + to_string(carInTeam + 1));
		}
	}

	void loadExternalTextures(BYTE* device)
	{
		carTextureIndex.assign(numCars, -1);
		promotionLogged.assign(numCars, false);

		firstDrawLogged = false;

		// GP4 gave up on the external steering wheels, nothing to draw
		if (device == NULL || MemUtils::addressToValue<DWORD>(stWheelVertexBuffer) == 0)
		{
			OutputGP4PPDebugString("Steering Wheels: external steering wheels not initialised by GP4, keeping default");
			return;
		}

		BYTE* manager = field<BYTE*>(device, DEVICE_MANAGER);

		if (manager == NULL)
		{
			return;
		}

		int defaultTextureIndex = MemUtils::addressToValue<int>(stWheelTextureIndex);

		// Same texture manager settings InitCarSteeringWheels uses for stwheel.tga
		DWORD textureResize = field<DWORD>(manager, MANAGER_TEXTURE_RESIZE);
		field<DWORD>(manager, MANAGER_TEXTURE_RESIZE) = 0;

		BackupTextureBasePathFunc(manager);
		SetTextureBasePathFunc(manager, MemUtils::addressToPtr<char>(stWheelTextureBasePath));

		for (int carIndex = 0; carIndex < numCars; carIndex++)
		{
			CarSlot slot = carSlot(carIndex);

			if (!validTeam(slot))
			{
				OutputGP4PPDebugString("Steering Wheels: car index " + to_string(carIndex + 1) + " has no valid team, using default external texture");
				continue;
			}

			checkCarSlot(carIndex, slot);

			string fileName = carFileName(externalSettings.fileName, carIndex);
			string assetName = carsFolderPath(texFileName(fileName));

			if (!assetFileExists(assetName))
			{
				OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " external texture not found, using default [" + assetName + "]");
				continue;
			}

			// GP4 loads each texture once, so cars using the same file get the same index
			int textureIndex = GetTextureIndexFunc(manager, fileName.c_str(), 2, 0x7fffffff, 0x7fffffff, 0, 0, 0);

			// GetTextureIndex reserves a slot even when loading fails, so check the actual D3D texture
			if (textureIndex < 0 || textureIndex == defaultTextureIndex || textureD3DPointer(manager, textureIndex) == 0)
			{
				OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " external texture failed to load, using default [" + assetName + "]");
				continue;
			}

			carTextureIndex[carIndex] = textureIndex;

			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " external texture loaded at index " + to_string(textureIndex) + " [" + assetName + "]");
		}

		RestoreTextureBasePathFunc(manager);

		field<DWORD>(manager, MANAGER_TEXTURE_RESIZE) = textureResize;
	}

	void __cdecl initCarSteeringWheelsFunc(BYTE* device)
	{
		// original function call
		InitCarSteeringWheelsFunc(device);

		if (externalSettings.enabled)
		{
			loadExternalTextures(device);
		}
	}

	int drawCockpitMeshWheels(BYTE* device, int skipCar);

	bool isFrameMeshCar(BYTE* car)
	{
		for (int i = 0; i < numFrameMeshCars; i++)
		{
			if (meshCars[i] == car)
			{
				return true;
			}
		}

		return false;
	}

	void __cdecl hideMeshCarWheels()
	{
		BYTE* lodManager = MemUtils::addressToValue<BYTE*>(lodManagerPointer);

		if (wheelIndexBuffer == NULL || numFrameMeshCars == 0 || lodManager == NULL)
		{
			return;
		}

		// Same car selection and buffer order as DrawCarSteeringWheels: no light map from the start, light map from the end
		float maxDistance = MemUtils::addressToValue<float>(stWheelMaxDistance);
		DWORD numEntries = field<DWORD>(lodManager, LOD_NUM_ENTRIES);
		BYTE* entry = lodManager + LOD_ENTRIES;
		int numFront = 0;
		int numBack = 0;
		int numHidden = 0;
		bool matched = true;

		for (DWORD entryIndex = 0; entryIndex < numEntries; entryIndex++, entry += LOD_ENTRY_SIZE)
		{
			int carIndex = field<int>(entry, LOD_ENTRY_CAR);

			if (frameSkipCar != -1 && carIndex == frameSkipCar)
			{
				continue;
			}

			if (field<BYTE>(entry, LOD_ENTRY_VISIBLE) == 0 || !(field<float>(entry, LOD_ENTRY_DISTANCE) < maxDistance))
			{
				continue;
			}

			BYTE* car = (carIndex >= 0 && carIndex < numCars) ? carAt(carIndex) : NULL;

			if (car == NULL)
			{
				// GP4 does not check the car index, so the buffer order can't be followed
				matched = false;
				break;
			}

			if (field<BYTE>(car, CAR_WHEEL_VISIBLE) == 0)
			{
				continue;
			}

			if (numFront + numBack >= numWheelBlocks)
			{
				matched = false;
				break;
			}

			int block = (field<int>(car, CAR_WHEEL_TEXTURE) == -1) ? numFront++ : numWheelBlocks - 1 - numBack++;

			if (isFrameMeshCar(car))
			{
				hiddenWheelBlocks[numHidden++] = block;
			}
		}

		// Only touch the buffer when every car GP4 wrote was accounted for
		if (!matched || numFront + numBack != MemUtils::addressToValue<int>(stWheelCarsDrawn))
		{
			if (!wheelBlocksLogged)
			{
				wheelBlocksLogged = true;

				OutputGP4PPDebugString("Steering Wheels: external wheel buffer order not matched, flat wheel kept on the 3D wheel cars");
			}

			return;
		}

		// Collapse the wheel front, back and rev lights to a point, the gloves and arms stay
		for (int i = 0; i < numHidden; i++)
		{
			WORD* indices = wheelIndexBuffer + hiddenWheelBlocks[i] * WHEEL_BLOCK_INDICES;
			WORD first = indices[0];

			for (int j = 1; j < WHEEL_ONLY_INDICES; j++)
			{
				indices[j] = first;
			}
		}
	}

	__declspec(naked) void wheelIndexBlockAddFunc()
	{
		//ECX - locked index buffer
		__asm mov wheelIndexBuffer, ECX

		__asm add ECX, wheelIndexBlockOffset //original instruction, offset read from it

		__asm jmp wheelIndexBlockAddJumpBack //jump back into regular flow
	}

	__declspec(naked) void wheelVertexUnlockFunc()
	{
		//ESI - vertex buffer, the index buffer is still locked
		__asm pushad
		__asm pushfd

		hideMeshCarWheels();

		__asm popfd
		__asm popad

		//original instructions
		__asm mov EAX, dword ptr[ESI]
		__asm push ESI
		__asm call dword ptr[EAX + 0x30]

		__asm jmp wheelVertexUnlockJumpBack //jump back into regular flow
	}

	void __cdecl renderCarSteeringWheelsFunc(BYTE* device, DWORD param)
	{
		// Draw the 3D wheels first, the flat wheel of those cars is hidden for the original call (only the wheel with the gloves patch)
		int numMeshCars = useCockpitMesh ? drawCockpitMeshWheels(device, (int)param) : 0;

		// With the gloves patch the original call draws those cars too, then only their wheel gets collapsed
		numFrameMeshCars = numMeshCars;
		frameSkipCar = (int)param;
		wheelIndexBuffer = NULL;

		int numChanged = 0;

		// Temporarily give the cars with their own wheel a separate draw with their texture
		for (int carIndex = 0; carIndex < numCars; carIndex++)
		{
			BYTE* car = carAt(carIndex);

			if (car == NULL || carTextureIndex[carIndex] < 0)
			{
				continue;
			}

			changedCars[numChanged] = car;
			oldTextures[numChanged] = field<int>(car, CAR_WHEEL_TEXTURE);
			numChanged++;

			field<int>(car, CAR_WHEEL_TEXTURE) = carTextureIndex[carIndex];
		}

		if (!firstDrawLogged)
		{
			firstDrawLogged = true;

			OutputGP4PPDebugString("Steering Wheels: first external draw with " + to_string(numChanged) + " car(s) using their own steering wheel");
		}

		insideWheelRender = true;

		// original function call
		RenderCarSteeringWheelsFunc(device, param);

		insideWheelRender = false;

		// Restore every car field that was changed
		for (int i = numChanged - 1; i >= 0; i--)
		{
			field<int>(changedCars[i], CAR_WHEEL_TEXTURE) = oldTextures[i];
		}

		for (int i = 0; i < numMeshCars; i++)
		{
			field<BYTE>(meshCars[i], CAR_WHEEL_VISIBLE) = oldWheelVisible[i];
		}
	}

	void __fastcall individualWheelFlushFunc(BYTE* device, void* /*edx*/)
	{
		if (insideWheelRender && device != NULL)
		{
			DWORD stageOneTexture = field<DWORD>(device, DEVICE_STAGE1_TEXTURE);
			BYTE* manager = field<BYTE*>(device, DEVICE_MANAGER);

			for (int carIndex = 0; stageOneTexture != 0 && carIndex < numCars; carIndex++)
			{
				// Only promote textures that are ours, leave any other GP4 overlay untouched
				if (carTextureIndex[carIndex] >= 0 && textureD3DPointer(manager, carTextureIndex[carIndex]) == stageOneTexture)
				{
					field<DWORD>(device, DEVICE_STAGE0_TEXTURE) = stageOneTexture;
					field<DWORD>(device, DEVICE_STAGE1_TEXTURE) = 0;
					field<BYTE>(device, DEVICE_STAGE0_DIRTY) = 1;
					field<BYTE>(device, DEVICE_STAGE1_DIRTY) = 1;
					field<BYTE>(device, DEVICE_TEXTURES_DIRTY) = 1;

					// Cars sharing a texture match the first of them, so this is logged once per texture
					if (!promotionLogged[carIndex])
					{
						promotionLogged[carIndex] = true;

						OutputGP4PPDebugString("Steering Wheels: external texture at index " + to_string(carTextureIndex[carIndex]) + " bound to stage 0");
					}

					break;
				}
			}
		}

		// original function call
		DeviceFlushStateFunc(device);
	}

	//
	// Cockpit steering wheel and dash
	//

	BYTE cockpitShader(DWORD shaderPushAddress, const string& name)
	{
		// Follow whatever shader GP4 uses for the vanilla mesh (0x13, or 0x12 for the wheel with StWheelAdvancedCarShader)
		BYTE* shaderPush = MemUtils::addressToPtr<BYTE>(shaderPushAddress);

		if (shaderPush[0] == 0x6a)
		{
			return shaderPush[1];
		}

		OutputGP4PPDebugString("Steering Wheels: unexpected " + name + " shader code, using default shader 0x13");

		return 0x13;
	}

	void hideAllObjects(BYTE* mesh)
	{
		if (mesh == NULL)
		{
			return;
		}

		DWORD numObjects = field<DWORD>(mesh, MESH_NUM_OBJECTS);

		if (numObjects > MAX_MESH_OBJECTS)
		{
			return;
		}

		for (int objectIndex = (int)numObjects - 1; objectIndex >= 0; objectIndex--)
		{
			SetObjectVisibleFunc(mesh, 0, objectIndex);
		}
	}

	bool initParentLinks(BYTE* mesh)
	{
		// Same parent matrix fix-up CGP4Cockpit::LoadMesh applies, with every pointer checked first
		DWORD numObjects = field<DWORD>(mesh, MESH_NUM_OBJECTS);
		BYTE* objects = field<BYTE*>(mesh, MESH_OBJECTS);
		int parent = field<int>(mesh, MESH_PARENT_OBJECT);

		if (numObjects == 0 || numObjects > MAX_MESH_OBJECTS || objects == NULL || parent < 0 || (DWORD)parent >= numObjects)
		{
			return false;
		}

		BYTE* parentRecord = objects + parent * MESH_OBJECT_SIZE;
		BYTE* parentData = field<BYTE*>(parentRecord, MESH_OBJECT_DATA);
		int parentObject = field<int>(parentRecord, MESH_OBJECT_INDEX);

		if (parentData == NULL || parentObject < 0 || (DWORD)parentObject > MAX_MESH_OBJECTS)
		{
			return false;
		}

		BYTE* parentTransforms = field<BYTE*>(parentData, OBJECT_DATA_TRANSFORMS);
		BYTE* parentFlags = field<BYTE*>(parentData, OBJECT_DATA_FLAGS);

		if (parentTransforms == NULL || parentFlags == NULL)
		{
			return false;
		}

		BYTE* rootMatrix = parentTransforms + parentObject * OBJECT_TRANSFORM_SIZE + OBJECT_TRANSFORM_MATRIX;

		for (DWORD objectIndex = 0; objectIndex < numObjects; objectIndex++)
		{
			BYTE* record = objects + objectIndex * MESH_OBJECT_SIZE;
			BYTE* data = field<BYTE*>(record, MESH_OBJECT_DATA);
			int object = field<int>(record, MESH_OBJECT_INDEX);

			if (data == NULL || object < 0 || (DWORD)object > MAX_MESH_OBJECTS || field<BYTE*>(data, OBJECT_DATA_TRANSFORMS) == NULL)
			{
				return false;
			}
		}

		for (DWORD objectIndex = 0; objectIndex < numObjects; objectIndex++)
		{
			BYTE* record = objects + objectIndex * MESH_OBJECT_SIZE;
			BYTE* data = field<BYTE*>(record, MESH_OBJECT_DATA);
			int object = field<int>(record, MESH_OBJECT_INDEX);

			field<BYTE*>(field<BYTE*>(data, OBJECT_DATA_TRANSFORMS), object * OBJECT_TRANSFORM_SIZE + OBJECT_TRANSFORM_PARENT) = rootMatrix;
		}

		field<DWORD>(parentFlags, parentObject * OBJECT_FLAGS_SIZE) |= 1;

		return true;
	}

	void readMapping(BYTE* cockpit, CockpitMapping& mapping)
	{
		memcpy(mapping.selectors, cockpit + COCKPIT_MESH_SELECTORS, sizeof(mapping.selectors));
		memcpy(mapping.indices, cockpit + COCKPIT_OBJECT_INDICES, sizeof(mapping.indices));
	}

	void writeMapping(BYTE* cockpit, const CockpitMapping& mapping)
	{
		memcpy(cockpit + COCKPIT_MESH_SELECTORS, mapping.selectors, sizeof(mapping.selectors));
		memcpy(cockpit + COCKPIT_OBJECT_INDICES, mapping.indices, sizeof(mapping.indices));
	}

	void buildMapping(BYTE* dashMesh, BYTE* wheelMesh, CockpitMapping& mapping)
	{
		// Same lookup as CGP4Cockpit::LoadMesh: dash first, the wheel mesh overrides objects with the same name
		const char** objectNames = MemUtils::addressToPtr<const char*>(cockpitObjectNames);

		for (int i = 0; i < NUM_COCKPIT_OBJECTS; i++)
		{
			mapping.selectors[i] = 0;
			mapping.indices[i] = -1;

			if (objectNames[i] == NULL)
			{
				continue;
			}

			DWORD dashIndex = GetObjectIndexByNameFunc(dashMesh, objectNames[i]);

			if (dashIndex != 0xffffffff)
			{
				mapping.selectors[i] = 0;
				mapping.indices[i] = (int)dashIndex;
			}

			DWORD wheelIndex = GetObjectIndexByNameFunc(wheelMesh, objectNames[i]);

			if (wheelIndex != 0xffffffff)
			{
				mapping.selectors[i] = 1;
				mapping.indices[i] = (int)wheelIndex;
			}
		}
	}

	bool loadCockpitMesh(BYTE* manager, BYTE shader, CockpitMesh& entry)
	{
		// Same mesh settings CGP4Cockpit::LoadMesh uses for cockpit_dash.gp4 and cockpit_wheel.gp4
		BYTE numMipMaps = field<BYTE>(manager, MANAGER_NUM_MIPMAPS);
		field<BYTE>(manager, MANAGER_NUM_MIPMAPS) = 1;

		DWORD meshIndex = AllocateMeshFunc(manager, 0, 0);
		DWORD* meshes = field<DWORD*>(manager, MANAGER_MESHES);
		BYTE* mesh = (meshIndex == 0xffffffff || meshes == NULL) ? NULL : (BYTE*)meshes[meshIndex];

		bool fileLoaded = false;

		if (mesh != NULL)
		{
			field<DWORD>(mesh, 0x22c) = 1;
			field<DWORD>(mesh, 0x230) = 1;
			field<DWORD>(mesh, 0x234) = 1;
			field<DWORD>(mesh, 0x238) = 0;
			field<DWORD>(mesh, 0x224) = 0x40800000;
			field<DWORD>(mesh, 0x220) = 0;
			field<DWORD>(mesh, 0x228) = 0;

			// Textures referenced by the mesh are resolved relative to the mesh's own folder
			fileLoaded = LoadGP4MeshFileFunc(mesh, entry.fileName.c_str(), 0);
		}

		field<BYTE>(manager, MANAGER_NUM_MIPMAPS) = numMipMaps;

		bool prepared = false;

		if (fileLoaded)
		{
			PrepareMeshFunc(mesh, 0, -1);
			field<BYTE>(mesh, MESH_PREPARED) = 1;

			prepared = initParentLinks(mesh);
		}

		BYTE* shaderObject = prepared ? GetShaderFunc(shader) : NULL;

		if (shaderObject == NULL)
		{
			if (fileLoaded)
			{
				hideAllObjects(mesh);
			}

			if (meshIndex != 0xffffffff)
			{
				RemoveMeshFunc(manager, meshIndex);
			}

			return false;
		}

		ApplyShaderToObjectFunc(mesh, shaderObject, -1, 1);

		hideAllObjects(mesh);

		entry.loaded = true;
		entry.mesh = mesh;
		entry.meshIndex = meshIndex;

		return true;
	}

	void resetCockpitState()
	{
		cockpitObject = NULL;
		vanillaDashMesh = NULL;
		vanillaWheelMesh = NULL;
		selectedSet = VANILLA;

		cockpitDashMeshes.clear();
		cockpitWheelMeshes.clear();
		cockpitSets.clear();
		carCockpitSet.assign(numCars, VANILLA);
		selectionLogged.assign(numCars, false);
		externalWheelObjects.clear();
	}

	int findCockpitMesh(const vector<CockpitMesh>& meshes, const string& fileName)
	{
		for (size_t index = 0; index < meshes.size(); index++)
		{
			if (_stricmp(meshes[index].fileName.c_str(), fileName.c_str()) == 0)
			{
				return (int)index;
			}
		}

		return VANILLA;
	}

	BYTE* dashMeshOf(int dash)
	{
		return dash == VANILLA ? vanillaDashMesh : cockpitDashMeshes[dash].mesh;
	}

	BYTE* wheelMeshOf(int wheel)
	{
		return wheel == VANILLA ? vanillaWheelMesh : cockpitWheelMeshes[wheel].mesh;
	}

	string meshFileOf(const vector<CockpitMesh>& meshes, int index)
	{
		return index == VANILLA ? "default" : meshes[index].fileName;
	}

	int loadCarCockpitMesh(BYTE* manager, BYTE shader, vector<CockpitMesh>& meshes, const string& pattern, int carIndex, const string& name)
	{
		// Each file is tried once, every car resolving to the same file shares the result
		string assetName = carsFolderPath(carFileName(pattern, carIndex));
		int index = findCockpitMesh(meshes, assetName);

		if (index != VANILLA)
		{
			if (meshes[index].loaded)
			{
				OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " " + name + " shared [" + assetName + "]");

				return index;
			}

			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " " + name + " not available, using default [" + assetName + "]");

			return VANILLA;
		}

		CockpitMesh entry;
		entry.loaded = false;
		entry.mesh = NULL;
		entry.meshIndex = 0xffffffff;
		entry.fileName = assetName;

		if (!assetFileExists(assetName))
		{
			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " " + name + " not found, using default [" + assetName + "]");
		}
		else if (loadCockpitMesh(manager, shader, entry))
		{
			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " " + name + " loaded [" + assetName + "]");
		}
		else
		{
			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " " + name + " failed to load, using default [" + assetName + "]");
		}

		meshes.push_back(entry);

		return entry.loaded ? (int)meshes.size() - 1 : VANILLA;
	}

	int findOrAddCockpitSet(int dash, int wheel)
	{
		for (size_t set = 0; set < cockpitSets.size(); set++)
		{
			if (cockpitSets[set].dash == dash && cockpitSets[set].wheel == wheel)
			{
				return (int)set;
			}
		}

		CockpitSet set;
		set.dash = dash;
		set.wheel = wheel;

		buildMapping(dashMeshOf(dash), wheelMeshOf(wheel), set.mapping);

		cockpitSets.push_back(set);

		return (int)cockpitSets.size() - 1;
	}

	void loadCockpitMeshes(BYTE* cockpit)
	{
		// A cockpit that was never destroyed through the hooked call cannot be cleaned up anymore, just forget it
		resetCockpitState();

		BYTE* device = field<BYTE*>(cockpit, COCKPIT_DEVICE);
		BYTE* manager = device != NULL ? field<BYTE*>(device, DEVICE_MANAGER) : NULL;
		BYTE* dashMesh = field<BYTE*>(cockpit, COCKPIT_DASH_MESH);
		BYTE* wheelMesh = field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH);

		if (manager == NULL || dashMesh == NULL || wheelMesh == NULL)
		{
			OutputGP4PPDebugString("Steering Wheels: cockpit meshes not loaded by GP4, keeping default cockpit dash and steering wheel");
			return;
		}

		cockpitObject = cockpit;
		vanillaDashMesh = dashMesh;
		vanillaWheelMesh = wheelMesh;
		readMapping(cockpit, vanillaMapping);

		BYTE dashShader = cockpitShader(cockpitDashShaderPush, "cockpit dash");
		BYTE wheelShader = cockpitShader(cockpitWheelShaderPush, "cockpit wheel");

		cockpitDashMeshes.reserve(numCars);
		cockpitWheelMeshes.reserve(numCars);

		for (int carIndex = 0; carIndex < numCars; carIndex++)
		{
			if (!validTeam(carSlot(carIndex)))
			{
				OutputGP4PPDebugString("Steering Wheels: car index " + to_string(carIndex + 1) + " has no valid team, using default cockpit dash and steering wheel");
				continue;
			}

			int dash = dashSettings.enabled ? loadCarCockpitMesh(manager, dashShader, cockpitDashMeshes, dashSettings.fileName, carIndex, "cockpit dash") : VANILLA;
			int wheel = cockpitSettings.enabled ? loadCarCockpitMesh(manager, wheelShader, cockpitWheelMeshes, cockpitSettings.fileName, carIndex, "cockpit steering wheel") : VANILLA;

			if (dash != VANILLA || wheel != VANILLA)
			{
				carCockpitSet[carIndex] = findOrAddCockpitSet(dash, wheel);
			}
		}
	}

	void __fastcall cockpitLoadMeshFunc(BYTE* cockpit, void* /*edx*/, BYTE* device)
	{
		// original function call
		CockpitLoadMeshFunc(cockpit, device);

		if (cockpit != NULL)
		{
			loadCockpitMeshes(cockpit);
		}
	}

	int viewedCar()
	{
		// Same lookup UpdateCockpitView makes right before the hooked call
		int view = MemUtils::addressToValue<int>(currentView);

		if (view < 1)
		{
			return VANILLA;
		}

		return MemUtils::addressToValue<int>(viewCameraCar + (view - 1) * sizeof(DWORD));
	}

	void selectCockpitSet(BYTE* cockpit, int carIndex)
	{
		bool validCar = carIndex >= 0 && carIndex < numCars;
		int wantedSet = validCar ? carCockpitSet[carIndex] : VANILLA;

		// Cars sharing a dash and steering wheel do not need a switch
		if (wantedSet == selectedSet)
		{
			return;
		}

		// Hide everything of the previous meshes; the cockpit update shows the objects of the new mapping again
		if (selectedSet == VANILLA)
		{
			hideAllObjects(vanillaDashMesh);
			hideAllObjects(vanillaWheelMesh);
		}
		else
		{
			hideAllObjects(dashMeshOf(cockpitSets[selectedSet].dash));
			hideAllObjects(wheelMeshOf(cockpitSets[selectedSet].wheel));
		}

		if (wantedSet == VANILLA)
		{
			field<BYTE*>(cockpit, COCKPIT_DASH_MESH) = vanillaDashMesh;
			field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) = vanillaWheelMesh;
			writeMapping(cockpit, vanillaMapping);
		}
		else
		{
			const CockpitSet& set = cockpitSets[wantedSet];

			field<BYTE*>(cockpit, COCKPIT_DASH_MESH) = dashMeshOf(set.dash);
			field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) = wheelMeshOf(set.wheel);
			writeMapping(cockpit, set.mapping);

			if (!selectionLogged[carIndex])
			{
				selectionLogged[carIndex] = true;

				OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " cockpit selected, dash [" + meshFileOf(cockpitDashMeshes, set.dash) + "], steering wheel [" + meshFileOf(cockpitWheelMeshes, set.wheel) + "]");
			}
		}

		selectedSet = wantedSet;
	}

	void __fastcall cockpitUpdateFunc(BYTE* cockpit, void* /*edx*/, DWORD steeringAngle, int team)
	{
		// Select the meshes of the viewed car before GP4 updates the cockpit objects; they stay selected for the draw
		if (cockpit != NULL && cockpit == cockpitObject)
		{
			selectCockpitSet(cockpit, viewedCar());
		}

		// original function call
		CockpitUpdateFunc(cockpit, steeringAngle, team);
	}

	void __fastcall cockpitDestructorFunc(BYTE* cockpit, void* /*edx*/)
	{
		if (cockpit != NULL && cockpit == cockpitObject)
		{
			// Hand the vanilla meshes and mappings back to GP4 before it cleans up
			field<BYTE*>(cockpit, COCKPIT_DASH_MESH) = vanillaDashMesh;
			field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) = vanillaWheelMesh;
			writeMapping(cockpit, vanillaMapping);

			BYTE* device = field<BYTE*>(cockpit, COCKPIT_DEVICE);
			BYTE* manager = device != NULL ? field<BYTE*>(device, DEVICE_MANAGER) : NULL;

			if (manager != NULL)
			{
				// Free our meshes, highest index first
				vector<DWORD> meshIndices;

				for (const vector<CockpitMesh>* meshes : { &cockpitDashMeshes, &cockpitWheelMeshes })
				{
					for (const CockpitMesh& entry : *meshes)
					{
						if (entry.loaded)
						{
							meshIndices.push_back(entry.meshIndex);
						}
					}
				}

				sort(meshIndices.begin(), meshIndices.end(), greater<DWORD>());

				for (DWORD meshIndex : meshIndices)
				{
					RemoveMeshFunc(manager, meshIndex);
				}

				OutputGP4PPDebugString("Steering Wheels: " + to_string(meshIndices.size()) + " cockpit dash and steering wheel mesh(es) freed");
			}

			resetCockpitState();
		}

		// original function call
		CockpitDestructorFunc(cockpit);
	}

	//
	// 3D external steering wheels
	//

	Matrix multiply(const Matrix& a, const Matrix& b)
	{
		Matrix result;

		for (int row = 0; row < 4; row++)
		{
			for (int column = 0; column < 4; column++)
			{
				result.m[row][column] = a.m[row][0] * b.m[0][column] + a.m[row][1] * b.m[1][column] + a.m[row][2] * b.m[2][column] + a.m[row][3] * b.m[3][column];
			}
		}

		return result;
	}

	Matrix identity()
	{
		Matrix result = {};

		result.m[0][0] = result.m[1][1] = result.m[2][2] = result.m[3][3] = 1.0f;

		return result;
	}

	Matrix translation(float x, float y, float z)
	{
		Matrix result = identity();

		result.m[3][0] = x;
		result.m[3][1] = y;
		result.m[3][2] = z;

		return result;
	}

	Matrix scaling(float scale)
	{
		Matrix result = identity();

		result.m[0][0] = result.m[1][1] = result.m[2][2] = scale;

		return result;
	}

	Matrix rotationX(float angle)
	{
		// Same as D3DXMatrixRotationX
		Matrix result = identity();

		result.m[1][1] = cosf(angle);
		result.m[1][2] = sinf(angle);
		result.m[2][1] = -sinf(angle);
		result.m[2][2] = cosf(angle);

		return result;
	}

	Matrix rotationZ(float angle)
	{
		// Same as D3DXMatrixRotationZ
		Matrix result = identity();

		result.m[0][0] = cosf(angle);
		result.m[0][1] = sinf(angle);
		result.m[1][0] = -sinf(angle);
		result.m[1][1] = cosf(angle);

		return result;
	}

	float externalWheelAngle(BYTE* car)
	{
		// Same wrap and clamp the flat external wheel uses
		const float pi = 3.1415927f;
		float angle = field<float>(car, CAR_STEERING_ANGLE);

		if (angle < 0.0f)
		{
			angle += 2.0f * pi;
		}

		if (angle > pi)
		{
			angle -= 2.0f * pi;
		}

		return max(-pi / 2.0f, min(pi / 2.0f, angle));
	}

	Matrix externalWheelMatrix(BYTE* car, int team)
	{
		// The mesh origin is the rotation point: placed at the centre of the flat wheel, turned and tilted like in the cockpit
		float offsetY = MemUtils::addressToValue<float>(stWheelTeamOffsets + team * 8);
		float offsetZ = MemUtils::addressToValue<float>(stWheelTeamOffsets + team * 8 + 4);
		float wheelY = MemUtils::addressToValue<float>(stWheelY);
		float wheelZ = MemUtils::addressToValue<float>(stWheelZ);
		float tilt = MemUtils::addressToValue<float>(cockpitWheelTilt);

		Matrix world = multiply(scaling(cockpitMeshScale), rotationZ(externalWheelAngle(car)));
		world = multiply(world, rotationX(tilt));
		world = multiply(world, translation(0.0f, offsetY + wheelY, offsetZ + wheelZ));

		return multiply(world, field<Matrix>(car, CAR_MATRIX));
	}

	BYTE* externalWheelMesh(BYTE* cockpit, int carIndex)
	{
		// The car's own cockpit wheel, whichever one the cockpit has selected for the viewed car
		if (cockpit == NULL)
		{
			return NULL;
		}

		if (cockpit == cockpitObject && vanillaWheelMesh != NULL)
		{
			int set = carCockpitSet[carIndex];

			return set == VANILLA ? vanillaWheelMesh : wheelMeshOf(cockpitSets[set].wheel);
		}

		// Cockpit patch not active, only the default wheel
		return field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH);
	}

	int findObject(BYTE* mesh, int cockpitObject)
	{
		const char* name = MemUtils::addressToPtr<const char*>(cockpitObjectNames)[cockpitObject];

		if (name == NULL)
		{
			return -1;
		}

		DWORD objectIndex = GetObjectIndexByNameFunc(mesh, name);

		return objectIndex == 0xffffffff ? -1 : (int)objectIndex;
	}

	const ExternalWheelObjects& findExternalWheelObjects(BYTE* mesh)
	{
		auto found = externalWheelObjects.find(mesh);

		if (found != externalWheelObjects.end())
		{
			return found->second;
		}

		ExternalWheelObjects& wheelObjects = externalWheelObjects[mesh];
		vector<int>& objects = wheelObjects.statics;

		// Rev lights, same objects CGP4Cockpit::SetRevLights uses
		int numRevLights = 0;

		for (int light = 0; light < NUM_REV_LIGHTS; light++)
		{
			for (int side = 0; side < 2; side++)
			{
				int revOn = (side == 0 ? COCKPIT_LEFT_REV1_ON : COCKPIT_RIGHT_REV1_ON) + light * 2;

				wheelObjects.revOn[side][light] = findObject(mesh, revOn);

				if (wheelObjects.revOn[side][light] != -1)
				{
					numRevLights++;
				}
			}
		}

		// Flag lights and gears, same objects CGP4Cockpit::SetFlagLights and UpdateDisplayObjects use
		int numFlagLights = 0;
		int numGears = 0;

		for (int light = 0; light < NUM_FLAG_LIGHTS; light++)
		{
			for (int side = 0; side < 2; side++)
			{
				int flagOn = (side == 0 ? COCKPIT_LEFT_FLAG1_ON : COCKPIT_RIGHT_FLAG1_ON) + light * 2;

				wheelObjects.flagOn[side][light] = findObject(mesh, flagOn);

				if (wheelObjects.flagOn[side][light] != -1)
				{
					numFlagLights++;
				}
			}
		}

		for (int gear = 0; gear < NUM_GEARS; gear++)
		{
			wheelObjects.gears[gear] = findObject(mesh, COCKPIT_GEAR_R + gear);

			if (wheelObjects.gears[gear] != -1)
			{
				numGears++;
			}
		}

		// What the cockpit shows when nothing is lit: the steering wheel itself, Warning_off and the LEDs in their off state
		for (int cockpitObject : COCKPIT_STATIC_OBJECTS)
		{
			int objectIndex = findObject(mesh, cockpitObject);

			if (objectIndex != -1)
			{
				objects.push_back(objectIndex);
			}
		}

		OutputGP4PPDebugString("Steering Wheels: 3D external steering wheel uses " + to_string(objects.size()) + " static object(s), " + to_string(numRevLights) + " rev light(s), " + to_string(numFlagLights) + " flag light(s) and " + to_string(numGears) + " gear(s) of the cockpit steering wheel mesh at " + MemUtils::dwordToString(PtrToUlong(mesh)));

		// Other displays, counted per display as their objects are often part of the dash mesh instead
		string displayCounts;

		for (const DisplayGroup& group : COCKPIT_DISPLAY_GROUPS)
		{
			int numObjects = 0;

			for (int slot = group.first; slot <= group.last; slot++)
			{
				int objectIndex = findObject(mesh, slot);

				if (objectIndex != -1)
				{
					wheelObjects.displays.push_back(make_pair(slot, objectIndex));
					numObjects++;
				}
			}

			displayCounts += (displayCounts.empty() ? "" : ", ") + string(group.name) + " " + to_string(numObjects);
		}

		OutputGP4PPDebugString("Steering Wheels: display objects of the cockpit steering wheel mesh at " + MemUtils::dwordToString(PtrToUlong(mesh)) + ": " + displayCounts);

		return wheelObjects;
	}

	void showStaticObjects(BYTE* mesh, const ExternalWheelObjects& wheelObjects)
	{
		// Once per mesh and draw: only the static objects
		if (find(drawnMeshes.begin(), drawnMeshes.end(), mesh) != drawnMeshes.end())
		{
			return;
		}

		hideAllObjects(mesh);

		for (int objectIndex : wheelObjects.statics)
		{
			SetObjectVisibleFunc(mesh, 1, objectIndex);
		}

		drawnMeshes.push_back(mesh);
	}

	void showObject(BYTE* mesh, int objectIndex, bool visible)
	{
		if (objectIndex != -1)
		{
			SetObjectVisibleFunc(mesh, visible ? 1 : 0, objectIndex);
		}
	}

	void showRevLights(BYTE* mesh, const ExternalWheelObjects& wheelObjects, BYTE* car)
	{
		// Per car, as cars sharing a mesh have their own revs
		int numLit = (int)field<BYTE>(car, CAR_REV_LIGHTS) - 1;

		for (int light = 0; light < NUM_REV_LIGHTS; light++)
		{
			for (int side = 0; side < 2; side++)
			{
				showObject(mesh, wheelObjects.revOn[side][light], light < numLit);
			}
		}
	}

	void updateCarDynamicData()
	{
		// The physics data is not in car order: match it to the cars through its id, as GP4's renderer does.
		// Stale entries can repeat an id further down the array, so the first match wins, as in GP4's lookup at 0x00408359
		carDynamicData.assign(numCars, NULL);

		for (int entryIndex = 0; entryIndex < numDynamicData; entryIndex++)
		{
			BYTE* data = MemUtils::addressToPtr<BYTE>(carDynamicDataArray + entryIndex * DYNAMIC_DATA_SIZE);
			int carIndex = (field<BYTE>(data, DYNAMIC_ID) & 0x3f) - 1;

			if (carIndex >= 0 && carIndex < numCars && carDynamicData[carIndex] == NULL)
			{
				carDynamicData[carIndex] = data;
			}
		}
	}

	int flagLights(BYTE* data)
	{
		// Same lights DashWriteFlagLights computes for the viewed car: 1 / 4 / 0x10 left flag 1 / 2 / 3, 2 / 8 / 0x20 right
		int lights = 0;
		int flashing = 0;

		if ((field<BYTE>(data, DYNAMIC_RACE_FLAGS) & RACE_FLAGS_YELLOW) != 0)
		{
			lights = 0x08;
			flashing = 0x0c;
		}

		if ((field<BYTE>(data, DYNAMIC_FLAGS_122) & FLAGS_122_BLUE) != 0)
		{
			lights |= 0x10;
			flashing |= 0x30;
		}

		if (lights == 0)
		{
			if ((field<BYTE>(data, DYNAMIC_PIT_FLAGS) & PIT_FLAGS_FAILURE) == 0 && (field<BYTE>(data, DYNAMIC_LED_FLAGS) & LED_FLAGS_WARNING) == 0)
			{
				return 0;
			}

			flashing = 0x3f;
		}

		// GP4 keeps its flash timer for the viewed car only, all external wheels flash together from the game clock
		bool flash = ((MemUtils::addressToValue<DWORD>(raceClock) / FLAG_FLASH_PERIOD) & 1) != 0;

		return flash ? lights ^ flashing : lights;
	}

	void showDashObjects(BYTE* mesh, const ExternalWheelObjects& wheelObjects, int carIndex)
	{
		BYTE* data = carDynamicData.empty() ? NULL : carDynamicData[carIndex];
		int lights = data != NULL ? flagLights(data) : 0;
		int gear = data != NULL ? (int)static_cast<signed char>(field<BYTE>(data, DYNAMIC_GEAR)) + 1 : -1;

		for (int light = 0; light < NUM_FLAG_LIGHTS; light++)
		{
			for (int side = 0; side < 2; side++)
			{
				int bit = 1 << (light * 2 + side);

				showObject(mesh, wheelObjects.flagOn[side][light], (lights & bit) != 0);
			}
		}

		// Only the current gear, as in UpdateDisplayObjects
		for (int gearIndex = 0; gearIndex < NUM_GEARS; gearIndex++)
		{
			showObject(mesh, wheelObjects.gears[gearIndex], gearIndex == gear);
		}
	}

	int bestLapPosition(BYTE* data)
	{
		// As GetCarBestLapRank and LcdWritePosition: place of the car id in the best lap order, P1 when the car is not in it
		BYTE id = field<BYTE>(data, DYNAMIC_ID);
		BYTE* order = MemUtils::addressToPtr<BYTE>(bestLapOrder);
		int count = MemUtils::addressToValue<int>(numBestLapOrder);

		for (int index = 0; index < count && index < numDynamicData; index++)
		{
			if (order[index] == id)
			{
				return index + 1;
			}
		}

		return 1;
	}

	void lapTimeDigits(BYTE* data, int* digits)
	{
		// M:SS.mmm, -1 = "-". While the car's lap or split time message is pending or on screen the time is held, as
		// UpdateDashData does: LcdWriteSplitTime's time to the millisecond. Otherwise LcdWriteCurrentLapTime's running clock,
		// in whole seconds. "------" with no lap started or a time of 10 minutes or more
		for (int digit = 0; digit < NUM_LAP_TIME_DIGITS; digit++)
		{
			digits[digit] = -1;
		}

		int time;

		if ((field<BYTE>(data, DYNAMIC_LED_FLAGS) & LED_FLAGS_TIME) != 0)
		{
			BYTE split = field<BYTE>(data, DYNAMIC_LAST_SPLIT);

			if (split >= NUM_SPLIT_TIMES)
			{
				return;
			}

			time = field<int>(data, DYNAMIC_SPLIT_TIMES + split * sizeof(DWORD));

			if (time >= (int)MAX_SPLIT_TIME)
			{
				return;
			}
		}
		else
		{
			DWORD lapStart = field<DWORD>(data, DYNAMIC_LAP_START);

			if (lapStart == LAP_NOT_STARTED)
			{
				return;
			}

			time = (int)(MemUtils::addressToValue<DWORD>(raceClock) - lapStart);

			if (time > MAX_LAP_CLOCK)
			{
				time = MAX_LAP_CLOCK;
			}
		}

		// GP4's conversions count nothing for negative times
		if (time < 0)
		{
			time = 0;
		}

		digits[0] = time / 60000;
		digits[1] = time / 10000 % 6;
		digits[2] = time / 1000 % 10;

		if ((field<BYTE>(data, DYNAMIC_LED_FLAGS) & LED_FLAGS_TIME) != 0)
		{
			digits[3] = time / 100 % 10;
			digits[4] = time / 10 % 10;
			digits[5] = time % 10;
		}
	}

	void litDisplays(BYTE* data, int carIndex, bool* lit)
	{
		// Same values the Lcd and Dash writers compute for the viewed car, from this car's physics data
		bool race = MemUtils::addressToValue<BYTE>(sessionType) == SESSION_RACE;

		// Water temperature (LcdWriteEngineTemp, SetWaterDigits): the hundreds digit only exists as Water_A_1
		DWORD temperature = field<DWORD>(data, DYNAMIC_ENGINE_TEMPERATURE) >> 16;

		if (temperature > MAX_ENGINE_TEMPERATURE)
		{
			temperature = MAX_ENGINE_TEMPERATURE;
		}

		lit[COCKPIT_WATER_A_1] = temperature >= 100;
		lit[COCKPIT_WATER_B_0 + temperature / 10 % 10] = true;
		lit[COCKPIT_WATER_C_0 + temperature % 10] = true;

		// Fuel laps (LcdWriteFuel, SetFuelDigits): in a race GP4 only shows the player's, the others get "--.-",
		// unless General's Show Opponents Fuel Laps patch is applied
		BYTE* player = MemUtils::addressToValue<BYTE*>(playerDynamicData);

		lit[COCKPIT_FUEL_POINT] = true;

		if (!General::IsOpponentsFuelLapsPatched() && race && player != NULL && data != player && (field<BYTE>(player, DYNAMIC_STATE) & STATE_NOT_ACTIVE) == 0)
		{
			lit[COCKPIT_FUEL_F_MINUS] = true;
			lit[COCKPIT_FUEL_A_MINUS] = true;
			lit[COCKPIT_FUEL_B_MINUS] = true;
			lit[COCKPIT_FUEL_C_MINUS] = true;
		}
		else
		{
			DWORD fuel = (field<BYTE>(data, DYNAMIC_LCD_FLAGS_8B) & LCD_FLAGS_8B_MENU_FUEL) != 0 ? field<WORD>(data, DYNAMIC_MENU_FUEL_LAPS) : field<WORD>(data, DYNAMIC_FUEL_LAPS);
			DWORD laps = fuel >> 6;

			if (laps > MAX_FUEL_LAPS)
			{
				laps = MAX_FUEL_LAPS;
			}

			if (laps >= 10)
			{
				lit[COCKPIT_FUEL_A_0 + laps / 10] = true;
			}

			lit[COCKPIT_FUEL_B_0 + laps % 10] = true;
			lit[COCKPIT_FUEL_C_0 + ((fuel & 0x3f) * 5 >> 5)] = true;
		}

		// Position (LcdWritePosition, UpdateDisplayObjects): race position, otherwise best lap order
		int position = race ? (field<BYTE>(data, DYNAMIC_RACE_POSITION) >> 1) + 1 : bestLapPosition(data);

		lit[COCKPIT_POSITION_P] = true;

		if (position / 10 <= 2)
		{
			lit[COCKPIT_POSITION_A_0 + position / 10] = true;
			lit[COCKPIT_POSITION_B_0 + position % 10] = true;
		}

		// Lap (LcdWriteLapsDone, SetLapDigits): no leading zero
		int lap = field<BYTE>(data, DYNAMIC_CURRENT_LAP) % 100;

		if (lap >= 10)
		{
			lit[COCKPIT_LAP_A_0 + lap / 10] = true;
		}

		lit[COCKPIT_LAP_B_0 + lap % 10] = true;

		// Lap time (SetLapTimeDigits): colon and point always on, "-" as the _minus object of the digit
		int digits[NUM_LAP_TIME_DIGITS];

		lapTimeDigits(data, digits);

		lit[COCKPIT_LAP_TIME_COLON] = true;
		lit[COCKPIT_LAP_TIME_POINT] = true;

		for (int digit = 0; digit < NUM_LAP_TIME_DIGITS; digit++)
		{
			lit[COCKPIT_LAP_TIME_A_0 + digit * COCKPIT_LAP_TIME_DIGIT_SLOTS + (digits[digit] == -1 ? COCKPIT_LAP_TIME_MINUS : digits[digit])] = true;
		}

		// Speed (CalcDashSpeedDigits, UpdateDisplayObjects): three digits, leading zeros shown
		DWORD speed = field<DWORD>(data, DYNAMIC_SPEED);

		if ((int)speed < 0)
		{
			speed = 0 - speed;
		}

		speed = ((speed >> 16) * (MemUtils::addressToValue<short>(metricUnits) < 0 ? SPEED_FACTOR_KMH : SPEED_FACTOR_MPH)) >> 16;

		if (speed / 100 <= 3)
		{
			lit[COCKPIT_SPEED_A_0 + speed / 100] = true;
			lit[COCKPIT_SPEED_B_0 + speed / 10 % 10] = true;
			lit[COCKPIT_SPEED_C_0 + speed % 10] = true;
		}

		// Driver status (DashWriteDriverStatus): red for the cars GP4 marks as driven
		bool driven = data == MemUtils::addressToValue<BYTE*>(playerDynamicData) || (MemUtils::addressToValue<BYTE>(driverStatusEnabled) != 0 && data == MemUtils::addressToValue<BYTE*>(driverStatusData));

		lit[driven ? COCKPIT_DRIVER_STATUS_RED : COCKPIT_DRIVER_STATUS_OFF] = true;

		// Warning lights (SetWarningLights)
		WORD damage = field<WORD>(data, DYNAMIC_DAMAGE);

		for (int bit = 0; bit < 16; bit++)
		{
			if ((damage & (1 << bit)) != 0 && COCKPIT_WARNINGS[bit] != -1)
			{
				lit[COCKPIT_WARNINGS[bit]] = true;
			}
		}

		// Pit light (GetPitLightState): own, otherwise in a race the active team-mate's (car ids pair up 1 / 2, 3 / 4, ...)
		if ((field<BYTE>(data, DYNAMIC_PIT_LIGHT_FLAGS) & PIT_LIGHT) != 0)
		{
			lit[COCKPIT_PIT_1] = true;
		}
		else if (race)
		{
			int mateIndex = carIndex ^ 1;
			BYTE* mate = mateIndex < numCars ? carDynamicData[mateIndex] : NULL;

			if (mate != NULL && (field<BYTE>(mate, DYNAMIC_STATE) & STATE_NOT_ACTIVE) == 0 && (field<BYTE>(mate, DYNAMIC_PIT_LIGHT_FLAGS) & PIT_LIGHT) != 0)
			{
				lit[COCKPIT_PIT_2] = true;
			}
		}
	}

	void showDisplays(BYTE* mesh, const ExternalWheelObjects& wheelObjects, int carIndex)
	{
		if (wheelObjects.displays.empty())
		{
			return;
		}

		// Every display object is set for each car, as cars sharing a mesh show their own values
		BYTE* data = carDynamicData.empty() ? NULL : carDynamicData[carIndex];
		bool lit[NUM_COCKPIT_OBJECTS] = {};

		if (data != NULL)
		{
			litDisplays(data, carIndex, lit);
		}

		for (const pair<int, int>& display : wheelObjects.displays)
		{
			showObject(mesh, display.second, lit[display.first]);
		}
	}

	int drawCockpitMeshWheels(BYTE* device, int skipCar)
	{
		BYTE* cockpit = MemUtils::addressToValue<BYTE*>(cockpitPointer);
		BYTE* lodManager = MemUtils::addressToValue<BYTE*>(lodManagerPointer);

		if (device == NULL || cockpit == NULL || lodManager == NULL)
		{
			return 0;
		}

		BYTE* shader = GetShaderFunc(COCKPIT_LIGHT_MAP_SHADER);

		if (shader == NULL)
		{
			return 0;
		}

		if (numDynamicData > 0)
		{
			updateCarDynamicData();
		}

		int numMeshCars = 0;
		DWORD lightMap = field<DWORD>(shader, SHADER_LIGHT_MAP);
		DWORD normalizeNormals = 0;
		Matrix worldMatrix = field<Matrix>(device, DEVICE_WORLD_MATRIX);
		float maxDistance = MemUtils::addressToValue<float>(stWheelMaxDistance);
		DWORD numEntries = field<DWORD>(lodManager, LOD_NUM_ENTRIES);
		BYTE* entry = lodManager + LOD_ENTRIES;

		// Same car selection as the flat external wheels
		for (DWORD entryIndex = 0; entryIndex < numEntries; entryIndex++, entry += LOD_ENTRY_SIZE)
		{
			int carIndex = field<int>(entry, LOD_ENTRY_CAR);

			if ((skipCar != -1 && carIndex == skipCar) || carIndex < 0 || carIndex >= numCars)
			{
				continue;
			}

			BYTE* car = carAt(carIndex);

			if (car == NULL || field<BYTE>(entry, LOD_ENTRY_VISIBLE) == 0 || !(field<float>(entry, LOD_ENTRY_DISTANCE) < maxDistance) || field<BYTE>(car, CAR_WHEEL_VISIBLE) == 0)
			{
				continue;
			}

			int team = field<int>(car, CAR_TEAM_INDEX);

			BYTE* mesh = externalWheelMesh(cockpit, carIndex);

			if (team < 0 || team >= numTeams || mesh == NULL)
			{
				continue;
			}

			if (numMeshCars == 0)
			{
				// No light map, and normals kept unit length on the scaled mesh
				field<DWORD>(shader, SHADER_LIGHT_MAP) = 0xffffffff;
				field<BYTE*>(device, DEVICE_CURRENT_SHADER) = NULL;

				DeviceGetRenderStateFunc(device, D3DRS_NORMALIZENORMALS, &normalizeNormals);
				DeviceQueueRenderStateFunc(device, D3DRS_NORMALIZENORMALS, 1);
			}

			const ExternalWheelObjects& wheelObjects = findExternalWheelObjects(mesh);

			showStaticObjects(mesh, wheelObjects);
			showRevLights(mesh, wheelObjects, car);
			showDashObjects(mesh, wheelObjects, carIndex);
			showDisplays(mesh, wheelObjects, carIndex);

			Matrix world = externalWheelMatrix(car, team);

			MeshSetTransformFunc(mesh, &world.m[0][0], -1);
			MeshUpdateTransformsFunc(mesh, -1, 0);

			MeshDrawType draw = *reinterpret_cast<MeshDrawType*>(field<BYTE*>(mesh, 0) + MESH_DRAW);
			draw(mesh, -1, NULL, 0);

			meshCars[numMeshCars] = car;
			oldWheelVisible[numMeshCars] = field<BYTE>(car, CAR_WHEEL_VISIBLE);
			numMeshCars++;

			// Without the gloves patch GP4 skips the car, gloves and arms included
			if (!glovesPatched)
			{
				field<BYTE>(car, CAR_WHEEL_VISIBLE) = 0;
			}
		}

		if (numMeshCars > 0)
		{
			// The cockpit shows its own objects again when it draws
			for (BYTE* mesh : drawnMeshes)
			{
				hideAllObjects(mesh);
			}

			drawnMeshes.clear();

			field<DWORD>(shader, SHADER_LIGHT_MAP) = lightMap;
			field<BYTE*>(device, DEVICE_CURRENT_SHADER) = NULL;

			DeviceQueueRenderStateFunc(device, D3DRS_NORMALIZENORMALS, normalizeNormals);

			// Put the selected mesh back where the cockpit placed it this frame, the others are placed before each draw
			CockpitSetMeshTransformsFunc(cockpit, cockpit + COCKPIT_ROTATION_MATRIX, cockpit + COCKPIT_WORLD_MATRIX);

			// Same world transform restore the flat external wheels do
			BYTE* d3dDevice = field<BYTE*>(device, DEVICE_D3D_DEVICE);

			field<Matrix>(device, DEVICE_WORLD_MATRIX) = worldMatrix;

			if (d3dDevice != NULL)
			{
				D3DSetTransformType setTransform = *reinterpret_cast<D3DSetTransformType*>(field<BYTE*>(d3dDevice, 0) + D3D_SET_TRANSFORM);
				setTransform(d3dDevice, D3DTS_WORLD, &worldMatrix.m[0][0]);
			}
		}

		if (!firstMeshDrawLogged && numMeshCars > 0)
		{
			firstMeshDrawLogged = true;

			OutputGP4PPDebugString("Steering Wheels: first 3D external draw with " + to_string(numMeshCars) + " car(s)");
		}

		return numMeshCars;
	}

	void loadAssetSettings(IniFile& iniSettings, AssetSettings& settings)
	{
		try
		{
			settings.perTeam = iniSettings[settings.section]["PerTeam"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Per Team " + settings.name + ": " + (settings.perTeam ? "Enabled" : "Disabled"));

		try
		{
			settings.perDriver = iniSettings[settings.section]["PerDriver"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Per Driver " + settings.name + ": " + (settings.perDriver ? "Enabled" : "Disabled"));

		try
		{
			settings.autoName = iniSettings[settings.section]["AutoName"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("AutoName " + settings.name + ": " + (settings.autoName ? "Enabled" : "Disabled"));

		if (settings.autoName)
		{
			// PerDriver supersedes PerTeam; with neither, GP4's own asset is used
			if (settings.perDriver)
			{
				settings.fileName = settings.perDriverFileName;
			}
			else if (settings.perTeam)
			{
				settings.fileName = settings.perTeamFileName;
			}

			settings.enabled = settings.perTeam || settings.perDriver;

			if (settings.enabled)
			{
				OutputGP4PPDebugString("File name computed for " + settings.name + ": " + settings.fileName);
			}
		}
		else
		{
			try
			{
				settings.fileName = iniSettings[settings.section]["FileName"].getAs<string>();
			}
			catch (exception ex) {}

			// An empty file name means GP4's own asset
			settings.enabled = !settings.fileName.empty();

			if (settings.enabled)
			{
				OutputGP4PPDebugString("File name loaded for " + settings.name + ": " + settings.fileName);
			}
		}

		OutputGP4PPDebugString(settings.name + ": " + (settings.enabled ? "Enabled" : "Disabled, using default"));
	}

	void LoadSettings(IniFile iniSettings)
	{
		loadAssetSettings(iniSettings, externalSettings);

		try
		{
			useCockpitMesh = iniSettings[externalSettings.section]["UseCockpitMesh"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("UseCockpitMesh " + externalSettings.name + ": " + (useCockpitMesh ? "Enabled" : "Disabled"));

		try
		{
			cockpitMeshScale = iniSettings[externalSettings.section]["CockpitMeshScale"].getAs<float>();
		}
		catch (exception ex) {}

		if (!(cockpitMeshScale > 0.0f))
		{
			OutputGP4PPDebugString("Invalid CockpitMeshScale " + to_string(cockpitMeshScale) + ", using default");

			cockpitMeshScale = DEFAULT_COCKPIT_MESH_SCALE;
		}

		OutputGP4PPDebugString("CockpitMeshScale " + externalSettings.name + ": " + to_string(cockpitMeshScale));

		loadAssetSettings(iniSettings, cockpitSettings);
		loadAssetSettings(iniSettings, dashSettings);
	}

	void ApplyPatches()
	{
		bool cockpitEnabled = cockpitSettings.enabled || dashSettings.enabled;
		bool externalEnabled = externalSettings.enabled || useCockpitMesh;
		bool countsRead = (externalEnabled || cockpitEnabled) && readCarCounts();

		// External steering wheels: texture loading, render and stage flush calls
		if (externalEnabled && countsRead)
		{
			if (checkCallBytes(initCarSteeringWheelsCall, initCarSteeringWheelsBytes, "InitCarSteeringWheels")
				&& checkCallBytes(renderCarSteeringWheelsCall, renderCarSteeringWheelsBytes, "RenderCarSteeringWheels")
				&& checkCallBytes(individualWheelFlushCall, individualWheelFlushBytes, "DeviceFlushState"))
			{
				MemUtils::injectFunctionCall(initCarSteeringWheelsCall, PtrToUlong(initCarSteeringWheelsFunc), VAR_NAME(initCarSteeringWheelsFunc));

				MemUtils::injectFunctionCall(individualWheelFlushCall, PtrToUlong(individualWheelFlushFunc), VAR_NAME(individualWheelFlushFunc));

				MemUtils::injectFunctionCall(renderCarSteeringWheelsCall, PtrToUlong(renderCarSteeringWheelsFunc), VAR_NAME(renderCarSteeringWheelsFunc));

				// Gears and flags of the 3D wheels only, the rest of the patch does not need the car physics data
				if (useCockpitMesh)
				{
					readDynamicDataCount();

					// GP4's gloves and arms on the 3D wheels, without it those cars skip the flat renderer
					if (readWheelBlockCount())
					{
						BYTE indexBlockAddPatch[] = { 0xe9, 0x00, 0x00, 0x00, 0x00, 0x90 };
						BYTE vertexUnlockPatch[] = { 0xe9, 0x00, 0x00, 0x00, 0x00, 0x90 };
						DWORD indexBlockAddOffset = PtrToUlong(wheelIndexBlockAddFunc) - wheelIndexBlockAdd - 5;
						DWORD vertexUnlockOffset = PtrToUlong(wheelVertexUnlockFunc) - wheelVertexUnlock - 5;

						memcpy(&indexBlockAddPatch[1], &indexBlockAddOffset, sizeof(DWORD));
						memcpy(&vertexUnlockPatch[1], &vertexUnlockOffset, sizeof(DWORD));

						MemUtils::patchAddress((LPVOID)wheelIndexBlockAdd, indexBlockAddPatch, sizeof(indexBlockAddPatch));
						MemUtils::patchAddress((LPVOID)wheelVertexUnlock, vertexUnlockPatch, sizeof(vertexUnlockPatch));

						glovesPatched = true;
					}

					OutputGP4PPDebugString("Steering Wheels: gloves and arms on the 3D external steering wheels : " + string(glovesPatched ? "Applied" : "Not applied"));
				}

				externalPatched = true;
			}
		}

		// Cockpit steering wheel and dash: mesh loading, per-frame selection and clean-up calls
		if (cockpitEnabled && countsRead)
		{
			if (checkCallBytes(cockpitLoadMeshCall, cockpitLoadMeshBytes, "CGP4Cockpit::LoadMesh")
				&& checkCallBytes(cockpitUpdateCall, cockpitUpdateBytes, "CGP4Cockpit update")
				&& checkCallBytes(cockpitDestructorCall, cockpitDestructorBytes, "CGP4Cockpit destructor"))
			{
				resetCockpitState();

				// Destructor first, so that a cockpit with our meshes can never be destroyed without clean-up
				MemUtils::injectFunctionCall(cockpitDestructorCall, PtrToUlong(cockpitDestructorFunc), VAR_NAME(cockpitDestructorFunc));

				MemUtils::injectFunctionCall(cockpitUpdateCall, PtrToUlong(cockpitUpdateFunc), VAR_NAME(cockpitUpdateFunc));

				MemUtils::injectFunctionCall(cockpitLoadMeshCall, PtrToUlong(cockpitLoadMeshFunc), VAR_NAME(cockpitLoadMeshFunc));

				cockpitPatched = true;
			}
		}

		OutputGP4PPDebugString("External Steering Wheels patch : " + string(externalPatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Cockpit Steering Wheels and Dashes patch : " + string(cockpitPatched ? "Applied" : "Not applied"));
	}
};
