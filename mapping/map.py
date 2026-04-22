"""
map.exe  -  Smart VIA keymap flasher with auto-backup & diff
=============================================================
Works with ANY QMK/VIA keyboard — 5 keys, 16 keys, 100 keys.
Key count is read automatically from the VIA definition file.

Usage:
  map.exe kb16VIA.json                        -> backup + diff + flash me.json
  map.exe kb16VIA.json my_keymap.json         -> backup + diff + flash explicit file
  map.exe kb16VIA.json --dump                 -> backup only (no flash)
  map.exe kb16VIA.json --restore backup.json  -> restore a previous backup
  map.exe --scan                              -> list all connected HID devices

Optional flags (append to any flash command):
  --no-backup   skip the auto-backup step
  --yes         skip the confirmation prompt

Flash behaviour:
  1. Reads key count from the VIA definition automatically (any keyboard size)
  2. Auto-backs up current keyboard state before writing anything
  3. Shows a diff of every key that will change, asks for confirmation
  4. Writes ONLY changed keys (faster and safer)
  5. If new keymap has FEWER keys than the layout  -> fills missing with KC_TRNS
  6. If new keymap has MORE keys than the layout   -> ignores the extras (warns you)
"""

import sys
import os
import json
import datetime

# ─────────────────────────────────────────────────────────────────────────────
# Keycode tables
# ─────────────────────────────────────────────────────────────────────────────
QMK_KEYCODES: dict[str, int] = {
    "KC_NO":0x0000,"KC_TRNS":0x0001,"KC_TRANSPARENT":0x0001,
    "KC_A":0x0004,"KC_B":0x0005,"KC_C":0x0006,"KC_D":0x0007,"KC_E":0x0008,
    "KC_F":0x0009,"KC_G":0x000A,"KC_H":0x000B,"KC_I":0x000C,"KC_J":0x000D,
    "KC_K":0x000E,"KC_L":0x000F,"KC_M":0x0010,"KC_N":0x0011,"KC_O":0x0012,
    "KC_P":0x0013,"KC_Q":0x0014,"KC_R":0x0015,"KC_S":0x0016,"KC_T":0x0017,
    "KC_U":0x0018,"KC_V":0x0019,"KC_W":0x001A,"KC_X":0x001B,"KC_Y":0x001C,
    "KC_Z":0x001D,
    "KC_1":0x001E,"KC_2":0x001F,"KC_3":0x0020,"KC_4":0x0021,"KC_5":0x0022,
    "KC_6":0x0023,"KC_7":0x0024,"KC_8":0x0025,"KC_9":0x0026,"KC_0":0x0027,
    "KC_ENT":0x0028,"KC_ENTER":0x0028,
    "KC_ESC":0x0029,"KC_BSPC":0x002A,"KC_TAB":0x002B,"KC_SPC":0x002C,
    "KC_MINUS":0x002D,"KC_EQUAL":0x002E,"KC_LBRC":0x002F,"KC_RBRC":0x0030,
    "KC_BSLS":0x0031,"KC_SCLN":0x0033,"KC_QUOT":0x0034,"KC_GRV":0x0035,
    "KC_COMM":0x0036,"KC_DOT":0x0037,"KC_SLSH":0x0038,"KC_CAPS":0x0039,
    "KC_F1":0x003A,"KC_F2":0x003B,"KC_F3":0x003C,"KC_F4":0x003D,
    "KC_F5":0x003E,"KC_F6":0x003F,"KC_F7":0x0040,"KC_F8":0x0041,
    "KC_F9":0x0042,"KC_F10":0x0043,"KC_F11":0x0044,"KC_F12":0x0045,
    "KC_PSCR":0x0046,"KC_SCRL":0x0047,"KC_PAUS":0x0048,
    "KC_INS":0x0049,"KC_HOME":0x004A,"KC_PGUP":0x004B,
    "KC_DEL":0x004C,"KC_END":0x004D,"KC_PGDN":0x004E,
    "KC_RGHT":0x004F,"KC_LEFT":0x0050,"KC_DOWN":0x0051,"KC_UP":0x0052,
    "KC_NUM":0x0053,"KC_PSLS":0x0054,"KC_PAST":0x0055,"KC_PMNS":0x0056,
    "KC_PPLS":0x0057,"KC_PENT":0x0058,
    "KC_P1":0x0059,"KC_P2":0x005A,"KC_P3":0x005B,"KC_P4":0x005C,
    "KC_P5":0x005D,"KC_P6":0x005E,"KC_P7":0x005F,"KC_P8":0x0060,
    "KC_P9":0x0061,"KC_P0":0x0062,"KC_PDOT":0x0063,
    "KC_LCTL":0x00E0,"KC_LSFT":0x00E1,"KC_LALT":0x00E2,"KC_LGUI":0x00E3,
    "KC_RCTL":0x00E4,"KC_RSFT":0x00E5,"KC_RALT":0x00E6,"KC_RGUI":0x00E7,
    "KC_TILD":0x0035|0x0200,"KC_EXLM":0x001E|0x0200,"KC_AT":0x001F|0x0200,
    "KC_HASH":0x0020|0x0200,"KC_DLR":0x0021|0x0200,"KC_PERC":0x0022|0x0200,
    "KC_CIRC":0x0023|0x0200,"KC_AMPR":0x0024|0x0200,"KC_ASTR":0x0025|0x0200,
    "KC_LPRN":0x0026|0x0200,"KC_RPRN":0x0027|0x0200,
    "KC_UNDS":0x002D|0x0200,"KC_PLUS":0x002E|0x0200,
    "KC_MUTE":0x7F,"KC_VOLU":0x80,"KC_VOLD":0x81,
    "KC_MPLY":0xE8,"KC_MSTP":0xE9,"KC_MPRV":0xEA,"KC_MNXT":0xEB,
    "KC_MFFD":0xEC,"KC_MRWD":0xED,
    "KC_MS_UP":0xF0,"KC_MS_DOWN":0xF1,"KC_MS_LEFT":0xF2,"KC_MS_RIGHT":0xF3,
    "KC_MS_BTN1":0xF4,"KC_MS_BTN2":0xF5,"KC_MS_BTN3":0xF6,
    "KC_GESC":0x5C16,"KC_UNDO":0x7A00,"KC_CUT":0x7A01,"KC_COPY":0x7A02,
    "KC_PASTE":0x7A03,"KC_SELECT":0x7A04,
    "KC_WWW_BACK":0x00F1,
    "RGB_TOG":0x5C00,"RGB_MOD":0x5C01,"RGB_RMOD":0x5C02,
    "RGB_HUI":0x5C03,"RGB_HUD":0x5C04,"RGB_SAI":0x5C05,"RGB_SAD":0x5C06,
    "RGB_VAI":0x5C07,"RGB_VAD":0x5C08,
    "RGB_SPI":0x5C09,"RGB_SPD":0x5C0A,
}

# Reverse lookup: int -> shortest name
_INT_TO_KC: dict[int, str] = {}
for _n, _v in QMK_KEYCODES.items():
    if _v not in _INT_TO_KC or len(_n) < len(_INT_TO_KC[_v]):
        _INT_TO_KC[_v] = _n

DEFAULT_KEY = "KC_TRNS"

# ─────────────────────────────────────────────────────────────────────────────
# Keycode helpers
# ─────────────────────────────────────────────────────────────────────────────

def int_to_kc(val: int) -> str:
    if val in _INT_TO_KC:
        return _INT_TO_KC[val]
    if (val >> 8) == 0x51: return f"TO({val & 0x0F})"
    if (val >> 8) == 0x52: return f"MO({val & 0x0F})"
    if (val >> 8) == 0x53: return f"TG({val & 0x0F})"
    if (val >> 8) == 0x50: return f"DF({val & 0x0F})"
    if (val >> 8) == 0x77: return f"MACRO{val & 0xFF:02d}"
    return f"0x{val:04X}"


def kc_to_int(kc: str) -> int:
    kc = kc.strip()
    if kc in QMK_KEYCODES:                             return QMK_KEYCODES[kc]
    if kc.startswith("TO(")   and kc.endswith(")"):   return 0x5100 | (int(kc[3:-1]) & 0x0F)
    if kc.startswith("MO(")   and kc.endswith(")"):   return 0x5200 | (int(kc[3:-1]) & 0x0F)
    if kc.startswith("TG(")   and kc.endswith(")"):   return 0x5300 | (int(kc[3:-1]) & 0x0F)
    if kc.startswith("DF(")   and kc.endswith(")"):   return 0x5000 | (int(kc[3:-1]) & 0x0F)
    if kc.startswith("MACRO") and kc[5:].isdigit():   return 0x7700 | (int(kc[5:]) & 0xFF)
    if kc.startswith("0x") or kc.startswith("0X"):    return int(kc, 16)
    if kc.isdigit():                                   return int(kc)
    print(f"  [WARN] Unknown keycode '{kc}' -> KC_TRNS")
    return 0x0001

# ─────────────────────────────────────────────────────────────────────────────
# Layout parser  — auto-adapts to ANY keyboard size
# ─────────────────────────────────────────────────────────────────────────────

def parse_layout(definition: dict) -> list[tuple[int, int]]:
    """
    Extract ordered (row, col) positions from ANY VIA definition.
    Skips spacing objects like {"x": 0.5}.
    Returns the exact list length = physical key count for this keyboard.
    Works for 5-key macropads, full 100-key boards, and everything between.
    """
    positions = []
    for row in definition["layouts"]["keymap"]:
        for item in row:
            if isinstance(item, str) and "," in item:
                r, c = item.split(",", 1)
                try:
                    positions.append((int(r.strip()), int(c.strip())))
                except ValueError:
                    print(f"  [WARN] Bad layout entry '{item}' — skipped")
    return positions


def normalize_layer(layer: list[str], n: int) -> list[str]:
    """Pad with DEFAULT_KEY or trim so layer has exactly n entries."""
    result = list(layer)
    if len(result) < n:
        result += [DEFAULT_KEY] * (n - len(result))
    return result[:n]

# ─────────────────────────────────────────────────────────────────────────────
# HID helpers
# ─────────────────────────────────────────────────────────────────────────────

def load_json(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def scan_all_devices():
    print("=" * 64)
    print("  HID DEVICE SCAN")
    print("=" * 64)
    try:
        from qmk_via_api import scan_keyboards
        devices = scan_keyboards()
        print(f"\n[qmk_via_api]  {len(devices)} VIA device(s):")
        for d in devices:
            print(f"  VID={d.vendor_id:#06x}  PID={d.product_id:#06x}"
                  f"  usage_page={d.usage_page:#06x}")
        if not devices:
            print("  (none)")
    except Exception as e:
        print(f"  [qmk_via_api] {e}")
    try:
        import hid
        devs = hid.enumerate()
        print(f"\n[hidapi raw]  {len(devs)} HID device(s):")
        for d in devs:
            print(f"  VID={d['vendor_id']:#06x}  PID={d['product_id']:#06x}"
                  f"  page={d['usage_page']:#06x}  usage={d['usage']:#06x}"
                  f"  '{d['manufacturer_string']}'  '{d['product_string']}'")
        if not devs:
            print("  (none) — try Run as Administrator")
    except Exception as e:
        print(f"  [hidapi] {e}")
    print("=" * 64 + "\n")


def connect_keyboard(definition: dict):
    """
    Locate and open the keyboard described by definition.
    Returns (api, kb_layers, vid, pid, positions).
    positions is the physical key list auto-detected from the definition.
    """
    from qmk_via_api import KeyboardApi, scan_keyboards

    vid_str = definition.get("vendorId", "0x0000")
    pid_str = definition.get("productId", "0x0000")
    vid = int(vid_str, 16) if isinstance(vid_str, str) else int(vid_str)
    pid = int(pid_str, 16) if isinstance(pid_str, str) else int(pid_str)

    positions = parse_layout(definition)
    n_keys    = len(positions)

    print(f"[*] Target   VID={vid:#06x}  PID={pid:#06x}")
    print(f"[*] Layout   {n_keys} physical keys auto-detected from definition")
    print()

    scan_all_devices()

    devices = scan_keyboards()
    dev     = next((d for d in devices
                    if d.vendor_id == vid and d.product_id == pid), None)
    if dev is None:
        print(f"[ERROR] Keyboard not found  (VID={vid:#06x} PID={pid:#06x})")
        print("  1. Check USB cable")
        print("  2. Run as Administrator")
        print("  3. Confirm VID/PID in definition matches --scan output")
        print("  4. Firmware must have VIA enabled")
        input("\nPress Enter to exit...")
        sys.exit(1)

    print(f"[+] Found    VID={dev.vendor_id:#06x}  PID={dev.product_id:#06x}"
          f"  usage_page={dev.usage_page:#06x}")
    api       = KeyboardApi(dev.vendor_id, dev.product_id, dev.usage_page)
    protocol  = api.get_protocol_version()
    kb_layers = api.get_layer_count()
    print(f"[*] Protocol v{protocol},  {kb_layers} layers on device\n")
    return api, kb_layers, vid, pid, positions

# ─────────────────────────────────────────────────────────────────────────────
# DUMP  (read keyboard -> backup JSON)
# ─────────────────────────────────────────────────────────────────────────────

def _write_backup(layers_out, vid, pid, n_keys, definition, definition_path,
                  out_path=None) -> str:
    if out_path is None:
        ts     = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        folder = os.path.dirname(os.path.abspath(definition_path))
        out_path = os.path.join(folder, f"backup_{ts}.json")
    data = {
        "_backup_note": (
            f"Backup {datetime.datetime.now().isoformat()}"
            f"  VID={vid:#06x} PID={pid:#06x}"
            f"  layout={n_keys} keys/layer"
        ),
        "name":            definition.get("name", "unknown"),
        "vendorProductId": (vid << 16) | pid,
        "layers":          layers_out,
        "macros":          [""] * 16,
    }
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    return out_path


def _read_all_layers(api, kb_layers, positions) -> list[list[str]]:
    """Read every key from an already-open keyboard. Returns list of layers."""
    layers_out = []
    for li in range(kb_layers):
        row_data = []
        for row, col in positions:
            try:
                row_data.append(int_to_kc(api.get_key(li, row, col)))
            except Exception:
                row_data.append(DEFAULT_KEY)
        layers_out.append(row_data)
    return layers_out


def dump(definition_path: str, out_path: str = None) -> str:
    """Backup-only mode: read keyboard and write JSON. Returns saved path."""
    definition = load_json(definition_path)
    print(f"[*] Definition : {definition_path}\n")
    api, kb_layers, vid, pid, positions = connect_keyboard(definition)
    n_keys = len(positions)
    print(f"[*] Reading {kb_layers} layer(s) × {n_keys} keys ...")

    layers_out = _read_all_layers(api, kb_layers, positions)

    for li, layer in enumerate(layers_out):
        non_default = sum(1 for k in layer if k not in (DEFAULT_KEY, "KC_NO"))
        print(f"  Layer {li:2d} ... {n_keys} keys read  ({non_default} non-transparent)")

    saved = _write_backup(layers_out, vid, pid, n_keys,
                          definition, definition_path, out_path)
    print(f"\n[+] Backup saved:  {saved}")
    print(f"\n  To restore:  map.exe {os.path.basename(definition_path)}"
          f" --restore {os.path.basename(saved)}")
    return saved

# ─────────────────────────────────────────────────────────────────────────────
# DIFF  (compare two keymaps)
# ─────────────────────────────────────────────────────────────────────────────

def diff_keymaps(current_layers: list, new_layers: list,
                 n_keys: int) -> list[tuple]:
    """
    Return list of (layer_idx, key_idx, current_kc, new_kc) for every change.
    Both sides are normalised to n_keys so size mismatches don't cause errors.
    """
    changes = []
    num_layers = max(len(current_layers), len(new_layers))
    for li in range(num_layers):
        cur = normalize_layer(current_layers[li] if li < len(current_layers) else [], n_keys)
        new = normalize_layer(new_layers[li]     if li < len(new_layers)     else [], n_keys)
        for ki in range(n_keys):
            if cur[ki] != new[ki]:
                changes.append((li, ki, cur[ki], new[ki]))
    return changes


def print_diff(changes: list, positions: list):
    if not changes:
        print("  (no changes — keyboard already matches the new keymap)")
        return
    from itertools import groupby
    print(f"  {'Layer':>5}  {'Key':>4}  {'Matrix':>8}  "
          f"{'Current (keyboard)':>20}  {'New (keymap)':>20}")
    print("  " + "-" * 70)
    for li, group in groupby(changes, key=lambda x: x[0]):
        for _, ki, old, new in group:
            pos    = positions[ki] if ki < len(positions) else ("?", "?")
            matrix = f"r{pos[0]}c{pos[1]}"
            print(f"  {li:>5}  {ki:>4}  {matrix:>8}  "
                  f"{old:>20}  ->  {new:<20}")
    print()
    print(f"  Total: {len(changes)} key(s) will change")

# ─────────────────────────────────────────────────────────────────────────────
# FLASH / RESTORE
# ─────────────────────────────────────────────────────────────────────────────

def flash(definition_path: str, keymap_path: str,
          label: str = "Flash",
          skip_backup: bool = False,
          skip_confirm: bool = False):
    """
    Full smart pipeline:
      1. Parse layout (auto-detects key count from definition)
      2. Warn about size mismatches in the keymap file
      3. Auto-backup current keyboard state
      4. Diff current vs new keymap
      5. Confirm with user
      6. Write ONLY changed keys
    """
    definition  = load_json(definition_path)
    keymap_data = load_json(keymap_path)
    positions   = parse_layout(definition)
    n_keys      = len(positions)
    new_layers  = keymap_data.get("layers", [])

    print(f"[*] Definition : {definition_path}")
    print(f"    -> {n_keys} physical keys auto-detected")
    print(f"[*] Keymap     : {keymap_path}")
    print(f"    -> {len(new_layers)} layer(s), "
          f"{len(new_layers[0]) if new_layers else 0} entries/layer")
    print()

    # ── Size mismatch warnings (informational, not fatal) ────────────────────
    if new_layers:
        layer_len = len(new_layers[0])
        if layer_len > n_keys:
            extra = layer_len - n_keys
            print(f"  [INFO] Keymap has {layer_len} entries/layer, "
                  f"layout has {n_keys} physical keys.")
            print(f"         Last {extra} entries in each layer will be IGNORED "
                  f"(no physical key there).")
            print()
        elif layer_len < n_keys:
            short = n_keys - layer_len
            print(f"  [INFO] Keymap has {layer_len} entries/layer, "
                  f"layout has {n_keys} physical keys.")
            print(f"         Last {short} key(s) not in keymap will default "
                  f"to {DEFAULT_KEY}.")
            print()

    # ── Connect ──────────────────────────────────────────────────────────────
    api, kb_layers, vid, pid, _ = connect_keyboard(definition)

    # ── Auto-backup ──────────────────────────────────────────────────────────
    backup_path    = None
    current_layers = None

    if not skip_backup:
        print("[*] Auto-backup: reading current keyboard state ...")
        current_layers = _read_all_layers(api, kb_layers, positions)
        backup_path    = _write_backup(current_layers, vid, pid, n_keys,
                                       definition, definition_path)
        print(f"[+] Backup saved:  {backup_path}\n")
    else:
        print("[*] Backup skipped (--no-backup)\n")

    # If we skipped backup treat everything as changed
    if current_layers is None:
        current_layers = [[DEFAULT_KEY] * n_keys] * kb_layers

    # ── Diff ─────────────────────────────────────────────────────────────────
    print("[*] Changes that will be written:")
    changes = diff_keymaps(current_layers, new_layers, n_keys)
    print_diff(changes, positions)

    if not changes:
        print("[+] Nothing to do — keyboard is already up to date.")
        input("\nPress Enter to exit...")
        return

    # ── Confirm ──────────────────────────────────────────────────────────────
    if not skip_confirm:
        ans = input(f"\n  Write {len(changes)} change(s) to keyboard? [y/N] ").strip().lower()
        if ans not in ("y", "yes"):
            print("  Aborted. No changes written.")
            if backup_path:
                print(f"  Backup kept at:  {backup_path}")
            input("\nPress Enter to exit...")
            return
    print()

    # ── Write only changed keys ───────────────────────────────────────────────
    num_layers = min(len(new_layers), kb_layers)
    print(f"[*] {label}: writing {len(changes)} key(s) ...")
    print()

    written = errors = 0
    for li in range(num_layers):
        new_layer = normalize_layer(new_layers[li], n_keys)
        cur_layer = normalize_layer(
                        current_layers[li] if li < len(current_layers) else [],
                        n_keys)
        layer_writes = 0
        for ki, (row, col) in enumerate(positions):
            if new_layer[ki] == cur_layer[ki]:
                continue                       # unchanged — skip
            kc_val = kc_to_int(new_layer[ki])
            try:
                api.set_key(li, row, col, kc_val)
                written      += 1
                layer_writes += 1
            except Exception as e:
                print(f"  [WARN] L{li} key[{ki}] r{row}c{col} "
                      f"'{new_layer[ki]}' -> {e}")
                errors += 1
        if layer_writes:
            print(f"  Layer {li:2d} ... {layer_writes} key(s) written")

    print()
    if errors:
        print(f"[!] Done with {errors} error(s). {written} key(s) written.")
    else:
        print(f"[+] Done!  {written} key(s) written successfully.")
    if backup_path:
        print(f"\n  Restore anytime:  "
              f"map.exe {os.path.basename(definition_path)}"
              f" --restore {os.path.basename(backup_path)}")
    input("\nPress Enter to exit...")

# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    args = sys.argv[1:]
    exe  = os.path.basename(sys.argv[0]).lower()
    args = [a for a in args if os.path.basename(a).lower() != exe]

    if not args or args[0] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    if args[0] == "--scan":
        scan_all_devices()
        input("Press Enter to exit...")
        sys.exit(0)

    definition_path = args[0]
    if not definition_path.lower().endswith(".json"):
        print(f"[ERROR] First argument must be the VIA definition .json\n")
        print(__doc__)
        input("\nPress Enter to exit...")
        sys.exit(1)
    if not os.path.isfile(definition_path):
        print(f"[ERROR] File not found: {definition_path}")
        input("\nPress Enter to exit...")
        sys.exit(1)

    rest = args[1:]

    # --dump [output.json]
    if rest and rest[0] == "--dump":
        out = rest[1] if len(rest) >= 2 else None
        try:
            dump(definition_path, out)
        except Exception as e:
            print(f"\n[ERROR] {e}")
            import traceback; traceback.print_exc()
        input("\nPress Enter to exit...")
        return

    # --restore <backup.json>
    if rest and rest[0] == "--restore":
        if len(rest) < 2:
            print("[ERROR] --restore requires a filename.")
            print(f"  Example:  map.exe {os.path.basename(definition_path)}"
                  f" --restore backup_20260421_194453.json")
            input("\nPress Enter to exit...")
            sys.exit(1)
        rpath = rest[1]
        if not os.path.isfile(rpath):
            print(f"[ERROR] Restore file not found: {rpath}")
            input("\nPress Enter to exit...")
            sys.exit(1)
        try:
            # Restoring from a backup: no point making another backup of the
            # same state; user still gets a confirmation diff.
            flash(definition_path, rpath,
                  label="Restore", skip_backup=True, skip_confirm=False)
        except Exception as e:
            print(f"\n[ERROR] {e}")
            import traceback; traceback.print_exc()
            input("\nPress Enter to exit...")
        return

    # Optional modifier flags
    skip_backup  = "--no-backup" in rest
    skip_confirm = "--yes"       in rest
    rest = [a for a in rest if a not in ("--no-backup", "--yes")]

    # Flash (default)
    if rest:
        keymap_path = rest[0]
    else:
        folder = os.path.dirname(os.path.abspath(definition_path))
        for candidate in ("me.json", "test.json"):
            keymap_path = os.path.join(folder, candidate)
            if os.path.isfile(keymap_path):
                print(f"[*] Auto-detected keymap: {keymap_path}")
                break
        else:
            print(f"[ERROR] No keymap given and me.json/test.json not found.")
            print(f"  Usage:  map.exe {os.path.basename(definition_path)} [keymap.json]")
            input("\nPress Enter to exit...")
            sys.exit(1)

    if not os.path.isfile(keymap_path):
        print(f"[ERROR] Keymap not found: {keymap_path}")
        input("\nPress Enter to exit...")
        sys.exit(1)

    try:
        flash(definition_path, keymap_path,
              skip_backup=skip_backup, skip_confirm=skip_confirm)
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback; traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)


if __name__ == "__main__":
    main()
