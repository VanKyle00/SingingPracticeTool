"""Generate the app icon (microphone glyph on a dark rounded-square background).

Run from the project root after `pip install pillow`:

    sidecar\\.venv\\Scripts\\python.exe resources\\make_icon.py

Produces:
    resources/Icon.png   (512x512, used by JUCE's juce_add_gui_app ICON_BIG)

JUCE converts this to .ico (Windows) and .icns (macOS) at build time.
"""
from pathlib import Path
from PIL import Image, ImageDraw


# Palette matches Source/UI/ModernLookAndFeel.h
COL_BG_DARK   = (31, 35, 43, 255)     # surface
COL_BG_OUTER  = (22, 24, 29, 255)     # background, outer ring
COL_ACCENT    = (89, 199, 204, 255)   # teal accent
COL_ACCENT_HI = (107, 214, 218, 255)  # lighter accent (highlight)
COL_GRILLE    = (22, 24, 29, 255)     # grille slots — punched holes in mic body


def main() -> None:
    out_dir = Path(__file__).parent.resolve()
    size = 512
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Rounded-square background
    pad = 24
    radius = 96
    d.rounded_rectangle((pad, pad, size - pad, size - pad), radius=radius, fill=COL_BG_DARK)

    # Mic capsule (rounded rect, vertical orientation).
    cw = 160
    ch = 280
    cx = size // 2
    capsule_top = 110
    cap_x0, cap_y0 = cx - cw // 2, capsule_top
    cap_x1, cap_y1 = cx + cw // 2, capsule_top + ch
    d.rounded_rectangle((cap_x0, cap_y0, cap_x1, cap_y1), radius=cw // 2, fill=COL_ACCENT)

    # Inner highlight rim (lighter ring on the upper half of the capsule)
    d.rounded_rectangle(
        (cap_x0 + 8, cap_y0 + 8, cap_x1 - 8, cap_y1 - 8),
        radius=cw // 2 - 8,
        outline=COL_ACCENT_HI,
        width=4,
    )

    # Grille slots — 4 horizontal punched holes spaced down the capsule.
    grille_x_pad = 30
    slot_h = 12
    slot_gap = 28
    slots_top = cap_y0 + 60
    for i in range(4):
        y = slots_top + i * (slot_h + slot_gap)
        d.rounded_rectangle(
            (cap_x0 + grille_x_pad, y, cap_x1 - grille_x_pad, y + slot_h),
            radius=slot_h // 2,
            fill=COL_GRILLE,
        )

    # U-bracket / yoke under the capsule.
    yoke_top = cap_y1 - 20
    yoke_bot = cap_y1 + 50
    yoke_thickness = 14
    yoke_xspread = 70
    # Left arm
    d.rounded_rectangle(
        (cx - yoke_xspread - yoke_thickness // 2, yoke_top,
         cx - yoke_xspread + yoke_thickness // 2, yoke_bot),
        radius=yoke_thickness // 2, fill=COL_ACCENT,
    )
    # Right arm
    d.rounded_rectangle(
        (cx + yoke_xspread - yoke_thickness // 2, yoke_top,
         cx + yoke_xspread + yoke_thickness // 2, yoke_bot),
        radius=yoke_thickness // 2, fill=COL_ACCENT,
    )
    # Cross-bar
    d.rounded_rectangle(
        (cx - yoke_xspread - yoke_thickness // 2, yoke_bot - yoke_thickness,
         cx + yoke_xspread + yoke_thickness // 2, yoke_bot),
        radius=yoke_thickness // 2, fill=COL_ACCENT,
    )

    # Stem from yoke to base
    stem_top = yoke_bot
    stem_bot = yoke_bot + 50
    stem_w = 18
    d.rounded_rectangle(
        (cx - stem_w // 2, stem_top, cx + stem_w // 2, stem_bot),
        radius=stem_w // 2, fill=COL_ACCENT,
    )

    # Base pill
    base_w = 180
    base_h = 18
    d.rounded_rectangle(
        (cx - base_w // 2, stem_bot, cx + base_w // 2, stem_bot + base_h),
        radius=base_h // 2, fill=COL_ACCENT,
    )

    icon_path = out_dir / "Icon.png"
    img.save(icon_path, "PNG")
    print(f"Wrote {icon_path}  ({img.size[0]}x{img.size[1]})")

    # Also emit a multi-resolution .ico for the Inno Setup wizard.
    ico_path = out_dir / "Icon.ico"
    img.save(ico_path, format="ICO", sizes=[(16, 16), (32, 32), (48, 48),
                                             (64, 64), (128, 128), (256, 256)])
    print(f"Wrote {ico_path}")


if __name__ == "__main__":
    main()
