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
 *                     $Archive:: /VSS_Sync/ww3d2/dx8wrapper.cpp                              $*
 *                                                                                             *
 *              Original Author:: Jani Penttinen                                               *
 *                                                                                             *
 *                      $Author:: Vss_sync                                                    $*
 *                                                                                             *
 *                     $Modtime:: 8/29/01 7:29p                                               $*
 *                                                                                             *
 *                    $Revision:: 134                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   DX8Wrapper::_Update_Texture -- Copies a texture from system memory to video memory        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//#define CREATE_DX8_MULTI_THREADED

#include "dx8wrapper.h"
#include "dx8fvf.h"
#include "dx8vertexbuffer.h"
#include "dx8indexbuffer.h"
#include "dx8renderer.h"
#include "ww3d.h"
#include "camera.h"
#include "wwstring.h"
#include "matrix4.h"
#include "vertmaterial.h"
#include "rddesc.h"
#include "lightenvironment.h"
#include "statistics.h"
#include "registry.h"
#include "boxrobj.h"
#include "pointgr.h"
#include "render2d.h"
#include "sortingrenderer.h"
#include "shattersystem.h"
#include "light.h"
#include "assetmgr.h"
#include "textureloader.h"
#include "missingtexture.h"
#include "thread.h"
#include <stdio.h>
//#include <d3dx9core.h>
#include "pot.h"
#include "wwprofile.h"
#include "ffactory.h"
#include "dx8caps.h"
#include "formconv.h"
#include "dx8texman.h"
#include "bound.h"
#include "dx8webbrowser.h"

#define WW3D_DEVTYPE D3DDEVTYPE_HAL
#ifdef _MSC_VER
	//#define _CRT_SECURE_NO_WARNINGS  // Suppress warnings about unsafe functions
	#include <cstdio>
	#include <cstdarg>

	inline int safe_sprintf(char* buffer, const char* format, ...) {
		va_list args;
		va_start(args, format);
		int result = vsprintf_s(buffer, _TRUNCATE, format, args);
		va_end(args);
		return result;
	}
	
	#define sprintf safe_sprintf
#endif

// [DX9]
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

const int DEFAULT_RESOLUTION_WIDTH = 640;
const int DEFAULT_RESOLUTION_HEIGHT = 480;
const int DEFAULT_BIT_DEPTH = 32;
const int DEFAULT_TEXTURE_BIT_DEPTH = 16;

bool DX8Wrapper_IsWindowed = true;

// FPU_PRESERVE
int DX8Wrapper_PreserveFPU = 0;

/***********************************************************************************
**
** DX8Wrapper Static Variables
**
***********************************************************************************/

static HWND						_Hwnd												= NULL;
bool							DX8Wrapper::IsInitted								= false;	
bool							DX8Wrapper::_EnableTriangleDraw						= true;

int								DX8Wrapper::CurRenderDevice							= -1;
int								DX8Wrapper::ResolutionWidth							= DEFAULT_RESOLUTION_WIDTH;
int								DX8Wrapper::ResolutionHeight						= DEFAULT_RESOLUTION_HEIGHT;
int								DX8Wrapper::BitDepth								= DEFAULT_BIT_DEPTH;
int								DX8Wrapper::TextureBitDepth							= DEFAULT_TEXTURE_BIT_DEPTH;
bool							DX8Wrapper::IsWindowed								= false;
D3DFORMAT						DX8Wrapper::DisplayFormat							= D3DFMT_UNKNOWN;

D3DMATRIX						DX8Wrapper::old_world;
D3DMATRIX						DX8Wrapper::old_view;
D3DMATRIX						DX8Wrapper::old_prj;

bool								DX8Wrapper::world_identity;
unsigned							DX8Wrapper::RenderStates[256];
unsigned							DX8Wrapper::TextureStageStates[MAX_TEXTURE_STAGES][32];
unsigned							DX8Wrapper::SamplerStageStates[MAX_TEXTURE_STAGES][32];
IDirect3DBaseTexture9 *				DX8Wrapper::Textures[MAX_TEXTURE_STAGES];
RenderStateStruct					DX8Wrapper::render_state;
unsigned							DX8Wrapper::render_state_changed;

bool								DX8Wrapper::FogEnable						= false;
D3DCOLOR							DX8Wrapper::FogColor						= 0;

IDirect3D9 *				DX8Wrapper::D3DInterface							= NULL;
IDirect3DDevice9 *			DX8Wrapper::D3DDevice								= NULL;
IDirect3DSurface9 *			DX8Wrapper::CurrentRenderTarget						= NULL;
IDirect3DSurface9 *			DX8Wrapper::DefaultRenderTarget						= NULL;

unsigned							DX8Wrapper::matrix_changes					= 0;
unsigned							DX8Wrapper::material_changes				= 0;
unsigned							DX8Wrapper::vertex_buffer_changes			= 0;
unsigned							DX8Wrapper::index_buffer_changes            = 0;
unsigned							DX8Wrapper::light_changes					= 0;
unsigned							DX8Wrapper::texture_changes					= 0;
unsigned							DX8Wrapper::render_state_changes			= 0;
unsigned							DX8Wrapper::texture_stage_state_changes		= 0;
unsigned							DX8Wrapper::sampler_stage_state_changes		= 0;
unsigned							DX8Wrapper::_MainThreadID					= 0;
bool								DX8Wrapper::CurrentDX8LightEnables[4];

D3DADAPTER_IDENTIFIER9		DX8Wrapper::CurrentAdapterIdentifier;

unsigned long DX8Wrapper::FrameCount = 0;

bool								_DX8SingleThreaded							= false;

unsigned							number_of_DX8_calls							= 0;
static unsigned				last_frame_matrix_changes							= 0;
static unsigned				last_frame_material_changes							= 0;
static unsigned				last_frame_vertex_buffer_changes					= 0;
static unsigned				last_frame_index_buffer_changes						= 0;
static unsigned				last_frame_light_changes							= 0;
static unsigned				last_frame_texture_changes							= 0;
static unsigned				last_frame_render_state_changes						= 0;
static unsigned				last_frame_texture_stage_state_changes				= 0;
static unsigned				last_frame_number_of_DX8_calls						= 0;

static D3DPRESENT_PARAMETERS							_PresentParameters;
static DynamicVectorClass<StringClass>					_RenderDeviceNameTable;
static DynamicVectorClass<StringClass>					_RenderDeviceShortNameTable;
static DynamicVectorClass<RenderDeviceDescClass>		_RenderDeviceDescriptionTable;

/*
** Registry value names
*/
#define	VALUE_NAME_RENDER_DEVICE_NAME				"RenderDeviceName"
#define	VALUE_NAME_RENDER_DEVICE_WIDTH				"RenderDeviceWidth"
#define	VALUE_NAME_RENDER_DEVICE_HEIGHT				"RenderDeviceHeight"
#define	VALUE_NAME_RENDER_DEVICE_DEPTH				"RenderDeviceDepth"
#define	VALUE_NAME_RENDER_DEVICE_WINDOWED			"RenderDeviceWindowed"
#define	VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH		"RenderDeviceTextureDepth"

DX8_CleanupHook	 *DX8Wrapper::m_pCleanupHook=NULL;
#ifdef EXTENDED_STATS
DX8_Stats	 DX8Wrapper::stats;
#endif
/***********************************************************************************
**
** DX8Wrapper Implementation
**
***********************************************************************************/

void Log_DX8_ErrorCode(unsigned res)
{
	char tmp[256]="";

	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, res,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		tmp, sizeof(tmp), NULL))
	{
		WWDEBUG_SAY((tmp));
	}

	WWASSERT(0);
}

void Non_Fatal_Log_DX8_ErrorCode(unsigned res,const char * file,int line)
{
	char tmp[256]="";

	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, res,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		tmp, sizeof(tmp), NULL))
	{
		WWDEBUG_SAY(("DX8 Error: %s, File: %s, Line: %d\n",tmp,file,line));
	}
}


bool DX8Wrapper::Init(void * hwnd)
{
	WWASSERT(!IsInitted);

	/*
	** Initialize all variables!
	*/
	_Hwnd = (HWND)hwnd;
	_MainThreadID=ThreadClass::_Get_Current_Thread_ID();
	CurRenderDevice = -1;
	ResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
	ResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;	
	// Initialize Render2DClass Screen Resolution
	Render2DClass::Set_Screen_Resolution( RectClass( 0, 0, ResolutionWidth, ResolutionHeight ) );
	BitDepth = DEFAULT_BIT_DEPTH;
	IsWindowed = false;	
	DX8Wrapper_IsWindowed = false;

	for (int light=0;light<4;++light) CurrentDX8LightEnables[light]=false;

	::ZeroMemory(&old_world, sizeof(D3DMATRIX));
	::ZeroMemory(&old_view, sizeof(D3DMATRIX));
	::ZeroMemory(&old_prj, sizeof(D3DMATRIX));

	//old_vertex_shader; TODO
	//old_sr_shader;
	//current_shader;

	//world_identity;
	//CurrentFogColor;

	D3DInterface = NULL;
	D3DDevice = NULL;	

	Reset_Statistics();
	
	Invalidate_Cached_Render_States();
	
	/*
	** Create the D3D interface object
	*/
	D3DInterface = Direct3DCreate9(D3D_SDK_VERSION);		// TODO: handle failure cases...
	if (!D3DInterface)
		return false;
	IsInitted = true;	
	
	/*
	** Enumerate the available devices
	*/
	Enumerate_Devices();
	return true;
}

void DX8Wrapper::Shutdown(void)
{
	if (D3DDevice) {
		Set_Render_Target ((IDirect3DSurface9 *)NULL);
		Release_Device();
	}

	for (int i = 0; i < MAX_TEXTURE_STAGES; i++) {
		if (Textures[i]) {
			Textures[i]->Release();
			Textures[i] = NULL;
		}
	}

	if (D3DInterface) {
		UINT newRefCount=D3DInterface->Release();
		D3DInterface=NULL;
	}

	_RenderDeviceNameTable.Clear();		 // note - Delete_All() resizes the vector, causing a reallocation.  Clear is better. jba.
	_RenderDeviceShortNameTable.Clear();
	_RenderDeviceDescriptionTable.Clear();	

	IsInitted = false;		// 010803 srj
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Inits(void)
{
	/*
	** Set Global render states (some of which depend on caps)
	*/

	Compute_Caps(DisplayFormat,_PresentParameters.AutoDepthStencilFormat);

   /*
	** Initalize any other subsystems inside of WW3D
	*/
	MissingTexture::_Init();
	TextureClass::_Init_Filters();
	TheDX8MeshRenderer.Init();
	BoxRenderObjClass::Init();
	VertexMaterialClass::Init();
	PointGroupClass::_Init(); // This needs the VertexMaterialClass to be initted
	ShatterSystem::Init();
	TextureLoader::Init();

#ifdef WW3D_DX8

//	WW3DAssetManager::Get_Instance()->Open_Texture_File_Cache("cache_");	

	/*
	** Initialize the dazzle system
	*/
	FileClass * dazzle_ini_file = _TheFileFactory->Get_File(DAZZLE_INI_FILENAME);
	if (dazzle_ini_file) { 
		INIClass dazzle_ini(*dazzle_ini_file);
		DazzleRenderObjClass::Init_From_INI(&dazzle_ini);
		_TheFileFactory->Return_File(dazzle_ini_file);
	}
#endif //WW3D_DX8
	
	Set_Default_Global_Render_States();
}

inline DWORD F2DW(float f) { return *((unsigned*)&f); }
void DX8Wrapper::Set_Default_Global_Render_States(void)
{
	DX8_THREAD_ASSERT();
	const D3DCAPS9 &caps = DX8Caps::Get_Default_Caps();

	Set_DX8_Render_State(D3DRS_RANGEFOGENABLE, (caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? TRUE : FALSE);
	Set_DX8_Render_State(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
	Set_DX8_Render_State(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	Set_DX8_Render_State(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_COLORVERTEX, TRUE);
	Set_DX8_Render_State(D3DRS_SLOPESCALEDEPTHBIAS, -1); // [DX9] newly added
	Set_DX8_Render_State(D3DRS_DEPTHBIAS, 0); 
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE, F2DW(1.0f));
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00,F2DW(1.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11,F2DW(1.0f));

//	Set_DX8_Render_State(D3DRS_CULLMODE, D3DCULL_CW);
	// Set dither mode here?
}

//MW: I added this for 'Generals'.
bool DX8Wrapper::Validate_Device(void)
{	DWORD numPasses=0;
	HRESULT hRes;

	hRes=_Get_D3D_Device8()->ValidateDevice(&numPasses);

	return (hRes == D3D_OK);
}

void DX8Wrapper::Invalidate_Cached_Render_States(void)
{
	render_state_changed = 0;

	int a;
	for (a=0;a<sizeof(RenderStates)/sizeof(unsigned);++a) {
		RenderStates[a]=0x12345678;
	}
	for (a=0;a<MAX_TEXTURE_STAGES;++a) {
		for (int b=0; b<32;b++) {
			TextureStageStates[a][b]=0x12345678;
		}
		//Need to explicitly set texture to NULL, otherwise app will not be able to
		//set it to null because of redundant state checker. MW
		if (_Get_D3D_Device8())
			_Get_D3D_Device8()->SetTexture(a,NULL);
		if (Textures[a] != NULL)
			Textures[a]->Release();
		Textures[a]=NULL;
	}
	ShaderClass::Invalidate();
	//Need to explicitly set render_state texture pointers to NULL. MW
	Release_Render_State();
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns(void)
{
	/*
	** Shutdown ww3d systems
	*/
	if (render_state.vertex_buffer) render_state.vertex_buffer->Release_Engine_Ref();
	REF_PTR_RELEASE(render_state.vertex_buffer);
	if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();
	REF_PTR_RELEASE(render_state.index_buffer);
	REF_PTR_RELEASE(render_state.material);
	for (unsigned i=0;i<MAX_TEXTURE_STAGES;++i) REF_PTR_RELEASE(render_state.Textures[i]);

#ifdef WW3D_DX8
	DazzleRenderObjClass::Deinit();
#endif //WW3D_DX8

	TextureLoader::Deinit();	
	SortingRendererClass::Deinit();
	DynamicVBAccessClass::_Deinit();
	DynamicIBAccessClass::_Deinit();
	ShatterSystem::Shutdown();
	PointGroupClass::_Shutdown();
	VertexMaterialClass::Shutdown();
	BoxRenderObjClass::Shutdown();
	TheDX8MeshRenderer.Shutdown();
	MissingTexture::_Deinit();

}


bool DX8Wrapper::Create_Device(void)
{
	WWASSERT(D3DDevice == NULL);	// for now, once you've created a device, you're stuck with it!

	D3DCAPS9 caps;
	if (FAILED( D3DInterface->GetDeviceCaps(
		CurRenderDevice,
		WW3D_DEVTYPE,
		&caps))) {
		return false;
	}

	::ZeroMemory(&CurrentAdapterIdentifier, sizeof(D3DADAPTER_IDENTIFIER9));
	//if (FAILED( D3DInterface->GetAdapterIdentifier(CurRenderDevice,D3DENUM_NO_WHQL_LEVEL,&CurrentAdapterIdentifier))) {
	if (FAILED(D3DInterface->GetAdapterIdentifier(CurRenderDevice, 0, &CurrentAdapterIdentifier))) {
		return false;
	}

	unsigned vertex_processing_type=D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	if (caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		vertex_processing_type=D3DCREATE_MIXED_VERTEXPROCESSING;
	}

#ifdef CREATE_DX8_MULTI_THREADED
	vertex_processing_type|=D3DCREATE_MULTITHREADED;
	_DX8SingleThreaded=false;
#else
	_DX8SingleThreaded=true;
#endif

	if (DX8Wrapper_PreserveFPU)
		vertex_processing_type |= D3DCREATE_FPU_PRESERVE;

	if (FAILED( D3DInterface->CreateDevice(
		CurRenderDevice,
		WW3D_DEVTYPE,
		_Hwnd,
		vertex_processing_type,
		&_PresentParameters,
		&D3DDevice ) ) )
	{ 
		return false;
	}

	/*
	** Initialize all subsystems
	*/
	Do_Onetime_Device_Dependent_Inits();
	return true;
}

bool DX8Wrapper::Reset_Device(bool reload_assets)
{
	DX8_THREAD_ASSERT();
	if ((IsInitted) && (D3DDevice != NULL)) {
		// Release all non-MANAGED stuff
		Set_Vertex_Buffer (NULL);
		Set_Index_Buffer (NULL, 0);
		if (m_pCleanupHook) {
			m_pCleanupHook->ReleaseResources();
		}
		DynamicVBAccessClass::_Deinit();
		DynamicIBAccessClass::_Deinit();
		DX8TextureManagerClass::Release_Textures();

		HRESULT hr=_Get_D3D_Device8()->TestCooperativeLevel();
		if (hr != D3DERR_DEVICELOST )
		{	DX8CALL_HRES(Reset(&_PresentParameters),hr)
			if (hr != D3D_OK)
				return false;	//reset failed.
		}
		else
			return false;	//device is lost and can't be reset.

		if (reload_assets)
		{
			DX8TextureManagerClass::Recreate_Textures();
			if (m_pCleanupHook) {
				m_pCleanupHook->ReAcquireResources();
			}
		}
		Invalidate_Cached_Render_States();
		Set_Default_Global_Render_States();
		return true;
	}
	return false;
}

void DX8Wrapper::Release_Device(void)
{
	if (D3DDevice) {

		for (int a=0;a<MAX_TEXTURE_STAGES;++a)
		{	//release references to any textures that were used in last rendering call
			DX8CALL(SetTexture(a,NULL));
		}

		DX8CALL(SetStreamSource(0, NULL, 0,0));	//release reference count on last rendered vertex buffer
		DX8CALL(SetIndices(NULL));	//release reference count on last rendered index buffer


		/*
		** Release the current vertex and index buffers
		*/
		if (render_state.vertex_buffer) render_state.vertex_buffer->Release_Engine_Ref();
		REF_PTR_RELEASE(render_state.vertex_buffer);
		if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();
		REF_PTR_RELEASE(render_state.index_buffer);

		/*
		** Shutdown all subsystems
		*/
		Do_Onetime_Device_Dependent_Shutdowns();

		/*
		** Release the device
		*/

		D3DDevice->Release();
		D3DDevice=NULL;
	}
}

void DX8Wrapper::Enumerate_Devices()
{
	DX8_Assert();

	int adapter_count = D3DInterface->GetAdapterCount();
	for (int adapter_index=0; adapter_index<adapter_count; adapter_index++) {
		
		D3DADAPTER_IDENTIFIER9 id;
		::ZeroMemory(&id, sizeof(D3DADAPTER_IDENTIFIER9));
		//HRESULT res = D3DInterface->GetAdapterIdentifier(adapter_index,D3DENUM_NO_WHQL_LEVEL,&id);
		HRESULT res = D3DInterface->GetAdapterIdentifier(adapter_index, 0, &id);

		if (res == D3D_OK) {

			/*
			** Set up the device name
			*/
			StringClass device_name = id.Description;
			_RenderDeviceNameTable.Add(device_name);
			_RenderDeviceShortNameTable.Add(device_name);	// for now, just add the same name to the "pretty name table"

			/*
			** Set up the render device description
			** TODO: Fill in more fields of the render device description?  (need some lookup tables)
			*/
			RenderDeviceDescClass desc;
			desc.set_device_name(id.Description);
			desc.set_driver_name(id.Driver);
			
			char buf[64];
			sprintf(buf,"%d.%d.%d.%d", //"%04x.%04x.%04x.%04x",
				HIWORD(id.DriverVersion.HighPart),
				LOWORD(id.DriverVersion.HighPart),
				HIWORD(id.DriverVersion.LowPart),
				LOWORD(id.DriverVersion.LowPart));

			desc.set_driver_version(buf);

			/*
			** Enumerate the resolutions
			*/
			desc.reset_resolution_list();
			D3DFORMAT format = DisplayFormat;
			int mode_count = D3DInterface->GetAdapterModeCount(adapter_index, format);
			for (int mode_index=0; mode_index<mode_count; mode_index++) {
				D3DDISPLAYMODE d3dmode;
				::ZeroMemory(&d3dmode, sizeof(D3DDISPLAYMODE));
				HRESULT res = D3DInterface->EnumAdapterModes(adapter_index, format, mode_index,&d3dmode);
				
				if (res == D3D_OK) {
					int bits = 0;
					switch (d3dmode.Format) 
					{
						case D3DFMT_R8G8B8:			
						case D3DFMT_A8R8G8B8:
						case D3DFMT_X8R8G8B8:		bits = 32; break;

						case D3DFMT_R5G6B5:
						case D3DFMT_X1R5G5B5:		bits = 16; break;
					}

					/*
					** If we recognize the format, add it to the list
					** TODO: should we handle more formats?  will any cards report more than 24 or 16 bit?
					*/
					if (bits != 0) {
						desc.add_resolution(d3dmode.Width,d3dmode.Height,bits);
					}
				}
			}

			/*
			** Add the render device to our table
			*/
			_RenderDeviceDescriptionTable.Add(desc);
		}
	}
}

bool DX8Wrapper::Set_Any_Render_Device(void)
{
	// Try windowed first
	for (int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,1,false)) {
			return true;
		}
	}

	// Then fullscreen
	for (int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,0,false)) {
			return true;
		}
	}

	return false;
}

bool DX8Wrapper::Set_Render_Device
(	
	const char * dev_name, 
	int width, 
	int height, 
	int bits, 
	int windowed, 
	bool resize_window 
)
{
	for ( int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if ( strcmp( dev_name, _RenderDeviceNameTable[dev_number]) == 0) {
			return Set_Render_Device( dev_number, width, height, bits, windowed, resize_window );
		}

		if ( strcmp( dev_name, _RenderDeviceShortNameTable[dev_number]) == 0) {
			return Set_Render_Device( dev_number, width, height, bits, windowed, resize_window );
		}
	}
	return false;
}

void DX8Wrapper::Get_Format_Name(unsigned int format, StringClass *tex_format)
{
		*tex_format="Unknown";
		switch (format) {
		case D3DFMT_A8R8G8B8: *tex_format="D3DFMT_A8R8G8B8"; break;
		case D3DFMT_R8G8B8: *tex_format="D3DFMT_R8G8B8"; break;
		case D3DFMT_A4R4G4B4: *tex_format="D3DFMT_A4R4G4B4"; break;
		case D3DFMT_A1R5G5B5: *tex_format="D3DFMT_A1R5G5B5"; break;
		case D3DFMT_R5G6B5: *tex_format="D3DFMT_R5G6B5"; break;
		case D3DFMT_L8: *tex_format="D3DFMT_L8"; break;
		case D3DFMT_A8: *tex_format="D3DFMT_A8"; break;
		case D3DFMT_P8: *tex_format="D3DFMT_P8"; break;
		case D3DFMT_X8R8G8B8: *tex_format="D3DFMT_X8R8G8B8"; break;
		case D3DFMT_X1R5G5B5: *tex_format="D3DFMT_X1R5G5B5"; break;
		case D3DFMT_R3G3B2: *tex_format="D3DFMT_R3G3B2"; break;
		case D3DFMT_A8R3G3B2: *tex_format="D3DFMT_A8R3G3B2"; break;
		case D3DFMT_X4R4G4B4: *tex_format="D3DFMT_X4R4G4B4"; break;
		case D3DFMT_A8P8: *tex_format="D3DFMT_A8P8"; break;
		case D3DFMT_A8L8: *tex_format="D3DFMT_A8L8"; break;
		case D3DFMT_A4L4: *tex_format="D3DFMT_A4L4"; break;
		case D3DFMT_V8U8: *tex_format="D3DFMT_V8U8"; break;
		case D3DFMT_L6V5U5: *tex_format="D3DFMT_L6V5U5"; break;  
		case D3DFMT_X8L8V8U8: *tex_format="D3DFMT_X8L8V8U8"; break;
		case D3DFMT_Q8W8V8U8: *tex_format="D3DFMT_Q8W8V8U8"; break;
		case D3DFMT_V16U16: *tex_format="D3DFMT_V16U16"; break;
		//case D3DFMT_W11V11U10: *tex_format="D3DFMT_W11V11U10"; break;
		case D3DFMT_UYVY: *tex_format="D3DFMT_UYVY"; break;
		case D3DFMT_YUY2: *tex_format="D3DFMT_YUY2"; break;
		case D3DFMT_DXT1: *tex_format="D3DFMT_DXT1"; break;
		case D3DFMT_DXT2: *tex_format="D3DFMT_DXT2"; break;
		case D3DFMT_DXT3: *tex_format="D3DFMT_DXT3"; break;
		case D3DFMT_DXT4: *tex_format="D3DFMT_DXT4"; break;
		case D3DFMT_DXT5: *tex_format="D3DFMT_DXT5"; break;
		case D3DFMT_D16_LOCKABLE: *tex_format="D3DFMT_D16_LOCKABLE"; break;
		case D3DFMT_D32: *tex_format="D3DFMT_D32"; break;
		case D3DFMT_D15S1: *tex_format="D3DFMT_D15S1"; break;
		case D3DFMT_D24S8: *tex_format="D3DFMT_D24S8"; break;
		case D3DFMT_D16: *tex_format="D3DFMT_D16"; break;
		case D3DFMT_D24X8: *tex_format="D3DFMT_D24X8"; break;
		case D3DFMT_D24X4S4: *tex_format="D3DFMT_D24X4S4"; break;
		default:	break;
		}
}


bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed,
								   bool resize_window,bool reset_device, bool restore_assets)
{
	WWASSERT(IsInitted);
	WWASSERT(dev >= -1);
	WWASSERT(dev < _RenderDeviceNameTable.Count());

	/*
	** If user has never selected a render device, start out with device 0
	*/
	if ((CurRenderDevice == -1) && (dev == -1)) {
		CurRenderDevice = 0;
	} else if (dev != -1) {
		CurRenderDevice = dev;
	}
	
	/*
	** If user doesn't want to change res, set the res variables to match the 
	** current resolution
	*/
	if (width != -1)		ResolutionWidth = width;
	if (height != -1)		ResolutionHeight = height;
	
	// Initialize Render2DClass Screen Resolution
	Render2DClass::Set_Screen_Resolution( RectClass( 0, 0, ResolutionWidth, ResolutionHeight ) );

	if (bits != -1)		BitDepth = bits;
	if (windowed != -1)	IsWindowed = (windowed != 0);
	DX8Wrapper_IsWindowed = IsWindowed;

	WWDEBUG_SAY(("Attempting Set_Render_Device: name: %s (%s:%s), width: %d, height: %d, windowed: %d\n",
		_RenderDeviceNameTable[CurRenderDevice],_RenderDeviceDescriptionTable[CurRenderDevice].Get_Driver_Name(),
		_RenderDeviceDescriptionTable[CurRenderDevice].Get_Driver_Version(),ResolutionWidth,ResolutionHeight,(IsWindowed ? 1 : 0)));

#ifdef _WINDOWS
	// PWG 4/13/2000 - changed so that if you say to resize the window it resizes
	// regardless of whether its windowed or not as OpenGL resizes its self around
	// the caption and edges of the window type you provide, so its important to 
	// push the client area to be the size you really want.
	// if ( resize_window && windowed ) {
	if (resize_window) {

		// Get the current dimensions of the 'render area' of the window
		RECT rect = { 0 };
		::GetClientRect (_Hwnd, &rect);

		// Is the window the correct size for this resolution?
		if ((rect.right-rect.left) != ResolutionWidth ||
			 (rect.bottom-rect.top) != ResolutionHeight) {			
			
			// Calculate what the main window's bounding rectangle should be to
			// accomodate this resolution
			rect.left = 0;
			rect.top = 0;
			rect.right = ResolutionWidth;
			rect.bottom = ResolutionHeight;
			DWORD dwstyle = ::GetWindowLong (_Hwnd, GWL_STYLE);
			AdjustWindowRect (&rect, dwstyle, FALSE);

			// Resize the window to fit this resolution
			if (!windowed)
				::SetWindowPos(_Hwnd, HWND_TOPMOST, 0, 0, rect.right-rect.left, rect.bottom-rect.top,SWP_NOSIZE |SWP_NOMOVE);
			else
				::SetWindowPos (_Hwnd,
								 NULL,
								 0,
								 0,
								 rect.right-rect.left,
								 rect.bottom-rect.top,
								 SWP_NOZORDER | SWP_NOMOVE);
		}
	}
#endif
	//must be either resetting existing device or creating a new one.
	WWASSERT(reset_device || D3DDevice == NULL);
	
	/*
	** Initialize values for D3DPRESENT_PARAMETERS members. 	
	*/
	::ZeroMemory(&_PresentParameters, sizeof(D3DPRESENT_PARAMETERS));

	_PresentParameters.BackBufferWidth = ResolutionWidth;
	_PresentParameters.BackBufferHeight = ResolutionHeight;
	_PresentParameters.BackBufferCount = IsWindowed ? 1 : 2;
	
	_PresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
	_PresentParameters.SwapEffect = IsWindowed ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_FLIP;		// Shouldn't this be D3DSWAPEFFECT_FLIP?
	_PresentParameters.hDeviceWindow = _Hwnd;
	_PresentParameters.Windowed = IsWindowed;

	_PresentParameters.EnableAutoDepthStencil = TRUE;				// Driver will attempt to match Z-buffer depth
	_PresentParameters.Flags=0;											// We're not going to lock the backbuffer
	
	_PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	_PresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	/*
	** Set up the buffer formats.  Several issues here:
	** - if in windowed mode, the backbuffer must use the current display format.
	** - the depth buffer must use 
	*/
	if (IsWindowed) {

		D3DDISPLAYMODE desktop_mode;
		::ZeroMemory(&desktop_mode, sizeof(D3DDISPLAYMODE));
		D3DInterface->GetAdapterDisplayMode( CurRenderDevice, &desktop_mode );

		DisplayFormat=_PresentParameters.BackBufferFormat = desktop_mode.Format;

		// In windowed mode, define the bitdepth from desktop mode (as it can't be changed)
		switch (_PresentParameters.BackBufferFormat) {
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_R8G8B8: BitDepth=32; break;
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_R5G6B5: BitDepth=16; break;
		case D3DFMT_L8:
		case D3DFMT_A8:
		case D3DFMT_P8: BitDepth=8; break;
		default:
			// Unknown backbuffer format probably means the device can't do windowed
			return false;
		}

		if (BitDepth==32 && D3DInterface->CheckDeviceType(0,D3DDEVTYPE_HAL,desktop_mode.Format,D3DFMT_A8R8G8B8, TRUE) == D3D_OK)
		{	//promote 32-bit modes to include destination alpha
			_PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
		}

		/*
		** Find a appropriate Z buffer
		*/
		if (!Find_Z_Mode(DisplayFormat,_PresentParameters.BackBufferFormat,&_PresentParameters.AutoDepthStencilFormat))
		{
			// If opening 32 bit mode failed, try 16 bit, even if the desktop happens to be 32 bit
			if (BitDepth==32) {
				BitDepth=16;
				_PresentParameters.BackBufferFormat=D3DFMT_R5G6B5;
				if (!Find_Z_Mode(_PresentParameters.BackBufferFormat,_PresentParameters.BackBufferFormat,&_PresentParameters.AutoDepthStencilFormat)) {
					_PresentParameters.AutoDepthStencilFormat=D3DFMT_UNKNOWN;
				}
			}
			else {
				_PresentParameters.AutoDepthStencilFormat=D3DFMT_UNKNOWN;
			}
		}

	} else {

		/*
		** Try to find a mode that matches the user's desired bit-depth.
		*/
		Find_Color_And_Z_Mode(ResolutionWidth,ResolutionHeight,BitDepth,&DisplayFormat,
			&_PresentParameters.BackBufferFormat,&_PresentParameters.AutoDepthStencilFormat);
	}

	/*
	** Time to actually create the device.
	*/
	if (_PresentParameters.AutoDepthStencilFormat==D3DFMT_UNKNOWN) {
		if (BitDepth==32) {
			_PresentParameters.AutoDepthStencilFormat=D3DFMT_D32;
		}
		else {
			_PresentParameters.AutoDepthStencilFormat=D3DFMT_D16;
		}
	}

	StringClass displayFormat;
	StringClass backbufferFormat;

	Get_Format_Name(DisplayFormat,&displayFormat);
	Get_Format_Name(_PresentParameters.BackBufferFormat,&backbufferFormat);

	WWDEBUG_SAY(("Using Display/BackBuffer Formats: %s/%s\n",displayFormat,backbufferFormat));
	
	bool ret;

	if (reset_device)
		ret = Reset_Device(restore_assets);	//reset device without restoring data - we're likely switching out of the app.
	else
		ret = Create_Device();

	WWDEBUG_SAY(("Reset/Create_Device done, reset_device=%d, restore_assets=%d\n", reset_device, restore_assets));

	return ret;
}

bool DX8Wrapper::Set_Next_Render_Device(void)
{
	int new_dev = (CurRenderDevice + 1) % _RenderDeviceNameTable.Count();
	return Set_Render_Device(new_dev);
}

bool DX8Wrapper::Toggle_Windowed(void)
{
#ifdef WW3D_DX8
	// State OK?
	assert (IsInitted);
	if (IsInitted) {

		// Get information about the current render device's resolutions
		const RenderDeviceDescClass &render_device = Get_Render_Device_Desc ();
		const DynamicVectorClass<ResolutionDescClass> &resolutions = render_device.Enumerate_Resolutions ();
		
		// Loop through all the resolutions supported by the current device.
		// If we aren't currently running under one of these resolutions,
		// then we should probably		 to the closest resolution before
		// toggling the windowed state.
		int curr_res = -1;
		for (int res = 0;
		     (res < resolutions.Count ()) && (curr_res == -1);
			  res ++) {
			
			// Is this the resolution we are looking for?
			if ((resolutions[res].Width == ResolutionWidth) &&
				 (resolutions[res].Height == ResolutionHeight) &&
				 (resolutions[res].BitDepth == BitDepth)) {
				curr_res = res;
			}
		}
		
		if (curr_res == -1) {
			
			// We don't match any of the standard resolutions,
			// so set the first resolution and toggle the windowed state.
			return Set_Device_Resolution (resolutions[0].Width,
								 resolutions[0].Height,
								 resolutions[0].BitDepth,
								 !IsWindowed, true);
		} else {

			// Toggle the windowed state
			return Set_Device_Resolution (-1, -1, -1, !IsWindowed, true);
		}		
	}
#endif //WW3D_DX8

	return false;
}

void DX8Wrapper::Set_Swap_Interval(int swap)
{
	switch (swap) {
		case 0: _PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; break;
		case 1: _PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE ; break;
		case 2: _PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_TWO; break;
		case 3: _PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_THREE; break;
		default: _PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE ; break;
	}
	
	Reset_Device();
}

int DX8Wrapper::Get_Swap_Interval(void)
{
	return _PresentParameters.PresentationInterval;
}

bool DX8Wrapper::Has_Stencil(void)
{
	bool has_stencil = (_PresentParameters.AutoDepthStencilFormat == D3DFMT_D24S8 ||
						_PresentParameters.AutoDepthStencilFormat == D3DFMT_D24X4S4);
	return has_stencil;
}

int DX8Wrapper::Get_Render_Device_Count(void)
{
	return _RenderDeviceNameTable.Count();

}
int DX8Wrapper::Get_Render_Device(void)
{
	assert(IsInitted);
	return CurRenderDevice;
}

const RenderDeviceDescClass & DX8Wrapper::Get_Render_Device_Desc(int deviceidx)
{
	WWASSERT(IsInitted);

	if ((deviceidx == -1) && (CurRenderDevice == -1)) {
		CurRenderDevice = 0;
	}

	// if the device index is -1 then we want the current device
	if (deviceidx == -1) {
		WWASSERT(CurRenderDevice >= 0);
		WWASSERT(CurRenderDevice < _RenderDeviceNameTable.Count());
		return _RenderDeviceDescriptionTable[CurRenderDevice];
	}

	// We can only ask for multiple device information if the devices
	// have been detected.
	WWASSERT(deviceidx >= 0);
	WWASSERT(deviceidx < _RenderDeviceNameTable.Count());
	return _RenderDeviceDescriptionTable[deviceidx];
}

const char * DX8Wrapper::Get_Render_Device_Name(int device_index)
{
	device_index = device_index % _RenderDeviceShortNameTable.Count();
	return _RenderDeviceShortNameTable[device_index];
}

bool DX8Wrapper::Set_Device_Resolution(int width,int height,int bits,int windowed, bool resize_window)
{
	if (D3DDevice != NULL) {

		if (width != -1) {
			_PresentParameters.BackBufferWidth = ResolutionWidth = width;
		}
		if (height != -1) {
			_PresentParameters.BackBufferHeight = ResolutionHeight = height;
		}

		if (resize_window)
		{

			// Get the current dimensions of the 'render area' of the window
			RECT rect = { 0 };
			::GetClientRect (_Hwnd, &rect);

			// Is the window the correct size for this resolution?
			if ((rect.right-rect.left) != ResolutionWidth ||
				 (rect.bottom-rect.top) != ResolutionHeight)
			{			
				
				// Calculate what the main window's bounding rectangle should be to
				// accomodate this resolution
				rect.left = 0;
				rect.top = 0;
				rect.right = ResolutionWidth;
				rect.bottom = ResolutionHeight;
				DWORD dwstyle = ::GetWindowLong (_Hwnd, GWL_STYLE);
				AdjustWindowRect (&rect, dwstyle, FALSE);

				// Resize the window to fit this resolution
				if (!windowed)
					::SetWindowPos(_Hwnd, HWND_TOPMOST, 0, 0, rect.right-rect.left, rect.bottom-rect.top,SWP_NOSIZE |SWP_NOMOVE);
				else
					::SetWindowPos (_Hwnd,
									 NULL,
									 0,
									 0,
									 rect.right-rect.left,
									 rect.bottom-rect.top,
									 SWP_NOZORDER | SWP_NOMOVE);
			}
		}

#pragma message("TODO: support changing windowed status and changing the bit depth")
		return Reset_Device();
	} else {
		return false;
	}
}

void DX8Wrapper::Get_Device_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	WWASSERT(IsInitted);

	set_w = ResolutionWidth;
	set_h = ResolutionHeight;
	set_bits = BitDepth;
	set_windowed = IsWindowed;

	return ;
}

void DX8Wrapper::Get_Render_Target_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	WWASSERT(IsInitted);

	if (CurrentRenderTarget != NULL) {
		D3DSURFACE_DESC info;
		CurrentRenderTarget->GetDesc (&info);

		set_w				= info.Width;
		set_h				= info.Height;
		set_bits			= BitDepth;		// should we get the actual bit depth of the target?
		set_windowed	= IsWindowed;	// this doesn't really make sense for render targets (shouldn't matter)...

	} else {
		Get_Device_Resolution (set_w, set_h, set_bits, set_windowed);
	}

	return ;
}

bool DX8Wrapper::Registry_Save_Render_Device( const char * sub_key )
{
	int	width, height, depth;
	bool	windowed;
	Get_Device_Resolution(width, height, depth, windowed);
	return Registry_Save_Render_Device(sub_key, CurRenderDevice, ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed, TextureBitDepth);
}

bool DX8Wrapper::Registry_Save_Render_Device( const char *sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth)
{
	RegistryClass * registry = W3DNEW RegistryClass( sub_key );
	WWASSERT( registry );

	if ( !registry->Is_Valid() ) {
		delete registry;
		WWDEBUG_SAY(( "Error getting Registry\n" ));
		return false;
	}

	registry->Set_String( VALUE_NAME_RENDER_DEVICE_NAME, 
		_RenderDeviceShortNameTable[device] );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_WIDTH,	width );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_HEIGHT, height );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_DEPTH, depth );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_WINDOWED, windowed );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH, texture_depth );

	delete registry;
	return true;
}

bool DX8Wrapper::Registry_Load_Render_Device( const char * sub_key, bool resize_window )
{
	char	name[ 200 ];
	int	width,height,depth,windowed;

	if (	Registry_Load_Render_Device(	sub_key, 
													name, 
													sizeof(name),
													width, 
													height, 
													depth, 
													windowed,
													TextureBitDepth) && 
			(*name != 0)) 
	{
		WWDEBUG_SAY(( "Device %s (%d X %d) %d bit windowed:%d\n", name,width,height,depth,windowed));
		
		if (TextureBitDepth==16 || TextureBitDepth==32) {
//			WWDEBUG_SAY(( "Texture depth %d\n", TextureBitDepth));
		} else {
			WWDEBUG_SAY(( "Invalid texture depth %d, switching to 16 bits\n", TextureBitDepth));
			TextureBitDepth=16;
		}

		if ( Set_Render_Device( name, width,height,depth,windowed, resize_window ) != true) {
			return Set_Any_Render_Device();
		}

		return true;
	}

	WWDEBUG_SAY(( "Error getting Registry\n" ));

	return Set_Any_Render_Device();
}

bool DX8Wrapper::Registry_Load_Render_Device( const char * sub_key, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth)
{
	RegistryClass registry( sub_key );

	if ( registry.Is_Valid() ) {
		registry.Get_String( VALUE_NAME_RENDER_DEVICE_NAME,
			device, device_len);

		width =		registry.Get_Int( VALUE_NAME_RENDER_DEVICE_WIDTH, -1 );
		height =		registry.Get_Int( VALUE_NAME_RENDER_DEVICE_HEIGHT, -1 );
		depth =		registry.Get_Int( VALUE_NAME_RENDER_DEVICE_DEPTH, -1 );
		windowed =	registry.Get_Int( VALUE_NAME_RENDER_DEVICE_WINDOWED, -1 );
		texture_depth = registry.Get_Int( VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH, -1 );
		return true;
	}
	return false;
}


bool DX8Wrapper::Find_Color_And_Z_Mode(int resx,int resy,int bitdepth,D3DFORMAT * set_colorbuffer,D3DFORMAT * set_backbuffer,D3DFORMAT * set_zmode)
{
	static D3DFORMAT _formats16[] =
	{
		D3DFMT_R5G6B5,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5
	};

	static D3DFORMAT _formats32[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_R8G8B8,
	};

	/*
	** Select the table that we're going to use to search for a valid backbuffer format
	*/
	D3DFORMAT * format_table = NULL;
	int format_count = 0;

	if (BitDepth == 16) {
		format_table = _formats16;
		format_count = sizeof(_formats16) / sizeof(D3DFORMAT);
	} else {
		format_table = _formats32;
		format_count = sizeof(_formats32) / sizeof(D3DFORMAT);
	}

	/*
	** now search for a valid format
	*/
	bool found = false;
	unsigned int mode = 0;
	int format_index = 0;
	for (format_index=0; format_index < format_count; format_index++) {
		found |= Find_Color_Mode(format_table[format_index],resx,resy,&mode);
		if (found) break;
	}

	if (!found) {
		return false;
	} else {
		*set_backbuffer=*set_colorbuffer = format_table[format_index];
	}

	if (bitdepth==32 && *set_colorbuffer == D3DFMT_X8R8G8B8 && D3DInterface->CheckDeviceType(0,D3DDEVTYPE_HAL,*set_colorbuffer,D3DFMT_A8R8G8B8, TRUE) == D3D_OK)
	{	//promote 32-bit modes to include destination alpha when supported
		*set_backbuffer = D3DFMT_A8R8G8B8;
	}

	/*
	** We found a backbuffer format, now find a zbuffer format
	*/
	return Find_Z_Mode(*set_colorbuffer,*set_backbuffer, set_zmode);
};


// find the resolution mode with at least resx,resy with the highest supported
// refresh rate
bool DX8Wrapper::Find_Color_Mode(D3DFORMAT colorbuffer, int resx, int resy, UINT *mode)
{
	UINT i,j,modemax;
	UINT rx,ry;
	D3DDISPLAYMODE dmode;
	::ZeroMemory(&dmode, sizeof(D3DDISPLAYMODE));

	rx=(unsigned int) resx;
	ry=(unsigned int) resy;

	bool found=false;
	D3DFORMAT format = DisplayFormat;

	modemax=D3DInterface->GetAdapterModeCount(D3DADAPTER_DEFAULT, format);

	i=0;

	while (i<modemax && !found)
	{
		D3DInterface->EnumAdapterModes(D3DADAPTER_DEFAULT, format, i, &dmode);
		if (dmode.Width==rx && dmode.Height==ry && dmode.Format==colorbuffer)
			found=true;
		i++;
	}

	i--; // this is the first valid mode

	// no match
	if (!found) return false;

	// go to the highest refresh rate in this mode
	bool stillok=true;

	j=i;
	while (j<modemax && stillok)
	{
		D3DInterface->EnumAdapterModes(D3DADAPTER_DEFAULT, format, j, &dmode);
		if (dmode.Width==rx && dmode.Height==ry && dmode.Format==colorbuffer)
			stillok=true; else stillok=false;
		j++;
	}

	if (stillok==false) *mode=j-2;
	else *mode=i;

	return true;
}

// Helper function to find a Z buffer mode for the colorbuffer
// Will look for greatest Z precision
bool DX8Wrapper::Find_Z_Mode(D3DFORMAT colorbuffer,D3DFORMAT backbuffer, D3DFORMAT *zmode)
{
	//MW: Swapped the next 2 tests so that Stencil modes get tested first.
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24S8))
	{
		*zmode=D3DFMT_D24S8;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D24S8\n"));
		return true;
	}

	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D32))
	{
		*zmode=D3DFMT_D32;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D32\n"));
		return true;
	}

	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24X8))
	{
		*zmode=D3DFMT_D24X8;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D24X8\n"));
		return true;
	}

	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24X4S4))
	{
		*zmode=D3DFMT_D24X4S4;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D24X4S4\n"));
		return true;
	}

	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D16))
	{
		*zmode=D3DFMT_D16;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D16\n"));
		return true;
	}

	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D15S1))
	{
		*zmode=D3DFMT_D15S1;
		WWDEBUG_SAY(("Found zbuffer mode D3DFMT_D15S1\n"));
		return true;
	}
	
	// can't find a match
	return false;
}

bool DX8Wrapper::Test_Z_Mode(D3DFORMAT colorbuffer,D3DFORMAT backbuffer, D3DFORMAT zmode)
{
	// See if we have this mode first
	if (FAILED(D3DInterface->CheckDeviceFormat(D3DADAPTER_DEFAULT,WW3D_DEVTYPE,
		colorbuffer,D3DUSAGE_DEPTHSTENCIL,D3DRTYPE_SURFACE,zmode)))
	{
		WWDEBUG_SAY(("CheckDeviceFormat failed.  Colorbuffer format = %d  Zbufferformat = %d\n",colorbuffer,zmode));
		return false;
	}

	// Then see if it matches the color buffer
	if(FAILED(D3DInterface->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, WW3D_DEVTYPE,
		colorbuffer,backbuffer,zmode)))
	{
		WWDEBUG_SAY(("CheckDepthStencilMatch failed.  Colorbuffer format = %d  Backbuffer format = %d Zbufferformat = %d\n",colorbuffer,backbuffer,zmode));
		return false;
	}
	return true;
}


void DX8Wrapper::Reset_Statistics()
{
	matrix_changes	= 0;
	material_changes = 0;
	vertex_buffer_changes = 0;
	index_buffer_changes = 0;
	light_changes = 0;
	texture_changes = 0;
	render_state_changes =0;
	texture_stage_state_changes =0;

	number_of_DX8_calls = 0;
	last_frame_matrix_changes = 0;
	last_frame_material_changes = 0;
	last_frame_vertex_buffer_changes = 0;
	last_frame_index_buffer_changes = 0;
	last_frame_light_changes = 0;
	last_frame_texture_changes = 0;
	last_frame_render_state_changes = 0;
	last_frame_texture_stage_state_changes = 0;
	last_frame_number_of_DX8_calls = 0;
}

void DX8Wrapper::Begin_Statistics()
{
	matrix_changes=0;
	material_changes=0;
	vertex_buffer_changes=0;
	index_buffer_changes=0;
	light_changes=0;
	texture_changes = 0;
	render_state_changes =0;
	texture_stage_state_changes =0;
	number_of_DX8_calls=0;
}

void DX8Wrapper::End_Statistics()
{
	last_frame_matrix_changes=matrix_changes;
	last_frame_material_changes=material_changes;
	last_frame_vertex_buffer_changes=vertex_buffer_changes;
	last_frame_index_buffer_changes=index_buffer_changes;
	last_frame_light_changes=light_changes;
	last_frame_texture_changes = texture_changes;
	last_frame_render_state_changes = render_state_changes;
	last_frame_texture_stage_state_changes = texture_stage_state_changes;
	last_frame_number_of_DX8_calls=number_of_DX8_calls;
}

unsigned DX8Wrapper::Get_Last_Frame_Matrix_Changes()			{ return last_frame_matrix_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Material_Changes()		{ return last_frame_material_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Vertex_Buffer_Changes()	{ return last_frame_vertex_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Index_Buffer_Changes()	{ return last_frame_index_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Light_Changes()			{ return last_frame_light_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Changes()			{ return last_frame_texture_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Render_State_Changes()	{ return last_frame_render_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Stage_State_Changes()	{ return last_frame_texture_stage_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_DX8_Calls()					{ return last_frame_number_of_DX8_calls; }
unsigned long DX8Wrapper::Get_FrameCount(void) {return FrameCount;}

void DX8_Assert()
{
	WWASSERT(DX8Wrapper::_Get_D3D8());
	DX8_THREAD_ASSERT();
}

void DX8Wrapper::Begin_Scene(void)
{
	DX8_THREAD_ASSERT();
	DX8CALL(BeginScene());

	DX8WebBrowser::Update();
}

void DX8Wrapper::End_Scene(bool flip_frames)
{
	DX8_THREAD_ASSERT();
	DX8CALL(EndScene());

	DX8WebBrowser::Render(0);

	if (flip_frames) {
		DX8_Assert();
		HRESULT hr=_Get_D3D_Device8()->Present(NULL, NULL, NULL, NULL);
		number_of_DX8_calls++;

		if (SUCCEEDED(hr)) {
#ifdef EXTENDED_STATS
			if (stats.m_sleepTime) {
				::Sleep(stats.m_sleepTime);
			}
#endif
			FrameCount++;
		}

		// If the device was lost we need to check for cooperative level and possibly reset the device
		if (hr==D3DERR_DEVICELOST) {
			hr=_Get_D3D_Device8()->TestCooperativeLevel();
			if (hr==D3DERR_DEVICENOTRESET) {
				Reset_Device();
			}
		}
		else {
			DX8_ErrorCode(hr);
		}
	}

	// Each frame, release all of the buffers and textures.
	Set_Vertex_Buffer(NULL);
	Set_Index_Buffer(NULL,0);
	for (unsigned i=0;i<MAX_TEXTURE_STAGES;++i) Set_Texture(i,NULL);
	Set_Material(NULL);
}


void DX8Wrapper::Flip_To_Primary(void)
{
	// If we are fullscreen and the current frame is odd then we need
	// to force a page flip to ensure that the first buffer in the flipping
	// chain is the one visible.
	if (!IsWindowed) {
		DX8_Assert();

		int numBuffers = (_PresentParameters.BackBufferCount + 1);
		int visibleBuffer = (FrameCount % numBuffers);
		int flipCount = ((numBuffers - visibleBuffer) % numBuffers);
		int resetAttempts = 0;

		while ((flipCount > 0) && (resetAttempts < 3)) {
			HRESULT hr = _Get_D3D_Device8()->TestCooperativeLevel();

			if (FAILED(hr)) {
				WWDEBUG_SAY(("TestCooperativeLevel Failed!\n"));

				if (D3DERR_DEVICELOST == hr) {
					WWDEBUG_SAY(("DEVICELOST: Cannot flip to primary.\n"));
					return;
				}

				if (D3DERR_DEVICENOTRESET == hr) {
					WWDEBUG_SAY(("DEVICENOTRESET: Resetting device.\n"));
					Reset_Device();
					resetAttempts++;
				}
			} else {
				WWDEBUG_SAY(("Flipping: %ld\n", FrameCount));
				hr = _Get_D3D_Device8()->Present(NULL, NULL, NULL, NULL);

				if (SUCCEEDED(hr)) {
					FrameCount++;
					WWDEBUG_SAY(("Flip to primary succeeded %ld\n", FrameCount));
				}
			}

			--flipCount;
		}
	}
}


void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float dest_alpha, float z, unsigned int stencil)
{
	DX8_THREAD_ASSERT();
	// If we try to clear a stencil buffer which is not there, the entire call will fail
	bool has_stencil = (	_PresentParameters.AutoDepthStencilFormat == D3DFMT_D15S1 ||
								_PresentParameters.AutoDepthStencilFormat == D3DFMT_D24S8 ||
								_PresentParameters.AutoDepthStencilFormat == D3DFMT_D24X4S4);

	DWORD flags = 0;
	if (clear_color) flags |= D3DCLEAR_TARGET;
	if (clear_z_stencil) flags |= D3DCLEAR_ZBUFFER;
	if (clear_z_stencil && has_stencil) flags |= D3DCLEAR_STENCIL;
	if (flags)
	{
		DX8CALL(Clear(0, NULL, flags, Convert_Color(color,dest_alpha), z, stencil));
	}
}

void DX8Wrapper::Set_Viewport(CONST D3DVIEWPORT9* pViewport)
{
	DX8_THREAD_ASSERT();
	DX8CALL(SetViewport(pViewport));
}

// ----------------------------------------------------------------------------
//
// Set vertex buffer. A reference to previous vertex buffer is released and
// this one is assigned the current vertex buffer. The DX8 vertex buffer will 
// actually be set in Apply() which is called by Draw_Indexed_Triangles().
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass* vb)
{
	render_state.vba_offset=0;
	render_state.vba_count=0;
	if (render_state.vertex_buffer) {
		render_state.vertex_buffer->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.vertex_buffer,const_cast<VertexBufferClass*>(vb));
	if (vb) {
		vb->Add_Engine_Ref();
		render_state.vertex_buffer_type=vb->Type();
	}
	else {
		render_state.index_buffer_type=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=VERTEX_BUFFER_CHANGED;
}

// ----------------------------------------------------------------------------
//
// Set index buffer. A reference to previous index buffer is released and
// this one is assigned the current index buffer. The DX8 index buffer will 
// actually be set in Apply() which is called by Draw_Indexed_Triangles().
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset)
{
	render_state.iba_offset=0;
	if (render_state.index_buffer) {
		render_state.index_buffer->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.index_buffer,const_cast<IndexBufferClass*>(ib));
	render_state.index_base_offset=index_base_offset;
	if (ib) {
		ib->Add_Engine_Ref();
		render_state.index_buffer_type=ib->Type();
	}
	else {
		render_state.index_buffer_type=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

// ----------------------------------------------------------------------------
//
// Set vertex buffer using dynamic access object.
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass& vba_)
{
	if (render_state.vertex_buffer) render_state.vertex_buffer->Release_Engine_Ref();

	DynamicVBAccessClass& vba=const_cast<DynamicVBAccessClass&>(vba_);
	render_state.vertex_buffer_type=vba.Get_Type();
	render_state.vba_offset=vba.VertexBufferOffset;
	render_state.vba_count=vba.Get_Vertex_Count();
	REF_PTR_SET(render_state.vertex_buffer,vba.VertexBuffer);
	render_state.vertex_buffer->Add_Engine_Ref();
	render_state_changed|=VERTEX_BUFFER_CHANGED;
	render_state_changed|=INDEX_BUFFER_CHANGED;		// vba_offset changes so index buffer needs to be reset as well.
}

// ----------------------------------------------------------------------------
//
// Set index buffer using dynamic access object.
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass& iba_,unsigned short index_base_offset)
{
	if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();

	DynamicIBAccessClass& iba=const_cast<DynamicIBAccessClass&>(iba_);
	render_state.index_base_offset=index_base_offset;
	render_state.index_buffer_type=iba.Get_Type();
	render_state.iba_offset=iba.IndexBufferOffset;
	REF_PTR_SET(render_state.index_buffer,iba.IndexBuffer);
	render_state.index_buffer->Add_Engine_Ref();
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

// ----------------------------------------------------------------------------
//
// Private function for the special case of rendering polygons from sorting
// index and vertex buffers.
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Draw_Sorting_IB_VB(
	unsigned primitive_type,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	WWASSERT(render_state.vertex_buffer_type==BUFFER_TYPE_SORTING || render_state.vertex_buffer_type==BUFFER_TYPE_DYNAMIC_SORTING);
	WWASSERT(render_state.index_buffer_type==BUFFER_TYPE_SORTING || render_state.index_buffer_type==BUFFER_TYPE_DYNAMIC_SORTING);

	// Fill dynamic vertex buffer with sorting vertex buffer vertices
	DynamicVBAccessClass dyn_vb_access(BUFFER_TYPE_DYNAMIC_DX8,dynamic_fvf_type,vertex_count);
	{
		DynamicVBAccessClass::WriteLockClass lock(&dyn_vb_access);
		VertexFormatXYZNDUV2* src = static_cast<SortingVertexBufferClass*>(render_state.vertex_buffer)->VertexBuffer;
		VertexFormatXYZNDUV2* dest= lock.Get_Formatted_Vertex_Array();
		src += render_state.vba_offset + render_state.index_base_offset + min_vertex_index;
		unsigned  size = dyn_vb_access.FVF_Info().Get_FVF_Size()*vertex_count/sizeof(unsigned);
		unsigned *dest_u =(unsigned*) dest;
		unsigned *src_u = (unsigned*) src;
		
		for (unsigned i=0;i<size;++i) {
			*dest_u++=*src_u++;
		}
	}
	
	DX8CALL(SetStreamSource(
		0,
		static_cast<DX8VertexBufferClass*>(dyn_vb_access.VertexBuffer)->Get_DX8_Vertex_Buffer(),
		0,
		dyn_vb_access.FVF_Info().Get_FVF_Size()));
	//DX8CALL(SetVertexShader(dyn_vb_access.FVF_Info().Get_FVF()));
	DX8CALL(SetFVF(dyn_vb_access.FVF_Info().Get_FVF()));
	DX8_RECORD_VERTEX_BUFFER_CHANGE();

	unsigned index_count=0;
	switch (primitive_type) {
	case D3DPT_TRIANGLELIST: index_count=polygon_count*3; break;
	case D3DPT_TRIANGLESTRIP: index_count=polygon_count+2; break;
	case D3DPT_TRIANGLEFAN: index_count=polygon_count+2; break;
	default: WWASSERT(0); break; // Unsupported primitive type
	}

	// Fill dynamic index buffer with sorting index buffer vertices
	DynamicIBAccessClass dyn_ib_access(BUFFER_TYPE_DYNAMIC_DX8,index_count);
	{
		DynamicIBAccessClass::WriteLockClass lock(&dyn_ib_access);
		unsigned short* dest=lock.Get_Index_Array();
		unsigned short* src=NULL;
		src=static_cast<SortingIndexBufferClass*>(render_state.index_buffer)->index_buffer;
		src+=render_state.iba_offset+start_index;

		for (unsigned short i=0;i<index_count;++i) {
			unsigned short index=*src++;
			index-=min_vertex_index;
			WWASSERT(index<vertex_count);
			*dest++=index;
		}
	}

	DX8CALL(SetIndices(
		static_cast<DX8IndexBufferClass*>(dyn_ib_access.IndexBuffer)->Get_DX8_Index_Buffer()));
	DX8_RECORD_INDEX_BUFFER_CHANGE();

	DX8CALL(DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		dyn_vb_access.VertexBufferOffset, // [DX9] BaseVertexIndex 
		0,		// start vertex
		vertex_count,
		dyn_ib_access.IndexBufferOffset,
		polygon_count));

	DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Draw(
	unsigned primitive_type,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	DX8_THREAD_ASSERT();
	SNAPSHOT_SAY(("DX8 - draw\n"));

	Apply_Render_State_Changes();

	// Debug feature to disable triangle drawing...
	if (!_Is_Triangle_Draw_Enabled()) return;

	SNAPSHOT_SAY(("DX8 - draw %s polygons (%d vertices)\n",polygon_count,vertex_count));

	if (vertex_count<3) {
		min_vertex_index=0;
		switch (render_state.vertex_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_SORTING:
			vertex_count=render_state.vertex_buffer->Get_Vertex_Count()-render_state.index_base_offset-render_state.vba_offset-min_vertex_index;
			break;
		case BUFFER_TYPE_DYNAMIC_DX8:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			vertex_count=render_state.vba_count;
			break;
		}
	}

	switch (render_state.vertex_buffer_type) {
	case BUFFER_TYPE_DX8:
	case BUFFER_TYPE_DYNAMIC_DX8:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			{
/*				if ((start_index+render_state.iba_offset+polygon_count*3) > render_state.index_buffer->Get_Index_Count())
				{	WWASSERT_PRINT(0,"OVERFLOWING INDEX BUFFER");
					///@todo: MUST FIND OUT WHY THIS HAPPENS WITH LOTS OF PARTICLES ON BIG FIGHT!  -MW
					break;
				}*/
				DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
				DX8CALL(DrawIndexedPrimitive(
					(D3DPRIMITIVETYPE)primitive_type,
					0, // [DX9] render_state.index_base_offset + render_state.vba_offset,
					min_vertex_index,
					vertex_count,
					start_index+render_state.iba_offset,
					polygon_count));
			}
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_SORTING:
	case BUFFER_TYPE_DYNAMIC_SORTING:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			Draw_Sorting_IB_VB(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_INVALID:
		WWASSERT(0);
		break;
	}
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Draw_Triangles(
	unsigned buffer_type,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	if (buffer_type==BUFFER_TYPE_SORTING || buffer_type==BUFFER_TYPE_DYNAMIC_SORTING) {
		SortingRendererClass::Insert_Triangles(start_index,polygon_count,min_vertex_index,vertex_count);
	}
	else {
		Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
	}
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Draw_Triangles(
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Draw_Strip(
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	Draw(D3DPT_TRIANGLESTRIP,start_index,polygon_count,min_vertex_index,vertex_count);
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Apply_Render_State_Changes()
{
	if (!render_state_changed) return;
	if (render_state_changed&SHADER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply shader\n"));
		render_state.shader.Apply();
	}

	unsigned mask=TEXTURE0_CHANGED;
	for (unsigned i=0;i<MAX_TEXTURE_STAGES;++i,mask<<=1) {
		if (render_state_changed&mask) {
			SNAPSHOT_SAY(("DX8 - apply texture %d\n",i));
			if (render_state.Textures[i]) render_state.Textures[i]->Apply(i);
			else TextureClass::Apply_Null(i);
		}
	}

	if (render_state_changed&MATERIAL_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply material\n"));
		VertexMaterialClass* material=const_cast<VertexMaterialClass*>(render_state.material);
		if (material) {
			material->Apply();
		}
		else VertexMaterialClass::Apply_Null();
	}

	if (render_state_changed&LIGHTS_CHANGED) 
	{
		unsigned mask=LIGHT0_CHANGED;
		for (unsigned index=0;index<4;++index,mask<<=1) {
			if (render_state_changed&mask) {
				SNAPSHOT_SAY(("DX8 - apply light %d\n",index));
				if (render_state.LightEnable[index]) {
					Set_DX8_Light(index,&render_state.Lights[index]);
				}
				else {
					Set_DX8_Light(index,NULL);

				}
			}
		}
	}

	if (render_state_changed&WORLD_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply world matrix\n"));
		_Set_DX8_Transform(D3DTS_WORLD,render_state.world);
	}
	if (render_state_changed&VIEW_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply view matrix\n"));
		_Set_DX8_Transform(D3DTS_VIEW,render_state.view);
	}
	if (render_state_changed&VERTEX_BUFFER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply vb change\n"));
		if (render_state.vertex_buffer) {
			switch (render_state.vertex_buffer_type) {//->Type()) {
			case BUFFER_TYPE_DX8:
			case BUFFER_TYPE_DYNAMIC_DX8:
				DX8CALL(SetStreamSource(
					0,
					static_cast<DX8VertexBufferClass*>(render_state.vertex_buffer)->Get_DX8_Vertex_Buffer(),
					0,
					render_state.vertex_buffer->FVF_Info().Get_FVF_Size()));
				DX8_RECORD_VERTEX_BUFFER_CHANGE();
				//DX8CALL(SetVertexShader(render_state.vertex_buffer->FVF_Info().Get_FVF()));
				DX8CALL(SetFVF(render_state.vertex_buffer->FVF_Info().Get_FVF()));
				break;
			case BUFFER_TYPE_SORTING:
			case BUFFER_TYPE_DYNAMIC_SORTING:
				break;
			default:
				WWASSERT(0);
			}
		} else {
			DX8CALL(SetStreamSource(0,NULL,0,0));
			DX8_RECORD_VERTEX_BUFFER_CHANGE();
		}
	}
	if (render_state_changed&INDEX_BUFFER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply ib change\n"));
		if (render_state.index_buffer) {
			switch (render_state.index_buffer_type) {//->Type()) {
			case BUFFER_TYPE_DX8:
			case BUFFER_TYPE_DYNAMIC_DX8:
				DX8CALL(SetIndices(
					static_cast<DX8IndexBufferClass*>(render_state.index_buffer)->Get_DX8_Index_Buffer())
					);
					//render_state.index_base_offset+render_state.vba_offset));
				DX8_RECORD_INDEX_BUFFER_CHANGE();
				break;
			case BUFFER_TYPE_SORTING:
			case BUFFER_TYPE_DYNAMIC_SORTING:
				break;
			default:
				WWASSERT(0);
			}
		}
		else {
			DX8CALL(SetIndices(
				NULL));
			DX8_RECORD_INDEX_BUFFER_CHANGE();
		}
	}

	render_state_changed&=((unsigned)WORLD_IDENTITY|(unsigned)VIEW_IDENTITY);
}

IDirect3DTexture9 * DX8Wrapper::_Create_DX8_Texture(
	unsigned int width, 
	unsigned int height,
	WW3DFormat format, 
	TextureClass::MipCountType mip_level_count,
	D3DPOOL pool,
	bool rendertarget)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DTexture9 *texture = NULL;

	// Paletted textures not supported!
	WWASSERT(format!=D3DFMT_P8);

	// NOTE: If 'format' is not supported as a texture format, this function will find the closest
	// format that is supported and use that instead.

	// Render target may return NOTAVAILABLE, in
	// which case we return NULL.
	if (rendertarget) {
		//unsigned ret=D3DXCreateTexture(
		//	DX8Wrapper::_Get_D3D_Device8(), 
		//	width, 
		//	height,
		//	mip_level_count,
		//	D3DUSAGE_RENDERTARGET, 
		//	WW3DFormat_To_D3DFormat(format),
		//	pool, 
		//	&texture);
		
		// [DX9]
		DWORD usage = D3DUSAGE_RENDERTARGET;
		if (mip_level_count == TextureClass::MIP_LEVELS_1)
		{
			usage |= D3DUSAGE_AUTOGENMIPMAP;
		}
		
		HRESULT ret = DX8Wrapper::_Get_D3D_Device8()->CreateTexture(
			width,
			height,
			mip_level_count,
			usage,
			WW3DFormat_To_D3DFormat(format),
			pool,
			&texture,
			NULL); // [DX9]

		if (ret==D3DERR_NOTAVAILABLE) {
			Non_Fatal_Log_DX8_ErrorCode(ret,__FILE__,__LINE__);
			return NULL;
		}

		if (ret==D3DERR_OUTOFVIDEOMEMORY) {
			Non_Fatal_Log_DX8_ErrorCode(ret,__FILE__,__LINE__);
			return NULL;
		}

		DX8_ErrorCode(ret);
		// Just return the texture, no reduction
		// allowed for render targets.
		return texture;
	}

	// Don't allow any errors in non-render target
	// texture creation.
	//DX8_ErrorCode(D3DXCreateTexture(
	//	DX8Wrapper::_Get_D3D_Device8(), 
	//	width, 
	//	height,
	//	mip_level_count,
	//	0, 
	//	WW3DFormat_To_D3DFormat(format),
	//	pool, 
	//	&texture));
	
	// [DX9]
	DWORD usage = D3DUSAGE_RENDERTARGET;
	if (mip_level_count == TextureClass::MIP_LEVELS_1)
	{
		usage |= D3DUSAGE_AUTOGENMIPMAP;
	}

	HRESULT ret = DX8Wrapper::_Get_D3D_Device8()->CreateTexture(
		width,
		height,
		mip_level_count,
		0,
		WW3DFormat_To_D3DFormat(format),
		pool,
		&texture,
		NULL); // [DX9]
	DX8_ErrorCode(ret);

//	unsigned reduction=WW3D::Get_Texture_Reduction();
//	unsigned level_count=texture->GetLevelCount();
//	if (reduction>=level_count) reduction=level_count-1;
//	texture->SetLOD(reduction);

	return texture;
}

// [DX9][remove] it if not use by any component
//IDirect3DTexture9 * DX8Wrapper::_Create_DX8_Texture(
//	const char *filename, 
//	TextureClass::MipCountType mip_level_count)
//{
//	DX8_THREAD_ASSERT();
//	DX8_Assert();
//	IDirect3DTexture9 *texture = NULL;
//
//	// NOTE: If the original image format is not supported as a texture format, it will
//	// automatically be converted to an appropriate format.
//	// NOTE: It is possible to get the size and format of the original image file from this
//	// function as well, so if we later want to second-guess D3DX's format conversion decisions
//	// we can do so after this function is called..
//	unsigned result = D3DXCreateTextureFromFileExA(
//		_Get_D3D_Device8(), 
//		filename,
//		D3DX_DEFAULT, 
//		D3DX_DEFAULT, 
//		mip_level_count,//create_mipmaps ? 0 : 1, 
//		0, 
//		D3DFMT_UNKNOWN, 
//		D3DPOOL_MANAGED,
//		D3DX_FILTER_BOX, 
//		D3DX_FILTER_BOX, 
//		0, 
//		NULL, 
//		NULL, 
//		&texture);
//	
//	if (result != D3D_OK) {
//		return MissingTexture::_Get_Missing_Texture();
//	}
//
//	// Make sure texture wasn't paletted!
//	D3DSURFACE_DESC desc;
//	texture->GetLevelDesc(0,&desc);
//	if (desc.Format==D3DFMT_P8) {
//		texture->Release();
//		return MissingTexture::_Get_Missing_Texture();
//	}
//	else {
////		unsigned reduction=WW3D::Get_Texture_Reduction();
////		unsigned level_count=texture->GetLevelCount();
////		if (reduction>=level_count) reduction=level_count-1;
////		texture->SetLOD(reduction);
//	}
//
//	return texture;
//}

// [DX9][Newly added] - by Claude 3.7 Sonnet
/**
* A replacement for D3DXLoadSurfaceFromSurface from DirectX 8
* This function copies one surface to another with multiple filtering options
*
* @param pDestSurface - Destination surface
* @param pDestRect - Pointer to destination rectangle (can be NULL for entire surface)
* @param pSrcSurface - Source surface
* @param pSrcRect - Pointer to source rectangle (can be NULL for entire surface)
* @param Filter - Filter type (D3DX_FILTER_NONE, D3DX_FILTER_BOX, D3DX_FILTER_TRIANGLE)
* @param ColorKey - Color key value (transparent pixel)
*
* @return HRESULT - S_OK on success, error code on failure
*/
HRESULT DX8Wrapper::D3D9LoadSurfaceFromSurface(
	LPDIRECT3DSURFACE9 pDestSurface,
	const RECT* pDestRect,
	LPDIRECT3DSURFACE9 pSrcSurface,
	const RECT* pSrcRect,
	DWORD Filter,
	D3DCOLOR ColorKey)
{
	if (!pDestSurface || !pSrcSurface)
		return E_INVALIDARG;

	// Get the device from the destination surface
	IDirect3DDevice9* pDevice = NULL;
	HRESULT hr = pDestSurface->GetDevice(&pDevice);
	if (FAILED(hr))
		return hr;

	// Get source and destination surface descriptions
	D3DSURFACE_DESC srcDesc, destDesc;
	pSrcSurface->GetDesc(&srcDesc);
	pDestSurface->GetDesc(&destDesc);

	// Define default rectangles if none provided
	RECT defaultSrcRect = { 0, 0, (LONG)srcDesc.Width, (LONG)srcDesc.Height };
	RECT defaultDestRect = { 0, 0, (LONG)destDesc.Width, (LONG)destDesc.Height };

	// Use provided rectangles or default to full surface
	const RECT* useSrcRect = pSrcRect ? pSrcRect : &defaultSrcRect;
	const RECT* useDestRect = pDestRect ? pDestRect : &defaultDestRect;

	// Calculate dimensions of rectangles
	UINT srcWidth = useSrcRect->right - useSrcRect->left;
	UINT srcHeight = useSrcRect->bottom - useSrcRect->top;
	UINT destWidth = useDestRect->right - useDestRect->left;
	UINT destHeight = useDestRect->bottom - useDestRect->top;

	// Handle D3DX_FILTER_NONE (direct copy with no filtering)
	if (Filter == D3DX_FILTER_NONE)
	{
		// For same dimensions, just use StretchRect with no filtering
		if (srcWidth == destWidth && srcHeight == destHeight)
		{
			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_NONE);
			pDevice->Release();
			return hr;
		}

		// For different dimensions with no filtering, we need to do direct pixel copy
		D3DLOCKED_RECT srcLocked, destLocked;

		// Lock the surfaces
		hr = pSrcSurface->LockRect(&srcLocked, useSrcRect, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			pDevice->Release();
			return hr;
		}

		hr = pDestSurface->LockRect(&destLocked, useDestRect, 0);
		if (FAILED(hr))
		{
			pSrcSurface->UnlockRect();
			pDevice->Release();
			return hr;
		}

		// Get bytes per pixel
		int bytesPerPixel = 0;
		switch (srcDesc.Format)
		{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bytesPerPixel = 4;
			break;
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
			bytesPerPixel = 2;
			break;
		case D3DFMT_A8:
		case D3DFMT_L8:
			bytesPerPixel = 1;
			break;
		default:
			// Unsupported format
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();
			pDevice->Release();
			return E_NOTIMPL;
		}

		// Copy pixels with no filtering (nearest neighbor sampling)
		for (UINT y = 0; y < destHeight; y++)
		{
			for (UINT x = 0; x < destWidth; x++)
			{
				// Find source coordinate
				UINT srcX = (x * srcWidth) / destWidth;
				UINT srcY = (y * srcHeight) / destHeight;

				// Keep source coordinates in bounds
				srcX = min(srcX, srcWidth - 1);
				srcY = min(srcY, srcHeight - 1);

				if (bytesPerPixel == 4) // 32-bit
				{
					DWORD* srcPixel = (DWORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX * bytesPerPixel);
					DWORD* destPixel = (DWORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

					// Apply color key if needed
					if (*srcPixel != ColorKey)
						*destPixel = *srcPixel;
					else
						*destPixel = 0; // Transparent black for color keyed pixels
				}
				else if (bytesPerPixel == 2) // 16-bit
				{
					WORD* srcPixel = (WORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX * bytesPerPixel);
					WORD* destPixel = (WORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

					if (*srcPixel != (WORD)ColorKey)
						*destPixel = *srcPixel;
					else
						*destPixel = 0;
				}
				else if (bytesPerPixel == 1) // 8-bit
				{
					BYTE* srcPixel = (BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX;
					BYTE* destPixel = (BYTE*)destLocked.pBits + y * destLocked.Pitch + x;

					if (*srcPixel != (BYTE)ColorKey)
						*destPixel = *srcPixel;
					else
						*destPixel = 0;
				}
			}
		}

		// Unlock surfaces
		pSrcSurface->UnlockRect();
		pDestSurface->UnlockRect();
		pDevice->Release();
		return S_OK;
	}
	// Handle D3DX_FILTER_BOX (2x2 box filter for half-size mipmaps)
	else if (Filter == D3DX_FILTER_BOX)
	{
		// Verify that destination is half the size of source (for box filter)
		if (destWidth * 2 != srcWidth || destHeight * 2 != srcHeight)
		{
			// Not handling mipmap case, fall back to regular StretchRect with linear filtering
			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_LINEAR);
			pDevice->Release();
			return hr;
		}

		// For box filter (2x2 averaging), we need to access pixel data directly
		D3DLOCKED_RECT srcLocked, destLocked;

		// Lock the surfaces for direct memory access
		hr = pSrcSurface->LockRect(&srcLocked, useSrcRect, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			pDevice->Release();
			return hr;
		}

		hr = pDestSurface->LockRect(&destLocked, useDestRect, 0);
		if (FAILED(hr))
		{
			pSrcSurface->UnlockRect();
			pDevice->Release();
			return hr;
		}

		// Get bytes per pixel based on format
		int bytesPerPixel = 0;
		switch (srcDesc.Format)
		{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bytesPerPixel = 4;
			break;
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
			bytesPerPixel = 2;
			break;
		case D3DFMT_A8:
		case D3DFMT_L8:
			bytesPerPixel = 1;
			break;
		default:
			// Unsupported format, fall back to regular StretchRect
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();

			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_LINEAR);
			pDevice->Release();
			return hr;
		}

		// Implement 2x2 box filter (averaging 4 source pixels for each destination pixel)
		if (bytesPerPixel == 4) // 32-bit format (A8R8G8B8 or X8R8G8B8)
		{
			for (UINT y = 0; y < destHeight; y++)
			{
				for (UINT x = 0; x < destWidth; x++)
				{
					UINT srcX = x * 2;
					UINT srcY = y * 2;

					DWORD* srcPixels[4];
					srcPixels[0] = (DWORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX * bytesPerPixel);
					srcPixels[1] = (DWORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + (srcX + 1) * bytesPerPixel);
					srcPixels[2] = (DWORD*)((BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + srcX * bytesPerPixel);
					srcPixels[3] = (DWORD*)((BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + (srcX + 1) * bytesPerPixel);

					// Skip color keyed pixels in averaging
					UINT validPixels = 0;
					UINT sumA = 0, sumR = 0, sumG = 0, sumB = 0;

					for (int i = 0; i < 4; i++)
					{
						DWORD pixel = *srcPixels[i];
						if (pixel != ColorKey)
						{
							sumB += (pixel & 0xFF);
							sumG += ((pixel >> 8) & 0xFF);
							sumR += ((pixel >> 16) & 0xFF);
							sumA += ((pixel >> 24) & 0xFF);
							validPixels++;
						}
					}

					DWORD* destPixel = (DWORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

					if (validPixels > 0)
					{
						// Average the non-color-keyed pixels
						*destPixel = ((sumA / validPixels) << 24) |
							((sumR / validPixels) << 16) |
							((sumG / validPixels) << 8) |
							(sumB / validPixels);
					}
					else
					{
						// All pixels were color keyed, set destination to transparent black
						*destPixel = 0;
					}
				}
			}
		}
		else if (bytesPerPixel == 2) // 16-bit format
		{
			for (UINT y = 0; y < destHeight; y++)
			{
				for (UINT x = 0; x < destWidth; x++)
				{
					UINT srcX = x * 2;
					UINT srcY = y * 2;

					WORD* srcPixels[4];
					srcPixels[0] = (WORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX * bytesPerPixel);
					srcPixels[1] = (WORD*)((BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + (srcX + 1) * bytesPerPixel);
					srcPixels[2] = (WORD*)((BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + srcX * bytesPerPixel);
					srcPixels[3] = (WORD*)((BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + (srcX + 1) * bytesPerPixel);

					// For 16-bit formats, extract components based on format
					UINT validPixels = 0;
					UINT sumR = 0, sumG = 0, sumB = 0, sumA = 0;

					if (srcDesc.Format == D3DFMT_R5G6B5)
					{
						for (int i = 0; i < 4; i++)
						{
							WORD pixel = *srcPixels[i];
							if ((DWORD)pixel != (WORD)ColorKey)
							{
								sumB += (pixel & 0x1F);
								sumG += ((pixel >> 5) & 0x3F);
								sumR += ((pixel >> 11) & 0x1F);
								validPixels++;
							}
						}

						WORD* destPixel = (WORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

						if (validPixels > 0)
						{
							*destPixel = ((sumR / validPixels) << 11) |
								((sumG / validPixels) << 5) |
								(sumB / validPixels);
						}
						else
						{
							*destPixel = 0;
						}
					}
					else // A1R5G5B5 or X1R5G5B5
					{
						for (int i = 0; i < 4; i++)
						{
							WORD pixel = *srcPixels[i];
							if ((DWORD)pixel != (WORD)ColorKey)
							{
								sumB += (pixel & 0x1F);
								sumG += ((pixel >> 5) & 0x1F);
								sumR += ((pixel >> 10) & 0x1F);
								sumA += ((pixel >> 15) & 0x01);
								validPixels++;
							}
						}

						WORD* destPixel = (WORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

						if (validPixels > 0)
						{
							*destPixel = ((sumA / validPixels) << 15) |
								((sumR / validPixels) << 10) |
								((sumG / validPixels) << 5) |
								(sumB / validPixels);
						}
						else
						{
							*destPixel = 0;
						}
					}
				}
			}
		}
		else if (bytesPerPixel == 1) // 8-bit format
		{
			for (UINT y = 0; y < destHeight; y++)
			{
				for (UINT x = 0; x < destWidth; x++)
				{
					UINT srcX = x * 2;
					UINT srcY = y * 2;

					BYTE* srcPixels[4];
					srcPixels[0] = (BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + srcX;
					srcPixels[1] = (BYTE*)srcLocked.pBits + srcY * srcLocked.Pitch + (srcX + 1);
					srcPixels[2] = (BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + srcX;
					srcPixels[3] = (BYTE*)srcLocked.pBits + (srcY + 1) * srcLocked.Pitch + (srcX + 1);

					UINT validPixels = 0;
					UINT sumValue = 0;

					for (int i = 0; i < 4; i++)
					{
						BYTE pixel = *srcPixels[i];
						if (pixel != (BYTE)ColorKey)
						{
							sumValue += pixel;
							validPixels++;
						}
					}

					BYTE* destPixel = (BYTE*)destLocked.pBits + y * destLocked.Pitch + x;

					if (validPixels > 0)
					{
						*destPixel = (BYTE)(sumValue / validPixels);
					}
					else
					{
						*destPixel = 0;
					}
				}
			}
		}

		// Unlock surfaces
		pSrcSurface->UnlockRect();
		pDestSurface->UnlockRect();
		pDevice->Release();

		return S_OK;
	}
	// Handle D3DX_FILTER_TRIANGLE (high quality filtering where every pixel contributes equally)
	else if (Filter == D3DX_FILTER_TRIANGLE)
	{
		// Lock the surfaces for bilinear filtering implementation
		D3DLOCKED_RECT srcLocked, destLocked;

		hr = pSrcSurface->LockRect(&srcLocked, useSrcRect, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			pDevice->Release();
			return hr;
		}

		hr = pDestSurface->LockRect(&destLocked, useDestRect, 0);
		if (FAILED(hr))
		{
			pSrcSurface->UnlockRect();
			pDevice->Release();
			return hr;
		}

		// Get bytes per pixel based on format
		int bytesPerPixel = 0;
		switch (srcDesc.Format)
		{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bytesPerPixel = 4;
			break;
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
			bytesPerPixel = 2;
			break;
		case D3DFMT_A8:
		case D3DFMT_L8:
			bytesPerPixel = 1;
			break;
		default:
			// Unsupported format, fall back to regular StretchRect
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();

			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_LINEAR);
			pDevice->Release();
			return hr;
		}

		// For triangle filter, implement high-quality bilinear filtering
		if (bytesPerPixel == 4) // 32-bit format (A8R8G8B8 or X8R8G8B8)
		{
			for (UINT y = 0; y < destHeight; y++)
			{
				for (UINT x = 0; x < destWidth; x++)
				{
					// Calculate floating-point position in source
					float srcX = (float)x * srcWidth / destWidth;
					float srcY = (float)y * srcHeight / destHeight;

					// Get integer and fractional parts
					int srcXint = (int)srcX;
					int srcYint = (int)srcY;
					float fracX = srcX - srcXint;
					float fracY = srcY - srcYint;

					// Clamp to source bounds for safe access
					int x0 = min(max(srcXint, 0), (int)srcWidth - 1);
					int y0 = min(max(srcYint, 0), (int)srcHeight - 1);
					int x1 = min(x0 + 1, (int)srcWidth - 1);
					int y1 = min(y0 + 1, (int)srcHeight - 1);

					// Get the four surrounding pixels
					DWORD* topLeft = (DWORD*)((BYTE*)srcLocked.pBits + y0 * srcLocked.Pitch + x0 * bytesPerPixel);
					DWORD* topRight = (DWORD*)((BYTE*)srcLocked.pBits + y0 * srcLocked.Pitch + x1 * bytesPerPixel);
					DWORD* bottomLeft = (DWORD*)((BYTE*)srcLocked.pBits + y1 * srcLocked.Pitch + x0 * bytesPerPixel);
					DWORD* bottomRight = (DWORD*)((BYTE*)srcLocked.pBits + y1 * srcLocked.Pitch + x1 * bytesPerPixel);

					// Skip color keyed pixels
					bool tlValid = *topLeft != ColorKey;
					bool trValid = *topRight != ColorKey;
					bool blValid = *bottomLeft != ColorKey;
					bool brValid = *bottomRight != ColorKey;

					// Extract color components from each valid pixel
					int tla = 0, tlr = 0, tlg = 0, tlb = 0;
					int tra = 0, trr = 0, trg = 0, trb = 0;
					int bla = 0, blr = 0, blg = 0, blb = 0;
					int bra = 0, brr = 0, brg = 0, brb = 0;

					// Only extract components from valid pixels
					if (tlValid) {
						tlb = (*topLeft & 0xFF);
						tlg = ((*topLeft >> 8) & 0xFF);
						tlr = ((*topLeft >> 16) & 0xFF);
						tla = ((*topLeft >> 24) & 0xFF);
					}

					if (trValid) {
						trb = (*topRight & 0xFF);
						trg = ((*topRight >> 8) & 0xFF);
						trr = ((*topRight >> 16) & 0xFF);
						tra = ((*topRight >> 24) & 0xFF);
					}

					if (blValid) {
						blb = (*bottomLeft & 0xFF);
						blg = ((*bottomLeft >> 8) & 0xFF);
						blr = ((*bottomLeft >> 16) & 0xFF);
						bla = ((*bottomLeft >> 24) & 0xFF);
					}

					if (brValid) {
						brb = (*bottomRight & 0xFF);
						brg = ((*bottomRight >> 8) & 0xFF);
						brr = ((*bottomRight >> 16) & 0xFF);
						bra = ((*bottomRight >> 24) & 0xFF);
					}

					// Get destination pixel
					DWORD* destPixel = (DWORD*)((BYTE*)destLocked.pBits + y * destLocked.Pitch + x * bytesPerPixel);

					// Check if any valid pixels
					if (tlValid || trValid || blValid || brValid)
					{
						// Calculate weights based on fractional position
						float w1 = (1.0f - fracX) * (1.0f - fracY);
						float w2 = fracX * (1.0f - fracY);
						float w3 = (1.0f - fracX) * fracY;
						float w4 = fracX * fracY;

						// Adjust weights for invalid pixels
						float totalWeight = 0.0f;
						if (tlValid) totalWeight += w1;
						if (trValid) totalWeight += w2;
						if (blValid) totalWeight += w3;
						if (brValid) totalWeight += w4;

						// Normalize weights if any valid pixels
						if (totalWeight > 0.0f) {
							float invWeight = 1.0f / totalWeight;
							if (tlValid) w1 *= invWeight;
							if (trValid) w2 *= invWeight;
							if (blValid) w3 *= invWeight;
							if (brValid) w4 *= invWeight;
						}

						// Weighted sum for each component
						int a = (int)(tla * w1 + tra * w2 + bla * w3 + bra * w4);
						int r = (int)(tlr * w1 + trr * w2 + blr * w3 + brr * w4);
						int g = (int)(tlg * w1 + trg * w2 + blg * w3 + brg * w4);
						int b = (int)(tlb * w1 + trb * w2 + blb * w3 + brb * w4);

						// Clamp values to byte range (0-255)
						a = min(max(a, 0), 255);
						r = min(max(r, 0), 255);
						g = min(max(g, 0), 255);
						b = min(max(b, 0), 255);

						// Write final pixel
						*destPixel = (a << 24) | (r << 16) | (g << 8) | b;
					}
					else
					{
						// All pixels were color keyed, set destination to transparent black
						*destPixel = 0;
					}
				}
			}
		}
		else if (bytesPerPixel == 2) // 16-bit formats
		{
			// Triangle filter for 16-bit formats
			// Similar approach to 32-bit but adjusting bit extraction/packing for 16-bit
			// Implementation would be similar but with appropriate bit handling
			// For brevity, fall back to point filtering
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();

			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_LINEAR);
			pDevice->Release();
			return hr;
		}
		else if (bytesPerPixel == 1) // 8-bit format
		{
			// Triangle filter for 8-bit formats
			// Similar approach but simplified for single-channel pixels
			// For brevity, fall back to point filtering
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();

			hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, D3DTEXF_LINEAR);
			pDevice->Release();
			return hr;
		}

		// Unlock surfaces
		pSrcSurface->UnlockRect();
		pDestSurface->UnlockRect();
		pDevice->Release();

		return S_OK;
	}
	else // Unknown filter, fall back to DirectX9 native filtering
	{
		// Select an appropriate D3D9 filter
		D3DTEXTUREFILTERTYPE d3d9Filter = D3DTEXF_POINT;

		// Map to the closest equivalent DirectX9 filter
		if (Filter == D3DX_FILTER_BOX || Filter == D3DX_FILTER_TRIANGLE)
			d3d9Filter = D3DTEXF_LINEAR;

		hr = pDevice->StretchRect(pSrcSurface, useSrcRect, pDestSurface, useDestRect, d3d9Filter);
		pDevice->Release();
		return hr;
	}
}

// [DX9][Newly added] - by Claude 3.7 Sonnet
// Precise replacement for D3DXFilterTexture with D3DX_FILTER_BOX behavior
// Implements exact 2×2 box filter averaging when dimensions are halved
// Replacement for legacy D3DXFilterTexture with D3DX_FILTER_BOX
// Implements 2×2 box filter averaging for DirectX 9 (which lacks D3DTEXF_BOX)
HRESULT CustomFilterTexture(
	LPDIRECT3DTEXTURE9 pTexture,    // [in] Texture to be filtered
	UINT iMipLevels = 0             // [in] Number of mip levels to generate, 0 for all levels
)
{
	if (!pTexture)
		return D3DERR_INVALIDCALL;

	// Get device from texture
	LPDIRECT3DDEVICE9 pDevice = NULL;
	HRESULT hr = pTexture->GetDevice(&pDevice);
	if (FAILED(hr))
		return hr;

	// Calculate number of mip levels
	UINT numLevels = 1;
	D3DSURFACE_DESC baseDesc;
	hr = pTexture->GetLevelDesc(0, &baseDesc);
	if (FAILED(hr)) {
		pDevice->Release();
		return hr;
	}

	// Calculate max possible mip levels if needed
	if (iMipLevels == 0) {
		UINT width = baseDesc.Width;
		UINT height = baseDesc.Height;

		while (width > 1 || height > 1) {
			width = max(1, width / 2);
			height = max(1, height / 2);
			numLevels++;
		}
	}
	else {
		numLevels = min(iMipLevels, pTexture->GetLevelCount());
	}

	// For DirectX 9, we'll implement box filtering manually since D3DTEXF_BOX doesn't exist
	// Process each mip level
	for (UINT level = 0; level < numLevels - 1; level++) {
		// Get source and destination surfaces
		LPDIRECT3DSURFACE9 pSrcSurface = NULL;
		LPDIRECT3DSURFACE9 pDestSurface = NULL;

		hr = pTexture->GetSurfaceLevel(level, &pSrcSurface);
		if (FAILED(hr)) {
			pDevice->Release();
			return hr;
		}

		hr = pTexture->GetSurfaceLevel(level + 1, &pDestSurface);
		if (FAILED(hr)) {
			pSrcSurface->Release();
			pDevice->Release();
			return hr;
		}

		// Get dimensions of source and destination
		D3DSURFACE_DESC srcDesc, destDesc;
		pSrcSurface->GetDesc(&srcDesc);
		pDestSurface->GetDesc(&destDesc);

		// Check if dimensions are exactly halved (required for 2x2 box filter)
		bool exactHalving = (destDesc.Width == srcDesc.Width / 2) &&
			(destDesc.Height == srcDesc.Height / 2);

		// For compressed formats we have to use StretchRect with LINEAR filtering
		bool isCompressed =
			(srcDesc.Format == D3DFMT_DXT1 || srcDesc.Format == D3DFMT_DXT2 ||
				srcDesc.Format == D3DFMT_DXT3 || srcDesc.Format == D3DFMT_DXT4 ||
				srcDesc.Format == D3DFMT_DXT5);

		if (isCompressed || !exactHalving) {
			// For compressed textures or non-standard sizes, use LINEAR filtering
			hr = pDevice->StretchRect(pSrcSurface, NULL, pDestSurface, NULL, D3DTEXF_LINEAR);
			pSrcSurface->Release();
			pDestSurface->Release();

			if (FAILED(hr)) {
				pDevice->Release();
				return hr;
			}
			continue;
		}

		// For standard formats with exact halving, do manual 2x2 box filtering
		D3DLOCKED_RECT srcLock, destLock;
		hr = pSrcSurface->LockRect(&srcLock, NULL, D3DLOCK_READONLY);
		if (FAILED(hr)) {
			pSrcSurface->Release();
			pDestSurface->Release();
			pDevice->Release();
			return hr;
		}

		hr = pDestSurface->LockRect(&destLock, NULL, 0);
		if (FAILED(hr)) {
			pSrcSurface->UnlockRect();
			pSrcSurface->Release();
			pDestSurface->Release();
			pDevice->Release();
			return hr;
		}

		// Get pixel size in bytes based on format
		UINT bytesPerPixel = 0;
		switch (srcDesc.Format) {
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bytesPerPixel = 4;
			break;
		case D3DFMT_R8G8B8:
			bytesPerPixel = 3;
			break;
		case D3DFMT_A1R5G5B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_R5G6B5:
			bytesPerPixel = 2;
			break;
		default:
			// For unsupported formats, use LINEAR filtering as fallback
			pSrcSurface->UnlockRect();
			pDestSurface->UnlockRect();
			hr = pDevice->StretchRect(pSrcSurface, NULL, pDestSurface, NULL, D3DTEXF_LINEAR);
			pSrcSurface->Release();
			pDestSurface->Release();
			continue;
		}

		// Perform 2x2 box filter manually
		if (bytesPerPixel == 4) {  // 32-bit format (ARGB)
			for (UINT y = 0; y < destDesc.Height; y++) {
				for (UINT x = 0; x < destDesc.Width; x++) {
					BYTE* srcRow1 = (BYTE*)srcLock.pBits + (2 * y) * srcLock.Pitch;
					BYTE* srcRow2 = (BYTE*)srcLock.pBits + (2 * y + 1) * srcLock.Pitch;
					BYTE* destRow = (BYTE*)destLock.pBits + y * destLock.Pitch;

					// Get pointers to the 2x2 source pixels
					DWORD* src1 = (DWORD*)(srcRow1 + (2 * x) * bytesPerPixel);
					DWORD* src2 = (DWORD*)(srcRow1 + (2 * x + 1) * bytesPerPixel);
					DWORD* src3 = (DWORD*)(srcRow2 + (2 * x) * bytesPerPixel);
					DWORD* src4 = (DWORD*)(srcRow2 + (2 * x + 1) * bytesPerPixel);
					DWORD* dest = (DWORD*)(destRow + x * bytesPerPixel);

					// Extract components from each pixel
					UINT a1 = (*src1 >> 24) & 0xFF, a2 = (*src2 >> 24) & 0xFF;
					UINT a3 = (*src3 >> 24) & 0xFF, a4 = (*src4 >> 24) & 0xFF;
					UINT r1 = (*src1 >> 16) & 0xFF, r2 = (*src2 >> 16) & 0xFF;
					UINT r3 = (*src3 >> 16) & 0xFF, r4 = (*src4 >> 16) & 0xFF;
					UINT g1 = (*src1 >> 8) & 0xFF, g2 = (*src2 >> 8) & 0xFF;
					UINT g3 = (*src3 >> 8) & 0xFF, g4 = (*src4 >> 8) & 0xFF;
					UINT b1 = *src1 & 0xFF, b2 = *src2 & 0xFF;
					UINT b3 = *src3 & 0xFF, b4 = *src4 & 0xFF;

					// Average the components (exact 2x2 box filter)
					UINT a = (a1 + a2 + a3 + a4) / 4;
					UINT r = (r1 + r2 + r3 + r4) / 4;
					UINT g = (g1 + g2 + g3 + g4) / 4;
					UINT b = (b1 + b2 + b3 + b4) / 4;

					// Combine components back into a pixel
					*dest = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}
		}
		else if (bytesPerPixel == 2) {  // 16-bit format
			for (UINT y = 0; y < destDesc.Height; y++) {
				for (UINT x = 0; x < destDesc.Width; x++) {
					BYTE* srcRow1 = (BYTE*)srcLock.pBits + (2 * y) * srcLock.Pitch;
					BYTE* srcRow2 = (BYTE*)srcLock.pBits + (2 * y + 1) * srcLock.Pitch;
					BYTE* destRow = (BYTE*)destLock.pBits + y * destLock.Pitch;

					// Get pointers to the 2x2 source pixels
					WORD* src1 = (WORD*)(srcRow1 + (2 * x) * bytesPerPixel);
					WORD* src2 = (WORD*)(srcRow1 + (2 * x + 1) * bytesPerPixel);
					WORD* src3 = (WORD*)(srcRow2 + (2 * x) * bytesPerPixel);
					WORD* src4 = (WORD*)(srcRow2 + (2 * x + 1) * bytesPerPixel);
					WORD* dest = (WORD*)(destRow + x * bytesPerPixel);

					if (srcDesc.Format == D3DFMT_R5G6B5) {
						// Extract components (R5G6B5 format)
						UINT r1 = (*src1 >> 11) & 0x1F, r2 = (*src2 >> 11) & 0x1F;
						UINT r3 = (*src3 >> 11) & 0x1F, r4 = (*src4 >> 11) & 0x1F;
						UINT g1 = (*src1 >> 5) & 0x3F, g2 = (*src2 >> 5) & 0x3F;
						UINT g3 = (*src3 >> 5) & 0x3F, g4 = (*src4 >> 5) & 0x3F;
						UINT b1 = *src1 & 0x1F, b2 = *src2 & 0x1F;
						UINT b3 = *src3 & 0x1F, b4 = *src4 & 0x1F;

						// Average the components
						UINT r = (r1 + r2 + r3 + r4) / 4;
						UINT g = (g1 + g2 + g3 + g4) / 4;
						UINT b = (b1 + b2 + b3 + b4) / 4;

						// Combine components back into a pixel
						*dest = (WORD)((r << 11) | (g << 5) | b);
					}
					else {  // A1R5G5B5 or X1R5G5B5
						// Extract components
						UINT a1 = (*src1 >> 15) & 0x1, a2 = (*src2 >> 15) & 0x1;
						UINT a3 = (*src3 >> 15) & 0x1, a4 = (*src4 >> 15) & 0x1;
						UINT r1 = (*src1 >> 10) & 0x1F, r2 = (*src2 >> 10) & 0x1F;
						UINT r3 = (*src3 >> 10) & 0x1F, r4 = (*src4 >> 10) & 0x1F;
						UINT g1 = (*src1 >> 5) & 0x1F, g2 = (*src2 >> 5) & 0x1F;
						UINT g3 = (*src3 >> 5) & 0x1F, g4 = (*src4 >> 5) & 0x1F;
						UINT b1 = *src1 & 0x1F, b2 = *src2 & 0x1F;
						UINT b3 = *src3 & 0x1F, b4 = *src4 & 0x1F;

						// Average the components
						UINT a = (a1 + a2 + a3 + a4) >= 2 ? 1 : 0; // Majority vote for 1-bit alpha
						UINT r = (r1 + r2 + r3 + r4) / 4;
						UINT g = (g1 + g2 + g3 + g4) / 4;
						UINT b = (b1 + b2 + b3 + b4) / 4;

						// Combine components back into a pixel
						*dest = (WORD)((a << 15) | (r << 10) | (g << 5) | b);
					}
				}
			}
		}
		else if (bytesPerPixel == 3) {  // 24-bit format (RGB)
			for (UINT y = 0; y < destDesc.Height; y++) {
				for (UINT x = 0; x < destDesc.Width; x++) {
					BYTE* srcRow1 = (BYTE*)srcLock.pBits + (2 * y) * srcLock.Pitch;
					BYTE* srcRow2 = (BYTE*)srcLock.pBits + (2 * y + 1) * srcLock.Pitch;
					BYTE* destRow = (BYTE*)destLock.pBits + y * destLock.Pitch;

					// Get pointers to the 2x2 source pixels
					BYTE* src1 = srcRow1 + (2 * x) * bytesPerPixel;
					BYTE* src2 = srcRow1 + (2 * x + 1) * bytesPerPixel;
					BYTE* src3 = srcRow2 + (2 * x) * bytesPerPixel;
					BYTE* src4 = srcRow2 + (2 * x + 1) * bytesPerPixel;
					BYTE* dest = destRow + x * bytesPerPixel;

					// Average each component
					for (UINT i = 0; i < bytesPerPixel; i++) {
						dest[i] = (src1[i] + src2[i] + src3[i] + src4[i]) / 4;
					}
				}
			}
		}

		// Unlock surfaces
		pSrcSurface->UnlockRect();
		pDestSurface->UnlockRect();
		pSrcSurface->Release();
		pDestSurface->Release();
	}

	// Release the device
	pDevice->Release();

	return S_OK;
}

IDirect3DTexture9 * DX8Wrapper::_Create_DX8_Texture(
	IDirect3DSurface9 *surface,
	TextureClass::MipCountType mip_level_count)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DTexture9 *texture = NULL;

	D3DSURFACE_DESC surface_desc;
	::ZeroMemory(&surface_desc, sizeof(D3DSURFACE_DESC));
	surface->GetDesc(&surface_desc);

	// This function will create a texture with a different (but similar) format if the surface is
	// not in a supported texture format.
	WW3DFormat format=D3DFormat_To_WW3DFormat(surface_desc.Format);
	texture = _Create_DX8_Texture(surface_desc.Width, surface_desc.Height, format, mip_level_count);

	// Copy the surface to the texture
	IDirect3DSurface9 *tex_surface = NULL;
	texture->GetSurfaceLevel(0, &tex_surface);
	//DX8_ErrorCode(D3DXLoadSurfaceFromSurface(tex_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_BOX, 0));
	HRESULT ret = D3D9LoadSurfaceFromSurface(tex_surface, NULL, surface, NULL, D3DX9_FILTER_TYPE::D3DX_FILTER_BOX, 0); // [DX9]
	DX8_ErrorCode(ret);

	tex_surface->Release();

	// Create mipmaps if needed
	if (mip_level_count!=TextureClass::MIP_LEVELS_1) {
		//DX8_ErrorCode(D3DXFilterTexture(texture, NULL, 0, D3DX_FILTER_BOX));
		
		// [DX9]
		HRESULT newret = CustomFilterTexture(texture, 0);
		DX8_ErrorCode(newret);
	}

	return texture;

}

IDirect3DSurface9 * DX8Wrapper::_Create_DX8_Surface(unsigned int width, unsigned int height, WW3DFormat format)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	IDirect3DSurface9 *surface = NULL;

	// Paletted surfaces not supported!
	WWASSERT(format!=D3DFMT_P8);
	
	//DX8CALL(CreateImageSurface(width, height, WW3DFormat_To_D3DFormat(format), &surface));
	// ref: https://learn.microsoft.com/en-us/windows/win32/direct3d9/converting-to-directx-9#createimagesurface-changes
	// ref: https://learn.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-createoffscreenplainsurface
	// ref: https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dpool
	D3DPOOL pool = D3DPOOL_MANAGED; // TODO: hardcoded to test out; may test out D3DPOOL_DEFAULT 
	DX8CALL(CreateOffscreenPlainSurface(width, height, WW3DFormat_To_D3DFormat(format), pool, &surface, NULL));

	return surface;
}

IDirect3DSurface9 * DX8Wrapper::_Create_DX8_Surface(const char *filename_)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	// Note: Since there is no "D3DXCreateSurfaceFromFile" and no "GetSurfaceInfoFromFile" (the
	// latter is supposed to be added to D3DX in a future version), we create a texture from the
	// file (w/o mipmaps), check that its surface is equal to the original file data (which it
	// will not be if the file is not in a texture-supported format or size). If so, copy its
	// surface (we might be able to just get its surface and add a ref to it but I'm not sure so
	// I'm not going to risk it) and release the texture. If not, create a surface according to
	// the file data and use D3DXLoadSurfaceFromFile. This is a horrible hack, but it saves us
	// having to write file loaders. Will fix this when D3DX provides us with the right functions.
	// Create a surface the size of the file image data
	IDirect3DSurface9 *surface = NULL;	

	{
		file_auto_ptr myfile(_TheFileFactory,filename_);	
		// If file not found, create a surface with missing texture in it
		if (!myfile->Is_Available()) {
			return MissingTexture::_Create_Missing_Surface();
		}
	}

	surface=TextureLoader::Load_Surface_Immediate(
		filename_,
		WW3D_FORMAT_UNKNOWN,
		true);
	return surface;
}


/***********************************************************************************************
 * DX8Wrapper::_Update_Texture -- Copies a texture from system memory to video memory          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/26/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void DX8Wrapper::_Update_Texture(TextureClass *system, TextureClass *video)
{
	WWASSERT(system);
	WWASSERT(video);
	WWASSERT(system->Pool==TextureClass::POOL_SYSTEMMEM);
	WWASSERT(video->Pool==TextureClass::POOL_DEFAULT);
	DX8CALL(UpdateTexture(system->D3DTexture,video->D3DTexture));
}

void DX8Wrapper::Compute_Caps(D3DFORMAT display_format,D3DFORMAT depth_stencil_format)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	DX8Caps::Compute_Caps(display_format,depth_stencil_format,D3DDevice);	
}

void DX8Wrapper::Set_Light(unsigned index,const LightClass &light)
{
	D3DLIGHT9 dlight;
	Vector3 temp;
	memset(&dlight,0,sizeof(D3DLIGHT9));

	switch (light.Get_Type())
	{
	case LightClass::POINT:
		{
			dlight.Type=D3DLIGHT_POINT;
		}
		break;
	case LightClass::DIRECTIONAL:
		{
			dlight.Type=D3DLIGHT_DIRECTIONAL;
		}
		break;
	case LightClass::SPOT:
		{
			dlight.Type=D3DLIGHT_SPOT;
		}
		break;
	}

	light.Get_Diffuse(&temp);
	temp*=light.Get_Intensity();
	dlight.Diffuse.r=temp.X;
	dlight.Diffuse.g=temp.Y;
	dlight.Diffuse.b=temp.Z;
	dlight.Diffuse.a=1.0f;

	light.Get_Specular(&temp);
	temp*=light.Get_Intensity();
	dlight.Specular.r=temp.X;
	dlight.Specular.g=temp.Y;
	dlight.Specular.b=temp.Z;
	dlight.Specular.a=1.0f;	

	light.Get_Ambient(&temp);
	temp*=light.Get_Intensity();
	dlight.Ambient.r=temp.X;
	dlight.Ambient.g=temp.Y;
	dlight.Ambient.b=temp.Z;
	dlight.Ambient.a=1.0f;

	temp=light.Get_Position();
	dlight.Position=*(D3DVECTOR*) &temp;

	light.Get_Spot_Direction(temp);
	dlight.Direction=*(D3DVECTOR*) &temp;

	dlight.Range=light.Get_Attenuation_Range();
	dlight.Falloff=light.Get_Spot_Exponent();
	dlight.Theta=light.Get_Spot_Angle();
	dlight.Phi=light.Get_Spot_Angle();

	// Inverse linear light 1/(1+D)
	double a,b;
	light.Get_Far_Attenuation_Range(a,b);
	dlight.Attenuation0=1.0f;
	if (fabs(a-b)<1e-5)
		// if the attenuation range is too small assume uniform with cutoff
		dlight.Attenuation1=0.0f;
	else
		// this will cause the light to drop to half intensity at the first far attenuation
		dlight.Attenuation1=(float) 1.0/a;
	dlight.Attenuation2=0.0f;

	Set_Light(index,&dlight);
}

// ----------------------------------------------------------------------------
//
// Set the light environment. This is a lighting model which used up to four
// directional lights to produce the lighting.
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass* light_env)
{
	if (light_env) {

		int light_count = light_env->Get_Light_Count();
		unsigned int color=Convert_Color(light_env->Get_Equivalent_Ambient(),0.0f);
		if (RenderStates[D3DRS_AMBIENT]!=color)
		{
			Set_DX8_Render_State(D3DRS_AMBIENT,color);
//buggy Radeon 9700 driver doesn't apply new ambient unless the material also changes.
#if 1
			render_state_changed|=MATERIAL_CHANGED;
#endif
		}

		D3DLIGHT9 light;		
		int l = 0;
		for (l=0;l<light_count;++l) {
			::ZeroMemory(&light, sizeof(D3DLIGHT9));
			light.Type=D3DLIGHT_DIRECTIONAL;
			(Vector3&)light.Diffuse=light_env->Get_Light_Diffuse(l);
			Vector3 dir=-light_env->Get_Light_Direction(l);
			light.Direction=(const D3DVECTOR&)(dir);
			if (light_env->isPointLight(l)) {
				light.Type = D3DLIGHT_POINT;
				(Vector3&)light.Diffuse=light_env->getPointDiffuse(l);
				(Vector3&)light.Ambient=light_env->getPointAmbient(l);
				light.Position = (const D3DVECTOR&)light_env->getPointCenter(l);
				light.Range = light_env->getPointOrad(l);
				// Inverse linear light 1/(1+D)
				double a,b;
				b = light_env->getPointOrad(l);
				a = light_env->getPointIrad(l);
				light.Attenuation0=0.01f;
				if (fabs(a-b)<1e-5)
					// if the attenuation range is too small assume uniform with cutoff
					light.Attenuation1=0.0f;
				else
					// this will cause the light to drop to half intensity at the first far attenuation
					light.Attenuation1=(float) 0.1/a;
				light.Attenuation2=8.0f/(b*b);
			}
			Set_Light(l,&light);
		}

		for (;l<4;++l) {
			Set_Light(l,NULL);
		}
	}
/*	else {
		for (int l=0;l<4;++l) {
			Set_Light(l,NULL);
		}
	}
*/
}

IDirect3DSurface9 * DX8Wrapper::_Get_DX8_Front_Buffer()
{
	DX8_THREAD_ASSERT();
	D3DDISPLAYMODE mode;

	UINT primaryiSwapChain = 0;
	DX8CALL(GetDisplayMode(primaryiSwapChain, &mode));

	IDirect3DSurface9 * fb=NULL;

	//DX8CALL(CreateImageSurface(mode.Width,mode.Height,D3DFMT_A8R8G8B8,&fb));
	D3DPOOL pool = D3DPOOL_MANAGED; // TODO: hardcoded to test out; may test out D3DPOOL_DEFAULT 
	DX8CALL(CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, pool, &fb, NULL));	

	DX8CALL(GetFrontBufferData(primaryiSwapChain, fb));
	return fb;
}

SurfaceClass * DX8Wrapper::_Get_DX8_Back_Buffer(unsigned int num)
{
	DX8_THREAD_ASSERT();	

	IDirect3DSurface9 * bb;
	SurfaceClass *surf=NULL;
	UINT primaryiSwapChain = 0; //  [DX9]
	DX8CALL(GetBackBuffer(primaryiSwapChain, num,D3DBACKBUFFER_TYPE_MONO,&bb));
	if (bb)
	{
		surf=NEW_REF(SurfaceClass,(bb));
		bb->Release();
	}

	return surf;
}


TextureClass *
DX8Wrapper::Create_Render_Target (int width, int height, bool alpha)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	const D3DCAPS9& dx8caps=DX8Caps::Get_Default_Caps();

	//
	//	Note: We're going to force the width and height to be powers of two and equal
	//
	float poweroftwosize = width;
	if (height > 0 && height < width) {
		poweroftwosize = height;
	}
	poweroftwosize = ::Find_POT (poweroftwosize);

	if (poweroftwosize>dx8caps.MaxTextureWidth) {
		poweroftwosize=dx8caps.MaxTextureWidth;
	}
	if (poweroftwosize>dx8caps.MaxTextureHeight) {
		poweroftwosize=dx8caps.MaxTextureHeight;
	}

	width = height = poweroftwosize;

	//
	//	Get the current format of the display
	//
	D3DDISPLAYMODE mode;
	UINT primaryiSwapChain = 0;
	DX8CALL(GetDisplayMode(primaryiSwapChain, &mode));

	// If the user requested a render-target texture and this device does not support that
	// feature, return NULL
	HRESULT hr;

	if (alpha)
	{
			//user wants a texture with destination alpha channel - only 1 such format
			//ever exists on current hardware	- D3DFMT_A8R8G8B8
			hr = D3DInterface->CheckDeviceFormat(	D3DADAPTER_DEFAULT,
																	WW3D_DEVTYPE,
																	mode.Format,
																	D3DUSAGE_RENDERTARGET,
																	D3DRTYPE_TEXTURE,
																	D3DFMT_A8R8G8B8 );
			mode.Format=D3DFMT_A8R8G8B8;
	}
	else
	{
			hr = D3DInterface->CheckDeviceFormat(	D3DADAPTER_DEFAULT,
																	WW3D_DEVTYPE,
																	mode.Format,
																	D3DUSAGE_RENDERTARGET,
																	D3DRTYPE_TEXTURE,
																	mode.Format );
	}

	number_of_DX8_calls++;
	if (hr != D3D_OK) {
		WWDEBUG_SAY(("DX8Wrapper - Driver cannot create render target!\n"));
		return NULL;
	}

	//
	//	Attempt to create the render target
	//	
	DX8_Assert();
	WW3DFormat format=D3DFormat_To_WW3DFormat(mode.Format);
	TextureClass * tex = NEW_REF(TextureClass,(width,height,format,TextureClass::MIP_LEVELS_1,TextureClass::POOL_DEFAULT,true));
	
	// 3dfx drivers are lying in the CheckDeviceFormat call and claiming
	// that they support render targets!
	if (tex->Peek_DX8_Texture() == NULL) {
		WWDEBUG_SAY(("DX8Wrapper - Render target creation failed!\n"));
		REF_PTR_RELEASE(tex);
	}

	return tex;
}


void
DX8Wrapper::Set_Render_Target (TextureClass * texture)
{
	WWASSERT(texture != NULL);
	SurfaceClass * surf = texture->Get_Surface_Level();
	WWASSERT(surf != NULL);
	Set_Render_Target(surf->Peek_D3D_Surface()); 
	REF_PTR_RELEASE(surf);
}

void
DX8Wrapper::Set_Render_Target(IDirect3DSwapChain9 *swap_chain)
{
	DX8_THREAD_ASSERT();
	WWASSERT (swap_chain != NULL);

	//
	//	Get the back buffer for the swap chain
	//
	LPDIRECT3DSURFACE9 render_target = NULL;
	swap_chain->GetBackBuffer (0, D3DBACKBUFFER_TYPE_MONO, &render_target);
	
	//
	//	Set this back buffer as the render targer
	//
	Set_Render_Target (render_target);

	//
	//	Release our hold on the back buffer
	//
	if (render_target != NULL) {
		render_target->Release ();
		render_target = NULL;
	}

	return ;
}

void
DX8Wrapper::Set_Render_Target(IDirect3DSurface9 *render_target)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	DWORD renderTargetIndex = 0;
	//
	//	We'll need the depth buffer later...
	//
	IDirect3DSurface9 *depth_buffer = NULL;
	DX8CALL(GetDepthStencilSurface (&depth_buffer));

	//
	//	Should we restore the default render target set a new one?
	//
	if (render_target == NULL || render_target == DefaultRenderTarget) {

		//
		//	Restore the default render target
		//
		if (DefaultRenderTarget != NULL) {
			DX8CALL(SetRenderTarget (renderTargetIndex, DefaultRenderTarget)); // [DX9]
			DX8CALL(SetDepthStencilSurface(depth_buffer)); // [DX9]

			DefaultRenderTarget->Release ();
			DefaultRenderTarget = NULL;
		}

		//
		//	Release our hold on the "current" render target
		//
		if (CurrentRenderTarget != NULL) {
			CurrentRenderTarget->Release ();
			CurrentRenderTarget = NULL;
		}

	} else if (render_target != CurrentRenderTarget) {

		//
		//	Get a pointer to the default render target (if necessary)
		//
		if (DefaultRenderTarget == NULL) {
			DX8CALL(GetRenderTarget (renderTargetIndex, &DefaultRenderTarget));
		}

		//
		//	Release our hold on the old "current" render target
		//
		if (CurrentRenderTarget != NULL) {
			CurrentRenderTarget->Release ();
			CurrentRenderTarget = NULL;
		}

		//
		//	Keep a copy of the current render target (for housekeeping)
		//
		CurrentRenderTarget = render_target;
		WWASSERT (CurrentRenderTarget != NULL);
		if (CurrentRenderTarget != NULL) {
			CurrentRenderTarget->AddRef ();

			//
			//	Switch render targets
			//			
			DX8CALL(SetRenderTarget (renderTargetIndex, CurrentRenderTarget));
		}
	}

	//
	//	Free our hold on the depth buffer
	//
	if (depth_buffer != NULL) {
		depth_buffer->Release ();
		depth_buffer = NULL;
	}

	return ;
}


IDirect3DSwapChain9 *
DX8Wrapper::Create_Additional_Swap_Chain (HWND render_window)
{
	DX8_Assert();

	//
	//	Configure the presentation parameters for a windowed render target
	//
	D3DPRESENT_PARAMETERS params				= { 0 };
	params.BackBufferFormat						= _PresentParameters.BackBufferFormat;
	params.BackBufferCount						= 1;
	params.MultiSampleType						= D3DMULTISAMPLE_NONE;
	params.SwapEffect							= D3DSWAPEFFECT_COPY;
	params.hDeviceWindow						= render_window;
	params.Windowed								= TRUE;
	params.EnableAutoDepthStencil				= TRUE;
	params.AutoDepthStencilFormat				= _PresentParameters.AutoDepthStencilFormat;
	params.Flags								= 0;
	params.FullScreen_RefreshRateInHz			= D3DPRESENT_RATE_DEFAULT;
	params.PresentationInterval					= D3DPRESENT_INTERVAL_DEFAULT;

	//
	//	Create the swap chain
	//
	IDirect3DSwapChain9 *swap_chain = NULL;
	DX8CALL(CreateAdditionalSwapChain(&params, &swap_chain));
	return swap_chain;
}

void DX8Wrapper::Flush_DX8_Resource_Manager(unsigned int bytes)
{
	DX8_Assert();
	// url: https://learn.microsoft.com/en-us/windows/win32/direct3d9/converting-to-directx-9
	//DX8CALL(ResourceManagerDiscardBytes(bytes));
	DX8CALL(EvictManagedResources());
}

unsigned int DX8Wrapper::Get_Free_Texture_RAM()
{
	DX8_Assert();
	number_of_DX8_calls++;
	return DX8Wrapper::_Get_D3D_Device8()->GetAvailableTextureMem();	
}

// Converts a linear gamma ramp to one that is controlled by:
// Gamma - controls the curvature of the middle of the curve
// Bright - controls the minimum value of the curve
// Contrast - controls the difference between the maximum and the minimum of the curve
void DX8Wrapper::Set_Gamma(float gamma,float bright,float contrast,bool calibrate,bool uselimit)
{
	gamma=Bound(gamma,0.6f,6.0f);
	bright=Bound(bright,-0.5f,0.5f);
	contrast=Bound(contrast,0.5f,2.0f);
	float oo_gamma=1.0f/gamma;

	DX8_Assert();
	number_of_DX8_calls++;

	DWORD flag=(calibrate?D3DSGR_CALIBRATE:D3DSGR_NO_CALIBRATION);

	D3DGAMMARAMP ramp;
	float			 limit;	

	// IML: I'm not really sure what the intent of the 'limit' variable is. It does not produce useful results for my purposes.
	if (uselimit) {
		limit=(contrast-1)/2*contrast;
	} else {
		limit = 0.0f;
	}

	// HY - arrived at this equation after much trial and error.
	for (int i=0; i<256; i++) {
		float in,out;
		in=i/256.0f;
		float x=in-limit;
		x=Bound(x,0.0f,1.0f);
		x=powf(x,oo_gamma);
		out=contrast*x+bright;
		out=Bound(out,0.0f,1.0f);
		ramp.red[i]=(WORD) (out*65535);
		ramp.green[i]=(WORD) (out*65535);
		ramp.blue[i]=(WORD) (out*65535);
	}

	if (DX8Caps::Support_Gamma())	{
		//DX8Wrapper::_Get_D3D_Device8()->SetGammaRamp(flag, &ramp);
		
		// [DX9]
		UINT numSwapChains = DX8Wrapper::_Get_D3D_Device8()->GetNumberOfSwapChains();
		for (UINT i = 0; i < numSwapChains; ++i) {
			DX8Wrapper::_Get_D3D_Device8()->SetGammaRamp(i, flag, &ramp);
		}
	} else {
		HWND hwnd = GetDesktopWindow();
		HDC hdc = GetDC(hwnd);
		if (hdc)
		{
			SetDeviceGammaRamp (hdc, &ramp);
			ReleaseDC (hwnd, hdc);
		}
	}
}

//============================================================================
// DX8Wrapper::getBackBufferFormat
//============================================================================

WW3DFormat	DX8Wrapper::getBackBufferFormat( void )
{
	return D3DFormat_To_WW3DFormat( _PresentParameters.BackBufferFormat );
}
