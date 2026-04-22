// DOIO Keyboard Editor
// Built with: Dear ImGui + DirectX 11 + Win32
// VS2022 / C++17

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"



// Add this function before wWinMain (after ApplyDoioStyle)

 bool LoadFontsWithFallback() {
    ImGuiIO& io = ImGui::GetIO();

    // Clear any existing fonts
    io.Fonts->Clear();

    // Comprehensive Unicode ranges for keyboard editor
    static const ImWchar keyboardRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (A-Z, a-z, 0-9, punctuation)
        0x0100, 0x017F, // Latin Extended-A (accents)
        0x0180, 0x024F, // Latin Extended-B
        0x0370, 0x03FF, // Greek and Coptic
        0x2000, 0x206F, // General Punctuation
        0x2070, 0x209F, // Superscripts and Subscripts
        0x20A0, 0x20CF, // Currency Symbols
        0x2100, 0x214F, // Letterlike Symbols
        0x2150, 0x218F, // Number Forms
        0x2190, 0x21FF, // Arrows (← ↑ → ↓)
        0x2200, 0x22FF, // Mathematical Operators
        0x2300, 0x23FF, // Miscellaneous Technical
        0x2400, 0x243F, // Control Pictures
        0x2440, 0x245F, // Optical Character Recognition
        0x2460, 0x24FF, // Enclosed Alphanumerics
        0x2500, 0x257F, // Box Drawing
        0x2580, 0x259F, // Block Elements
        0x25A0, 0x25FF, // Geometric Shapes (▽ ○ □ ▲ ▼)
        0x2600, 0x26FF, // Miscellaneous Symbols (🔇 🔊 🔆 🔅)
        0x2700, 0x27BF, // Dingbats
        0x27C0, 0x27EF, // Miscellaneous Mathematical Symbols-A
        0x27F0, 0x27FF, // Supplemental Arrows-A
        0x2900, 0x297F, // Supplemental Arrows-B
        0x2980, 0x29FF, // Miscellaneous Mathematical Symbols-B
        0x2A00, 0x2AFF, // Supplemental Mathematical Operators
        0x2B00, 0x2BFF, // Miscellaneous Symbols and Arrows
        0x2E00, 0x2E7F, // Supplemental Punctuation
        0x3000, 0x303F, // CJK Symbols and Punctuation
        0x3040, 0x309F, // Hiragana
        0x30A0, 0x30FF, // Katakana
        0x4E00, 0x9FFF, // CJK Unified Ideographs (common Chinese/Japanese)
        0x1F300, 0x1F5FF, // Miscellaneous Symbols and Pictographs (emojis)
        0x1F600, 0x1F64F, // Emoticons (emojis)
        0x1F680, 0x1F6FF, // Transport and Map Symbols
        0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
        0,
    };

    // Try multiple fonts in order of preference
    const struct {
        const char* path;
        const char* name;
        float size;
    } fontCandidates[] = {
        {"C:\\Windows\\Fonts\\segoeui.ttf", "Segoe UI", 15.0f},        // Best Windows font
        {"C:\\Windows\\Fonts\\consola.ttf", "Consolas", 15.0f},        // Monospace fallback
        {"C:\\Windows\\Fonts\\msgothic.ttc", "MS Gothic", 15.0f},      // Japanese fallback
        {"C:\\Windows\\Fonts\\msyh.ttc", "Microsoft YaHei", 15.0f},    // Chinese fallback
        {"C:\\Windows\\Fonts\\malgun.ttf", "Malgun Gothic", 15.0f},    // Korean fallback
    };

    bool fontLoaded = false;

    // Try to load main font
    for (const auto& candidate : fontCandidates) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            candidate.path,
            candidate.size,
            nullptr,
            keyboardRanges
        );

        if (font) {
            io.FontDefault = font;
            fontLoaded = true;

            // Output debug info (if console is attached)
            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg),
                "Loaded font: %s (%s)\n",
                candidate.name, candidate.path);
            OutputDebugStringA(debugMsg);
            break;
        }
    }

    // If no font loaded, use default ImGui font
    if (!fontLoaded) {
        io.FontDefault = io.Fonts->AddFontDefault();
        OutputDebugStringA("Warning: Using default ImGui font (limited Unicode support)\n");
    }

    // Try to load emoji font as merged fallback (optional)
    const char* emojiPaths[] = {
        "C:\\Windows\\Fonts\\seguiemj.ttf",  // Windows 10/11 emoji font
        "C:\\Windows\\Fonts\\color-emoji.ttf", // Alternative emoji font
    };

    for (const char* emojiPath : emojiPaths) {
        ImFontConfig config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = 15.0f; // Match main font size

        static const ImWchar emojiRanges[] = {
            0x1F300, 0x1F5FF, // Misc Symbols and Pictographs
            0x1F600, 0x1F64F, // Emoticons
            0x1F680, 0x1F6FF, // Transport and Map Symbols
            0x1F900, 0x1F9FF, // Supplemental Symbols
            0,
        };

        ImFont* emojiFont = io.Fonts->AddFontFromFileTTF(
            emojiPath,
            15.0f,
            &config,
            emojiRanges
        );

       
    }

    // Build font atlas
    io.Fonts->Build();

    
   

    return true;
}

 