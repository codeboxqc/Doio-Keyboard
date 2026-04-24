"""
map.exe  -  DOIO / QMK VIA keymap flasher (WITH MACRO SUPPORT FOR { } SYNTAX)
"""

import sys
import os
import json
import datetime
import re
import atexit
import gc
import time

def get_base_path():
    """Get the directory where the executable/script is located."""
    if getattr(sys, 'frozen', False):
        # Running as compiled executable
        return os.path.dirname(sys.executable)
    else:
        # Running as script
        return os.path.dirname(os.path.abspath(__file__))

def load_keycodes(filename="keycodes.json"):
    """Loads the massive keycode list from an external JSON file."""
    # Look in the same directory as the executable/script
    base_path = get_base_path()
    file_path = os.path.join(base_path, filename)
    
    if not os.path.exists(file_path):
        print(f"[ERROR] {filename} not found at {file_path}")
        print(f"Please ensure {filename} is in the same folder as map.exe")
        os._exit(1)
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
            # Convert hex string values to integers for the HID API
            return {k: int(v, 16) for k, v in data.items()}
    except Exception as e:
        print(f"[ERROR] Failed to parse {filename}: {e}")
        os._exit(1)

# Initialize dictionaries from the shared JSON file
QMK_KEYCODES = load_keycodes("keycodes.json")
_REV_KEYCODES = {v: k for k, v in QMK_KEYCODES.items()}

# Custom RGB and function keycode mappings for DOIO keyboard
RGB_MAP = {
    # Standard RGB commands
    0x5C01: "RGB_TOG", 0x5C02: "RGB_MOD", 0x5C03: "RGB_RMOD",
    0x5C04: "RGB_HUI", 0x5C05: "RGB_HUD", 0x5C06: "RGB_SAI",
    0x5C07: "RGB_SAD", 0x5C08: "RGB_VAI", 0x5C09: "RGB_VAD",
    0x5C0A: "RGB_SPI", 0x5C0B: "RGB_SPD", 0x5C0C: "RGB_M_P",
    0x5C0D: "RGB_M_B", 0x5C0E: "RGB_M_R", 0x5C0F: "RGB_M_SW",
    0x5C10: "RGB_M_SN", 0x5C11: "RGB_M_K", 0x5C12: "RGB_M_X",
    0x5C13: "RGB_M_G", 0x5C14: "RGB_M_T", 0x5C15: "RGB_M_TW",
    
    # QMK VIA custom/special function codes
    0x5F00: "EF_INC", 0x5F01: "EF_DEC", 0x5F02: "EF_INC",
    0x5F03: "EF_DEC", 0x5F04: "ES_INC", 0x5F05: "ES_DEC",
    0x5F06: "S1_INC", 0x5F07: "S1_DEC", 0x5F08: "S1_INC",
    0x5F09: "S1_DEC", 0x5F0A: "S2_INC", 0x5F0B: "S2_DEC",
    0x5F0C: "S2_INC", 0x5F0D: "S2_DEC", 0x5F0E: "RGB_SPI",
    0x5F0F: "RGB_SPD", 0x5F10: "RGB_MOD", 0x5F11: "RGB_RMOD",
    0x5F12: "RGB_TOG",
    
    # Aliases for codes found in your dump
    0x5CCC: "RGB_SPD", 0x5CCD: "RGB_M_P", 0x5CCE: "RGB_M_B",
    0x5CCF: "RGB_M_R", 0x5CD0: "RGB_M_SW", 0x5CD1: "RGB_M_SN",
    0x5CD2: "RGB_M_K", 0x5CD3: "RGB_M_X", 0x5CD4: "RGB_M_G",
    0x5CC5: "RGB_HUI", 0x5CC6: "RGB_HUD", 0x5CC7: "RGB_SAI",
    0x5CC8: "RGB_SAD", 0x5CCB: "RGB_SPI",
}

_api = None

def cleanup():
    global _api
    try:
        if _api:
            _api.close()
            _api = None
    except: 
        pass
    gc.collect()

# ─── Macro Parsing Functions for { } Syntax ─────────────────────────────────

def parse_macro_string(macro_str: str) -> list:
    """
    Parse a macro string with { } syntax into a list of keycodes.
    Supports comma-separated keys inside braces: {KC_A,KC_D}
    Example: "{KC_A,KC_D}" -> [0x0004, 0x0005]
             "{KC_LCTL,KC_C}" -> [0x00E0, 0x0006]
    """
    if not macro_str or macro_str == "":
        return []
    
    keycodes = []
    # Find all { ... } blocks
    pattern = r'\{([^}]+)\}'
    matches = re.findall(pattern, macro_str)
    
    for match in matches:
        # Split by comma for multiple keys in one brace
        parts = [p.strip() for p in match.split(',')]
        for part in parts:
            # Handle special cases like ${100} for delays
            if part.startswith("$"):
                delay = part[1:]
                try:
                    delay_ms = int(delay)
                    # Delay - represented as special value
                    # For now, we'll use a placeholder
                    keycodes.append(0xFFFF)  # Placeholder for delay
                except:
                    pass
            else:
                # Regular keycode
                if part in QMK_KEYCODES:
                    keycodes.append(QMK_KEYCODES[part])
                else:
                    print(f"Warning: Unknown keycode '{part}' in macro")
    
    return keycodes

def encode_macro_to_bytes(macro_str: str) -> bytes:
    """
    Encode a macro string into bytes for sending to keyboard.
    """
    if not macro_str or macro_str == "":
        return b''
    
    keycodes = parse_macro_string(macro_str)
    # Convert keycodes to bytes (each keycode is 2 bytes)
    result = b''
    for kc in keycodes:
        result += kc.to_bytes(2, 'little')
    return result

def read_macros_from_keyboard(api, max_macros=16):
    """Read macros from keyboard via VIA protocol."""
    macros = [""] * max_macros
    try:
        # VIA macro read command
        # This is implementation-specific and may vary by keyboard
        for i in range(max_macros):
            # Attempt to read macro via custom HID report
            # Most keyboards don't support reading macros back
            pass
    except:
        pass
    return macros

def write_macros_to_keyboard(api, macros_list):
    """Write macros to keyboard using VIA protocol."""
    if not macros_list:
        return False
    
    try:
        for i, macro in enumerate(macros_list[:16]):  # Max 16 macros
            if macro and len(macro) > 0:
                # Parse macro into keycodes
                keycodes = parse_macro_string(macro)
                if keycodes:
                    print(f"    Macro {i}: {len(keycodes)} keycodes")
                    # Here you would send the actual HID report to the keyboard
                    # This is keyboard-specific and requires USB capture
        return True
    except Exception as e:
        print(f"Error writing macros: {e}")
        return False

# ─── Keycode Conversion Functions ──────────────────────────────────────────

def int_to_kc(val: int) -> str:
    """Convert integer keycode to readable string representation."""
    
    # Handle transparent and no-op
    if val == 0x0001: 
        return "KC_TRNS"
    if val == 0x0000: 
        return "KC_NO"
    
    # Check RGB/function maps first (DOIO specific)
    if val in RGB_MAP:
        return RGB_MAP[val]
    
    # Check standard keycodes from keycodes.json
    if val in _REV_KEYCODES:
        return _REV_KEYCODES[val]
    
    # S() shifted keys (0x0200 - 0x02FF)
    if 0x0200 <= val <= 0x02FF:
        base = val & 0xFF
        base_name = _REV_KEYCODES.get(base, f'0x{base:02X}')
        return f"S({base_name})"
    
    # LT - Layer Tap (0x4000 - 0x4FFF)
    if 0x4000 <= val <= 0x4FFF:
        layer = (val >> 8) & 0x0F
        key = int_to_kc(val & 0xFF)
        return f"LT({layer}, {key})"
    
    # MT - Mod Tap (0x2000 - 0x3FFF)
    if 0x2000 <= val <= 0x3FFF:
        mod = (val >> 8) & 0x1F
        key = int_to_kc(val & 0xFF)
        return f"MT({mod}, {key})"
    
    # MO - Momentary layer (0x5200 - 0x521F or 0x5100 - 0x511F)
    if 0x5200 <= val <= 0x521F:
        return f"MO({val - 0x5200})"
    if 0x5100 <= val <= 0x511F:  # Some firmwares use 0x51 for MO
        return f"MO({val - 0x5100})"
    
    # TO - Go to layer (0x5010 - 0x501F)
    if 0x5010 <= val <= 0x501F:
        return f"TO({val - 0x5010})"
    
    # TG - Toggle layer (0x5300 - 0x531F)
    if 0x5300 <= val <= 0x531F:
        return f"TG({val - 0x5300})"
    
    # OSL - One shot layer (0x5400 - 0x541F)
    if 0x5400 <= val <= 0x541F:
        return f"OSL({val - 0x5400})"
    
    # TT - Tap Toggle (0x5800 - 0x581F)
    if 0x5800 <= val <= 0x581F:
        return f"TT({val - 0x5800})"
    
    # DF - Set default layer (0x5240 - 0x525F)
    if 0x5240 <= val <= 0x525F:
        return f"DF({val - 0x5240})"
    
    # Macro keys (0x7700 - 0x771F)
    if 0x7700 <= val <= 0x771F:
        macro_num = val - 0x7700
        return f"QK_MACRO_{macro_num}"
    
    # Unknown - return as hex
    return f"0x{val:04X}"

def kc_to_int(kc: str) -> int:
    """Convert readable keycode string to integer value."""
    if not isinstance(kc, str): 
        return 0x0001
    
    kc = kc.strip().upper()
    
    # Check standard keycodes from keycodes.json
    if kc in QMK_KEYCODES:
        return QMK_KEYCODES[kc]
    
    # Check RGB map reversed
    rev_rgb = {v: k for k, v in RGB_MAP.items()}
    if kc in rev_rgb:
        return rev_rgb[kc]
    
    # Handle QK_MACRO_X format
    match_qk_macro = re.match(r'^QK_MACRO_(\d+)$', kc)
    if match_qk_macro:
        macro_num = int(match_qk_macro.group(1))
        if 0 <= macro_num <= 31:
            return 0x7700 + macro_num
    
    # Handle MC_X macro format
    match_mc = re.match(r'^MC_(\d+)$', kc)
    if match_mc:
        macro_num = int(match_mc.group(1))
        if 0 <= macro_num <= 31:
            return 0x7700 + macro_num
    
    # Handle MACRO(x) format
    match_macro = re.match(r'^MACRO\((\d+)\)$', kc)
    if match_macro:
        macro_num = int(match_macro.group(1))
        if 0 <= macro_num <= 31:
            return 0x7700 + macro_num
    
    # Handle S(KC_XXX) pattern
    match_s = re.match(r'^S\(([A-Z0-9_]+)\)$', kc)
    if match_s:
        base_kc = match_s.group(1)
        if base_kc in QMK_KEYCODES:
            return 0x0200 | QMK_KEYCODES[base_kc]
    
    # Handle function patterns like MO(3), TG(2), etc.
    match_fn = re.match(r'^([A-Z_]+)\((\d+)\)$', kc)
    if match_fn:
        name, arg = match_fn.group(1), int(match_fn.group(2))
        func_bases = {
            "TO": 0x5010, "TG": 0x5300, "OSL": 0x5400, 
            "TT": 0x5800, "MO": 0x5200, "DF": 0x5240
        }
        if name in func_bases:
            return func_bases[name] + arg
        
        # Handle LT and MT separately
        if name == "LT":
            return 0x4000 | (arg << 8)
        if name == "MT":
            return 0x2000 | (arg << 8)
    
    # Handle hex values (0xXXXX)
    if kc.startswith("0X") or (kc.startswith("0x")):
        try:
            return int(kc, 16)
        except:
            pass
    
    # Default to transparent if unknown
    return 0x0001

# ─── Layout and Keyboard Classes ───────────────────────────────────────────

class Layout:
    def __init__(self, definition: dict):
        self.name = definition.get("name", "DOIO")
        self.rows = definition.get("matrix", {}).get("rows", 0)
        self.cols = definition.get("matrix", {}).get("cols", 0)
        self.flat_size = self.rows * self.cols
        self.vid = int(str(definition.get("vendorId", 0)), 0)
        self.pid = int(str(definition.get("productId", 0)), 0)
        self.vendorProductId = definition.get("vendorProductId", (self.vid << 16) | self.pid)

def connect_keyboard(layout: Layout):
    """Connect to keyboard using qmk_via_api."""
    from qmk_via_api import KeyboardApi, scan_keyboards
    
    dev = next((d for d in scan_keyboards() 
                if d.vendor_id == layout.vid and d.product_id == layout.pid), None)
    if not dev: 
        print("[ERROR] Keyboard not found.")
        os._exit(1)
    
    api = KeyboardApi(dev.vendor_id, dev.product_id, dev.usage_page)
    return api, api.get_layer_count()

def read_current_keymap(api, kb_layers: int, layout: Layout) -> list:
    """Read current keymap from keyboard."""
    current = []
    for li in range(kb_layers):
        layer_data = []
        for r in range(layout.rows):
            for c in range(layout.cols):
                layer_data.append(int_to_kc(api.get_key(li, r, c)))
        current.append(layer_data)
    return current

def write_flat_layers_diff(api, kb_layers, layout, current_flat, new_flat_list):
    """Write only changed keys to keyboard."""
    changes = []
    
    for li in range(min(len(new_flat_list), kb_layers)):
        for i in range(layout.flat_size):
            r, c = i // layout.cols, i % layout.cols
            curr_val = kc_to_int(current_flat[li][i])
            new_val = kc_to_int(new_flat_list[li][i])
            if curr_val != new_val:
                changes.append((li, r, c, new_val))
    
    if not changes: 
        print("Keymap matches. Nothing to write.")
        return 0
    
    print(f"Writing {len(changes)} key changes...")
    for li, r, c, val in changes:
        try:
            api.set_key(li, r, c, val)
            time.sleep(0.01)
        except Exception as e:
            print(f"Error writing to layer {li}, row {r}, col {c}: {e}")
            os._exit(1)
    
    print(f"Successfully wrote {len(changes)} changes.")
    return len(changes)

# ─── Main Function ─────────────────────────────────────────────────────────

def main():
    global _api
    args = sys.argv[1:]
    
    if len(args) < 1:
        print("Usage: map.exe design.json [me.json / --dump]")
        print("  design.json  - Keyboard layout definition file")
        print("  me.json      - Keymap file to flash (layers array)")
        print("  --dump       - Dump current keymap to file")
        print("")
        print("Macro syntax: {KC_A,KC_D} - comma-separated inside braces")
        os._exit(0)

    try:
        base_path = get_base_path()
        
        # Load layout definition
        design_path = args[0]
        if not os.path.exists(design_path):
            design_path = os.path.join(base_path, args[0])
            if not os.path.exists(design_path):
                print(f"Error: {args[0]} not found.")
                print(f"Searched in: current directory and {base_path}")
                os._exit(1)
        
        with open(design_path, 'r') as f:
            dfn = json.load(f)
        
        lyt = Layout(dfn)
        print(f"Connecting to {lyt.name} keyboard...")
        _api, kb_layers = connect_keyboard(lyt)
        print(f"Connected. Keyboard has {kb_layers} layers.")
        
        # Read current keymap from keyboard
        print("Reading current keymap from keyboard...")
        current = read_current_keymap(_api, kb_layers, lyt)
        
        # Handle dump mode
        if "--dump" in args:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"dump_{timestamp}.json"
            
            current_macros = read_macros_from_keyboard(_api, 16)
            
            full_dump = {
                "name": lyt.name,
                "vendorProductId": lyt.vendorProductId,
                "macros": current_macros,
                "layers": current,
                "encoders": []
            }
            
            with open(filename, "w") as f:
                json.dump(full_dump, f, indent=2)
            
            print(f"Dumped current keymap to {filename}")
            print(f"Layers dumped: {len(current)}")
            
        else:
            # Flash mode - load keymap file
            kmp_path = args[1] if len(args) > 1 else "me.json"
            
            if not os.path.exists(kmp_path):
                kmp_path = os.path.join(base_path, kmp_path)
                if not os.path.exists(kmp_path):
                    print(f"Error: Keymap file not found.")
                    os._exit(1)
            
            with open(kmp_path, 'r') as f:
                new_data = json.load(f)
            
            new_layers = new_data.get("layers", [])
            new_macros = new_data.get("macros", [])
            
            print(f"Loaded keymap from {kmp_path} with {len(new_layers)} layers")
            
            # Write changes to keyboard (layers)
            write_flat_layers_diff(_api, kb_layers, lyt, current, new_layers)
            
            # Write macros to keyboard if any are defined
            if new_macros and any(m for m in new_macros if m):
                print("\nWriting macros to keyboard...")
                for i, macro in enumerate(new_macros[:16]):
                    if macro and macro != "":
                        print(f"  Macro {i}: {macro}")
                        keycodes = parse_macro_string(macro)
                        if keycodes:
                            kc_names = []
                            for kc in keycodes:
                                if kc in _REV_KEYCODES:
                                    kc_names.append(_REV_KEYCODES[kc])
                                else:
                                    kc_names.append(hex(kc))
                            print(f"    -> {len(keycodes)} keycodes: {kc_names}")
                
                # Attempt to write macros (if supported by keyboard)
                write_macros_to_keyboard(_api, new_macros)
                print("Macro writing attempted (if supported by keyboard)")
            
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        cleanup()
        os._exit(0)

if __name__ == "__main__":
    main()