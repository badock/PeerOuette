/*
This code is very inspired from:
https://github.com/gnif/LookingGlass/blob/master/host/TextureConverter.h
*/

#pragma once
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "src/streaming/streaming.h"

#include <DirectXMath.h>
#include <vector>

typedef std::vector<ID3D11Texture2D*> TextureList;

typedef enum FrameType
{
	FRAME_TYPE_INVALID,
	FRAME_TYPE_ARGB, // ABGR interleaved: A,R,G,B 32bpp
	FRAME_TYPE_YUV420, // YUV420
	FRAME_TYPE_H264, // H264 compressed
	FRAME_TYPE_MAX, // sentinel value
}
FrameType;

class TextureConverter
{
public:
  TextureConverter();
  ~TextureConverter();

  bool Initialize(
    ID3D11DeviceContext* deviceContext,
    ID3D11Device*        device,
    const unsigned int     width,
    const unsigned int     height,
    FrameType              format
  );

  void DeInitialize();

  bool Convert(ID3D11Texture2D* texture, TextureList & output);

private:
  struct VS_INPUT
  {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
  };

  bool IntializeBuffers();

  ID3D11DeviceContext* m_deviceContext;
  ID3D11Device*        m_device;
  unsigned int           m_width, m_height;
  FrameType              m_format;

  DXGI_FORMAT                 m_texFormats   [3];
  unsigned int                m_scaleFormats [3];

  int initialised = 0;
  ID3D11Texture2D*          m_destTexture[3];
  ID3D11Texture2D*          m_targetTexture[3];
  ID3D11RenderTargetView*   m_renderView   [3];
  ID3D11ShaderResourceView* m_shaderView   [3];

  ID3D11InputLayout*        m_layout;
  ID3D11VertexShader*       m_vertexShader;
  ID3D11PixelShader*        m_psCopy;
  ID3D11PixelShader*        m_psConversion;
  ID3D11SamplerState*       m_samplerState;

  ID3D11Buffer*             m_vertexBuffer;
  unsigned int                m_vertexCount;
  ID3D11Buffer*             m_indexBuffer;
  unsigned int                m_indexCount;
};

