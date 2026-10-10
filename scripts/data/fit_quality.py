"""Quality (0..1, the propellant column Qlty) of a powder model fitted to published load data.

Same scale as the original database (steps of 0.05, 0 = unknown). C++ twin: powderfit::fitQuality (src/core/powderfit.cpp).
"""


def quality(v_rms: float, samples: int, p_rms: float | None = None) -> float:
    """v_rms, p_rms: velocity / pressure error (rms) in percent (p_rms only when pressures were fitted); samples: fitted loads."""
    if samples <= 0:
        return 0.0
    q = max(0.0, 1.0 - v_rms / 15.0)
    if p_rms:
        q = (q + max(0.0, 1.0 - p_rms / 30.0)) / 2.0
    q *= min(1.0, samples / 100.0)
    return max(0.05, round(round(q / 0.05) * 0.05, 2))  # a fit is never "unknown" (0)
