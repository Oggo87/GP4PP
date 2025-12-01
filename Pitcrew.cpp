#include "Pitcrew.h"
#include "Helpers.h"
#include "GP4MemLib/GP4MemLib.h"
#include "IniLib/IniLib.h"
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

		// Individual Pitcrews
		try
		{
			individualMeshes = iniSettings["Pitcrews"]["IndividualMeshes"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Individual Pitcrews : " + string(individualMeshes ? "Enabled" : "Disabled"));

		// Pitcrews Buffer Size
		try
		{
			d3dBufferSize = iniSettings["Pitcrews"]["CustomD3DBufferSize"].getAs<int>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Pitcrews D3D Buffer Size: " + to_string(d3dBufferSize));
	};

	void ApplyPatches()
	{

		if (hiResMeshes)
		{
			ApplyHiResMeshesPatch();
		}

		if (individualMeshes)
		{

			//	Patch first function
			///////////////////////////////////////////////////
			//	004f8a89 89  f1					MOV        ECX, ESI										Save anim slot index into ECX
			//	004f8a8b a0  54  1b	aa  00      MOV        AL, [BOOL_useRaceAnimFolder_0x00aa1b54]		Check if using race anim folder
			//	004f8a90 84  c0					TEST       AL, AL										If not
			//	004f8a92 75  02					JNZ        LAB_004f8a96									Skip
			//	004f8a94 d1  f9					SAR        ECX, 0x1										If not race anim folder, divide index by 2
			//	004f8a96 d1  e1					SHL        ECX, 0x1										Multiply index by 2 (to get correct offset in MDB mesh array)
			//	004f8a98 89  4c  24  20			MOV        dword ptr[ESP + 0x20], ECX					Save new index
			///////////////////////////////////////////////////

			// Should be adding 1 when calling for low detail meshes (this->useLowRes?_0x204 == true)
			// Will probably require jumping to a patch function to handle properly

			DWORD firstPatchAddress = 0x004F8A89;
			BYTE firstPatch[] = { 0x89, 0xF1, 0xA0, 0x54, 0x1B, 0xAA, 0x00, 0x84, 0xC0, 0x75, 0x02, 0xD1, 0xF9, 0xD1, 0xE1, 0x89, 0x4C, 0x24, 0x20 };
			GP4MemLib::MemUtils::patchAddress((LPVOID)firstPatchAddress, firstPatch, sizeof(firstPatch));

			//	Patch first function
			///////////////////////////////////////////////////
			//	004f7961 89  da					MOV        EDX ,EBX										Save anim slot index into ECX
			//	004f7963 a0  54  1b  aa  00     MOV        AL, [BOOL_useRaceAnimFolder_0x00aa1b54]		Check if using race anim folder
			//	004f7968 84  c0					TEST       AL, AL										If not
			//	004f796a 75  02					JNZ        LAB_004f796e									Skip
			//	004f796c d1  fa					SAR        EDX, 0x1										If not race anim folder, divide index by 2
			//	004f796e d1  e2					SHL        EDX, 0x1										Multiply index by 2 (to get correct offset in MDB mesh array)
			//	004f7970 89  54  24  28			MOV        dword ptr[ESP + 0x28], EDX					Save new index
			///////////////////////////////////////////////////

			// Should be adding 1 when calling for low detail meshes (this->useLowRes?_0x204 == true)
			// Will probably require jumping to a patch function to handle properly

			DWORD secondPatchAddress = 0x004F7961;
			BYTE secondPatch[] = { 0x89, 0xda, 0xa0, 0x54, 0x1b, 0xaa, 0x00, 0x84, 0xc0, 0x75, 0x02, 0xd1, 0xfa, 0xd1, 0xe2, 0x89, 0x54, 0x24, 0x28 };
			GP4MemLib::MemUtils::patchAddress((LPVOID)secondPatchAddress, secondPatch, sizeof(secondPatch));


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

