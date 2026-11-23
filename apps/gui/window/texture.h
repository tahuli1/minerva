#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace gui {

// An image on the graphics card, ready for ImGui::Image().
struct Texture {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
    int width = 0;
    int height = 0;
};

// Loads a PNG that is built into the exe as an RCDATA resource (the .rc file),
// decoding it with WIC, the image reader that ships with Windows. Returns an
// empty Texture if anything fails, so the caller can simply skip the image.
// Needs COM initialized on this thread.
//
// The image is scaled to `width` x `height`, the size it is drawn at. ImGui's
// sampler reads only the full-size image (no mipmaps), so a large image drawn
// small would skip most of its pixels and look grainy; WIC's filter averages them.
Texture loadPngResource(ID3D11Device* device, int resourceId, int width, int height);

} // namespace gui
