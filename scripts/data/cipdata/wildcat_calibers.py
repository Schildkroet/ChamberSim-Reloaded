#!/usr/bin/env python3
"""Writes caliber records of wildcat cartridges from the dimensioned case drawings of Zeglin's "Reloader's Handbook of Wildcat Cartridge Design".

    wildcat_calibers.py [--data data] [--dry-run]

The drawings give, in inch: the mouth (diameter, length L3), the end of the shoulder (neck diameter, L2), the start of the shoulder (body diameter
P2, L1), the shoulder angle (half angle), the diameter at the extractor groove / base (P1) and its distance from the head (E), the rim (diameter R1,
thickness R). The cartridge length L6 and the bullet diameter G1 are those of the Hornady Handbook (hornady_handbook_extract.py). Pmax is that of
the parent case (the book gives no pressures), V is estimated from the geometry like cip_estimate_v.py (the 6.5-284 drawing states 67.02 gr).
The values were read off the drawings by eye; the drawn dimensions are all given to 0.0001 in.
"""
import argparse
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cip_estimate_v import capacity  # noqa: E402
from recordfiles import slug  # noqa: E402

IN = 25.4
# name: parent, type, primer, (mouth dia, L3), (neck dia at shoulder end, L2), (body dia at shoulder, L1), half angle deg,
#       (base dia P1, E groove distance; E None for a rimmed case), (rim dia, rim thickness), Hornady L6 (in), bullet dia (in), book page, V stated
CARTRIDGES = {
    "338-06": dict(parent="30-06 Spring.", alt=".338-06", ciptype="1 rimless", primer="LR", mouth=(0.3650, 2.4940), neck=(0.3650, 2.0680),
                   shoulder=(0.4410, 1.9480), angle=17.5, base=(0.4710, 0.1250), rim=(0.4730, 0.0490), l6=3.340, dia=0.338, page=93),
    "6.5-06": dict(parent="30-06 Spring.", alt=".264-06,6.5mm-06", ciptype="1 rimless", primer="LR", mouth=(0.2980, 2.4940), neck=(0.2990, 2.1732),
                   shoulder=(0.4410, 1.9480), angle=17.5, base=(0.4710, 0.1360), rim=(0.4730, 0.0490), l6=3.340, dia=0.264, page=177),
    "8mm-06": dict(parent="30-06 Spring.", alt=".323-06,8 mm-06", ciptype="1 rimless", primer="LR", mouth=(0.3540, 2.4940), neck=(0.3540, 2.0860),
                   shoulder=(0.4410, 1.9480), angle=17.5, base=(0.4710, 0.1250), rim=(0.4730, 0.0490), l6=3.228, dia=0.323, page=177),
    "22 K-Hornet": dict(parent="22 Hornet", alt=".22 K-Hornet,22 Kilbourn Magnum Hornet", ciptype="2 rimmed", primer="SR", mouth=(0.2415, 1.3850),
                        neck=(0.2415, 1.0382), shoulder=(0.2860, 1.0117), angle=40.0, base=(0.2967, None), rim=(0.3500, 0.0650), l6=1.723,
                        dia=0.224, page=158),
    "22 BR Rem.": dict(parent="22 PPC-USA", alt=".22 BR Remington,22 Bench Rest Remington", ciptype="1 rimless", primer="SR", mouth=(0.2460, 1.5200),
                       neck=(0.2460, 1.2600), shoulder=(0.4580, 1.0750), angle=30.0, base=(0.4730, 0.1400), rim=(0.4730, 0.0490), l6=2.020,
                       dia=0.224, page=213),
    "6.5mm TCU": dict(parent="223 Rem.", alt=".264 TCU,6.5 TCU", ciptype="1 rimless", primer="SR", mouth=(0.2920, 1.7490), neck=(0.2930, 1.4942),
                      shoulder=(0.3680, 1.4495), angle=40.0, base=(0.3760, 0.1150), rim=(0.3780, 0.0450), l6=2.700, dia=0.264, page=216),
    "7mm TCU": dict(parent="223 Rem.", alt=".284 TCU,7 TCU", ciptype="1 rimless", primer="SR", mouth=(0.3000, 1.7490), neck=(0.3010, 1.4895),
                    shoulder=(0.3680, 1.4495), angle=40.0, base=(0.3760, 0.1150), rim=(0.3780, 0.0450), l6=2.675, dia=0.284, page=216),
    # a military cartridge, not a wildcat: dimensions from the Hornady Handbook drawing (p. 639; 25\u00b030' half angle), Pmax from Cartridges of the World
    # ("military loads develop about 42,000 psi"), 7.7x58 Arisaka / Type 99
    "7.7x58 Arisaka": dict(parent=None, page=639, alt="7.7 x 58 Jap.,7.7 Japanese,.31 Jap,7.7x58mm Japanese Arisaka", ciptype="1 rimless", primer="LR",
                           mouth=(0.337, 2.270), neck=(0.338, 1.968), shoulder=(0.429, 1.872), angle=25.5, base=(0.470, 0.150), rim=(0.471, 0.048),
                           l6=3.100, dia=0.312, pmax=2896, ciporigin="JP", source="Hornady Handbook 10th ed. (case drawing, p. 639); Pmax: Cartridges of the World, 15th ed.",
                           note="Pmax is the 42,000 psi of the military loads (Cartridges of the World)"),
    "6.5-284 Win.": dict(parent="284 Win.", alt=".264-284,6.5mm-284,6.5/284 Winchester", ciptype="1 rimless", primer="LR", mouth=(0.2880, 2.1700),
                         neck=(0.2880, 1.8854), shoulder=(0.4748, 1.7741), angle=40.0, base=(0.5008, 0.1487), rim=(0.4730, 0.0540), l6=2.800,
                         dia=0.264, page=559, v=67.02),
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    parents = {}
    for f in (args.data / "calibers").glob("*.json"):
        r = json.loads(f.read_text(encoding="utf-8"))
        parents[r["cipname"]] = r
    for name, c in CARTRIDGES.items():
        p = parents[c["parent"]] if c["parent"] else {}
        rec = {"cipname": name, "altname": c["alt"], "standard": "WildCat", "ciporigin": c.get("ciporigin", "US"), "ciptype": c["ciptype"], "primer": c["primer"],
               "L1": round(c["shoulder"][1] * IN, 2), "L2": round(c["neck"][1] * IN, 2), "L3": round(c["mouth"][1] * IN, 2),
               "L6": round(c["l6"] * IN, 2), "R": round(c["rim"][1] * IN, 2), "R1": round(c["rim"][0] * IN, 2),
               "P1": round(c["base"][0] * IN, 2), "P2": round(c["shoulder"][0] * IN, 2), "alpha": str(2 * c["angle"]),
               "H1": round(c["neck"][0] * IN, 2), "H2": round(c["mouth"][0] * IN, 2), "G1": round(c["dia"] * IN, 2)}
        if c["base"][1] is not None:
            rec["E"] = round(c["base"][1] * IN, 2)
        pmax = c.get("pmax") or (p["Pmax"] if not p.get("PmaxSAAMI") else min(p["Pmax"], p["PmaxSAAMI"]))  # load data makers use the lower of the two
        rec["Pmax"] = pmax
        v = c.get("v") or round(capacity(rec), 1)
        rec["V"] = v
        rec["c_Q"] = round(math.pi / 4 * rec["G1"] ** 2, 2)
        rec["origin"] = c.get("source") or f"Zeglin, Wildcat Cartridges (case drawing, p. {c['page']}); L6 and bullet diameter: Hornady Handbook 10th ed."
        rec["descr"] = (f"Dimensions read off the case drawing; {c['note'] if c.get('note') else 'Pmax of the parent case ' + c['parent'] + ' (the book gives none)'}; "
                        + ("V from the drawing" if c.get("v") else "V estimated from the geometry (wall 0.45 mm, head 0.50 x P1)"))
        path = args.data / "calibers" / (slug(name) + ".json")
        print(f"{name:14s} V {v:5.1f} gr  L3 {rec['L3']} L6 {rec['L6']} Pmax {pmax}  -> {path.name}")
        if not args.dry_run:
            path.write_text(json.dumps(rec, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
