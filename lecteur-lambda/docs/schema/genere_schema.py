"""Schéma électronique PROVISOIRE du lecteur lambda LSU ADV (pilotage direct, Pico).

Trois feuilles SVG, reliées par des étiquettes de réseau (VG, VM, IP, UN, V_CMD...) :
    1-alimentation-chauffe.svg   entrée 12 V, protections, 5 V, chauffage, mesure batterie
    2-interface-sonde.svg        masse virtuelle, pompe, mesure de Ip, Nernst et injection
    3-pico-ecran-sortie.svg      brochage du Pico, écran TFT, sortie vers le Speeduino

Les valeurs suivent config.h (section 4) et la carte de référence rusEFI. Usage :
    pip install schemdraw
    python3 genere_schema.py
"""
import os

import schemdraw
import schemdraw.elements as elm

schemdraw.use("svg")
HERE = os.path.dirname(os.path.abspath(__file__))
FRAME = "#8a8984"


def drawing():
    d = schemdraw.Drawing(show=False)
    d.config(unit=2.2, fontsize=11, font="sans-serif", bgcolor="white", lw=1.4)
    return d


BLOCKS = []
EXTRA = []  # points (x, y) que les cadres doivent englober : bouts d'étiquettes, notes


def begin(d):
    """Début d'un bloc : à passer à block() une fois le bloc dessiné."""
    return (len(d.elements), len(EXTRA))


def note(at, text, fontsize=10):
    """Texte libre aligné à gauche ; son étendue estimée est englobée par le cadre du bloc."""
    lines = text.split("\n")
    w = 0.165 * fontsize / 11 * max(len(t) for t in lines)
    h = 0.42 * fontsize / 11 * len(lines)
    EXTRA.extend([(at[0], at[1] + h / 2), (at[0] + w, at[1] - h / 2)])
    return elm.Label().at(at).label(text, halign="left", fontsize=fontsize)


def block(d, start, title, margin=(0.9, 0.9, 0.9, 0.9)):
    """Note les éléments ajoutés depuis 'start' : leur cadre est dessiné à la fin (frame_blocks).
    margin = (gauche, droite, bas, haut), pour laisser la place aux textes."""
    # schemdraw n'ajoute un élément à d.elements qu'à la création du suivant : les deux bornes,
    # relevées pendant le dessin, sont donc en retard d'un élément.
    BLOCKS.append((start[0] + 1, len(d.elements) + 1, start[1], len(EXTRA), title, margin))


def frame_blocks(d):
    """Une fois tous les éléments placés : cadre pointillé et titre autour de chaque bloc."""
    for start, end, xstart, xend, title, margin in BLOCKS:
        xs = [p[0] for p in EXTRA[xstart:xend]]
        ys = [p[1] for p in EXTRA[xstart:xend]]
        for e in d.elements[start:end]:
            if isinstance(e, elm.Label):  # textes seuls : couverts par les marges
                continue
            for pt in e.absanchors.values():
                xs.append(pt[0])
                ys.append(pt[1])
        ml, mr, mb, mt = margin
        x0, x1, y0, y1 = min(xs) - ml, max(xs) + mr, min(ys) - mb, max(ys) + mt
        corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]
        for p, q in zip(corners, corners[1:]):
            d.add(elm.Line().at(p).to(q).linestyle("--").color(FRAME).linewidth(1.0))
        if os.environ.get("SCHEMA_DEBUG"):
            print(f"{title}: x {x0:.1f}..{x1:.1f}  y {y0:.1f}..{y1:.1f}")
        d.add(elm.Label().at((x0 + 0.2, y1 + 0.5)).label(title, halign="left", fontsize=12, color="#33332f"))
    BLOCKS.clear()
    EXTRA.clear()


def tag_width(name):
    return 0.19 * len(name) + 0.5


def tag_left(at, name):
    """Étiquette de réseau dont la pointe touche le fil en 'at', texte à gauche."""
    EXTRA.append((at[0] - tag_width(name) - 0.1, at[1]))
    return elm.Tag(width=tag_width(name)).at(at).left().label(name)


def tag_right(at, name):
    """Étiquette de réseau dont la pointe touche le fil en 'at', texte à droite."""
    EXTRA.append((at[0] + tag_width(name) + 0.1, at[1]))
    return elm.Tag(width=tag_width(name)).at(at).right().label(name)


def follower(at_plus, label):
    """Ampli op suiveur : + en haut, - relié à la sortie par en dessous. Renvoie l'ampli."""
    op = elm.Opamp(leads=True).flip().anchor("in2").at(at_plus)
    elm.Label().at(op.center).label(label, fontsize=10)
    elm.Line().at(op.in1).down(1.0)
    p = elm.Line().right().tox(op.out)
    elm.Line().up().toy(op.out)
    elm.Dot().at(op.out)
    return op


# ======================================================================================
# Feuille 1 : alimentation et chauffage
# ======================================================================================
def sheet1(path):
    d = drawing()
    with d:
        elm.Label().at((-5.0, 6.4)).label(
            "Feuille 1 / 3 : alimentation et chauffage (schéma provisoire)", halign="left", fontsize=15)

        # --- Entrée 12 V, protections, 5 V ---
        s = begin(d)
        tag_left((0, 0), "+12 V après contact")
        elm.Line().at((0, 0)).right(0.8)
        n0 = elm.Dot()
        elm.Fuse().right().label("F1  1 A")
        elm.Schottky().right().label("D1 SS34\nanti-inversion")
        n1 = elm.Dot()
        elm.Line().right(2.2)
        c1t = elm.Dot()
        elm.Line().right(2.2)
        c2t = elm.Dot()
        elm.Line().right(1.8)
        u4 = elm.Ic(pins=[elm.IcPin(name="IN", side="left"), elm.IcPin(name="OUT", side="right"),
                          elm.IcPin(name="GND", side="bottom")], size=(3.4, 1.8)).anchor("IN")
        elm.Label().at((u4.center[0], u4.center[1] + 1.9)).label("U4 module abaisseur 12 V → 5 V\n(MP1584, LM2596…)")
        EXTRA.append((u4.center[0], u4.center[1] + 2.5))
        elm.Line().at(u4.GND).down(0.6)
        elm.Ground()
        elm.DiodeTVS().at(n1.center).down().label("D2\nSMBJ18CA", loc="bottom")
        elm.Ground()
        elm.Capacitor(polar=True).at(c1t.center).down().label("C1 100 µF\n35 V", loc="bottom")
        elm.Ground()
        elm.Capacitor().at(c2t.center).down().label("C2\n100 nF", loc="bottom")
        elm.Ground()
        elm.Line().at(u4.OUT).right(1.0)
        n5 = elm.Dot()
        elm.Line().right(2.2)
        c3t = elm.Dot()
        elm.Schottky().right().label("D3 SS14")
        tag_right(d.here, "VSYS Pico (broche 39)")
        elm.Capacitor(polar=True).at(c3t.center).down().label("C3\n22 µF", loc="bottom")
        elm.Ground()
        elm.Line().at(n5.center).up(1.4)
        tag_right(d.here, "+5V vers U3 (feuille 3)")
        # Branche du chauffage, sur son propre fusible.
        elm.Line().at(n0.center).down(5.0)
        elm.Fuse().right().label("F2  7,5 A (à dimensionner)")
        elm.Line().right(1.0)
        tag_right(d.here, "+12V_CH → J2 broche 4 (H+)")
        block(d, s, "Entrée 12 V, protections et 5 V")

        # --- Commande du chauffage ---
        s = begin(d)
        tag_left((35, 0), "H− ← J2 broche 3")
        elm.Line().at((35, 0)).right(1.0)
        nd = elm.Dot()
        elm.Line().down(1.2)
        q = elm.NFet().anchor("drain")
        elm.Label().at((q.center[0] - 0.6, q.center[1] - 2.1)).label("Q1  MOSFET N logique\n(AO3400A, IRLML6344)",
                                                                     halign="right", fontsize=10)
        elm.Line().at(q.source).down(0.8)
        elm.Ground()
        elm.Line().at(q.gate).right(1.2)
        ng = elm.Dot()
        elm.Resistor().right().label("R30  100 Ω")
        tag_right(d.here, "GP15 PWM chauffe")
        elm.Resistor().at(ng.center).down().label("R31\n100 kΩ", loc="bottom")
        elm.Ground()
        elm.Line().at(nd.center).up(0.6)
        elm.Schottky().up().label("D4 SS34\nroue libre", loc="bottom")
        tag_right(d.here, "+12V_CH")
        block(d, s, "Commande du chauffage (PWM 200 Hz)")

        # --- Mesure de la tension batterie ---
        s = begin(d)
        tag_left((0, -10.5), "+12V_CH")
        elm.Line().at((0, -10.5)).right(1.0)
        elm.Resistor().down().label("R20\n10 kΩ")
        nb = elm.Dot()
        elm.Resistor().down().label("R21\n2 kΩ")
        elm.Ground()
        elm.Line().at(nb.center).right(2.2)
        ncap = elm.Dot()
        elm.Capacitor().down().label("C20\n100 nF", loc="bottom")
        elm.Ground()
        elm.Resistor().at(ncap.center).right().label("R22  1 kΩ")
        na = elm.Dot()
        elm.Line().right(2.8)
        tag_right(d.here, "GP28 ADC2 (Ubat)")
        elm.Schottky().at(na.center).up().label("D5 BAT54S", loc="bottom")
        elm.Vdd().label("3V3_A")
        elm.Schottky().at(na.center).down().reverse()
        elm.Ground()
        block(d, s, "Mesure de la batterie (÷ 6)")

        # --- Alimentation des amplis ---
        s = begin(d)
        tag_left((22, -10.5), "3V3 OUT Pico (broche 36)")
        elm.Line().at((22, -10.5)).right(0.6)
        elm.Inductor2(loops=2).right().label("FB1 ferrite (ou 10 Ω)")
        nv = elm.Dot()
        elm.Line().right(2.2)
        c11 = elm.Dot()
        elm.Line().right(1.0)
        tag_right(d.here, "3V3_A")
        elm.Capacitor().at(nv.center).down().label("C10\n10 µF", loc="bottom")
        elm.Ground().label("AGND", loc="right", fontsize=9)
        elm.Capacitor().at(c11.center).down().label("C11\n100 nF", loc="bottom")
        elm.Ground()
        note((16.8, -15.4),
            "AGND = broche 33 du Pico. La masse de puissance du\n"
            "chauffage revient séparément à la batterie (étoile).")
        block(d, s, "Alimentation des amplis : 3V3_A")
    frame_blocks(d)
    d.save(path)


# ======================================================================================
# Feuille 2 : interface sonde
# ======================================================================================
def sheet2(path):
    d = drawing()
    with d:
        elm.Label().at((0, 4.4)).label(
            "Feuille 2 / 3 : interface de la sonde LSU ADV (schéma provisoire)", halign="left", fontsize=15)

        # --- A. Masse virtuelle ---
        s = begin(d)
        elm.Vdd().at((0, 1.2)).label("3V3_A")
        elm.Resistor().at((0, 1.2)).down().label("R1\n10 kΩ")
        na = elm.Dot()
        elm.Resistor().down().label("R2\n10 kΩ")
        elm.Ground()
        elm.Line().at(na.center).right(1.8)
        nc = elm.Dot()
        elm.Capacitor().down().label("C12\n1 µF", loc="bottom")
        elm.Ground()
        elm.Line().at(nc.center).right(1.0)
        op = follower(d.here, "U1A")
        elm.Line().at(op.out).right(0.8)
        nvg = elm.Dot()
        elm.Line().up(1.2)
        tag_right(d.here, "VG  1,65 V")
        elm.Resistor().at(nvg.center).right().label("R3  10 Ω")
        tag_right(d.here, "VM → J2 broche 2")
        block(d, s, "A. Masse virtuelle 1,65 V (VM)")

        # --- F. Connecteur de la sonde ---
        s = begin(d)
        # schemdraw liste les broches d'un côté de bas en haut : broche 1 en dernier pour l'avoir en haut.
        j2 = elm.Ic(pins=[elm.IcPin(name="UN / RE", pin="6", side="left", anchorname="P6"),
                          elm.IcPin(name="n.c.", pin="5", side="left", anchorname="P5"),
                          elm.IcPin(name="Uh+ / H+", pin="4", side="left", anchorname="P4"),
                          elm.IcPin(name="Uh− / H−", pin="3", side="left", anchorname="P3"),
                          elm.IcPin(name="VM / IPN", pin="2", side="left", anchorname="P2"),
                          elm.IcPin(name="IP / APE", pin="1", side="left", anchorname="P1")],
                    size=(3.4, 6.0), pinspacing=0.9, edgepadH=0.5).theta(0).at((34, -4.2))
        elm.Label().at((j2.center[0], j2.center[1] + 3.8)).label("J2  sonde LSU ADV\n(contre-pièce F02U.B00.725-01)")
        EXTRA.extend([(j2.center[0] + 1.8, j2.center[1] + 4.4), (j2.center[0] + 2.6, j2.center[1] - 3.1)])
        for anchor, name in (("P1", "IP"), ("P2", "VM"), ("P3", "H− (Q1)"), ("P4", "+12V_CH"), ("P6", "UN")):
            elm.Line().at(getattr(j2, anchor)).left(0.8)
            tag_left(d.here, name)
        elm.NoConnect().at(j2.P5)
        block(d, s, "F. Connecteur de la sonde")

        # --- B. Filtre de la commande de pompe ---
        s = begin(d)
        y = -10.5
        tag_left((0, y), "GP10 PWM pompe")
        elm.Resistor().at((0, y)).right().label("R4\n10 kΩ")
        a = elm.Dot()
        elm.Resistor().right().label("R5\n10 kΩ")
        b = elm.Dot()
        elm.Line().right(0.8)
        op = follower(d.here, "U2B")
        elm.Capacitor().at(b.center).down().label("C5\n10 nF")
        elm.Ground()
        elm.Line().at(a.center).up(2.0)
        elm.Capacitor().right().label("C4  22 nF")
        elm.Line().right().tox(op.out)
        elm.Line().down().toy(op.out)
        elm.Line().at(op.out).right(0.8)
        tag_right(d.here, "V_CMD")
        block(d, s, "B. Filtre de commande (Sallen-Key, ~1 kHz)")

        # --- C. Source de courant de pompe (Howland) et shunt ---
        s = begin(d)
        y = -10.5
        x = 21.0
        tag_left((x, y), "V_CMD")
        elm.Resistor().at((x, y)).right().label("R6\n10 kΩ")
        p = elm.Dot()
        elm.Line().right(0.8)
        op = elm.Opamp(leads=True).flip().anchor("in2")
        elm.Label().at(op.center).label("U1B", fontsize=10)
        elm.Resistor().at(op.out).right().label("R10\n316 Ω")
        nl = elm.Dot()
        elm.Resistor().right().label("R11  61,9 Ω\n0,1 %")
        nb2 = elm.Dot()
        elm.Line().right(0.8)
        tag_right(d.here, "IP → J2 broche 1")
        # R7 de + vers la sortie (au-dessus).
        elm.Line().at(p.center).up(2.0)
        elm.Resistor().right().label("R7  10 kΩ")
        elm.Line().right().tox(nl.center)
        elm.Line().down().toy(nl.center)
        # R8 de VG vers -, R9 de - vers la sortie (en dessous).
        elm.Line().at(op.in1).left(0.6)
        nm = elm.Dot()
        elm.Resistor().left().label("R8\n10 kΩ", loc="bottom")
        tag_left(d.here, "VG")
        elm.Line().at(nm.center).down(1.6)
        elm.Resistor().right().label("R9  10 kΩ", loc="bottom")
        elm.Line().right().tox(op.out)
        elm.Line().up().toy(op.out)
        elm.Dot().at(op.out)
        # Points de mesure du shunt.
        elm.Line().at(nl.center).down(1.6)
        tag_left(d.here, "SH_A")
        elm.Line().at(nb2.center).down(1.6)
        tag_right(d.here, "SH_B")
        note((x + 1.0, y - 4.6),
            "Ip ≈ (V_CMD − VG) ÷ 316 Ω : +3,2 mA par volt. Sens à vérifier au banc\n"
            "(config.h : CIRCUIT_V_PAR_MA_POMPE = +0,316 V/mA).")
        block(d, s, "C. Source de courant de pompe (Howland) et shunt")

        # --- D. Mesure du courant de pompe ---
        s = begin(d)
        y = -21.5
        tag_left((0, y), "SH_A")
        elm.Resistor().at((0, y)).right().label("R12\n100 kΩ")
        p2 = elm.Dot()
        elm.Line().right(0.8)
        op = elm.Opamp(leads=True).flip().anchor("in2")
        elm.Label().at(op.center).label("U1C", fontsize=10)
        elm.Line().at(p2.center).up(2.0)
        elm.Resistor().right().label("R13  1 MΩ")
        tag_right(d.here, "VG")
        tag_left((0, op.in1[1]), "SH_B")
        elm.Resistor().at((0, op.in1[1])).right().label("R14\n100 kΩ", loc="bottom")
        m2 = elm.Dot()
        elm.Line().right().tox(op.in1)
        elm.Line().at(m2.center).down(1.6)
        elm.Resistor().right().label("R15  1 MΩ", loc="bottom")
        elm.Line().right().tox(op.out)
        elm.Line().up().toy(op.out)
        elm.Dot().at(op.out)
        elm.Resistor().at(op.out).right().label("R16\n10 kΩ")
        nf = elm.Dot()
        elm.Capacitor().down().label("C16\n100 nF", loc="bottom")
        elm.Ground()
        elm.Line().at(nf.center).right(0.8)
        tag_right(d.here, "GP27 ADC1 (Ip)")
        note((0.5, y - 7.0),
            "Sortie = VG + 10 × 61,9 Ω × Ip, soit 0,619 V/mA. Résistances 0,1 % appariées.")
        block(d, s, "D. Mesure du courant de pompe (× 10)")

        # --- E. Cellule de Nernst : injection et mesure ---
        s = begin(d)
        y = -21.5
        x = 21.0
        tag_left((x, y), "GP14 injection")
        elm.Resistor().at((x, y)).right().label("R26  22 kΩ\n1 %")
        nun = elm.Dot()
        elm.Line().up(1.4)
        tag_right(d.here, "UN ← J2 broche 6")
        # Option : courant de référence entrant dans RE (voir README du schéma).
        elm.Line().at(nun.center).down(0.8)
        elm.Resistor().down().linestyle("--").label("R_REF 30 kΩ\n(option,\nnon montée)")
        tag_left(d.here, "3V3_A")
        elm.Line().at(nun.center).right(0.6)
        op = follower(d.here, "U1D")
        elm.Resistor().at(op.out).right().label("R18\n10 kΩ")
        p3 = elm.Dot()
        elm.Line().right(0.6)
        op2 = elm.Opamp(leads=True).flip().anchor("in2")
        elm.Label().at(op2.center).label("U2A", fontsize=10)
        elm.Line().at(p3.center).up(1.8)
        elm.Resistor().right().label("R19  27 kΩ")
        elm.Ground().right()
        elm.Line().at(op2.in1).left(0.6)
        m3 = elm.Dot()
        elm.Line().left(1.2)
        elm.Resistor().down().label("R23\n10 kΩ")
        elm.Line().down(0.6)
        tag_left(d.here, "VM (mesure, J2 broche 2)")
        elm.Line().at(m3.center).down(1.6)
        elm.Resistor().right().label("R24  27 kΩ", loc="bottom")
        elm.Line().right().tox(op2.out)
        elm.Line().up().toy(op2.out)
        elm.Dot().at(op2.out)
        elm.Resistor().at(op2.out).right().label("R25\n1 kΩ")
        nf2 = elm.Dot()
        elm.Capacitor().down().label("C25\n22 nF", loc="bottom")
        elm.Ground()
        elm.Line().at(nf2.center).right(0.8)
        tag_right(d.here, "GP26 ADC0 (Nernst)")
        note((x + 0.5, y - 7.0),
            "Sortie = 2,7 × (UN − VM). U1D évite de charger l'injection de 22 kΩ ;\n"
            "VM est pris directement sur la broche 2 (config.h : CIRCUIT_R_SERIE_VM_OHM = 0).\n"
            "R_REF : courant de référence entrant dans RE, à décider (README du schéma).")
        block(d, s, "E. Cellule de Nernst : injection pour Ri et mesure (× 2,7)")

        note((0, -32.0),
            "U1, U2 : MCP6004 (4 amplis op rail à rail, alimentés en 3V3_A ; broche 4 → 3V3_A, broche 11 → AGND, "
            "100 nF au plus près). U2C et U2D inutilisés : en suiveurs, entrée + à VG.")
    frame_blocks(d)
    d.save(path)


# ======================================================================================
# Feuille 3 : Pico, écran, sortie Speeduino
# ======================================================================================
def sheet3(path):
    d = drawing()
    with d:
        elm.Label().at((-6.0, 8.6)).label(
            "Feuille 3 / 3 : Raspberry Pi Pico, écran et sortie Speeduino (schéma provisoire)",
            halign="left", fontsize=15)

        # --- Pico ---
        s = begin(d)
        left = [("GP4", "6", "SDA (option I2C)"), ("GP5", "7", "SCL (option I2C)"), ("GP10", "14", "PWM pompe"),
                ("GP13", "17", "PWM sortie Speeduino"), ("GP14", "19", "injection Ri"), ("GP15", "20", "PWM chauffe")]
        right = [("GP17", "22", "TFT CS"), ("GP18", "24", "TFT SCK"), ("GP19", "25", "TFT SDA (MOSI)"),
                 ("GP20", "26", "TFT A0 (DC)"), ("GP21", "27", "TFT RESET"), ("GP26", "31", "ADC0 Nernst"),
                 ("GP27", "32", "ADC1 Ip"), ("GP28", "34", "ADC2 Ubat")]
        power = [("3V3 OUT", "36", ""), ("VSYS", "39", ""), ("AGND", "33", ""), ("GND", "38", "")]
        # schemdraw liste les broches d'un côté de bas en haut : listes inversées.
        pins = [elm.IcPin(name=n, pin=p, side="left", anchorname=n) for n, p, _ in reversed(left)]
        pins += [elm.IcPin(name=n, pin=p, side="right", anchorname=n) for n, p, _ in reversed(right)]
        pins += [elm.IcPin(name=n, pin=p, side="top", anchorname=n.replace(" ", "_"), pos=pos)
                 for (n, p, _), pos in zip(power[:2], (0.25, 0.75))]
        pins += [elm.IcPin(name=n, pin=p, side="bottom", anchorname=n, pos=pos)
                 for (n, p, _), pos in zip(power[2:], (0.25, 0.75))]
        pico = elm.Ic(pins=pins, size=(7.0, 9.0), label="Raspberry Pi Pico\n(RP2040)").theta(0).at((9, -4))
        for n, _, net in left:
            elm.Line().at(getattr(pico, n)).left(0.8)
            tag_left(d.here, net)
        for n, _, net in right:
            elm.Line().at(getattr(pico, n)).right(0.8)
            tag_right(d.here, net)
        elm.Line().at(pico.__getattr__("3V3_OUT")).up(0.8)
        elm.Line().left(1.2)
        tag_left(d.here, "3V3 → FB1 (feuille 1)")
        elm.Line().at(pico.VSYS).up(0.8)
        elm.Line().right(1.2)
        tag_right(d.here, "VSYS ← D3 (feuille 1)")
        elm.Line().at(pico.AGND).down(0.8)
        elm.Ground().label("AGND", loc="left", fontsize=9)
        elm.Line().at(pico.GND).down(0.8)
        elm.Ground()
        block(d, s, "Raspberry Pi Pico : brochage du programme (config.h, section 7)")

        # --- Écran TFT ---
        s = begin(d)
        tft_pins = [("VCC", "3V3"), ("GND", "GND"), ("CS", "TFT CS"), ("RESET", "TFT RESET"), ("A0", "TFT A0 (DC)"),
                    ("SDA", "TFT SDA (MOSI)"), ("SCK", "TFT SCK"), ("LED", "3V3")]
        tft = elm.Ic(pins=[elm.IcPin(name=n, side="left", anchorname="T" + n) for n, _ in reversed(tft_pins)],
                     size=(5.2, 7.0), pinspacing=0.8, label="Écran TFT\nST7735\n160 × 128").theta(0).at((31, -3.8))
        EXTRA.append((tft.center[0] + 2.8, tft.center[1]))
        for n, net in tft_pins:
            elm.Line().at(getattr(tft, "T" + n)).left(0.8)
            if net is None:
                elm.Ground()
            else:
                tag_left(d.here, net)
        note((tft.center[0] - 4.5, tft.center[1] - 5.2),
            "Variante : OLED SSD1306 ou LCD 16 × 2\nen I2C sur GP4 / GP5 (CFG_ECRAN).")
        block(d, s, "Écran (comme le croquis jan31a)")

        # --- Sortie Speeduino ---
        s = begin(d)
        y = -13.5
        tag_left((0, y), "PWM sortie Speeduino")
        elm.Resistor().at((0, y)).right().label("R40\n10 kΩ")
        c40 = elm.Dot()
        elm.Resistor().right().label("R41\n10 kΩ")
        c41 = elm.Dot()
        elm.Line().right(1.8)
        op = elm.Opamp(leads=True).flip().anchor("in2")
        elm.Label().at(op.center).label("U3A", fontsize=10)
        elm.Capacitor().at(c40.center).down().label("C40\n100 nF")
        elm.Ground()
        elm.Capacitor().at(c41.center).down().label("C41\n100 nF")
        elm.Ground()
        elm.Resistor().at(op.out).right().label("R44\n100 Ω")
        elm.Line().right(1.0)
        j3 = elm.Ic(pins=[elm.IcPin(name="masse capteurs", side="left", anchorname="G"),
                          elm.IcPin(name="O2 (A8)", side="left", anchorname="O2")],
                    size=(3.6, 2.2)).theta(0).anchor("O2")
        elm.Label().at((j3.center[0], j3.center[1] + 1.7)).label("J3  Speeduino v0.4")
        EXTRA.extend([(j3.center[0] + 1.9, j3.center[1] + 2.1), (j3.center[0] + 1.9, j3.center[1] - 1.2)])
        elm.Line().at(j3.G).left(1.0)
        elm.Ground()
        # Ampli non inverseur : R42 de - à la masse, R43 de - à la sortie.
        elm.Line().at(op.in1).left(0.6)
        elm.Line().down(1.2)
        nm = elm.Dot()
        elm.Resistor().down().label("R42\n10 kΩ")
        elm.Ground()
        elm.Line().at(nm.center).right(0.4)
        elm.Resistor().right().label("R43  5,1 kΩ", loc="bottom")
        elm.Line().right().tox(op.out)
        elm.Line().up().toy(op.out)
        elm.Dot().at(op.out)
        note((0.5, y - 6.8),
            "Gain 1 + 5,1 / 10 = 1,51 : 0-3,3 V → 0-5 V. U3 : MCP6002 alimenté en +5V (broche 8), "
            "broche 4 à la masse.\nVariante : MCP4725 en 5 V sur l'I2C, avec adaptateur de niveaux.")
        block(d, s, "Sortie vers le Speeduino : 0-5 V = AFR 10-20")
    frame_blocks(d)
    d.save(path)


if __name__ == "__main__":
    sheet1(os.path.join(HERE, "1-alimentation-chauffe.svg"))
    sheet2(os.path.join(HERE, "2-interface-sonde.svg"))
    sheet3(os.path.join(HERE, "3-pico-ecran-sortie.svg"))
    print("ok")
