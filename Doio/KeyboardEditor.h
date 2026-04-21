#pragma once
#include "KeyboardConfig.h"
#include "KeycodeDefs.h"
#include "LedScheme.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <imgui.h>

// ─── Undo / Redo ─────────────────────────────────────────────────────────────

struct UndoRecord {
    int         layer;
    int         flatIdx;
    std::string oldCode;
    std::string newCode;
};

// ─── Macro action ─────────────────────────────────────────────────────────────

enum class MacroActionType {
    KeyTap,     // tap a single keycode
    KeyDown,    // hold a key
    KeyUp,      // release a key
    TypeString, // emit a string of characters
    Delay,      // wait N milliseconds
    COUNT
};

inline const char* MacroActionTypeName(MacroActionType t) {
    switch (t) {
    case MacroActionType::KeyTap:     return "Key Tap";
    case MacroActionType::KeyDown:    return "Key Down (hold)";
    case MacroActionType::KeyUp:      return "Key Up (release)";
    case MacroActionType::TypeString: return "Type String";
    case MacroActionType::Delay:      return "Delay (ms)";
    default: return "?";
    }
}

struct MacroAction {
    MacroActionType type      = MacroActionType::KeyTap;
    std::string     keycode   = "KC_A";   // used for KeyTap / KeyDown / KeyUp
    std::string     text;                 // used for TypeString
    int             delayMs   = 50;       // used for Delay
};

struct MacroEntry {
    std::string               name;           // user-friendly name
    std::vector<MacroAction>  actions;
};

#include "Keytest.h"
// ─── Main editor ─────────────────────────────────────────────────────────────

class KeyboardEditor {
public:
    KeyboardEditor();

    // File operations (open system file dialog)
    void OpenDesign();
    void OpenConfig();
    void SaveConfig();
    void SaveConfigAs();
    void SaveToKeyboard();

    // Edit operations
    bool IsConfigLoaded() const { return m_configLoaded; }
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }
    void Undo();
    void Redo();
    void ResetCurrentLayer();

    // Main render call — call once per frame inside ImGui frame
    void Render();

private:
    // ── Data ─────────────────────────────────────────────────────────────────
    KeyboardLayout   m_layout;
    KeyboardConfig   m_config;
    bool             m_layoutLoaded = false;
    bool             m_configLoaded = false;
    std::string      m_configPath;

    int  m_currentLayer  = 0;
    int  m_selectedKey   = -1;
    bool m_dirty         = false;

    // Keycode database
    std::vector<KeycodeDef>                     m_keycodeDb;
    std::unordered_map<std::string, KeycodeDef> m_keycodeMap;

    // Keycode picker state
    char m_searchBuf[128]    = {};
    int  m_selectedCategory  = -1;

    // Undo/redo stacks
    std::deque<UndoRecord> m_undoStack;
    std::deque<UndoRecord> m_redoStack;

    // ── Macros ────────────────────────────────────────────────────────────────
    // Rich macro list (replaces simple m_config.macros strings for editing)
    std::vector<MacroEntry>  m_macros;
    int                      m_editingMacro   = -1;
    int                      m_editingAction  = -1;
    char                     m_macroSearchBuf[64] = {};
    // For new-action temp state
    MacroActionType          m_newActionType  = MacroActionType::KeyTap;
    char                     m_newActionKey[64]  = "KC_A";
    char                     m_newActionText[256] = {};
    int                      m_newActionDelay = 50;

    // Sync m_macros → m_config.macros (serialize for save)
    void SyncMacrosToConfig();
    // Parse m_config.macros → m_macros on load
    void ParseMacrosFromConfig();
    // Human-readable summary of a MacroEntry
    std::string MacroSummary(const MacroEntry& m) const;

    // ── Key Tester ────────────────────────────────────────────────────────────
    KeyTester                  m_keyTester;
    float m_appStartTime = 0.f; // seconds, set in constructor

    // ── LED Schemes ───────────────────────────────────────────────────────────
    std::vector<LedScheme>  m_predefinedSchemes;

    void EnsureLayerSchemes();      // resize vectors to match layer count
    ImU32 KeyFaceColor(int ki) const;  // returns the draw color for key ki

    // ── Sub-panels ────────────────────────────────────────────────────────────
    void RenderLayerPanel();
    void RenderKeyboardPanel();
    void RenderKeycodePanel();
    void RenderMacroPanel();
    void RenderLedSchemePanel();
    void RenderStatusBar();

    // ── Keyboard visual ───────────────────────────────────────────────────────
    void DrawKeyboard(float originX, float originY, float keyUnit);

    // ── Helpers ───────────────────────────────────────────────────────────────
    void ApplyKeycode(int keyIdx, const std::string& code);
    void PushUndo(const UndoRecord& r);

    // Layer colors (for layer tab buttons, still used)
    static const int   kMaxLayers = 9;
    static const float kLayerColors[kMaxLayers][3];
};
