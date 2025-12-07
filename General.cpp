#include "General.h"
#include "Helpers.h"
#include "GP4MemLib/GP4MemLib.h"

using namespace GP4MemLib;
using namespace GP4PP;

namespace General
{
	//General settings variables
	bool disableCDCheck = false;
	bool fix3DWheelsTreadMapping = false;
	bool stWheelAdvancedCarShader = false;

	//Target Addresses
	DWORD wheelShaderSetMatrixStartAddress = 0x00488661;

	//Jump Back Addresses
	DWORD wheelShaderSetMatrixJumpBackAddress = 0x00488668;

	//Vars and Data Addresses
	DWORD d3dMatrixAddr_0xb8 = 0x00a4ca50;
	DWORD d3dMatrixAddr_0xbc = 0x00a4ca10;
	DWORD d3dMatrixAddr_0xc0 = 0x00a4d5a0;

	DWORD ptrCockpitWheels = 0x00a4d684;

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

		//invert the Y axis for the tyre tread texture on the left wheels
		if (collisionMeshIndex == 0x12 || collisionMeshIndex == 0x16)
		{
			(*d3dMatrix_0xc0)[1][1] = -1;
		}
		//rotate the tyre tread texture when in cockpit view
		if (ptrMeshContainer == GP4MemLib::MemUtils::addressToValue<DWORD>(ptrCockpitWheels))
		{
			(*d3dMatrix_0xc0)[0][0] = -1;
			(*d3dMatrix_0xc0)[1][1] = -(*d3dMatrix_0xc0)[1][1];
		}

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

		// Enable Advanced Car Shader for in-cockpit steering wheel
		try
		{
			stWheelAdvancedCarShader = iniSettings["Settings"]["StWheelAdvancedCarShader"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Advanced Car Shader for Cockpit Steering Wheel : " + string(stWheelAdvancedCarShader ? "Enabled" : "Disabled"));
	}

	void ApplyPatches()
	{
		// Apply Disable CD Check Patch
		if (disableCDCheck)
		{
			//Patch exe to always return 1 when checking for optical drive
			BYTE retVal = 0x01;
			MemUtils::patchAddress((LPVOID)0x0053712B, (BYTE*)&retVal, sizeof(BYTE));
		}

		// Apply Fix 3D Wheels Tread Mapping Patch
		if (fix3DWheelsTreadMapping)
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
			GP4MemLib::MemUtils::patchAddress((LPVOID)carShaderPatchAddress, (BYTE*)&carShaderPatch, sizeof(BYTE));
		}
	};
};