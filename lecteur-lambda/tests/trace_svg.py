"""Trace le cycle simulé (CSV de trace_chauffe) en SVG, sans dépendance.

Trois panneaux sur le même axe du temps : tension efficace de chauffe, résistance
interne Ri mesurée par injection (échelle logarithmique), lambda mesuré et simulé.
Usage :
    python3 trace_svg.py cycle.csv cycle.svg
"""
import csv
import math
import sys

W, LEFT, RIGHT = 860, 64, 24
TOP1, H1 = 64, 150
TOP2, H2 = TOP1 + H1 + 40, 130
TOP3, H3 = TOP2 + H2 + 40, 130
HEIGHT = TOP3 + H3 + 48
T_MAX = 60.0
U_MAX = 13.0
RI_MIN, RI_MAX = 100.0, 30000.0
L_MIN, L_MAX = 0.80, 1.20

PHASES = {
    "ATTENT": "Attente moteur",
    "CONDEN": "Condensation",
    "12V": "12 V",
    "9V": "9 V",
    "RAMPE": "Rampe",
    "REGUL": "Régulation",
}


def x_of(t):
    return LEFT + (W - LEFT - RIGHT) * t / T_MAX


def y_volt(u):
    return TOP1 + H1 * (1 - u / U_MAX)


def y_ri(ri):
    ri = min(max(ri, RI_MIN), RI_MAX)
    f = (math.log10(ri) - math.log10(RI_MIN)) / (math.log10(RI_MAX) - math.log10(RI_MIN))
    return TOP2 + H2 * (1 - f)


def y_lambda(lam):
    lam = min(max(lam, L_MIN), L_MAX)
    return TOP3 + H3 * (1 - (lam - L_MIN) / (L_MAX - L_MIN))


def polyline(points, cls):
    pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
    return f'<polyline class="{cls}" points="{pts}"/>'


def main(src, dst):
    rows = list(csv.DictReader(open(src, encoding="utf-8")))
    t = [float(r["t_s"]) for r in rows]
    u = [float(r["Uh_eff_V"]) for r in rows]
    ri = [float(r["Ri_mesuree_ohm"]) for r in rows]
    lam = [float(r["lambda_mesure"]) for r in rows]
    lam_true = [float(r["lambda_vrai"]) for r in rows]
    ready = [r["pret"] == "1" for r in rows]
    phase = [r["phase"] for r in rows]

    spans = []
    start = 0
    for i in range(1, len(rows) + 1):
        if i == len(rows) or phase[i] != phase[start]:
            spans.append((phase[start], t[start], t[i - 1]))
            start = i
    t_boost = next(s for p, s, _ in spans if p == "12V")
    t_pump = next(a for a, r in zip(t, rows) if r["pompe"] == "1")
    t_ready = next(a for a, ok in zip(t, ready) if ok)

    out = []
    add = out.append
    add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {HEIGHT}" width="{W}" height="{HEIGHT}" '
        'font-family="system-ui, -apple-system, Segoe UI, sans-serif" font-size="12" '
        'role="img" aria-labelledby="titre desc">')
    add('<title id="titre">Cycle simulé de la sonde LSU ADV en pilotage direct</title>')
    add('<desc id="desc">Tension efficace de chauffe, résistance interne Ri mesurée par injection, '
        'lambda mesuré et lambda simulé, en fonction du temps. Données : cycle-chauffe-simule.csv.</desc>')
    add("""<style>
  .surface { fill: #fcfcfb; }
  .band { fill: #f0efec; }
  .grid { stroke: #e4e3df; stroke-width: 1; }
  .axis { stroke: #8a8984; stroke-width: 1; }
  .ink { fill: #0b0b0b; }
  .ink2 { fill: #52514e; }
  .series { fill: none; stroke: #2a78d6; stroke-width: 2; stroke-linejoin: round; }
  .truth { fill: none; stroke: #52514e; stroke-width: 1.5; stroke-dasharray: 5 4; }
  .ref { stroke: #52514e; stroke-width: 1; stroke-dasharray: 4 3; }
  .key { stroke: #2a78d6; stroke-width: 2; }
  @media (prefers-color-scheme: dark) {
    .surface { fill: #1a1a19; }
    .band { fill: #262624; }
    .grid { stroke: #33332f; }
    .axis { stroke: #6f6e69; }
    .ink { fill: #f0efec; }
    .ink2 { fill: #c3c2b7; }
    .series { stroke: #3987e5; }
    .key { stroke: #3987e5; }
    .truth { stroke: #c3c2b7; }
    .ref { stroke: #c3c2b7; }
  }
</style>""")
    add(f'<rect class="surface" x="0" y="0" width="{W}" height="{HEIGHT}"/>')
    add(f'<text class="ink" x="{LEFT}" y="22" font-size="15" font-weight="600">'
        'Pilotage direct simulé : chauffe de la fiche Bosch, Ri, pompe et lambda</text>')
    add(f'<text class="ink2" x="{LEFT}" y="40">Sonde simulée (modèle approché, pas une mesure). '
        f'Pompe activée à {t_pump:.1f} s, mesure exploitable à {t_ready:.1f} s.</text>')

    # Bandes de phase, une sur deux, communes aux trois panneaux.
    for k, (p, s, e) in enumerate(spans):
        x0, x1 = x_of(s), x_of(min(e + 0.1, T_MAX))
        if k % 2 == 1:
            for top, h in ((TOP1, H1), (TOP2, H2), (TOP3, H3)):
                add(f'<rect class="band" x="{x0:.1f}" y="{top}" width="{x1 - x0:.1f}" height="{h}"/>')
        label = PHASES.get(p, p)
        if x1 - x0 > 7 * len(label):
            add(f'<text class="ink2" x="{(x0 + x1) / 2:.1f}" y="{TOP1 - 6}" text-anchor="middle">{label}</text>')
        elif p == "9V":
            add(f'<text class="ink2" x="{x0 + 2:.1f}" y="{TOP1 - 6}">{label}</text>')

    # Panneau 1 : tension efficace de chauffe.
    for v in (0, 4, 8, 12):
        y = y_volt(v)
        add(f'<line class="grid" x1="{LEFT}" x2="{W - RIGHT}" y1="{y:.1f}" y2="{y:.1f}"/>')
        add(f'<text class="ink2" x="{LEFT - 8}" y="{y + 4:.1f}" text-anchor="end">{v} V</text>')
    add(f'<text class="ink" x="{W - RIGHT}" y="{TOP1 + 14}" text-anchor="end" font-weight="600">'
        'Tension efficace de chauffe U<tspan baseline-shift="sub" font-size="9">H,eff</tspan></text>')
    add(polyline([(x_of(a), y_volt(b)) for a, b in zip(t, u)], "series"))

    # Panneau 2 : Ri mesurée par injection, consigne 300 Ω.
    for v, lab in ((100, "100"), (300, "300"), (1000, "1 k"), (3000, "3 k"), (10000, "10 k"), (30000, "30 k")):
        y = y_ri(v)
        add(f'<line class="grid" x1="{LEFT}" x2="{W - RIGHT}" y1="{y:.1f}" y2="{y:.1f}"/>')
        add(f'<text class="ink2" x="{LEFT - 8}" y="{y + 4:.1f}" text-anchor="end">{lab} Ω</text>')
    y300 = y_ri(300)
    add(f'<line class="ref" x1="{LEFT}" x2="{W - RIGHT}" y1="{y300:.1f}" y2="{y300:.1f}"/>')
    add(f'<text class="ink2" x="{W - RIGHT - 4}" y="{y300 - 5:.1f}" text-anchor="end">consigne 300 Ω (fiche)</text>')
    add(f'<text class="ink" x="{W - RIGHT}" y="{TOP2 + 14}" text-anchor="end" font-weight="600">'
        'Ri mesurée par injection (échelle log)</text>')
    add(polyline([(x_of(a), y_ri(b)) for a, b in zip(t, ri)], "series"))

    # Panneau 3 : lambda mesuré (seulement quand la mesure est prête) et lambda simulé.
    for v in (0.8, 0.9, 1.0, 1.1, 1.2):
        y = y_lambda(v)
        add(f'<line class="grid" x1="{LEFT}" x2="{W - RIGHT}" y1="{y:.1f}" y2="{y:.1f}"/>')
        add(f'<text class="ink2" x="{LEFT - 8}" y="{y + 4:.1f}" text-anchor="end">{v:.1f}</text>')
    add(f'<text class="ink" x="{W - RIGHT}" y="{TOP3 + 14}" text-anchor="end" font-weight="600">Lambda</text>')
    lx = W - RIGHT - 250
    add(f'<line class="key" x1="{lx}" x2="{lx + 22}" y1="{TOP3 + 10}" y2="{TOP3 + 10}"/>')
    add(f'<text class="ink2" x="{lx + 28}" y="{TOP3 + 14}">mesuré</text>')
    add(f'<line class="truth" x1="{lx + 86}" x2="{lx + 108}" y1="{TOP3 + 10}" y2="{TOP3 + 10}"/>')
    add(f'<text class="ink2" x="{lx + 114}" y="{TOP3 + 14}">simulé</text>')
    add(polyline([(x_of(a), y_lambda(b)) for a, b in zip(t, lam_true)], "truth"))
    add(polyline([(x_of(a), y_lambda(b)) for a, b, ok in zip(t, lam, ready) if ok], "series"))
    add(f'<text class="ink2" x="{LEFT + 6}" y="{TOP3 + H3 - 8}">'
        'avant {:.1f} s : mesure pas encore prête</text>'.format(t_ready))

    # t = 0 de la fiche et axe du temps commun.
    xb = x_of(t_boost)
    add(f'<line class="ref" x1="{xb:.1f}" x2="{xb:.1f}" y1="{TOP1}" y2="{TOP3 + H3}"/>')
    add(f'<text class="ink2" x="{xb + 4:.1f}" y="{TOP1 + H1 + 16}">t = 0 de la fiche</text>')
    for top, h in ((TOP1, H1), (TOP2, H2), (TOP3, H3)):
        add(f'<line class="axis" x1="{LEFT}" x2="{W - RIGHT}" y1="{top + h}" y2="{top + h}"/>')
    for k in range(0, 61, 10):
        x = x_of(k)
        add(f'<line class="axis" x1="{x:.1f}" x2="{x:.1f}" y1="{TOP3 + H3}" y2="{TOP3 + H3 + 4}"/>')
        add(f'<text class="ink2" x="{x:.1f}" y="{TOP3 + H3 + 18}" text-anchor="middle">{k} s</text>')
    add(f'<text class="ink2" x="{W - RIGHT}" y="{HEIGHT - 8}" text-anchor="end">'
        'temps depuis la mise sous tension (moteur démarré à 1 s)</text>')
    add("</svg>")
    open(dst, "w", encoding="utf-8").write("\n".join(out) + "\n")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
