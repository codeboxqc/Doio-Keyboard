#pragma once
// SessionState.h  ─  Persist last-used design.json / me.json paths across runs
// Writes a tiny INI file next to the .exe: "doio_session.ini"
//
// Usage:
//   SessionState ss;
//   ss.Load();                       // call once at startup
//   ss.designPath / ss.configPath    // non-empty if a previous session existed
//   ss.Save(designPath, configPath); // call whenever either path changes

#include <string>
#include <fstream>
#include <sstream>
#include <windows.h>   // GetModuleFileNameA

struct SessionState {
    std::string designPath;
    std::string configPath;

    // ── Locate the INI file next to the running .exe ──────────────────────────
    static std::string IniPath() {
        char exe[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exe, MAX_PATH);
        // Strip the filename, keep the directory
        std::string p(exe);
        auto slash = p.find_last_of("\\/");
        if (slash != std::string::npos) p = p.substr(0, slash + 1);
        return p + "doio_session.ini";
    }

    // ── Load from INI ─────────────────────────────────────────────────────────
    bool Load() {
        std::ifstream f(IniPath());
        if (!f.is_open()) return false;

        std::string line;
        while (std::getline(f, line)) {
            // Skip blank lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key   = Trim(line.substr(0, eq));
            std::string value = Trim(line.substr(eq + 1));
            if (key == "design") designPath = value;
            if (key == "config") configPath = value;
        }
        return !designPath.empty() || !configPath.empty();
    }

    // ── Save to INI ───────────────────────────────────────────────────────────
    void Save(const std::string& design, const std::string& config) {
        designPath = design;
        configPath = config;
        std::ofstream f(IniPath(), std::ios::trunc);
        if (!f.is_open()) return;
        f << "; DOIO Editor – last session (auto-generated, do not edit manually)\n";
        f << "design=" << design << "\n";
        f << "config=" << config << "\n";
    }

    // Update only the design path (config unchanged)
    void SaveDesign(const std::string& design) { Save(design, configPath); }

    // Update only the config path (design unchanged)
    void SaveConfig(const std::string& config) { Save(designPath, config); }

private:
    static std::string Trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
    }
};
