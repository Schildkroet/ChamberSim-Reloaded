# Making, opening and saving loads

## New load

**File** → **New load… (Ctrl+N)** opens a dialog with two steps.

### Step 1: choose the components

The tabs 1. Caliber, 2. Projectile and 3. Propellant list the records of
the database. Select one record in each tab; below the lists the dialog
shows what you have chosen so far. The projectile list can be restricted
to bullets that fit the caliber (Show only bullets that fit the
caliber). Next \> becomes available when all three components are
chosen.

### Step 2: check the load data

The dialog shows the chosen components and the Load data: propellant
charge, barrel length, bullet seating depth, twist length and propellant
temperature. They are filled with defaults for your selection (barrel
length of the caliber, the cartridge at its maximum length, a charge of
70 % of the case capacity) and you can change them freely. The charge is
also shown in the other mass unit (grams or grains). Reset to defaults
restores the defaults.

\< Back returns to step 1. If you come back to step 2 with the *same*
three components, your changes are kept; if you change any component,
the load data are filled in again with the defaults of the new
selection.

Create load opens the new load in its own tab. It is called “Untitled”
until you save it.

## Opening loads

**File** → Open load… (Ctrl+O) opens one or several files at once; you
can also give files on the command line. Each load gets its own tab,
named after the file. The file dialog offers three filters: all loads,
`.csrload` only, and the original’s `.grtload` only.

- A load that is already open is not opened twice; its tab is shown
  instead.

- **File** → Open recent lists the last 12 loads. An entry whose file
  has disappeared or cannot be read is removed from the list.

- If a file cannot be read, a message names the file and the reason.

## Saving loads

A tab with unsaved changes shows an asterisk after its name.
**File** → Save (Ctrl+S) writes the load to its file; a new load
(“Untitled”) and Save as… (Ctrl+Shift+S) ask for a file name. If you
leave out the extension, `.csrload` is added. When you close a tab or
quit with unsaved changes the program asks whether to Save, Discard or
Cancel.

## The two file formats

| Extension | Description |
| :--- | :--- |
| `.csrload` | The program’s own format (XML). It contains everything: all input values, the caliber, projectile and propellant records with their values, the physical burn values of the Physical Model, the shotshell parts and your measurements and notes. |
| `.grtload` | The format of Gordon’s Reloading Tool. It can be opened and, as an export, written. |

The format is chosen by the file name’s extension.

### Opening an original file

A `.grtload` file is read completely, including the measurements, notes
and pictures of its appendix. Because the program saves only in its own
format, Save on such a load works like Save as…: it suggests the same
name with the extension `.csrload`. The original file is never changed.

### Exporting for the original program

In Save as… choose the filter *Original GRT loads, without the data of
ChamberSim Reloaded* (or type a name ending in `.grtload`). The exported
file can be opened in Gordon’s Reloading Tool. It leaves out what that
program does not know: the physical burn values and the shotshell part
choices. Keep a `.csrload` copy if you do not want to lose these.

## What a load file contains

A load file is self-contained. The records of the caliber, projectile
and propellant are stored in the file with their values, not as
references to the database. Consequently:

- the load opens and calculates the same on another computer, without
  the original database;

- deleting or changing a record in the database does not change loads
  that were made with it (the program tells you so when you delete a
  record);

- to apply changes of a record to an existing load, select it again in
  the load (Select Propellant… and so on); for a powder a banner points
  out that the load differs from the database, see
  section <a href="#sec:propellant" data-reference-type="ref"
  data-reference="sec:propellant">6.3</a>.

Measurement series, notes and pictures are part of the file as well.
Parts of an original file that the program does not interpret are kept
unchanged when you save.

## Undo and redo

Every load remembers its last 100 edits. **Edit** → Undo (Ctrl+Z) takes
back the last edit and **Edit** → Redo (Ctrl+Shift+Z or Ctrl+Y) restores
it again. The menu entries name the edit, for example Undo Select
Propellant. The following edits are recorded:

- changing a value in the property tables;

- choosing another caliber, projectile or propellant;

- applying the result of Fit to measurements and accepting the physical
  burn values of a powder;

- changes in the Measurements tab.

The history belongs to one load and is not saved in the file. It starts
empty when a load is opened and is kept while the load is saved. If you
undo back to the state that was saved, the load no longer counts as
modified. A new edit after an undo discards the steps that could have
been redone. Switching the selected ladder curve is not an edit that can
be undone.

## Sessions

The program remembers the loads that were open when you quit (only those
that have a file). With **File** → Settings → Start-up →  Open the loads
of the last session at start they are opened again at the next start,
and the tab that was shown is selected again. Files that no longer exist
are skipped.

## Tips

- Save a load under a new name before you experiment with it; Save as…
  leaves the old file as it is.

- Put the files of one rifle and component set into one folder and use
  the series of the Measurements tab for the range sessions, so that a
  single file holds the history of a load.

- Files are plain XML text and suit version control or backup tools.
