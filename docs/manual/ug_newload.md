# Entering a load

The left half of a load tab has three input tabs, the right half shows
the results.

## Cartridge & gun

Shows the caliber (Select Caliber… replaces it) and a table of
properties: designation, overall length, case length, case volume, bore,
effective area, maximum permitted pressure, barrel length, twist length,
seating depth and more. Below the table the drawings show the cartridge,
the chamber and (when available) the powder.

### Replacing the caliber.

After you choose another caliber, the program can also set barrel
length, seating depth and charge to the defaults of the new caliber
(tick the corresponding option in the selection dialog).

## Projectile

Mass, diameter, length and the other bullet data. Select Projectile…
replaces the bullet; tick Show only bullets that fit the caliber to
narrow the list. The Initial Gas Pressure is the pressure at which the
bullet starts to move and engraves. The Bullet friction is an extra
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
