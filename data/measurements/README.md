Own chronograph measurements (muzzle velocity, no pressure). Format as csr_eval input (charge_g, col_mm = L6, jump_mm = RotL Weg).

Assumed, not measured: Vectan SP10 is not in the project data (use a record interpolated between SP9 and SP11: Ba 0.5335, a0 1.19, z1 0.49, z2 0.82);
HDB bullet lengths are scaled from the HDB Universal records of the original database; Fox/Hornady/PPU lengths are the catalogue values.
Result 2026-10-12 (Physical model): mean -2.0 %, rms 3.6 % in muzzle velocity (original -3.2/5.0, published -4.3/5.1). All rifle cartridges.

proofhouse_loads.csv (2026-11-14): .357 Magnum, Fiocchi 158 gr SP, 15.5 gr Vihtavuori N110, CCI small rifle primer, from a proof house (manometer barrel 150 mm: 441.5 m/s, 2174.2 bar; Pedersoli rifle, 610 mm: 551.0 m/s, no pressure). Bullet length, seating depth and case volume are those of the LOS SWC report of the same cartridge (col_mm = 40.31 and case_scale 1.0054 reproduce them with the project caliber). Used to choose the burn form of N110 (analysis/form_references.json) and checked by analysis/refit.sh.
