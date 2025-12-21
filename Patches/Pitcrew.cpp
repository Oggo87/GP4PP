#include "Pitcrew.h"
#include "../Utils/Helpers.h"
#include "../GP4MemLib/GP4MemLib.h"
#include "../IniLib/IniLib.h"
#include <windows.h>

using namespace std;
using namespace GP4MemLib;
using namespace IniLib;
using namespace GP4PP;

namespace Pitcrew
{
	// Settings variables
	bool hiResMeshes = false;
	bool individualTextures = false;
	bool individualMeshes = false;
	int d3dBufferSize = 0x4000;
};

namespace {

	////////////////////////////////////////////
	//Original vertex buffer at 0x00aa0828
	//Original normals buffer at 0x00a9f560
	//Original D3D size of 0x4000 at 0x004F2CA6,
	////////////////////////////////////////////
	int ApplyHiResMeshesPatch()
	{
		using namespace Pitcrew;

		ostringstream messageBuilder;

		LPBYTE pitcrewVertexBuffer = (LPBYTE)VirtualAlloc(NULL, 65536 * 4 * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!pitcrewVertexBuffer) {
			OutputGP4PPDebugString("Error allocating memory for new Vertex Buffer");
			return 1;
		}

		messageBuilder.str(string());
		messageBuilder << hex << showbase << PtrToUlong(pitcrewVertexBuffer)
			<< ", " << PtrToUlong(pitcrewVertexBuffer + 4)
			<< ", " << PtrToUlong(pitcrewVertexBuffer + 8);

		OutputGP4PPDebugString("Address of new Vertex Buffer: " + messageBuilder.str());

		//Patch exe for new vertex buffer
		MemUtils::patchAddress((LPVOID)0x004F929A, pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		MemUtils::patchAddress((LPVOID)0x004F92F5, pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		pitcrewVertexBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92FE, pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		pitcrewVertexBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F9307, pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));

		LPBYTE pitcrewNormalsBuffer = (LPBYTE)VirtualAlloc(NULL, 65536 * 4 * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!pitcrewNormalsBuffer) {
			OutputGP4PPDebugString("Error allocating memory for new Normals Buffer");
			return 1;
		}

		messageBuilder.str(string());
		messageBuilder << hex << showbase << PtrToUlong(pitcrewNormalsBuffer)
			<< ", " << PtrToUlong(pitcrewNormalsBuffer + 4)
			<< ", " << PtrToUlong(pitcrewNormalsBuffer + 8);

		OutputGP4PPDebugString("Address of new Normals Buffer: " + messageBuilder.str());

		//Patch exe for new normals buffer
		MemUtils::patchAddress((LPVOID)0x004F92A1, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F8187, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F92D8, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		pitcrewNormalsBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92E4, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F81AB, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		pitcrewNormalsBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92ED, pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));

		//Patch exe for larger D3D allocated memory space
		MemUtils::patchAddress((LPVOID)0x004F2CA6, MemUtils::toBytes<int>(d3dBufferSize), sizeof(int));

		//Patch exe to avoid recursion CTD in AnimSetData::meth_0x4f2130
		//It should really be converted to a non-recursive function
		//but for now just patching the jump to always taken
		byte jmp = 0xEB;
		MemUtils::patchAddress((LPVOID)0x004F21B6, MemUtils::toBytes<BYTE>(jmp), sizeof(BYTE));

		return 0;
	}
};

namespace Pitcrew
{

	void LoadSettings(IniFile iniSettings)
	{
		// Hi-Res Pitcrews
		try
		{
			hiResMeshes = iniSettings["Pitcrews"]["HiResMeshes"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Hi-Res Pitcrews : " + string(hiResMeshes ? "Enabled" : "Disabled"));

		if (hiResMeshes)
		{
			// Pitcrews Buffer Size
			try
			{
				d3dBufferSize = iniSettings["Pitcrews"]["CustomD3DBufferSize"].getAs<int>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Pitcrews D3D Buffer Size: " + to_string(d3dBufferSize));
		}

		// Individual Pitcrew Textures
		try
		{
			individualTextures = iniSettings["Pitcrews"]["IndividualTextures"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual Engineer Textures : " + string(individualTextures ? "Enabled" : "Disabled"));

		// Individual Pitcrew Meshes
		try
		{
			individualMeshes = iniSettings["Pitcrews"]["IndividualMeshes"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual Pitcrews : " + string(individualMeshes ? "Enabled" : "Disabled"));
	};

	//Target Addresses
	DWORD pitcrewStartAddress = 0x004F8A89;
	DWORD pitcrewStartAddress2 = 0x004f7961;

	DWORD pitcrewTextureStartAddress1 = 0x004f89fa;
	DWORD pitcrewTextureStartAddress2 = 0x004f896b;
	DWORD pitcrewTextureStartAddress3 = 0x004f89ac;
	DWORD pitcrewTextureStartAddress4 = 0x004f78c6;
	DWORD pitcrewTextureStartAddress5 = 0x004f7878;

	//Jump Back Addresses
	DWORD pitcrewJumpBackAddress = 0x004f8a9c;
	DWORD pitcrewJumpBackAddress2 = 0x004f7974;

	DWORD pitcrewTextureJumpBackAddress1 = 0x004f8a03;
	DWORD pitcrewTextureJumpBackAddress2 = 0x004f8974;
	DWORD pitcrewTextureJumpBackAddress3 = 0x004f89b5;
	DWORD pitcrewTextureJumpBackAddress4 = 0x004f78cf;
	DWORD pitcrewTextureJumpBackAddress5 = 0x004f7881;

	//Vars and Data Addresses
	DWORD useRaceAnimFolderAddress = 0x00aa1b54;

	int mdbSlotIndex;
	DWORD animSetDataAddress;
	DWORD isHighResMeshAddress;
	int* stackPtr;
	bool useRaceAnimFolder;
	bool isHighResMesh;

	__declspec(naked) void pitcrewFunc1()
	{

		//save source variables and address from registers
		__asm {
			mov stackPtr, ESP

			mov mdbSlotIndex, ESI		//mdb slot index = anim slot index

			mov animSetDataAddress, EBP
		}

		//read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		isHighResMeshAddress = MemUtils::addressToValue<DWORD>(animSetDataAddress + 0x204);

		isHighResMesh = MemUtils::addressToPtr<bool>(isHighResMeshAddress)[mdbSlotIndex];

		//adjust stack value for mdb slot index
		stackPtr += 0x8;

		//if not using race anim folder, there's one anim slot per car (two per team)
		//divide index by 2 to get one mdb slot per team
		if (!useRaceAnimFolder)
		{
			mdbSlotIndex /= 2;
		}

		//mdb slots are stored as pairs (even = hi-res, odd = low-res)
		mdbSlotIndex *= 2;

		if (!isHighResMesh)
		{
			mdbSlotIndex += 1;
		}

		//write back adjusted mdb slot index to stack
		*stackPtr = mdbSlotIndex;

		__asm jmp pitcrewJumpBackAddress //jump back into regular flow
	}

	__declspec(naked) void pitcrewFunc2()
	{
		//save source variables and address from registers
		__asm {
			mov stackPtr, ESP

			mov mdbSlotIndex, EBX	//mdb slot index = anim slot index

			mov animSetDataAddress, EBP
		}

		//read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		isHighResMeshAddress = MemUtils::addressToValue<DWORD>(animSetDataAddress + 0x204);

		isHighResMesh = MemUtils::addressToPtr<bool>(isHighResMeshAddress)[mdbSlotIndex];

		//adjust stack value for anim slot index
		stackPtr += 0xA;

		//if not using race anim folder, there's one anim slot per car (two per team)
		//divide index by 2 to get one mdb slot per team
		if (!useRaceAnimFolder)
		{
			mdbSlotIndex /= 2;
		}

		//mdb slots are stored as pairs (even = hi-res, odd = low-res)
		mdbSlotIndex *= 2;

		if (!isHighResMesh)
		{
			mdbSlotIndex += 1;
		}

		//write back adjusted mdb slot index to stack
		*stackPtr = mdbSlotIndex;

		__asm jmp pitcrewJumpBackAddress2 //jump back into regular flow
	}

	__declspec(naked) void pitcrewTextureFunc1()
	{
		//Original instructions
		__asm mov DL, byte ptr[EAX + 0x1f9c]

			RegUtils::saveVolatileRegisters();

		//Read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		if (useRaceAnimFolder)
		{
			RegUtils::restoreVolatileRegisters();

			//Original instruction
			//Index 0xb (11) - Marshall
			__asm mov ECX, dword ptr[ECX + 0x2c]
		}
		else
		{
			RegUtils::restoreVolatileRegisters();

			//Modified instruction
			//Index 0x16 (22) - Marshall
			__asm mov ECX, dword ptr[ECX + 0x58]
		}

		//Jump back into regular flow
		__asm jmp pitcrewTextureJumpBackAddress1
	}

	__declspec(naked) void pitcrewTextureFunc2()
	{
		//Original instructions
		__asm mov EAX, dword ptr[EBP + 0x1e8]

			RegUtils::saveVolatileRegisters();

		//Read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		if (useRaceAnimFolder)
		{
			RegUtils::restoreVolatileRegisters();

			//Original instruction
			//Index 0xc (12) - Race Director
			__asm mov ECX, dword ptr[EAX + 0x30]
		}
		else
		{
			RegUtils::restoreVolatileRegisters();

			//Modified instruction
			//Index 0x17 (23) - Race Director
			__asm mov ECX, dword ptr[EAX + 0x5c]
		}

		//Jump back into regular flow
		__asm jmp pitcrewTextureJumpBackAddress2
	}

	__declspec(naked) void pitcrewTextureFunc3()
	{
		//Original instructions
		__asm mov DL, byte ptr[EAX + 0x1f9c]

			RegUtils::saveVolatileRegisters();

		//Read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		if (useRaceAnimFolder)
		{
			RegUtils::restoreVolatileRegisters();

			//Original instruction
			//Index 0xb (11) - Marshall
			__asm mov ECX, dword ptr[ECX + 0x2c]
		}
		else
		{
			RegUtils::restoreVolatileRegisters();

			//Modified instruction
			//Index 0x16 (22) - Marshall
			__asm mov ECX, dword ptr[ECX + 0x58]
		}

		//Jump back into regular flow
		__asm jmp pitcrewTextureJumpBackAddress3
	}

	__declspec(naked) void pitcrewTextureFunc4()
	{
		//Original instructions
		__asm mov EAX, dword ptr[EBP + 0x1e8]

			RegUtils::saveVolatileRegisters();

		//Read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		if (useRaceAnimFolder)
		{
			RegUtils::restoreVolatileRegisters();

			//Original instruction
			//Index 0xb (11) - Marshall
			__asm mov ECX, dword ptr[EAX + 0x2c]
		}
		else
		{
			RegUtils::restoreVolatileRegisters();

			//Modified instruction
			//Index 0x16 (22) - Marshall
			__asm mov ECX, dword ptr[EAX + 0x58]
		}

		//Jump back into regular flow
		__asm jmp pitcrewTextureJumpBackAddress4
	}

	__declspec(naked) void pitcrewTextureFunc5()
	{
		//Original instructions
		__asm mov EAX, dword ptr[EBP + 0x1e8]

			RegUtils::saveVolatileRegisters();

		//Read necessary variables from memory
		useRaceAnimFolder = MemUtils::addressToValue<bool>(useRaceAnimFolderAddress);

		if (useRaceAnimFolder)
		{
			RegUtils::restoreVolatileRegisters();

			//Original instruction
			//Index 0xc (12) - Race Director
			__asm mov ECX, dword ptr[EAX + 0x30]
		}
		else
		{
			RegUtils::restoreVolatileRegisters();

			//Modified instruction
			//Index 0x17 (23) - Race Director
			__asm mov ECX, dword ptr[EAX + 0x5c]
		}

		//Jump back into regular flow
		__asm jmp pitcrewTextureJumpBackAddress5
	}

	void ApplyPatches()
	{

		if (hiResMeshes)
		{
			ApplyHiResMeshesPatch();
		}

		if (individualTextures)
		{
			DWORD targetAddress1 = 0x004f7918;
			DWORD targetAddress2 = 0x004f8a40;

			//Patch exe to skip over to always using anim slot index as texture slot index
			// 
			//During race sessions, only 11 anim slots are used (one per team)
			//During non-race sessions, 22 anim slots are used (one per car)
			//This enables as many textures as slot indices, allowing individual textures for each engineer during non-race sessions

			BYTE patch[] = { 0xeb, 0x00 }; //jmp to next instruction
			MemUtils::patchAddress((LPVOID)targetAddress1, patch, sizeof(patch));
			MemUtils::patchAddress((LPVOID)targetAddress2, patch, sizeof(patch));

			//Patch exe to use correct indices for marshall and race director textures
			// 
			//Race session: 0xb (11) Marshall and 0xc (12) Race Director
			//Non-race session: 0x16 (22) Marshall and 0x17 (23) Race Director

			//Re-route for pitcrew texture function 1
			MemUtils::rerouteFunction(pitcrewTextureStartAddress1, PtrToUlong(pitcrewTextureFunc1), VAR_NAME(pitcrewTextureFunc1));

			//Re-route for pitcrew texture function 2
			MemUtils::rerouteFunction(pitcrewTextureStartAddress2, PtrToUlong(pitcrewTextureFunc2), VAR_NAME(pitcrewTextureFunc2));

			//Re-route for pitcrew texture function 3
			MemUtils::rerouteFunction(pitcrewTextureStartAddress3, PtrToUlong(pitcrewTextureFunc3), VAR_NAME(pitcrewTextureFunc3));

			//Re-route for pitcrew texture function 4
			MemUtils::rerouteFunction(pitcrewTextureStartAddress4, PtrToUlong(pitcrewTextureFunc4), VAR_NAME(pitcrewTextureFunc4));

			//Re-route for pitcrew texture function 5
			MemUtils::rerouteFunction(pitcrewTextureStartAddress5, PtrToUlong(pitcrewTextureFunc5), VAR_NAME(pitcrewTextureFunc5));
		}

		if (individualMeshes)
		{

			//Re-route for pitctrew function 1
			MemUtils::rerouteFunction(pitcrewStartAddress, PtrToUlong(pitcrewFunc1), VAR_NAME(pitcrewFunc1));

			//Re-route for pitcrew function 2
			MemUtils::rerouteFunction(pitcrewJumpBackAddress2, PtrToUlong(pitcrewFunc2), VAR_NAME(pitcrewFunc2));

			// Adjust for marshalls meshes now using mesh slots 22 (hi-res) and 23 (low res)
			byte lowResMarshallIndex = 0x17; // 23

			// First function
			MemUtils::patchAddress((LPVOID)0x004F8A32, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));
			MemUtils::patchAddress((LPVOID)0x004F89e4, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));
			MemUtils::patchAddress((LPVOID)0x004F895C, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));

			// Second function
			MemUtils::patchAddress((LPVOID)0x004F790A, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));
			MemUtils::patchAddress((LPVOID)0x004F78BC, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));
			MemUtils::patchAddress((LPVOID)0x004F7869, MemUtils::toBytes<BYTE>(lowResMarshallIndex), sizeof(BYTE));
		}


	}
};

