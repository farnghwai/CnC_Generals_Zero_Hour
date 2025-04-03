/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/formconv.cpp                           $*
 *                                                                                             *
 *              Original Author:: Nathaniel Hoffman                                            *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/16/01 1:33p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "form11conv.h"

DXGI_FORMAT WW3DFormatToD3DFormatConversionArray[WW3D_FORMAT_COUNT] = {
	DXGI_FORMAT_UNKNOWN, // DX8: D3DFMT_UNKNOWN
	// DX8: D3DFMT_R8G8B8
	DXGI_FORMAT_B8G8R8A8_UNORM,  // (DXGI 1.1), // DX8: D3DFMT_A8R8G8B8
	DXGI_FORMAT_B8G8R8X8_UNORM, // (DXGI 1.1) // DX8: D3DFMT_X8R8G8B8
	DXGI_FORMAT_B5G6R5_UNORM, // (DXGI 1.2) // DX8: D3DFMT_R5G6B5
	// DX8: D3DFMT_X1R5G5B5
	DXGI_FORMAT_B5G5R5A1_UNORM, // (DXGI 1.2), // DX8: D3DFMT_A1R5G5B5
	DXGI_FORMAT_B4G4R4A4_UNORM, // (DXGI 1.2), // DX8: D3DFMT_A4R4G4B4
	// DX8: D3DFMT_R3G3B2
	DXGI_FORMAT_A8_UNORM, // DX8: D3DFMT_A8
	// DX8: D3DFMT_A8R3G3B2
	// DX8: D3DFMT_X4R4G4B4
	// DX8: D3DFMT_A8P8
	// DX8: D3DFMT_P8
	DXGI_FORMAT_R8_UNORM, // DX8: D3DFMT_L8
	// DX8: D3DFMT_A8L8
	// DX8: D3DFMT_A4L4
	DXGI_FORMAT_R8G8_SNORM, // DX8: D3DFMT_V8U8 // Bumpmap
	// DX8: D3DFMT_L6V5U5 // Bumpmap
	// DX8: D3DFMT_X8L8V8U8 // Bumpmap
	DXGI_FORMAT_BC1_UNORM_SRGB, // DX8: D3DFMT_DXT1
	DXGI_FORMAT_BC2_UNORM, // DX8: D3DFMT_DXT2
	DXGI_FORMAT_BC2_UNORM_SRGB, // DX8: D3DFMT_DXT3
	DXGI_FORMAT_BC3_UNORM, // DX8: D3DFMT_DXT4
	DXGI_FORMAT_BC3_UNORM_SRGB // DX8: D3DFMT_DXT5
};

/*
#define HIGHEST_SUPPORTED_D3DFORMAT D3DFMT_X8L8V8U8	//A4L4
WW3DFormat D3DFormatToWW3DFormatConversionArray[HIGHEST_SUPPORTED_D3DFORMAT + 1] = {
	WW3D_FORMAT_UNKNOWN,		// 0
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_R8G8B8,		// 20
	WW3D_FORMAT_A8R8G8B8,
	WW3D_FORMAT_X8R8G8B8,
	WW3D_FORMAT_R5G6B5,
	WW3D_FORMAT_X1R5G5B5,
	WW3D_FORMAT_A1R5G5B5,
	WW3D_FORMAT_A4R4G4B4,
	WW3D_FORMAT_R3G3B2,
	WW3D_FORMAT_A8,
	WW3D_FORMAT_A8R3G3B2,
	WW3D_FORMAT_X4R4G4B4,	// 30
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_A8P8,			// 40
	WW3D_FORMAT_P8,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,	WW3D_FORMAT_UNKNOWN,
	WW3D_FORMAT_L8,			// 50
	WW3D_FORMAT_A8L8,
	WW3D_FORMAT_A4L4
};
*/

#define HIGHEST_SUPPORTED_D3DFORMAT DXGI_FORMAT_B4G4R4A4_UNORM
WW3D11Format D3DFormatToWW3DFormatConversionArray[HIGHEST_SUPPORTED_D3DFORMAT + 1];

DXGI_FORMAT WW3DFormat_To_D3DFormat(WW3D11Format ww3d_format) {
	if (ww3d_format < WW3D_FORMAT_UNKNOWN || ww3d_format >= WW3D_FORMAT_COUNT) {
		return DXGI_FORMAT_UNKNOWN;
	} else {
		return WW3DFormatToD3DFormatConversionArray[(unsigned int)ww3d_format];
	}
}

WW3D11Format D3DFormat_To_WW3DFormat(DXGI_FORMAT d3d_format)
{
	switch (d3d_format) {
	// The DXT-codes are created with FOURCC macro and thus can't be placed in the conversion table
	case DXGI_FORMAT_BC1_UNORM_SRGB: return WW3D_FORMAT_DXT1;
	case DXGI_FORMAT_BC2_UNORM: return WW3D_FORMAT_DXT2;
	case DXGI_FORMAT_BC2_UNORM_SRGB: return WW3D_FORMAT_DXT3;
	case DXGI_FORMAT_BC3_UNORM: return WW3D_FORMAT_DXT4;
	case DXGI_FORMAT_BC3_UNORM_SRGB: return WW3D_FORMAT_DXT5;
	default:
		if (d3d_format < DXGI_FORMAT_UNKNOWN || d3d_format > HIGHEST_SUPPORTED_D3DFORMAT) {
			return WW3D_FORMAT_UNKNOWN;
		} else {
			return D3DFormatToWW3DFormatConversionArray[(unsigned int)d3d_format];
		}
		break;
	}
}

void Init_D3D_To_WW3_Conversion()
{
	for (int i=0;i<HIGHEST_SUPPORTED_D3DFORMAT;++i) {
		D3DFormatToWW3DFormatConversionArray[i]=WW3D_FORMAT_UNKNOWN;
	}

	//D3DFormatToWW3DFormatConversionArray[D3DFMT_R8G8B8]=WW3D_FORMAT_R8G8B8;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_B8G8R8A8_UNORM]=WW3D_FORMAT_A8R8G8B8;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_B8G8R8X8_UNORM]=WW3D_FORMAT_X8R8G8B8;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_B5G6R5_UNORM]=WW3D_FORMAT_R5G6B5;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_X1R5G5B5]=WW3D_FORMAT_X1R5G5B5;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_B5G5R5A1_UNORM]=WW3D_FORMAT_A1R5G5B5;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_B4G4R4A4_UNORM]=WW3D_FORMAT_A4R4G4B4;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_R3G3B2]=WW3D_FORMAT_R3G3B2;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_A8_UNORM]=WW3D_FORMAT_A8;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_A8R3G3B2]=WW3D_FORMAT_A8R3G3B2;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_X4R4G4B4]=WW3D_FORMAT_X4R4G4B4;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_A8P8]=WW3D_FORMAT_A8P8;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_P8]=WW3D_FORMAT_P8;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_R8_UNORM]=WW3D_FORMAT_L8;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_A8L8]=WW3D_FORMAT_A8L8;
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_A4L4]=WW3D_FORMAT_A4L4;
	D3DFormatToWW3DFormatConversionArray[DXGI_FORMAT_R8G8_SNORM]=WW3D_FORMAT_U8V8;	// Bumpmap
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_L6V5U5]=WW3D_FORMAT_L6V5U5;		// Bumpmap
	//D3DFormatToWW3DFormatConversionArray[D3DFMT_X8L8V8U8]=WW3D_FORMAT_X8L8V8U8;	// Bumpmap
};
