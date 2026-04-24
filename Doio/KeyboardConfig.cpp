#include "KeyboardConfig.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <regex>
#include <sstream>
#include <cctype>

using json = nlohmann::json;

// Forward declaration - keycode map will be provided by the editor
// For now, just store macros as strings without parsing

// ─── Design.json loader ──────────────────────────────────────────────────────

static MatrixPos ParseMatrixPos(const std::string& s) {
    MatrixPos p;
    p.row = 0;
    p.col = 0;

    // Find first comma - could be after some garbage
    size_t comma = s.find(',');
    if (comma == std::string::npos) {
        // Try to find any numbers separated by something
        std::regex numRegex(R"(\d+,\d+)");
        std::smatch match;
        if (std::regex_search(s, match, numRegex)) {
            std::string cleaned = match.str();
            comma = cleaned.find(',');
            if (comma != std::string::npos) {
                p.row = std::stoi(cleaned.substr(0, comma));
                p.col = std::stoi(cleaned.substr(comma + 1));
                return p;
            }
        }
        throw std::runtime_error("Invalid matrix pos: " + s);
    }

    // Handle newlines and special characters in the string
    std::string clean = s;
    // Remove everything after the first newline
    size_t nl = clean.find_first_of("\n\r");
    if (nl != std::string::npos) {
        clean = clean.substr(0, nl);
    }
    // Remove encoder markers (e0, e1, etc.)
    size_t ePos = clean.find('e');
    if (ePos != std::string::npos && ePos > 0 && std::isdigit(clean[ePos - 1])) {
        clean = clean.substr(0, ePos);
    }

    comma = clean.find(',');
    if (comma == std::string::npos) {
        throw std::runtime_error("Invalid matrix pos after cleaning: " + s);
    }

    try {
        p.row = std::stoi(clean.substr(0, comma));
        p.col = std::stoi(clean.substr(comma + 1));
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Invalid matrix pos numbers: " + s);
    }

    return p;
}

// Simple macro validation - just check for valid { } syntax
bool IsValidMacroSyntax(const std::string& macroStr) {
    if (macroStr.empty()) return true;

    // Check for balanced braces
    int braceCount = 0;
    for (char c : macroStr) {
        if (c == '{') braceCount++;
        if (c == '}') braceCount--;
        if (braceCount < 0) return false;
    }
    return braceCount == 0;
}

// Parse macro for validation only (returns true if format looks correct)
bool ParseMacroForValidation(const std::string& macroStr) {
    if (macroStr.empty()) return true;

    std::regex bracePattern(R"(\{([^}]+)\})");
    auto begin = std::sregex_iterator(macroStr.begin(), macroStr.end(), bracePattern);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string content = (*it)[1].str();
        // Split by comma and trim
        std::stringstream ss(content);
        std::string keycode;
        while (std::getline(ss, keycode, ',')) {
            // Trim whitespace
            keycode.erase(0, keycode.find_first_not_of(" \t\r\n"));
            keycode.erase(keycode.find_last_not_of(" \t\r\n") + 1);
            // Basic validation - should start with KC_ or be a known keycode pattern
            if (!keycode.empty() && keycode.find("KC_") != 0 &&
                keycode.find("DELAY") != 0 && keycode.find("$") != 0) {
                // Not necessarily an error - could be custom
            }
        }
    }
    return true;
}

bool LoadDesign2(const std::string& path, KeyboardLayout& layout) {
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


bool LoadDesign(const std::string& path, KeyboardLayout& layout) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json root;
    try {
        f >> root;
    }
    catch (const json::parse_error& e) {
        fprintf(stderr, "[ERROR] JSON parse error: %s\n", e.what());
        return false;
    }

    layout.name = root.value("name", "Unknown");

    // Safely get matrix dimensions
    if (!root.contains("matrix")) {
        fprintf(stderr, "[ERROR] design.json missing 'matrix' field\n");
        return false;
    }

    layout.rows = root["matrix"].value("rows", 0);
    layout.cols = root["matrix"].value("cols", 0);

    if (layout.rows == 0 || layout.cols == 0) {
        fprintf(stderr, "[ERROR] Invalid matrix dimensions: %dx%d\n", layout.rows, layout.cols);
        return false;
    }

    layout.keys.clear();

    // Check if layouts.keymap exists
    if (!root.contains("layouts") || !root["layouts"].contains("keymap")) {
        fprintf(stderr, "[ERROR] design.json missing 'layouts.keymap'\n");
        return false;
    }

    const auto& keymap = root["layouts"]["keymap"];

    if (!keymap.is_array() || keymap.empty()) {
        fprintf(stderr, "[ERROR] layouts.keymap is not an array or is empty\n");
        return false;
    }

    float curY = 0.f;
    int parsedKeyCount = 0;

    for (const auto& row : keymap) {
        if (!row.is_array()) {
            fprintf(stderr, "[WARNING] Skipping non-array row in keymap\n");
            continue;
        }

        float curX = 0.f;
        float nextW = 1.f;
        float nextH = 1.f;
        float xPending = 0.f;

        // Track current position in the row
        int colIndex = 0;
        int maxCols = 20; // Reasonable upper bound

        for (const auto& item : row) {
            if (item.is_string()) {
                // Parse matrix position (may have extra data like "0,4\n...\ne0")
                std::string matrixStr = item.get<std::string>();

                // Extract just the "row,col" part before any newline or special chars
                size_t nlPos = matrixStr.find_first_of("\n\r");
                if (nlPos != std::string::npos) {
                    matrixStr = matrixStr.substr(0, nlPos);
                }

                // Also strip any trailing encoder markers (e0, e1, etc.)
                size_t ePos = matrixStr.find('e');
                if (ePos != std::string::npos && ePos > 0 && std::isdigit(matrixStr[ePos - 1])) {
                    // Keep only up to the 'e'
                    matrixStr = matrixStr.substr(0, ePos);
                }

                PhysicalKey key;
                try {
                    key.matrix = ParseMatrixPos(matrixStr);

                    // Validate matrix indices are within range
                    if (key.matrix.row < 0 || key.matrix.row >= layout.rows ||
                        key.matrix.col < 0 || key.matrix.col >= layout.cols) {
                        fprintf(stderr, "[WARNING] Matrix position %d,%d out of range (max %d,%d)\n",
                            key.matrix.row, key.matrix.col, layout.rows - 1, layout.cols - 1);
                        // Still add it but continue
                    }
                }
                catch (const std::exception& e) {
                    fprintf(stderr, "[WARNING] Failed to parse matrix position: '%s'\n", matrixStr.c_str());
                    // Skip this key
                    colIndex++;
                    curX += nextW;
                    xPending = 0.f;
                    nextW = 1.f;
                    nextH = 1.f;
                    continue;
                }

                key.x = curX + xPending;
                key.y = curY;
                key.w = nextW;
                key.h = nextH;
                layout.keys.push_back(key);
                parsedKeyCount++;

                curX = key.x + key.w;
                xPending = 0.f;
                nextW = 1.f;
                nextH = 1.f;
                colIndex++;
            }
            else if (item.is_object()) {
                // Handle positioning objects
                if (item.contains("x")) {
                    if (item["x"].is_number()) {
                        xPending += item["x"].get<float>();
                    }
                    else if (item["x"].is_string()) {
                        xPending += std::stof(item["x"].get<std::string>());
                    }
                }
                if (item.contains("y")) {
                    if (item["y"].is_number()) {
                        curY += item["y"].get<float>();
                    }
                    else if (item["y"].is_string()) {
                        curY += std::stof(item["y"].get<std::string>());
                    }
                }
                if (item.contains("w")) {
                    if (item["w"].is_number()) {
                        nextW = item["w"].get<float>();
                    }
                    else if (item["w"].is_string()) {
                        nextW = std::stof(item["w"].get<std::string>());
                    }
                }
                if (item.contains("h")) {
                    if (item["h"].is_number()) {
                        nextH = item["h"].get<float>();
                    }
                    else if (item["h"].is_string()) {
                        nextH = std::stof(item["h"].get<std::string>());
                    }
                }
                // Ignore other properties like "c", "p", "w2", "l", "n", "h2", etc.
            }

            // Safety limit to prevent infinite loops
            if (colIndex > maxCols * 2) break;
        }
        curY += 1.f;
    }

    layout.totalW = 0.f;
    layout.totalH = 0.f;
    for (const auto& k : layout.keys) {
        layout.totalW = std::max(layout.totalW, k.x + k.w);
        layout.totalH = std::max(layout.totalH, k.y + k.h);
    }

    if (layout.keys.empty()) {
        fprintf(stderr, "[ERROR] No valid keys found in design.json\n");
        return false;
    }

    fprintf(stderr, "[INFO] Loaded design: %s (%d keys, %dx%d matrix)\n",
        layout.name.c_str(), (int)layout.keys.size(), layout.rows, layout.cols);

    return true;
}

// ─── me.json loader ──────────────────────────────────────────────────────────

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

    // ── Macros - preserve as raw strings ─────────────────────────────────────
    config.macros.clear();
    if (root.contains("macros")) {
        for (const auto& m : root["macros"]) {
            std::string macroStr = m.get<std::string>();
            // Validate macro syntax
            IsValidMacroSyntax(macroStr);
            config.macros.push_back(macroStr);
        }
    }

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
                is_old_fmt = ((int)raw.size() == phys_count &&
                    (int)raw.size() != expected_flat);
                first = false;
            }

            if (is_old_fmt) {
                std::vector<std::string> flat_layer;
                ConvertOldLayer(raw, flat_layer, layout);
                config.layers.push_back(std::move(flat_layer));
            }
            else {
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

    // Ensure at least one layer
    if (config.layers.empty()) {
        config.layers.push_back(std::vector<std::string>(layout.FlatSize(), "KC_NO"));
    }

    return config.IsValid();
}

// ─── me.json saver ───────────────────────────────────────────────────────────

bool SaveConfig(const std::string& path, const KeyboardConfig& config) {
    json root;
    root["_format"] = "doio-me-v1";
    root["name"] = config.name;
    root["vendorProductId"] = config.vendorProductId;

    // Macros - preserve original strings with { } syntax
    root["macros"] = json::array();
    for (const auto& m : config.macros)
        root["macros"].push_back(m);  // Keep as raw string

    // Layers
    root["layers"] = json::array();
    for (const auto& layer : config.layers) {
        json arr = json::array();
        for (const auto& kc : layer) arr.push_back(kc);
        root["layers"].push_back(arr);
    }

    // LED schemes
    root["ledSchemes"] = json::array();
    for (const auto& ls : config.ledSchemes) {
        json s;
        s["name"] = ls.name;
        s["type"] = (int)ls.type;
        s["primary"] = { {"r", ls.primary.r}, {"g", ls.primary.g}, {"b", ls.primary.b} };
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