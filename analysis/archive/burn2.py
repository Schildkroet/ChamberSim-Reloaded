import json, numpy as np
R = "../../reference/results/"
runs = ["exp_base","exp_mc_x0.6","exp_mc_x0.8","exp_mc_x1.2","exp_mp_x0.5","exp_mp_x2.0","exp_mp_x4.0","exp_sebert_0.0","exp_sebert_1.0","exp_ps_50","exp_ps_500"]
def at(name, zq):
    P = json.load(open(R + name + ".json"))["points"]
    x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    dzdt = v * np.gradient(z, x * 1e-3); zi = np.maximum.accumulate(z)
    i = np.searchsorted(zi, zq); return p[i], dzdt[i]
for zq in (0.1, 0.3, 0.4):
    pp, rr = zip(*[at(n, zq) for n in runs]); pp, rr = np.array(pp), np.array(rr)
    A = np.column_stack([pp, np.ones_like(pp)]); (a, c), *_ = np.linalg.lstsq(A, rr, rcond=None)
    A2 = np.column_stack([np.log(pp), np.ones_like(pp)]); (n_, lc), *_ = np.linalg.lstsq(A2, np.log(rr), rcond=None)
    print("z=%.2f  linear: dz/dt = %.4f*p + %.1f   resid rms %.1f (/s) | power law: dz/dt ~ p^%.4f   resid(log) rms %.4f" % (
        zq, a, c, np.sqrt(np.mean((A @ [a, c] - rr)**2)), n_, np.sqrt(np.mean((A2 @ [n_, lc] - np.log(rr))**2))))
    for n, p_, r_ in zip(runs, pp, rr): print("     %-16s p=%7.1f  dz/dt=%8.1f" % (n[4:], p_, r_))
