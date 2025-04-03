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
 *                 Project Name : dx8 caps                                                     *
 *                                                                                             *
 *                     $Archive:: /VSS_Sync/ww3d2/dx8caps.cpp                                 $*
 *                                                                                             *
 *              Original Author:: Hector Yee                                                   *
 *                                                                                             *
 *                      $Author:: Vss_sync                                                    $*
 *                                                                                             *
 *                     $Modtime:: 8/29/01 8:16p                                               $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"
#include "dx11caps.h"
#include "dx11wrapper.h"
#include "form11conv.h"

//D3DCAPS8 DX8Caps::hwVPCaps;
//D3DCAPS8 DX8Caps::swVPCaps;
bool DX11Caps::UseTnL;	
bool DX11Caps::SupportDOT3;
bool DX11Caps::SupportDXTC;
bool DX11Caps::supportGamma;
bool DX11Caps::SupportNPatches;
bool DX11Caps::SupportBumpEnvmap;
bool DX11Caps::SupportBumpEnvmapLuminance;
bool DX11Caps::SupportTextureFormat[WW3D_FORMAT_COUNT];
int DX11Caps::VertexShaderVersion;
int DX11Caps::PixelShaderVersion;
int DX11Caps::MaxSimultaneousTextures;

enum {
	VENDOR_ID_NVIDIA=0x10de,
	VENROD_ID_ATI=0x1002
};

// ----------------------------------------------------------------------------
//
// Init the caps structure
//
// ----------------------------------------------------------------------------

void DX11Caps::Init_Caps(ID3D11Device* D3DDevice)
{
	D3DDevice->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING,TRUE);
	DX11CALL(GetDeviceCaps(&swVPCaps));

	if ((swVPCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)==D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		UseTnL=true;

		D3DDevice->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING,FALSE);
		DX11CALL(GetDeviceCaps(&hwVPCaps));	
	} else {
		UseTnL=false;			
	}
}

// ----------------------------------------------------------------------------
//
// Compute the caps bits
//
// ----------------------------------------------------------------------------

void DX11Caps::Compute_Caps(DXGI_FORMAT display_format, DXGI_FORMAT depth_stencil_format, ID3D11Device* D3DDevice)
{
	const DXGI_ADAPTER_DESC& adapter_id=DX11Wrapper::Get_Current_Adapter_Identifier();

	Init_Caps(D3DDevice);

	const D3DCAPS8& caps=Get_Default_Caps();

	if ((caps.DevCaps&D3DDEVCAPS_NPATCHES)==D3DDEVCAPS_NPATCHES) {
		SupportNPatches=true;
	} else {
		SupportNPatches=false;
	}

	if ((caps.TextureOpCaps&D3DTEXOPCAPS_DOTPRODUCT3)==D3DTEXOPCAPS_DOTPRODUCT3) {
		SupportDOT3=true;
	} else {
		SupportDOT3=false;
	}

	supportGamma=((swVPCaps.Caps2&D3DCAPS2_FULLSCREENGAMMA)==D3DCAPS2_FULLSCREENGAMMA);

	Check_Texture_Format_Support(display_format,caps);
	Check_Texture_Compression_Support(caps);
	Check_Bumpmap_Support(caps);
	Check_Shader_Support(caps);
	Check_Maximum_Texture_Support(caps);
	// Vendor_Specific_Hacks(adapter_id); // Assumption: Hacks no longer required. 
}

// ----------------------------------------------------------------------------
//
// Check bump map texture support
//
// ----------------------------------------------------------------------------

void DX11Caps::Check_Bumpmap_Support(const D3DCAPS8& caps)
{
	SupportBumpEnvmap=!!(caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP);
	SupportBumpEnvmapLuminance=!!(caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE);
}

void DX11Caps::Check_Bumpmap_Support(ID3D11Device* device)
{
	// In DirectX 11, bump mapping is typically handled through shaders.  There's no direct
	// equivalent to the D3DTEXOPCAPS flags.  Support is generally assumed unless you
	// have specific reasons to believe it's not available (very unusual on modern hardware).

	SupportBumpEnvmap = true;
	SupportBumpEnvmapLuminance = true;
}


// ----------------------------------------------------------------------------
//
// Check compressed texture support
//
// ----------------------------------------------------------------------------

void DX11Caps::Check_Texture_Compression_Support(const D3DCAPS8& caps)
{
	SupportDXTC=SupportTextureFormat[WW3D_FORMAT_DXT1]|
		SupportTextureFormat[WW3D_FORMAT_DXT2]|
		SupportTextureFormat[WW3D_FORMAT_DXT3]|
		SupportTextureFormat[WW3D_FORMAT_DXT4]|
		SupportTextureFormat[WW3D_FORMAT_DXT5];
}

void DX11Caps::Check_Texture_Compression_Support(ID3D11Device* device)
{
	// DirectX 11 texture compression is handled differently. You'll need to query
	// feature levels and capabilities to determine support for BCn formats (DXT equivalents).
	D3D_FEATURE_LEVEL featureLevel;
	DX11Wrapper::GetFeatureLevel(device, featureLevel);

	SupportDXTC = false; // Assume no support initially

	if (featureLevel >= D3D_FEATURE_LEVEL_9_1) {  // BCn formats generally available from 9.1
		// Check for specific BCn format support using CheckFormatSupport:
		UINT supportFlags = 0;
		HRESULT hr;

		hr = device->CheckFormatSupport(DXGI_FORMAT_BC1_UNORM, &supportFlags);
		SupportDXTC |= SUCCEEDED(hr) && (supportFlags & D3D11_FORMAT_SUPPORT_TEXTURE2D);

		hr = device->CheckFormatSupport(DXGI_FORMAT_BC2_UNORM, &supportFlags);
		SupportDXTC |= SUCCEEDED(hr) && (supportFlags & D3D11_FORMAT_SUPPORT_TEXTURE2D);

		hr = device->CheckFormatSupport(DXGI_FORMAT_BC3_UNORM, &supportFlags);
		SupportDXTC |= SUCCEEDED(hr) && (supportFlags & D3D11_FORMAT_SUPPORT_TEXTURE2D);

		hr = device->CheckFormatSupport(DXGI_FORMAT_BC4_UNORM, &supportFlags);
		SupportDXTC |= SUCCEEDED(hr) && (supportFlags & D3D11_FORMAT_SUPPORT_TEXTURE2D);

		hr = device->CheckFormatSupport(DXGI_FORMAT_BC5_UNORM, &supportFlags);
		SupportDXTC |= SUCCEEDED(hr) && (supportFlags & D3D11_FORMAT_SUPPORT_TEXTURE2D);

		//Add support for BC6H and BC7 if needed and supported by the hardware
	}
}


void DX11Caps::Check_Texture_Format_Support(DXGI_FORMAT display_format,const D3DCAPS8& caps)
{
	for (unsigned i=0;i<WW3D_FORMAT_COUNT;++i) {
		if (i==WW3D_FORMAT_UNKNOWN) {
			SupportTextureFormat[i]=false;
		}
		else {
			SupportTextureFormat[i]=SUCCEEDED(
				DX8Wrapper::_Get_D3D8()->CheckDeviceFormat(
					caps.AdapterOrdinal,
					caps.DeviceType,
					display_format,
					0,
					D3DRTYPE_TEXTURE,
					WW3DFormat_To_D3DFormat((WW3DFormat)i)));
		}
	}
}

void DX11Caps::Check_Maximum_Texture_Support(const D3DCAPS8& caps)
{
	MaxSimultaneousTextures=caps.MaxSimultaneousTextures;
}


void DX11Caps::Check_Maximum_Texture_Support(ID3D11Device* device)
{
	// DirectX 11 has different limits.  Query the device capabilities through D3D11_FEATURE_DATA_THREADING.
	 // This only gives you the number of concurrent threads and not directly the maximum textures
	// The number of simultaneous textures depends on many factors (GPU, driver, etc.) and is often very high.


	D3D11_FEATURE_DATA_THREADING threadingCaps;
	HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingCaps, sizeof(threadingCaps));
	if (SUCCEEDED(hr))
	{
		//Use threadingCaps.NumSimultaneousThreads to calculate the max number of textures supported, if needed
		//This is a complex task and might require detailed hardware information
		//Consider using a reasonable upper limit or querying other relevant parameters in the device capabilites
		MaxSimultaneousTextures = 16; // A placeholder: replace with a suitable approach.
	}
	else
	{
		MaxSimultaneousTextures = 8; // A default value in case the query fails.
	}
}



void DX11Caps::Check_Shader_Support(const D3DCAPS8& caps)
{
	VertexShaderVersion=caps.VertexShaderVersion;
	PixelShaderVersion=caps.PixelShaderVersion;
}

void DX11Caps::Check_Shader_Support(ID3D11Device* device, D3D_FEATURE_LEVEL featureLevel)
{
	// DirectX 11 shader models are different.  Query the device's feature level to infer support.
	//  Example:  Feature level 11.0 supports Shader Model 5.0, etc.

	switch (featureLevel) {
	case D3D_FEATURE_LEVEL_11_0:
	case D3D_FEATURE_LEVEL_11_1:
		VertexShaderVersion = 512; // Shader Model 5.0 (major 5, minor 0)
		PixelShaderVersion = 512;  // Shader Model 5.0
		break;
		// Add other feature levels and their corresponding shader model versions here.
	default:
		VertexShaderVersion = 0;  // No shader support (unlikely for DX11)
		PixelShaderVersion = 0;
		break;
	}

}


// ----------------------------------------------------------------------------
//
// Implement some vendor-specific hacks to fix certain driver bugs that can't be
// avoided otherwise.
//
// ----------------------------------------------------------------------------

void DX11Caps::Vendor_Specific_Hacks(const DXGI_ADAPTER_DESC& adapter_id)
{
	//if (adapter_id.VendorId==VENDOR_ID_NVIDIA) {
	//	SupportNPatches = false;	// Driver incorrectly report N-Patch support
	//	SupportTextureFormat[WW3D_FORMAT_DXT1] = false;			// DXT1 is broken on NVidia hardware
	//	SupportDXTC=
	//		SupportTextureFormat[WW3D_FORMAT_DXT1]|
	//		SupportTextureFormat[WW3D_FORMAT_DXT2]|
	//		SupportTextureFormat[WW3D_FORMAT_DXT3]|
	//		SupportTextureFormat[WW3D_FORMAT_DXT4]|
	//		SupportTextureFormat[WW3D_FORMAT_DXT5];
	//}

//	SupportDXTC=false;

}







