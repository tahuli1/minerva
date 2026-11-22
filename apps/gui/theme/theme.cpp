#include "theme/theme.h"

#include <filesystem>

namespace gui {

namespace {

ImVec4 vec(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

// A Windows font if it is there, else ImGui's built-in one, so the app still runs.
ImFont* load(const char* file, float size) {
    const std::filesystem::path path = std::filesystem::path("C:/Windows/Fonts") / file;
    ImGuiIO& io = ImGui::GetIO();
    if (std::filesystem::exists(path)) return io.Fonts->AddFontFromFileTTF(path.string().c_str(), size);
    ImFontConfig config;
    config.SizePixels = size;
    return io.Fonts->AddFontDefault(&config);
}

} // namespace

Fonts applyTheme() {
    Fonts fonts;
    fonts.body = load("segoeui.ttf", 17.0f); // the first font loaded is ImGui's default
    fonts.title = load("segoeuib.ttf", 30.0f);
    fonts.number = load("consola.ttf", 17.0f);
    fonts.bigNumber = load("consolab.ttf", 26.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(24, 20);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 12);
    style.WindowRounding = 0;
    style.ChildRounding = 16;
    style.FrameRounding = 10;
    style.GrabRounding = 10;
    style.GrabMinSize = 18;
    style.WindowBorderSize = 0;
    style.ChildBorderSize = 0;

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = vec(color::kBackground);
    c[ImGuiCol_ChildBg] = vec(color::kPanel);
    c[ImGuiCol_Text] = vec(color::kText);
    c[ImGuiCol_TextDisabled] = vec(color::kMuted);
    c[ImGuiCol_FrameBg] = vec(color::kLine);
    c[ImGuiCol_FrameBgHovered] = vec(color::kLine);
    c[ImGuiCol_FrameBgActive] = vec(color::kLine);
    c[ImGuiCol_SliderGrab] = vec(color::kPurple);
    c[ImGuiCol_SliderGrabActive] = vec(color::kPurpleLight);
    c[ImGuiCol_Button] = vec(color::kLine);
    c[ImGuiCol_ButtonHovered] = vec(color::kDim);
    c[ImGuiCol_ButtonActive] = vec(color::kPurpleBack);
    return fonts;
}

} // namespace gui
