#include "window/texture.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <wincodec.h>

#include <vector>

namespace gui {

Texture loadPngResource(ID3D11Device* device, int resourceId, int width, int height) {
    using Microsoft::WRL::ComPtr;
    Texture texture;

    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10)); // 10: RT_RCDATA
    HGLOBAL loaded = resource ? LoadResource(nullptr, resource) : nullptr;
    const void* bytes = loaded ? LockResource(loaded) : nullptr;
    if (bytes == nullptr) return texture;
    const DWORD size = SizeofResource(nullptr, resource);

    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> premultiplied;
    ComPtr<IWICBitmapScaler> scaler;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(bytes)), size)) ||
        FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) ||
        // Scale with premultiplied alpha: otherwise the hidden colour of fully
        // transparent pixels is averaged into the edges as a dark fringe.
        FAILED(factory->CreateFormatConverter(&premultiplied)) ||
        FAILED(premultiplied->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                         nullptr, 0.0, WICBitmapPaletteTypeCustom)) ||
        FAILED(factory->CreateBitmapScaler(&scaler)) ||
        FAILED(scaler->Initialize(premultiplied.Get(), static_cast<UINT>(width), static_cast<UINT>(height),
                                  WICBitmapInterpolationModeHighQualityCubic)) ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        // ImGui blends with straight (not premultiplied) alpha, as RGBA bytes.
        FAILED(converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr,
                                     0.0, WICBitmapPaletteTypeCustom)))
        return texture;

    const UINT w = static_cast<UINT>(width), h = static_cast<UINT>(height);
    std::vector<BYTE> pixels(static_cast<size_t>(w) * h * 4);
    if (FAILED(converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(pixels.size()), pixels.data())))
        return texture;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{pixels.data(), w * 4, 0};
    ComPtr<ID3D11Texture2D> image;
    if (FAILED(device->CreateTexture2D(&desc, &data, &image)) ||
        FAILED(device->CreateShaderResourceView(image.Get(), nullptr, &texture.view)))
        return Texture{};
    texture.width = width;
    texture.height = height;
    return texture;
}

} // namespace gui
