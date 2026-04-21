#pragma once
// MacroLibrary.h  ─  200 predefined macro actions (self-contained)
// ─────────────────────────────────────────────────────────────────
// Deliberately has ZERO dependency on KeyboardEditor.h / MacroAction.
// The library stores raw string data; the editor converts to MacroAction
// at insert time inside RenderMacroLibraryPanel().

#include <string>
#include <vector>
#include <cctype>   // tolower

// ─── Own action-type enum (mirrors MacroActionType, no linkage to it) ─────────

enum class LibActionType {
    KeyTap,
    KeyDown,
    KeyUp,
    TypeString,
    Delay
};

inline const char* LibActionTypeName(LibActionType t) {
    switch (t) {
    case LibActionType::KeyTap:     return "Key Tap";
    case LibActionType::KeyDown:    return "Key Down";
    case LibActionType::KeyUp:      return "Key Up";
    case LibActionType::TypeString: return "Type String";
    case LibActionType::Delay:      return "Delay";
    default:                        return "?";
    }
}

// ─── Library entry ────────────────────────────────────────────────────────────

struct MacroLibraryEntry {
    std::string   category;
    std::string   name;
    std::string   description;
    LibActionType libType  = LibActionType::KeyTap;
    std::string   keycode;    // KeyTap / KeyDown / KeyUp
    std::string   text;       // TypeString
    int           delayMs = 50; // Delay
};

// ─── Builder helpers (inline so the header stays header-only) ────────────────

inline MacroLibraryEntry LTap(const char* cat, const char* name,
                               const char* desc, const char* kc) {
    MacroLibraryEntry e;
    e.category = cat; e.name = name; e.description = desc;
    e.libType = LibActionType::KeyTap; e.keycode = kc;
    return e;
}
inline MacroLibraryEntry LDown(const char* cat, const char* name,
                                const char* desc, const char* kc) {
    MacroLibraryEntry e;
    e.category = cat; e.name = name; e.description = desc;
    e.libType = LibActionType::KeyDown; e.keycode = kc;
    return e;
}
inline MacroLibraryEntry LUp(const char* cat, const char* name,
                              const char* desc, const char* kc) {
    MacroLibraryEntry e;
    e.category = cat; e.name = name; e.description = desc;
    e.libType = LibActionType::KeyUp; e.keycode = kc;
    return e;
}
inline MacroLibraryEntry LType(const char* cat, const char* name,
                                const char* desc, const char* text) {
    MacroLibraryEntry e;
    e.category = cat; e.name = name; e.description = desc;
    e.libType = LibActionType::TypeString; e.text = text;
    return e;
}
inline MacroLibraryEntry LDelay(const char* cat, const char* name,
                                 const char* desc, int ms) {
    MacroLibraryEntry e;
    e.category = cat; e.name = name; e.description = desc;
    e.libType = LibActionType::Delay; e.delayMs = ms;
    return e;
}

// ─── 200-entry library ────────────────────────────────────────────────────────

inline std::vector<MacroLibraryEntry> BuildMacroLibrary() {
    return {
        // ── Basic Keys (20) ──────────────────────────────────────────────────
        LTap("Basic Keys","Enter",        "Tap Enter",                       "KC_ENT"),
        LTap("Basic Keys","Escape",       "Tap Escape",                      "KC_ESC"),
        LTap("Basic Keys","Tab",          "Tap Tab",                         "KC_TAB"),
        LTap("Basic Keys","Space",        "Tap Space",                       "KC_SPC"),
        LTap("Basic Keys","Backspace",    "Tap Backspace",                   "KC_BSPC"),
        LTap("Basic Keys","Delete",       "Tap Delete",                      "KC_DEL"),
        LTap("Basic Keys","Insert",       "Tap Insert",                      "KC_INS"),
        LTap("Basic Keys","Caps Lock",    "Toggle Caps Lock",                "KC_CAPS"),
        LTap("Basic Keys","Print Screen", "Tap Print Screen",                "KC_PSCR"),
        LTap("Basic Keys","Scroll Lock",  "Tap Scroll Lock",                 "KC_SLCK"),
        LTap("Basic Keys","Pause/Break",  "Tap Pause",                       "KC_PAUS"),
        LTap("Basic Keys","Num Lock",     "Toggle Num Lock",                 "KC_NLCK"),
        LTap("Basic Keys","Home",         "Tap Home",                        "KC_HOME"),
        LTap("Basic Keys","End",          "Tap End",                         "KC_END"),
        LTap("Basic Keys","Page Up",      "Tap Page Up",                     "KC_PGUP"),
        LTap("Basic Keys","Page Down",    "Tap Page Down",                   "KC_PGDN"),
        LTap("Basic Keys","Arrow Up",     "Tap Up Arrow",                    "KC_UP"),
        LTap("Basic Keys","Arrow Down",   "Tap Down Arrow",                  "KC_DOWN"),
        LTap("Basic Keys","Arrow Left",   "Tap Left Arrow",                  "KC_LEFT"),
        LTap("Basic Keys","Arrow Right",  "Tap Right Arrow",                 "KC_RGHT"),

        // ── Modifiers (12) ────────────────────────────────────────────────────
        LDown("Modifiers","Hold L-Ctrl",      "Press and hold Left Ctrl",    "KC_LCTL"),
        LUp  ("Modifiers","Release L-Ctrl",   "Release Left Ctrl",           "KC_LCTL"),
        LDown("Modifiers","Hold L-Shift",     "Press and hold Left Shift",   "KC_LSFT"),
        LUp  ("Modifiers","Release L-Shift",  "Release Left Shift",          "KC_LSFT"),
        LDown("Modifiers","Hold L-Alt",       "Press and hold Left Alt",     "KC_LALT"),
        LUp  ("Modifiers","Release L-Alt",    "Release Left Alt",            "KC_LALT"),
        LDown("Modifiers","Hold L-GUI",       "Press and hold Left Win/Cmd", "KC_LGUI"),
        LUp  ("Modifiers","Release L-GUI",    "Release Left Win/Cmd",        "KC_LGUI"),
        LDown("Modifiers","Hold R-Ctrl",      "Press and hold Right Ctrl",   "KC_RCTL"),
        LUp  ("Modifiers","Release R-Ctrl",   "Release Right Ctrl",          "KC_RCTL"),
        LDown("Modifiers","Hold R-Shift",     "Press and hold Right Shift",  "KC_RSFT"),
        LUp  ("Modifiers","Release R-Shift",  "Release Right Shift",         "KC_RSFT"),

        // ── System (20) ───────────────────────────────────────────────────────
        LTap("System","Copy",        "Copy selection",          "KC_COPY"),
        LTap("System","Cut",         "Cut selection",           "KC_CUT"),
        LTap("System","Paste",       "Paste",                   "KC_PASTE"),
        LTap("System","Undo",        "Undo",                    "KC_UNDO"),
        LTap("System","Redo",        "Redo",                    "KC_REDO"),
        LTap("System","Select All",  "Select all",              "KC_SELECT"),
        LTap("System","Find",        "Find",                    "KC_FIND"),
        LTap("System","Save",        "Save (KC_S)",             "KC_S"),
        LTap("System","Print",       "Print (KC_P)",            "KC_P"),
        LTap("System","New",         "New document",            "KC_N"),
        LTap("System","Close",       "Close tab / window",      "KC_W"),
        LTap("System","New Tab",     "Open new tab",            "KC_T"),
        LTap("System","Alt+F4 key",  "Close window",            "KC_F4"),
        LTap("System","Alt+Tab key", "Switch app",              "KC_TAB"),
        LTap("System","Win+D",       "Show desktop",            "KC_D"),
        LTap("System","Win+L",       "Lock screen",             "KC_L"),
        LTap("System","Win+E",       "Open Explorer",           "KC_E"),
        LTap("System","Win+R",       "Open Run dialog",         "KC_R"),
        LTap("System","Win+I",       "Open Settings",           "KC_I"),
        LTap("System","App Menu",    "Context menu key",        "KC_APP"),

        // ── Media (14) ────────────────────────────────────────────────────────
        LTap("Media","Play/Pause",     "Toggle media playback",           "KC_MPLY"),
        LTap("Media","Stop",           "Stop media",                      "KC_MSTP"),
        LTap("Media","Next Track",     "Skip to next track",              "KC_MNXT"),
        LTap("Media","Prev Track",     "Go to previous track",            "KC_MPRV"),
        LTap("Media","Fast Forward",   "Fast-forward",                    "KC_MFFD"),
        LTap("Media","Rewind",         "Rewind",                          "KC_MRWD"),
        LTap("Media","Mute",           "Toggle mute",                     "KC_MUTE"),
        LTap("Media","Volume Up",      "Increase volume",                 "KC_VOLU"),
        LTap("Media","Volume Down",    "Decrease volume",                 "KC_VOLD"),
        LTap("Media","Brightness Up",  "Increase brightness",             "KC_BRIU"),
        LTap("Media","Brightness Down","Decrease brightness",             "KC_BRID"),
        LTap("Media","Eject",          "Eject media",                     "KC_EJCT"),
        LDelay("Media","Short 100ms",  "Pause between media commands",    100),
        LDelay("Media","Long 500ms",   "Longer gap between media actions",500),

        // ── Browser (10) ──────────────────────────────────────────────────────
        LTap("Browser","Back",         "Browser go back",    "KC_WWW_BACK"),
        LTap("Browser","Forward",      "Browser go forward", "KC_WWW_FWRD"),
        LTap("Browser","Home",         "Browser home page",  "KC_WWW_HOME"),
        LTap("Browser","Refresh",      "Refresh page",       "KC_WWW_RFSH"),
        LTap("Browser","New Tab",      "Ctrl+T",             "KC_T"),
        LTap("Browser","Close Tab",    "Ctrl+W",             "KC_W"),
        LTap("Browser","Reopen Tab",   "Ctrl+Shift+T",       "KC_T"),
        LTap("Browser","Address Bar",  "Ctrl+L / F6",        "KC_L"),
        LTap("Browser","Bookmark",     "Ctrl+D",             "KC_D"),
        LTap("Browser","Find in Page", "Ctrl+F",             "KC_F"),

        // ── Function Keys (16) ────────────────────────────────────────────────
        LTap("Function Keys","F1", "F1",            "KC_F1"),
        LTap("Function Keys","F2", "F2",            "KC_F2"),
        LTap("Function Keys","F3", "F3",            "KC_F3"),
        LTap("Function Keys","F4", "F4",            "KC_F4"),
        LTap("Function Keys","F5", "F5 Refresh",    "KC_F5"),
        LTap("Function Keys","F6", "F6",            "KC_F6"),
        LTap("Function Keys","F7", "F7",            "KC_F7"),
        LTap("Function Keys","F8", "F8",            "KC_F8"),
        LTap("Function Keys","F9", "F9",            "KC_F9"),
        LTap("Function Keys","F10","F10",           "KC_F10"),
        LTap("Function Keys","F11","F11 Fullscreen","KC_F11"),
        LTap("Function Keys","F12","F12 Dev Tools", "KC_F12"),
        LTap("Function Keys","F13","F13",           "KC_F13"),
        LTap("Function Keys","F14","F14",           "KC_F14"),
        LTap("Function Keys","F15","F15",           "KC_F15"),
        LTap("Function Keys","F16","F16",           "KC_F16"),

        // ── Numpad (15) ───────────────────────────────────────────────────────
        LTap("Numpad","Num 0","Numpad 0",       "KC_P0"),
        LTap("Numpad","Num 1","Numpad 1",       "KC_P1"),
        LTap("Numpad","Num 2","Numpad 2",       "KC_P2"),
        LTap("Numpad","Num 3","Numpad 3",       "KC_P3"),
        LTap("Numpad","Num 4","Numpad 4",       "KC_P4"),
        LTap("Numpad","Num 5","Numpad 5",       "KC_P5"),
        LTap("Numpad","Num 6","Numpad 6",       "KC_P6"),
        LTap("Numpad","Num 7","Numpad 7",       "KC_P7"),
        LTap("Numpad","Num 8","Numpad 8",       "KC_P8"),
        LTap("Numpad","Num 9","Numpad 9",       "KC_P9"),
        LTap("Numpad","Num +","Numpad Plus",    "KC_PPLS"),
        LTap("Numpad","Num -","Numpad Minus",   "KC_PMNS"),
        LTap("Numpad","Num *","Numpad Multiply","KC_PAST"),
        LTap("Numpad","Num /","Numpad Divide",  "KC_PSLS"),
        LTap("Numpad","Num Enter","Numpad Enter","KC_PENT"),

        // ── Mouse (16) ────────────────────────────────────────────────────────
        LTap("Mouse","Left Click",   "Mouse button 1",         "KC_MS_BTN1"),
        LTap("Mouse","Right Click",  "Mouse button 2",         "KC_MS_BTN2"),
        LTap("Mouse","Middle Click", "Mouse button 3",         "KC_MS_BTN3"),
        LTap("Mouse","Button 4",     "Mouse button 4 back",    "KC_MS_BTN4"),
        LTap("Mouse","Button 5",     "Mouse button 5 forward", "KC_MS_BTN5"),
        LTap("Mouse","Move Up",      "Mouse cursor up",        "KC_MS_UP"),
        LTap("Mouse","Move Down",    "Mouse cursor down",      "KC_MS_DOWN"),
        LTap("Mouse","Move Left",    "Mouse cursor left",      "KC_MS_LEFT"),
        LTap("Mouse","Move Right",   "Mouse cursor right",     "KC_MS_RIGHT"),
        LTap("Mouse","Scroll Up",    "Scroll wheel up",        "KC_MS_WH_UP"),
        LTap("Mouse","Scroll Down",  "Scroll wheel down",      "KC_MS_WH_DOWN"),
        LTap("Mouse","Scroll Left",  "Scroll wheel left",      "KC_MS_WH_LEFT"),
        LTap("Mouse","Scroll Right", "Scroll wheel right",     "KC_MS_WH_RIGHT"),
        LTap("Mouse","Accel 0",      "Mouse accel 0 slow",     "KC_MS_ACCEL0"),
        LTap("Mouse","Accel 1",      "Mouse accel 1",          "KC_MS_ACCEL1"),
        LTap("Mouse","Accel 2",      "Mouse accel 2 fast",     "KC_MS_ACCEL2"),

        // ── RGB (11) ──────────────────────────────────────────────────────────
        LTap("RGB","Toggle RGB",   "Turn RGB on / off",          "RGB_TOG"),
        LTap("RGB","Next Mode",    "Cycle to next RGB mode",      "RGB_MOD"),
        LTap("RGB","Prev Mode",    "Cycle to previous RGB mode",  "RGB_RMOD"),
        LTap("RGB","Hue +",        "Increase hue",                "RGB_HUI"),
        LTap("RGB","Hue -",        "Decrease hue",                "RGB_HUD"),
        LTap("RGB","Saturation +", "Increase saturation",         "RGB_SAI"),
        LTap("RGB","Saturation -", "Decrease saturation",         "RGB_SAD"),
        LTap("RGB","Brightness +", "Increase brightness",         "RGB_VAI"),
        LTap("RGB","Brightness -", "Decrease brightness",         "RGB_VAD"),
        LTap("RGB","Speed +",      "Increase animation speed",    "RGB_SPI"),
        LTap("RGB","Speed -",      "Decrease animation speed",    "RGB_SPD"),

        // ── Delays (8) ────────────────────────────────────────────────────────
        LDelay("Delays","10 ms",   "Very short pause",    10),
        LDelay("Delays","25 ms",   "Short pause",         25),
        LDelay("Delays","50 ms",   "Inter-key gap",       50),
        LDelay("Delays","100 ms",  "100ms pause",        100),
        LDelay("Delays","200 ms",  "200ms pause",        200),
        LDelay("Delays","500 ms",  "Half-second",        500),
        LDelay("Delays","1000 ms", "One second",        1000),
        LDelay("Delays","2000 ms", "Two seconds",       2000),

        // ── Text Templates (10) ───────────────────────────────────────────────
        LType("Text Templates","My email",           "Email address placeholder", "your@email.com"),
        LType("Text Templates","Thank you",          "Polite closing",            "Thank you,\n"),
        LType("Text Templates","Best regards",       "Email sign-off",            "Best regards,\n"),
        LType("Text Templates","Hello",              "Greeting",                  "Hello,\n"),
        LType("Text Templates","Please find attach", "Email preamble",            "Please find attached "),
        LType("Text Templates","LGTM",               "Code review approval",      "LGTM"),
        LType("Text Templates","WIP",                "Work-in-progress tag",      "WIP: "),
        LType("Text Templates","TODO",               "Code TODO comment",         "// TODO: "),
        LType("Text Templates","FIXME",              "Code FIXME comment",        "// FIXME: "),
        LType("Text Templates","HACK",               "Code HACK comment",         "// HACK: "),

        // ── Shell / CLI (20) ──────────────────────────────────────────────────
        LType("Shell / CLI","git status",    "Check repo status",    "git status"),
        LType("Shell / CLI","git add -A",    "Stage all changes",    "git add -A"),
        LType("Shell / CLI","git commit",    "Start a commit",       "git commit -m \""),
        LType("Shell / CLI","git push",      "Push to origin",       "git push"),
        LType("Shell / CLI","git pull",      "Pull from origin",     "git pull"),
        LType("Shell / CLI","git log",       "Short log",            "git log --oneline -20"),
        LType("Shell / CLI","git diff",      "Show unstaged diff",   "git diff"),
        LType("Shell / CLI","npm install",   "Install dependencies", "npm install"),
        LType("Shell / CLI","npm run dev",   "Start dev server",     "npm run dev"),
        LType("Shell / CLI","npm run build", "Build project",        "npm run build"),
        LType("Shell / CLI","pip install",   "Python install pkg",   "pip install "),
        LType("Shell / CLI","python venv",   "Create virtualenv",    "python -m venv .venv"),
        LType("Shell / CLI","docker ps",     "List containers",      "docker ps -a"),
        LType("Shell / CLI","docker up",     "Start compose stack",  "docker compose up -d"),
        LType("Shell / CLI","ls -la",        "List files detailed",  "ls -la"),
        LType("Shell / CLI","cd ..",         "Go up one directory",  "cd .."),
        LType("Shell / CLI","clear",         "Clear terminal",       "clear"),
        LType("Shell / CLI","grep -r",       "Recursive grep",       "grep -r \"\" ."),
        LType("Shell / CLI","ssh",           "Start SSH command",    "ssh "),
        LType("Shell / CLI","cat",           "Start cat command",    "cat "),

        // ── Layers (9) ────────────────────────────────────────────────────────
        LTap("Layers","Go Layer 0","Switch to layer 0",         "TO(0)"),
        LTap("Layers","Go Layer 1","Switch to layer 1",         "TO(1)"),
        LTap("Layers","Go Layer 2","Switch to layer 2",         "TO(2)"),
        LTap("Layers","Go Layer 3","Switch to layer 3",         "TO(3)"),
        LTap("Layers","MO Layer 1","Momentarily activate layer 1","MO(1)"),
        LTap("Layers","MO Layer 2","Momentarily activate layer 2","MO(2)"),
        LTap("Layers","TG Layer 1","Toggle layer 1",            "TG(1)"),
        LTap("Layers","TG Layer 2","Toggle layer 2",            "TG(2)"),
        LTap("Layers","TG Layer 3","Toggle layer 3",            "TG(3)"),

        // ── Symbols (20) ─────────────────────────────────────────────────────
        LTap("Symbols","!","Exclamation", "KC_EXLM"),
        LTap("Symbols","@","At sign",     "KC_AT"),
        LTap("Symbols","#","Hash",        "KC_HASH"),
        LTap("Symbols","$","Dollar",      "KC_DLR"),
        LTap("Symbols","%","Percent",     "KC_PERC"),
        LTap("Symbols","^","Caret",       "KC_CIRC"),
        LTap("Symbols","&","Ampersand",   "KC_AMPR"),
        LTap("Symbols","*","Asterisk",    "KC_ASTR"),
        LTap("Symbols","(","Left paren",  "KC_LPRN"),
        LTap("Symbols",")","Right paren", "KC_RPRN"),
        LTap("Symbols","_","Underscore",  "KC_UNDS"),
        LTap("Symbols","+","Plus",        "KC_PLUS"),
        LTap("Symbols","{","Left brace",  "KC_LCBR"),
        LTap("Symbols","}","Right brace", "KC_RCBR"),
        LTap("Symbols","|","Pipe",        "KC_PIPE"),
        LTap("Symbols",":","Colon",       "KC_COLN"),
        LTap("Symbols","\"","Double quote","KC_DQUO"),
        LTap("Symbols","~","Tilde",       "KC_TILD"),
        LTap("Symbols","<","Less-than",   "KC_LT"),
        LTap("Symbols",">","Greater-than","KC_GT"),

        // ── Special / QMK (9) ─────────────────────────────────────────────────
        LTap("Special","No-op",       "Do nothing",                      "KC_NO"),
        LTap("Special","Transparent", "Pass through to lower layer",     "KC_TRNS"),
        LTap("Special","Esc/Grave",   "Esc normally, grave when shifted","KC_GESC"),
        LTap("Special","Undo app",    "App-level Undo",                  "KC_UNDO"),
        LTap("Special","Redo app",    "App-level Redo",                  "KC_REDO"),
        LTap("Special","Cut app",     "App-level Cut",                   "KC_CUT"),
        LTap("Special","Copy app",    "App-level Copy",                  "KC_COPY"),
        LTap("Special","Paste app",   "App-level Paste",                 "KC_PASTE"),
        LTap("Special","Select All",  "App-level Select-All",            "KC_SELECT"),

        // ── Code Snippets (10) ────────────────────────────────────────────────
        LType("Code Snippets","Arrow =>",     "JS arrow function",   "=> "),
        LType("Code Snippets","Closure ()=>", "JS closure",          "() => "),
        LType("Code Snippets","async fn",     "Async function",      "async function "),
        LType("Code Snippets","console.log",  "Debug print",         "console.log()"),
        LType("Code Snippets","printf",        "C printf",            "printf(\"%s\\n\", );"),
        LType("Code Snippets","std::cout",    "C++ stream",          "std::cout << "),
        LType("Code Snippets","#include",     "C/C++ include",       "#include <"),
        LType("Code Snippets","#pragma once", "Header guard",        "#pragma once\n"),
        LType("Code Snippets","namespace",    "C++ namespace block", "namespace  {\n\n}\n"),
        LType("Code Snippets","return early", "Guard clause",        "if () return;\n"),
    };
}

// ─── Filter helpers ───────────────────────────────────────────────────────────

inline std::vector<std::string> GetLibraryCategories(
    const std::vector<MacroLibraryEntry>& lib)
{
    std::vector<std::string> cats;
    for (size_t i = 0; i < lib.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < cats.size(); ++j)
            if (cats[j] == lib[i].category) { found = true; break; }
        if (!found) cats.push_back(lib[i].category);
    }
    return cats;
}

inline std::vector<const MacroLibraryEntry*> FilterLibrary(
    const std::vector<MacroLibraryEntry>& lib,
    const std::string& category,
    const std::string& search)
{
    std::vector<const MacroLibraryEntry*> out;
    for (size_t i = 0; i < lib.size(); ++i) {
        const MacroLibraryEntry& e = lib[i];
        if (!category.empty() && e.category != category) continue;
        if (!search.empty()) {
            std::string haystack = e.name + " " + e.description;
            std::string needle   = search;
            // manual tolower — avoids <algorithm> include complications
            for (size_t k = 0; k < haystack.size(); ++k)
                haystack[k] = (char)tolower((unsigned char)haystack[k]);
            for (size_t k = 0; k < needle.size(); ++k)
                needle[k] = (char)tolower((unsigned char)needle[k]);
            if (haystack.find(needle) == std::string::npos) continue;
        }
        out.push_back(&e);
    }
    return out;
}
