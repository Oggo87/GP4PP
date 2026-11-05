#include "SteeringWheels.h"
#include "../Utils/Helpers.h"
#include "../GP4MemLib/GP4MemLib.h"
#include "../IniLib/IniLib.h"
#include <algorithm>
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

	// Constants
	const int NUM_COCKPIT_OBJECTS = 0x126;
	const int VANILLA = -1;

	// Car and team counts, read from GP4's code by readCarCounts()
	int numCars = 0;
	int numTeams = 0;

	// Compare instruction closing the car creation loop of D3D_LoadCars: cmp ebx, <number of cars>
	DWORD carLoopCompare = 0x0048d80f;
	BYTE carLoopCompareBytes[] = { 0x83, 0xfb };

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

	// Structure offsets
	const DWORD CAR_TEAM_INDEX = 0x20;
	const DWORD CAR_TEAM_CAR = 0x24;			// 0 or 1, first or second car of the team
	const DWORD CAR_WHEEL_TEXTURE = 0xd0;

	const DWORD DEVICE_MANAGER = 0x04;
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

	// External wheels storage variables
	bool externalPatched = false;
	vector<int> carTextureIndex;
	bool insideWheelRender = false;
	bool firstDrawLogged = false;
	vector<bool> promotionLogged;
	vector<BYTE*> changedCars;
	vector<int> oldTextures;

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
		carCockpitSet.assign(numCars, VANILLA);
		selectionLogged.assign(numCars, false);

		OutputGP4PPDebugString("Steering Wheels: " + to_string(numCars) + " cars, " + to_string(numTeams) + " teams");

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

		loadExternalTextures(device);
	}

	void __cdecl renderCarSteeringWheelsFunc(BYTE* device, DWORD param)
	{
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
		loadAssetSettings(iniSettings, cockpitSettings);
		loadAssetSettings(iniSettings, dashSettings);
	}

	void ApplyPatches()
	{
		bool cockpitEnabled = cockpitSettings.enabled || dashSettings.enabled;
		bool countsRead = (externalSettings.enabled || cockpitEnabled) && readCarCounts();

		// External steering wheels: texture loading, render and stage flush calls
		if (externalSettings.enabled && countsRead)
		{
			if (checkCallBytes(initCarSteeringWheelsCall, initCarSteeringWheelsBytes, "InitCarSteeringWheels")
				&& checkCallBytes(renderCarSteeringWheelsCall, renderCarSteeringWheelsBytes, "RenderCarSteeringWheels")
				&& checkCallBytes(individualWheelFlushCall, individualWheelFlushBytes, "DeviceFlushState"))
			{
				MemUtils::injectFunctionCall(initCarSteeringWheelsCall, PtrToUlong(initCarSteeringWheelsFunc), VAR_NAME(initCarSteeringWheelsFunc));

				MemUtils::injectFunctionCall(individualWheelFlushCall, PtrToUlong(individualWheelFlushFunc), VAR_NAME(individualWheelFlushFunc));

				MemUtils::injectFunctionCall(renderCarSteeringWheelsCall, PtrToUlong(renderCarSteeringWheelsFunc), VAR_NAME(renderCarSteeringWheelsFunc));

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
