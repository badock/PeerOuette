//
// Created by jonathan on 11/20/2018.
//

#include "capture.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
//#include <src/log/log.h>
#include <dxgi1_3.h>
#include <dxgi.h>
#include <wrl.h>
#include <wincodec.h>
#include <exception>
#include <algorithm>
#include <memory>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

class myexception : public exception
{
	virtual const char* what() const throw()
	{
		return "My exception happened";
	}
} myexception;;

int init_directx(CaptureContext* cc)
{

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// This flag adds support for surfaces with a color-channel ordering different
	// from the API default. It is required for compatibility with Direct2D.
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create the Direct3D 11 API device object and a corresponding context.
	Microsoft::WRL::ComPtr<ID3D11Device>        device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	D3D_FEATURE_LEVEL       m_featureLevel;

	HRESULT hr = D3D11CreateDevice(
		nullptr,                    // Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
		0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
		deviceFlags,                // Set debug and Direct2D compatibility flags.
		levels,                     // List of feature levels this app can support.
		ARRAYSIZE(levels),          // Size of the list above.
		D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&device,                    // Returns the Direct3D device created.
		&m_featureLevel,            // Returns feature level of device created.
		&context                    // Returns the device immediate context.
	);

	if (FAILED(hr))
	{
		// Handle device interface creation failure if it occurs.
		// For example, reduce the feature level requirement, or fail over 
		// to WARP rendering.
		int i = 0;
	}

	// Store pointers to the Direct3D 11.1 API device and immediate context.
	cc->device = device.Get();
	cc->context = context.Get();

	// Find adapter
	UINT i = 0;
	IDXGIAdapter1 * adapter;
	IDXGIFactory1* factory;

	IDXGIDevice* dxgi_device;
	hr = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgi_device);
	cc->dxgi_device = dxgi_device;

	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

	for (int i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
		break;
	}
	cc->adapter = adapter;

	return 0;
}


int init_capture(CaptureContext* cc) {

	IDXGIOutput* output;
	IDXGIOutput1* m_output;
	for (int i = 0; cc->adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++) {

		DXGI_OUTPUT_DESC outputDesc;
		output->GetDesc(&outputDesc);
		if (!outputDesc.AttachedToDesktop)
		{
			output = NULL;
			continue;
		}

		m_output = (IDXGIOutput1*) output;
		if (!m_output)
		{
			log_error("Failed to get IDXGIOutput1");
			//DeInitialize();
			return false;
		}

		DXGI_ADAPTER_DESC adapterDesc;
		cc->adapter->GetDesc(&adapterDesc);
		log_info("Device Description: %ls", adapterDesc.Description);
		log_info("Device Vendor ID : 0x%x", adapterDesc.VendorId);
		log_info("Device Device ID : 0x%x", adapterDesc.DeviceId);
		log_info("Device Video Mem : %lld MB", adapterDesc.DedicatedVideoMemory / 1048576);
		log_info("Device Sys Mem   : %lld MB", adapterDesc.DedicatedSystemMemory / 1048576);
		log_info("Shared Sys Mem   : %lld MB", adapterDesc.SharedSystemMemory / 1048576);

		cc->m_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
		cc->m_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
		log_info("Capture Size     : %u x %u", cc->m_width, cc->m_height);
	}

	cc->dxgi_device->SetGPUThreadPriority(7);

	for (int i = 0; i < 2; ++i)
	{
		HRESULT status = m_output->DuplicateOutput(cc->device, &cc->m_dup);
		if (SUCCEEDED(status))
			break;
		Sleep(200);
	}

	return 0;
}

int init_video_mode(CaptureContext* cc) {

	if (cc->capture_mode == RGBA) {
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = cc->m_width;
		texDesc.Height = cc->m_height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.MiscFlags = 0;

		HRESULT status = cc->device->CreateTexture2D(&texDesc, NULL, &cc->m_ftexture_rgba);
		if (FAILED(status))
		{
			log_error("Failed to create texture", status);
			return false;
		}

		return true;

	} else if (cc->capture_mode == YUV420P) {
		HRESULT status;
		D3D11_TEXTURE2D_DESC texDesc;

		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = cc->m_width;
		texDesc.Height = cc->m_height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.MiscFlags = 0;

		status = cc->device->CreateTexture2D(&texDesc, NULL, &cc->m_ftextures_yuv420p[0]);
		if (FAILED(status))
		{
			log_error("Failed to create texture", status);
			return false;
		}

		texDesc.Width /= 2;
		texDesc.Height /= 2;

		status = cc->device->CreateTexture2D(&texDesc, NULL, &cc->m_ftextures_yuv420p[1]);
		if (FAILED(status))
		{
			log_error("Failed to create texture", status);
			return false;
		}

		status = cc->device->CreateTexture2D(&texDesc, NULL, &cc->m_ftextures_yuv420p[2]);
		if (FAILED(status))
		{
			log_error("Failed to create texture", status);
			return false;
		}

		cc->texture_converter = new TextureConverter();
		if (!cc->texture_converter->Initialize(cc->context, cc->device, cc->m_width, cc->m_height, FRAME_TYPE_YUV420))
			return false;

		return true;
	}
	
	return 0;
}

int capture_frame(CaptureContext* cc, D3D_FRAME_DATA* Data, FrameData* ffmpeg_frame_data) {
	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	int timeout = false;

	// Get new frame
	HRESULT hr = cc->m_dup->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
	ffmpeg_frame_data->dxframe_acquired_time_point = std::chrono::system_clock::now();

	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		timeout = true;
		return -1;
	}

	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_ACCESS_LOST) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (DXGI_ERROR_ACCESS_LOST)");
		}
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (DXGI_ERROR_WAIT_TIMEOUT)");
		}
		if (hr == DXGI_ERROR_INVALID_CALL) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (DXGI_ERROR_INVALID_CALL)");
		}
		if (hr == E_INVALIDARG) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (E_INVALIDARG)");
		}
		log_error("Failed to acquire next frame in DUPLICATIONMANAGER");
		return -1;
	}

	// QI for IDXGIResource
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&cc->m_AcquiredDesktopImage));
	DesktopResource->Release();
	
	DesktopResource = nullptr;
	if (FAILED(hr))
	{
		log_error("Failed to QI for ID3D11Texture2D from acquired IDXGIResource in DUPLICATIONMANAGER");
		return -1;
	}

	// Get metadata
    if (FrameInfo.TotalMetadataBufferSize)
    {
        // Old buffer too small
        if (FrameInfo.TotalMetadataBufferSize > cc->m_MetaDataSize)
        {
            if (cc->m_MetaDataBuffer)
            {
                delete [] cc->m_MetaDataBuffer;
                cc->m_MetaDataBuffer = nullptr;
            }
            cc->m_MetaDataBuffer = new BYTE[FrameInfo.TotalMetadataBufferSize];
            if (!cc->m_MetaDataBuffer)
            {
                cc->m_MetaDataSize = 0;
                Data->MoveCount = 0;
                Data->DirtyCount = 0;
				log_error("Failed to allocate memory for metadata in DUPLICATIONMANAGER");
                return -1;
            }
            cc->m_MetaDataSize = FrameInfo.TotalMetadataBufferSize;
        }

        UINT BufSize = FrameInfo.TotalMetadataBufferSize;

        // Get move rectangles
        hr = cc->m_dup->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(cc->m_MetaDataBuffer), &BufSize);
        if (FAILED(hr))
        {
            Data->MoveCount = 0;
            Data->DirtyCount = 0;
			log_error("Failed to get frame move rects in DUPLICATIONMANAGER");
            return -1;
        }
        Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);

        BYTE* DirtyRects = cc->m_MetaDataBuffer + BufSize;
        BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

        // Get dirty rectangles
        hr = cc->m_dup->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
        if (FAILED(hr))
        {
            Data->MoveCount = 0;
            Data->DirtyCount = 0;

			if (hr == DXGI_ERROR_ACCESS_LOST) {
				log_error("Failed to get frame dirty rects in DUPLICATIONMANAGER (DXGI_ERROR_ACCESS_LOST)");
			}
			if (hr == DXGI_ERROR_MORE_DATA) {
				log_error("Failed to get frame dirty rects in DUPLICATIONMANAGER (DXGI_ERROR_MORE_DATA)");
			}
			if (hr == DXGI_ERROR_INVALID_CALL) {
				log_error("Failed to get frame dirty rects in DUPLICATIONMANAGER (DXGI_ERROR_INVALID_CALL)");
			}
			if (hr == E_INVALIDARG) {
				log_error("Failed to get frame dirty rects in DUPLICATIONMANAGER (E_INVALIDARG)");
			}

			log_error("Failed to get frame dirty rects in DUPLICATIONMANAGER");
            return -1;
        }
        Data->DirtyCount = BufSize / sizeof(RECT);

        Data->MetaData = cc->m_MetaDataBuffer;
    }

    Data->Frame = cc->m_AcquiredDesktopImage;
    Data->FrameInfo = FrameInfo;


	return 0;
}

int done_with_frame(CaptureContext* cc) {
	HRESULT hr = cc->m_dup->ReleaseFrame();
	if (FAILED(hr))
	{
		log_error("Failed to release frame in DUPLICATIONMANAGER");
		return -1;
	}

	return 0;
}


int get_pixels(CaptureContext* cc, FrameData* ffmpeg_frame_data) {
	int               result;
	D3D11_MAPPED_SUBRESOURCE mapping;
	ID3D11Texture2D* m_ftexture;

	// Get the device context
	ComPtr<ID3D11Device> d3dDevice;
	cc->m_AcquiredDesktopImage->GetDevice(&d3dDevice);
	ComPtr<ID3D11DeviceContext> d3dContext;
	d3dDevice->GetImmediateContext(&d3dContext);
	
	// First verify that we can map the texture
	D3D11_TEXTURE2D_DESC desc;
	cc->m_AcquiredDesktopImage->GetDesc(&desc);

	//ID3D11Texture2D* m_ftexture;
	D3D11_TEXTURE2D_DESC desc2;
	desc2.Width = desc.Width;
	desc2.Height = desc.Height;
	desc2.MipLevels = desc.MipLevels;
	desc2.ArraySize = desc.ArraySize;
	desc2.Format = desc.Format;
	desc2.SampleDesc = desc.SampleDesc;
	desc2.Usage = D3D11_USAGE_STAGING;
	desc2.BindFlags = 0;
	desc2.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc2.MiscFlags = 0;
	HRESULT hr = d3dDevice->CreateTexture2D(&desc2, nullptr, &m_ftexture);
	if (FAILED(hr)) {
		log_error("Failed to create staging texture");
		throw myexception;
	}


	//cc->context->CopyResource(cc->m_AcquiredDesktopImage, m_ftexture);
	cc->context->CopyResource(m_ftexture, cc->m_AcquiredDesktopImage);


	HRESULT status;
	status = cc->context->Map(m_ftexture, 0, D3D11_MAP_READ, 0, &mapping);
	if (FAILED(status))
	{
		// REGARDER ICI!!!!!!
		return -1;
	}

	uint8_t* first_value = (uint8_t *) mapping.pData;
	int b = *(first_value + (mapping.RowPitch * 5));
	int g = *(first_value + (mapping.RowPitch * 5) + sizeof(uint8_t));
	int r = *(first_value + (mapping.RowPitch * 5) + 2 * sizeof(uint8_t));
	int a = *(first_value + (mapping.RowPitch * 5) + 3 * sizeof(uint8_t));

	//log_info("RGBA(%i, %i, %i, %i)", r, g, b, a);

	return 1;

	uint8_t* buffer = (uint8_t *) malloc(sizeof(uint8_t) * 1920 * 1080 * 4);
	int pitch = cc->m_width * 4;
	int stride = cc->m_width;

	
	if (pitch == mapping.RowPitch)
		memcpy
		(buffer, mapping.pData, pitch * cc->m_height);
	else
		for (unsigned int y = 0; y < cc->m_height; ++y)
			memcpy(
			(uint8_t *)buffer + (pitch      * y),
				(uint8_t *)mapping.pData + (mapping.RowPitch * y),
				pitch
			);

	cc->context->Unmap(cc->m_AcquiredDesktopImage, 0);
	free(buffer);

	return 0;
}

// The following function is inspired by https://github.com/brichard19/memcpy_sse
void memcpy_sse(void *dest, const void *src, size_t count)
{
	__m128i *srcPtr = (__m128i *)src;
	__m128i *destPtr = (__m128i *)dest;

	unsigned int index = 0;
	while (count) {

		__m128i x = _mm_load_si128(&srcPtr[index]);
		_mm_stream_si128(&destPtr[index], x);

		count -= 16;
		index++;
	}
}

int get_pixels_yuv420p(CaptureContext* cc, FrameData* ffmpeg_frame_data) {

	int               result;
	TextureList planes;
	if (!cc->texture_converter->Convert(cc->m_AcquiredDesktopImage, planes))
		return -1;

	for (int i = 0; i < 3; ++i)
	{
		ID3D11Texture2D* t = planes.at(i);
		cc->context->CopyResource(cc->m_ftextures_yuv420p[i], t);
	}

	//result = done_with_frame(cc);
	//if (result != 0)
	//	return result;
	
	//uint8_t * data = (uint8_t *) ffmpeg_frame_data->buffer;
	//size_t    remain = ffmpeg_frame_data->numBytes;
	for (int i = 0; i < 3; ++i)
	{
		HRESULT                  status;
		D3D11_MAPPED_SUBRESOURCE mapping;
		D3D11_TEXTURE2D_DESC     desc;

		cc->m_ftextures_yuv420p[i]->GetDesc(&desc);
		status = cc->context->Map(cc->m_ftextures_yuv420p[i], 0, D3D11_MAP_READ, 0, &mapping);

		if (FAILED(status))
		{
			log_error("Failed to map the texture %i", status);
			//DeInitialize();
			return -1;
		}

		const unsigned int size = desc.Height * desc.Width;

		//memcpy_sse(ffmpeg_frame_data->pFrame->data[i], (uint8_t *) mapping.pData, size);
		//memcpy(ffmpeg_frame_data->pFrame->data[i], (uint8_t *)mapping.pData, size);
		ffmpeg_frame_data->pFrame->data[i] = (uint8_t *)mapping.pData;

		cc->context->Unmap(cc->m_ftextures_yuv420p[i], 0);
	}

	ffmpeg_frame_data->pitch = cc->m_width;
	ffmpeg_frame_data->stride = cc->m_width;

	return 0;
}