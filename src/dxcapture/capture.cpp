//
// Created by jonathan on 11/20/2018.
//
#define NOMINMAX
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

		cc->se->width = cc->m_width;
		cc->se->height = cc->m_height;
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

	HRESULT status;
	D3D11_TEXTURE2D_DESC texDesc = cc->texDesc;

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

	bool texDescSizeReduce = false;

	for (int i = 0; i < 3; ++i) {
		status = cc->device->CreateTexture2D(&texDesc, NULL, &cc->textures[i]);
		if (FAILED(status)) {
			log_error("Failed to create texture", status);
			return false;
		}

		if (!texDescSizeReduce) {
			texDesc.Width /= 2;
			texDesc.Height /= 2;

			texDescSizeReduce = true;
		}
	}

	cc->texture_converter = std::make_unique<TextureConverter>();
	if (!cc->texture_converter->Initialize(cc->context, cc->device, cc->m_width, cc->m_height, FRAME_TYPE_YUV420)) {
		return false;
	}

	return true;
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
			log_error("I think that this is caused by a screen resolution change : I will create a new flow");
			cc->se->flow_id += 1;
			log_error("A new flow with ID=%d has been created!", cc->se->flow_id);
		}
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (DXGI_ERROR_WAIT_TIMEOUT)");
		}
		if (hr == DXGI_ERROR_INVALID_CALL) {
			log_error("Failed to acquire next frame in DUPLICATIONMANAGER (DXGI_ERROR_INVALID_CALL)");
			log_error("I think that this is caused by a ctrl+del : I will create a new flow");
			cc->se->flow_id += 1;
			log_error("A new flow with ID=%d has been created!", cc->se->flow_id);
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
	
    Data->Frame = cc->m_AcquiredDesktopImage;
    Data->FrameInfo = FrameInfo;

	return 0;
}

int done_with_frame(CaptureContext* cc) {
	
	HRESULT hr = cc->m_dup->ReleaseFrame();
	if (FAILED(hr))
	{
		log_error("Failed to release frame in DUPLICATIONMANAGER");
		log_error("It may be caused by a screen resolution change!");
		return -1;
	}	

	return 0;
}

int get_pixels_yuv420p(CaptureContext* cc, FrameData* ffmpeg_frame_data) {

	int result;
	TextureList planes;
	if (!cc->texture_converter->Convert(cc->m_AcquiredDesktopImage, planes))
		return -1;

	for (int i = 0; i < 3; ++i) {
		ComPtr<ID3D11Texture2D> t = planes.at(i);
		cc->context->CopyResource(cc->textures[i].Get(), t.Get());
	}

	for (int i = 0; i < 3; ++i)	{
		HRESULT                  status;
		D3D11_MAPPED_SUBRESOURCE mapping;
		D3D11_TEXTURE2D_DESC     desc;

		cc->textures[i]->GetDesc(&desc);

		status = cc->context->Map(cc->textures[i].Get(), 0, D3D11_MAP_READ, 0, &mapping);

		if (FAILED(status)) {
			log_error("Failed to map the texture %i", status);
			//DeInitialize();
			return -1;
		}

		const unsigned int size = desc.Height * desc.Width;

		ffmpeg_frame_data->pFrame->data[i] = (uint8_t *) mapping.pData;
		ffmpeg_frame_data->pFrame->linesize[i] = mapping.RowPitch;

		cc->context->Unmap(cc->textures[i].Get(), 0);
	}

	ffmpeg_frame_data->pitch = cc->m_width;
	ffmpeg_frame_data->stride = cc->m_width;
	ffmpeg_frame_data->flow_id = cc->se->flow_id;

	return 0;
}