#pragma once

#include <windows.h>
#include <cmath>

// Direct3D 8 types and constants used by the patches, with the same names and values as d3d8types.h and d3d8.h.
// GP4 uses Direct3D 8, whose headers are not part of the Windows SDK.

namespace GP4PP
{
	#ifndef MAKEFOURCC
	#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
	#endif

	// Device types
	enum D3DDEVTYPE
	{
		D3DDEVTYPE_HAL = 1,
		D3DDEVTYPE_REF = 2,
		D3DDEVTYPE_SW = 3,

		D3DDEVTYPE_FORCE_DWORD = 0x7fffffff
	};

	// Surface, texture, depth buffer and vertex/index buffer formats
	enum D3DFORMAT
	{
		D3DFMT_UNKNOWN = 0,

		D3DFMT_R8G8B8 = 20,
		D3DFMT_A8R8G8B8 = 21,
		D3DFMT_X8R8G8B8 = 22,
		D3DFMT_R5G6B5 = 23,
		D3DFMT_X1R5G5B5 = 24,
		D3DFMT_A1R5G5B5 = 25,
		D3DFMT_A4R4G4B4 = 26,
		D3DFMT_R3G3B2 = 27,
		D3DFMT_A8 = 28,
		D3DFMT_A8R3G3B2 = 29,
		D3DFMT_X4R4G4B4 = 30,
		D3DFMT_A2B10G10R10 = 31,
		D3DFMT_G16R16 = 34,

		D3DFMT_A8P8 = 40,
		D3DFMT_P8 = 41,

		D3DFMT_L8 = 50,
		D3DFMT_A8L8 = 51,
		D3DFMT_A4L4 = 52,

		D3DFMT_V8U8 = 60,
		D3DFMT_L6V5U5 = 61,
		D3DFMT_X8L8V8U8 = 62,
		D3DFMT_Q8W8V8U8 = 63,
		D3DFMT_V16U16 = 64,
		D3DFMT_W11V11U10 = 65,
		D3DFMT_A2W10V10U10 = 67,

		D3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
		D3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
		D3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
		D3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
		D3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
		D3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
		D3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),

		D3DFMT_D16_LOCKABLE = 70,
		D3DFMT_D32 = 71,
		D3DFMT_D15S1 = 73,
		D3DFMT_D24S8 = 75,
		D3DFMT_D16 = 80,
		D3DFMT_D24X8 = 77,
		D3DFMT_D24X4S4 = 79,

		D3DFMT_VERTEXDATA = 100,
		D3DFMT_INDEX16 = 101,
		D3DFMT_INDEX32 = 102,

		D3DFMT_FORCE_DWORD = 0x7fffffff
	};

	// Row-major 4x4 matrix
	struct D3DMATRIX
	{
		union
		{
			struct
			{
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float m[4][4];
		};
	};

	// Transform states (D3DTRANSFORMSTATETYPE)
	const DWORD D3DTS_WORLD = 256;				// D3DTS_WORLDMATRIX(0)

	// Render states (D3DRENDERSTATETYPE)
	const DWORD D3DRS_NORMALIZENORMALS = 143;

	// IDirect3DDevice8 vtable offsets
	const DWORD D3DDEVICE8_SET_TRANSFORM = 0x94;

	// IDirect3DDevice8::SetTransform
	typedef long(__stdcall* D3DSetTransformType)(void*, DWORD, const D3DMATRIX*);

	//
	// Matrix helpers, same results as the D3DX functions of the same name
	//

	inline D3DMATRIX matrixMultiply(const D3DMATRIX& a, const D3DMATRIX& b)
	{
		D3DMATRIX result;

		for (int row = 0; row < 4; row++)
		{
			for (int column = 0; column < 4; column++)
			{
				result.m[row][column] = a.m[row][0] * b.m[0][column] + a.m[row][1] * b.m[1][column] + a.m[row][2] * b.m[2][column] + a.m[row][3] * b.m[3][column];
			}
		}

		return result;
	}

	inline D3DMATRIX matrixIdentity()
	{
		D3DMATRIX result = {};

		result.m[0][0] = result.m[1][1] = result.m[2][2] = result.m[3][3] = 1.0f;

		return result;
	}

	inline D3DMATRIX matrixTranslation(float x, float y, float z)
	{
		D3DMATRIX result = matrixIdentity();

		result.m[3][0] = x;
		result.m[3][1] = y;
		result.m[3][2] = z;

		return result;
	}

	inline D3DMATRIX matrixScaling(float scale)
	{
		D3DMATRIX result = matrixIdentity();

		result.m[0][0] = result.m[1][1] = result.m[2][2] = scale;

		return result;
	}

	inline D3DMATRIX matrixRotationX(float angle)
	{
		D3DMATRIX result = matrixIdentity();

		result.m[1][1] = cosf(angle);
		result.m[1][2] = sinf(angle);
		result.m[2][1] = -sinf(angle);
		result.m[2][2] = cosf(angle);

		return result;
	}

	inline D3DMATRIX matrixRotationZ(float angle)
	{
		D3DMATRIX result = matrixIdentity();

		result.m[0][0] = cosf(angle);
		result.m[0][1] = sinf(angle);
		result.m[1][0] = -sinf(angle);
		result.m[1][1] = cosf(angle);

		return result;
	}
}
