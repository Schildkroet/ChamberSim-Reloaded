# Project database

Calibers, projectiles and propellants that come with ChamberSim Reloaded. A caliber is one JSON file; the projectiles and the
propellants have one file per manufacturer with an array of records:

```
data/calibers/<name>.json
data/projectiles/<manufacturer>.json
data/propellants/<manufacturer>.json
```

The files are built into the program and appear in the database together with your own records (and the
records of the original program's database, if you have chosen one in *File ▸ Settings*). They are read-only in the
program: *Copy…* makes an editable own copy.

Where the records come from (C.I.P. sheets, manufacturers' product pages, Wikipedia infoboxes, fits to published load data) and how
good they are: see [docs/DATA_SOURCES.md](../docs/DATA_SOURCES.md).

## Adding a record

1. Create the record in the program (*Database* tab ▸ *New…*, or *Copy…* of a similar one) and check it with a load.
2. Select it and click *Export…*. This writes the file in the format below with a suitable name.
3. Put the file into the matching folder. A projectile or a propellant then goes into its manufacturer's file:
   `python3 scripts/data/recordfiles.py data/projectiles` (or `data/propellants`) merges it (a record with the same manufacturer,
   name and lot is replaced) and removes the single file. The program reads a single-record file as well.
4. Open a pull request (see [CONTRIBUTING.md](../docs/CONTRIBUTING.md)).

CI checks every file (`csr_datacheck`): valid JSON, known columns, numbers where numbers belong, a name, plausible
model values, and no two records with the same name.

## Format

One object with the columns of the database table (the same columns as the original program's database). Columns
that are left out are empty (text) or 0 (numbers). `id` and `geloescht` must not be in the file.

Units are those of the database columns, **not** the units shown in the program:

| Table | Column | Meaning | Unit |
|---|---|---|---|
| propellants | `mname`, `pname`, `lotid` | manufacturer, name, lot | |
| | `Ba` | burn rate coefficient | |
| | `Qex` | explosion heat | kJ/kg |
| | `k` | ratio of specific heats | |
| | `a0`, `a1`, `z1`, `z2` | form function, transition limits (0 < z1 < z2 < 1) | |
| | `pc`, `pcd` | material density, bulk density | kg/m³ |
| | `pt` | reference temperature | °C |
| | `tcc`, `tch` | temperature coefficients (cold, hot) | |
| projectiles | `mname`, `pname`, `lotid`, `caliber` | manufacturer, name, lot, caliber designation | |
| | `gdia`, `glen` | diameter, length | mm |
| | `gmass` | mass | **grain** |
| | `gtailtype` | base: 0 flat, 1 hollow, 2 boat tail | |
| | `gtaildiaA`, `gtaildiaB`, `gtailh` | base diameters, boat-tail / cavity length | mm |
| | `g1bc`, `g7bc` | ballistic coefficients | |
| calibers | `cipname`, `altname` | name, other names | |
| | `standard` | `CIP`, `SAAMI` or `WildCat` | |
| | `ciporigin` | country (ISO code, e.g. `DE`) | |
| | `ciptype` | `1 rimless`, `2 rimmed`, `3 belted`, `3 magnum`, `4 pistol/revolver`, `5 rimfire`, `7 shotshell`, `10 other` | |
| | `L1` … `L6`, `R`, `R1`, `P1`, `P2`, `H1`, `H2`, `G1`, … | CIP case dimensions | mm |
| | `V` | case capacity | **grain H₂O** |
| | `Pmax`, `PK`, `PE` | maximum pressures | bar |
| | `c_Z`, `c_F` | groove and bore diameter | mm |
| | `c_Q` | bore cross-section | mm² |
| | `c_u_` | twist length | mm |

## License

The records in this folder are licensed under [CC BY-SA 4.0](LICENSE.md), separately from the program's code.
