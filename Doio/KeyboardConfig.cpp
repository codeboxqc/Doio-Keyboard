#include "KeyboardConfig.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

using json = nlohmann::json;

// ─── Helpers ────────────────────────────────────────────────────────────────

static MatrixPos ParseMatrixPos(const std::string& s) {
    // Expected format: "row,col"
    auto comma = s.find(',');
    if (comma == std::string::npos)
        throw std::runtime_error("Invalid matrix pos: " + s);
    MatrixPos p;
    p.row = std::stoi(s.substr(0, comma));
    p.col = std::stoi(s.substr(comma + 1));
    return p;
}

// ─── Load design.json ───────────────────────────────────────────────────────
//
// VIA / KLE-compatible format:
//   layouts.keymap = array of rows
//   each row = array of (string "r,c" | object {x,y,w,h,...})
//   Object properties are applied to the NEXT key encountered.
//   "x" / "y" are *relative* offsets (additive), "w" / "h" replace default 1.

bool LoadDesign(const std::string& path, KeyboardLayout& layout) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try { f >> root; } catch (...) { return false; }

    layout.name = root.value("name", "Unknown");
    layout.rows = root.at("matrix").at("rows").get<int>();
    layout.cols = root.at("matrix").at("cols").get<int>();
    layout.keys.clear();

    const auto& keymap = root.at("layouts").at("keymap");

    float curY = 0.f;

    for (const auto& row : keymap) {
        float curX     = 0.f;
        float nextW    = 1.f;
        float nextH    = 1.f;
        float xPending = 0.f; // accumulated x offset before next key

        for (const auto& item : row) {
            if (item.is_string()) {
                // This is a key
                PhysicalKey key;
                key.matrix = ParseMatrixPos(item.get<std::string>());
                key.x = curX + xPending;
                key.y = curY;
                key.w = nextW;
                key.h = nextH;
                layout.keys.push_back(key);

                curX   = key.x + key.w;
                xPending = 0.f;
                nextW = 1.f;
                nextH = 1.f;
            } else if (item.is_object()) {
                // Property block — accumulate before next key
                if (item.contains("x")) xPending += item["x"].get<float>();
                if (item.contains("y")) curY      += item["y"].get<float>(); // rare
                if (item.contains("w")) nextW      = item["w"].get<float>();
                if (item.contains("h")) nextH      = item["h"].get<float>();
            }
        }

        curY += 1.f; // advance to next row
    }

    // Compute bounding box
    layout.totalW = 0.f;
    layout.totalH = 0.f;
    for (const auto& k : layout.keys) {
        layout.totalW = std::max(layout.totalW, k.x + k.w);
        layout.totalH = std::max(layout.totalH, k.y + k.h);
    }

    return !layout.keys.empty();
}

// ─── Load me.json ───────────────────────────────────────────────────────────

bool LoadConfig(const std::string& path, KeyboardConfig& config) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try { f >> root; } catch (...) { return false; }

    config.name            = root.value("name", "Unknown");
    config.vendorProductId = root.value("vendorProductId", 0u);

    config.macros.clear();
    if (root.contains("macros")) {
        for (const auto& m : root["macros"])
            config.macros.push_back(m.get<std::string>());
    }

    config.layers.clear();
    if (root.contains("layers")) {
        for (const auto& layer : root["layers"]) {
            std::vector<std::string> keycodes;
            keycodes.reserve(layer.size());
            for (const auto& kc : layer)
                keycodes.push_back(kc.get<std::string>());
            config.layers.push_back(std::move(keycodes));
        }
    }

    return config.IsValid();
}

// ─── Save me.json ────────────────────────────────────────────────────────────

bool SaveConfig(const std::string& path, const KeyboardConfig& config) {
    json root;
    root["name"]            = config.name;
    root["vendorProductId"] = config.vendorProductId;

    root["macros"] = json::array();
    for (const auto& m : config.macros)
        root["macros"].push_back(m);

    root["layers"] = json::array();
    for (const auto& layer : config.layers) {
        json arr = json::array();
        for (const auto& kc : layer) arr.push_back(kc);
        root["layers"].push_back(arr);
    }

    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << root.dump(2);
    return f.good();
}
