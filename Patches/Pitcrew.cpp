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

		char* pitcrewVertexBuffer = (char*)VirtualAlloc(NULL, 65536 * 4 * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
		MemUtils::patchAddress((LPVOID)0x004F929A, (BYTE*)&pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		MemUtils::patchAddress((LPVOID)0x004F92F5, (BYTE*)&pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		pitcrewVertexBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92FE, (BYTE*)&pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));
		pitcrewVertexBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F9307, (BYTE*)&pitcrewVertexBuffer, sizeof(pitcrewVertexBuffer));

		char* pitcrewNormalsBuffer = (char*)VirtualAlloc(NULL, 65536 * 4 * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
		MemUtils::patchAddress((LPVOID)0x004F92A1, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F8187, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F92D8, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		pitcrewNormalsBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92E4, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		MemUtils::patchAddress((LPVOID)0x004F81AB, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));
		pitcrewNormalsBuffer += 4;
		MemUtils::patchAddress((LPVOID)0x004F92ED, (BYTE*)&pitcrewNormalsBuffer, sizeof(pitcrewNormalsBuffer));

		//Patch exe for larger D3D allocated memory space
		MemUtils::patchAddress((LPVOID)0x004F2CA6, (BYTE*)&d3dBufferSize, sizeof(int));

		//Patch exe to avoid recursion CTD in AnimSetData::meth_0x4f2130
		//It should really be converted to a non-recursive function
		//but for now just patching the jump to always taken
		byte jmp = 0xEB;
		MemUtils::patchAddress((LPVOID)0x004F21B6, (BYTE*)&jmp, sizeof(byte));

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

		if(hiResMeshes)
		{
			// Pitcrews Buffer Size
			try
			{
				d3dBufferSize = iniSettings["Pitcrews"]["CustomD3DBufferSize"].getAs<int>();
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Pitcrews D3D Buffer Size: " + to_string(d3dBufferSize));
		}

		// Individual Pitcrews
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

	//Jump Back Addresses
	DWORD pitcrewJumpBackAddress = 0x004f8a9c;
	DWORD pitcrewJumpBackAddress2 = 0x004f7974;

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


	void ApplyPatches()
	{

		if (hiResMeshes)
		{
			ApplyHiResMeshesPatch();
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
			MemUtils::patchAddress((LPVOID)0x004F8A32, (BYTE*)&lowResMarshallIndex, sizeof(byte));
			MemUtils::patchAddress((LPVOID)0x004F89e4, (BYTE*)&lowResMarshallIndex, sizeof(byte));
			MemUtils::patchAddress((LPVOID)0x004F895C, (BYTE*)&lowResMarshallIndex, sizeof(byte));

			// Second function
			MemUtils::patchAddress((LPVOID)0x004F790A, (BYTE*)&lowResMarshallIndex, sizeof(byte));
			MemUtils::patchAddress((LPVOID)0x004F78BC, (BYTE*)&lowResMarshallIndex, sizeof(byte));
			MemUtils::patchAddress((LPVOID)0x004F7869, (BYTE*)&lowResMarshallIndex, sizeof(byte));
		}


	}
};

