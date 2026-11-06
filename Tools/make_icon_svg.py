#!/usr/bin/env python3
"""Generates Resources/Icon.svg — a stylised golden saxophone on a midnight tile.

The horn is built from a centreline (cubic Béziers) and a width profile that
grows conically along the bore and flares exponentially at the bell, so the
outline looks like a real tapering brass tube rather than a stroked line.
"""
import math, os, sys

S = 1024.0

def bez(p0, p1, p2, p3, t):
    u = 1 - t
    return (u*u*u*p0[0] + 3*u*u*t*p1[0] + 3*u*t*t*p2[0] + t*t*t*p3[0],
            u*u*u*p0[1] + 3*u*u*t*p1[1] + 3*u*t*t*p2[1] + t*t*t*p3[1])

# Centreline: neck -> body -> bow -> bell (player on the left, bell facing up-right)
SEGS = [
    ((262, 286), (318, 214), (430, 196), (478, 282)),   # neck crook
    ((478, 282), (506, 352), (478, 560), (474, 676)),   # body
    ((474, 676), (468, 846), (676, 862), (700, 718)),   # bow
    ((700, 718), (712, 650), (716, 598), (760, 500)),   # bell tube
]

def sample_centreline(n_per=160):
    pts = []
    for si, seg in enumerate(SEGS):
        for i in range(n_per):
            if si > 0 and i == 0:
                continue
            pts.append(bez(*seg, i / (n_per - 1)))
    return pts

def cumulative(pts):
    d = [0.0]
    for a, b in zip(pts, pts[1:]):
        d.append(d[-1] + math.hypot(b[0]-a[0], b[1]-a[1]))
    return d

def width_at(u):
    """u = 0..1 along the horn. Conical bore with exponential bell flare."""
    w = 30 + 74 * u            # cone
    if u > 0.84:               # flare
        f = (u - 0.84) / 0.16
        w += 150 * (math.exp(3.2 * f) - 1) / (math.exp(3.2) - 1)
    return w

def normals(pts):
    ns = []
    for i in range(len(pts)):
        a = pts[max(0, i-1)]; b = pts[min(len(pts)-1, i+1)]
        dx, dy = b[0]-a[0], b[1]-a[1]
        L = math.hypot(dx, dy) or 1
        ns.append((-dy/L, dx/L))
    return ns

def band(pts, ns, us, lo, hi):
    """Polygon between offsets lo*w and hi*w (fractions of half-width, -1..1)."""
    left, right = [], []
    for p, n, u in zip(pts, ns, us):
        hw = width_at(u) / 2
        left.append((p[0] + n[0]*hw*hi, p[1] + n[1]*hw*hi))
        right.append((p[0] + n[0]*hw*lo, p[1] + n[1]*hw*lo))
    poly = left + right[::-1]
    return "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in poly) + " Z"

def main(out_path):
    pts = sample_centreline()
    d = cumulative(pts); total = d[-1]
    us = [x / total for x in d]
    ns = normals(pts)

    body      = band(pts, ns, us, -1.0, 1.0)
    highlight = band(pts, ns, us, 0.18, 0.46)
    sheen     = band(pts, ns, us, 0.52, 0.62)
    shadow    = band(pts, ns, us, -1.0, -0.62)

    # Bell rim: ellipse across the mouth, perpendicular to the end tangent
    end = pts[-1]; n_end = ns[-1]
    rim_w = width_at(1.0)
    ang = math.degrees(math.atan2(n_end[1], n_end[0]))

    # Mouthpiece: extends back from the start of the neck
    p0, p1 = pts[0], pts[6]
    tx, ty = p0[0]-p1[0], p0[1]-p1[1]; L = math.hypot(tx, ty); tx, ty = tx/L, ty/L
    nx, ny = -ty, tx
    def mp(off_t, off_n):
        return (p0[0] + tx*off_t + nx*off_n, p0[1] + ty*off_t + ny*off_n)
    mpoly = [mp(-4, 15), mp(70, 12), mp(104, 7), mp(112, 0), mp(104, -9), mp(70, -13), mp(-4, -15)]
    mouthpiece = "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in mpoly) + " Z"
    lig = [mp(14, 17), mp(34, 16), mp(34, -16), mp(14, -17)]
    ligature = "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in lig) + " Z"

    # Pearl keys along the body
    keys = []
    for u, r, side in [(0.30, 15, 0.0), (0.36, 17, 0.0), (0.42, 18, 0.0),
                       (0.50, 19, 0.05), (0.56, 20, 0.05), (0.62, 20, 0.05)]:
        i = min(range(len(us)), key=lambda j: abs(us[j]-u))
        p, n = pts[i], ns[i]
        hw = width_at(u) / 2
        keys.append((p[0] + n[0]*hw*side, p[1] + n[1]*hw*side, r))
    # Key rod running beside the keys
    rod = []
    for u in [0.27 + 0.37*k/30 for k in range(31)]:
        i = min(range(len(us)), key=lambda j: abs(us[j]-u))
        p, n = pts[i], ns[i]; hw = width_at(u)/2
        rod.append((p[0] - n[0]*hw*0.72, p[1] - n[1]*hw*0.72))
    rod_d = "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in rod)

    # Bow guard + bell keys
    guard = []
    for u, r in [(0.76, 16), (0.83, 15)]:
        i = min(range(len(us)), key=lambda j: abs(us[j]-u))
        p, n = pts[i], ns[i]; hw = width_at(u)/2
        guard.append((p[0] - n[0]*hw*0.15, p[1] - n[1]*hw*0.15, r))

    key_svg = "\n".join(
        f'    <circle cx="{x:.1f}" cy="{y:.1f}" r="{r+5}" fill="url(#keyRing)"/>\n'
        f'    <circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="url(#pearl)"/>'
        for x, y, r in keys + guard)

    # Sound waves: concentric arcs centred on the bell mouth, around the bell axis
    ex, ey = end
    ax = math.atan2(pts[-1][1]-pts[-12][1], pts[-1][0]-pts[-12][0])
    arcs = []
    for k, r in enumerate([150, 200, 250]):
        a0, a1 = ax - math.radians(26), ax + math.radians(26)
        x0, y0 = ex + r*math.cos(a0), ey + r*math.sin(a0)
        x1, y1 = ex + r*math.cos(a1), ey + r*math.sin(a1)
        arcs.append(f'  <path d="M{x0:.1f},{y0:.1f} A{r},{r} 0 0 1 {x1:.1f},{y1:.1f}" fill="none" '
                    f'stroke="url(#gold)" stroke-width="{15 - 3*k}" stroke-linecap="round" opacity="{0.75 - 0.2*k:.2f}"/>')
    waves = "\n".join(arcs)

    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1024" viewBox="0 0 1024 1024">
  <defs>
    <radialGradient id="bg" cx="42%" cy="38%" r="75%">
      <stop offset="0" stop-color="#22345C"/>
      <stop offset="0.55" stop-color="#121B33"/>
      <stop offset="1" stop-color="#070A14"/>
    </radialGradient>
    <radialGradient id="glow" cx="50%" cy="50%" r="50%">
      <stop offset="0" stop-color="#E9A93A" stop-opacity="0.22"/>
      <stop offset="1" stop-color="#1A2850" stop-opacity="0"/>
    </radialGradient>
    <linearGradient id="gold" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#FFF3C4"/>
      <stop offset="0.35" stop-color="#F3C55A"/>
      <stop offset="0.7" stop-color="#C08A2C"/>
      <stop offset="1" stop-color="#7A5114"/>
    </linearGradient>
    <linearGradient id="brass" x1="0.2" y1="0" x2="0.8" y2="1">
      <stop offset="0" stop-color="#FCE08A"/>
      <stop offset="0.45" stop-color="#E0A93C"/>
      <stop offset="0.8" stop-color="#A9721F"/>
      <stop offset="1" stop-color="#6B4410"/>
    </linearGradient>
    <linearGradient id="rimFill" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#3A2608"/>
      <stop offset="1" stop-color="#140C02"/>
    </linearGradient>
    <linearGradient id="border" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#F6D27A" stop-opacity="0.9"/>
      <stop offset="0.5" stop-color="#8A6420" stop-opacity="0.35"/>
      <stop offset="1" stop-color="#F6D27A" stop-opacity="0.7"/>
    </linearGradient>
    <radialGradient id="pearl" cx="38%" cy="35%" r="70%">
      <stop offset="0" stop-color="#FFFFFF"/>
      <stop offset="0.5" stop-color="#F1E9D8"/>
      <stop offset="1" stop-color="#B9AC92"/>
    </radialGradient>
    <linearGradient id="keyRing" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#FFE9A8"/>
      <stop offset="1" stop-color="#8C5E18"/>
    </linearGradient>
    <linearGradient id="black" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#4A4F5C"/>
      <stop offset="0.5" stop-color="#15171D"/>
      <stop offset="1" stop-color="#050506"/>
    </linearGradient>
  </defs>
  <rect x="24" y="24" width="976" height="976" rx="220" fill="url(#bg)"/>
  <circle cx="520" cy="560" r="360" fill="url(#glow)"/>
{waves}
  <path d="{body}" fill="#000000" opacity="0.35" transform="translate(10,16)"/>
  <path d="{body}" fill="url(#brass)"/>
  <path d="{shadow}" fill="#5A3A0C" opacity="0.45"/>
  <path d="{highlight}" fill="#FFF6D6" opacity="0.45"/>
  <path d="{sheen}" fill="#FFFFFF" opacity="0.35"/>
  <path d="{rod_d}" fill="none" stroke="#FFE7A0" stroke-width="5" stroke-linecap="round" opacity="0.8"/>
{key_svg}
  <g transform="translate({end[0]:.1f},{end[1]:.1f}) rotate({ang:.1f})">
    <ellipse cx="0" cy="0" rx="{rim_w/2 + 6:.1f}" ry="34" fill="url(#gold)"/>
    <ellipse cx="0" cy="2" rx="{rim_w/2 - 8:.1f}" ry="22" fill="url(#rimFill)"/>
  </g>
  <path d="{mouthpiece}" fill="url(#black)"/>
  <path d="{ligature}" fill="url(#gold)"/>
  <rect x="24" y="24" width="976" height="976" rx="220" fill="none" stroke="url(#border)" stroke-width="8"/>
</svg>
'''
    with open(out_path, "w") as f:
        f.write(svg)
    print("wrote", out_path)

if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "Resources", "Icon.svg"))
