#!/usr/bin/env python3
"""Generates THIRD_PARTY_NOTICES.txt from the licence files shipped inside ThirdParty/JUCE.

Run again whenever JUCE is updated:   python3 Tools/make_notices.py
The output is embedded in the plug-in (About dialog) and copied into every bundle's Resources.
"""
import os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
J = os.path.join(ROOT, "ThirdParty", "JUCE")
M = os.path.join(J, "modules")


def read(*parts):
    with open(os.path.join(*parts), encoding="utf-8", errors="replace") as f:
        return f.read().replace("\r\n", "\n").strip("\n")


def section(text, start_pat, end_pat):
    """Text from the line matching start_pat up to (not including) the line matching end_pat."""
    m = re.search(start_pat, text, re.M)
    if not m:
        raise SystemExit(f"pattern not found: {start_pat}")
    rest = text[m.start():]
    e = re.search(end_pat, rest[1:], re.M)
    return (rest[: e.start() + 1] if e else rest).strip("\n")


def juce_version():
    m = re.search(r"project\(JUCE VERSION ([0-9.]+)", read(J, "CMakeLists.txt"))
    return m.group(1) if m else "8"


RULE = "=" * 78

components = [
    ("JUCE Framework " + juce_version(),
     "Copyright (c) Raw Material Software Limited. https://juce.com\n"
     "Used under either the GNU Affero General Public License v3 (AGPLv3) or a commercial\n"
     "JUCE licence, as chosen by the distributor of this software (see README.md, 'Licensing').",
     section(read(J, "LICENSE.md"), r"^# The JUCE Framework", r"^## The JUCE Framework Dependencies")),

    ("Steinberg VST3 SDK (VST3 plug-in only)",
     "Copyright (c) Steinberg Media Technologies GmbH. All rights reserved.\n"
     "VST is a registered trademark of Steinberg Media Technologies GmbH.",
     read(M, "juce_audio_processors", "format_types", "VST3_SDK", "LICENSE.txt")),

    ("Apple AudioUnitSDK (Audio Unit plug-in only) - Apache License 2.0",
     "Copyright (c) Apple Inc. All rights reserved.\n"
     "Audio Units is a trademark of Apple Inc.",
     read(M, "juce_audio_plugin_client", "AU", "AudioUnitSDK", "LICENSE.txt")),

    ("zlib",
     "(C) 1995-2004 Jean-loup Gailly and Mark Adler",
     section(read(M, "juce_core", "zip", "zlib", "README"), r"^Copyright notice:", r"^If you use the zlib library in a product")),

    ("libpng",
     "",
     read(M, "juce_graphics", "image_formats", "pnglib", "LICENSE")),

    ("Independent JPEG Group's JPEG library",
     "This software is based in part on the work of the Independent JPEG Group.",
     section(read(M, "juce_graphics", "image_formats", "jpglib", "README"), r"^LEGAL ISSUES\n=+", r"^REFERENCES\n=+")),

    ("HarfBuzz - Old MIT licence",
     "",
     read(M, "juce_graphics", "fonts", "harfbuzz", "COPYING")),

    ("SheenBidi - Apache License 2.0",
     "Copyright (C) 2014-2018 Muhammad Tayyab Akram\n"
     "Licensed under the Apache License, Version 2.0 (full text in the AudioUnitSDK section above).",
     ""),
]

header = f"""SilkReed Sax - Third-Party Notices and Trademarks
{RULE}

SilkReed Sax is an independent product. It is not affiliated with, endorsed by or
sponsored by any recording artist or by the makers of any other software instrument.
All sounds are synthesised in real time; no recordings or samples of any performer
are included. The application icon and all factory presets are original works.

Trademarks: VST is a registered trademark of Steinberg Media Technologies GmbH.
Audio Units, macOS and Logic Pro are trademarks of Apple Inc. All other product
names are the property of their respective owners and are used for identification
only.

This software includes the following third-party components, used under the
licences reproduced below.
"""

out = [header]
for name, note, text in components:
    out.append(f"\n{RULE}\n{name}\n{RULE}\n")
    if note:
        out.append(note + "\n")
    if text:
        out.append("\n" + text + "\n")

dest = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "THIRD_PARTY_NOTICES.txt")
with open(dest, "w", encoding="utf-8") as f:
    f.write("".join(out))
print("wrote", dest, f"({sum(len(s) for s in out)} chars)")
