// KeyboardEditor_changes.cpp
// ==========================
// Merge guide: copy each numbered SECTION into KeyboardEditor.cpp,
// replacing the function listed in the section header.
// The INCLUDES block must appear at the TOP of KeyboardEditor.cpp
// (it already has most of them; just add the two new ones).
//
// ── REQUIRED INCLUDES (add these two to the existing include block) ──────────
//   #include "SessionState.h"    ← already in KeyboardEditor.h, no extra line needed
//   #include "MacroLibrary.h"    ← already in KeyboardEditor.h, no extra line needed
//
// Both headers are transitively included through the new KeyboardEditor.h,
// so NO extra #include lines are needed in KeyboardEditor.cpp itself.
// ─────────────────────────────────────────────────────────────────────────────


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 1 — Constructor  (REPLACE existing KeyboardEditor::KeyboardEditor)
// ════════════════════════════════════════════════════════════════════════════════

KeyboardEditor::KeyboardEditor() {
    m_keycodeDb         = BuildKeycodeDatabase();
    m_keycodeMap        = BuildKeycodeMap();
    m_predefinedSchemes = BuildPredefinedSchemes();
    m_vkDown.assign(256, false);
    m_appStartTime = (float)ImGui::GetTime();

    // ── Build predefined macro library ───────────────────────────────────────
    m_macroLib = BuildMacroLibrary();
    m_libCategories.push_back("All");   // index 0 = show everything
    std::vector<std::string> cats = GetLibraryCategories(m_macroLib);
    for (int i = 0; i < (int)cats.size(); ++i)
        m_libCategories.push_back(cats[i]);

    // ── Auto-load last session ────────────────────────────────────────────────
    if (m_session.Load())
        TryAutoLoad(m_session.designPath, m_session.configPath);
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 2 — TryAutoLoad  (NEW — paste after the constructor)
// ════════════════════════════════════════════════════════════════════════════════

bool KeyboardEditor::TryAutoLoad(const std::string& designPath,
                                  const std::string& configPath)
{
    bool ok = false;

    if (!designPath.empty()) {
        KeyboardLayout tmp;
        if (LoadDesign(designPath, tmp)) {
            m_layout       = std::move(tmp);
            m_layoutLoaded = true;
            m_selectedKey  = -1;
            EnsureLayerSchemes();
            ok = true;
        }
    }

    if (!configPath.empty()) {
        KeyboardConfig tmp;
        if (LoadConfig(configPath, tmp)) {
            m_config       = std::move(tmp);
            m_configLoaded = true;
            m_configPath   = configPath;
            m_currentLayer = 0;
            m_selectedKey  = -1;
            m_dirty        = false;
            m_undoStack.clear();
            m_redoStack.clear();
            ParseMacrosFromConfig();
            EnsureLayerSchemes();
            ok = true;
        }
    }

    return ok;
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 3 — OpenDesign  (REPLACE existing OpenDesign)
// ════════════════════════════════════════════════════════════════════════════════

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
        m_session.SaveDesign(path);   // ← persist
    }
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 4 — OpenConfig  (REPLACE existing OpenConfig)
// ════════════════════════════════════════════════════════════════════════════════

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
        m_session.SaveConfig(path);   // ← persist
    }
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 5 — SaveConfig / SaveConfigAs  (REPLACE both existing functions)
// ════════════════════════════════════════════════════════════════════════════════

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
        m_dirty      = false;
        m_session.SaveConfig(path);           // ← persist
    }
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 6 — RenderMacroPanel  (REPLACE the existing function in full)
//
// The only meaningful change vs the original is the call to
// RenderMacroLibraryPanel(me) at the very bottom of the right pane,
// and the action list height being fixed to 160 px so the library panel
// gets the remaining space.
// ════════════════════════════════════════════════════════════════════════════════

void KeyboardEditor::RenderMacroPanel() {
    // ── Add macro / search bar ────────────────────────────────────────────────
    if (ImGui::Button("+ New Macro")) {
        MacroEntry me;
        char buf[32];
        snprintf(buf, sizeof(buf), "MACRO%02d", (int)m_macros.size());
        me.name = buf;
        m_macros.push_back(me);
        m_editingMacro = (int)m_macros.size() - 1;
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.f);
    ImGui::InputText("Search##msrch", m_macroSearchBuf, sizeof(m_macroSearchBuf));
    ImGui::Separator();

    float listH = ImGui::GetContentRegionAvail().y;

    // ── Left pane: macro list ─────────────────────────────────────────────────
    ImGui::BeginChild("MacroList", ImVec2(200.f, listH), true);

    for (int mi = 0; mi < (int)m_macros.size(); ++mi) {
        MacroEntry& me = m_macros[mi];

        if (m_macroSearchBuf[0]) {
            std::string lc = me.name;
            std::string srch(m_macroSearchBuf);
            for (size_t k = 0; k < lc.size();   ++k) lc[k]   = (char)tolower((unsigned char)lc[k]);
            for (size_t k = 0; k < srch.size();  ++k) srch[k] = (char)tolower((unsigned char)srch[k]);
            if (lc.find(srch) == std::string::npos) continue;
        }

        ImGui::PushID(mi);
        bool sel = (m_editingMacro == mi);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.45f, 0.80f, 1.f));

        if (ImGui::Selectable(me.name.c_str(), sel,
                              ImGuiSelectableFlags_AllowDoubleClick))
            m_editingMacro = mi;

        if (sel) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", MacroSummary(me).c_str());

        if (ImGui::BeginPopupContextItem("MacroCtx")) {
            static char renameBuf[64] = {};
            if (ImGui::IsWindowAppearing()) strncpy(renameBuf, me.name.c_str(), 63);
            ImGui::SetNextItemWidth(140.f);
            if (ImGui::InputText("Name##rn", renameBuf, sizeof(renameBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                me.name = renameBuf; m_dirty = true;
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

    // ── Right pane: action editor + library ───────────────────────────────────
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
    if (ImGui::SmallButton("Duplicate")) {
        MacroEntry copy = me;
        char buf[64];
        snprintf(buf, sizeof(buf), "%s_copy", me.name.c_str());
        copy.name = buf;
        m_macros.insert(m_macros.begin() + m_editingMacro + 1, copy);
        m_dirty = true;
    }
    ImGui::Separator();

    // ── Action list (fixed height) ────────────────────────────────────────────
    ImGui::Text("Actions (%d):", (int)me.actions.size());
    ImGui::BeginChild("##actions", ImVec2(0, 160.f), true);

    for (int ai = 0; ai < (int)me.actions.size(); ++ai) {
        MacroAction& a = me.actions[ai];
        ImGui::PushID(ai);

        if (ImGui::ArrowButton("##u", ImGuiDir_Up) && ai > 0) {
            std::swap(me.actions[ai], me.actions[ai - 1]); m_dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##d", ImGuiDir_Down) && ai < (int)me.actions.size() - 1) {
            std::swap(me.actions[ai], me.actions[ai + 1]); m_dirty = true;
        }
        ImGui::SameLine();

        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.f, 1.f), "[%s]",
                           MacroActionTypeName(a.type));
        ImGui::SameLine();

        switch (a.type) {
        case MacroActionType::KeyTap:
        case MacroActionType::KeyDown:
        case MacroActionType::KeyUp: {
            char kbuf[64]; strncpy(kbuf, a.keycode.c_str(), 63);
            ImGui::SetNextItemWidth(100.f);
            if (ImGui::InputText("##kc", kbuf, sizeof(kbuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                a.keycode = kbuf; m_dirty = true;
            }
            break;
        }
        case MacroActionType::TypeString: {
            char tbuf[256]; strncpy(tbuf, a.text.c_str(), 255);
            ImGui::SetNextItemWidth(180.f);
            if (ImGui::InputText("##txt", tbuf, sizeof(tbuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                a.text = tbuf; m_dirty = true;
            }
            break;
        }
        case MacroActionType::Delay: {
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
    ImGui::EndChild();   // ##actions

    ImGui::Spacing();

    // ── Manual add row ────────────────────────────────────────────────────────
    ImGui::TextDisabled("Add action manually:");
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::BeginCombo("##nat", MacroActionTypeName(m_newActionType))) {
        for (int t = 0; t < (int)MacroActionType::COUNT; ++t) {
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
        case MacroActionType::KeyUp:      a.keycode = m_newActionKey;   break;
        case MacroActionType::TypeString: a.text    = m_newActionText;  break;
        case MacroActionType::Delay:      a.delayMs = m_newActionDelay; break;
        default: break;
        }
        me.actions.push_back(a);
        m_dirty = true;
    }

    ImGui::Separator();

    // ── Predefined library panel ───────────────────────────────────────────────
    RenderMacroLibraryPanel(me);   // ← NEW

    ImGui::EndChild();  // MacroEdit
}


// ════════════════════════════════════════════════════════════════════════════════
// SECTION 7 — RenderMacroLibraryPanel  (NEW — add anywhere in the .cpp)
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
        ImGuiTableFlags_Borders      |
        ImGuiTableFlags_RowBg        |
        ImGuiTableFlags_ScrollY      |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##libtable", 4, tflags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Category",    ImGuiTableColumnFlags_WidthFixed,   110.f);
        ImGui::TableSetupColumn("Action",      ImGuiTableColumnFlags_WidthFixed,   120.f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Insert",      ImGuiTableColumnFlags_WidthFixed,    52.f);
        ImGui::TableHeadersRow();

        for (int fi = 0; fi < (int)filtered.size(); ++fi) {
            const MacroLibraryEntry& e = *filtered[fi];
            ImGui::TableNextRow();

            // Category cell — colour by category name hash
            ImGui::TableSetColumnIndex(0);
            float hue = 0.f;
            for (size_t k = 0; k < e.category.size(); ++k)
                hue = fmodf(hue + (float)(unsigned char)e.category[k] * 0.031f, 1.f);
            ImVec4 catCol;
            ImGui::ColorConvertHSVtoRGB(hue, 0.5f, 0.85f,
                                        catCol.x, catCol.y, catCol.z);
            catCol.w = 1.f;
            ImGui::TextColored(catCol, "%s", e.category.c_str());

            // Action name (selectable spans all columns for hover / dbl-click)
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
                    a.type = MacroActionType::KeyTap;
                    a.keycode = e.keycode;
                    break;
                case LibActionType::KeyDown:
                    a.type = MacroActionType::KeyDown;
                    a.keycode = e.keycode;
                    break;
                case LibActionType::KeyUp:
                    a.type = MacroActionType::KeyUp;
                    a.keycode = e.keycode;
                    break;
                case LibActionType::TypeString:
                    a.type = MacroActionType::TypeString;
                    a.text = e.text;
                    break;
                case LibActionType::Delay:
                    a.type = MacroActionType::Delay;
                    a.delayMs = e.delayMs;
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
