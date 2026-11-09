#include "D3D.h"
#include "../GP4MemLib/GP4MemLib.h"
#include "../Utils/Helpers.h"
#include "../Utils/D3DTypes.h"

using namespace GP4MemLib;
using namespace GP4PP;

namespace D3D
{
	// General settings variables
	bool forceZBufferFormat = false;
	string zBufferFormatStr = "D16";
	D3DFORMAT zBufferFormat = D3DFMT_D16;

	// Target Addresses
	DWORD setZBufferCall = 0x005050fc;

	static unsigned int SetD3DBackBufferFormat(unsigned int adapterId, D3DDEVTYPE deviceType, D3DFORMAT displayFormat, D3DFORMAT* backBufferFormat)
	{
		// Set back buffer format
		*backBufferFormat = zBufferFormat;

		return 1; // Return 1 to indicate success
	}

	void LoadSettings(IniFile iniSettings)
	{
		// Force Z-Buffer Format
		try 
		{
			forceZBufferFormat = iniSettings["D3D"]["ForceZBufferFormat"].getAs<bool>();
		}
		catch (exception ex) {}

		OutputGP4PPDebugString("Force Z-Buffer Format : " + string(forceZBufferFormat ? "Enabled" : "Disabled"));

		if(forceZBufferFormat)
		{
			try
			{
				zBufferFormatStr = iniSettings["D3D"]["ZBufferFormat"].getString();

				if (zBufferFormatStr == "D32")
					zBufferFormat = D3DFMT_D32;
				else if (zBufferFormatStr == "D15S1")
					zBufferFormat = D3DFMT_D15S1;
				else if (zBufferFormatStr == "D24S8")
					zBufferFormat = D3DFMT_D24S8;
				else if (zBufferFormatStr == "D24X8")
					zBufferFormat = D3DFMT_D24X8;
				else if (zBufferFormatStr == "D24X4S4")
					zBufferFormat = D3DFMT_D24X4S4;
				else if (zBufferFormatStr == "D16")
					zBufferFormat = D3DFMT_D16;
				else
				{
					zBufferFormatStr = "D16";
					OutputGP4PPDebugString("Invalid Z-Buffer Format specified in INI file, defaulting to D16");
				}
			}
			catch (exception ex) {}

			OutputGP4PPDebugString("Z-Buffer Format : D3DFMT_" + zBufferFormatStr + " (" + to_string(zBufferFormat) + ")");
		}
	}

	void ApplyPatches()
	{
		MemUtils::injectFunctionCall(setZBufferCall, PtrToUlong(SetD3DBackBufferFormat), VAR_NAME(SetD3DBackBufferFormat));
	}
}