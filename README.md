WORK in Progress


DOIO kb16b-02 V2 .json KB16-01

## 🚀 Quick Start

### Windows Users (Pre-built Binary)

1. **Download** the latest release from [Releases](#) page
2. **Extract** all files to a folder (keep `map.exe` alongside `DOIOEditor.exe`)
3. **Run** `DOIOEditor.exe`
4. **Load** your `design.json` (File → Open Design)
5. **Load or create** a keymap (File → Open Config)
6. **Start editing!**

### What You Need

| Item | Description | Where to Get |
|------|-------------|--------------|
| `design.json` | Keyboard layout definition | Download from DOIO or create yourself |
| `me.json` | Keymap configuration | Created/edited by the editor |
| `map.exe` | Flash utility | Built from `map.py` (included) |

### First-Time Setup

1. **Get your keyboard's design.json**
   - Check DOIO's official repository
   - Or use the example in `examples/` folder
   - Must match your exact keyboard model

2. **Create a new keymap**
   - File → Open Config (choose existing or create new)
   - Editor creates default configuration with KC_TRNS for physical keys

3. **Test your setup**
   - Click a key to select it
   - Pick a keycode from the picker
   - See the label update on the keyboard visualization

4. **Flash to keyboard**
   - Save your config (Ctrl+S)
   - File → Flash to Keyboard (Ctrl+Shift+S)
   - Confirm the operation

Preview editor
<img width="1913" height="1077" alt="Untitled" src="https://github.com/user-attachments/assets/be1e9ba4-80d3-41ba-b641-83a9a88507d5" />


test working 
<img width="1000" height="450" alt="1776947733886" src="https://github.com/user-attachments/assets/ba0f86a5-d285-4792-b108-a1c55ca763b6" />
<img width="1000" height="450" alt="1776947733913" src="https://github.com/user-attachments/assets/a4bae8e6-8fd8-40eb-b979-9c61ddc5fc55" />

## 🛠️ Building from Source

### System Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Windows 10/11 (64-bit) |
| **Compiler** | Visual Studio 2022 (C++17) |
| **Python** | 3.8 or later |
| **RAM** | 4GB minimum (8GB recommended) |
| **Disk** | 500MB free space |

### Dependencies

#### C++ Libraries (included or system)
- **Dear ImGui** (v1.90+ with docking branch)
- **DirectX 11** (Windows SDK)
- **nlohmann/json** (header-only)

#### Python Packages (for map.exe)
```bash
pip install hidapi qmk-via pyinstaller

Step-by-Step Build
1. Clone Repository
bash
git clone https://github.com/yourusername/doio-keyboard-editor.git
cd doio-keyboard-editor
2. Setup Submodules (if any)
bash
git submodule update --init --recursive
3. Install Python Dependencies
bash
pip install -r requirements.txt
Or manually:

bash
pip install hidapi qmk-via
4. Open Visual Studio Solution
Open DOIOEditor.sln in Visual Studio 2022

Ensure platform is set to x64 (not x86)

Select configuration: Release for distribution, Debug for development

5. Build the Project
Via Visual Studio:

Build → Build Solution (Ctrl+Shift+B)

Via Command Line:

bash
msbuild DOIOEditor.sln /p:Configuration=Release /p:Platform=x64 /m
6. Build map.exe
bash
pyinstaller --onefile --console --name map.exe --hidden-import=hid --hidden-import=qmk_via_api map.py
7. Run
Copy DOIOEditor.exe and map.exe to the same folder

Run DOIOEditor.exe



Project Structure
text
doio-keyboard-editor/
├── DOIOEditor.sln          # Visual Studio solution
├── Doio.cpp                # Main entry point, ImGui setup
├── Doio.h                  # Main header
├── KeyboardEditor.cpp      # Core editor logic
├── KeyboardEditor.h        # Editor declarations
├── KeyboardConfig.cpp      # JSON loading/saving
├── KeyboardConfig.h        # Config structures
├── KeycodeDefs.h           # Keycode database
├── Keytest.cpp             # Key tester implementation
├── Keytest.h               # Key tester header
├── LedScheme.h             # LED configuration
├── MacroLibrary.h          # Predefined macros (200+)
├── SessionState.h          # Session persistence
├── font.cpp                # Font loading
├── Resource.h              # Windows resources
├── map.py                  # Flash utility (Python)
├── examples/               # Example design.json files
│   ├── kb16-01.json
│   ├── kb16b-02.json
│   └── kb64g-01.json
└── README.md               # This file



🔑 Keycode Reference
Basic Keycodes
Code	Label	Description
KC_NO	---	No action (empty)
KC_TRNS	▽	Transparent (pass through)
KC_A...KC_Z	A-Z	Letters
KC_0...KC_9	0-9	Numbers
KC_ENT	Ent	Enter/Return
KC_ESC	Esc	Escape
KC_BSPC	⌫	Backspace
KC_TAB	Tab	Tab
KC_SPC	Spc	Space
KC_DEL	Del	Delete
KC_INS	Ins	Insert
Modifiers
Code	Label	Description
KC_LCTL	LCtl	Left Control
KC_LSFT	LSft	Left Shift
KC_LALT	LAlt	Left Alt
KC_LGUI	LGui	Left GUI (Windows/Cmd)
KC_RCTL	RCtl	Right Control
KC_RSFT	RSft	Right Shift
KC_RALT	RAlt	Right Alt
KC_RGUI	RGui	Right GUI
Layer Actions
Code	Description	Example
TO(n)	Jump to layer n	TO(1)
MO(n)	Momentarily activate layer n	MO(2)
TG(n)	Toggle layer n on/off	TG(1)
DF(n)	Set default layer to n	DF(0)
OSL(n)	One-shot layer n	OSL(3)
TT(n)	Tap-toggle layer n	TT(1)
Advanced QMK
Code	Description
MT(mod, kc)	Mod-Tap (hold = mod, tap = key)
LT(layer, kc)	Layer-Tap (hold = layer, tap = key)
S(kc)	Shift + keycode
MACRO(n)	Execute macro n




