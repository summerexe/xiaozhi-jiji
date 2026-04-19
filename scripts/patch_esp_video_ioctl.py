#!/usr/bin/env python3
"""Apply lwIP-safe linux/ioctl.h into managed espressif__esp_video (root-cause fix).

esp_video's V4L2 headers define _IO/_IOR/_IOW unconditionally; lwIP's sockets.h
defines the same names for POSIX socket ioctl. Guarding those four macros
matches what upstream should do and removes redefinition without -Wno-*.
"""
from __future__ import annotations

import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PATCH = ROOT / "cmake" / "patches" / "esp_video_linux_ioctl.h"
DST = (
    ROOT
    / "managed_components"
    / "espressif__esp_video"
    / "include"
    / "linux"
    / "ioctl.h"
)


def main() -> int:
    if not PATCH.is_file():
        print(f"patch_esp_video_ioctl: missing {PATCH}", file=sys.stderr)
        return 0
    if not DST.parent.is_dir():
        # Dependencies not fetched yet; CMake may re-run after idf.py fetches components.
        return 0
    shutil.copyfile(PATCH, DST)
    print("patch_esp_video_ioctl: applied cmake/patches/esp_video_linux_ioctl.h -> managed_components/.../ioctl.h")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
