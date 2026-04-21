#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

enum class KeyCategory {
    Basic,
    Navigation,
    Modifiers,
    Function,
    Numpad,
    Media,
    Mouse,
    RGB,
    Macro,
    Layer,
    Special,
    Symbols,
    COUNT
};

inline const char* CategoryName(KeyCategory c) {
    switch (c) {
    case KeyCategory::Basic:      return "Basic";
    case KeyCategory::Navigation: return "Navigation";
    case KeyCategory::Modifiers:  return "Modifiers";
    case KeyCategory::Function:   return "Function";
    case KeyCategory::Numpad:     return "Numpad";
    case KeyCategory::Media:      return "Media";
    case KeyCategory::Mouse:      return "Mouse";
    case KeyCategory::RGB:        return "RGB";
    case KeyCategory::Macro:      return "Macro";
    case KeyCategory::Layer:      return "Layer";
    case KeyCategory::Special:    return "Special";
    case KeyCategory::Symbols:    return "Symbols";
    default:                      return "Unknown";
    }
}

struct KeycodeDef {
    std::string code;    // QMK keycode string e.g. "KC_A"
    std::string label;   // Short display label e.g. "A"
    std::string tooltip; // Full description
    KeyCategory category;
};

inline std::vector<KeycodeDef> BuildKeycodeDatabase() {
    return {
        // Special / No-op
        {"KC_NO",    "---",    "No key",              KeyCategory::Special},
        {"KC_TRNS",  "▽",     "Transparent (pass-through)", KeyCategory::Special},

        // Basic alphanumeric
        {"KC_A","A","A",KeyCategory::Basic},{"KC_B","B","B",KeyCategory::Basic},
        {"KC_C","C","C",KeyCategory::Basic},{"KC_D","D","D",KeyCategory::Basic},
        {"KC_E","E","E",KeyCategory::Basic},{"KC_F","F","F",KeyCategory::Basic},
        {"KC_G","G","G",KeyCategory::Basic},{"KC_H","H","H",KeyCategory::Basic},
        {"KC_I","I","I",KeyCategory::Basic},{"KC_J","J","J",KeyCategory::Basic},
        {"KC_K","K","K",KeyCategory::Basic},{"KC_L","L","L",KeyCategory::Basic},
        {"KC_M","M","M",KeyCategory::Basic},{"KC_N","N","N",KeyCategory::Basic},
        {"KC_O","O","O",KeyCategory::Basic},{"KC_P","P","P",KeyCategory::Basic},
        {"KC_Q","Q","Q",KeyCategory::Basic},{"KC_R","R","R",KeyCategory::Basic},
        {"KC_S","S","S",KeyCategory::Basic},{"KC_T","T","T",KeyCategory::Basic},
        {"KC_U","U","U",KeyCategory::Basic},{"KC_V","V","V",KeyCategory::Basic},
        {"KC_W","W","W",KeyCategory::Basic},{"KC_X","X","X",KeyCategory::Basic},
        {"KC_Y","Y","Y",KeyCategory::Basic},{"KC_Z","Z","Z",KeyCategory::Basic},
        {"KC_1","1","1",KeyCategory::Basic},{"KC_2","2","2",KeyCategory::Basic},
        {"KC_3","3","3",KeyCategory::Basic},{"KC_4","4","4",KeyCategory::Basic},
        {"KC_5","5","5",KeyCategory::Basic},{"KC_6","6","6",KeyCategory::Basic},
        {"KC_7","7","7",KeyCategory::Basic},{"KC_8","8","8",KeyCategory::Basic},
        {"KC_9","9","9",KeyCategory::Basic},{"KC_0","0","0",KeyCategory::Basic},
        {"KC_ENT","Ent","Enter",KeyCategory::Basic},
        {"KC_ESC","Esc","Escape",KeyCategory::Basic},
        {"KC_BSPC","⌫","Backspace",KeyCategory::Basic},
        {"KC_TAB","Tab","Tab",KeyCategory::Basic},
        {"KC_SPC","Spc","Space",KeyCategory::Basic},
        {"KC_MINS","-","Minus",KeyCategory::Basic},
        {"KC_EQL","=","Equal",KeyCategory::Basic},
        {"KC_LBRC","[","Left Bracket",KeyCategory::Basic},
        {"KC_RBRC","]","Right Bracket",KeyCategory::Basic},
        {"KC_BSLS","\\","Backslash",KeyCategory::Basic},
        {"KC_SCLN",";","Semicolon",KeyCategory::Basic},
        {"KC_QUOT","'","Quote",KeyCategory::Basic},
        {"KC_GRV","`","Grave",KeyCategory::Basic},
        {"KC_COMM",",","Comma",KeyCategory::Basic},
        {"KC_DOT",".","Dot",KeyCategory::Basic},
        {"KC_SLSH","/","Slash",KeyCategory::Basic},
        {"KC_CAPS","Cap","Caps Lock",KeyCategory::Basic},
        {"KC_DEL","Del","Delete",KeyCategory::Basic},
        {"KC_INS","Ins","Insert",KeyCategory::Basic},

        // Escape/Grave combo
        {"KC_GESC","Esc~","Escape or Grave (shifted)",KeyCategory::Special},

        // Modifiers
        {"KC_LCTL","LCtl","Left Control",KeyCategory::Modifiers},
        {"KC_LSFT","LSft","Left Shift",KeyCategory::Modifiers},
        {"KC_LALT","LAlt","Left Alt",KeyCategory::Modifiers},
        {"KC_LGUI","LGui","Left GUI/Win",KeyCategory::Modifiers},
        {"KC_RCTL","RCtl","Right Control",KeyCategory::Modifiers},
        {"KC_RSFT","RSft","Right Shift",KeyCategory::Modifiers},
        {"KC_RALT","RAlt","Right Alt / AltGr",KeyCategory::Modifiers},
        {"KC_RGUI","RGui","Right GUI/Win",KeyCategory::Modifiers},

        // Navigation
        {"KC_UP","↑","Up Arrow",KeyCategory::Navigation},
        {"KC_DOWN","↓","Down Arrow",KeyCategory::Navigation},
        {"KC_LEFT","←","Left Arrow",KeyCategory::Navigation},
        {"KC_RGHT","→","Right Arrow",KeyCategory::Navigation},
        {"KC_HOME","Home","Home",KeyCategory::Navigation},
        {"KC_END","End","End",KeyCategory::Navigation},
        {"KC_PGUP","PgUp","Page Up",KeyCategory::Navigation},
        {"KC_PGDN","PgDn","Page Down",KeyCategory::Navigation},

        // Function keys
        {"KC_F1","F1","F1",KeyCategory::Function},{"KC_F2","F2","F2",KeyCategory::Function},
        {"KC_F3","F3","F3",KeyCategory::Function},{"KC_F4","F4","F4",KeyCategory::Function},
        {"KC_F5","F5","F5",KeyCategory::Function},{"KC_F6","F6","F6",KeyCategory::Function},
        {"KC_F7","F7","F7",KeyCategory::Function},{"KC_F8","F8","F8",KeyCategory::Function},
        {"KC_F9","F9","F9",KeyCategory::Function},{"KC_F10","F10","F10",KeyCategory::Function},
        {"KC_F11","F11","F11",KeyCategory::Function},{"KC_F12","F12","F12",KeyCategory::Function},
        {"KC_F13","F13","F13",KeyCategory::Function},{"KC_F14","F14","F14",KeyCategory::Function},
        {"KC_F15","F15","F15",KeyCategory::Function},{"KC_F16","F16","F16",KeyCategory::Function},

        // Numpad
        {"KC_PSLS","N/","Numpad Divide",KeyCategory::Numpad},
        {"KC_PAST","N*","Numpad Multiply",KeyCategory::Numpad},
        {"KC_PMNS","N-","Numpad Minus",KeyCategory::Numpad},
        {"KC_PPLS","N+","Numpad Plus",KeyCategory::Numpad},
        {"KC_PENT","N↵","Numpad Enter",KeyCategory::Numpad},
        {"KC_P1","N1","Numpad 1",KeyCategory::Numpad},{"KC_P2","N2","Numpad 2",KeyCategory::Numpad},
        {"KC_P3","N3","Numpad 3",KeyCategory::Numpad},{"KC_P4","N4","Numpad 4",KeyCategory::Numpad},
        {"KC_P5","N5","Numpad 5",KeyCategory::Numpad},{"KC_P6","N6","Numpad 6",KeyCategory::Numpad},
        {"KC_P7","N7","Numpad 7",KeyCategory::Numpad},{"KC_P8","N8","Numpad 8",KeyCategory::Numpad},
        {"KC_P9","N9","Numpad 9",KeyCategory::Numpad},{"KC_P0","N0","Numpad 0",KeyCategory::Numpad},
        {"KC_PDOT","N.","Numpad Dot",KeyCategory::Numpad},
        {"KC_NLCK","NLk","Num Lock",KeyCategory::Numpad},

        // Media
        {"KC_MPLY","⏯","Media Play/Pause",KeyCategory::Media},
        {"KC_MSTP","⏹","Media Stop",KeyCategory::Media},
        {"KC_MFFD","⏭","Media Fast Forward / Next",KeyCategory::Media},
        {"KC_MRWD","⏮","Media Rewind / Prev",KeyCategory::Media},
        {"KC_MNXT","⏭","Media Next Track",KeyCategory::Media},
        {"KC_MPRV","⏮","Media Previous Track",KeyCategory::Media},
        {"KC_MUTE","🔇","Mute",KeyCategory::Media},
        {"KC_VOLU","🔊","Volume Up",KeyCategory::Media},
        {"KC_VOLD","🔉","Volume Down",KeyCategory::Media},
        {"KC_BRIU","🔆","Brightness Up",KeyCategory::Media},
        {"KC_BRID","🔅","Brightness Down",KeyCategory::Media},
        {"KC_EJCT","⏏","Eject",KeyCategory::Media},

        // Mouse
        {"KC_MS_UP","M↑","Mouse Move Up",KeyCategory::Mouse},
        {"KC_MS_DOWN","M↓","Mouse Move Down",KeyCategory::Mouse},
        {"KC_MS_LEFT","M←","Mouse Move Left",KeyCategory::Mouse},
        {"KC_MS_RIGHT","M→","Mouse Move Right",KeyCategory::Mouse},
        {"KC_MS_BTN1","MB1","Mouse Button 1 (Left)",KeyCategory::Mouse},
        {"KC_MS_BTN2","MB2","Mouse Button 2 (Right)",KeyCategory::Mouse},
        {"KC_MS_BTN3","MB3","Mouse Button 3 (Middle)",KeyCategory::Mouse},
        {"KC_MS_BTN4","MB4","Mouse Button 4",KeyCategory::Mouse},
        {"KC_MS_BTN5","MB5","Mouse Button 5",KeyCategory::Mouse},
        {"KC_MS_WH_UP","WH↑","Mouse Wheel Up",KeyCategory::Mouse},
        {"KC_MS_WH_DOWN","WH↓","Mouse Wheel Down",KeyCategory::Mouse},
        {"KC_MS_WH_LEFT","WH←","Mouse Wheel Left",KeyCategory::Mouse},
        {"KC_MS_WH_RIGHT","WH→","Mouse Wheel Right",KeyCategory::Mouse},
        {"KC_MS_ACCEL0","MA0","Mouse Accel 0",KeyCategory::Mouse},
        {"KC_MS_ACCEL1","MA1","Mouse Accel 1",KeyCategory::Mouse},
        {"KC_MS_ACCEL2","MA2","Mouse Accel 2",KeyCategory::Mouse},

        // RGB
        {"RGB_TOG","RGB","RGB Toggle",KeyCategory::RGB},
        {"RGB_MOD","R+M","RGB Mode Next",KeyCategory::RGB},
        {"RGB_RMOD","R-M","RGB Mode Prev",KeyCategory::RGB},
        {"RGB_HUI","HU+","Hue Increase",KeyCategory::RGB},
        {"RGB_HUD","HU-","Hue Decrease",KeyCategory::RGB},
        {"RGB_SAI","SA+","Saturation Increase",KeyCategory::RGB},
        {"RGB_SAD","SA-","Saturation Decrease",KeyCategory::RGB},
        {"RGB_VAI","VA+","Value/Brightness Increase",KeyCategory::RGB},
        {"RGB_VAD","VA-","Value/Brightness Decrease",KeyCategory::RGB},
        {"RGB_SPI","SP+","Speed Increase",KeyCategory::RGB},
        {"RGB_SPD","SP-","Speed Decrease",KeyCategory::RGB},

        // Symbols (shifted keys)
        {"KC_EXLM","!","!",KeyCategory::Symbols},
        {"KC_AT","@","@",KeyCategory::Symbols},
        {"KC_HASH","#","#",KeyCategory::Symbols},
        {"KC_DLR","$","$",KeyCategory::Symbols},
        {"KC_PERC","%","%",KeyCategory::Symbols},
        {"KC_CIRC","^","^",KeyCategory::Symbols},
        {"KC_AMPR","&","&",KeyCategory::Symbols},
        {"KC_ASTR","*","*",KeyCategory::Symbols},
        {"KC_LPRN","(","(",KeyCategory::Symbols},
        {"KC_RPRN",")",")",KeyCategory::Symbols},
        {"KC_UNDS","_","_",KeyCategory::Symbols},
        {"KC_PLUS","+","+",KeyCategory::Symbols},
        {"KC_LCBR","{","{",KeyCategory::Symbols},
        {"KC_RCBR","}","}",KeyCategory::Symbols},
        {"KC_PIPE","|","|",KeyCategory::Symbols},
        {"KC_COLN",":",": ",KeyCategory::Symbols},
        {"KC_DQUO","\"","\"",KeyCategory::Symbols},
        {"KC_TILD","~","~",KeyCategory::Symbols},
        {"KC_LT","<","<",KeyCategory::Symbols},
        {"KC_GT",">",">",KeyCategory::Symbols},
        {"KC_QUES","?","?",KeyCategory::Symbols},

        // Edit
        {"KC_UNDO","Undo","Undo",KeyCategory::Special},
        {"KC_REDO","Redo","Redo",KeyCategory::Special},
        {"KC_CUT","Cut","Cut",KeyCategory::Special},
        {"KC_COPY","Copy","Copy",KeyCategory::Special},
        {"KC_PASTE","Paste","Paste",KeyCategory::Special},
        {"KC_SELECT","Sel","Select All",KeyCategory::Special},
        {"KC_FIND","Find","Find",KeyCategory::Special},
        {"KC_PSCR","PrtSc","Print Screen",KeyCategory::Special},
        {"KC_SLCK","SLck","Scroll Lock",KeyCategory::Special},
        {"KC_PAUS","Pause","Pause/Break",KeyCategory::Special},
        {"KC_APP","Menu","Application Menu",KeyCategory::Special},

        // WWW
        {"KC_WWW_BACK","Web←","Browser Back",KeyCategory::Special},
        {"KC_WWW_FWRD","Web→","Browser Forward",KeyCategory::Special},
        {"KC_WWW_HOME","Web⌂","Browser Home",KeyCategory::Special},
        {"KC_WWW_RFSH","Web↺","Browser Refresh",KeyCategory::Special},
    };
}

// Build a lookup map from code -> KeycodeDef
inline std::unordered_map<std::string, KeycodeDef> BuildKeycodeMap() {
    std::unordered_map<std::string, KeycodeDef> m;
    for (auto& def : BuildKeycodeDatabase()) {
        m[def.code] = def;
    }
    return m;
}

// Get a short display label for any keycode string (including dynamic ones like TO(1), MACRO00)
inline std::string GetKeycodeLabel(const std::string& code,
    const std::unordered_map<std::string, KeycodeDef>& db)
{
    auto it = db.find(code);
    if (it != db.end()) return it->second.label;

    // Dynamic patterns
    if (code.rfind("TO(", 0) == 0)   return "L>" + code.substr(3, code.size()-4);
    if (code.rfind("MO(", 0) == 0)   return "L+" + code.substr(3, code.size()-4);
    if (code.rfind("TG(", 0) == 0)   return "LT" + code.substr(3, code.size()-4);
    if (code.rfind("MACRO", 0) == 0) return "M"  + code.substr(5);

    // Fallback: strip KC_ prefix and shorten
    if (code.rfind("KC_", 0) == 0) {
        std::string s = code.substr(3);
        if (s.size() > 6) s = s.substr(0, 6);
        return s;
    }
    return code.size() > 6 ? code.substr(0, 6) : code;
}
