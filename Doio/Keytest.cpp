#include "Keytest.h"
#include <windows.h>
#include <imgui.h>
#include <algorithm>

#undef max
#undef min

static const KeyDef FULL_KEYBOARD_LAYOUT[] = {
    { VK_ESCAPE, "Esc", 0.0f, 0.0f, 1.0f, 1.0f, 65 },
    { VK_F1, "F1", 2.0f, 0.0f, 1.0f, 1.0f, 69 },
    { VK_F2, "F2", 3.0f, 0.0f, 1.0f, 1.0f, 73 },
    { VK_F3, "F3", 4.0f, 0.0f, 1.0f, 1.0f, 77 },
    { VK_F4, "F4", 5.0f, 0.0f, 1.0f, 1.0f, 82 },
    { VK_F5, "F5", 6.5f, 0.0f, 1.0f, 1.0f, 87 },
    { VK_F6, "F6", 7.5f, 0.0f, 1.0f, 1.0f, 92 },
    { VK_F7, "F7", 8.5f, 0.0f, 1.0f, 1.0f, 98 },
    { VK_F8, "F8", 9.5f, 0.0f, 1.0f, 1.0f, 103 },
    { VK_F9, "F9", 11.0f, 0.0f, 1.0f, 1.0f, 110 },
    { VK_F10, "F10", 12.0f, 0.0f, 1.0f, 1.0f, 116 },
    { VK_F11, "F11", 13.0f, 0.0f, 1.0f, 1.0f, 123 },
    { VK_F12, "F12", 14.0f, 0.0f, 1.0f, 1.0f, 130 },
    { VK_SNAPSHOT, "Print", 15.25f, 0.0f, 1.0f, 1.0f, 138 },
    { VK_SCROLL, "Scroll", 16.25f, 0.0f, 1.0f, 1.0f, 146 },
    { VK_PAUSE, "Pause", 17.25f, 0.0f, 1.0f, 1.0f, 155 },
    { VK_SLEEP, "Sleep", 18.5f, 0.0f, 1.0f, 1.0f, 164 },
    { VK_VOLUME_MUTE, "Mute", 19.5f, 0.0f, 1.0f, 1.0f, 174 },
    { VK_VOLUME_DOWN, "Vol -", 20.5f, 0.0f, 1.0f, 1.0f, 185 },
    { VK_VOLUME_UP, "Vol +", 21.5f, 0.0f, 1.0f, 1.0f, 196 },
    { VK_OEM_3, "~", 0.0f, 1.25f, 1.0f, 1.0f, 207 },
    { '1', "1", 1.0f, 1.25f, 1.0f, 1.0f, 220 },
    { '2', "2", 2.0f, 1.25f, 1.0f, 1.0f, 233 },
    { '3', "3", 3.0f, 1.25f, 1.0f, 1.0f, 246 },
    { '4', "4", 4.0f, 1.25f, 1.0f, 1.0f, 261 },
    { '5', "5", 5.0f, 1.25f, 1.0f, 1.0f, 277 },
    { '6', "6", 6.0f, 1.25f, 1.0f, 1.0f, 293 },
    { '7', "7", 7.0f, 1.25f, 1.0f, 1.0f, 311 },
    { '8', "8", 8.0f, 1.25f, 1.0f, 1.0f, 329 },
    { '9', "9", 9.0f, 1.25f, 1.0f, 1.0f, 349 },
    { '0', "0", 10.0f, 1.25f, 1.0f, 1.0f, 369 },
    { VK_OEM_MINUS, "-", 11.0f, 1.25f, 1.0f, 1.0f, 392 },
    { VK_OEM_PLUS, "=", 12.0f, 1.25f, 1.0f, 1.0f, 415 },
    { VK_BACK, "Backspace", 13.0f, 1.25f, 2.0f, 1.0f, 440 },
    { VK_INSERT, "Ins", 15.25f, 1.25f, 1.0f, 1.0f, 466 },
    { VK_HOME, "Home", 16.25f, 1.25f, 1.0f, 1.0f, 493 },
    { VK_PRIOR, "PgUp", 17.25f, 1.25f, 1.0f, 1.0f, 523 },
    { VK_NUMLOCK, "N.Lck", 18.5f, 1.25f, 1.0f, 1.0f, 554 },
    { VK_DIVIDE, "/", 19.5f, 1.25f, 1.0f, 1.0f, 587 },
    { VK_MULTIPLY, "*", 20.5f, 1.25f, 1.0f, 1.0f, 622 },
    { VK_SUBTRACT, "-", 21.5f, 1.25f, 1.0f, 1.0f, 659 },
    { VK_TAB, "Tab", 0.0f, 2.25f, 1.5f, 1.0f, 698 },
    { 'Q', "Q", 1.5f, 2.25f, 1.0f, 1.0f, 739 },
    { 'W', "W", 2.5f, 2.25f, 1.0f, 1.0f, 783 },
    { 'E', "E", 3.5f, 2.25f, 1.0f, 1.0f, 830 },
    { 'R', "R", 4.5f, 2.25f, 1.0f, 1.0f, 880 },
    { 'T', "T", 5.5f, 2.25f, 1.0f, 1.0f, 932 },
    { 'Y', "Y", 6.5f, 2.25f, 1.0f, 1.0f, 987 },
    { 'U', "U", 7.5f, 2.25f, 1.0f, 1.0f, 1046 },
    { 'I', "I", 8.5f, 2.25f, 1.0f, 1.0f, 1108 },
    { 'O', "O", 9.5f, 2.25f, 1.0f, 1.0f, 1174 },
    { 'P', "P", 10.5f, 2.25f, 1.0f, 1.0f, 1244 },
    { VK_OEM_4, "[", 11.5f, 2.25f, 1.0f, 1.0f, 1318 },
    { VK_OEM_6, "]", 12.5f, 2.25f, 1.0f, 1.0f, 1396 },
    { VK_OEM_5, "\\", 13.5f, 2.25f, 1.5f, 1.0f, 1479 },
    { VK_DELETE, "Del", 15.25f, 2.25f, 1.0f, 1.0f, 1567 },
    { VK_END, "End", 16.25f, 2.25f, 1.0f, 1.0f, 1660 },
    { VK_NEXT, "PgDn", 17.25f, 2.25f, 1.0f, 1.0f, 1759 },
    { VK_NUMPAD7, "7", 18.5f, 2.25f, 1.0f, 1.0f, 1863 },
    { VK_NUMPAD8, "8", 19.5f, 2.25f, 1.0f, 1.0f, 1974 },
    { VK_NUMPAD9, "9", 20.5f, 2.25f, 1.0f, 1.0f, 2091 },
    { VK_ADD, "+", 21.5f, 2.25f, 1.0f, 2.0f, 2216 },
    { VK_CAPITAL, "Caps Lock", 0.0f, 3.25f, 1.75f, 1.0f, 2347 },
    { 'A', "A", 1.75f, 3.25f, 1.0f, 1.0f, 2487 },
    { 'S', "S", 2.75f, 3.25f, 1.0f, 1.0f, 2635 },
    { 'D', "D", 3.75f, 3.25f, 1.0f, 1.0f, 2791 },
    { 'F', "F", 4.75f, 3.25f, 1.0f, 1.0f, 2957 },
    { 'G', "G", 5.75f, 3.25f, 1.0f, 1.0f, 3133 },
    { 'H', "H", 6.75f, 3.25f, 1.0f, 1.0f, 3319 },
    { 'J', "J", 7.75f, 3.25f, 1.0f, 1.0f, 3516 },
    { 'K', "K", 8.75f, 3.25f, 1.0f, 1.0f, 3726 },
    { 'L', "L", 9.75f, 3.25f, 1.0f, 1.0f, 3947 },
    { VK_OEM_1, ";", 10.75f, 3.25f, 1.0f, 1.0f, 4182 },
    { VK_OEM_7, "'", 11.75f, 3.25f, 1.0f, 1.0f, 4431 },
    { VK_RETURN, "Enter", 12.75f, 3.25f, 2.25f, 1.0f, 4694 },
    { VK_NUMPAD4, "4", 18.5f, 3.25f, 1.0f, 1.0f, 4974 },
    { VK_NUMPAD5, "5", 19.5f, 3.25f, 1.0f, 1.0f, 5269 },
    { VK_NUMPAD6, "6", 20.5f, 3.25f, 1.0f, 1.0f, 5583 },
    { VK_LSHIFT, "Left Shift", 0.0f, 4.25f, 2.25f, 1.0f, 5915 },
    { 'Z', "Z", 2.25f, 4.25f, 1.0f, 1.0f, 6266 },
    { 'X', "X", 3.25f, 4.25f, 1.0f, 1.0f, 6639 },
    { 'C', "C", 4.25f, 4.25f, 1.0f, 1.0f, 7033 },
    { 'V', "V", 5.25f, 4.25f, 1.0f, 1.0f, 7452 },
    { 'B', "B", 6.25f, 4.25f, 1.0f, 1.0f, 7895 },
    { 'N', "N", 7.25f, 4.25f, 1.0f, 1.0f, 8364 },
    { 'M', "M", 8.25f, 4.25f, 1.0f, 1.0f, 8861 },
    { VK_OEM_COMMA, ",", 9.25f, 4.25f, 1.0f, 1.0f, 9388 },
    { VK_OEM_PERIOD, ".", 10.25f, 4.25f, 1.0f, 1.0f, 9946 },
    { VK_OEM_2, "/", 11.25f, 4.25f, 1.0f, 1.0f, 10538 },
    { VK_RSHIFT, "Right Shift", 12.25f, 4.25f, 2.75f, 1.0f, 11164 },
    { VK_UP, "Up", 16.25f, 4.25f, 1.0f, 1.0f, 11828 },
    { VK_NUMPAD1, "1", 18.5f, 4.25f, 1.0f, 1.0f, 12531 },
    { VK_NUMPAD2, "2", 19.5f, 4.25f, 1.0f, 1.0f, 13276 },
    { VK_NUMPAD3, "3", 20.5f, 4.25f, 1.0f, 1.0f, 14066 },
    { VK_RETURN, "N.Ent", 21.5f, 4.25f, 1.0f, 2.0f, 14902 },
    { VK_LCONTROL, "LCtrl", 0.0f, 5.25f, 1.25f, 1.0f, 15788 },
    { VK_LWIN, "LWin", 1.25f, 5.25f, 1.25f, 1.0f, 16726 },
    { VK_LMENU, "LAlt", 2.5f, 5.25f, 1.25f, 1.0f, 17721 },
    { VK_SPACE, "Space", 3.75f, 5.25f, 6.25f, 1.0f, 18774 },
    { VK_RMENU, "RAlt", 10.0f, 5.25f, 1.25f, 1.0f, 19890 },
    { VK_RWIN, "RWin", 11.25f, 5.25f, 1.25f, 1.0f, 21073 },
    { VK_APPS, "Menu", 12.5f, 5.25f, 1.25f, 1.0f, 22326 },
    { VK_RCONTROL, "RCtrl", 13.75f, 5.25f, 1.25f, 1.0f, 23653 },
    { VK_LEFT, "Left", 15.25f, 5.25f, 1.0f, 1.0f, 25059 },
    { VK_DOWN, "Down", 16.25f, 5.25f, 1.0f, 1.0f, 26549 },
    { VK_RIGHT, "Right", 17.25f, 5.25f, 1.0f, 1.0f, 28128 },
    { VK_NUMPAD0, "0", 18.5f, 5.25f, 2.0f, 1.0f, 29799 },
    { VK_DECIMAL, ".", 20.5f, 5.25f, 1.0f, 1.0f, 31571 },
};

#pragma warning(disable: 4996)
std::string KeyTester::VkName(int vk) {
    if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
    if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
    switch(vk) {
    case VK_BACK: return "Backspace"; case VK_TAB: return "Tab"; case VK_RETURN: return "Enter";
    case VK_SHIFT: case VK_LSHIFT: return "LShift"; case VK_RSHIFT: return "RShift";
    case VK_CONTROL: case VK_LCONTROL: return "LCtrl"; case VK_RCONTROL: return "RCtrl";
    case VK_MENU: case VK_LMENU: return "LAlt"; case VK_RMENU: return "RAlt";
    case VK_PAUSE: return "Pause"; case VK_CAPITAL: return "Caps"; case VK_ESCAPE: return "Esc";
    case VK_SPACE: return "Space"; case VK_PRIOR: return "PgUp"; case VK_NEXT: return "PgDn";
    case VK_END: return "End"; case VK_HOME: return "Home"; case VK_LEFT: return "Left";
    case VK_UP: return "Up"; case VK_RIGHT: return "Right"; case VK_DOWN: return "Down";
    case VK_SNAPSHOT: return "PrintScreen"; case VK_INSERT: return "Insert"; case VK_DELETE: return "Delete";
    case VK_LWIN: return "LWin"; case VK_RWIN: return "RWin"; case VK_APPS: return "Menu";
    case VK_NUMPAD0: return "Num 0"; case VK_NUMPAD1: return "Num 1"; case VK_NUMPAD2: return "Num 2";
    case VK_NUMPAD3: return "Num 3"; case VK_NUMPAD4: return "Num 4"; case VK_NUMPAD5: return "Num 5";
    case VK_NUMPAD6: return "Num 6"; case VK_NUMPAD7: return "Num 7"; case VK_NUMPAD8: return "Num 8";
    case VK_NUMPAD9: return "Num 9"; case VK_MULTIPLY: return "Num *"; case VK_ADD: return "Num +";
    case VK_SUBTRACT: return "Num -"; case VK_DECIMAL: return "Num ."; case VK_DIVIDE: return "Num /";
    case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3"; case VK_F4: return "F4";
    case VK_F5: return "F5"; case VK_F6: return "F6"; case VK_F7: return "F7"; case VK_F8: return "F8";
    case VK_F9: return "F9"; case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
    case VK_NUMLOCK: return "NumLock"; case VK_SCROLL: return "ScrollLock";
    case VK_OEM_1: return ";"; case VK_OEM_PLUS: return "="; case VK_OEM_COMMA: return ",";
    case VK_OEM_MINUS: return "-"; case VK_OEM_PERIOD: return "."; case VK_OEM_2: return "/";
    case VK_OEM_3: return "`"; case VK_OEM_4: return "["; case VK_OEM_5: return "\\";
    case VK_OEM_6: return "]"; case VK_OEM_7: return "'";
    case VK_VOLUME_MUTE: return "Mute"; case VK_VOLUME_DOWN: return "Vol-"; case VK_VOLUME_UP: return "Vol+";
    case VK_MEDIA_NEXT_TRACK: return "Next"; case VK_MEDIA_PREV_TRACK: return "Prev";
    case VK_MEDIA_STOP: return "Stop"; case VK_MEDIA_PLAY_PAUSE: return "Play/Pause";
    }
    char buf[16]; snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

KeyTester::KeyTester() {
    m_vkDown.assign(256, false);
    m_vkAnimTime.assign(256, 0.0f);
    m_audioThreadExit = false;
    m_beepActive = false;
    m_currentFreq = 0;
    m_audioThread = std::thread(&KeyTester::AudioThreadFunc, this);
}

KeyTester::~KeyTester() {
    m_audioThreadExit = true;
    m_beepActive = false;
    if (m_audioThread.joinable()) {
        m_audioThread.join();
    }
}

void KeyTester::AudioThreadFunc() {
    while (!m_audioThreadExit) {
        int freq = 0;
        bool beeping = false;
        {
            std::lock_guard<std::mutex> lock(m_audioMutex);
            if (m_beepActive && m_currentFreq > 0) {
                freq = m_currentFreq;
                beeping = true;
            }
        }

        if (beeping) {
            Beep(freq, 100);
            {
                std::lock_guard<std::mutex> lock(m_audioMutex);
                m_beepActive = false;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void KeyTester::PollKeyTester(float appStartTime) {
    if (!m_testerActive) return;

    bool anyNewKey = false;
    int latestFreq = 0;

    for (int vk = 1; vk < 256; ++vk) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (down && !m_vkDown[vk]) {
            // Key pressed
            KeyPressEvent ev;
            ev.vkCode    = vk;
            ev.name      = VkName(vk);
            ev.isDown    = true;
            ev.timestamp = (float)ImGui::GetTime() - appStartTime;
            m_keyLog.push_back(ev);
            if ((int)m_keyLog.size() > kLogMaxSize) m_keyLog.pop_front();

            m_vkAnimTime[vk] = (float)ImGui::GetTime();
            anyNewKey = true;

            for (const auto& kd : FULL_KEYBOARD_LAYOUT) {
                if (kd.vk == vk) {
                    latestFreq = kd.freq;
                    break;
                }
            }
        } else if (!down && m_vkDown[vk]) {
            // Key released
            KeyPressEvent ev;
            ev.vkCode    = vk;
            ev.name      = VkName(vk);
            ev.isDown    = false;
            ev.timestamp = (float)ImGui::GetTime() - appStartTime;
            m_keyLog.push_back(ev);
            if ((int)m_keyLog.size() > kLogMaxSize) m_keyLog.pop_front();
        }
        m_vkDown[vk] = down;
    }

    if (anyNewKey && latestFreq > 0) {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        m_currentFreq = latestFreq;
        m_beepActive = true;
    }
}

void KeyTester::RenderKeyTesterPanel() {
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
            m_vkAnimTime.assign(256, 0.0f);
        }
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Log")) m_keyLog.clear();
    ImGui::SameLine();
    ImGui::TextDisabled(m_testerActive
        ? "Press keys — full layout — melody on press"
        : "Click Start to begin capture");

    ImGui::Separator();

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

    // Render Full Keyboard Layout
    ImGui::Text("Full Keyboard Test:");
    ImVec2 avail = ImGui::GetContentRegionAvail();

    float totalW = 22.5f;
    float totalH = 6.5f;
    float ku = std::min(avail.x / (totalW + 0.5f), 42.f);
    ku = std::max(ku, 14.f);

    float ox = ImGui::GetCursorScreenPos().x + 4.f;
    float oy = ImGui::GetCursorScreenPos().y + 4.f;
    ImGui::Dummy(ImVec2(totalW * ku + 8.f, totalH * ku + 8.f));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float p = 2.f, r = 3.f;
    float now = (float)ImGui::GetTime();

    for (const auto& kd : FULL_KEYBOARD_LAYOUT) {
        ImVec2 tl(ox + kd.x * ku + p, oy + kd.y * ku + p);
        ImVec2 br(ox + (kd.x + kd.w) * ku - p, oy + (kd.y + kd.h) * ku - p);

        bool pressed = m_vkDown[kd.vk];
        float animT = now - m_vkAnimTime[kd.vk];

        ImU32 face = IM_COL32(40, 45, 55, 220);
        ImU32 border = IM_COL32(80, 85, 100, 255);

        if (pressed) {
            face = IM_COL32(60, 200, 100, 255);
            border = IM_COL32(120, 255, 160, 255);
        } else if (animT < 0.5f) {
            // Fade out effect
            float t = animT / 0.5f;
            int rC = (int)(60 + (40 - 60) * t);
            int gC = (int)(200 + (45 - 200) * t);
            int bC = (int)(100 + (55 - 100) * t);
            face = IM_COL32(rC, gC, bC, 255);
        }

        dl->AddRectFilled(tl, br, face, r);
        dl->AddRect(tl, br, border, r, 0, 1.5f);

        std::string lbl = kd.label;
        ImVec2 ts = ImGui::CalcTextSize(lbl.c_str());
        if ((br.x - tl.x) > ts.x + 2.f) {
            float cx = tl.x + (br.x - tl.x - ts.x) * 0.5f;
            float cy = tl.y + (br.y - tl.y - ts.y) * 0.5f;
            dl->AddText({cx, cy},
                pressed ? IM_COL32(20,30,20,255) : IM_COL32(180,185,200,255),
                lbl.c_str());
        }
    }

    // ── Log table ────────────────────────────────────────────────────────────
    ImGui::Text("Event Log:");
    ImGui::BeginChild("KeyLogScroll", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    if (ImGui::BeginTable("KeyLogTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto it = m_keyLog.rbegin(); it != m_keyLog.rend(); ++it) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.3f", it->timestamp);
            ImGui::TableSetColumnIndex(1);
            if (it->isDown) {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.f), "PRESS");
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.3f, 1.f), "RELEASE");
            }
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", it->name.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}
