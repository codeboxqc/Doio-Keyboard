#include "KeyboardConfig.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <stdexcept>

using json = nlohmann::json;

// ─── Design.json loader ──────────────────────────────────────────────────────

static MatrixPos ParseMatrixPos(const std::string& s) {
    auto comma = s.find(',');
    if (comma == std::string::npos)
        throw std::runtime_error("Invalid matrix pos: " + s);
    MatrixPos p;
    p.row = std::stoi(s.substr(0, comma));
    p.col = std::stoi(s.substr(comma + 1));
    return p;
}

bool LoadDesign(const std::string& path, KeyboardLayout& layout) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try { f >> root; }
    catch (...) { return false; }

    layout.name = root.value("name", "Unknown");
    layout.rows = root.at("matrix").at("rows").get<int>();
    layout.cols = root.at("matrix").at("cols").get<int>();
    layout.keys.clear();

    const auto& keymap = root.at("layouts").at("keymap");

    float curY = 0.f;
    for (const auto& row : keymap) {
        float curX = 0.f;
        float nextW = 1.f;
        float nextH = 1.f;
        float xPending = 0.f;

        for (const auto& item : row) {
            if (item.is_string()) {
                PhysicalKey key;
                key.matrix = ParseMatrixPos(item.get<std::string>());
                key.x = curX + xPending;
                key.y = curY;
                key.w = nextW;
                key.h = nextH;
                layout.keys.push_back(key);
                curX = key.x + key.w;
                xPending = 0.f;
                nextW = 1.f;
                nextH = 1.f;
            }
            else if (item.is_object()) {
                if (item.contains("x")) xPending += item["x"].get<float>();
                if (item.contains("y")) curY += item["y"].get<float>();
                if (item.contains("w")) nextW = item["w"].get<float>();
                if (item.contains("h")) nextH = item["h"].get<float>();
            }
        }
        curY += 1.f;
    }

    layout.totalW = 0.f; layout.totalH = 0.f;
    for (const auto& k : layout.keys) {
        layout.totalW = std::max(layout.totalW, k.x + k.w);
        layout.totalH = std::max(layout.totalH, k.y + k.h);
    }
    return !layout.keys.empty();
}

// ─── me.json loader ──────────────────────────────────────────────────────────
//
// Supports two formats:
//   NEW (doio-me-v1): layer has rows*cols entries, indexed row*cols+col
//   OLD (test.json):  layer has phys_count entries in layout-definition order
//
// Old files are auto-converted to the new format in memory (not on disk).

static void ConvertOldLayer(
    const std::vector<std::string>& old_layer,
    std::vector<std::string>& new_layer,
    const KeyboardLayout& layout)
{
    int flat = layout.FlatSize();
    new_layer.assign(flat, "KC_NO");
    int phys = (int)layout.keys.size();
    for (int ki = 0; ki < phys && ki < (int)old_layer.size(); ++ki) {
        const auto& m = layout.keys[ki].matrix;
        int fi = layout.FlatIndex(m);
        if (fi >= 0 && fi < flat)
            new_layer[fi] = old_layer[ki];
    }
}

bool LoadConfig(const std::string& path,
    const KeyboardLayout& layout,
    KeyboardConfig& config)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try { f >> root; }
    catch (...) { return false; }

    config.name = root.value("name", "Unknown");
    config.vendorProductId = root.value("vendorProductId", 0u);

    // ── Macros ────────────────────────────────────────────────────────────────
    config.macros.clear();
    if (root.contains("macros"))
        for (const auto& m : root["macros"])
            config.macros.push_back(m.get<std::string>());

    // ── Layers ────────────────────────────────────────────────────────────────
    config.layers.clear();
    if (root.contains("layers")) {
        int expected_flat = layout.FlatSize();
        int phys_count = (int)layout.keys.size();
        bool first = true;
        bool is_old_fmt = false;

        for (const auto& layer_json : root["layers"]) {
            std::vector<std::string> raw;
            raw.reserve(layer_json.size());
            for (const auto& kc : layer_json)
                raw.push_back(kc.get<std::string>());

            if (first) {
                // Detect format from first layer's size
                is_old_fmt = ((int)raw.size() == phys_count &&
                    (int)raw.size() != expected_flat);
                first = false;
            }

            if (is_old_fmt) {
                // Convert layout-order -> flat row-major
                std::vector<std::string> flat_layer;
                ConvertOldLayer(raw, flat_layer, layout);
                config.layers.push_back(std::move(flat_layer));
            }
            else {
                // Pad/trim to exact flat size
                raw.resize(expected_flat, "KC_NO");
                config.layers.push_back(std::move(raw));
            }
        }
    }

    // ── LED Schemes ───────────────────────────────────────────────────────────
    config.ledSchemes.clear();
    if (root.contains("ledSchemes")) {
        for (const auto& s : root["ledSchemes"]) {
            LedScheme ls;
            ls.name = s.value("name", "Custom");
            ls.type = (LedSchemeType)s.value("type", 0);
            if (s.contains("primary")) {
                ls.primary.r = s["primary"].value("r", 1.f);
                ls.primary.g = s["primary"].value("g", 1.f);
                ls.primary.b = s["primary"].value("b", 1.f);
            }
            if (s.contains("secondary")) {
                ls.secondary.r = s["secondary"].value("r", 1.f);
                ls.secondary.g = s["secondary"].value("g", 1.f);
                ls.secondary.b = s["secondary"].value("b", 1.f);
            }
            ls.speed = s.value("speed", 1.f);
            ls.brightness = s.value("brightness", 1.f);
            config.ledSchemes.push_back(ls);
        }
    }

    return config.IsValid();
}

// ─── me.json saver ───────────────────────────────────────────────────────────

bool SaveConfig(const std::string& path, const KeyboardConfig& config) {
    json root;
    root["_format"] = "doio-me-v1";
    root["name"] = config.name;
    root["vendorProductId"] = config.vendorProductId;

    root["macros"] = json::array();
    for (const auto& m : config.macros)
        root["macros"].push_back(m);

    // Layers — always save in flat row-major format
    root["layers"] = json::array();
    for (const auto& layer : config.layers) {
        json arr = json::array();
        for (const auto& kc : layer) arr.push_back(kc);
        root["layers"].push_back(arr);
    }

    root["ledSchemes"] = json::array();
    for (const auto& ls : config.ledSchemes) {
        json s;
        s["name"] = ls.name;
        s["type"] = (int)ls.type;
        s["primary"] = { {"r", ls.primary.r},   {"g", ls.primary.g},   {"b", ls.primary.b} };
        s["secondary"] = { {"r", ls.secondary.r}, {"g", ls.secondary.g}, {"b", ls.secondary.b} };
        s["speed"] = ls.speed;
        s["brightness"] = ls.brightness;
        root["ledSchemes"].push_back(s);
    }

    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << root.dump(2);
    return f.good();
}

// ─── Factory ─────────────────────────────────────────────────────────────────

static const float kLayerColors[9][3] = {
    {0.25f, 0.55f, 0.95f}, {0.30f, 0.85f, 0.50f}, {0.95f, 0.65f, 0.20f},
    {0.90f, 0.30f, 0.55f}, {0.65f, 0.40f, 0.95f}, {0.20f, 0.85f, 0.80f},
    {0.95f, 0.90f, 0.20f}, {0.80f, 0.40f, 0.25f}, {0.55f, 0.85f, 0.30f},
};

KeyboardConfig NewConfig(const KeyboardLayout& layout, int layerCount) {
    KeyboardConfig cfg;
    cfg.name = layout.name;
    int flat = layout.FlatSize();

    for (int li = 0; li < layerCount; ++li) {
        // KC_TRNS for physical keys, KC_NO for unused slots
        std::vector<std::string> layer(flat, "KC_NO");
        for (const auto& key : layout.keys)
            layer[layout.FlatIndex(key.matrix)] = "KC_TRNS";
        cfg.layers.push_back(std::move(layer));

        LedScheme ls;
        ls.name = "Layer " + std::to_string(li);
        ls.type = LedSchemeType::Solid;
        const float* c = kLayerColors[li % 9];
        ls.primary = { c[0], c[1], c[2] };
        cfg.ledSchemes.push_back(ls);
    }

    cfg.macros.assign(16, "");
    return cfg;
}


// ─── 2-arg legacy LoadConfig overload ────────────────────────────────────────
// No layout available — no format conversion possible.
// Layers are loaded as-is; caller must ensure size is correct (rows*cols).

bool LoadConfig(const std::string& path, KeyboardConfig& config) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try { f >> root; }
    catch (...) { return false; }

    config.name = root.value("name", "Unknown");
    config.vendorProductId = root.value("vendorProductId", 0u);

    config.macros.clear();
    if (root.contains("macros"))
        for (const auto& m : root["macros"])
            config.macros.push_back(m.get<std::string>());

    config.layers.clear();
    if (root.contains("layers")) {
        for (const auto& layer_json : root["layers"]) {
            std::vector<std::string> row;
            row.reserve(layer_json.size());
            for (const auto& kc : layer_json)
                row.push_back(kc.get<std::string>());
            config.layers.push_back(std::move(row));
        }
    }

    config.ledSchemes.clear();
    if (root.contains("ledSchemes")) {
        for (const auto& s : root["ledSchemes"]) {
            LedScheme ls;
            ls.name = s.value("name", "Custom");
            ls.type = (LedSchemeType)s.value("type", 0);
            if (s.contains("primary")) {
                ls.primary.r = s["primary"].value("r", 1.f);
                ls.primary.g = s["primary"].value("g", 1.f);
                ls.primary.b = s["primary"].value("b", 1.f);
            }
            if (s.contains("secondary")) {
                ls.secondary.r = s["secondary"].value("r", 1.f);
                ls.secondary.g = s["secondary"].value("g", 1.f);
                ls.secondary.b = s["secondary"].value("b", 1.f);
            }
            ls.speed = s.value("speed", 1.f);
            ls.brightness = s.value("brightness", 1.f);
            config.ledSchemes.push_back(ls);
        }
    }

    return config.IsValid();
}
