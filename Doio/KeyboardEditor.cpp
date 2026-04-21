#include "KeyboardEditor.h"
#include "imgui.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <sstream>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")

// ─── Layer button colours ────────────────────────────────────────────────────
const float KeyboardEditor::kLayerColors[kMaxLayers][3] = {
    {0.25f, 0.55f, 0.95f},
    {0.30f, 0.85f, 0.50f},
    {0.95f, 0.65f, 0.20f},
    {0.90f, 0.30f, 0.55f},
    {0.65f, 0.40f, 0.95f},
    {0.20f, 0.85f, 0.80f},
    {0.95f, 0.90f, 0.20f},
    {0.80f, 0.40f, 0.25f},
    {0.55f, 0.85f, 0.30f},
};

// ─── File dialog helpers ─────────────────────────────────────────────────────

static std::string OpenFileDialog(const char* title, const char* filter) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile   = buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = title;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return buf;
    return {};
}

static std::string SaveFileDialog(const char* title, const char* filter,
                                  const char* defaultExt) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.lpstrFilter  = filter;
    ofn.lpstrFile    = buf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = title;
    ofn.lpstrDefExt  = defaultExt;
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) return buf;
    return {};
}

// ─── VK name lookup ──────────────────────────────────────────────────────────

std::string KeyboardEditor::VkName(int vk) {
    // Printable ASCII
    if (vk >= 'A' && vk <= 'Z') { char s[2] = { (char)vk, 0 }; return s; }
    if (vk >= '0' && vk <= '9') { char s[2] = { (char)vk, 0 }; return s; }
    // Named keys
    switch (vk) {
    case VK_RETURN:   return "Enter";
    case VK_ESCAPE:   return "Esc";
    case VK_BACK:     return "Backspace";
    case VK_TAB:      return "Tab";
    case VK_SPACE:    return "Space";
    case VK_SHIFT:    return "Shift";
    case VK_LSHIFT:   return "LShift";
    case VK_RSHIFT:   return "RShift";
    case VK_CONTROL:  return "Ctrl";
    case VK_LCONTROL: return "LCtrl";
    case VK_RCONTROL: return "RCtrl";
    case VK_MENU:     return "Alt";
    case VK_LMENU:    return "LAlt";
    case VK_RMENU:    return "RAlt";
    case VK_LWIN:     return "LWin";
    case VK_RWIN:     return "RWin";
    case VK_APPS:     return "Menu";
    case VK_CAPITAL:  return "CapsLock";
    case VK_NUMLOCK:  return "NumLock";
    case VK_SCROLL:   return "ScrLock";
    case VK_SNAPSHOT: return "PrtSc";
    case VK_PAUSE:    return "Pause";
    case VK_INSERT:   return "Insert";
    case VK_DELETE:   return "Delete";
    case VK_HOME:     return "Home";
    case VK_END:      return "End";
    case VK_PRIOR:    return "PageUp";
    case VK_NEXT:     return "PageDown";
    case VK_UP:       return "Up";
    case VK_DOWN:     return "Down";
    case VK_LEFT:     return "Left";
    case VK_RIGHT:    return "Right";
    case VK_OEM_1:    return ";";
    case VK_OEM_2:    return "/";
    case VK_OEM_3:    return "`";
    case VK_OEM_4:    return "[";
    case VK_OEM_5:    return "\\";
    case VK_OEM_6:    return "]";
    case VK_OEM_7:    return "'";
    case VK_OEM_MINUS:  return "-";
    case VK_OEM_PLUS:   return "=";
    case VK_OEM_COMMA:  return ",";
    case VK_OEM_PERIOD: return ".";
    case VK_MULTIPLY:   return "Num*";
    case VK_ADD:        return "Num+";
    case VK_SUBTRACT:   return "Num-";
    case VK_DIVIDE:     return "Num/";
    case VK_DECIMAL:    return "Num.";
    case VK_NUMPAD0:    return "Num0";
    case VK_NUMPAD1:    return "Num1";
    case VK_NUMPAD2:    return "Num2";
    case VK_NUMPAD3:    return "Num3";
    case VK_NUMPAD4:    return "Num4";
    case VK_NUMPAD5:    return "Num5";
    case VK_NUMPAD6:    return "Num6";
    case VK_NUMPAD7:    return "Num7";
    case VK_NUMPAD8:    return "Num8";
    case VK_NUMPAD9:    return "Num9";
    case VK_DECIMAL:    return "Num.";
    case VK_DIVIDE:     return "Num/";
    case VK_MULTIPLY:   return "Num*";
    case VK_SUBTRACT:   return "Num-";
    case VK_ADD:        return "Num+";
    case VK_VOLUME_MUTE: return "Mute";
    case VK_VOLUME_DOWN: return "Vol-";
    case VK_VOLUME_UP:   return "Vol+";
    case VK_MEDIA_NEXT_TRACK: return "Next";
    case VK_MEDIA_PREV_TRACK: return "Prev";
    case VK_MEDIA_STOP:       return "Stop";
    case VK_MEDIA_PLAY_PAUSE: return "Play";
    case VK_BROWSER_BACK:     return "Back";
    case VK_BROWSER_FORWARD:  return "Fwd";
    case VK_BROWSER_HOME:     return "Home";
    case VK_BROWSER_REFRESH:  return "Refresh";
    default:
        if (vk >= VK_F1 && vk <= VK_F24) {
            char s[8]; snprintf(s, sizeof(s), "F%d", vk - VK_F1 + 1); return s;
        }
        char s[8]; snprintf(s, sizeof(s), "VK%02X", vk); return s;
    }
}

// ─── Constructor ─────────────────────────────────────────────────────────────

KeyboardEditor::KeyboardEditor() {
    m_keycodeDb      = BuildKeycodeDatabase();
    m_keycodeMap     = BuildKeycodeMap();
    m_predefinedSchemes = BuildPredefinedSchemes();
    m_vkDown.assign(256, false);

    // Record app start time for tester timestamps
    m_appStartTime = (float)ImGui::GetTime();
}

// ─── File operations ─────────────────────────────────────────────────────────

void KeyboardEditor::OpenDesign() {
    auto path = OpenFileDialog("Open Design JSON",
                               "JSON Files\0*.json\0All Files\0*.*\0");
    if (path.empty()) return;
    KeyboardLayout tmp;
    if (LoadDesign(path, tmp)) {
        m_layout       = std::move(tmp);
        m_layoutLoaded = true;
        m_selectedKey  = -1;
        EnsureLayerSchemes();
    }
}

void KeyboardEditor::OpenConfig() {
    auto path = OpenFileDialog("Open Config JSON (me.json)",
                               "JSON Files\0*.json\0All Files\0*.*\0");
    if (path.empty()) return;
    KeyboardConfig tmp;
    if (LoadConfig(path, tmp)) {
        m_config       = std::move(tmp);
        m_configLoaded = true;
        m_configPath   = path;
        m_currentLayer = 0;
        m_selectedKey  = -1;
        m_dirty        = false;
        m_undoStack.clear();
        m_redoStack.clear();
        ParseMacrosFromConfig();
        EnsureLayerSchemes();
    }
}

void KeyboardEditor::SaveConfig() {
    if (!m_configLoaded) return;
    SyncMacrosToConfig();
    if (m_configPath.empty()) { SaveConfigAs(); return; }
    if (::SaveConfig(m_configPath, m_config)) m_dirty = false;
}

void KeyboardEditor::SaveConfigAs() {
    if (!m_configLoaded) return;
    SyncMacrosToConfig();
    auto path = SaveFileDialog("Save Config JSON",
                               "JSON Files\0*.json\0All Files\0*.*\0", "json");
    if (path.empty()) return;
    if (::SaveConfig(path, m_config)) {
        m_configPath = path;
        m_dirty      = false;
    }
}

void KeyboardEditor::SaveToKeyboard() {
    if (!m_configLoaded) return;
    SaveConfig();
    // In a real app, this would send the config to the HID device.
    // For now, we simulate success via a transient status message.
}

// ─── Edit operations ─────────────────────────────────────────────────────────

void KeyboardEditor::ApplyKeycode(int keyIdx, const std::string& code) {
    if (!m_configLoaded || !m_layoutLoaded) return;
    if (keyIdx < 0 || keyIdx >= (int)m_layout.keys.size()) return;
    const MatrixPos& mp = m_layout.keys[keyIdx].matrix;
    int fi = m_layout.ConfigIndex(mp);
    if (fi < 0 || fi >= m_config.KeyCount()) return;

    UndoRecord r;
    r.layer   = m_currentLayer;
    r.flatIdx = fi;
    r.oldCode = m_config.Keycode(m_currentLayer, fi);
    r.newCode = code;
    if (r.oldCode == r.newCode) return;

    m_config.Keycode(m_currentLayer, fi) = code;
    PushUndo(r);
    m_dirty = true;
}

void KeyboardEditor::PushUndo(const UndoRecord& r) {
    m_undoStack.push_back(r);
    if (m_undoStack.size() > 200) m_undoStack.pop_front();
    m_redoStack.clear();
}

void KeyboardEditor::Undo() {
    if (m_undoStack.empty()) return;
    auto r = m_undoStack.back(); m_undoStack.pop_back();
    m_config.Keycode(r.layer, r.flatIdx) = r.oldCode;
    m_redoStack.push_back(r);
    m_dirty = true;
}

void KeyboardEditor::Redo() {
    if (m_redoStack.empty()) return;
    auto r = m_redoStack.back(); m_redoStack.pop_back();
    m_config.Keycode(r.layer, r.flatIdx) = r.newCode;
    m_undoStack.push_back(r);
    m_dirty = true;
}

void KeyboardEditor::ResetCurrentLayer() {
    if (!m_configLoaded) return;
    for (auto& kc : m_config.layers[m_currentLayer]) kc = "KC_TRNS";
    m_dirty = true;
    m_redoStack.clear();
}

// ─── Macro helpers ────────────────────────────────────────────────────────────

// Build a compact single-line description of a macro entry
std::string KeyboardEditor::MacroSummary(const MacroEntry& m) const {
    if (m.actions.empty()) return "(empty)";
    std::string s;
    for (int i = 0; i < (int)m.actions.size() && i < 6; ++i) {
        const auto& a = m.actions[i];
        if (!s.empty()) s += " → ";
        switch (a.type) {
        case MacroActionType::KeyTap:
            s += GetKeycodeLabel(a.keycode, m_keycodeMap);
            break;
        case MacroActionType::KeyDown:
            s += "↓" + GetKeycodeLabel(a.keycode, m_keycodeMap);
            break;
        case MacroActionType::KeyUp:
            s += "↑" + GetKeycodeLabel(a.keycode, m_keycodeMap);
            break;
        case MacroActionType::TypeString:
            s += "\"" + (a.text.size() > 10 ? a.text.substr(0, 10) + "…" : a.text) + "\"";
            break;
        case MacroActionType::Delay:
            s += std::to_string(a.delayMs) + "ms";
            break;
        default: break;
        }
    }
    if (m.actions.size() > 6) s += " …";
    return s;
}

void KeyboardEditor::SyncMacrosToConfig() {
    // Serialize rich macro actions back to the simple string format in config
    // Format: "KEYDOWN:KC_A | KEYUP:KC_A | TYPE:hello | DELAY:100 | TAP:KC_ENT"
    m_config.macros.resize(m_macros.size());
    for (int i = 0; i < (int)m_macros.size(); ++i) {
        std::string s;
        for (const auto& a : m_macros[i].actions) {
            if (!s.empty()) s += " | ";
            switch (a.type) {
            case MacroActionType::KeyTap:
                s += "TAP:" + a.keycode; break;
            case MacroActionType::KeyDown:
                s += "KEYDOWN:" + a.keycode; break;
            case MacroActionType::KeyUp:
                s += "KEYUP:" + a.keycode; break;
            case MacroActionType::TypeString:
                s += "TYPE:" + a.text; break;
            case MacroActionType::Delay:
                s += "DELAY:" + std::to_string(a.delayMs); break;
            default: break;
            }
        }
        m_config.macros[i] = s;
    }
}

void KeyboardEditor::ParseMacrosFromConfig() {
    m_macros.resize(m_config.macros.size());
    for (int i = 0; i < (int)m_config.macros.size(); ++i) {
        char nameBuf[32]; snprintf(nameBuf, sizeof(nameBuf), "MACRO%02d", i);
        m_macros[i].name = nameBuf;
        m_macros[i].actions.clear();

        const std::string& raw = m_config.macros[i];
        // Try to parse our serialised format
        // Otherwise treat the whole thing as a TypeString action
        if (raw.find(':') != std::string::npos) {
            // Split by " | "
            size_t pos = 0;
            while (pos < raw.size()) {
                size_t sep = raw.find(" | ", pos);
                std::string token = raw.substr(pos, sep == std::string::npos
                                               ? std::string::npos
                                               : sep - pos);
                pos = sep == std::string::npos ? raw.size() : sep + 3;

                MacroAction act;
                if (token.rfind("TAP:", 0) == 0) {
                    act.type = MacroActionType::KeyTap;
                    act.keycode = token.substr(4);
                } else if (token.rfind("KEYDOWN:", 0) == 0) {
                    act.type = MacroActionType::KeyDown;
                    act.keycode = token.substr(8);
                } else if (token.rfind("KEYUP:", 0) == 0) {
                    act.type = MacroActionType::KeyUp;
                    act.keycode = token.substr(6);
                } else if (token.rfind("TYPE:", 0) == 0) {
                    act.type = MacroActionType::TypeString;
                    act.text = token.substr(5);
                } else if (token.rfind("DELAY:", 0) == 0) {
                    act.type = MacroActionType::Delay;
                    act.delayMs = std::stoi(token.substr(6));
                } else if (!token.empty()) {
                    act.type = MacroActionType::TypeString;
                    act.text = token;
                }
                if (!token.empty()) m_macros[i].actions.push_back(act);
            }
        } else if (!raw.empty()) {
            MacroAction act;
            act.type = MacroActionType::TypeString;
            act.text = raw;
            m_macros[i].actions.push_back(act);
        }
    }
}

// ─── LED scheme helpers ───────────────────────────────────────────────────────

void KeyboardEditor::EnsureLayerSchemes() {
    int n =  max(m_config.LayerCount(), m_layoutLoaded ? 1 : 0);
    if (n == 0) n = 1;

    if (m_configLoaded) {
        while ((int)m_config.ledSchemes.size() < n) {
            int li = (int)m_config.ledSchemes.size();
            LedScheme ls;
            // Default: pick predefined scheme by layer index cycling through palettes
            if (!m_predefinedSchemes.empty()) {
                ls = m_predefinedSchemes[li % m_predefinedSchemes.size()];
            } else {
                ls.name = "Layer " + std::to_string(li);
                ls.type = LedSchemeType::Solid;
                const float* c = kLayerColors[li % kMaxLayers];
                ls.primary = {c[0], c[1], c[2]};
            }
            m_config.ledSchemes.push_back(ls);
        }
    }
}

// Returns the key face colour taking LED scheme into account
ImU32 KeyboardEditor::KeyFaceColor(int ki) const {
    // Fall back to a neutral dark if no data
    if (!m_configLoaded || !m_layoutLoaded) return IM_COL32(50, 55, 65, 255);
    if (ki == m_selectedKey) return IM_COL32(220, 180, 20, 255);

    const PhysicalKey& pk = m_layout.keys[ki];
    int fi = m_layout.ConfigIndex(pk.matrix);
    const std::string& kc = m_config.Keycode(m_currentLayer, fi);

    if (kc == "KC_TRNS" || kc == "KC_NO")
        return IM_COL32(40, 42, 48, 200);

    // Determine which scheme to use
    int li = m_currentLayer;
    if (li < 0 || li >= (int)m_config.ledSchemes.size())
        return IM_COL32(80, 100, 140, 255);

    const LedScheme& scheme = m_config.ledSchemes[li];

    float br = scheme.brightness;
    RgbF p = scheme.primary;

    switch (scheme.type) {
    case LedSchemeType::Rainbow: {
        // Map key x position to hue
        float t = (m_layout.totalW > 0.f)
                  ? (pk.x / m_layout.totalW)
                  : 0.f;
        // Simple HSV→RGB for hue sweep
        float h = t * 360.f;
        float s = 0.9f, v = br;
        float c2 = v * s;
        float x2 = c2 * (1.f - fabsf(fmodf(h / 60.f, 2.f) - 1.f));
        float r = 0, g = 0, b = 0;
        int seg = (int)(h / 60.f) % 6;
        switch (seg) {
        case 0: r=c2; g=x2; break;
        case 1: r=x2; g=c2; break;
        case 2: g=c2; b=x2; break;
        case 3: g=x2; b=c2; break;
        case 4: r=x2; b=c2; break;
        case 5: r=c2; b=x2; break;
        }
        return IM_COL32((uint8_t)((r+(v-c2))*160),
                        (uint8_t)((g+(v-c2))*160),
                        (uint8_t)((b+(v-c2))*160), 255);
    }
    case LedSchemeType::RainbowWave: {
        float t = fmodf((float)ImGui::GetTime() * scheme.speed * 0.3f
                        + pk.x / (m_layout.totalW > 0 ? m_layout.totalW : 1.f), 1.f);
        float h = t * 360.f;
        float c2 = br, x2 = c2 * (1.f - fabsf(fmodf(h/60.f,2.f)-1.f));
        float r=0,g=0,b=0;
        int seg = (int)(h/60.f)%6;
        switch(seg){
        case 0:r=c2;g=x2;break; case 1:r=x2;g=c2;break;
        case 2:g=c2;b=x2;break; case 3:g=x2;b=c2;break;
        case 4:r=x2;b=c2;break; case 5:r=c2;b=x2;break;
        }
        return IM_COL32((uint8_t)(r*200),(uint8_t)(g*200),(uint8_t)(b*200),255);
    }
    case LedSchemeType::Breathing: {
        float pulse = 0.5f + 0.5f * sinf((float)ImGui::GetTime()
                                          * scheme.speed * 2.f);
        float scale = (0.4f + 0.6f * pulse) * br;
        return IM_COL32((uint8_t)(p.r * scale * 220),
                        (uint8_t)(p.g * scale * 220),
                        (uint8_t)(p.b * scale * 220), 255);
    }
    default:
        return IM_COL32((uint8_t)(p.r * br * 160),
                        (uint8_t)(p.g * br * 160),
                        (uint8_t)(p.b * br * 160), 255);
    }
}

// ─── Key Tester polling ───────────────────────────────────────────────────────

void KeyboardEditor::PollKeyTester() {
    if (!m_testerActive) return;
    float now = (float)ImGui::GetTime() - m_appStartTime;

    for (int vk = 1; vk < 256; ++vk) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (down && !m_vkDown[vk]) {
            // Key pressed
            KeyPressEvent ev;
            ev.vkCode    = vk;
            ev.name      = VkName(vk);
            ev.isDown    = true;
            ev.timestamp = now;
            m_keyLog.push_back(ev);
            if ((int)m_keyLog.size() > kLogMaxSize) m_keyLog.pop_front();
        } else if (!down && m_vkDown[vk]) {
            // Key released
            KeyPressEvent ev;
            ev.vkCode    = vk;
            ev.name      = VkName(vk);
            ev.isDown    = false;
            ev.timestamp = now;
            m_keyLog.push_back(ev);
            if ((int)m_keyLog.size() > kLogMaxSize) m_keyLog.pop_front();
        }
        m_vkDown[vk] = down;
    }
}

// ─── Main Render ─────────────────────────────────────────────────────────────

void KeyboardEditor::Render() {
    PollKeyTester();

    ImGuiWindowFlags wf = ImGuiWindowFlags_NoCollapse;

    // ── Keyboard + Layers ────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(820, 680), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keyboard View", nullptr, wf)) {
        RenderLayerPanel();
        ImGui::Separator();
        RenderKeyboardPanel();
    }
    ImGui::End();

    // ── Keycode picker ───────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(340, 680), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keycode Picker", nullptr, wf)) {
        RenderKeycodePanel();
    }
    ImGui::End();

    // ── Macro editor ─────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(560, 460), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Macros", nullptr, wf)) {
        RenderMacroPanel();
    }
    ImGui::End();

    // ── Key Tester ───────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(760, 440), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Key Tester", nullptr, wf)) {
        RenderKeyTesterPanel();
    }
    ImGui::End();

    // ── LED Schemes ──────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("LED Colour Schemes", nullptr, wf)) {
        RenderLedSchemePanel();
    }
    ImGui::End();

    // ── Status bar ───────────────────────────────────────────────────────────
    RenderStatusBar();

    // ── Keyboard shortcuts ───────────────────────────────────────────────────
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (io.KeyShift) SaveToKeyboard();
            else SaveConfig();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_O)) OpenConfig();
    }
}

// ─── Layer selector ──────────────────────────────────────────────────────────

void KeyboardEditor::RenderLayerPanel() {
    if (!m_configLoaded) {
        ImGui::TextDisabled("No config loaded");
        return;
    }
    ImGui::Text("Layer:");
    ImGui::SameLine();
    for (int i = 0; i < m_config.LayerCount(); ++i) {
        const float* c = kLayerColors[i % kMaxLayers];
        ImVec4 col(c[0], c[1], c[2], 1.f);
        ImVec4 colDim(c[0]*0.5f, c[1]*0.5f, c[2]*0.5f, 1.f);

        if (m_currentLayer == i) ImGui::PushStyleColor(ImGuiCol_Button, col);
        else                     ImGui::PushStyleColor(ImGuiCol_Button, colDim);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);

        char lbl[8]; snprintf(lbl, sizeof(lbl), " %d ", i);
        if (ImGui::Button(lbl)) { m_currentLayer = i; m_selectedKey = -1; }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

// ─── Keyboard visual ─────────────────────────────────────────────────────────

void KeyboardEditor::RenderKeyboardPanel() {
    if (!m_layoutLoaded) {
        ImGui::TextDisabled("Load a design.json to see the keyboard layout.");
        if (ImGui::Button("Open design.json")) OpenDesign();
        return;
    }

    ImVec2 avail   = ImGui::GetContentRegionAvail();
    float  keyUnit = min(avail.x / (m_layout.totalW + 0.5f),
                         avail.y / (m_layout.totalH + 0.5f));
    keyUnit = max(keyUnit, 20.f);
    keyUnit = min(keyUnit, 80.f);

    float originX = ImGui::GetCursorScreenPos().x + 12.f;
    float originY = ImGui::GetCursorScreenPos().y + 12.f;

    ImGui::Dummy(ImVec2(m_layout.totalW * keyUnit + 24.f,
                        m_layout.totalH * keyUnit + 24.f));

    DrawKeyboard(originX, originY, keyUnit);
}

void KeyboardEditor::DrawKeyboard(float ox, float oy, float ku) {
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImGuiIO&    io  = ImGui::GetIO();
    const float pad     = 3.f;
    const float rounding = 5.f;

    ImU32 selBorder = IM_COL32(255, 240, 100, 255);

    for (int ki = 0; ki < (int)m_layout.keys.size(); ++ki) {
        const PhysicalKey& pk = m_layout.keys[ki];

        ImVec2 tl(ox + pk.x * ku + pad, oy + pk.y * ku + pad);
        ImVec2 br(ox + (pk.x + pk.w) * ku - pad, oy + (pk.y + pk.h) * ku - pad);

        std::string kc = "---";
        if (m_configLoaded) {
            int fi = m_layout.ConfigIndex(pk.matrix);
            kc = m_config.Keycode(m_currentLayer, fi);
        }

        ImU32 faceCol  = KeyFaceColor(ki);
        ImU32 borderCol;
        if (ki == m_selectedKey) {
            borderCol = selBorder;
        } else {
            // Slightly lighter version of face for border
            borderCol = IM_COL32(
                 min(255, (int)(((faceCol >> 0)  & 0xFF) + 60)),
                 min(255, (int)(((faceCol >> 8)  & 0xFF) + 60)),
                 min(255, (int)(((faceCol >> 16) & 0xFF) + 60)),
                255);
        }

        // Shadow
        dl->AddRectFilled(ImVec2(tl.x+2,tl.y+2), ImVec2(br.x+2,br.y+2),
                          IM_COL32(0,0,0,100), rounding);
        // Face
        dl->AddRectFilled(tl, br, faceCol, rounding);
        // Border
        dl->AddRect(tl, br, borderCol, rounding, 0, 2.f);

        // Matrix badge
        char badge[16];
        snprintf(badge, sizeof(badge), "%d,%d", pk.matrix.row, pk.matrix.col);
        dl->AddText(ImVec2(tl.x+3, tl.y+3), IM_COL32(180,180,180,100), badge);

        // Label
        std::string label = GetKeycodeLabel(kc, m_keycodeMap);
        ImVec2 tsz = ImGui::CalcTextSize(label.c_str());
        float cx = tl.x + (br.x - tl.x - tsz.x) * 0.5f;
        float cy = tl.y + (br.y - tl.y - tsz.y) * 0.5f;
        ImU32 textCol = (ki == m_selectedKey)
                        ? IM_COL32(30,20,0,255)
                        : IM_COL32(230,230,230,255);
        dl->AddText(ImVec2(cx, cy), textCol, label.c_str());

        // Mouse interaction
        ImVec2 mpos = io.MousePos;
        bool hovered = mpos.x >= tl.x && mpos.x < br.x &&
                       mpos.y >= tl.y && mpos.y < br.y;
        if (hovered) {
            dl->AddRect(tl, br, IM_COL32(255,255,255,180), rounding, 0, 2.5f);
            ImGui::SetTooltip("[%d,%d]  %s", pk.matrix.row, pk.matrix.col, kc.c_str());
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                m_selectedKey = ki;
        }
    }
}

// ─── Keycode picker ──────────────────────────────────────────────────────────

void KeyboardEditor::RenderKeycodePanel() {
    if (m_selectedKey < 0) {
        ImGui::TextDisabled("Click a key to assign a keycode.");
        return;
    }
    if (!m_configLoaded) {
        ImGui::TextDisabled("No config loaded.");
        return;
    }

    const PhysicalKey& pk = m_layout.keys[m_selectedKey];
    int fi = m_layout.ConfigIndex(pk.matrix);
    const std::string& cur = m_config.Keycode(m_currentLayer, fi);

    ImGui::Text("Selected:  [%d,%d]", pk.matrix.row, pk.matrix.col);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.f,0.9f,0.3f,1.f), "  %s", cur.c_str());

    ImGui::Separator();
    ImGui::Text("Copy from layer:");
    ImGui::SameLine();
    for (int i = 0; i < m_config.LayerCount(); ++i) {
        if (i == m_currentLayer) { ImGui::SameLine(); continue; }
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%d", i);
        if (ImGui::SmallButton(lbl)) ApplyKeycode(m_selectedKey, m_config.Keycode(i, fi));
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##search", m_searchBuf, sizeof(m_searchBuf));

    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4,2));
        if (ImGui::RadioButton("All", m_selectedCategory == -1))
            m_selectedCategory = -1;
        for (int ci = 0; ci < (int)KeyCategory::COUNT; ++ci) {
            ImGui::SameLine();
            if (ImGui::RadioButton(CategoryName((KeyCategory)ci), m_selectedCategory == ci))
                m_selectedCategory = ci;
            if ((ci+1) % 4 == 0) ImGui::NewLine();
        }
        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    std::string search(m_searchBuf);
    std::transform(search.begin(), search.end(), search.begin(), ::tolower);

    ImGui::BeginChild("##kclist", ImVec2(0,0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    int col = 0, ncols = 3;
    float btnW = (ImGui::GetContentRegionAvail().x - (ncols-1)*4) / ncols;

    for (const auto& def : m_keycodeDb) {
        if (m_selectedCategory >= 0 && (int)def.category != m_selectedCategory) continue;
        if (!search.empty()) {
            std::string lc = def.code;  std::transform(lc.begin(),lc.end(),lc.begin(),::tolower);
            std::string ll = def.label; std::transform(ll.begin(),ll.end(),ll.begin(),::tolower);
            std::string lt = def.tooltip; std::transform(lt.begin(),lt.end(),lt.begin(),::tolower);
            if (lc.find(search)==std::string::npos &&
                ll.find(search)==std::string::npos &&
                lt.find(search)==std::string::npos) continue;
        }

        bool isCur = (def.code == cur);
        if (isCur) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f,0.7f,0.1f,1.f));

        if (col > 0 && col < ncols) ImGui::SameLine();
        ImGui::PushID(def.code.c_str());
        if (ImGui::Button(def.label.c_str(), ImVec2(btnW, 0)))
            ApplyKeycode(m_selectedKey, def.code);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s\n%s", def.code.c_str(), def.tooltip.c_str());
        ImGui::PopID();
        if (isCur) ImGui::PopStyleColor();
        col = (col+1) % ncols;
    }

    ImGui::Separator();
    ImGui::Text("Custom keycode:");
    static char customBuf[64] = {};
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.f);
    ImGui::InputText("##custom", customBuf, sizeof(customBuf));
    ImGui::SameLine();
    if (ImGui::Button("Apply") && customBuf[0]) {
        ApplyKeycode(m_selectedKey, customBuf);
        customBuf[0] = '\0';
    }

    ImGui::Separator();
    ImGui::Text("Layer actions:");
    for (int i = 0; i < m_config.LayerCount(); ++i) {
        char lbl[32];
        snprintf(lbl, sizeof(lbl), "TO(%d)", i);
        ImGui::PushID(lbl);
        if (ImGui::SmallButton(lbl)) ApplyKeycode(m_selectedKey, lbl);
        ImGui::PopID(); ImGui::SameLine();
        snprintf(lbl, sizeof(lbl), "MO(%d)", i);
        ImGui::PushID(lbl);
        if (ImGui::SmallButton(lbl)) ApplyKeycode(m_selectedKey, lbl);
        ImGui::PopID();
        if ((i+1) % 4 != 0) ImGui::SameLine(); else ImGui::NewLine();
    }

    if (!m_macros.empty()) {
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::Text("Assign Macro:");
        for (int mi = 0; mi < (int)m_macros.size(); ++mi) {
            char lbl[32]; snprintf(lbl, sizeof(lbl), "M%02d", mi);
            ImGui::PushID(mi + 1000);
            char kclbl[32]; snprintf(kclbl, sizeof(kclbl), "MACRO%02d", mi);
            if (ImGui::SmallButton(lbl)) ApplyKeycode(m_selectedKey, kclbl);
            if (ImGui::IsItemHovered()) {
                std::string summary = MacroSummary(m_macros[mi]);
                ImGui::SetTooltip("%s\n%s", m_macros[mi].name.c_str(), summary.c_str());
            }
            ImGui::PopID();
            if ((mi+1) % 8 != 0) ImGui::SameLine(); else ImGui::NewLine();
        }
    }

    ImGui::EndChild();
}

// ─── Macro editor ─────────────────────────────────────────────────────────────

void KeyboardEditor::RenderMacroPanel() {
    // Sync macro count with config (always show at least what config has)
    if (m_configLoaded && (int)m_macros.size() < m_config.LayerCount()) {
        ParseMacrosFromConfig();
    }

    // Top bar: add macro, search
    if (ImGui::Button("+ New Macro")) {
        MacroEntry e;
        char nb[32]; snprintf(nb, sizeof(nb), "MACRO%02d", (int)m_macros.size());
        e.name = nb;
        m_macros.push_back(e);
        m_config.macros.push_back("");
        m_dirty = true;
        m_editingMacro = (int)m_macros.size() - 1;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.f);
    ImGui::InputText("Search##msearch", m_macroSearchBuf, sizeof(m_macroSearchBuf));

    if (m_macros.empty()) {
        ImGui::TextDisabled("No macros yet. Click '+ New Macro'.");
        return;
    }

    ImGui::Separator();

    // ── Left: macro list ──────────────────────────────────────────────────────
    float listW = 180.f;
    ImGui::BeginChild("##macrolist", ImVec2(listW, 0), true);

    std::string mSearch(m_macroSearchBuf);
    std::transform(mSearch.begin(), mSearch.end(), mSearch.begin(), ::tolower);

    for (int i = 0; i < (int)m_macros.size(); ++i) {
        MacroEntry& me = m_macros[i];

        // Search filter
        std::string ln = me.name; std::transform(ln.begin(),ln.end(),ln.begin(),::tolower);
        if (!mSearch.empty() && ln.find(mSearch) == std::string::npos) continue;

        ImGui::PushID(i);
        bool selected = (m_editingMacro == i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f,0.5f,0.85f,1.f));

        char label[48];
        snprintf(label, sizeof(label), "%s##mli", me.name.c_str());

        if (ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns)) {
            m_editingMacro  = i;
            m_editingAction = -1;
        }
        if (selected) ImGui::PopStyleColor();

        // Summary tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", MacroSummary(me).c_str());
        }

        // Delete button on right-click context
        if (ImGui::BeginPopupContextItem("##mctx")) {
            if (ImGui::MenuItem("Delete Macro")) {
                m_macros.erase(m_macros.begin() + i);
                if (m_configLoaded && i < (int)m_config.macros.size())
                    m_config.macros.erase(m_config.macros.begin() + i);
                m_editingMacro  = -1;
                m_editingAction = -1;
                m_dirty = true;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // ── Right: action editor ──────────────────────────────────────────────────
    ImGui::BeginChild("##macroedit", ImVec2(0, 0), true);

    if (m_editingMacro < 0 || m_editingMacro >= (int)m_macros.size()) {
        ImGui::TextDisabled("Select a macro from the left.");
        ImGui::EndChild();
        return;
    }

    MacroEntry& me = m_macros[m_editingMacro];

    // Name field
    {
        char nb[64]; strncpy_s(nb, me.name.c_str(), sizeof(nb)-1);
        ImGui::SetNextItemWidth(200.f);
        if (ImGui::InputText("Name##mname", nb, sizeof(nb)))
            me.name = nb;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%d actions)", (int)me.actions.size());
    ImGui::Separator();

    // ── Action list table ─────────────────────────────────────────────────────
    if (ImGui::BeginTable("##actions", 4,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit,
        ImVec2(0.f, 180.f)))
    {
        ImGui::TableSetupColumn("#",       ImGuiTableColumnFlags_WidthFixed, 28.f);
        ImGui::TableSetupColumn("Type",    ImGuiTableColumnFlags_WidthFixed, 120.f);
        ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("  ",      ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableHeadersRow();

        for (int ai = 0; ai < (int)me.actions.size(); ++ai) {
            MacroAction& act = me.actions[ai];
            ImGui::TableNextRow();
            ImGui::PushID(ai);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", ai+1);

            ImGui::TableSetColumnIndex(1);
            // Inline type combo
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::BeginCombo("##atype",
                MacroActionTypeName(act.type), ImGuiComboFlags_NoArrowButton))
            {
                for (int t = 0; t < (int)MacroActionType::COUNT; ++t) {
                    bool sel = ((int)act.type == t);
                    if (ImGui::Selectable(MacroActionTypeName((MacroActionType)t), sel))
                        act.type = (MacroActionType)t;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1.f);
            switch (act.type) {
            case MacroActionType::KeyTap:
            case MacroActionType::KeyDown:
            case MacroActionType::KeyUp: {
                char kbuf[64]; strncpy_s(kbuf, act.keycode.c_str(), sizeof(kbuf)-1);
                if (ImGui::InputText("##kc", kbuf, sizeof(kbuf))) {
                    act.keycode = kbuf;
                    m_dirty = true;
                }
                break;
            }
            case MacroActionType::TypeString: {
                char tbuf[256]; strncpy_s(tbuf, act.text.c_str(), sizeof(tbuf)-1);
                if (ImGui::InputText("##txt", tbuf, sizeof(tbuf))) {
                    act.text = tbuf;
                    m_dirty = true;
                }
                break;
            }
            case MacroActionType::Delay: {
                if (ImGui::InputInt("##dms", &act.delayMs)) {
                    if (act.delayMs < 1) act.delayMs = 1;
                    m_dirty = true;
                }
                break;
            }
            default: break;
            }

            ImGui::TableSetColumnIndex(3);
            // Up / Down / Delete
            if (ImGui::SmallButton("↑") && ai > 0)
                std::swap(me.actions[ai], me.actions[ai-1]);
            ImGui::SameLine();
            if (ImGui::SmallButton("✕")) {
                me.actions.erase(me.actions.begin() + ai);
                m_dirty = true;
                ImGui::PopID();
                break; // avoid dangling reference
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // ── Add action row ────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Add action:");
    ImGui::SameLine();

    // Type selector
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::BeginCombo("##natype",
        MacroActionTypeName(m_newActionType), ImGuiComboFlags_NoArrowButton))
    {
        for (int t = 0; t < (int)MacroActionType::COUNT; ++t) {
            bool sel = ((int)m_newActionType == t);
            if (ImGui::Selectable(MacroActionTypeName((MacroActionType)t), sel))
                m_newActionType = (MacroActionType)t;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    switch (m_newActionType) {
    case MacroActionType::KeyTap:
    case MacroActionType::KeyDown:
    case MacroActionType::KeyUp:
        ImGui::SetNextItemWidth(120.f);
        ImGui::InputText("##nak", m_newActionKey, sizeof(m_newActionKey));
        break;
    case MacroActionType::TypeString:
        ImGui::SetNextItemWidth(180.f);
        ImGui::InputText("##nat", m_newActionText, sizeof(m_newActionText));
        break;
    case MacroActionType::Delay:
        ImGui::SetNextItemWidth(80.f);
        ImGui::InputInt("ms##nad", &m_newActionDelay);
        if (m_newActionDelay < 1) m_newActionDelay = 1;
        break;
    default: break;
    }
    ImGui::SameLine();

    if (ImGui::Button("Add")) {
        MacroAction a;
        a.type = m_newActionType;
        switch (m_newActionType) {
        case MacroActionType::KeyTap:
        case MacroActionType::KeyDown:
        case MacroActionType::KeyUp:
            a.keycode = m_newActionKey;
            break;
        case MacroActionType::TypeString:
            a.text = m_newActionText;
            break;
        case MacroActionType::Delay:
            a.delayMs = m_newActionDelay;
            break;
        default: break;
        }
        me.actions.push_back(a);
        m_dirty = true;
    }

    // ── Quick-insert common keys ──────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextDisabled("Quick-insert common keys:");
    const char* quickKeys[] = {
        "KC_A","KC_B","KC_C","KC_ENT","KC_SPC","KC_BSPC",
        "KC_TAB","KC_ESC","KC_LSFT","KC_LCTL","KC_LALT"
    };
    for (int qi = 0; qi < (int)(sizeof(quickKeys)/sizeof(quickKeys[0])); ++qi) {
        ImGui::PushID(qi + 9000);
        std::string ql = GetKeycodeLabel(quickKeys[qi], m_keycodeMap);
        if (ImGui::SmallButton(ql.c_str())) {
            MacroAction a;
            a.type    = MacroActionType::KeyTap;
            a.keycode = quickKeys[qi];
            me.actions.push_back(a);
            m_dirty = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", quickKeys[qi]);
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::EndChild();
}

// ─── Key Tester ───────────────────────────────────────────────────────────────

void KeyboardEditor::RenderKeyTesterPanel() {
    // Toggle
    if (m_testerActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f,0.2f,0.2f,1.f));
        if (ImGui::Button("■ Stop Testing")) {
            m_testerActive = false;
            m_vkDown.assign(256, false);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.6f,0.3f,1.f));
        if (ImGui::Button("▶ Start Key Test")) {
            m_testerActive = true;
            m_keyLog.clear();
            m_vkDown.assign(256, false);
        }
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Log")) m_keyLog.clear();
    ImGui::SameLine();
    ImGui::TextDisabled(m_testerActive
        ? "Press keys — detects via GetAsyncKeyState"
        : "Click Start to begin capture");

    ImGui::Separator();

    // ── Currently held keys (visual chips) ───────────────────────────────────
    ImGui::Text("Held now:");
    ImGui::SameLine();
    bool anyHeld = false;
    for (int vk = 1; vk < 256; ++vk) {
        if (!m_vkDown[vk]) continue;
        anyHeld = true;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f,0.55f,0.95f,1.f));
        ImGui::SmallButton(VkName(vk).c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    if (!anyHeld) ImGui::TextDisabled("(none)");
    ImGui::NewLine();
    ImGui::Separator();

    // ── Keyboard grid visual ─────────────────────────────────────────────────
    // Map VK codes to QMK keycodes so we can highlight on the layout
    // Simple: if layout loaded, highlight keys whose keycode VK matches pressed
    if (m_layoutLoaded && m_configLoaded) {
        ImGui::Text("Layout highlight (layer %d):", m_currentLayer);
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float ku = min(avail.x / (m_layout.totalW + 0.5f), 42.f);
        ku = max(ku, 14.f);
        float ox = ImGui::GetCursorScreenPos().x + 4.f;
        float oy = ImGui::GetCursorScreenPos().y + 4.f;
        ImGui::Dummy(ImVec2(m_layout.totalW * ku + 8.f,
                             m_layout.totalH * ku + 8.f));
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Build a small VK→KC map for common keys
        static const struct { int vk; const char* kc; } vkmap[] = {
            {'A',"KC_A"},{'B',"KC_B"},{'C',"KC_C"},{'D',"KC_D"},{'E',"KC_E"},
            {'F',"KC_F"},{'G',"KC_G"},{'H',"KC_H"},{'I',"KC_I"},{'J',"KC_J"},
            {'K',"KC_K"},{'L',"KC_L"},{'M',"KC_M"},{'N',"KC_N"},{'O',"KC_O"},
            {'P',"KC_P"},{'Q',"KC_Q"},{'R',"KC_R"},{'S',"KC_S"},{'T',"KC_T"},
            {'U',"KC_U"},{'V',"KC_V"},{'W',"KC_W"},{'X',"KC_X"},{'Y',"KC_Y"},
            {'Z',"KC_Z"},
            {'0',"KC_0"},{'1',"KC_1"},{'2',"KC_2"},{'3',"KC_3"},{'4',"KC_4"},
            {'5',"KC_5"},{'6',"KC_6"},{'7',"KC_7"},{'8',"KC_8"},{'9',"KC_9"},
            {VK_RETURN,"KC_ENT"},{VK_ESCAPE,"KC_ESC"},{VK_BACK,"KC_BSPC"},
            {VK_TAB,"KC_TAB"},{VK_SPACE,"KC_SPC"},{VK_LSHIFT,"KC_LSFT"},
            {VK_RSHIFT,"KC_RSFT"},{VK_LCONTROL,"KC_LCTL"},{VK_RCONTROL,"KC_RCTL"},
            {VK_LMENU,"KC_LALT"},{VK_RMENU,"KC_RALT"},{VK_LWIN,"KC_LGUI"},
            {VK_CAPITAL,"KC_CAPS"},{VK_DELETE,"KC_DEL"},{VK_INSERT,"KC_INS"},
            {VK_HOME,"KC_HOME"},{VK_END,"KC_END"},{VK_PRIOR,"KC_PGUP"},
            {VK_NEXT,"KC_PGDN"},{VK_UP,"KC_UP"},{VK_DOWN,"KC_DOWN"},
            {VK_LEFT,"KC_LEFT"},{VK_RIGHT,"KC_RGHT"},
            {VK_OEM_MINUS,"KC_MINS"},{VK_OEM_PLUS,"KC_EQL"},
            {VK_OEM_4,"KC_LBRC"},{VK_OEM_6,"KC_RBRC"},{VK_OEM_5,"KC_BSLS"},
            {VK_OEM_1,"KC_SCLN"},{VK_OEM_7,"KC_QUOT"},{VK_OEM_3,"KC_GRV"},
            {VK_OEM_COMMA,"KC_COMM"},{VK_OEM_PERIOD,"KC_DOT"},{VK_OEM_2,"KC_SLSH"},
            {VK_F1,"KC_F1"},{VK_F2,"KC_F2"},{VK_F3,"KC_F3"},{VK_F4,"KC_F4"},
            {VK_F5,"KC_F5"},{VK_F6,"KC_F6"},{VK_F7,"KC_F7"},{VK_F8,"KC_F8"},
            {VK_F9,"KC_F9"},{VK_F10,"KC_F10"},{VK_F11,"KC_F11"},{VK_F12,"KC_F12"},
            {VK_NUMPAD0,"KC_P0"},{VK_NUMPAD1,"KC_P1"},{VK_NUMPAD2,"KC_P2"},
            {VK_NUMPAD3,"KC_P3"},{VK_NUMPAD4,"KC_P4"},{VK_NUMPAD5,"KC_P5"},
            {VK_NUMPAD6,"KC_P6"},{VK_NUMPAD7,"KC_P7"},{VK_NUMPAD8,"KC_P8"},
            {VK_NUMPAD9,"KC_P9"},{VK_DECIMAL,"KC_PDOT"},{VK_DIVIDE,"KC_PSLS"},
            {VK_MULTIPLY,"KC_PAST"},{VK_SUBTRACT,"KC_PMNS"},{VK_ADD,"KC_PPLS"},
            {VK_VOLUME_MUTE,"KC_MUTE"},{VK_VOLUME_DOWN,"KC_VOLD"},{VK_VOLUME_UP,"KC_VOLU"},
            {VK_MEDIA_NEXT_TRACK,"KC_MNXT"},{VK_MEDIA_PREV_TRACK,"KC_MPRV"},
            {VK_MEDIA_STOP,"KC_MSTP"},{VK_MEDIA_PLAY_PAUSE,"KC_MPLY"},
            {VK_BROWSER_BACK,"KC_WWW_BACK"},{VK_BROWSER_FORWARD,"KC_WWW_FWRD"},
            {VK_BROWSER_HOME,"KC_WWW_HOME"},{VK_BROWSER_REFRESH,"KC_WWW_RFSH"},
        };

        // Build set of currently-pressed QMK keycodes
        std::unordered_map<std::string,bool> pressedKc;
        if (m_testerActive) {
            for (auto& vm : vkmap) {
                if (m_vkDown[vm.vk]) pressedKc[vm.kc] = true;
            }
        }

        const float p = 2.f, r = 3.f;
        for (const auto& pk : m_layout.keys) {
            ImVec2 tl(ox + pk.x*ku+p, oy + pk.y*ku+p);
            ImVec2 br(ox + (pk.x+pk.w)*ku-p, oy + (pk.y+pk.h)*ku-p);

            int fi = m_layout.ConfigIndex(pk.matrix);
            const std::string& kc = m_config.Keycode(m_currentLayer, fi);
            bool pressed = pressedKc.count(kc) > 0;

            ImU32 face = pressed
                ? IM_COL32(60, 200, 100, 255)
                : IM_COL32(40, 45, 55, 220);
            ImU32 border = pressed
                ? IM_COL32(120, 255, 160, 255)
                : IM_COL32(80, 85, 100, 255);

            dl->AddRectFilled(tl, br, face, r);
            dl->AddRect(tl, br, border, r, 0, 1.5f);

            std::string lbl = GetKeycodeLabel(kc, m_keycodeMap);
            ImVec2 ts = ImGui::CalcTextSize(lbl.c_str());
            // Only draw label if key is wide enough
            if ((br.x - tl.x) > ts.x + 2.f) {
                float cx = tl.x + (br.x-tl.x-ts.x)*0.5f;
                float cy = tl.y + (br.y-tl.y-ts.y)*0.5f;
                dl->AddText({cx,cy},
                    pressed ? IM_COL32(20,30,20,255) : IM_COL32(180,185,200,255),
                    lbl.c_str());
            }
        }
    } else {
        ImGui::TextDisabled("Load design.json + me.json to see layout highlighting.");
    }

    ImGui::Separator();

    // ── Event log ─────────────────────────────────────────────────────────────
    ImGui::Text("Event log (%d events):", (int)m_keyLog.size());
    ImGui::SameLine();
    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::BeginChild("##keylog", ImVec2(0, 120), true);
    for (auto it = m_keyLog.rbegin(); it != m_keyLog.rend(); ++it) {
        ImVec4 col = it->isDown
            ? ImVec4(0.4f, 0.9f, 0.4f, 1.f)
            : ImVec4(0.7f, 0.7f, 0.7f, 1.f);
        ImGui::TextColored(col, "[%6.2fs] %s  VK=0x%02X  %s",
            it->timestamp,
            it->isDown ? "DOWN" : "UP  ",
            it->vkCode,
            it->name.c_str());
    }
    if (autoScroll) ImGui::SetScrollHereY(0.f);
    ImGui::EndChild();
}

// ─── LED Scheme panel ─────────────────────────────────────────────────────────

void KeyboardEditor::RenderLedSchemePanel() {
    EnsureLayerSchemes();

    if (!m_configLoaded) {
        ImGui::TextDisabled("Load a me.json to configure LED schemes.");
        return;
    }

    int layerCount = m_config.LayerCount();

    ImGui::Text("Per-layer LED colour scheme");
    ImGui::TextDisabled("Each layer has its own independent lighting configuration.");
    ImGui::Separator();

    // Tabs: one per layer
    if (ImGui::BeginTabBar("##ledlayers")) {
        for (int li = 0; li < layerCount && li < (int)m_config.ledSchemes.size(); ++li) {
            char tabName[16]; snprintf(tabName, sizeof(tabName), "Layer %d", li);
            LedScheme& cur = m_config.ledSchemes[li];

            // Colour the tab with the layer's primary colour
            ImGui::PushStyleColor(ImGuiCol_Tab,
                ImVec4(cur.primary.r * 0.4f, cur.primary.g * 0.4f, cur.primary.b * 0.4f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_TabActive,
                ImVec4(cur.primary.r * 0.7f, cur.primary.g * 0.7f, cur.primary.b * 0.7f, 1.f));

            if (ImGui::BeginTabItem(tabName)) {
                ImGui::PopStyleColor(2);
                ImGui::PushID(li);

                // ── Predefined palette grid ────────────────────────────────
                ImGui::Text("Apply Template:");
                ImGui::Separator();

                int cols = 3;
                float btnW = (ImGui::GetContentRegionAvail().x - (cols - 1) * 6) / cols;
                for (int pi = 0; pi < (int)m_predefinedSchemes.size(); ++pi) {
                    const LedScheme& ps = m_predefinedSchemes[pi];

                    // Coloured button
                    ImVec4 c(ps.primary.r * 0.8f + 0.1f,
                        ps.primary.g * 0.8f + 0.1f,
                        ps.primary.b * 0.8f + 0.1f, 1.f);
                    ImVec4 cDim(c.x * 0.5f, c.y * 0.5f, c.z * 0.5f, 1.f);

                    ImGui::PushID(pi);
                    ImGui::PushStyleColor(ImGuiCol_Button, cDim);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c);

                    if (ImGui::Button(ps.name.c_str(), ImVec2(btnW, 0))) {
                        cur = ps; // Copy entire scheme
                        m_dirty = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Apply '%s' template", ps.name.c_str());

                    ImGui::PopStyleColor(2);
                    ImGui::PopID();
                    if ((pi + 1) % cols != 0) ImGui::SameLine();
                }

                ImGui::Separator();
                ImGui::Text("Adjust Parameters:");

                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.13f, 0.18f, 1.f));
                ImGui::BeginChild("##custscheme", ImVec2(0, 200), true);

                // Type
                ImGui::SetNextItemWidth(180.f);
                if (ImGui::BeginCombo("Animation Type", LedSchemeTypeName(cur.type))) {
                    for (int t = 0; t < (int)LedSchemeType::COUNT; ++t) {
                        bool sel = ((int)cur.type == t);
                        if (ImGui::Selectable(LedSchemeTypeName((LedSchemeType)t), sel)) {
                            cur.type = (LedSchemeType)t;
                            m_dirty = true;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Primary colour picker
                float pArr[3] = { cur.primary.r, cur.primary.g, cur.primary.b };
                if (ImGui::ColorEdit3("Primary Colour", pArr,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel)) {
                    cur.primary = { pArr[0], pArr[1], pArr[2] };
                    m_dirty = true;
                }

                // Secondary (shown for gradient types)
                if (cur.type == LedSchemeType::Rainbow ||
                    cur.type == LedSchemeType::RainbowWave) {
                    float sArr[3] = { cur.secondary.r, cur.secondary.g, cur.secondary.b };
                    if (ImGui::ColorEdit3("Secondary Colour", sArr,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel)) {
                        cur.secondary = { sArr[0], sArr[1], sArr[2] };
                        m_dirty = true;
                    }
                }

                if (ImGui::SliderFloat("Speed", &cur.speed, 0.1f, 5.f)) m_dirty = true;
                if (ImGui::SliderFloat("Brightness", &cur.brightness, 0.f, 1.f)) m_dirty = true;

                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::PopID();

                ImGui::EndTabItem();
            } else {
                ImGui::PopStyleColor(2);
            }
        }
        ImGui::EndTabBar();
    }
}

// ─── Status bar ───────────────────────────────────────────────────────────────

void KeyboardEditor::RenderStatusBar() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float h = ImGui::GetFrameHeight() + 4.f;
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - h));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, h));
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f,0.12f,0.15f,1.f));
    ImGui::Begin("##statusbar", nullptr, flags);

    if (m_configLoaded) {
        ImGui::Text("Config: %s",
            m_configPath.empty() ? m_config.name.c_str() : m_configPath.c_str());
        ImGui::SameLine();
        if (m_dirty) ImGui::TextColored(ImVec4(1,0.5f,0.2f,1), " [unsaved]");
    } else {
        ImGui::TextDisabled("No config loaded  |  File → Open Config (me.json)");
    }

    if (m_layoutLoaded) {
        ImGui::SameLine(0,20);
        ImGui::TextDisabled("Layout: %s  |  %d keys  |  %d layers",
            m_layout.name.c_str(), (int)m_layout.keys.size(),
            m_config.LayerCount());
    }

    if (m_testerActive) {
        ImGui::SameLine(0,20);
        ImGui::TextColored(ImVec4(0.3f,0.9f,0.4f,1.f), "● KEY TESTER ACTIVE");
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 240.f + ImGui::GetCursorPosX());
    ImGui::TextDisabled("Ctrl+Z Undo  |  Ctrl+Y Redo  |  Ctrl+S Save");

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
