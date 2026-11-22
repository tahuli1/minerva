#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11.h>
#include <wrl/client.h>

namespace gui {

// A Win32 window with a Direct3D 11 swap chain to draw into: the part of a GUI app
// that ImGui leaves to the program. ComPtr releases the Direct3D objects.
//
// Main thread only, like every window on Windows.
class GraphicsWindow {
public:
    // `wndProc` gets the window's messages (keys, resizing, closing).
    bool create(const wchar_t* title, int width, int height, WNDPROC wndProc);
    void destroy();

    // After the window is resized: the swap chain's buffers must match it.
    void resize(UINT width, UINT height);
    // Clears the frame to `rgba` and makes it the target for drawing.
    void beginFrame(const float rgba[4]);
    // Shows the frame, waiting for the screen's next refresh (vsync) so the main
    // thread draws at the screen's rate instead of spinning.
    void present();

    HWND hwnd() const { return hwnd_; }
    ID3D11Device* device() const { return device_.Get(); }
    ID3D11DeviceContext* context() const { return context_.Get(); }

private:
    bool createDevice();
    void createTarget();

    HWND hwnd_ = nullptr;
    WNDCLASSEXW windowClass_{};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
};

} // namespace gui
