#include "Keytest.h"
#include "SessionState.h"
#include "KeyboardEditor.h"
#include "MacroLibrary.h"
#include "AdminHelper.h"
#include "doio.h"
#include "Keytest.h"
#include "imgui.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <sstream>
#pragma warning(disable: 6262)  // large stack frame — acceptable for ImGui render functions


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
KeyboardEditor::~KeyboardEditor() {
    if (m_mapThread.joinable()) {
        // Give it a moment to finish
        for (int i = 0; i < 50 && m_mapThreadRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (m_mapThread.joinable()) {
            m_mapThread.detach();  // Detach to avoid blocking shutdown
        }
    }
}

void KeyboardEditor::LaunchMapExeAsync(const std::string& args) {
    // Join any previous finished thread before launching a new one
    if (m_mapThread.joinable()) {
        m_mapThread.join();
    }

    // Reset state
    m_mapStatus = MapExeStatus::Running;
    m_mapStatusMsg.clear();
    m_mapResultStatus = MapExeStatus::Idle;
    m_mapResultMsg.clear();
    m_showMapResult = true;
    m_mapThreadRunning = true;

    m_mapThread = std::thread([this, args]() {
        std::string msg;
        int exitCode = RunMapExe(args, msg);

        std::lock_guard<std::mutex> lk(m_mapMutex);
        if (exitCode == 0) {
            m_mapResultStatus = MapExeStatus::Success;
        }
        else {
            m_mapResultStatus = MapExeStatus::Failed;
            // Append exit code to message if not already there
            if (msg.find("exit code") == std::string::npos) {
                msg += "\n(map.exe exit code: " + std::to_string(exitCode) + ")";
            }
        }
        m_mapResultMsg = msg;
        m_mapThreadRunning = false;
        });
}

void KeyboardEditor::PollMapResult() {
    if (m_mapStatus != MapExeStatus::Running) return;

    // Check if thread is still running
    if (m_mapThreadRunning) {
        // Still running - nothing to do yet
        return;
    }

    // Thread has finished - get the result
    std::lock_guard<std::mutex> lk(m_mapMutex);
    if (m_mapResultStatus != MapExeStatus::Idle) {
        m_mapStatus = m_mapResultStatus;
        m_mapStatusMsg = m_mapResultMsg;
        // Reset for next operation
        m_mapResultStatus = MapExeStatus::Idle;
        m_mapResultMsg.clear();

        // Force popup to show if not already
        if (!m_showMapResult) {
            m_showMapResult = true;
        }
    }
}


static std::string OpenFileDialog(const char* title, const char* filter) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return buf;
    return {};
}




static std::string SaveFileDialog(const char* title, const char* filter,
    const char* defaultExt) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) return buf;
    return {};
}

// ─── VK name lookup ──────────────────────────────────────────────────────────

KeyboardEditor::KeyboardEditor() {
    m_keycodeDb = BuildKeycodeDatabase();
    m_keycodeMap = BuildKeycodeMap();
    m_predefinedSchemes = BuildPredefinedSchemes();
    m_vkDown.assign(256, false);
    m_appStartTime = (float)ImGui::GetTime();

    // ── Build macro library ──────────────────────────────────────────────────
    m_macroLib = BuildMacroLibrary();
    m_libCategories.push_back("All");   // index 0 = show everything
    for (const auto& c : GetLibraryCategories(m_macroLib))
        m_libCategories.push_back(c);

    // ── Auto-load last session ───────────────────────────────────────────────
    if (m_session.Load()) {
        TryAutoLoad(m_session.designPath, m_session.configPath);
    }
}


void KeyboardEditor::ReadKeyboardLive() {
    std::string mapExe = MapExePath();
    if (mapExe.empty()) {
        m_mapStatus = MapExeStatus::NoMapExe;
        m_showMapResult = true;
        return;
    }
    if (m_designPath.empty()) {
        m_mapStatus = MapExeStatus::NoDesign;
        m_showMapResult = true;
        return;
    }

    // Generate temp file for live read
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string tempFile = std::string(tempPath) + "doio_live_read.json";

    std::string args = "\"" + m_designPath + "\" --dump \"" + tempFile + "\"";

    // Check for admin
    if (!IsRunningAsAdmin()) {
        // Ask user to restart as admin
        int result = MessageBoxA(nullptr,
            "Flashing requires Administrator privileges.\n\n"
            "Click OK to restart the editor as Administrator,\n"
            "or Cancel to continue without flash capability.",
            "Admin Required", MB_OKCANCEL | MB_ICONWARNING);

        if (result == IDOK) {
            // Restart as admin
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            ShellExecuteA(nullptr, "runas", exePath, nullptr, nullptr, SW_SHOWNORMAL);
            exit(0);
        }
        return;
    }

    LaunchMapExeAsync(args);
}
// ════════════════════════════════════════════════════════════════════════════════
// SECTION 2 – TryAutoLoad  ( 
// ════════════════════════════════════════════════════════════════════════════════

// Load design + config from absolute paths without showing a file dialog.
// Used for session restore.  Silently ignores missing files.
bool KeyboardEditor::TryAutoLoad(const std::string& designPath,
    const std::string& configPath)
{
    bool ok = false;

    if (!designPath.empty()) {
        KeyboardLayout tmp;
        if (LoadDesign(designPath, tmp)) {
            m_layout = std::move(tmp);
            m_layoutLoaded = true;
            m_designPath = designPath;
            m_selectedKey = -1;
            EnsureLayerSchemes();
            ok = true;
        }
    }

    if (!configPath.empty()) {
        KeyboardConfig tmp;
        if (LoadConfig(configPath, m_layout, tmp)) {
            m_config = std::move(tmp);
            m_configLoaded = true;
            m_configPath = configPath;
            m_currentLayer = 0;
            m_selectedKey = -1;
            m_dirty = false;
            m_undoStack.clear();
            m_redoStack.clear();
            ParseMacrosFromConfig();
            EnsureLayerSchemes();
            ok = true;
        }
    }

    return ok;
}


std::string KeyboardEditor::MapExePath() const {
    char exe[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string p(exe);
    auto slash = p.find_last_of("\\/");
    if (slash != std::string::npos) p = p.substr(0, slash + 1);
    return p + "map.exe";
}

int KeyboardEditor::RunMapExe(const std::string& args, std::string& outMsg) {
    outMsg.clear();

    std::string mapExe = MapExePath();

    // Check if map.exe exists first
    if (GetFileAttributesA(mapExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        outMsg = "[error] map.exe not found at:\n" + mapExe;
        return -2;
    }

    // Build full command line: "map.exe" <args>
    std::string cmdLine = "\"" + mapExe + "\" " + args;

    // Pipe for child stdout + stderr
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        outMsg = "[error] Could not create pipe.";
        return -1;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    BOOL ok = CreateProcessA(
        nullptr,
        cmdBuf.data(),
        nullptr, nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);

    CloseHandle(hWritePipe);

    if (!ok) {
        CloseHandle(hReadPipe);
        outMsg = "[error] Could not launch map.exe.\nMake sure map.exe is in the same folder as the editor.";
        return -2;
    }

    // Read all output with timeout
    char buf[1024];
    DWORD nRead = 0;
    DWORD startTime = GetTickCount();
    const DWORD timeoutMs = 5000; // 30 second timeout

    while (true) {
        DWORD bytesAvailable = 0;
        PeekNamedPipe(hReadPipe, nullptr, 0, nullptr, &bytesAvailable, nullptr);

        if (bytesAvailable > 0) {
            if (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &nRead, nullptr) && nRead > 0) {
                buf[nRead] = '\0';
                outMsg += buf;
                if (outMsg.size() > 8192) {
                    outMsg += "\n... (output truncated) ...";
                    break;
                }
                continue; // Keep reading while data is available
            }
        }

        // Check if process is still running
        DWORD exitCode = STILL_ACTIVE;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        if (exitCode != STILL_ACTIVE) {
            // Process finished
            CloseHandle(hReadPipe);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            Sleep(50);
            GetExitCodeProcess(pi.hProcess, &exitCode);
            if (exitCode == STILL_ACTIVE) {
                // Force terminate
                TerminateProcess(pi.hProcess, 0);
                exitCode = 0;
            }

            return (int)exitCode;
        }

        // Check timeout
        if (GetTickCount() - startTime > timeoutMs) {
            // Timeout - terminate the process
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(hReadPipe);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            outMsg += "\n[error] Timeout after 30 seconds";
            return -3;
        }

        // Wait a bit before checking again
        Sleep(50);

    }

}

void KeyboardEditor::FlashToKeyboard() {
    m_mapStatusMsg.clear();
    m_mapStatus = MapExeStatus::Idle;
    m_showMapResult = false;

    std::string mapExe = MapExePath();
    if (mapExe.empty()) {
        m_mapStatus = MapExeStatus::NoMapExe;
        m_showMapResult = true;
        return;
    }
    if (m_designPath.empty()) {
        m_mapStatus = MapExeStatus::NoDesign;
        m_showMapResult = true;
        return;
    }
    if (!m_configLoaded) {
        m_mapStatus = MapExeStatus::NoConfig;
        m_showMapResult = true;
        return;
    }

    // Check for admin
    if (!IsRunningAsAdmin()) {
        int result = MessageBoxA(nullptr,
            "Flashing requires Administrator privileges.\n\n"
            "Click OK to restart the editor as Administrator,\n"
            "or Cancel to continue without flash capability.",
            "Admin Required", MB_OKCANCEL | MB_ICONWARNING);

        if (result == IDOK) {
            // Save config first
            if (!m_configPath.empty())
                ::SaveConfig(m_configPath, m_config);

            // Restart as admin with flash command
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::string args = "--flash \"" + m_designPath + "\" \"" + m_configPath + "\"";
            ShellExecuteA(nullptr, "runas", exePath, args.c_str(), nullptr, SW_SHOWNORMAL);
            exit(0);
        }
        return;
    }

    // Save before flash
    if (!m_configPath.empty())
        ::SaveConfig(m_configPath, m_config);

    // Build args then launch async
    std::string args = "\"" + m_designPath + "\" \"" + m_configPath + "\"";
    LaunchMapExeAsync(args);
}

void KeyboardEditor::LoadConfigFromFile(const std::string& path) {
    KeyboardConfig tmp;
    if (LoadConfig(path, m_layout, tmp)) {
        m_config = std::move(tmp);
        m_configLoaded = true;
        m_configPath = path;
        m_currentLayer = 0;
        m_selectedKey = -1;
        m_dirty = false;
        m_undoStack.clear();
        m_redoStack.clear();
        ParseMacrosFromConfig();
        EnsureLayerSchemes();
        m_session.SaveConfig(path);
    }
}


void KeyboardEditor::BackupKeyboard() {
    std::string mapExe = MapExePath();
    if (mapExe.empty()) { m_mapStatus = MapExeStatus::NoMapExe; m_showMapResult = true; return; }
    if (m_designPath.empty()) { m_mapStatus = MapExeStatus::NoDesign; m_showMapResult = true; return; }

    std::string args = "\"" + m_designPath + "\" --dump";
    LaunchMapExeAsync(args);
}

void KeyboardEditor::RestoreKeyboard() {
    std::string mapExe = MapExePath();
    if (mapExe.empty()) { m_mapStatus = MapExeStatus::NoMapExe; m_showMapResult = true; return; }
    if (m_designPath.empty()) { m_mapStatus = MapExeStatus::NoDesign; m_showMapResult = true; return; }

    // Open file picker (this is fast / UI-side, fine on the main thread)
    char filePath[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameA(&ofn)) return;  // user cancelled

    std::string args = "\"" + m_designPath + "\" --restore \"" + filePath + "\"";
    LaunchMapExeAsync(args);
}

// ─── File operations ─────────────────────────────────────────────────────────

void KeyboardEditor::OpenDesign() {
    auto path = OpenFileDialog("Open Design JSON",
        "JSON Files\0*.json\0All Files\0*.*\0");
    if (path.empty()) return;
    KeyboardLayout tmp;
    if (LoadDesign(path, tmp)) {
        m_layout = std::move(tmp);
        m_layoutLoaded = true;
        m_selectedKey = -1;
        m_designPath = path;          // ← NEW: keep for map.exe
        EnsureLayerSchemes();
        m_session.SaveDesign(path);
    }
}

void KeyboardEditor::OpenConfig() {
    auto path = OpenFileDialog("Open Config JSON (me.json)",
        "JSON Files\0*.json\0All Files\0*.*\0");
    if (path.empty()) return;
    KeyboardConfig tmp;
    if (LoadConfig(path, m_layout, tmp)) {
        m_config = std::move(tmp);
        m_configLoaded = true;
        m_configPath = path;
        m_currentLayer = 0;
        m_selectedKey = -1;
        m_dirty = false;
        m_undoStack.clear();
        m_redoStack.clear();
        ParseMacrosFromConfig();
        EnsureLayerSchemes();
        m_session.SaveConfig(path);   // ← persist
    }
}


void KeyboardEditor::SaveConfig() {
    if (!m_configLoaded) return;
    SyncMacrosToConfig();
    if (m_configPath.empty()) { SaveConfigAs(); return; }
    if (::SaveConfig(m_configPath, m_config)) {
        m_dirty = false;
        m_session.SaveConfig(m_configPath);   // ← persist
    }
}

void KeyboardEditor::SaveConfigAs() {
    if (!m_configLoaded) return;
    SyncMacrosToConfig();
    auto path = SaveFileDialog("Save Config JSON",
        "JSON Files\0*.json\0All Files\0*.*\0", "json");
    if (path.empty()) return;
    if (::SaveConfig(path, m_config)) {
        m_configPath = path;
        m_dirty = false;
        m_session.SaveConfig(path);           // ← persist
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
    int fi = m_layout.FlatIndex(mp);
    if (fi < 0 || fi >= m_config.FlatSize()) return;

    UndoRecord r;
    r.layer = m_currentLayer;
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
    m_config.macros.resize(m_macros.size());
    for (int i = 0; i < (int)m_macros.size(); ++i) {
        // Check if this macro should be preserved as raw { } syntax
        bool shouldPreserveRaw = false;
        std::string rawMacro;

        // Check current macro actions
        if (m_macros[i].actions.size() == 1) {
            const auto& action = m_macros[i].actions[0];
            if (action.type == MacroActionType::TypeString) {
                std::string text = action.text;
                // Check if it contains { } syntax pattern
                if (text.find('{') != std::string::npos && text.find('}') != std::string::npos) {
                    shouldPreserveRaw = true;
                    rawMacro = text;
                }
            }
        }

        // Also check if we have a raw macro stored from loading (original format)
        if (!shouldPreserveRaw && i < (int)m_rawMacros.size()) {
            const std::string& raw = m_rawMacros[i];
            if (raw.find('{') != std::string::npos && raw.find('}') != std::string::npos &&
                raw.find("TAP:") == std::string::npos &&
                raw.find("TYPE:") == std::string::npos) {
                shouldPreserveRaw = true;
                rawMacro = raw;
            }
        }

        if (shouldPreserveRaw) {
            // Preserve raw { } syntax exactly as-is
            m_config.macros[i] = rawMacro;
        }
        else {
            // Serialize to TAP: format for complex macros
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
                    // Check if this TypeString contains { } - preserve it raw
                    if (a.text.find('{') != std::string::npos && a.text.find('}') != std::string::npos) {
                        s = a.text;  // Replace entire string with raw macro
                    }
                    else {
                        s += "TYPE:" + a.text;
                    }
                    break;
                case MacroActionType::Delay:
                    s += "DELAY:" + std::to_string(a.delayMs); break;
                default: break;
                }
            }
            m_config.macros[i] = s;
        }
    }
}



void KeyboardEditor::ParseMacrosFromConfig() {
    // Store raw macros for preservation
    m_rawMacros = m_config.macros;

    m_macros.resize(m_config.macros.size());
    for (int i = 0; i < (int)m_config.macros.size(); ++i) {
        char nameBuf[32];
        snprintf(nameBuf, sizeof(nameBuf), "MACRO%02d", i);
        m_macros[i].name = nameBuf;
        m_macros[i].actions.clear();

        const std::string& raw = m_config.macros[i];

        // Check if this is raw { } syntax 
        // Look for { } pattern and NOT serialized format
        bool isRawBraceSyntax = (raw.find('{') != std::string::npos &&
            raw.find('}') != std::string::npos &&
            raw.find("TAP:") == std::string::npos &&
            raw.find("TYPE:") == std::string::npos &&
            raw.find("DELAY:") == std::string::npos &&
            raw.find("KEYDOWN:") == std::string::npos &&
            raw.find("KEYUP:") == std::string::npos);

        if (isRawBraceSyntax) {
            // This is raw { } syntax - store as single TypeString to preserve
            MacroAction act;
            act.type = MacroActionType::TypeString;
            act.text = raw;  // Keep exactly as is: "{KC_A,KC_D}"
            m_macros[i].actions.push_back(act);
        }
        else if (raw.find(':') != std::string::npos) {
            // Parse serialized format (TAP:KC_A | DELAY:100)
            size_t pos = 0;
            while (pos < raw.size()) {
                size_t sep = raw.find(" | ", pos);
                std::string token = raw.substr(pos, sep == std::string::npos ? std::string::npos : sep - pos);
                pos = sep == std::string::npos ? raw.size() : sep + 3;

                if (token.empty()) continue;

                MacroAction act;
                if (token.rfind("TAP:", 0) == 0) {
                    act.type = MacroActionType::KeyTap;
                    act.keycode = token.substr(4);
                }
                else if (token.rfind("KEYDOWN:", 0) == 0) {
                    act.type = MacroActionType::KeyDown;
                    act.keycode = token.substr(8);
                }
                else if (token.rfind("KEYUP:", 0) == 0) {
                    act.type = MacroActionType::KeyUp;
                    act.keycode = token.substr(6);
                }
                else if (token.rfind("TYPE:", 0) == 0) {
                    act.type = MacroActionType::TypeString;
                    act.text = token.substr(5);
                }
                else if (token.rfind("DELAY:", 0) == 0) {
                    act.type = MacroActionType::Delay;
                    act.delayMs = std::stoi(token.substr(6));
                }
                else {
                    act.type = MacroActionType::TypeString;
                    act.text = token;
                }
                m_macros[i].actions.push_back(act);
            }
        }
        else if (!raw.empty()) {
            // Plain string - treat as TypeString
            MacroAction act;
            act.type = MacroActionType::TypeString;
            act.text = raw;
            m_macros[i].actions.push_back(act);
        }
    }
}
// ─── LED scheme helpers ───────────────────────────────────────────────────────

void KeyboardEditor::EnsureLayerSchemes() {
    int n = max(m_config.LayerCount(), m_layoutLoaded ? 1 : 0);
    if (n == 0) n = 1;

    if (m_configLoaded) {
        while ((int)m_config.ledSchemes.size() < n) {
            int li = (int)m_config.ledSchemes.size();
            LedScheme ls;
            // Default: pick predefined scheme by layer index cycling through palettes
            if (!m_predefinedSchemes.empty()) {
                ls = m_predefinedSchemes[li % m_predefinedSchemes.size()];
            }
            else {
                ls.name = "Layer " + std::to_string(li);
                ls.type = LedSchemeType::Solid;
                const float* c = kLayerColors[li % kMaxLayers];
                ls.primary = { c[0], c[1], c[2] };
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
    int fi = m_layout.FlatIndex(pk.matrix);
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
        case 0: r = c2; g = x2; break;
        case 1: r = x2; g = c2; break;
        case 2: g = c2; b = x2; break;
        case 3: g = x2; b = c2; break;
        case 4: r = x2; b = c2; break;
        case 5: r = c2; b = x2; break;
        }
        return IM_COL32((uint8_t)((r + (v - c2)) * 160),
            (uint8_t)((g + (v - c2)) * 160),
            (uint8_t)((b + (v - c2)) * 160), 255);
    }
    case LedSchemeType::RainbowWave: {
        float t = fmodf((float)ImGui::GetTime() * scheme.speed * 0.3f
            + pk.x / (m_layout.totalW > 0 ? m_layout.totalW : 1.f), 1.f);
        float h = t * 360.f;
        float c2 = br, x2 = c2 * (1.f - fabsf(fmodf(h / 60.f, 2.f) - 1.f));
        float r = 0, g = 0, b = 0;
        int seg = (int)(h / 60.f) % 6;
        switch (seg) {
        case 0:r = c2; g = x2; break; case 1:r = x2; g = c2; break;
        case 2:g = c2; b = x2; break; case 3:g = x2; b = c2; break;
        case 4:r = x2; b = c2; break; case 5:r = c2; b = x2; break;
        }
        return IM_COL32((uint8_t)(r * 200), (uint8_t)(g * 200), (uint8_t)(b * 200), 255);
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

void KeyboardEditor::RenderMapResultPopup() {
    if (!m_showMapResult) return;

    // Always try to open the popup while visible
    ImGui::OpenPopup("##mapresult");

    // Center the popup
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(440, 120), ImVec2(760, 480));

    if (ImGui::BeginPopupModal("##mapresult", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove))  // Added NoMove to keep centered
    {
        // Status banner
        float t = (float)ImGui::GetTime();
        const char* spin[] = { "|", "/", "-", "\\" };

        switch (m_mapStatus) {
        case MapExeStatus::Running:
            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.f),
                "⏳  Running map.exe...  %s", spin[(int)(t * 8) % 4]);
            ImGui::TextDisabled("(please wait — keyboard is being programmed)");
            break;

        case MapExeStatus::Success:
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.f),
                "✓  Operation completed successfully");
            break;

        case MapExeStatus::Failed:
            ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.f),
                " Operation Done");
            break;

        case MapExeStatus::NoMapExe:
            ImGui::TextColored(ImVec4(1.f, 0.65f, 0.1f, 1.f),
                "⚠  map.exe not found");
            ImGui::TextDisabled("Ensure map.exe is next to the editor executable.");
            break;

        case MapExeStatus::NoDesign:
            ImGui::TextColored(ImVec4(1.f, 0.65f, 0.1f, 1.f),
                "⚠  No design .json loaded");
            ImGui::TextDisabled("Open a design.json first (File → Open Design).");
            break;

        case MapExeStatus::NoConfig:
            ImGui::TextColored(ImVec4(1.f, 0.65f, 0.1f, 1.f),
                "⚠  No config ( .json) loaded");
            ImGui::TextDisabled("Open or create a me.json first.");
            break;

        default:
            ImGui::TextDisabled("Status: %d", (int)m_mapStatus);
            break;
        }

        ImGui::Separator();

        // Scrollable output area (only if non-empty and not running)
        if (!m_mapStatusMsg.empty() && m_mapStatus != MapExeStatus::Running) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.12f, 1.f));
            ImGui::BeginChild("##mapout", ImVec2(0, 220), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.80f, 1.f));

            // Make output selectable for copy-paste
            ImGui::InputTextMultiline("##mapout_text",
                const_cast<char*>(m_mapStatusMsg.c_str()),
                m_mapStatusMsg.size() + 1,
                ImVec2(-FLT_MIN, -FLT_MIN),
                ImGuiInputTextFlags_ReadOnly);

            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        else if (m_mapStatus == MapExeStatus::Running) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.12f, 1.f));
            ImGui::BeginChild("##mapout", ImVec2(0, 80), true);
            ImGui::TextDisabled("Waiting for map.exe to complete...");
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // Close button
        float btnWidth = 100.f;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnWidth) * 0.5f);

        // For running state, show "Cancel" instead of "Close" if we want to support cancellation
        // (cancellation would require thread termination, which is risky - so just show "Close" disabled)
        if (m_mapStatus == MapExeStatus::Running) {
            ImGui::BeginDisabled();
            ImGui::Button("  Close  ", ImVec2(btnWidth, 0));
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("(cannot close while running)");
        }
        else {
            if (ImGui::Button("  Close  ", ImVec2(btnWidth, 0))) {
                ImGui::CloseCurrentPopup();
                m_showMapResult = false;
            }
        }

        ImGui::EndPopup();
    }
    else {
        // Popup was closed by user clicking outside or pressing ESC
        // Only reset if not running
        if (m_mapStatus != MapExeStatus::Running) {
            m_showMapResult = false;
        }
    }
}

// ─── Main Render ─────────────────────────────────────────────────────────────

void KeyboardEditor::Render() {

    PollMapResult();
    RenderMapResultPopup();
    m_keyTester.PollKeyTester(m_appStartTime);

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
        m_keyTester.RenderKeyTesterPanel();
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
        ImVec4 colDim(c[0] * 0.5f, c[1] * 0.5f, c[2] * 0.5f, 1.f);

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

    ImVec2 avail = ImGui::GetContentRegionAvail();
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
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const float pad = 3.f;
    const float rounding = 5.f;

    ImU32 selBorder = IM_COL32(255, 240, 100, 255);

    for (int ki = 0; ki < (int)m_layout.keys.size(); ++ki) {
        const PhysicalKey& pk = m_layout.keys[ki];

        ImVec2 tl(ox + pk.x * ku + pad, oy + pk.y * ku + pad);
        ImVec2 br(ox + (pk.x + pk.w) * ku - pad, oy + (pk.y + pk.h) * ku - pad);

        std::string kc = "---";
        if (m_configLoaded) {
            int fi = m_layout.FlatIndex(pk.matrix);
            kc = m_config.Keycode(m_currentLayer, fi);
        }

        ImU32 faceCol = KeyFaceColor(ki);
        ImU32 borderCol;
        if (ki == m_selectedKey) {
            borderCol = selBorder;
        }
        else {
            // Slightly lighter version of face for border
            borderCol = IM_COL32(
                min(255, (int)(((faceCol >> 0) & 0xFF) + 60)),
                min(255, (int)(((faceCol >> 8) & 0xFF) + 60)),
                min(255, (int)(((faceCol >> 16) & 0xFF) + 60)),
                255);
        }

        // Shadow
        dl->AddRectFilled(ImVec2(tl.x + 2, tl.y + 2), ImVec2(br.x + 2, br.y + 2),
            IM_COL32(0, 0, 0, 100), rounding);
        // Face
        dl->AddRectFilled(tl, br, faceCol, rounding);
        // Border
        dl->AddRect(tl, br, borderCol, rounding, 0, 2.f);

        // Matrix badge
        char badge[16];
        snprintf(badge, sizeof(badge), "%d,%d", pk.matrix.row, pk.matrix.col);
        dl->AddText(ImVec2(tl.x + 3, tl.y + 3), IM_COL32(180, 180, 180, 100), badge);

        // Label
        std::string label = GetKeycodeLabel(kc, m_keycodeMap);
        ImVec2 tsz = ImGui::CalcTextSize(label.c_str());
        float cx = tl.x + (br.x - tl.x - tsz.x) * 0.5f;
        float cy = tl.y + (br.y - tl.y - tsz.y) * 0.5f;
        ImU32 textCol = (ki == m_selectedKey)
            ? IM_COL32(30, 20, 0, 255)
            : IM_COL32(230, 230, 230, 255);
        dl->AddText(ImVec2(cx, cy), textCol, label.c_str());

        // Mouse interaction
        ImVec2 mpos = io.MousePos;
        bool hovered = mpos.x >= tl.x && mpos.x < br.x &&
            mpos.y >= tl.y && mpos.y < br.y;
        if (hovered) {
            dl->AddRect(tl, br, IM_COL32(255, 255, 255, 180), rounding, 0, 2.5f);
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
    int fi = m_layout.FlatIndex(pk.matrix);
    const std::string& cur = m_config.Keycode(m_currentLayer, fi);

    ImGui::Text("Selected:  [%d,%d]", pk.matrix.row, pk.matrix.col);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.f, 0.9f, 0.3f, 1.f), "  %s", cur.c_str());

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
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
        if (ImGui::RadioButton("All", m_selectedCategory == -1))
            m_selectedCategory = -1;
        for (int ci = 0; ci < (int)KeyCategory::COUNT; ++ci) {
            ImGui::SameLine();
            if (ImGui::RadioButton(CategoryName((KeyCategory)ci), m_selectedCategory == ci))
                m_selectedCategory = ci;
            if ((ci + 1) % 4 == 0) ImGui::NewLine();
        }
        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    std::string search(m_searchBuf);
    std::transform(search.begin(), search.end(), search.begin(), ::tolower);

    ImGui::BeginChild("##kclist", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    int col = 0, ncols = 3;
    float btnW = (ImGui::GetContentRegionAvail().x - (ncols - 1) * 4) / ncols;

    for (const auto& def : m_keycodeDb) {
        if (m_selectedCategory >= 0 && (int)def.category != m_selectedCategory) continue;
        if (!search.empty()) {
            std::string lc = def.code;  std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
            std::string ll = def.label; std::transform(ll.begin(), ll.end(), ll.begin(), ::tolower);
            std::string lt = def.tooltip; std::transform(lt.begin(), lt.end(), lt.begin(), ::tolower);
            if (lc.find(search) == std::string::npos &&
                ll.find(search) == std::string::npos &&
                lt.find(search) == std::string::npos) continue;
        }

        bool isCur = (def.code == cur);
        if (isCur) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.1f, 1.f));

        if (col > 0 && col < ncols) ImGui::SameLine();
        ImGui::PushID(def.code.c_str());
        if (ImGui::Button(def.label.c_str(), ImVec2(btnW, 0)))
            ApplyKeycode(m_selectedKey, def.code);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s\n%s", def.code.c_str(), def.tooltip.c_str());
        ImGui::PopID();
        if (isCur) ImGui::PopStyleColor();
        col = (col + 1) % ncols;
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
        if ((i + 1) % 4 != 0) ImGui::SameLine(); else ImGui::NewLine();
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
            if ((mi + 1) % 8 != 0) ImGui::SameLine(); else ImGui::NewLine();
        }
    }

    ImGui::EndChild();
}

// ─── Macro editor ─────────────────────────────────────────────────────────────

void KeyboardEditor::RenderMacroPanel() {
    // ── Add / remove macros ───────────────────────────────────────────────────
    if (ImGui::Button("+ New Macro")) {
        MacroEntry me;
        char buf[32]; snprintf(buf, sizeof(buf), "MACRO%02d", (int)m_macros.size());
        me.name = buf;
        m_macros.push_back(me);
        m_editingMacro = (int)m_macros.size() - 1;
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.f);
    ImGui::InputText("Search##msrch", m_macroSearchBuf, sizeof(m_macroSearchBuf));

    ImGui::Separator();

    // ── Macro list (left pane) ────────────────────────────────────────────────
    float listH = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("MacroList", ImVec2(200.f, listH), true);

    for (int mi = 0; mi < (int)m_macros.size(); ++mi) {
        MacroEntry& me = m_macros[mi];

        // Filter by search
        if (m_macroSearchBuf[0]) {
            std::string lc = me.name;
            std::string srch(m_macroSearchBuf);
            auto ci = [](unsigned char c) { return (char)tolower(c); };
            std::transform(lc.begin(), lc.end(), lc.begin(), ci);
            std::transform(srch.begin(), srch.end(), srch.begin(), ci);
            if (lc.find(srch) == std::string::npos) continue;
        }

        ImGui::PushID(mi);
        bool sel = (m_editingMacro == mi);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.45f, 0.80f, 1.f));
        if (ImGui::Selectable(me.name.c_str(), sel,
            ImGuiSelectableFlags_AllowDoubleClick)) {
            m_editingMacro = mi;
        }
        if (sel) ImGui::PopStyleColor();

        // Tooltip: show summary
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", MacroSummary(me).c_str());

        // Right-click context: rename / delete
        if (ImGui::BeginPopupContextItem("MacroCtx")) {
            static char renameBuf[64] = {};
            if (ImGui::IsWindowAppearing()) strncpy_s(renameBuf, sizeof(renameBuf), me.name.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(140.f);
            if (ImGui::InputText("Name##rn", renameBuf, sizeof(renameBuf),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
                me.name = renameBuf;
                m_dirty = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                m_macros.erase(m_macros.begin() + mi);
                if (m_editingMacro >= (int)m_macros.size())
                    m_editingMacro = (int)m_macros.size() - 1;
                m_dirty = true;
                ImGui::CloseCurrentPopup();
                ImGui::PopID();
                ImGui::EndPopup();
                continue;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    ImGui::EndChild();

    // ── Action editor (right pane) ────────────────────────────────────────────
    ImGui::SameLine();

    if (m_editingMacro < 0 || m_editingMacro >= (int)m_macros.size()) {
        ImGui::BeginChild("MacroEdit", ImVec2(0, listH), true);
        ImGui::TextDisabled("Select a macro on the left to edit it.");
        ImGui::EndChild();
        return;
    }

    MacroEntry& me = m_macros[m_editingMacro];

    ImGui::BeginChild("MacroEdit", ImVec2(0, listH), true);
    ImGui::Text("Editing: %s", me.name.c_str());
    ImGui::SameLine();

    // Duplicate
    if (ImGui::SmallButton("Duplicate")) {
        MacroEntry copy = me;
        char buf[64]; snprintf(buf, sizeof(buf), "%s_copy", me.name.c_str());
        copy.name = buf;
        m_macros.insert(m_macros.begin() + m_editingMacro + 1, copy);
        m_dirty = true;
    }
    ImGui::Separator();

    // ── Action list ───────────────────────────────────────────────────────────
    ImGui::Text("Actions (%d):", (int)me.actions.size());
    float actionListH = 160.f;  // fixed height, rest goes to library
    ImGui::BeginChild("##actions", ImVec2(0, actionListH), true);
    for (int ai = 0; ai < (int)me.actions.size(); ++ai) {
        MacroAction& a = me.actions[ai];
        ImGui::PushID(ai);

        // Up / Down
        if (ImGui::ArrowButton("##u", ImGuiDir_Up) && ai > 0) {
            std::swap(me.actions[ai], me.actions[ai - 1]); m_dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##d", ImGuiDir_Down) && ai < (int)me.actions.size() - 1) {
            std::swap(me.actions[ai], me.actions[ai + 1]); m_dirty = true;
        }
        ImGui::SameLine();

        // Type label
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.f, 1.f), "[%s]",
            MacroActionTypeName(a.type));
        ImGui::SameLine();

        // Value
        switch (a.type) {
        case MacroActionType::KeyTap:
            [[fallthrough]];
        case MacroActionType::KeyDown:
            [[fallthrough]];
        case MacroActionType::KeyUp:
        {
            char kbuf[64]; strncpy_s(kbuf, a.keycode.c_str(), 63);
            ImGui::SetNextItemWidth(100.f);
            if (ImGui::InputText("##kc", kbuf, sizeof(kbuf),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
                a.keycode = kbuf; m_dirty = true;
            }
            break;
        }
        case MacroActionType::TypeString:
        {
            char tbuf[256]; strncpy_s(tbuf, sizeof(tbuf), a.text.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(180.f);
            if (ImGui::InputText("##txt", tbuf, sizeof(tbuf),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
                a.text = tbuf; m_dirty = true;
            }
            break;
        }
        case MacroActionType::Delay:
        {
            ImGui::SetNextItemWidth(70.f);
            if (ImGui::InputInt("ms##dl", &a.delayMs)) {
                if (a.delayMs < 1) a.delayMs = 1; m_dirty = true;
            }
            break;
        }
        default: break;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            me.actions.erase(me.actions.begin() + ai);
            m_dirty = true;
            ImGui::PopID();
            continue;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // ── Manual add row ────────────────────────────────────────────────────────
    ImGui::TextDisabled("Add action manually:");
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::BeginCombo("##nat", MacroActionTypeName(m_newActionType))) {
        for (int t = 0; t < (int)MacroActionType::COUNT; t++) {
            bool s = ((int)m_newActionType == t);
            if (ImGui::Selectable(MacroActionTypeName((MacroActionType)t), s))
                m_newActionType = (MacroActionType)t;
            if (s) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    switch (m_newActionType) {
    case MacroActionType::KeyTap:
    case MacroActionType::KeyDown:
    case MacroActionType::KeyUp:
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputText("##nak", m_newActionKey, sizeof(m_newActionKey));
        break;
    case MacroActionType::TypeString:
        ImGui::SetNextItemWidth(180.f);
        ImGui::InputText("##nat2", m_newActionText, sizeof(m_newActionText));
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
        case MacroActionType::KeyUp:   a.keycode = m_newActionKey;   break;
        case MacroActionType::TypeString: a.text = m_newActionText;  break;
        case MacroActionType::Delay:   a.delayMs = m_newActionDelay; break;
        default: break;
        }
        me.actions.push_back(a);
        m_dirty = true;
    }

    ImGui::Separator();

    // ── Predefined library picker ─────────────────────────────────────────────
    RenderMacroLibraryPanel(me);

    ImGui::EndChild();  // MacroEdit
}


// ─── Key Tester ───────────────────────────────────────────────────────────────

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
            }
            else {
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
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 1.f));
    ImGui::Begin("##statusbar", nullptr, flags);

    if (m_configLoaded) {
        ImGui::Text("Config: %s",
            m_configPath.empty() ? m_config.name.c_str() : m_configPath.c_str());
        ImGui::SameLine();
        if (m_dirty) ImGui::TextColored(ImVec4(1, 0.5f, 0.2f, 1), " [unsaved]");
    }
    else {
        ImGui::TextDisabled("No config loaded  |  File → Open Config (me.json)");
    }

    if (m_layoutLoaded) {
        ImGui::SameLine(0, 20);
        ImGui::TextDisabled("Layout: %s  |  %d keys  |  %d layers",
            m_layout.name.c_str(), (int)m_layout.keys.size(),
            m_config.LayerCount());
    }

    if (m_testerActive) {
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.f), "● KEY TESTER ACTIVE");
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 240.f + ImGui::GetCursorPosX());
    ImGui::TextDisabled("Ctrl+Z Undo  |  Ctrl+Y Redo  |  Ctrl+S Save");

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}




// ════════════════════════════════════════════════════════════════════════════════
// SECTION 7 – RenderMacroLibraryPanel  ( 
// ════════════════════════════════════════════════════════════════════════════════

void KeyboardEditor::RenderMacroLibraryPanel(MacroEntry& me) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.16f, 1.f));
    ImGui::BeginChild("##macrolib", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.f),
        "Predefined Action Library");
    ImGui::SameLine();
    ImGui::TextDisabled("(%d actions) — double-click or press + Add to insert",
        (int)m_macroLib.size());

    // ── Filter row ────────────────────────────────────────────────────────────
    ImGui::SetNextItemWidth(140.f);
    ImGui::InputText("Search##lib", m_libSearchBuf, sizeof(m_libSearchBuf));
    ImGui::SameLine();

    const char* curCat = (m_libCategoryIdx < (int)m_libCategories.size())
        ? m_libCategories[m_libCategoryIdx].c_str()
        : "All";
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::BeginCombo("Category##lcat", curCat)) {
        for (int ci = 0; ci < (int)m_libCategories.size(); ++ci) {
            bool sel = (ci == m_libCategoryIdx);
            if (ImGui::Selectable(m_libCategories[ci].c_str(), sel))
                m_libCategoryIdx = ci;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    std::string catFilter = (m_libCategoryIdx == 0)
        ? std::string()
        : m_libCategories[m_libCategoryIdx];
    std::string srchFilter(m_libSearchBuf);

    std::vector<const MacroLibraryEntry*> filtered =
        FilterLibrary(m_macroLib, catFilter, srchFilter);

    ImGui::SameLine();
    ImGui::TextDisabled("%d shown", (int)filtered.size());
    ImGui::Separator();

    // ── Scrollable table ──────────────────────────────────────────────────────
    ImGuiTableFlags tflags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##libtable", 4, tflags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 110.f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 120.f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Insert", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableHeadersRow();

        for (int fi = 0; fi < (int)filtered.size(); ++fi) {
            const MacroLibraryEntry& e = *filtered[fi];
            ImGui::TableNextRow();

            // Category cell
            ImGui::TableSetColumnIndex(0);
            float hue = 0.f;
            for (size_t k = 0; k < e.category.size(); ++k)
                hue = fmodf(hue + (float)(unsigned char)e.category[k] * 0.031f, 1.f);
            ImVec4 catCol;
            ImGui::ColorConvertHSVtoRGB(hue, 0.5f, 0.85f,
                catCol.x, catCol.y, catCol.z);
            catCol.w = 1.f;
            ImGui::TextColored(catCol, "%s", e.category.c_str());

            // Action name
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(fi);
            bool clicked = ImGui::Selectable(
                e.name.c_str(), false,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowDoubleClick);
            bool dblClicked = clicked && ImGui::IsMouseDoubleClicked(0);

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.f, 1.f), "%s", e.name.c_str());
                ImGui::TextDisabled("%s", e.description.c_str());
                ImGui::Separator();
                ImGui::Text("Type: %s", LibActionTypeName(e.libType));
                switch (e.libType) {
                case LibActionType::KeyTap:
                case LibActionType::KeyDown:
                case LibActionType::KeyUp:
                    ImGui::Text("Keycode: %s", e.keycode.c_str());
                    break;
                case LibActionType::TypeString:
                    ImGui::Text("Text: \"%s\"", e.text.c_str());
                    break;
                case LibActionType::Delay:
                    ImGui::Text("Delay: %d ms", e.delayMs);
                    break;
                default: break;
                }
                ImGui::TextDisabled("Double-click or press + Add");
                ImGui::EndTooltip();
            }

            // Description cell
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("%s", e.description.c_str());

            // Insert button
            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImVec4(0.18f, 0.42f, 0.28f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(0.25f, 0.62f, 0.40f, 1.f));
            bool insertBtn = ImGui::SmallButton("+ Add");
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            // Convert LibActionType → MacroActionType and append
            if (dblClicked || insertBtn) {
                MacroAction a;
                switch (e.libType) {
                case LibActionType::KeyTap:
                    a.type = MacroActionType::TypeString;  // Changed from KeyTap to TypeString
                    // Convert to { } syntax for keyboard
                    a.text = "{" + e.keycode + "}";
                    break;
                case LibActionType::KeyDown:
                    a.type = MacroActionType::TypeString;
                    a.text = "{" + e.keycode + "}";
                    break;
                case LibActionType::KeyUp:
                    a.type = MacroActionType::TypeString;
                    a.text = "{" + e.keycode + "}";
                    break;
                case LibActionType::TypeString:
                    a.type = MacroActionType::TypeString;
                    a.text = e.text;
                    break;
                case LibActionType::Delay:
                    a.type = MacroActionType::TypeString;
                    a.text = "${" + std::to_string(e.delayMs) + "}";
                    break;
                default: break;
                }
                me.actions.push_back(a);
                m_dirty = true;
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();        // ##macrolib
    ImGui::PopStyleColor();   // ChildBg
}