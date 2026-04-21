#pragma once
#include <string>
#include <vector>
#include <optional>

// ─── Physical layout (from design.json) ────────────────────────────────────

struct MatrixPos {
    int row = 0;
    int col = 0;
};

struct PhysicalKey {
    MatrixPos matrix;
    float     x = 0.f, y = 0.f;  // position in key units (origin = top-left)
    float     w = 1.f, h = 1.f;  // size in key units
};

struct KeyboardLayout {
    std::string            name;
    int                    rows = 0;
    int                    cols = 0;
    std::vector<PhysicalKey> keys;   // ordered list of physical keys
    float                  totalW = 0.f; // bounding box in key units
    float                  totalH = 0.f;

    // Map matrix pos → index into keys[]
    int IndexForMatrix(int r, int c) const {
        for (int i = 0; i < (int)keys.size(); ++i)
            if (keys[i].matrix.row == r && keys[i].matrix.col == c) return i;
        return -1;
    }

    // Flat index in me.json array for a given matrix pos
    int ConfigIndex(int r, int c) const { return r * cols + c; }
    int ConfigIndex(const MatrixPos& m) const { return m.row * cols + m.col; }
};

// ─── Keymap config (from me.json) ──────────────────────────────────────────

struct KeyboardConfig {
    std::string                           name;
    uint32_t                              vendorProductId = 0;
    std::vector<std::string>              macros;
    std::vector<std::vector<std::string>> layers; // [layerIdx][flatIdx]

    bool IsValid() const { return !layers.empty(); }
    int  LayerCount() const { return (int)layers.size(); }
    int  KeyCount()   const { return layers.empty() ? 0 : (int)layers[0].size(); }

    const std::string& Keycode(int layer, int flatIdx) const {
        static const std::string empty = "KC_NO";
        if (layer < 0 || layer >= LayerCount()) return empty;
        if (flatIdx < 0 || flatIdx >= (int)layers[layer].size()) return empty;
        return layers[layer][flatIdx];
    }
    std::string& Keycode(int layer, int flatIdx) {
        return layers[layer][flatIdx];
    }
};

// ─── I/O ────────────────────────────────────────────────────────────────────

bool LoadDesign(const std::string& path, KeyboardLayout& layout);
bool LoadConfig (const std::string& path, KeyboardConfig& config);
bool SaveConfig (const std::string& path, const KeyboardConfig& config);
