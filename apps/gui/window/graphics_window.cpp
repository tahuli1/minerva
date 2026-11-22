#include "window/graphics_window.h"

#include <initializer_list>

namespace gui {

bool GraphicsWindow::create(const wchar_t* title, int width, int height, WNDPROC wndProc) {
    windowClass_ = {sizeof(WNDCLASSEXW)};
    windowClass_.style = CS_CLASSDC;
    windowClass_.lpfnWndProc = wndProc;
    windowClass_.hInstance = GetModuleHandleW(nullptr);
    windowClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass_.lpszClassName = L"MinervaWindow";
    RegisterClassExW(&windowClass_);

    hwnd_ = CreateWindowW(windowClass_.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          width, height, nullptr, nullptr, windowClass_.hInstance, nullptr);
    if (hwnd_ == nullptr || !createDevice()) {
        destroy();
        return false;
    }
    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);
    return true;
}

bool GraphicsWindow::createDevice() {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd_;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    // The graphics card first; WARP (Windows' software renderer) if there is none.
    for (D3D_DRIVER_TYPE driver : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP}) {
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                                                    &desc, &swapChain_, &device_, nullptr, &context_))) {
            createTarget();
            return true;
        }
    }
    return false;
}

void GraphicsWindow::createTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &target_);
}

void GraphicsWindow::resize(UINT width, UINT height) {
    if (!swapChain_ || width == 0 || height == 0) return; // 0: minimized
    target_.Reset();
    swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createTarget();
}

void GraphicsWindow::beginFrame(const float rgba[4]) {
    ID3D11RenderTargetView* target = target_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->ClearRenderTargetView(target, rgba);
}

void GraphicsWindow::present() { swapChain_->Present(1, 0); }

void GraphicsWindow::destroy() {
    target_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    if (hwnd_ != nullptr) DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    if (windowClass_.lpszClassName != nullptr) UnregisterClassW(windowClass_.lpszClassName, windowClass_.hInstance);
    windowClass_ = {};
}

} // namespace gui
