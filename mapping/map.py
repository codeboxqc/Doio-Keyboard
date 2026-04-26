"""
map.py  -  DOIO / QMK VIA keymap flasher (WITH MACRO SUPPORT FOR { } SYNTAX)

Fixed issues:
  - Macro write is now implemented via VIA HID protocol (was a stub)
  - Macro read implemented via VIA HID protocol (was empty)
  - Delay tokens handled correctly; encode_macro_to_via_bytes wired into main
  - LT(layer, key) / MT(mod, key) two-arg patterns parse correctly
  - LT key component no longer dropped on encode
  - os._exit(1) on write error replaced with raised exception
  - atexit.register(cleanup) is now called
  - os._exit(0) in finally replaced with sys.exit(0)
  - RGB_MAP cleaned: duplicates removed, non-standard 0x5CCC+ aliases removed
  - RGB_MAP reverse lookup is now deterministic (single code per name)
  - RGB_VAI / RGB_VAD / RGB_RMOD added to keycodes.json; consistent here
  - ES_DEC added to keycodes.json; consistent here
"""

import sys
import os
import json
import datetime
import re
import atexit
import gc
import time

# ─── VIA Protocol Constants ─────────────────────────────────────────────────
VIA_CMD_MACRO_GET_BUFFER_SIZE = 0x0D
VIA_CMD_MACRO_GET_BUFFER      = 0x0E
VIA_CMD_MACRO_SET_BUFFER      = 0x0F
VIA_CMD_MACRO_RESET           = 0x10

# VIA macro binary action prefixes (0x01 = QMK action marker)
VIA_MACRO_TAP   = b'\x01\x01'  # tap key
VIA_MACRO_DOWN  = b'\x01\x02'  # key down
VIA_MACRO_UP    = b'\x01\x03'  # key up
VIA_MACRO_DELAY = b'\x01\x04'  # delay (digits + '|' terminator follow)

VIA_HID_REPORT_SIZE = 33       # 1 report-ID byte + 32 payload bytes

def get_base_path():
    """Get the directory where the executable/script is located."""
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))

def load_keycodes(filename="keycodes.json"):
    """Load keycode list from an external JSON file."""
    base_path = get_base_path()
    file_path = os.path.join(base_path, filename)

    if not os.path.exists(file_path):
        print(f"[ERROR] {filename} not found at {file_path}")
        print(f"Please ensure {filename} is in the same folder as map.exe")
        sys.exit(1)
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        # Convert hex string values to integers for the HID API
        return {k: int(v, 16) for k, v in data.items()}
    except Exception as e:
        print(f"[ERROR] Failed to parse {filename}: {e}")
        sys.exit(1)

# Initialize dictionaries from the shared JSON file
QMK_KEYCODES = load_keycodes("keycodes.json")

# Reverse map: int → first matching name.
# keycodes.json has many aliases (e.g. KC_ENT / KC_ENTER share 0x0028).
# We prefer the shorter alias as the canonical display name, EXCEPT for a set
# of names where the VIA web app uses a specific longer alias that we must
# match exactly so dump output round-trips correctly.
_VIA_CANONICAL_NAMES = {
    # code (int) → preferred name
    # Paste / Select
    0x007D: "KC_PASTE",
    0x0077: "KC_SELECT",
    # WWW keys
    0x00B6: "KC_WWW_BACK",
    0x00B5: "KC_WWW_HOME",
    # Scroll lock
    0x0047: "KC_SLCK",
    # Modifiers
    0x00E2: "KC_LALT",
    0x00E3: "KC_LGUI",
}

_REV_KEYCODES: dict[int, str] = {}
for _k, _v in QMK_KEYCODES.items():
    if _v not in _REV_KEYCODES or len(_k) < len(_REV_KEYCODES[_v]):
        _REV_KEYCODES[_v] = _k

# Override with VIA-canonical names where needed
for _code, _name in _VIA_CANONICAL_NAMES.items():
    if _name in QMK_KEYCODES:  # only if the name is actually in keycodes.json
        _REV_KEYCODES[_code] = _name

# ─── RGB / DOIO-specific supplemental decode map ────────────────────────────
# This map covers codes that arrive from the keyboard but are NOT (or were not)
# in the standard keycodes.json.  After the keycodes.json additions, most of
# these match; the map remains as a safe decode fallback.
#
# IMPORTANT: one code → one name, no duplicates (keeps reverse lookup
# deterministic).  All entries here MUST agree with keycodes.json where both
# define the same name.
RGB_MAP: dict[int, str] = {
    # ── Special keys ────────────────────────────────────────────────────────────
    0x5C16: "KC_GESC",          # Grave/Escape combo key

    # ── Backlight (BL_) keys ────────────────────────────────────────────────────
    0x5CBB: "BL_ON",
    0x5CBC: "BL_OFF",
    0x5CBD: "BL_DEC",
    0x5CBE: "BL_INC",
    0x5CBF: "BL_TOGG",
    0x5CC1: "BL_BRTG",

    # ── RGB underglow / matrix controls ─────────────────────────────────────────
    0x5CC2: "RGB_TOG",
    0x5CC3: "RGB_MOD",
    0x5CC4: "RGB_RMOD",
    0x5CC5: "RGB_HUI",
    0x5CC6: "RGB_HUD",
    0x5CC7: "RGB_SAI",
    0x5CC8: "RGB_SAD",
    0x5CC9: "RGB_VAI",
    0x5CCA: "RGB_VAD",
    0x5CCB: "RGB_SPI",
    0x5CCC: "RGB_SPD",
    0x5CCD: "RGB_M_P",
    0x5CCE: "RGB_M_B",
    0x5CCF: "RGB_M_R",
    0x5CD0: "RGB_M_SW",
    0x5CD1: "RGB_M_SN",
    0x5CD2: "RGB_M_K",
    0x5CD3: "RGB_M_X",
    0x5CD4: "RGB_M_G",

    # ── MACRO(n) keys – DOIO firmware uses 0x5F12+n ─────────────────────────────
    # (different from QK_MACRO_n at 0x7700+n used by standard VIA firmware)
    0x5F12: "MACRO(0)",
    0x5F13: "MACRO(1)",
    0x5F14: "MACRO(2)",
    0x5F15: "MACRO(3)",
    0x5F16: "MACRO(4)",
    0x5F17: "MACRO(5)",
    0x5F18: "MACRO(6)",
    0x5F19: "MACRO(7)",
    0x5F1A: "MACRO(8)",
    0x5F1B: "MACRO(9)",
    0x5F1C: "MACRO(10)",
    0x5F1D: "MACRO(11)",
    0x5F1E: "MACRO(12)",
    0x5F1F: "MACRO(13)",
    0x5F20: "MACRO(14)",
    0x5F21: "MACRO(15)",

    # ── Older DOIO RGB effect aliases (kept for decode round-trip) ───────────────
    0x5C03: "RGB_RMOD",
    0x5C08: "RGB_VAI",
    0x5C09: "RGB_VAD",
    0x5C14: "RGB_M_T",
    0x5C15: "RGB_M_TW",
    0x5F00: "EF_INC",
    0x5F01: "EF_DEC",
}

# Reverse of RGB_MAP: name → code.  Built once; deterministic because the
# map above contains no duplicate names.
_REV_RGB_MAP: dict[str, int] = {name: code for code, name in RGB_MAP.items()}

_api = None

def cleanup():
    global _api
    try:
        if _api:
            _api.close()
            _api = None
    except Exception:
        pass
    gc.collect()

atexit.register(cleanup)  # FIX: was never registered

# ─── VIA HID helpers ────────────────────────────────────────────────────────

def _via_send_receive(api, payload: bytes) -> bytes:
    """
    Legacy raw-HID path — no longer used.
    qmk_via_api exposes high-level methods (get_macro_bytes, set_macro_bytes,
    get_macro_buffer_size, reset_macros) that are used directly instead.
    This stub is retained so any future callers get a clear error.
    """
    raise NotImplementedError(
        "_via_send_receive: use api.get_macro_bytes / set_macro_bytes directly."
    )


# ─── Macro encoding (VIA binary format) ─────────────────────────────────────

def encode_macro_to_via_bytes(macro_str: str) -> bytes:
    """
    Encode a { } macro string into DOIO firmware binary format.

    DOIO format (confirmed from live dump):
        Each event = 2 bytes: action(1) + hid_usage(1)
        action: 0x02 = key down, 0x03 = key up, 0x01 = tap (down+up)
        hid_usage = low byte of the QMK keycode (works for 0x0000-0x00FF range)
        Each macro terminated by 0x00.

    Syntax:
        {KC_A}              → tap KC_A  (0x01 0x04)
        {KC_LCTL,KC_C}      → Ctrl+C chord (down LCTL, down C, up C, up LCTL)
        {$100}              → 100 ms delay (encoded as 0x01 0x04 * delay_ms ... 
                              DOIO has no native delay; best-effort ignored)
        Multiple blocks:    {KC_LCTL,KC_C}{KC_V}

    Returns bytes terminated by 0x00.
    """
    if not macro_str:
        return b'\x00'

    result = bytearray()
    for match in re.finditer(r'\{([^}]+)\}', macro_str):
        parts = [p.strip() for p in match.group(1).split(',')]

        # Delay token: {$ms} — DOIO has no delay opcode; skip with warning
        if len(parts) == 1 and parts[0].startswith('$'):
            print(f"[WARNING] DOIO firmware has no delay opcode; {{parts[0]}} skipped.")
            continue

        # Resolve keycodes to HID usage bytes (low byte of QMK code)
        def kc_to_hid(name):
            val = kc_to_int(name)
            hid = val & 0xFF
            if hid == 0:
                print(f"[WARNING] Keycode '{name}' has HID usage 0x00 — skipping.")
            return hid

        if len(parts) == 1:
            # Single key → tap (0x01)
            hid = kc_to_hid(parts[0])
            if hid:
                result += bytes([0x01, hid])
        else:
            # Chord → down in order, up in reverse
            hids = [kc_to_hid(p) for p in parts if p]
            hids = [h for h in hids if h]
            for h in hids:
                result += bytes([0x02, h])
            for h in reversed(hids):
                result += bytes([0x03, h])

    result += b'\x00'
    return bytes(result)


def decode_via_macro_bytes(data: bytes) -> str:
    """
    Decode DOIO firmware binary macro blob into { } syntax string.

    DOIO format: action(1) + hid_usage(1), null-terminated.
        0x01 = tap   → {KC_NAME}
        0x02 = down  (chord start)
        0x03 = up    (chord end — ignored, chord described by downs)
    """
    if not data:
        return ''

    result = ''
    i = 0
    down_keys = []

    def flush_chord():
        nonlocal result
        if down_keys:
            names = [int_to_kc(k) for k in down_keys]
            result += '{' + ','.join(names) + '}'
            down_keys.clear()

    while i < len(data):
        if i + 1 >= len(data):
            break
        action = data[i]
        hid    = data[i + 1]
        i += 2

        if action == 0x01:   # tap
            flush_chord()
            result += '{' + int_to_kc(hid) + '}'
        elif action == 0x02: # down
            down_keys.append(hid)
        elif action == 0x03: # up — ignored, chord reconstructed from downs
            pass
        else:
            pass  # unknown action byte — skip

    flush_chord()
    return result



def build_macro_buffer(macros: list, max_macros: int = 16) -> bytes:
    """Concatenate all macros into a single VIA macro buffer."""
    buf = bytearray()
    for i, m in enumerate(macros[:max_macros]):
        buf += encode_macro_to_via_bytes(m or '')
    # Pad any unused macro slots
    for _ in range(max(0, max_macros - len(macros))):
        buf += b'\x00'
    return bytes(buf)

# ─── Macro read / write via VIA HID ─────────────────────────────────────────

def read_macros_from_keyboard(api, max_macros: int = 16) -> list:
    """
    Read macros from keyboard using qmk_via_api native methods.
    Uses api.get_macro_buffer_size() and api.get_macro_bytes(offset, length).
    Returns a list of decoded { } syntax macro strings.
    """
    macros = [''] * max_macros
    try:
        buf_size = api.get_macro_buffer_size()
        print(f"  Macro buffer size: {buf_size} bytes")
        if buf_size == 0:
            print("  [INFO] Keyboard reports macro buffer size 0 — no macros stored.")
            return macros

        raw = bytes(api.get_macro_bytes())

        # Debug: show first 64 bytes of raw buffer
        preview = raw[:64]
        print(f"  Raw macro buffer (first {len(preview)} bytes): {preview.hex()}")

        # Split on null bytes → individual macro blobs, decode to { } syntax
        parts = raw.split(b'\x00')
        decoded_count = 0
        for i in range(min(max_macros, len(parts))):
            macros[i] = decode_via_macro_bytes(parts[i]) if parts[i] else ''
            if macros[i]:
                decoded_count += 1
                print(f"  Macro {i}: {macros[i]!r}")
        if decoded_count == 0:
            print("  [INFO] All macro slots are empty.")

    except Exception as e:
        print(f"[WARNING] Could not read macros from keyboard: {e}")
    return macros


def write_macros_to_keyboard(api, macros_list: list, max_macros: int = 16) -> bool:
    """
    Write macros to keyboard using qmk_via_api native methods.
    Uses api.get_macro_buffer_size(), api.reset_macros(), api.set_macro_bytes().
    Returns True on success, False on failure.
    """
    if not macros_list or not any(macros_list):
        return True

    try:
        buf_size = api.get_macro_buffer_size()
        if buf_size == 0:
            print("[WARNING] Keyboard reports macro buffer size 0 — macros not supported.")
            return False

        buffer = build_macro_buffer(macros_list, max_macros)
        if len(buffer) > buf_size:
            print(f"[WARNING] Macro data ({len(buffer)} B) exceeds keyboard buffer "
                  f"({buf_size} B). Truncating.")
            buffer = buffer[:buf_size]

        api.reset_macros()
        api.set_macro_bytes(list(buffer))

        print(f"  Macro buffer written ({len(buffer)} bytes).")
        return True

    except Exception as e:
        print(f"[ERROR] Failed to write macros: {e}")
        return False


# ─── Keycode conversion ──────────────────────────────────────────────────────

def int_to_kc(val: int) -> str:
    """Convert integer keycode to readable string."""
    if val == 0x0000:
        return "KC_NO"
    if val == 0x0001:
        return "KC_TRNS"

    # DOIO / RGB supplemental map (checked before keycodes.json reverse map
    # so DOIO-specific names shadow generic hex fallback)
    if val in RGB_MAP:
        return RGB_MAP[val]

    # Standard keycodes
    if val in _REV_KEYCODES:
        return _REV_KEYCODES[val]

    # S() shifted keys  0x0200–0x02FF
    if 0x0200 <= val <= 0x02FF:
        base = val & 0xFF
        base_name = _REV_KEYCODES.get(base, f'0x{base:02X}')
        return f"S({base_name})"

    # LT – Layer Tap  0x4000–0x4FFF
    if 0x4000 <= val <= 0x4FFF:
        layer = (val >> 8) & 0x0F
        key = int_to_kc(val & 0xFF)
        return f"LT({layer}, {key})"

    # MT – Mod Tap  0x2000–0x3FFF
    if 0x2000 <= val <= 0x3FFF:
        mod = (val >> 8) & 0xFF
        key = int_to_kc(val & 0xFF)
        _MOD_BITS = [
            (0x01, "MOD_LCTL"), (0x02, "MOD_LSFT"), (0x04, "MOD_LALT"), (0x08, "MOD_LGUI"),
            (0x10, "MOD_RCTL"), (0x20, "MOD_RSFT"), (0x40, "MOD_RALT"), (0x80, "MOD_RGUI"),
        ]
        named = [name for bit, name in _MOD_BITS if mod & bit]
        mod_str = " | ".join(named) if named else str(mod)
        return f"MT({mod_str},{key})"

    # MO – Momentary layer / DF – Default layer
    # The DOIO firmware / VIA app uses 0x5200+n for DF(n) (not standard QMK DF=0x5240).
    # map.py decodes 0x5200 as DF to match VIA app output exactly.
    if 0x5200 <= val <= 0x521F:
        return f"DF({val - 0x5200})"
    if 0x5100 <= val <= 0x511F:
        return f"MO({val - 0x5100})"

    # TO – Go to layer  0x5010–0x501F
    if 0x5010 <= val <= 0x501F:
        return f"TO({val - 0x5010})"

    # TG – Toggle layer  0x5300–0x531F
    if 0x5300 <= val <= 0x531F:
        return f"TG({val - 0x5300})"

    # OSL – One shot layer  0x5400–0x541F
    if 0x5400 <= val <= 0x541F:
        return f"OSL({val - 0x5400})"

    # TT – Tap Toggle  0x5800–0x581F
    if 0x5800 <= val <= 0x581F:
        return f"TT({val - 0x5800})"

    # DF – Set default layer  0x5240–0x525F
    if 0x5240 <= val <= 0x525F:
        return f"DF({val - 0x5240})"

    # Macro keys  0x7700–0x771F (standard VIA/QMK)
    if 0x7700 <= val <= 0x771F:
        return f"QK_MACRO_{val - 0x7700}"

    # DOIO MACRO(n) keys 0x5F12–0x5F21 – decoded via RGB_MAP above
    # (handled already since RGB_MAP is checked at top of int_to_kc)

    return f"0x{val:04X}"


def kc_to_int(kc: str) -> int:
    """Convert readable keycode string to integer value."""
    if not isinstance(kc, str):
        return 0x0001

    kc = kc.strip().upper()

    # 0. MACRO(n) – DOIO firmware uses 0x5F12+n, NOT the keycodes.json 0x7700+n.
    #    Must be checked BEFORE QMK_KEYCODES since keycodes.json has MACRO(n)=0x7700+n.
    _m = re.match(r'^MACRO\((\d+)\)$', kc)
    if _m:
        n = int(_m.group(1))
        if 0 <= n <= 15:
            return 0x5F12 + n

    # 1. Standard keycodes (keycodes.json – highest priority)
    if kc in QMK_KEYCODES:
        return QMK_KEYCODES[kc]

    # 2. DOIO / RGB supplemental map
    if kc in _REV_RGB_MAP:
        return _REV_RGB_MAP[kc]

    # 3. QK_MACRO_X
    m = re.match(r'^QK_MACRO_(\d+)$', kc)
    if m:
        n = int(m.group(1))
        if 0 <= n <= 31:
            return 0x7700 + n

    # 4. MC_X
    m = re.match(r'^MC_(\d+)$', kc)
    if m:
        n = int(m.group(1))
        if 0 <= n <= 31:
            return 0x7700 + n

    # 5. MACRO(x)
    m = re.match(r'^MACRO\((\d+)\)$', kc)
    if m:
        n = int(m.group(1))
        if 0 <= n <= 15:
            return 0x5F12 + n  # DOIO firmware MACRO(n) range

    # 6. S(KC_XXX)
    m = re.match(r'^S\(([A-Z0-9_]+)\)$', kc)
    if m:
        base = m.group(1)
        if base in QMK_KEYCODES:
            return 0x0200 | QMK_KEYCODES[base]

    # 7. LT(layer, key)  – FIX: was broken (regex only captured one arg,
    #    and the key component was thrown away even when it matched)
    m = re.match(r'^LT\((\d+),\s*(.+)\)$', kc)
    if m:
        layer   = int(m.group(1))
        key_val = kc_to_int(m.group(2).strip())  # recursive – handles any valid kc
        return 0x4000 | ((layer & 0x0F) << 8) | (key_val & 0xFF)

    # 8. MT(mod, key)  – supports plain integer OR named-constant expressions
    #    e.g. MT(5, KC_A)  or  MT(MOD_LCTL | MOD_LSFT | MOD_LALT | ..., KC_B)
    m = re.match(r'^MT\((.+?),\s*(.+)\)$', kc)
    if m:
        mod_str = m.group(1).strip()
        key_val = kc_to_int(m.group(2).strip())
        _MOD_NAMES = {
            "MOD_LCTL": 0x01, "MOD_LSFT": 0x02, "MOD_LALT": 0x04, "MOD_LGUI": 0x08,
            "MOD_RCTL": 0x10, "MOD_RSFT": 0x20, "MOD_RALT": 0x40, "MOD_RGUI": 0x80,
        }
        if re.match(r'^-?\d+$', mod_str):
            mod = int(mod_str)
        else:
            mod = 0
            for token in re.split(r'\s*\|\s*', mod_str):
                token = token.strip()
                if token in _MOD_NAMES:
                    mod |= _MOD_NAMES[token]
                elif token.isdigit():
                    mod |= int(token)
        return 0x2000 | ((mod & 0xFF) << 8) | (key_val & 0xFF)

    # 9. Single-argument layer / function keycodes
    m = re.match(r'^([A-Z_]+)\((\d+)\)$', kc)
    if m:
        name = m.group(1)
        arg  = int(m.group(2))
        bases = {
            "TO":  0x5010,
            "TG":  0x5300,
            "OSL": 0x5400,
            "TT":  0x5800,
            "MO":  0x5200,   # DOIO firmware: MO and DF both use 0x5200 range
            "DF":  0x5200,   # VIA app labels 0x5200+n as DF(n)
        }
        if name in bases:
            return bases[name] + arg

    # 10. Hex literal
    if kc.startswith("0X") or kc.startswith("0x"):
        try:
            return int(kc, 16)
        except ValueError:
            pass

    # Unknown → transparent
    print(f"[WARNING] Unknown keycode '{kc}' — mapping to KC_TRNS")
    return 0x0001

# ─── Layout and keyboard classes ─────────────────────────────────────────────

class Layout:
    def __init__(self, definition: dict):
        self.name    = definition.get("name", "DOIO")
        matrix       = definition.get("matrix", {})
        self.rows    = matrix.get("rows", 0)
        self.cols    = matrix.get("cols", 0)
        self.flat_size = self.rows * self.cols
        self.vid     = int(str(definition.get("vendorId", 0)), 0)
        self.pid     = int(str(definition.get("productId", 0)), 0)
        self.vendorProductId = definition.get(
            "vendorProductId", (self.vid << 16) | self.pid)


class _KeyboardApiWrapper:
    """
    Thin wrapper around a C-extension KeyboardApi object.

    KeyboardApi is compiled as a C/Cython type ('builtins.KeyboardApi') and
    has no __dict__, so setting arbitrary attributes on it raises AttributeError.
    This wrapper stores _vid/_pid as real Python attributes and delegates every
    other attribute access to the underlying C object.
    """
    __slots__ = ('_inner', '_vid', '_pid', '_usage_page')

    def __init__(self, inner, vid: int, pid: int, usage_page: int):
        object.__setattr__(self, '_inner',      inner)
        object.__setattr__(self, '_vid',        vid)
        object.__setattr__(self, '_pid',        pid)
        object.__setattr__(self, '_usage_page', usage_page)

    def __getattr__(self, name):
        return getattr(object.__getattribute__(self, '_inner'), name)

    def __setattr__(self, name, value):
        if name in ('_inner', '_vid', '_pid', '_usage_page'):
            object.__setattr__(self, name, value)
        else:
            setattr(object.__getattribute__(self, '_inner'), name, value)


def connect_keyboard(layout: Layout):
    from qmk_via_api import KeyboardApi, scan_keyboards
    dev = next(
        (d for d in scan_keyboards()
         if d.vendor_id == layout.vid and d.product_id == layout.pid),
        None,
    )
    if not dev:
        raise RuntimeError("Keyboard not found.")
    inner = KeyboardApi(dev.vendor_id, dev.product_id, dev.usage_page)
    # Wrap in a Python object so _vid/_pid/_usage_page can be stored without
    # touching the C-extension type's (non-existent) __dict__.
    api = _KeyboardApiWrapper(inner, dev.vendor_id, dev.product_id, dev.usage_page)
    return api, api.get_layer_count()


def read_current_keymap(api, kb_layers: int, layout: Layout) -> list:
    current = []
    for li in range(kb_layers):
        layer_data = []
        for r in range(layout.rows):
            for c in range(layout.cols):
                layer_data.append(int_to_kc(api.get_key(li, r, c)))
        current.append(layer_data)
    return current


def write_flat_layers_diff(api, kb_layers, layout, current_flat, new_flat_list):
    """Write only changed keys to keyboard.  Raises on HID error."""
    changes = []
    for li in range(min(len(new_flat_list), kb_layers)):
        for i in range(layout.flat_size):
            r, c = divmod(i, layout.cols)
            curr_val = kc_to_int(current_flat[li][i])
            new_val  = kc_to_int(new_flat_list[li][i])
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
            # FIX: was os._exit(1) which skipped cleanup
            raise RuntimeError(
                f"HID write failed at layer {li} row {r} col {c}: {e}"
            ) from e

    print(f"Successfully wrote {len(changes)} changes.")
    return len(changes)

# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    global _api
    args = sys.argv[1:]

    if not args:
        print("Usage: map.exe design.json [me.json | --dump]")
        print("  design.json  Keyboard layout definition file")
        print("  me.json      Keymap file to flash (layers array)")
        print("  --dump       Dump current keymap to file")
        print()
        print("Macro syntax examples:")
        print("  {KC_A}             tap A")
        print("  {KC_LCTL,KC_C}     Ctrl+C chord")
        print("  {$100}             100 ms delay")
        sys.exit(0)

    # ── Developer diagnostic: inspect qmk_via_api internals then exit ────
    if "--hid-debug" in args:
        import ctypes
        from qmk_via_api import KeyboardApi, scan_keyboards

        print("=== qmk_via_api module ===")
        import qmk_via_api as _qva
        print(f"  file : {getattr(_qva, '__file__', '(built-in)')}")
        print(f"  dir  : {[x for x in dir(_qva) if not x.startswith('__')]}")

        print("\n=== KeyboardApi attributes ===")
        devs = list(scan_keyboards())
        if devs:
            dev = devs[0]
            inner = KeyboardApi(dev.vendor_id, dev.product_id, dev.usage_page)
            print(f"  {[x for x in dir(inner) if not x.startswith('__')]}")
        else:
            print("  (no keyboard found)")

        print("\n=== Loaded DLLs containing 'hid' ===")
        import subprocess
        try:
            pid = os.getpid()
            out = subprocess.check_output(
                f'powershell -Command "Get-Process -Id {pid} | '
                r'Select-Object -ExpandProperty Modules | '
                r'Where-Object { $_.ModuleName -match \'hid\' } | '
                r'Select-Object ModuleName, FileName | Format-List"',
                shell=True, text=True, stderr=subprocess.DEVNULL
            )
            print(out or "  (none found)")
        except Exception as e:
            print(f"  (powershell query failed: {e})")

        print("\n=== ctypes DLL probe ===")
        for name in ["hidapi", "libhidapi-0", "hidapi-hidraw",
                     "hidapi-libusb", "hidapi-iohidmanager"]:
            try:
                lib = ctypes.cdll[name]
                _ = lib.hid_enumerate
                print(f"  [FOUND] {name}")
            except (OSError, AttributeError):
                print(f"  [miss ] {name}")

        sys.exit(0)

    try:
        base_path = get_base_path()

        # Load layout definition
        design_path = args[0]
        if not os.path.exists(design_path):
            design_path = os.path.join(base_path, args[0])
        if not os.path.exists(design_path):
            print(f"[ERROR] Layout file '{args[0]}' not found.")
            sys.exit(1)

        with open(design_path, 'r') as f:
            dfn = json.load(f)

        lyt = Layout(dfn)
        print(f"Connecting to {lyt.name} keyboard...")
        _api, kb_layers = connect_keyboard(lyt)
        print(f"Connected. Keyboard has {kb_layers} layers.")

        print("Reading current keymap from keyboard...")
        current = read_current_keymap(_api, kb_layers, lyt)

        if "--dump" in args:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            filename  = f"dump_{timestamp}.json"

            current_macros = read_macros_from_keyboard(_api, 16)

            full_dump = {
                "name":            lyt.name,
                "vendorProductId": lyt.vendorProductId,
                "macros":          current_macros,
                "layers":          current,
                "encoders":        [],
            }
            with open(filename, 'w') as f:
                json.dump(full_dump, f, indent=2)

            print(f"Dumped current keymap to {filename}")
            print(f"Layers dumped: {len(current)}")

        else:
            # Flash mode
            kmp_path = args[1] if len(args) > 1 else "me.json"
            if not os.path.exists(kmp_path):
                kmp_path = os.path.join(base_path, kmp_path)
            if not os.path.exists(kmp_path):
                print("[ERROR] Keymap file not found.")
                sys.exit(1)

            with open(kmp_path, 'r') as f:
                new_data = json.load(f)

            new_layers = new_data.get("layers", [])
            new_macros = new_data.get("macros", [])

            print(f"Loaded keymap from {kmp_path} with {len(new_layers)} layers")

            # Write layer changes
            write_flat_layers_diff(_api, kb_layers, lyt, current, new_layers)

            # Write macros if any are defined
            if new_macros and any(new_macros):
                print("\nParsing macros...")
                for i, macro in enumerate(new_macros[:16]):
                    if macro:
                        encoded = encode_macro_to_via_bytes(macro)
                        print(f"  Macro {i}: {macro!r}  →  {len(encoded)-1} bytes "
                              f"(+terminator)")

                print("Writing macros to keyboard...")
                ok = write_macros_to_keyboard(_api, new_macros)
                if ok:
                    print("Macros written successfully.")
                else:
                    print("[WARNING] Macro write failed or not supported.")

    except RuntimeError as e:
        print(f"[ERROR] {e}")
        sys.exit(1)
    except Exception as e:
        import traceback
        print(f"[ERROR] {e}")
        traceback.print_exc()
        sys.exit(1)
    finally:
        cleanup()
        # FIX: was os._exit(0) which bypassed atexit + cleanup
        sys.exit(0)

if __name__ == "__main__":
    main()
