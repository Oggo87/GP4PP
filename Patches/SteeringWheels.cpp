#include "SteeringWheels.h"
#include "General.h"
#include "../Utils/Helpers.h"
#include "../Utils/D3DTypes.h"
#include "../Utils/Tweaker.h"
#include "../Utils/BGRA.h"
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
	bool externalLcd = false;				// LCD on the viewed car's 3D external wheel
	AssetSettings lcdSettings = { "CockpitLCD", "Cockpit LCD", "{teamname}\\{language}.pos", "{teamname}_car{car}\\{language}.pos", false, false, true, "", false };
	bool lcdPerWheel = false;
	bool alwaysShowLcd = false;
	BGRA lcdColour = { 0, 0, 0, 0 };		// GP4's own: black
	float lcdScale = 1.0f;					// multiplier on GP4's LCD text scale
	const int GP4_LCD_FONT_SIZE = 16;
	const int GP4_LCD_FONT_WEIGHT = 700;
	const int GP4_MAX_LCD_FONT_SIZE = 23;	// without the glyph page fix
	const int MAX_LCD_FONT_SIZE = 32;		// with it: 4 pages of (256 / size)^2 glyphs, 8x cells within the 256 px DIB
	int lcdFontSize = GP4_LCD_FONT_SIZE;
	int lcdFontWeight = GP4_LCD_FONT_WEIGHT;
	bool lcdIndividual = false;
	IniFile lcdIni;							// per-car sections, read once the car count is known
	vector<DWORD> lcdCarColours;
	vector<float> lcdCarScales;
	bool lcdColourPatched = false;
	bool lcdScalePatched = false;
	bool lcdFontPatched = false;
	string lcdFontFile;
	string lcdFontFace;
	bool lcdCustomFont = false;				// FontFile registered and FontFace found
	bool glyphPagePatched = false;

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
	DWORD lcdDrawCall = 0x0049cfa6;
	DWORD lcdColourCall = 0x0047ac52;		// CGP4LCD::Draw: call SetFontColour?(0, 0)
	DWORD lcdScaleCall = 0x0047ac3a;		// CGP4LCD::Draw: call SetFontScale?(0, 4.8f, 0)

	// Original bytes of the call instructions above, checked before patching
	BYTE initCarSteeringWheelsBytes[] = { 0xe8, 0x9d, 0xda, 0xff, 0xff };
	BYTE renderCarSteeringWheelsBytes[] = { 0xe8, 0x41, 0xfa, 0xfe, 0xff };
	BYTE individualWheelFlushBytes[] = { 0xe8, 0xfe, 0x09, 0xfc, 0xff };
	BYTE cockpitLoadMeshBytes[] = { 0xe8, 0xfa, 0x9a, 0xfd, 0xff };
	BYTE cockpitUpdateBytes[] = { 0xe8, 0xc5, 0x35, 0xfd, 0xff };
	BYTE cockpitDestructorBytes[] = { 0xe8, 0xc7, 0x74, 0xfd, 0xff };
	BYTE lcdDrawBytes[] = { 0xe8, 0x65, 0xdc, 0xfd, 0xff };
	BYTE lcdColourBytes[] = { 0xe8, 0x49, 0xcc, 0xff, 0xff };
	BYTE lcdScaleBytes[] = { 0xe8, 0x41, 0xcd, 0xff, 0xff };

	// LanguageData::CreateFonts: the LCD font (fonts_0x0[0]) calls, bitmap branch and the 6 TrueType language cases
	DWORD lcdBitmapFontCall = 0x00476602;	// CGP4Font::CGP4Font(device, "digital.tga", 700, 0xff000000)
	DWORD lcdTrueTypeFontCalls[] = { 0x00476862, 0x00476aee, 0x00476dda, 0x004770c2, 0x00477389, 0x0047764f };	// CGP4Font::Create(16, fonts, "dummy", "Arial", 'C', 700, 700, 0xff000000)

	// CGP4Font::Create: BuildGlyphPage?'s charCount is 0xff instead of glyphsPerPage, so page 0 takes chars 0..254 and,
	// from about size 24, writes past its texture (Ghidra notes B4). Pushing glyphsPerPage (already in edx) fixes it up to size 32
	DWORD glyphPageCharCountPush = 0x004725a0;
	BYTE glyphPageCharCountBytes[] = { 0x68, 0xff, 0x00, 0x00, 0x00 };		// push 0xff
	BYTE glyphPageCharCountPatch[] = { 0x52, 0x90, 0x90, 0x90, 0x90 };		// push edx

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
	DWORD lcdPointer = 0x00a53358;				// cgp4Lcd, CGP4LCD*
	DWORD currentCamera = 0x0066d7a4;			// byte CurrentCamera, CAMERA_COCKPIT in the cockpit
	DWORD cockpitCarData = 0x00679d2c;			// g_cockpitCar, car physics data the LCD is filled from
	DWORD vduLiveViewActive = 0x00a53ad0;		// dword, the LCD is filled from g_viewedCar instead
	DWORD lcdForceRefresh = 0x00893cfa;			// byte, every LCD field is rewritten
	DWORD tweakerSteerPosTable = 0x4005df64;	// GP4Tweak.dll, x, y, z of the flat external wheel per driver, used by its SteerPos patch instead of the team offsets

	// Function Addresses
	DWORD InitCarSteeringWheels = 0x0048b2c0;
	DWORD RenderCarSteeringWheels = 0x0048b390;
	DWORD DeviceFlushState = 0x0044d440;
	DWORD CockpitLoadMesh = 0x0046f850;
	DWORD CockpitUpdate = 0x00470910;
	DWORD CockpitDestructor = 0x0046f540;
	DWORD LcdDraw = 0x0047ac10;
	DWORD LcdUpdate = 0x0047acb0;			// CGP4LCD::Update, thiscall
	DWORD UpdateDashData = 0x00514ab5;		// registers in, ESI changed, no stack arguments
	DWORD StringConstruct = 0x00471ce0;		// std::basic_string(const char*, const allocator&), MSVC6
	DWORD StringDestruct = 0x00471da0;		// std::~basic_string
	DWORD PosObjectLoad = 0x0047e740;		// GP4ScreenPosObject::Load(basic_string* posPath), base fields only
	DWORD SetFontColour = 0x004778a0;		// LanguageData::SetFontColour?(int font, D3DCOLOR colour)
	DWORD SetFontScale = 0x00477980;		// LanguageData::SetFontScale?(int font, float scaleY, float scaleX), scaleX 0 = scaleY
	DWORD FontCreate = 0x00472100;			// CGP4Font::Create, TrueType font, RET 0x24
	DWORD FontBitmapConstruct = 0x004754c0;	// CGP4Font::CGP4Font, .tga font, RET 0x10
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
	typedef void(__thiscall* LcdDrawType)(BYTE*);
	typedef void(__thiscall* LcdUpdateType)(BYTE*);
	typedef BYTE*(__thiscall* StringConstructType)(BYTE*, const char*, const BYTE*);
	typedef void(__thiscall* StringDestructType)(BYTE*);
	typedef bool(__thiscall* PosObjectLoadType)(BYTE*, BYTE*);
	typedef void(__thiscall* SetFontColourType)(BYTE*, int, DWORD);
	typedef void(__thiscall* SetFontScaleType)(BYTE*, int, float, float);
	typedef BYTE*(__thiscall* FontCreateType)(BYTE*, BYTE*, int, BYTE*, BYTE*, BYTE*, int, int, int, DWORD);
	typedef BYTE*(__thiscall* FontBitmapConstructType)(BYTE*, BYTE*, const char*, int, DWORD);
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
	typedef void(__thiscall* MeshSetTransformType)(BYTE*, const D3DMATRIX*, int);
	typedef void(__thiscall* MeshUpdateTransformsType)(BYTE*, int, char);
	typedef void(__thiscall* CockpitSetMeshTransformsType)(BYTE*, BYTE*, BYTE*);
	typedef void(__thiscall* MeshDrawType)(BYTE*, int, void*, char);
	typedef int(__thiscall* DeviceGetRenderStateType)(BYTE*, DWORD, DWORD*);
	typedef int(__thiscall* DeviceQueueRenderStateType)(BYTE*, DWORD, DWORD);

	// Function Pointers
	InitCarSteeringWheelsType InitCarSteeringWheelsFunc = (InitCarSteeringWheelsType)InitCarSteeringWheels;
	RenderCarSteeringWheelsType RenderCarSteeringWheelsFunc = (RenderCarSteeringWheelsType)RenderCarSteeringWheels;
	DeviceFlushStateType DeviceFlushStateFunc = (DeviceFlushStateType)DeviceFlushState;
	CockpitLoadMeshType CockpitLoadMeshFunc = (CockpitLoadMeshType)CockpitLoadMesh;
	CockpitUpdateType CockpitUpdateFunc = (CockpitUpdateType)CockpitUpdate;
	CockpitDestructorType CockpitDestructorFunc = (CockpitDestructorType)CockpitDestructor;
	LcdDrawType LcdDrawFunc = (LcdDrawType)LcdDraw;
	LcdUpdateType LcdUpdateFunc = (LcdUpdateType)LcdUpdate;
	StringConstructType StringConstructFunc = (StringConstructType)StringConstruct;
	StringDestructType StringDestructFunc = (StringDestructType)StringDestruct;
	PosObjectLoadType PosObjectLoadFunc = (PosObjectLoadType)PosObjectLoad;
	SetFontColourType SetFontColourFunc = (SetFontColourType)SetFontColour;
	SetFontScaleType SetFontScaleFunc = (SetFontScaleType)SetFontScale;
	FontCreateType FontCreateFunc = (FontCreateType)FontCreate;
	FontBitmapConstructType FontBitmapConstructFunc = (FontBitmapConstructType)FontBitmapConstruct;
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
	const DWORD CAR_DRIVER_INDEX = 0x1c;
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

	const int TWEAKER_STEER_POS_DRIVERS = 22;	// Tweaker caps the driver index (low 6 bits) at 21

	const DWORD DEVICE_MANAGER = 0x04;
	const DWORD DEVICE_D3D_DEVICE = 0x08;
	const DWORD DEVICE_WORLD_MATRIX = 0x2018;
	const DWORD DEVICE_CURRENT_SHADER = 0x24a0;
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
	const DWORD LCD_MESH = 0x00;				// CGP4LCD mesh_0x0, the "LCD" object of this mesh places the text
	const BYTE CAMERA_COCKPIT = 0;
	const DWORD CAR_DATA_RACE_FLAGS = 0x083;	// CarDynamicData flagsRaceFlags_0x083, bit 5 makes UpdateDashData reset the dash
	const BYTE RACE_FLAG_DASH_RESET = 0x20;
	const DWORD MESH_FILE_NAME = 0xa0;			// CGP4Mesh fileName_0xa0, char[128]
	const DWORD LCD_LANGUAGE = 0x08;			// CGP4LCD LanguageData*
	const DWORD LCD_GROUPS = 0x19c;				// CGP4LCD GP4ScreenObjectManager*[40]
	const int LCD_NUM_GROUPS = 40;
	const DWORD GROUP_OBJECTS = 0x0c;			// GP4ScreenObjectManager GP4ScreenObject*[20]
	const DWORD GROUP_COUNT = 0x5c;
	const int GROUP_MAX_OBJECTS = 20;
	const DWORD OBJECT_POS_X = 0xb8;			// GP4ScreenObject posX_0xb8 / posY_0xbc, from the .pos file
	const DWORD OBJECT_POS_Y = 0xbc;
	const DWORD LANGUAGE_SELECTED = 0x10;		// LanguageData selectedLanguage_0x10
	const DWORD LANGUAGE_NAMES = 0x24;			// LanguageData basic_string[6]
	const DWORD LANGUAGE_FONTS_FOLDER = 0x14;	// LanguageData basic_string fontsFolder_0x14, "fonts"
	const int NUM_LANGUAGES = 6;
	const DWORD STRING_SIZE = 0x10;				// MSVC6 std::basic_string
	const DWORD STRING_PTR = 0x04;
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
		bool loaded = false;	// false when the file was not found or failed to load
		BYTE* mesh = nullptr;
		DWORD meshIndex = -1;
		string fileName = "";
	};

	// Position of one LCD text or value object, read from a steering wheel's own .pos file
	struct LcdPosition
	{
		float x;
		float y;
		bool set;			// false: GP4's position is kept
	};

	// One LCD positions file, shared by every car whose file name resolves to it
	struct LcdLayoutFile
	{
		string fileName;
		vector<LcdPosition> positions;	// empty: not found, or nothing moved
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
		vector<int> statics = {};
		int revOn[2][NUM_REV_LIGHTS] = {};		// left, right
		int flagOn[2][NUM_FLAG_LIGHTS] = {};		// left, right
		int gears[NUM_GEARS] = {};
		vector<pair<int, int>> displays = {};		// cockpit slot and mesh object of each per-car display object found
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
	float (*tweakerSteerPos)[3] = NULL;	// set on the first draw when Tweaker's SteerPos is enabled, Tweaker loads after ApplyPatches
	bool tweakerSteerPosChecked = false;
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
	vector<LcdLayoutFile> lcdLayoutFiles;
	vector<int> carLcdLayout;			// index into lcdLayoutFiles per car, or VANILLA for GP4's positions
	bool lcdLayoutsLoaded = false;		// LCD positions resolved for this session
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
		lcdLayoutFiles.clear();
		carLcdLayout.clear();
		lcdLayoutsLoaded = false;
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

		CockpitSet set = {};
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

	// Every LCD text and value object, in the order GP4 draws them
	vector<BYTE*> lcdObjects(BYTE* lcd)
	{
		vector<BYTE*> objects;

		for (int group = 0; group < LCD_NUM_GROUPS; group++)
		{
			BYTE* manager = field<BYTE*>(lcd, LCD_GROUPS + group * 4);

			if (manager == NULL)
			{
				continue;
			}

			int count = min(field<int>(manager, GROUP_COUNT), GROUP_MAX_OBJECTS);

			for (int i = 0; i < count; i++)
			{
				BYTE* object = field<BYTE*>(manager, GROUP_OBJECTS + i * 4);

				if (object != NULL)
				{
					objects.push_back(object);
				}
			}
		}

		return objects;
	}

	string lcdLanguageName(BYTE* lcd)
	{
		BYTE* language = field<BYTE*>(lcd, LCD_LANGUAGE);

		if (language == NULL)
		{
			return "";
		}

		int selected = field<int>(language, LANGUAGE_SELECTED);

		if (selected < 0 || selected >= NUM_LANGUAGES)
		{
			return "";
		}

		const char* name = field<const char*>(language, LANGUAGE_NAMES + selected * STRING_SIZE + STRING_PTR);

		return name != NULL ? name : "";
	}

	string lcdFontsFolder(BYTE* lcd)
	{
		BYTE* language = field<BYTE*>(lcd, LCD_LANGUAGE);
		const char* folder = language != NULL ? field<const char*>(language, LANGUAGE_FONTS_FOLDER + STRING_PTR) : NULL;

		return folder != NULL ? folder : "";
	}

	// Reads an LCD positions file with GP4's loader, keeping GP4's positions in the LCD objects
	vector<LcdPosition> loadLcdLayout(BYTE* lcd, const string& posFile)
	{
		vector<LcdPosition> positions;

		// GP4's loader opens the file with fopen: only files on disk, never in cars.wad
		if (GetFileAttributesA(posFile.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			return positions;
		}

		BYTE path[STRING_SIZE];
		BYTE allocator = 0;
		int moved = 0;

		StringConstructFunc(path, posFile.c_str(), &allocator);

		for (BYTE* object : lcdObjects(lcd))
		{
			float x = field<float>(object, OBJECT_POS_X);
			float y = field<float>(object, OBJECT_POS_Y);
			LcdPosition position = { x, y, false };

			if (PosObjectLoadFunc(object, path))
			{
				position.x = field<float>(object, OBJECT_POS_X);
				position.y = field<float>(object, OBJECT_POS_Y);

				// GP4 gives 0, 0 for a key that is not in the file
				position.set = position.x != 0.0f || position.y != 0.0f;
				moved += position.set ? 1 : 0;
			}

			field<float>(object, OBJECT_POS_X) = x;
			field<float>(object, OBJECT_POS_Y) = y;

			positions.push_back(position);
		}

		StringDestructFunc(path);

		OutputGP4PPDebugString("Steering Wheels: LCD positions " + posFile + ": " + to_string(moved) + " of " + to_string(positions.size()) + " moved");

		if (moved == 0)
		{
			positions.clear();
		}

		return positions;
	}

	// Resolves every car's LCD positions file on the first LCD draw of the session, when the LCD objects exist
	void loadLcdLayouts(BYTE* lcd)
	{
		lcdLayoutsLoaded = true;
		carLcdLayout.assign(numCars, VANILLA);

		for (const CockpitMesh& entry : cockpitWheelMeshes)
		{
			if (entry.loaded && entry.mesh != NULL)
			{
				bool hasLcd = GetObjectIndexByNameFunc(entry.mesh, "LCD") != 0xffffffff;

				OutputGP4PPDebugString("Steering Wheels: " + entry.fileName
					+ (hasLcd ? " has an LCD object" : (alwaysShowLcd ? " has no LCD object, LCD forced (AlwaysShow)" : " has no LCD object, LCD not shown")));
			}
		}

		if (!lcdSettings.enabled)
		{
			return;
		}

		string language = lcdLanguageName(lcd);
		string fontsFolder = lcdFontsFolder(lcd);

		if (language.empty() || fontsFolder.empty())
		{
			OutputGP4PPDebugString("Steering Wheels: GP4 language or fonts folder not readable, using default LCD positions");

			return;
		}

		map<string, string> variables;
		variables["language"] = language;

		for (int carIndex = 0; carIndex < numCars; carIndex++)
		{
			if (!validTeam(carSlot(carIndex)))
			{
				continue;
			}

			// Positions made for a custom steering wheel are only used by the cars that have it
			if (lcdPerWheel && lcdSettings.autoName)
			{
				int set = carIndex < (int)carCockpitSet.size() ? carCockpitSet[carIndex] : VANILLA;

				if (set == VANILLA || cockpitSets[set].wheel == VANILLA)
				{
					OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " has the default steering wheel, using default LCD positions");

					continue;
				}
			}

			string fileName = fontsFolder + "\\" + replaceVariables(carFileName(lcdSettings.fileName, carIndex), variables);
			int index = VANILLA;

			for (size_t i = 0; i < lcdLayoutFiles.size(); i++)
			{
				if (_stricmp(lcdLayoutFiles[i].fileName.c_str(), fileName.c_str()) == 0)
				{
					index = (int)i;
				}
			}

			bool shared = index != VANILLA;

			if (!shared)
			{
				lcdLayoutFiles.push_back({ fileName, loadLcdLayout(lcd, fileName) });
				index = (int)lcdLayoutFiles.size() - 1;
			}

			bool found = !lcdLayoutFiles[index].positions.empty();

			OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " LCD positions "
				+ (found ? (shared ? "shared" : "loaded") : "not found, using default") + " [" + fileName + "]");

			carLcdLayout[carIndex] = found ? index : VANILLA;
		}
	}

	// Swaps the wheel's positions with the ones in the LCD objects: called once before and once after the draw
	void swapLcdLayout(BYTE* lcd, vector<LcdPosition>& positions)
	{
		vector<BYTE*> objects = lcdObjects(lcd);

		if (objects.size() != positions.size())
		{
			return;
		}

		for (size_t i = 0; i < objects.size(); i++)
		{
			if (positions[i].set)
			{
				swap(field<float>(objects[i], OBJECT_POS_X), positions[i].x);
				swap(field<float>(objects[i], OBJECT_POS_Y), positions[i].y);
			}
		}
	}

	// LCD colours are BGR: GP4's font ignores alpha, so a fourth value is dropped
	BGRA lcdColourFromIni(const vector<short>& values)
	{
		BGRA colour = values;

		return BGRA(colour.getB(), colour.getG(), colour.getR(), 0);
	}

	// Replaces CGP4LCD::Draw's SetFontColour?(font, 0) call: thiscall with 2 stack arguments, matched by __fastcall
	void __fastcall lcdSetFontColourFunc(BYTE* language, void* /*edx*/, int font, DWORD /*colour*/)
	{
		int car = viewedCar();
		DWORD colour = (car >= 0 && car < (int)lcdCarColours.size()) ? lcdCarColours[car] : lcdColour.getColour();

		SetFontColourFunc(language, font, colour);
	}

	// Replaces CGP4LCD::Draw's SetFontScale?(font, 4.8f, 0) call: thiscall with 3 stack arguments, matched by __fastcall
	void __fastcall lcdSetFontScaleFunc(BYTE* language, void* /*edx*/, int font, float scaleY, float scaleX)
	{
		int car = viewedCar();
		float scale = (car >= 0 && car < (int)lcdCarScales.size()) ? lcdCarScales[car] : lcdScale;

		// GP4's scale divides the text size: a larger Scale is a smaller value. scaleX 0 (same as scaleY) stays 0
		SetFontScaleFunc(language, font, scaleY / scale, scaleX / scale);
	}

	// A 5-byte call at callAddress whose target is function
	bool checkCallTarget(DWORD callAddress, DWORD function, const string& name)
	{
		BYTE* call = MemUtils::addressToPtr<BYTE>(callAddress);

		if (call[0] == 0xe8 && callAddress + 5 + *(DWORD*)(call + 1) == function)
		{
			return true;
		}

		OutputGP4PPDebugString("Steering Wheels: unexpected bytes at " + name + ", not patched");

		return false;
	}

	// Registers FontFile for GP4 only, and checks Windows finds FontFace in it
	bool registerLcdFont()
	{
		// GP4's fonts folder, next to GP4.exe: GP4PP.dll and its ini can live elsewhere
		char exePath[MAX_PATH] = "";

		GetModuleFileNameA(NULL, exePath, MAX_PATH);

		string gp4Folder = exePath;
		gp4Folder = gp4Folder.substr(0, gp4Folder.find_last_of("\\/") + 1);

		string path = gp4Folder + "fonts\\" + lcdFontFile;

		if (AddFontResourceExA(path.c_str(), FR_PRIVATE, 0) == 0)
		{
			OutputGP4PPDebugString("Steering Wheels: LCD font not loaded [" + path + "]");

			return false;
		}

		// Windows silently substitutes another font for an unknown face: compare the face it picks
		HDC dc = CreateCompatibleDC(NULL);
		HFONT font = CreateFontA(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, lcdFontFace.c_str());
		HGDIOBJ old = SelectObject(dc, font);
		char face[LF_FACESIZE] = "";

		GetTextFaceA(dc, LF_FACESIZE, face);
		SelectObject(dc, old);
		DeleteObject(font);
		DeleteDC(dc);

		bool found = _stricmp(face, lcdFontFace.c_str()) == 0;

		OutputGP4PPDebugString("Steering Wheels: LCD font " + (found ? string("loaded") : "face not found, Windows uses " + string(face)) + " [" + path + ", " + lcdFontFace + "]");

		if (!found)
		{
			RemoveFontResourceExA(path.c_str(), FR_PRIVATE, 0);
		}

		return found;
	}

	BYTE* createLcdTrueTypeFont(BYTE* font, BYTE* device, BYTE* folder, BYTE* fileName, BYTE* faceName, int unusedLetter, int maxChars, DWORD colour)
	{
		BYTE allocator = 0;
		BYTE customFace[STRING_SIZE];

		if (lcdCustomFont)
		{
			StringConstructFunc(customFace, lcdFontFace.c_str(), &allocator);
		}

		BYTE* result = FontCreateFunc(font, device, lcdFontSize, folder, fileName, lcdCustomFont ? customFace : faceName, unusedLetter, maxChars, lcdFontWeight, colour);

		if (lcdCustomFont)
		{
			StringDestructFunc(customFace);
		}

		return result;
	}

	// Replaces the 6 TrueType LCD font calls: thiscall with 9 stack arguments, matched by __fastcall
	BYTE* __fastcall lcdTrueTypeFontFunc(BYTE* font, void* /*edx*/, BYTE* device, int /*fontSize*/, BYTE* folder, BYTE* fileName, BYTE* faceName, int unusedLetter, int maxChars, int /*weight*/, DWORD colour)
	{
		return createLcdTrueTypeFont(font, device, folder, fileName, faceName, unusedLetter, maxChars, colour);
	}

	// Replaces the bitmap LCD font call (GP4's TrueType fonts off): thiscall with 4 stack arguments, matched by __fastcall.
	// Both fonts are built in the same HeapAlloc(0xb30) block, so the custom TrueType font takes the bitmap one's place
	BYTE* __fastcall lcdBitmapFontFunc(BYTE* font, void* /*edx*/, BYTE* device, const char* textureName, int maxChars, DWORD colour)
	{
		if (!lcdCustomFont)
		{
			return FontBitmapConstructFunc(font, device, textureName, maxChars, colour);
		}

		// Same folder and file name GP4's TrueType branch passes: there is no fonts\dummy.ttf, so Create registers nothing itself
		BYTE allocator = 0;
		BYTE folder[STRING_SIZE];
		BYTE fileName[STRING_SIZE];

		StringConstructFunc(folder, "fonts", &allocator);
		StringConstructFunc(fileName, "dummy", &allocator);

		BYTE* result = createLcdTrueTypeFont(font, device, folder, fileName, NULL, 'C', maxChars, colour);

		StringDestructFunc(fileName);
		StringDestructFunc(folder);

		return result;
	}

	void __fastcall lcdDrawFunc(BYTE* lcd, void* /*edx*/)
	{
		BYTE* cockpit = MemUtils::addressToValue<BYTE*>(cockpitPointer);
		BYTE* lcdMesh = lcd != NULL ? field<BYTE*>(lcd, LCD_MESH) : NULL;
		BYTE* wheelMesh = (cockpit != NULL && cockpit == cockpitObject) ? field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) : NULL;
		bool custom = wheelMesh != NULL && lcdMesh != NULL && wheelMesh != lcdMesh;
		bool hasLcd = custom && GetObjectIndexByNameFunc(wheelMesh, "LCD") != 0xffffffff;

		vector<LcdPosition>* layout = NULL;

		if (!lcdLayoutsLoaded && cockpit != NULL && cockpit == cockpitObject)
		{
			loadLcdLayouts(lcd);
		}

		int car = viewedCar();

		if (car >= 0 && car < (int)carLcdLayout.size() && carLcdLayout[car] != VANILLA)
		{
			layout = &lcdLayoutFiles[carLcdLayout[car]].positions;
		}

		if (hasLcd)
		{
			// GP4 places the text with the LCD object of the wheel it was built with: use the drawn wheel's own
			field<BYTE*>(lcd, LCD_MESH) = wheelMesh;
		}
		else if (custom && alwaysShowLcd)
		{
			// The default wheel is not placed while another wheel is drawn: put it where the drawn wheel is
			field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) = lcdMesh;
			CockpitSetMeshTransformsFunc(cockpit, cockpit + COCKPIT_ROTATION_MATRIX, cockpit + COCKPIT_WORLD_MATRIX);
			field<BYTE*>(cockpit, COCKPIT_WHEEL_MESH) = wheelMesh;
		}

		if (layout != NULL)
		{
			swapLcdLayout(lcd, *layout);
		}

		// original function call
		LcdDrawFunc(lcd);

		if (layout != NULL)
		{
			swapLcdLayout(lcd, *layout);
		}

		// The LCD keeps the mesh it was built with, ours are freed with the cockpit
		if (hasLcd)
		{
			field<BYTE*>(lcd, LCD_MESH) = lcdMesh;
		}
	}

	// UpdateDashData keeps the car in ESI and runs its writers inside PUSHAD ... POPAD
	void updateDashData()
	{
		__asm pushad
		__asm call dword ptr[UpdateDashData]
		__asm popad
	}

	// GP4 only fills and draws the LCD in the cockpit view: do both for the viewed car's 3D external wheel, as if it were the cockpit car
	void drawExternalLcd(BYTE* device, BYTE* mesh, const D3DMATRIX& world, int carIndex)
	{
		BYTE* lcd = MemUtils::addressToValue<BYTE*>(lcdPointer);
		BYTE* data = carDynamicData.empty() ? NULL : carDynamicData[carIndex];
		BYTE* cockpit = MemUtils::addressToValue<BYTE*>(cockpitPointer);
		BYTE* lcdMesh = lcd != NULL ? field<BYTE*>(lcd, LCD_MESH) : NULL;

		if (data == NULL || lcdMesh == NULL || MemUtils::addressToValue<DWORD>(vduLiveViewActive) != 0)
		{
			return;
		}

		BYTE* textMesh = mesh;

		if (GetObjectIndexByNameFunc(mesh, "LCD") == 0xffffffff)
		{
			if (!alwaysShowLcd || lcdMesh == mesh || GetObjectIndexByNameFunc(lcdMesh, "LCD") == 0xffffffff)
			{
				return;
			}

			// Same as AlwaysShow in the cockpit: the default wheel, placed where this car's wheel is, places the text
			textMesh = lcdMesh;

			MeshSetTransformFunc(textMesh, &world, -1);
			MeshUpdateTransformsFunc(textMesh, -1, 0);
		}

		if (!lcdLayoutsLoaded && cockpit != NULL && cockpit == cockpitObject)
		{
			loadLcdLayouts(lcd);
		}

		BYTE* camera = MemUtils::addressToPtr<BYTE>(currentCamera);
		BYTE** cockpitCar = MemUtils::addressToPtr<BYTE*>(cockpitCarData);
		BYTE oldCamera = *camera;
		BYTE* oldCockpitCar = *cockpitCar;
		BYTE oldRaceFlags = field<BYTE>(data, CAR_DATA_RACE_FLAGS);

		*camera = CAMERA_COCKPIT;
		*cockpitCar = data;
		*MemUtils::addressToPtr<BYTE>(lcdForceRefresh) = 1;	// what RenderGameFrame does every cockpit frame

		updateDashData();
		LcdUpdateFunc(lcd);

		vector<LcdPosition>* layout = NULL;

		if (carIndex < (int)carLcdLayout.size() && carLcdLayout[carIndex] != VANILLA)
		{
			layout = &lcdLayoutFiles[carLcdLayout[carIndex]].positions;
		}

		// The text is placed with the LCD object of the external mesh, which was just placed on the car
		field<BYTE*>(lcd, LCD_MESH) = textMesh;

		if (layout != NULL)
		{
			swapLcdLayout(lcd, *layout);
		}

		// The original Draw, the colour, scale and font hooks pick this car through viewedCar()
		LcdDrawFunc(lcd);

		if (layout != NULL)
		{
			swapLcdLayout(lcd, *layout);
		}

		field<BYTE*>(lcd, LCD_MESH) = lcdMesh;

		*camera = oldCamera;
		*cockpitCar = oldCockpitCar;
		field<BYTE>(data, CAR_DATA_RACE_FLAGS) = (field<BYTE>(data, CAR_DATA_RACE_FLAGS) & ~RACE_FLAG_DASH_RESET) | (oldRaceFlags & RACE_FLAG_DASH_RESET);

		// The font changes the device state: the next mesh sets its shader again
		field<BYTE*>(device, DEVICE_CURRENT_SHADER) = NULL;
	}

	//
	// 3D external steering wheels
	//

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

	bool tweakerSteerPosition(BYTE* car, float& x, float& y, float& z)
	{
		if (!tweakerSteerPosChecked)
		{
			tweakerSteerPosChecked = true;

			Tweaker* tweaker = Tweaker::getInstance();

			if (tweaker != NULL && tweaker->isEnabled() && tweaker->getTweakState("SteerPos"))
			{
				tweakerSteerPos = MemUtils::addressToPtr<float[3]>(tweakerSteerPosTable);
			}

			OutputGP4PPDebugString("Steering Wheels: GP4 Tweaker SteerPos " + string(tweakerSteerPos != NULL ? "enabled, used for the 3D external steering wheels" : "not enabled"));
		}

		if (tweakerSteerPos == NULL)
		{
			return false;
		}

		// Same driver lookup as Tweaker
		int driver = min((int)(field<DWORD>(car, CAR_DRIVER_INDEX) & 0x3f), TWEAKER_STEER_POS_DRIVERS - 1);

		x = tweakerSteerPos[driver][0];
		y = tweakerSteerPos[driver][1];
		z = tweakerSteerPos[driver][2];

		return true;
	}

	D3DMATRIX externalWheelMatrix(BYTE* car, int team)
	{
		// The mesh origin is the rotation point: placed at the centre of the flat wheel, turned and tilted like in the cockpit
		float offsetX = 0.0f;
		float offsetY, offsetZ;

		// Same offsets the flat wheel gets: GP4 Tweaker's per-car SteerPos when enabled, GP4's team offsets otherwise
		if (!tweakerSteerPosition(car, offsetX, offsetY, offsetZ))
		{
			offsetY = MemUtils::addressToValue<float>(stWheelTeamOffsets + team * 8);
			offsetZ = MemUtils::addressToValue<float>(stWheelTeamOffsets + team * 8 + 4);
		}

		float wheelY = MemUtils::addressToValue<float>(stWheelY);
		float wheelZ = MemUtils::addressToValue<float>(stWheelZ);
		float tilt = MemUtils::addressToValue<float>(cockpitWheelTilt);

		D3DMATRIX world = matrixMultiply(matrixScaling(cockpitMeshScale), matrixRotationZ(externalWheelAngle(car)));
		world = matrixMultiply(world, matrixRotationX(tilt));
		world = matrixMultiply(world, matrixTranslation(offsetX, offsetY + wheelY, offsetZ + wheelZ));

		return matrixMultiply(world, field<D3DMATRIX>(car, CAR_MATRIX));
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
		D3DMATRIX worldMatrix = field<D3DMATRIX>(device, DEVICE_WORLD_MATRIX);
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

			D3DMATRIX world = externalWheelMatrix(car, team);

			MeshSetTransformFunc(mesh, &world, -1);
			MeshUpdateTransformsFunc(mesh, -1, 0);

			MeshDrawType draw = *reinterpret_cast<MeshDrawType*>(field<BYTE*>(mesh, 0) + MESH_DRAW);
			draw(mesh, -1, NULL, 0);

			// Only the viewed car's wheel, and not in the cockpit, where GP4 draws the LCD itself
			if (externalLcd && carIndex == viewedCar() && MemUtils::addressToValue<BYTE>(currentCamera) != CAMERA_COCKPIT)
			{
				drawExternalLcd(device, mesh, world, carIndex);
			}

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

			field<D3DMATRIX>(device, DEVICE_WORLD_MATRIX) = worldMatrix;

			if (d3dDevice != NULL)
			{
				D3DSetTransformType setTransform = *reinterpret_cast<D3DSetTransformType*>(field<BYTE*>(d3dDevice, 0) + D3DDEVICE8_SET_TRANSFORM);
				setTransform(d3dDevice, D3DTS_WORLD, &worldMatrix);
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

	// [CockpitLCD] PerWheel: the folder of the cockpit steering wheel file name, mirrored in the fonts folder
	string perWheelLcdPattern()
	{
		if (!cockpitSettings.enabled)
		{
			return "";
		}

		const string cockpitsFolder = "cockpits\\";
		string pattern = cockpitSettings.fileName;
		size_t folderEnd = pattern.find_last_of('\\');
		string folder = folderEnd != string::npos ? pattern.substr(0, folderEnd) : "";

		// AutoName steering wheels all live in cars\cockpits\, which the LCD folders leave out
		if (cockpitSettings.autoName && _strnicmp(folder.c_str(), cockpitsFolder.c_str(), cockpitsFolder.size()) == 0)
		{
			folder = folder.substr(cockpitsFolder.size());
		}

		// No folder to mirror: the file name without its extension, so the file never ends up next to GP4's own
		if (folder.empty())
		{
			string name = folderEnd != string::npos ? pattern.substr(folderEnd + 1) : pattern;

			folder = name.substr(0, name.find_last_of('.'));
		}

		return folder + "\\{language}.pos";
	}

	void LoadSettings(IniFile iniSettings, string basePath)
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

		try
		{
			externalLcd = iniSettings[externalSettings.section]["ShowLCD"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("ShowLCD " + externalSettings.name + ": " + (externalLcd && useCockpitMesh ? "Enabled" : "Disabled"));

		loadAssetSettings(iniSettings, cockpitSettings);
		loadAssetSettings(iniSettings, dashSettings);
		loadAssetSettings(iniSettings, lcdSettings);

		try
		{
			lcdPerWheel = iniSettings[lcdSettings.section]["PerWheel"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Per Wheel " + lcdSettings.name + ": " + (lcdPerWheel ? "Enabled" : "Disabled"));

		// PerWheel supersedes PerDriver and PerTeam, mirroring [CockpitSteeringWheels]
		if (lcdSettings.autoName && lcdPerWheel)
		{
			lcdSettings.fileName = perWheelLcdPattern();
			lcdSettings.enabled = !lcdSettings.fileName.empty();

			OutputGP4PPDebugString(lcdSettings.name + ": " + (lcdSettings.enabled ? "PerWheel file name " + lcdSettings.fileName : "PerWheel needs Cockpit Steering Wheels, using default"));
		}

		try
		{
			alwaysShowLcd = iniSettings[lcdSettings.section]["AlwaysShow"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("AlwaysShow " + lcdSettings.name + ": " + (alwaysShowLcd ? "Enabled" : "Disabled"));

		try
		{
			lcdFontSize = iniSettings[lcdSettings.section]["FontSize"].getAs<int>();
		}
		catch (exception ex) {}

		// 24-32 need the glyph page fix, applied in ApplyPatches
		if (lcdFontSize < 1 || lcdFontSize > MAX_LCD_FONT_SIZE)
		{
			OutputGP4PPDebugString("Invalid " + lcdSettings.name + " FontSize " + to_string(lcdFontSize) + ", using GP4's");

			lcdFontSize = GP4_LCD_FONT_SIZE;
		}

		OutputGP4PPDebugString(lcdSettings.name + " FontSize: " + to_string(lcdFontSize));

		try
		{
			lcdFontWeight = iniSettings[lcdSettings.section]["FontWeight"].getAs<int>();
		}
		catch (exception ex) {}

		if (lcdFontWeight < 1 || lcdFontWeight > 1000)
		{
			OutputGP4PPDebugString("Invalid " + lcdSettings.name + " FontWeight " + to_string(lcdFontWeight) + ", using GP4's");

			lcdFontWeight = GP4_LCD_FONT_WEIGHT;
		}

		OutputGP4PPDebugString(lcdSettings.name + " FontWeight: " + to_string(lcdFontWeight));

		try
		{
			lcdFontFile = iniSettings[lcdSettings.section]["FontFile"].getString();
		}
		catch (exception ex) {}

		try
		{
			lcdFontFace = iniSettings[lcdSettings.section]["FontFace"].getString();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString(lcdSettings.name + " Font: " + (lcdFontFile.empty() || lcdFontFace.empty() ? string("GP4's") : lcdFontFile + ", " + lcdFontFace));

		lcdCustomFont = !lcdFontFile.empty() && !lcdFontFace.empty() && registerLcdFont();

		try
		{
			lcdScale = iniSettings[lcdSettings.section]["Scale"].getAs<float>();
		}
		catch (exception ex) {}

		if (!(lcdScale > 0.0f))
		{
			OutputGP4PPDebugString("Invalid " + lcdSettings.name + " Scale " + to_string(lcdScale) + ", using default");

			lcdScale = 1.0f;
		}

		OutputGP4PPDebugString(lcdSettings.name + " Scale: " + to_string(lcdScale));

		try
		{
			lcdColour =lcdColourFromIni(iniSettings[lcdSettings.section]["Colour"].getVectorAs<short>());
		}
		catch (exception ex) {}

		OutputGP4PPDebugString(lcdSettings.name + " Colour: B: " + to_string(lcdColour.getB()) + " G: " + to_string(lcdColour.getG()) + " R: " + to_string(lcdColour.getR()));

		try
		{
			lcdIndividual = iniSettings[lcdSettings.section]["Individual"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual " + lcdSettings.name + ": " + (lcdIndividual ? "Enabled" : "Disabled"));

		if (lcdIndividual)
		{
			string iniFileName = "LCD.ini";

			try
			{
				iniFileName = iniSettings[lcdSettings.section]["Ini"].getString();
			}
			catch (exception ex) {}

			lcdIndividual = lcdIni.load(basePath + iniFileName);

			OutputGP4PPDebugString((lcdIndividual ? "Loaded " : "Failed to open ") + lcdSettings.name + " INI: " + iniFileName);
		}
	}

	void ApplyPatches()
	{
		bool lcdColourEnabled = lcdIndividual || lcdColour.getColour() != 0;
		bool lcdScaleEnabled = lcdIndividual || lcdScale != 1.0f;
		bool cockpitEnabled = cockpitSettings.enabled || dashSettings.enabled || lcdSettings.enabled || lcdColourEnabled || lcdScaleEnabled;
		bool externalEnabled = externalSettings.enabled || useCockpitMesh;
		bool countsRead = (externalEnabled || cockpitEnabled) && readCarCounts();

		// LCD font: GP4 creates it after this, in D3D_Init and on every language change
		if (lcdCustomFont || lcdFontSize != GP4_LCD_FONT_SIZE || lcdFontWeight != GP4_LCD_FONT_WEIGHT)
		{
			// Sizes GP4 cannot build without overflowing its glyph texture
			if (lcdFontSize > GP4_MAX_LCD_FONT_SIZE)
			{
				if (memcmp(MemUtils::addressToPtr<BYTE>(glyphPageCharCountPush), glyphPageCharCountBytes, sizeof(glyphPageCharCountBytes)) == 0)
				{
					MemUtils::patchAddress((LPVOID)glyphPageCharCountPush, glyphPageCharCountPatch, sizeof(glyphPageCharCountPatch));

					glyphPagePatched = true;
				}
				else
				{
					OutputGP4PPDebugString("Steering Wheels: unexpected bytes in CGP4Font::Create, LCD FontSize " + to_string(lcdFontSize) + " limited to " + to_string(GP4_MAX_LCD_FONT_SIZE));

					lcdFontSize = GP4_MAX_LCD_FONT_SIZE;
				}
			}

			bool callsFound = !lcdCustomFont || checkCallTarget(lcdBitmapFontCall, FontBitmapConstruct, "CGP4Font::CGP4Font call");

			for (int i = 0; callsFound && i < 6; i++)
			{
				callsFound = checkCallTarget(lcdTrueTypeFontCalls[i], FontCreate, "CGP4Font::Create call");
			}

			if (callsFound)
			{
				if (lcdCustomFont)
				{
					MemUtils::injectFunctionCall(lcdBitmapFontCall, PtrToUlong(lcdBitmapFontFunc), VAR_NAME(lcdBitmapFontFunc));
				}

				for (int i = 0; i < 6; i++)
				{
					MemUtils::injectFunctionCall(lcdTrueTypeFontCalls[i], PtrToUlong(lcdTrueTypeFontFunc), VAR_NAME(lcdTrueTypeFontFunc));
				}

				lcdFontPatched = true;
			}
		}

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
				&& checkCallBytes(cockpitDestructorCall, cockpitDestructorBytes, "CGP4Cockpit destructor")
				&& checkCallBytes(lcdDrawCall, lcdDrawBytes, "CGP4LCD::Draw"))
			{
				resetCockpitState();

				// Destructor first, so that a cockpit with our meshes can never be destroyed without clean-up
				MemUtils::injectFunctionCall(cockpitDestructorCall, PtrToUlong(cockpitDestructorFunc), VAR_NAME(cockpitDestructorFunc));

				MemUtils::injectFunctionCall(cockpitUpdateCall, PtrToUlong(cockpitUpdateFunc), VAR_NAME(cockpitUpdateFunc));

				MemUtils::injectFunctionCall(cockpitLoadMeshCall, PtrToUlong(cockpitLoadMeshFunc), VAR_NAME(cockpitLoadMeshFunc));

				MemUtils::injectFunctionCall(lcdDrawCall, PtrToUlong(lcdDrawFunc), VAR_NAME(lcdDrawFunc));

				bool colourHook = lcdColourEnabled && checkCallBytes(lcdColourCall, lcdColourBytes, "CGP4LCD::Draw SetFontColour");
				bool scaleHook = lcdScaleEnabled && checkCallBytes(lcdScaleCall, lcdScaleBytes, "CGP4LCD::Draw SetFontScale");

				// Every car starts from the defaults, the individual entries override them
				for (int carIndex = 0; lcdIndividual && (colourHook || scaleHook) && carIndex < numCars; carIndex++)
				{
					string section = lcdSettings.section + to_string(carIndex + 1);
					BGRA colour = lcdColour;
					float scale = lcdScale;

					try
					{
						colour = lcdColourFromIni(lcdIni[section]["Colour"].getVectorAs<short>());
					}
					catch (exception ex) {}

					try
					{
						scale = lcdIni[section]["Scale"].getAs<float>();
					}
					catch (exception ex) {}

					if (!(scale > 0.0f))
					{
						scale = lcdScale;
					}

					lcdCarColours.push_back(colour.getColour());
					lcdCarScales.push_back(scale);

					OutputGP4PPDebugString("Steering Wheels: " + carName(carIndex) + " LCD colour " + to_string(colour.getB()) + ", " + to_string(colour.getG()) + ", " + to_string(colour.getR()) + ", scale " + to_string(scale));
				}

				if (colourHook)
				{
					MemUtils::injectFunctionCall(lcdColourCall, PtrToUlong(lcdSetFontColourFunc), VAR_NAME(lcdSetFontColourFunc));

					lcdColourPatched = true;
				}

				if (scaleHook)
				{
					MemUtils::injectFunctionCall(lcdScaleCall, PtrToUlong(lcdSetFontScaleFunc), VAR_NAME(lcdSetFontScaleFunc));

					lcdScalePatched = true;
				}

				cockpitPatched = true;
			}
		}

		OutputGP4PPDebugString("External Steering Wheels patch : " + string(externalPatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Cockpit Steering Wheels and Dashes patch : " + string(cockpitPatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Cockpit LCD colour patch : " + string(lcdColourPatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Cockpit LCD scale patch : " + string(lcdScalePatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Cockpit LCD font patch : " + string(lcdFontPatched ? "Applied" : "Not applied"));
		OutputGP4PPDebugString("Font glyph page fix : " + string(glyphPagePatched ? "Applied" : "Not applied"));
	}
};
