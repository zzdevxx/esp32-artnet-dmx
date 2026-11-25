#!/usr/bin/env python3
"""Verify SHA-256 byte parity between esp32-artnet-dmx/firmware & schematics and zzluxora-thesis."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
THESIS_ROOT = ROOT.parent / "zzluxora-thesis"


def sha256_bytes(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_pair(dir_a: Path, dir_b: Path, label: str) -> list[str]:
    errs: list[str] = []
    if not dir_a.is_dir():
        return [f"{label}: missing source dir {dir_a}"]
    if not dir_b.is_dir():
        print(f"[SKIP] {label}: sibling repo dir not present ({dir_b})")
        return []
    files_a = {p.name: p for p in sorted(dir_a.iterdir()) if p.is_file()}
    files_b = {p.name: p for p in sorted(dir_b.iterdir()) if p.is_file()}
    for name in sorted(set(files_a) | set(files_b)):
        if name not in files_a:
            errs.append(f"{label}: {name} missing in {dir_a}")
        elif name not in files_b:
            errs.append(f"{label}: {name} missing in {dir_b}")
        else:
            ha, hb = sha256_bytes(files_a[name]), sha256_bytes(files_b[name])
            if ha != hb:
                errs.append(f"{label}: SHA-256 mismatch on {name} ({ha[:12]} != {hb[:12]})")
            else:
                print(f"[OK] {label}/{name}: {ha[:16]}...")
    return errs


def main() -> int:
    errors: list[str] = []
    ino = ROOT / "firmware" / "artnet_dmx_final.ino"
    v5 = ROOT / "firmware" / "artnet_dmx_v5.txt"
    if not ino.is_file() or not v5.is_file():
        errors.append("Missing artnet_dmx_final.ino or artnet_dmx_v5.txt")
    elif sha256_bytes(ino) != sha256_bytes(v5):
        errors.append("artnet_dmx_final.ino does not match artnet_dmx_v5.txt")
    else:
        print(f"[OK] local final.ino == v5.txt ({sha256_bytes(ino)[:16]}...)")

    errors.extend(check_pair(ROOT / "firmware", THESIS_ROOT / "artnet_projects", "firmware"))
    errors.extend(check_pair(ROOT / "schematics", THESIS_ROOT / "image_references" / "hardware_schematics_hd", "schematics"))

    if errors:
        for e in errors:
            print(f"[FAIL] {e}")
        return 1
    print("[PASS] Firmware and schematic parity verified.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
