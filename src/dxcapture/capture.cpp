//
// Created by jonathan on 11/20/2018.
//

#include "capture.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <src/log/log.h>
#include <dxgi1_3.h>
#include <dxgi.h>
#include <wrl.h>

using namespace DirectX;

int init_directx(CaptureContext* cc)
{

	D3D_FEATURE_LEVEL levels[] = {
	D3D_FEATURE_LEVEL_9_1,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_11_1
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

int capture_frame(CaptureContext* cc, D3D_FRAME_DATA* Data) {
	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	int timeout = false;

	// Get new frame
	HRESULT hr = cc->m_dup->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
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

int get_pixel_map(CaptureContext* cc, D3D_FRAME_DATA* Data, FrameData* ffmpeg_frame_data) {

	HRESULT hr;

	// Vertices for drawing whole texture
	VERTEX Vertices[NUMVERTICES] =
	{
		{XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
		{XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f)},
	};

	D3D11_TEXTURE2D_DESC FrameDesc;
	cc->m_AcquiredDesktopImage->GetDesc(&FrameDesc);

	// Set resources
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;

	D3D11_BUFFER_DESC BufferDesc;
	RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	RtlZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Vertices;

	ID3D11Buffer* VertexBuffer = nullptr;

	// Create vertex buffer
	hr = cc->device->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);
	if (FAILED(hr))
	{
		//ShaderResource->Release();
		//ShaderResource = nullptr;
		log_error("Failed to create vertex buffer when drawing a frame");
		return -1;
	}
	cc->context->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

	// Draw textured quad onto render target
	cc->context->Draw(NUMVERTICES, 0);

	// <badock>
	// Desktop dimensions
	D3D11_TEXTURE2D_DESC FullDesc;
	cc->m_AcquiredDesktopImage->GetDesc(&FullDesc);
	INT DesktopWidth = FullDesc.Width;
	INT DesktopHeight = FullDesc.Height;

	// Staging buffer/texture
	D3D11_TEXTURE2D_DESC CopyBufferDesc;
	CopyBufferDesc.Width = DesktopWidth;
	CopyBufferDesc.Height = DesktopHeight;
	CopyBufferDesc.MipLevels = 1;
	CopyBufferDesc.ArraySize = 1;
	CopyBufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	CopyBufferDesc.SampleDesc.Count = 1;
	CopyBufferDesc.SampleDesc.Quality = 0;
	CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
	CopyBufferDesc.BindFlags = 0;
	CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	CopyBufferDesc.MiscFlags = 0;

	ID3D11Texture2D* CopyBuffer = nullptr;
	hr = cc->device->CreateTexture2D(&CopyBufferDesc, nullptr, &CopyBuffer);

	if (FAILED(hr))
	{
		log_error("Failed creating staging texture for pointer");
		return -1;
	}

	// Copy needed part of desktop image
	D3D11_BOX *Box = new D3D11_BOX();
	Box->left = 0;
	Box->top = 0;
	Box->right = DesktopWidth;
	Box->bottom = DesktopHeight;
	cc->context->CopySubresourceRegion(CopyBuffer, 0, 0, 0, 0, cc->m_AcquiredDesktopImage, 0, Box);


	// QI for IDXGISurface
	IDXGISurface* CopySurface = nullptr;
	hr = CopyBuffer->QueryInterface(__uuidof(IDXGISurface), (void **)&CopySurface);
	CopyBuffer->Release();
	CopyBuffer = nullptr;
	if (FAILED(hr))
	{
		log_error("Failed to QI staging texture into IDXGISurface for pointer");
		return -1;
	}

	// Map pixels
	DXGI_MAPPED_RECT MappedSurface;
	hr = CopySurface->Map(&MappedSurface, DXGI_MAP_READ);
	if (FAILED(hr))
	{
		CopySurface->Release();
		CopySurface = nullptr;
		log_error("Failed to map surface for pointer");
		return -1;
	}

	UINT* Desktop32 = reinterpret_cast<UINT*>(MappedSurface.pBits);
	UINT  DesktopPitchInPixels = MappedSurface.Pitch / sizeof(UINT);
	UINT pixel;

	// Badock: Iterate over pixels!
	for (INT Row = 0; Row < DesktopHeight; ++Row)
	{
		for (INT Col = 0; Col < DesktopWidth; ++Col)
		{
			// Set new pixel
			pixel = Desktop32[(Row * DesktopPitchInPixels) + Col];
			//ffmpeg_frame_data->pFrame->data[0][(Col* DesktopWidth) + Row] = pixel;
		}
	}

	//avpicture_fill(ffmpeg_frame_data->pFrame, buffer, pix_fmt, width, height);

	// Done with resource
	hr = CopySurface->Unmap();
	CopySurface->Release();
	CopySurface = nullptr;
	if (FAILED(hr))
	{
		log_error("Failed to unmap surface for pointer");
		return -1;
	}
	// </badock>

	VertexBuffer->Release();
	VertexBuffer = nullptr;

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