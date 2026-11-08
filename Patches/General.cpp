#include "General.h"
#include "../Utils/Helpers.h"
#include "../GP4MemLib/GP4MemLib.h"

using namespace GP4MemLib;
using namespace GP4PP;

namespace General
{
	//General settings variables
	bool disableCDCheck = false;
	bool fix3DWheelsTreadMapping = false;
	bool fix3DWheelsCockpitView = false;
	bool stWheelAdvancedCarShader = false;
	bool showOpponentsFuelLaps = false;
	bool opponentsFuelLapsPatched = false;
	bool fixProjectedShadows = false;

	//Target Addresses
	DWORD wheelShaderSetMatrixStartAddress = 0x00488661;

	//Projected shadows: the shadow map covers +-3.45 around the car origin (RenderProjectedShadowMap) and its decal +-3.5
	//(BuildCarShadowDecals), so the shadow of a longer car is cut off at the map edge. Fix: coverage 6.9 and the two projected
	//decal size reads (mov edx, [g_carShadowDecalSize]) repointed to g_float7 (0x005f06d8, 7.0); the static read at 0x00491a62 keeps 3.5
	DWORD shadowDecalSizeOperands[] = { 0x0049199b, 0x00491a0c };	//projected + blur, projected unblurred
	BYTE shadowDecalSizeOldBytes[] = { 0xb8, 0x50, 0x64, 0x00 };	//g_carShadowDecalSize (0x006450b8)
	BYTE shadowDecalSizeNewBytes[] = { 0xd8, 0x06, 0x5f, 0x00 };	//g_float7 (0x005f06d8)
	DWORD shadowMapCoverage = 0x005f09d4;
	BYTE shadowMapCoverageOldBytes[] = { 0xcc, 0xcc, 0x5c, 0x40 };	//3.45
	BYTE shadowMapCoverageNewBytes[] = { 0xcd, 0xcc, 0xdc, 0x40 };	//6.9

	//Jump Back Addresses
	DWORD wheelShaderSetMatrixJumpBackAddress = 0x00488668;

	//Vars and Data Addresses
	DWORD d3dMatrixAddr_0xb8 = 0x00a4ca50;
	DWORD d3dMatrixAddr_0xbc = 0x00a4ca10;
	DWORD d3dMatrixAddr_0xc0 = 0x00a4d5a0;

	DWORD ptrCockpitWheels = 0x00a4d684;

	//LcdWriteFuel: jnz to the fuel display when not in a race, falling through to the opponents' "--.-" checks
	DWORD opponentsFuelLapsJump = 0x0051518e;
	BYTE opponentsFuelLapsJumpBytes[] = { 0x0f, 0x85, 0xa9, 0x00, 0x00, 0x00 };

	//Tyre Tread Variables
	float(*d3dMatrix_0xc0)[4][4], (*d3dMatrix_0xbc)[4][4], (*d3dMatrix_0xb8)[4][4];

	DWORD ptrMeshContainer = 0x0;
	int collisionMeshIndex = 0;

	void initD3DMatrixVariables()
	{
		//set matrices
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				if (i == j)
				{
					(*d3dMatrix_0xc0)[i][j] = (*d3dMatrix_0xbc)[i][j] = (*d3dMatrix_0xb8)[i][j] = 1;
				}
				else
				{
					(*d3dMatrix_0xc0)[i][j] = (*d3dMatrix_0xbc)[i][j] = (*d3dMatrix_0xb8)[i][j] = 0;
				}
			}
		}
	}

	void setD3DMatrix()
	{
		bool isCockpitMesh = (ptrMeshContainer == GP4MemLib::MemUtils::addressToValue<DWORD>(ptrCockpitWheels));

		//invert the Y axis for the tyre tread texture on the left wheels
		if (((fix3DWheelsTreadMapping && !isCockpitMesh) || (fix3DWheelsCockpitView && isCockpitMesh))
			&& (collisionMeshIndex == 0x12 || collisionMeshIndex == 0x16))
		{
			(*d3dMatrix_0xc0)[1][1] = -1;
		}

		//rotate the tyre tread texture when in cockpit view
		if (fix3DWheelsCockpitView && isCockpitMesh)
		{
			(*d3dMatrix_0xc0)[0][0] = -1;
			(*d3dMatrix_0xc0)[1][1] = -(*d3dMatrix_0xc0)[1][1];
		}
	}

	__declspec(naked) void wheelShaderSetMatrixFunc()
	{
		//EBP - CGP4Car
		//EDI - collision mesh index
		//EBX - CGP4WheelShader *
		//ESP + 0x1ec - collision mesh index

		//save mesh container pointer
		__asm mov EDI, dword ptr[ESP + 0x1e0]
			__asm mov ptrMeshContainer, EDI

		//save collision mesh index
		__asm mov EDI, dword ptr[ESP + 0x1ec]
			__asm mov collisionMeshIndex, EDI

		//set matrices
		initD3DMatrixVariables();

		setD3DMatrix();

		//set the matrices in the shader
		__asm {
			mov ECX, d3dMatrix_0xb8
			mov dword ptr[EBX + 0xb8], ECX
			mov ECX, d3dMatrix_0xbc
			mov dword ptr[EBX + 0xbc], ECX
			mov ECX, d3dMatrix_0xc0
			mov dword ptr[EBX + 0xc0], ECX
		}

		__asm lea ECX, [ESP + 0xdc] //original instruction

		_asm jmp wheelShaderSetMatrixJumpBackAddress //jump back into regular flow
	}

	void LoadSettings(IniFile iniSettings)
	{
		// CD Check Disable
		try
		{
			disableCDCheck = iniSettings["Settings"]["DisableCDCheck"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("CD Check : " + string(disableCDCheck ? "Disabled" : "Enabled"));

		// Fix 3D Wheels Tread Mapping
		try
		{
			fix3DWheelsTreadMapping = iniSettings["Settings"]["Fix3DWheels"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Fix 3D Wheels Tread Mapping : " + string(fix3DWheelsTreadMapping ? "Enabled" : "Disabled"));

		// Rotate cockpit view for front wheels (1 = enabled, 0 = disabled)
		try
		{
			fix3DWheelsCockpitView = iniSettings["Settings"]["Fix3DWheelsCockpitView"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Rotate Treads in Cockpit View : " + string(fix3DWheelsCockpitView ? "Enabled" : "Disabled"));

		// Enable Advanced Car Shader for in-cockpit steering wheel
		try
		{
			stWheelAdvancedCarShader = iniSettings["Settings"]["StWheelAdvancedCarShader"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Advanced Car Shader for Cockpit Steering Wheel : " + string(stWheelAdvancedCarShader ? "Enabled" : "Disabled"));
  
		// Show the opponents' fuel laps in a race
		try
		{
			showOpponentsFuelLaps = iniSettings["Settings"]["ShowOpponentsFuelLaps"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Show Opponents Fuel Laps : " + string(showOpponentsFuelLaps ? "Enabled" : "Disabled"));
    
		// Fix projected shadows of long cars
		try
		{
			fixProjectedShadows = iniSettings["Settings"]["FixProjectedShadows"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Fix Projected Shadows : " + string(fixProjectedShadows ? "Enabled" : "Disabled"));
	}

	bool IsOpponentsFuelLapsPatched()
	{
		return opponentsFuelLapsPatched;
	}

	void ApplyPatches()
	{
		// Apply Disable CD Check Patch
		if (disableCDCheck)
		{
			//Patch exe to always return 1 when checking for optical drive
			BYTE retVal = 0x01;
			MemUtils::patchAddress((LPVOID)0x0053712B, MemUtils::toBytes<BYTE>(retVal), sizeof(BYTE));
		}

		// Apply Fix 3D Wheels Tread Mapping Patch
		if (fix3DWheelsTreadMapping || fix3DWheelsCockpitView)
		{
			//Re-route for wheel tread texture set matrix
			MemUtils::rerouteFunction(wheelShaderSetMatrixStartAddress, PtrToUlong(wheelShaderSetMatrixFunc), VAR_NAME(wheelShaderSetMatrixFunc));

			//Assign matrix pointers to addresses in GP4 memory
			d3dMatrix_0xb8 = GP4MemLib::MemUtils::addressToPtr<float[4][4]>(d3dMatrixAddr_0xb8);
			d3dMatrix_0xbc = GP4MemLib::MemUtils::addressToPtr<float[4][4]>(d3dMatrixAddr_0xbc);
			d3dMatrix_0xc0 = GP4MemLib::MemUtils::addressToPtr<float[4][4]>(d3dMatrixAddr_0xc0);

			//Patch CGP4WheelShader
			//Skip the check for "Rotate wheels via textures" (CGP4WheelShader + 0xb4) by setting the comparison result always to TRUE (AL = 1)
			DWORD shaderPatchAddress = 0x0046a2a8;
			BYTE shaderPatch[] = { 0xb0, 0x01, 0x90, 0x90, 0x90, 0x90 }; //mov al, 1 - nop - nop - nop - nop - nop
			GP4MemLib::MemUtils::patchAddress((LPVOID)shaderPatchAddress, shaderPatch, sizeof(shaderPatch));
		}

		// Apply Advanced Car Shader for in-cockpit steering wheel Patch
		if (stWheelAdvancedCarShader)
		{
			//Patch to set the steering wheel in cockpit view to use the advanced car shader (0x12)
			DWORD carShaderPatchAddress = 0x0046faf3;
			BYTE carShaderPatch = { 0x12 };
			GP4MemLib::MemUtils::patchAddress((LPVOID)carShaderPatchAddress, MemUtils::toBytes<BYTE>(carShaderPatch), sizeof(BYTE));
		}

		// Apply Show Opponents Fuel Laps Patch
		if (showOpponentsFuelLaps)
		{
			//Patch the jnz to a jmp to the same target, so every car shows its fuel laps in a race too
			if (memcmp(MemUtils::addressToPtr<BYTE>(opponentsFuelLapsJump), opponentsFuelLapsJumpBytes, sizeof(opponentsFuelLapsJumpBytes)) == 0)
			{
				BYTE fuelLapsPatch[] = { 0xe9, 0xaa, 0x00, 0x00, 0x00, 0x90 }; //jmp 0x0051523d - nop
				GP4MemLib::MemUtils::patchAddress((LPVOID)opponentsFuelLapsJump, fuelLapsPatch, sizeof(fuelLapsPatch));

				opponentsFuelLapsPatched = true;
			}
			else
			{
				OutputGP4PPDebugString("Show Opponents Fuel Laps : unexpected bytes at " + MemUtils::dwordToString(opponentsFuelLapsJump) + " (LcdWriteFuel), opponents' fuel laps stay hidden");
			}

			OutputGP4PPDebugString("Show Opponents Fuel Laps patch : " + string(opponentsFuelLapsPatched ? "Applied" : "Not applied"));
		// Apply Fix Projected Shadows Patch
		if (fixProjectedShadows)
		{
			//All three locations or none
			bool bytesMatch = memcmp(MemUtils::addressToPtr<BYTE>(shadowMapCoverage), shadowMapCoverageOldBytes, sizeof(shadowMapCoverageOldBytes)) == 0;

			for (DWORD operand : shadowDecalSizeOperands)
			{
				bytesMatch = bytesMatch && memcmp(MemUtils::addressToPtr<BYTE>(operand), shadowDecalSizeOldBytes, sizeof(shadowDecalSizeOldBytes)) == 0;
			}

			if (bytesMatch)
			{
				for (DWORD operand : shadowDecalSizeOperands)
				{
					GP4MemLib::MemUtils::patchAddress((LPVOID)operand, shadowDecalSizeNewBytes, sizeof(shadowDecalSizeNewBytes));
				}

				GP4MemLib::MemUtils::patchAddress((LPVOID)shadowMapCoverage, shadowMapCoverageNewBytes, sizeof(shadowMapCoverageNewBytes));

				OutputGP4PPDebugString("Fix Projected Shadows patch : Applied");
			}
			else
			{
				OutputGP4PPDebugString("Fix Projected Shadows : unexpected bytes in BuildCarShadowDecals or at the shadow map coverage, patch not applied");
			}
		}
	};
};