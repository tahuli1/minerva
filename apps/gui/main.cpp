#include "window/graphics_window.h"

#include "engine/engine.h"
#include "keyboard/controls.h"
#include "keyboard/note_keys.h"
#include "panels/panels.h"
#include "resource.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"
#include "theme/theme.h"
#include "window/texture.h"

#include <chrono>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <implot.h>

// ImGui's Win32 backend leaves this declaration to the program.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {

// Everything the window's messages act on. Main thread only: Windows delivers a
// window's messages on the thread that created it.
struct App {
    minerva::Engine* engine = nullptr;
    gui::GraphicsWindow* window = nullptr;
    app::Settings settings;
    app::NoteKeys noteKeys;
};
App* g_app = nullptr;

// Audio thread. `user` is the engine.
void runEngine(float* out, int n, void* user) { static_cast<minerva::Engine*>(user)->process(out, n); }

void keyDown(App& a, int key) {
    if (auto note = a.noteKeys.press(key, a.settings.octave)) {
        a.engine->noteOn(*note, a.settings.velocity);
        return;
    }
    const minerva::Waveform waveform = a.settings.waveform;
    if (!app::handleKey(key, a.settings)) return;
    if (a.settings.waveform != waveform) a.engine->setWaveform(a.settings.waveform);
    app::apply(a.settings, *a.engine);
}

void keyUp(App& a, int key) {
    if (auto note = a.noteKeys.release(key)) a.engine->noteOff(*note);
}

LRESULT WINAPI wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
    if (g_app != nullptr) {
        switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: // F10 and Alt+key come as system keys
            keyDown(*g_app, static_cast<int>(wParam));
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            keyUp(*g_app, static_cast<int>(wParam));
            return 0;
        case WM_KILLFOCUS:
            // The key-ups now go to another window; stop every note so none is stuck.
            for (int note : g_app->noteKeys.releaseAll()) g_app->engine->noteOff(note);
            return 0;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) g_app->window->resize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// What the window shows besides the settings: drawn from, never written to the
// audio side.
struct View {
    gui::Fonts fonts;
    gui::Texture logo;
    minerva::TimingSummary timing; // the last full second
};

void drawUi(App& a, const View& view, const minerva::RtAudioBackend& backend) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Minerva", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    gui::drawHeader(view.fonts, view.logo, backend.info(), backend.audioThreadPriority());

    // Three panels in a row, as in the mockup.
    const float gap = 16;
    const float width = (ImGui::GetContentRegionAvail().x - 2 * gap) / 3;
    const ImVec2 size(width, 236);
    gui::Changed changed = gui::drawSoundPanel(a.settings, size);
    ImGui::SameLine(0, gap);
    const gui::Changed effects = gui::drawEffectsPanel(a.settings, size);
    ImGui::SameLine(0, gap);
    gui::drawEnginePanel(view.fonts, view.timing, size);

    // A change made with the mouse reaches the engine the way a key press does.
    if (changed.waveform) a.engine->setWaveform(a.settings.waveform);
    if (changed.settings || effects.settings) app::apply(a.settings, *a.engine);
    ImGui::End();
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // COM, for WIC (the logo). Apartment-threaded: this is the window's thread.
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    minerva::Engine engine;
    App a;
    a.engine = &engine;
    app::apply(a.settings, engine); // before prepare(), so the smoothers start at these values
    engine.setWaveform(a.settings.waveform);
    engine.distortion().setLevel(0.3f); // the overdrive peaks near full scale; keep headphones safe

    // Declared after the engine so it is destroyed first: the stream stops before
    // the engine it calls into goes away.
    minerva::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, runEngine, &engine)) {
        MessageBoxA(nullptr, backend.lastError().c_str(), "Minerva: cannot open audio", MB_ICONERROR);
        return 1;
    }
    engine.prepare(static_cast<int>(backend.info().sampleRate), static_cast<int>(backend.info().bufferFrames));

    gui::GraphicsWindow window;
    a.window = &window;
    g_app = &a;
    if (!window.create(L"Minerva", 1280, 880, wndProc)) {
        MessageBoxA(nullptr, "Direct3D 11 is not available", "Minerva", MB_ICONERROR);
        return 1;
    }

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // no imgui.ini next to the exe
    ImGui_ImplWin32_Init(window.hwnd());
    ImGui_ImplDX11_Init(window.device(), window.context());

    View view;
    view.fonts = gui::applyTheme();
    // WIC needs COM, initialized above.
    view.logo = gui::loadPngResource(window.device(), IDR_LOGO, gui::kLogoSize, gui::kLogoSize);
    minerva::TimingStats stats(backend.ticksPerSecond(), static_cast<int>(backend.info().sampleRate));
    auto nextSummary = std::chrono::steady_clock::now() + std::chrono::seconds(1);

    if (!backend.start()) {
        MessageBoxA(nullptr, backend.lastError().c_str(), "Minerva: cannot start audio", MB_ICONERROR);
        return 1;
    }

    // Main thread, not RT: one frame per screen refresh. Drawing may take a few
    // milliseconds, or stall while the window is dragged; the audio thread never
    // waits for it.
    const float background[4] = {0.067f, 0.075f, 0.09f, 1.0f};
    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        // Drained every frame, so the queue never fills; summarized once a second.
        minerva::CallbackTiming timing;
        while (backend.popTiming(timing)) stats.add(timing);
        if (std::chrono::steady_clock::now() >= nextSummary) {
            view.timing = stats.summary();
            stats.reset();
            nextSummary += std::chrono::seconds(1);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        drawUi(a, view, backend);
        ImGui::Render();
        window.beginFrame(background);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        window.present();
    }

    backend.stop();
    g_app = nullptr;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    window.destroy();
    CoUninitialize();
    return 0;
}
