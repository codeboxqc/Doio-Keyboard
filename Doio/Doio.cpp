// DOIO Keyboard Editor
// Built with: Dear ImGui + DirectX 11 + Win32
// VS2022 / C++17

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "KeyboardEditor.h"
#include "doio.h"

// Link DirectX
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "comdlg32.lib")

// ─── DX11 globals ─────────────────────────────────────────────────────────────

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0;
static UINT                     g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res != S_OK) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release();          g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release();   g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release();          g_pd3dDevice = nullptr; }
}

// Forward-declare ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ─── Custom ImGui style ───────────────────────────────────────────────────────

static void ApplyDoioStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.FrameRounding = 4.f;
    s.GrabRounding = 4.f;
    s.ScrollbarRounding = 4.f;
    s.TabRounding = 4.f;
    s.FramePadding = ImVec2(6, 4);
    s.ItemSpacing = ImVec2(6, 4);
    s.WindowPadding = ImVec2(10, 10);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.18f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.38f, 0.60f);
    c[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.45f, 0.80f, 0.60f);
    c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.28f, 0.58f, 1.00f);
    c[ImGuiCol_Tab] = ImVec4(0.15f, 0.20f, 0.30f, 1.00f);
    c[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.00f);
    c[ImGuiCol_TabActive] = ImVec4(0.20f, 0.38f, 0.70f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.32f, 0.55f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.46f, 0.78f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.28f, 0.60f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.32f, 0.55f, 0.60f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.46f, 0.78f, 0.80f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.28f, 0.60f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.40f, 0.60f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.50f, 0.85f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.58f, 1.00f);
    c[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.50f, 0.85f, 0.60f);
    c[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
}

// ─── Entry point ─────────────────────────────────────────────────────────────

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // Allow attaching a console for debug output
    // AllocConsole();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.lpszClassName = L"DOIOEditor";
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowExW(
        0, L"DOIOEditor", L"DOIO Keyboard Editor  v1.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 900,
        nullptr, nullptr, hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "doio_editor_layout.ini";

    ApplyDoioStyle();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load a font slightly larger than default with extended glyph ranges for keyboard symbols
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2100, 0x21FF, // Arrows
        0x2300, 0x23FF, // Miscellaneous Technical
        0x2500, 0x25FF, // Geometric Shapes
        0,
    };

   // io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, nullptr, ranges);
   // io.FontDefault = io.Fonts->Fonts.back();

    if (!LoadFontsWithFallback()) {
        // Fallback to basic font loading
        static const ImWchar basicRanges[] = {
            0x0020, 0x00FF, // Basic Latin
            0x2500, 0x25FF, // Geometric Shapes
            0,
        };
        io.Fonts->AddFontDefault();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, nullptr, basicRanges);
        io.FontDefault = io.Fonts->Fonts.back();
    }


    KeyboardEditor editor;

    const ImVec4 clearColor(0.10f, 0.10f, 0.12f, 1.00f);
    bool done = false;

    while (!done) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded &&
            g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Full-window dockspace
        {
            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            ImGui::SetNextWindowViewport(vp->ID);
            ImGuiWindowFlags dsFlags =
                ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_MenuBar;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("##DockSpace", nullptr, dsFlags);
            ImGui::PopStyleVar();

            // ── Main menu bar ─────────────────────────────────────────────────
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open Design (design.json)", "")) editor.OpenDesign();
                    if (ImGui::MenuItem("Open Config (me.json)", "Ctrl+O")) editor.OpenConfig();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save Config", "Ctrl+S", false, editor.CanUndo() || true)) editor.SaveConfig();
                    if (ImGui::MenuItem("Save Config As…", ""))       editor.SaveConfigAs();
                    if (ImGui::MenuItem("Save to Keyboard Memory", "Ctrl+Shift+S", false, editor.IsConfigLoaded())) editor.SaveToKeyboard();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit")) done = true;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor.CanUndo())) editor.Undo();
                    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor.CanRedo())) editor.Redo();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Reset Current Layer to Transparent")) editor.ResetCurrentLayer();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    ImGui::Text("DOIO Keyboard Editor v1.0");
                    ImGui::Separator();
                    ImGui::BulletText("Open design.json first (layout)");
                    ImGui::BulletText("Open me.json (keymaps)");
                    ImGui::BulletText("Click a key, pick a keycode");
                    ImGui::BulletText("Ctrl+S to save");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
                ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::End();
        }

        // Render all editor panels
        editor.Render();

        // ── Render frame ─────────────────────────────────────────────────────
        ImGui::Render();
        const float cc[4] = {
            clearColor.x * clearColor.w, clearColor.y * clearColor.w,
            clearColor.z * clearColor.w, clearColor.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, hInstance);

    return 0;
}
