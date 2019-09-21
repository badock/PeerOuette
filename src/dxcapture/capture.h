//
// Created by jonathan on 11/20/2018.
//

#ifndef GAMECLIENTSDL_CAPTURE_H
#define GAMECLIENTSDL_CAPTURE_H
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "src/streaming/streaming.h"
#include "src/texture_converter/TextureConverter.h"

#define NUMVERTICES 6

using namespace Microsoft::WRL;

enum CaptureMode {
	RGBA,
	YUV420P
};

//
// FRAME_DATA holds information about an acquired frame
//
typedef struct _D3D_FRAME_DATA
{
	ID3D11Texture2D* Frame;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData;
	UINT DirtyCount;
	UINT MoveCount;
} D3D_FRAME_DATA;

//
// A vertex with a position and texture coordinate
//
typedef struct _VERTEX
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexCoord;
} VERTEX;

typedef struct _CaptureContext {
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGIDevice* dxgi_device;
	IDXGIOutputDuplication* m_dup;
	ID3D11Texture2D* m_AcquiredDesktopImage;

	BYTE* m_MetaDataBuffer; // INPUT
	UINT m_MetaDataSize; // INPUT

	Microsoft::WRL::ComPtr<ID3D11Device>        ptr_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> ptr_context;

	D3D_FEATURE_LEVEL       m_featureLevel;
	IDXGIAdapter1 * adapter;

	unsigned int m_width;
	unsigned int m_height;

	std::unique_ptr<TextureConverter> texture_converter;
	// ComPtr<ID3D11Texture2D> m_ftextures_yuv420p[3];

	CaptureMode capture_mode;	
	StreamingEnvironment* se;

	D3D11_TEXTURE2D_DESC texDesc;
} CaptureContext;

int init_directx(CaptureContext* cc);
// RGB
int init_capture(CaptureContext* cc);
int capture_frame(CaptureContext* cc, D3D_FRAME_DATA* Data, FrameData* ffmpeg_frame_data);
int capture_frame_yuv420p(CaptureContext* cc, D3D_FRAME_DATA* Data);
int done_with_frame(CaptureContext* cc);
int get_pixels(CaptureContext* cc, FrameData* ffmpeg_frame_data);
int get_pixels_yuv420p(CaptureContext* cc, FrameData* ffmpeg_frame_data);
int init_video_mode(CaptureContext* cc);
#endif //GAMECLIENTSDL_CAPTURE_H
