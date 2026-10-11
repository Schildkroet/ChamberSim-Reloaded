"""Drops published loads that disagree with the table they come from (extraction errors, atypical bullets).

For every group of loads with the same cartridge, powder and kind (start or maximum load) of at least `min_group` rows the published velocity is
fitted as ln v = a + b ln(bullet mass) + c ln(charge) with a robust loss (the model's own errors play no part). Rows whose velocity is more than
`threshold` (relative, default 0.15) off that fit are dropped. On the pistol and revolver tables (2,078 rows in groups) the fit leaves 5.9 % rms,
the table's own noise floor; 46 rows (mostly solid-copper Lehigh XD, CEB Raptor/Solid, e.g. a 120 gr at 168 m/s next to a 100 gr at 321 m/s) lie
beyond 15 %.
"""
import math
import numpy as np
from scipy.optimize import least_squares


def inconsistent(rows, threshold=0.15, min_group=12):
    """Indices of the rows (dicts with caliber, powder, kind, bullet_gr, charge_g, velocity_ms) to drop."""
    groups = {}
    for i, r in enumerate(rows):
        groups.setdefault((r["caliber"], r["powder"], r["kind"]), []).append(i)
    drop = set()
    for idx in groups.values():
        if len(idx) < min_group:
            continue
        try:
            X = np.array([[1.0, math.log(float(rows[i]["bullet_gr"])), math.log(float(rows[i]["charge_g"]))] for i in idx])
            y = np.array([math.log(float(rows[i]["velocity_ms"])) for i in idx])
        except (ValueError, KeyError):
            continue
        sol = least_squares(lambda b: X @ b - y, np.linalg.lstsq(X, y, rcond=None)[0], loss="soft_l1", f_scale=0.05)
        res = X @ sol.x - y
        drop.update(i for i, e in zip(idx, res) if abs(e) > threshold)
    return drop
