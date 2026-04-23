"""
map.exe  -  DOIO / QMK VIA keymap flasher  (unified me.json format)
====================================================================
Works with ALL DOIO keyboards: KB16-01, KB16B-02, KB64G-01, etc.

The keymap file ("me.json") uses ROW-MAJOR flat indexing:
  layer[i] has (rows * cols) entries, indexed as row*cols + col.
  This matches exactly what the DOIO Editor C++ app stores.

Usage:
  map.exe design.json                          -> flash me.json (auto)
  map.exe design.json my.json                  -> flash explicit file
  map.exe design.json --dump                   -> backup keyboard -> me_backup_DATE.json
  map.exe design.json --restore backup.json    -> restore a backup
  map.exe design.json --export                 -> export current keyboard as me.json
  map.exe --scan                               -> list all connected HID devices

Flags (append to flash command):
  --no-backup   skip auto-backup step
  --yes         skip confirmation prompt
"""

import sys
import os
import json
import datetime
import re
import atexit
import gc

# ─── Complete QMK Keycode Tables (Matched to Short-Name Design Style) ──────

QMK_KEYCODES: dict[str, int] = {
    # Basic Keycodes (0x0000 - 0x00FF)
    "KC_NO": 0x0000, "KC_TRNS": 0x0001,
    "KC_A": 0x0004, "KC_B": 0x0005, "KC_C": 0x0006, "KC_D": 0x0007,
    "KC_E": 0x0008, "KC_F": 0x0009, "KC_G": 0x000A, "KC_H": 0x000B,
    "KC_I": 0x000C, "KC_J": 0x000D, "KC_K": 0x000E, "KC_L": 0x000F,
    "KC_M": 0x0010, "KC_N": 0x0011, "KC_O": 0x0012, "KC_P": 0x0013,
    "KC_Q": 0x0014, "KC_R": 0x0015, "KC_S": 0x0016, "KC_T": 0x0017,
    "KC_U": 0x0018, "KC_V": 0x0019, "KC_W": 0x001A, "KC_X": 0x001B,
    "KC_Y": 0x001C, "KC_Z": 0x001D,
    "KC_1": 0x001E, "KC_2": 0x001F, "KC_3": 0x0020, "KC_4": 0x0021,
    "KC_5": 0x0022, "KC_6": 0x0023, "KC_7": 0x0024, "KC_8": 0x0025,
    "KC_9": 0x0026, "KC_0": 0x0027,
    "KC_ENT": 0x0028, "KC_ESC": 0x0029, "KC_BSPC": 0x002A, "KC_TAB": 0x002B,
    "KC_SPC": 0x002C, "KC_MINS": 0x002D, "KC_EQL": 0x002E, "KC_LBRC": 0x002F,
    "KC_RBRC": 0x0030, "KC_BSLS": 0x0031, "KC_SCLN": 0x0033, "KC_QUOT": 0x0034,
    "KC_GRV": 0x0035, "KC_COMM": 0x0036, "KC_DOT": 0x0037, "KC_SLSH": 0x0038,
    "KC_CAPS": 0x0039, "KC_F1": 0x003A, "KC_F2": 0x003B, "KC_F3": 0x003C,
    "KC_F4": 0x003D, "KC_F5": 0x003E, "KC_F6": 0x003F, "KC_F7": 0x0040,
    "KC_F8": 0x0041, "KC_F9": 0x0042, "KC_F10": 0x0043, "KC_F11": 0x0044,
    "KC_F12": 0x0045, "KC_PSCR": 0x0046, "KC_SCRL": 0x0047, "KC_SLCK": 0x0047,
    "KC_PAUS": 0x0048, "KC_INS": 0x0049, "KC_HOME": 0x004A, "KC_PGUP": 0x004B,
    "KC_DEL": 0x004C, "KC_END": 0x004D, "KC_PGDN": 0x004E, "KC_RGHT": 0x004F,
    "KC_LEFT": 0x0050, "KC_DOWN": 0x0051, "KC_UP": 0x0052,
    "KC_MUTE": 0x00A8, "KC_VOLU": 0x00A9, "KC_VOLD": 0x00AA,
    "KC_MNXT": 0x00AB, "KC_MPRV": 0x00AC, "KC_MSTP": 0x00AD, "KC_MPLY": 0x00AE,
    "KC_EJCT": 0x00B0, "KC_MAIL": 0x00B1, "KC_CALC": 0x00B2,
    "KC_WHOM": 0x00B5, "KC_WWW_BACK": 0x00B6,
    "KC_LCTL": 0x00E0, "KC_LSFT": 0x00E1, "KC_LALT": 0x00E2, "KC_LGUI": 0x00E3,
    "KC_RCTL": 0x00E4, "KC_RSFT": 0x00E5, "KC_RALT": 0x00E6, "KC_RGUI": 0x00E7,
    "KC_COPY": 0x007C, "KC_CUT": 0x007B, "KC_PASTE": 0x007D, "KC_UNDO": 0x007A,
    "KC_SELECT": 0x0077, "KC_MENU": 0x0065, "KC_LNUM": 0x0083,

    # Lighting / RGB (DOIO UI ranges)
    "BL_ON": 0x5CBB, "BL_OFF": 0x5CBC, "BL_DEC": 0x5CBD, "BL_INC": 0x5CBE,
    "BL_TOGG": 0x5CBF, "BL_BRTG": 0x5CC1, "RGB_TOG": 0x5CC2,
    "RGB_MOD": 0x5CC3, "RGB_RMOD": 0x5CC4, "RGB_HUI": 0x5CC5,
    "RGB_HUD": 0x5CC6, "RGB_SAI": 0x5CC7, "RGB_SAD": 0x5CC8,
    "RGB_VAI": 0x5CC9, "RGB_VAD": 0x5CCA, "RGB_SPI": 0x5CCB,
    "RGB_SPD": 0x5CCC, "RGB_M_P": 0x5CCD, "RGB_M_B": 0x5CCE,
    "RGB_M_R": 0x5CCF, "RGB_M_SW": 0x5CD0, "RGB_M_K": 0x5CD2,
    "RGB_M_X": 0x5CD3, "RGB_M_G": 0x5CD4, "KC_GESC": 0x5C16,
}

# Reverse mapping: picks the SHORTEST name to match design.json style
_REV_KEYCODES: dict[int, str] = {}
for _name, _code in QMK_KEYCODES.items():
    if _code not in _REV_KEYCODES or len(_name) < len(_REV_KEYCODES[_code]):
        _REV_KEYCODES[_code] = _name

EMPTY_KEY = "KC_NO"
TRNS_KEY = "KC_TRNS"

# Store API reference for cleanup
_api = None
_api_ref = None

def cleanup():
    """Force cleanup of HID resources and exit cleanly"""
    global _api, _api_ref
    try:
        if _api is not None:
            # Try to close any open device handles
            if hasattr(_api, 'close'):
                _api.close()
            elif hasattr(_api, '_device') and hasattr(_api._device, 'close'):
                _api._device.close()
        if _api_ref is not None:
            if hasattr(_api_ref, 'close'):
                _api_ref.close()
    except:
        pass
    # Force garbage collection
    gc.collect()

# Register cleanup to run on exit
atexit.register(cleanup)

# ─── Helper Functions for Dynamic Keycodes ──────────────────────────────────

def int_to_kc(val: int) -> str:
    if val == 0x0001: return "KC_TRNS"
    if val == 0x0000: return "KC_NO"
    if val in _REV_KEYCODES: return _REV_KEYCODES[val]

    # Shift mask (0x0200)
    if 0x0200 <= val <= 0x02FF:
        base = val & 0xFF
        return f"S({_REV_KEYCODES.get(base, f'0x{base:02X}')})"

    # DOIO/VIA Layer switching offsets
    if 0x5010 <= val <= 0x501F: return f"TO({val - 0x5010})"
    if 0x5200 <= val <= 0x521F: return f"TO({val & 0x1F})"
    if 0x5220 <= val <= 0x523F: return f"MO({val & 0x1F})"
    if 0x5240 <= val <= 0x525F: return f"DF({val & 0x1F})"
    if 0x5300 <= val <= 0x531F: return f"TG({val - 0x5300})"
    if 0x5400 <= val <= 0x541F: return f"OSL({val - 0x5400})"
    if 0x5800 <= val <= 0x581F: return f"TT({val - 0x5800})"
    
    # Macros
    if 0x5F12 <= val <= 0x5F21: return f"MACRO({val - 0x5F12})"

    # Layer-Tap / Mod-Tap
    if 0x4000 <= val <= 0x4FFF:
        return f"LT({(val >> 8) & 0x0F}, {int_to_kc(val & 0xFF)})"
    if 0x2000 <= val <= 0x3FFF:
        return f"MT({(val >> 8) & 0x1F}, {int_to_kc(val & 0xFF)})"

    return f"0x{val:04X}"

def kc_to_int(kc: str) -> int:
    kc = kc.strip().upper()
    if kc in QMK_KEYCODES: return QMK_KEYCODES[kc]
    
    # S(KC_...)
    match_s = re.match(r'^S\((KC_[A-Z0-9_]+)\)$', kc)
    if match_s: return 0x0200 | QMK_KEYCODES.get(match_s.group(1), 0)

    # Function parsing
    match_fn = re.match(r'^([A-Z_]+)\((\d+)\)$', kc)
    if match_fn:
        name, arg = match_fn.group(1), int(match_fn.group(2))
        bases = {"TO": 0x5010, "TG": 0x5300, "OSL": 0x5400, "TT": 0x5800, "MO": 0x5220, "DF": 0x5240, "MACRO": 0x5F12}
        if name in bases: return bases[name] + arg

    if kc.startswith("0X"):
        try: return int(kc, 16)
        except: pass
    return 0x0001

# ─── Layout helpers ──────────────────────────────────────────────────────────

class Layout:
    def __init__(self, definition: dict):
        self.name = definition.get("name", "Unknown")
        self.rows = definition.get("matrix", {}).get("rows", 0)
        self.cols = definition.get("matrix", {}).get("cols", 0)
        self.flat_size = self.rows * self.cols
        vid_raw = definition.get("vendorId", 0)
        pid_raw = definition.get("productId", 0)
        self.vid = int(str(vid_raw), 0) if isinstance(vid_raw, str) else int(vid_raw)
        self.pid = int(str(pid_raw), 0) if isinstance(pid_raw, str) else int(pid_raw)

def load_json(path):
    with open(path, "r", encoding="utf-8") as f: return json.load(f)

def save_json(data, path):
    with open(path, "w", encoding="utf-8") as f: json.dump(data, f, indent=2)

def detect_and_normalise_layers(raw_layers, layout):
    normalized = []
    for layer in raw_layers:
        l_list = list(layer)
        while len(l_list) < layout.flat_size: l_list.append(EMPTY_KEY)
        normalized.append(l_list[:layout.flat_size])
    return normalized

# ─── HID logic ──────────────────────────────────────────────────────────────

def scan_all_devices():
    import hid
    print("=" * 64 + "\n  HID DEVICE SCAN\n" + "=" * 64)
    for d in hid.enumerate():
        if d['usage_page'] == 0xFF60:
            print(f"  VID={d['vendor_id']:#06x} PID={d['product_id']:#06x} | {d['product_string']}")
    print("=" * 64 + "\n")
    # Force hid module cleanup
    if hasattr(hid, 'close'):
        hid.close()

def connect_keyboard(layout: Layout):
    global _api, _api_ref
    from qmk_via_api import KeyboardApi, scan_keyboards
    
    dev = next((d for d in scan_keyboards() if d.vendor_id == layout.vid and d.product_id == layout.pid), None)
    if not dev: 
        print("[ERROR] Keyboard not found.")
        sys.exit(1)
    
    _api_ref = dev  # Keep reference for cleanup
    _api = KeyboardApi(dev.vendor_id, dev.product_id, dev.usage_page)
    return _api, _api.get_layer_count()

def read_flat_layers(api, kb_layers, layout):
    layers = []
    for li in range(kb_layers):
        flat = []
        for r in range(layout.rows):
            for c in range(layout.cols):
                try:
                    val = api.get_key(li, r, c)
                    flat.append(int_to_kc(val))
                except: 
                    flat.append(EMPTY_KEY)
        layers.append(flat)
    return layers

def write_flat_layers_diff(api, kb_layers, layout, current, new, skip_confirm):
    changes = []
    for li in range(min(len(new), kb_layers)):
        for i in range(layout.flat_size):
            r, c = i // layout.cols, i % layout.cols
            if current[li][i] != new[li][i]:
                changes.append((li, r, c, new[li][i]))
    if not changes: 
        print("  Already matches file.")
        return 0
    if not skip_confirm and input(f"  Write {len(changes)} keys? [y/N] ").lower() != 'y': 
        return -1
    for li, r, c, kc in changes:
        api.set_key(li, r, c, kc_to_int(kc))
    print("[+] Written.")
    return len(changes)

# ─── Standard DOIO JSON format ────────────────────────────────────────────────

def make_standard_json(flat_layers, layout, definition):
    return {
        "name": definition.get("name", "DOIO"),
        "vendorProductId": (layout.vid << 16) | layout.pid,
        "macros": [""] * 16,
        "layers": flat_layers,
        "encoders": []
    }

# ─── Commands ────────────────────────────────────────────────────────────────

def cmd_dump(definition_path, out_path=None):
    global _api
    dfn = load_json(definition_path)
    lyt = Layout(dfn)
    api, kb_layers = connect_keyboard(lyt)
    layers = read_flat_layers(api, kb_layers, lyt)
    if not out_path: 
        out_path = f"me_backup_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    data = make_standard_json(layers, lyt, dfn)
    save_json(data, out_path)
    print(f"[+] Saved: {out_path}")
    
    # Close HID connection explicitly
    try:
        if hasattr(api, 'close'):
            api.close()
    except:
        pass

def cmd_flash(definition_path, keymap_path, skip_backup=False, skip_confirm=False):
    global _api
    dfn = load_json(definition_path)
    kmp = load_json(keymap_path)
    lyt = Layout(dfn)
    api, kb_layers = connect_keyboard(lyt)
    current = read_flat_layers(api, kb_layers, lyt)
    
    if not skip_backup:
        bak = make_standard_json(current, lyt, dfn)
        save_json(bak, f"me_backup_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json")
    
    write_flat_layers_diff(api, kb_layers, lyt, current, 
                          detect_and_normalise_layers(kmp.get("layers", []), lyt), 
                          skip_confirm)
    
    # Close HID connection explicitly
    try:
        if hasattr(api, 'close'):
            api.close()
    except:
        pass

# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    args = sys.argv[1:]
    
    # Check for --yes flag (non-interactive mode)
    non_interactive = "--yes" in args
    
    try:
        if "--scan" in args: 
            scan_all_devices()
            return
            
        if not args: 
            print(__doc__)
            return
            
        dfn_path = args[0]
        
        if "--dump" in args: 
            cmd_dump(dfn_path)
        elif "--export" in args: 
            cmd_dump(dfn_path, "me.json")
        elif "--restore" in args: 
            cmd_flash(dfn_path, args[args.index("--restore")+1], skip_backup=True)
        else:
            kmp = args[1] if len(args) > 1 and not args[1].startswith("-") else "me.json"
            cmd_flash(dfn_path, kmp, "--no-backup" in args, "--yes" in args)
        
    except Exception as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        sys.exit(1)
    finally:
        # Always clean up resources
        cleanup()
    
    # Skip the "Press Enter" prompt in non-interactive mode
    if not non_interactive:
        try:
            input("\nPress Enter to exit...")
        except:
            pass

if __name__ == "__main__": 
    main()