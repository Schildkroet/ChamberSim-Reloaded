# Introduction

ChamberSim Reloaded calculates the *interior ballistics* of a handload:
you choose a cartridge, a bullet and a powder, enter the powder charge,
and the program simulates how the pressure, the bullet velocity and the
bullet energy develop while the bullet travels through the barrel. From
that it reports the maximum pressure, the muzzle velocity, the barrel
time and many related values. A load development ladder, charts, reports
and printouts help to plan a series of test loads.

It follows the workflow of *Gordon’s Reloading Tool* (GRT). It can open
the original’s `.grtload` files and can use the original’s database, but
stores its own loads in the `.csrload` format.

## What the program can do

- Cartridge, projectile and propellant records from a database; replace
  any component of a load; add your own records.

- Pressure, energy and velocity curves with markers and pressure bands,
  a burn-rate chart and a load ladder.

- Drawings of the cartridge and the chamber with the C.I.P. dimensions.

- A Measurements tab for chronograph and pressure data, with statistics
  (mean, SD, ES) and a comparison with the simulation.

- Fit to measurements: adapts the model values of a powder to your own
  data.

- Reports with a template editor, print preview and PDF export.

- Metric, imperial and original units (also per field), light and dark
  themes, adjustable font size.

- Translations: English, German, French and Italian.

## How accurate is it?

The default *Physical Model* is fitted to published load data of powder
manufacturers. Against loads that were left out of the fit, the muzzle
velocity is typically within 3–6 % (root mean square; Vihtavuori powders
about 3 %, Hodgdon about 6 %) and the peak pressure within about 15 %
where the tables publish it. The scatter of the published tables
themselves (lots, barrels, components) is of that size. Single loads can
deviate much more. Unusual loads, other powders and other cartridges
than the ones tested can deviate more again; slow powders at low
pressure, pistol and revolver cartridges (about 8 % in velocity and 19 %
in pressure) and shotgun loads (about 3 % in velocity and 16 % in
pressure, with their own constants) are known weak spots. Powders
without physical burn values, such as your own records, are less
accurate than the project’s powders. The GRT Original model reproduces
the results of the original program to about 1 % in velocity and 3 % in
pressure. The files `README.md` and `docs/ACCURACY.md` of the project
contain the current accuracy tables.

> [!WARNING]
> Never use a calculated load without checking it against the
> manufacturer’s data. Work up from a safe starting charge and watch for
> signs of excessive pressure.
