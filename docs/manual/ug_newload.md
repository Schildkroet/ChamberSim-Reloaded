# Entering a load

The left half of a load tab has four input tabs, the right half shows
the results.

## Cartridge & gun

Shows the caliber (Select Caliber… replaces it) and a table of
properties: designation, overall length, case length, case volume, bore,
effective area, maximum permitted pressure, barrel length, twist length,
seating depth and more. Below the table the drawings show the cartridge,
the chamber and (when available) the powder.

The Sebert factor (0 to 1) is kept as the file has it, 0 included. The
Original and Tuned models and the recoil figures use it as it is; the
Physical model keeps its calibrated share of 1/4 of the charge for the
moving gas, whatever the file holds. A value outside 0 to 1 becomes 0.5
when the load is opened; the panel accepts only 0 to 1. The report shows
the value the simulation used.

### Primer and start pressure.

The Primer (a drop-down: small or large, pistol or rifle, standard or
magnum, .50 BMG) starts with the usual primer of the caliber. The start
pressure, the gas pressure at which the bullet starts to move, is 250 bar
plus the Additional Start Pressure for the bullet, the crimp and the case,
as in the original program; a new load takes it from the bullet record
(its start pressure minus 250 bar). Only the physical model uses the
primer: it adds the difference between the primer type's start pressure
and 250 bar (fitted to Hodgdon's published loads: Large Rifle 250 bar,
the others 230 to 450 bar). For pistol and revolver cartridges the
physical model starts at no less than 0.3 times the maximum permitted
pressure, so there the primer usually has no effect. Loads of earlier
versions and of the original keep their start pressure exactly.

### Replacing the caliber.

After you choose another caliber, the program can also set barrel
length, seating depth and charge to the defaults of the new caliber
(tick the corresponding option in the selection dialog). The primer
becomes the usual primer of the new caliber; the additional start
pressure stays.

## Projectile

Mass, diameter, length and the other bullet data. Select Projectile…
replaces the bullet; tick Show only bullets that fit the caliber to
narrow the list. The Initial Gas Pressure is the pressure at which the
bullet starts to move and engraves. It is the primer’s start pressure plus the
Additional Start Pressure of the Cartridge & gun tab; editing it changes
the additional one. The Bullet friction is an extra
resistance in bar that acts all along the barrel; it is always applied.
Negative values are possible and describe low-friction bullets (the
original’s database has values down to $-100$ bar). For 362 rifle
bullets the project records carry start pressure and friction values
fitted to published loads. A load keeps the values it was made with
until you select the bullet again.

## Propellant

Powder charge, explosion heat, burn rate and the form-function values.
Select Propellant… replaces the powder.

With the Physical Model, project powders also have *physical burn
values*. They are kept in the load file. If they are missing, an orange
banner tells you so and offers Use the physical values….

A load keeps its own copy of the powder values. If a database record of
that powder was corrected after the load was made, or you edited the
values in the load, a second orange banner says *Powder values differ
from the database*. The load is still calculated with its own values.
Use the database values… replaces them with those of the closest record
(the charge and all other settings stay; Undo takes it back). A powder
can have several records, so the banner appears only if no record has
exactly the values of the load. With the Physical Model only the
project’s records are compared, because the original database is not
updated; with the other models all records count.

Loads saved by the original program name a powder slightly differently
(`RS 40` instead of `RS40`); spaces and capitals are ignored when a load
is matched with the records, so the project’s record and its physical
burn values are found for such a load too. The two banners are
independent: *Use the database values…* replaces the powder values of
the load (heat of explosion, gas properties, densities and the burn
values), *Use the physical values…* only adds the physical burn values
and keeps the load’s own values. To get the same result as selecting the
powder again from the database, use the database values first.

## Charge

The charge and the load development ladder in large input fields, so
that they are easy to reach while you try charges. A switch in the
corner of the charge card changes the unit of the charge and of the
charge step between grains and grams (the same per-field units as in the
propellant table; the value in the other unit is shown below the field).

Propellant charge  
the charge; with a ladder, the highest charge.

Steps  
the number of ladder steps, 1 to 10; 1 means no ladder.

Charge step  
the powder taken away from one step to the next (only with two or more
steps).

A line below the fields shows the range the ladder covers. The table
Ladder results lists the charge, maximum pressure and velocity of every
step; a step above the permitted pressure is marked red. Click a row to
show its curve in the chart. The values are the same fields as in the
propellant table: a change in one place shows in the other, and Undo
takes it back.

## Editing values

Click a value in a property table to edit it. Values with a unit have a
drop-down menu for the unit; the choice applies to this field only.
Reset per-field units in the settings returns every field to the chosen
unit system. Some fields are coded values (for example the Shotshell
flag) and offer a list of choices.

## Measuring a load

A good Case Volume is the single most important input after the powder.
Measure a fired case by filling it with water and weighing it. Also
measure the real bullet length and cartridge overall length and enter
the mean values.

## Shotgun loads

Calibers with Shotshell set to yes use their own model terms. Enter such
a load as follows:

- Projectile: mass = shot + wad (about 2.5 g for a plastic wad), length
  = height of the shot column plus the wad, diameter = bore diameter;

- seated completely inside the hull: cartridge overall length = hull
  length, seating depth = projectile length;

- Initial Gas Pressure = crimp release, about 20 bar.

The load also has the choices Shotshell Hull, Shotshell Wad and
Shotshell Primer, each with three groups of similar parts (low, medium
and high pressure). They change the pressure by up to about
$\pm 20\,\%$. Shotgun results are less reliable than rifle and handgun
results.
