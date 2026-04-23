#pragma once
#include <string>
#include <vector>
#include <optional>
#include "LedScheme.h"

// ─── Physical layout (from design.json) ────────────────────────────────────

struct UndoRecord {
    int         layer = 0;
    int         flatIdx = 0;
    std::string oldCode;
    std::string newCode;
};



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
    std::string              name;
    int                      rows = 0;
    int                      cols = 0;
    std::vector<PhysicalKey> keys;
    float                    totalW = 0.f;
    float                    totalH = 0.f;

    // Total flat slots — matches Python Layout.flat_size
    int FlatSize() const { return rows * cols; }

    // Row-major flat index — matches Python Layout.flat_idx() and map.py
    int FlatIndex(int r, int c) const { return r * cols + c; }
    int FlatIndex(const MatrixPos& m) const { return m.row * cols + m.col; }

    // Alias kept for backward compat with any call sites not yet updated
    int ConfigIndex(int r, int c)        const { return FlatIndex(r, c); }
    int ConfigIndex(const MatrixPos& m)  const { return FlatIndex(m); }

    // Physical key lookup
    int PhysicalIndex(int r, int c) const {
        for (int i = 0; i < (int)keys.size(); ++i)
            if (keys[i].matrix.row == r && keys[i].matrix.col == c) return i;
        return -1;
    }
    bool IsPhysical(int r, int c) const { return PhysicalIndex(r, c) >= 0; }
};


// ─── Keymap config (from me.json) ──────────────────────────────────────────

struct KeyboardConfig {
    std::string                           name;
    uint32_t                              vendorProductId = 0;
    std::vector<std::string>              macros;
    std::vector<std::vector<std::string>> layers;     // [layerIdx][flatIdx]
    std::vector<LedScheme>                ledSchemes;

    bool IsValid()    const { return !layers.empty(); }
    int  LayerCount() const { return (int)layers.size(); }

    // Number of entries per layer (rows * cols)
    int FlatSize() const {
        return layers.empty() ? 0 : (int)layers[0].size();
    }

    // KeyCount() kept as alias — used in many existing call sites
    int KeyCount() const { return FlatSize(); }

    const std::string& Keycode(int layer, int flatIdx) const {
        static const std::string empty = "KC_NO";
        if (layer < 0 || layer >= LayerCount())   return empty;
        if (flatIdx < 0 || flatIdx >= FlatSize()) return empty;
        return layers[layer][flatIdx];
    }
    std::string& Keycode(int layer, int flatIdx) {
        return layers[layer][flatIdx];
    }
};

 
// ─── I/O ────────────────────────────────────────────────────────────────────
bool LoadDesign(const std::string& path, KeyboardLayout& layout);

// 3-arg version: uses layout to auto-convert old layout-order files
// Use this whenever the layout is available.
bool LoadConfig(const std::string& path, const KeyboardLayout& layout,
    KeyboardConfig& config);

// 2-arg legacy overload: no format conversion.
// Safe to use after --export or when layer size is already rows*cols.
bool LoadConfig(const std::string& path, KeyboardConfig& config);

bool SaveConfig(const std::string& path, const KeyboardConfig& config);

// Create a blank config: KC_TRNS for physical keys, KC_NO for unused slots
KeyboardConfig NewConfig(const KeyboardLayout& layout, int layerCount = 9);

