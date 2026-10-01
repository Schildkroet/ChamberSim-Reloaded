import json, numpy as np
d = json.load(open("../reference/results/8x57is.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
x_, p_ = x * 1e-3, p * 1e5
A, mc, mp = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3
# acceleration term v dv/dx via central differences
a_x = v[1:-1] * (v[2:] - v[:-2]) / (x_[2:] - x_[:-2])
ratio = A * p_[1:-1] / a_x
print("A p / (v dv/dx)  [g]   (mp = %.3f g, mp+mc/2 = %.3f, mp+0.5mc..)" % (mp*1e3, (mp+mc/2)*1e3))
for i in (5, 20, 50, 100, 150, 200, 300, 500, 800, 1200, 1500, 1900):
    print(i, "x=%.1f  p=%.0f bar  v=%.1f  ratio=%.4f g" % (x[i+1], p[i+1], v[i+1], ratio[i]*1e3))
# energy check: e == 0.5*mp*v^2 ?
print("e vs 0.5 mp v^2 max rel diff:", np.max(np.abs(e[1:] - 0.5*mp*v[1:]**2)/e[1:]))
# t check: dt = dx / v_avg ?
vm = 0.5*(v[1:]+v[:-1]); dt = (x_[1:]-x_[:-1]) / np.where(vm>0, vm, np.nan) * 1e3
print("dt_ms from kinematics vs table:", [(i, round(dt[i],5), round(t[i+1]-t[i],5)) for i in (2,5,50,500,1500)])
