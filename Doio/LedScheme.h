#pragma once
#include <string>
#include <vector>
#include <array>

// ─── LED Colour Scheme ────────────────────────────────────────────────────────

enum class LedSchemeType {
    Solid,          // Single colour, all keys the same
    Breathing,      // One colour that pulses (runtime hint only)
    Rainbow,        // Hue sweep across keys left→right
    RainbowWave,    // Hue sweep over time (runtime hint)
    ReactiveFade,   // Keys light up on press then fade (runtime hint)
    Custom,         // User-defined per-key colour (not yet per-key, single override)
    COUNT
};

inline const char* LedSchemeTypeName(LedSchemeType t) {
    switch (t) {
    case LedSchemeType::Solid:         return "Solid";
    case LedSchemeType::Breathing:     return "Breathing";
    case LedSchemeType::Rainbow:       return "Rainbow";
    case LedSchemeType::RainbowWave:   return "Rainbow Wave";
    case LedSchemeType::ReactiveFade:  return "Reactive Fade";
    case LedSchemeType::Custom:        return "Custom";
    default:                           return "Unknown";
    }
}

// RGB 0-1 float
struct RgbF {
    float r = 1.f, g = 1.f, b = 1.f;
    // Returns an ImU32 suitable for ImGui (ABGR packed)
    unsigned int ToImU32(float alpha = 1.f) const {
        auto b8 = [](float v) { return (unsigned char)(v * 255.f + 0.5f); };
        return (unsigned int)(0xFF000000u
            | ((unsigned int)b8(b) << 16)
            | ((unsigned int)b8(g) << 8)
            | (unsigned int)b8(r));
    }
};

struct LedScheme {
    std::string   name;
    LedSchemeType type     = LedSchemeType::Solid;
    RgbF          primary  = {0.25f, 0.55f, 0.95f}; // main / breathing colour
    RgbF          secondary= {0.95f, 0.30f, 0.55f}; // used for gradient end
    float         speed    = 1.f;                    // animation speed hint (0.1–5)
    float         brightness = 1.f;                  // 0–1
};

// ─── Predefined palettes ──────────────────────────────────────────────────────

inline std::vector<LedScheme> BuildPredefinedSchemes() {
    return {
        {"Ocean Blue",      LedSchemeType::Solid,        {0.10f, 0.45f, 0.90f}},
        {"Mint Green",      LedSchemeType::Solid,        {0.15f, 0.85f, 0.55f}},
        {"Sunset Orange",   LedSchemeType::Solid,        {0.95f, 0.55f, 0.10f}},
        {"Rose Pink",       LedSchemeType::Solid,        {0.95f, 0.30f, 0.55f}},
        {"Lavender",        LedSchemeType::Solid,        {0.65f, 0.40f, 0.95f}},
        {"Teal",            LedSchemeType::Solid,        {0.15f, 0.85f, 0.80f}},
        {"Gold",            LedSchemeType::Solid,        {0.95f, 0.80f, 0.15f}},
        {"White",           LedSchemeType::Solid,        {0.95f, 0.95f, 0.95f}},
        {"Red Alert",       LedSchemeType::Solid,        {0.90f, 0.10f, 0.10f}},
        {"Aqua Breathe",    LedSchemeType::Breathing,    {0.10f, 0.80f, 0.90f}},
        {"Purple Breathe",  LedSchemeType::Breathing,    {0.60f, 0.20f, 0.90f}},
        {"Rainbow",         LedSchemeType::Rainbow,      {1.f,   0.f,   0.f  }, {0.f, 0.f, 1.f}},
        {"Rainbow Wave",    LedSchemeType::RainbowWave,  {1.f,   0.f,   0.f  }, {0.f, 0.f, 1.f}},
        {"Reactive Fade",   LedSchemeType::ReactiveFade, {0.25f, 0.55f, 0.95f}},
        {"Off",             LedSchemeType::Solid,        {0.f,   0.f,   0.f  }},
    };
}
