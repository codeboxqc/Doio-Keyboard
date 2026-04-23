
---

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

---

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
