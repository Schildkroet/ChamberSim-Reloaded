# Add a new powder from your own measurements

You can make a propellant model for your own use directly in the program. The model values (Ba, a0, z1, z2) are
fitted to your chronograph data. The result is saved as an own record, so you can use the powder in every load.
(To contribute a powder to the project database, see [Procedure 2](CONTRIBUTING.md#procedure-2-make-a-propellant-model-from-measurements).)

CAUTION: Reloading is dangerous. Use only loads from the current load data of the manufacturer, and start low.
The result is only as good as the measurements and the data of the powder.

What you need: a chronograph (for example MagnetoSpeed or Labradar) and, if you can, a pressure measurement. The more
different loads you measure (2 or more calibers, 2 or more bullet weights, 2 or more charges each, at least 5 shots
per charge), the better the model fits other loads. One load with few charges only determines Ba.

1. **Start from a similar powder.** Open the tab *Database*, select the *Propellants* list, select a powder with
   a similar burn rate and click *Copy…*. (Or click *New…* if you know all values.) Type the name, the
   manufacturer and the lot. Check the values that you know: explosion heat `Qex`, ratio of specific heats `k`,
   material density `pc`, bulk density `pcd` and the reference temperature `pt`. These do not come from the fit.
   Optionally add an overview photo and a detail photo (*Choose file…*, with the scale in px/mm if you know it).
   Click *Save*.
2. **Make a load with the new powder.** Create a load with *File ▸ New load…* (or open a `.grtreload` or `.grtload` file). On the
   tab *Propellant* click *Select Propellant…* and choose your new powder.
3. **Measure the case and the cartridge.** Measure the volume of a fired case with water and type it into *Case
   Volume*. Measure the length of the bullets and the cartridges and type the mean values into *Projectile Overall
   Length* and *Cartridge Overall Length*. A good case volume makes the fit much better.
4. **Type the measurements.** Open the tab *Measurements*. Click *Add charge* for each charge (or *Add ladder
   charges*). Select a charge and type the velocity of each shot in the shot list (and the pressure, if you
   measured it). You can also use *Paste shots* (one shot per line, velocity and optionally pressure), or click
   *Import chronograph file…* and choose a MagnetoSpeed `.csv` or a Labradar `Report.csv`. The files do not contain
   the powder charge: type it for each series in the import dialog. The table shows the mean velocity, SD and ES
   and the difference to the simulation.
   Use *New series* if you have several measurement series for the load (for example one for each range day).
5. **Fit the model.** Click *Fit to measurements…*. Check which values to fit (the default depends on the data: Ba alone for fewer than 3 measured
   charges, Ba and a0 for 3 to 5, all four values for 6 or more charges from at least 2 load files) and click *Fit*. The dialog shows the velocity and
   pressure of each charge before and after the fit. Read the warnings, for example when the case volume is only the
   database value or when there are fewer charges than values. With the physical model (the default) and a project powder, the fit
   changes the load's physical burn values (*Physical Model: ...* in the propellant table) and keeps them in the load; *Save as own
   propellant…* is then not offered.
6. **Check the result.** The differences after the fit should be small (less than approximately 2 % in velocity).
   If they are not, check the measurements, the case volume and the bullet and powder data.
7. **Save the model.** Click *Save as own propellant…* in the dialog. The program makes a new record
   "<name> (fitted)" in your own database and keeps the photos of the powder. Select it in other loads with
   *Select Propellant…*. You can also click *Apply to this load* to put the fitted values into this load only.
8. Fit again later with more loads: the fit uses all measurement series of the load, and the command-line tool
   `grt_fitpowder` (see Procedure 2) fits one model to several load files.

The fit is made for one load at a time. A powder that is fitted to one caliber and one bullet only can give wrong
results in other cartridges. Check every new load with a measurement before you trust the model.
