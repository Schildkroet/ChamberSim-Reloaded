# Bullet file for ChamberSim Reloaded

Bullet manufacturers can publish the data of their projectiles as one file that users add to the program with
**Database > Projectiles > Import bullets…**. The imported bullets become the user's own records (they can be copied, edited and
deleted); nothing is sent anywhere. The same records can also be contributed to the project's built-in database (see
CONTRIBUTING.md): put the `"bullets"` array into `data/projectiles/<manufacturer>.json`.

File name: `<manufacturer>.csrbullets` (UTF-8 JSON, a plain `.json` file works as well). A complete example is in
[examples/example.csrbullets](examples/example.csrbullets).

```json
{
  "format": "ChamberSimReloaded bullets",
  "version": 1,
  "manufacturer": "Acme Bullets",
  "source": "https://www.acme-bullets.example/data (bullet data sheet, 15-Jan-2026)",
  "date": "2026-01-15",
  "bullets": [
    {
      "pname": "Match HPBT .308 175 gr",
      "caliber": ".308",
      "gdia": 7.82,
      "glen": 31.2,
      "gmass": 175.0,
      "gtailtype": 2,
      "gtaildiaA": 7.82,
      "gtaildiaB": 6.9,
      "gtailh": 4.6,
      "g1bc": 0.52,
      "g7bc": 0.26,
      "gpressure": 250.0
    }
  ]
}
```

## Header

| Key | Meaning |
|---|---|
| `format` | always `"ChamberSimReloaded bullets"` |
| `version` | `1`; a program that only knows older versions refuses the file |
| `manufacturer` | used for every bullet without its own `mname` |
| `source` | where the data come from (web address, data sheet and its date); stored with every bullet (field "origin") |
| `date` | date of the data (yyyy-mm-dd) |
| `bullets` | the list of bullets |

## One bullet

All lengths in **millimetres**, the mass in **grains**, pressures in **bar**. Numbers are JSON numbers (a decimal point, no units in
the text). Keys that are left out stay empty (0 or empty text).

| Key | Required | Meaning |
|---|---|---|
| `pname` | yes | name of the bullet, with caliber and mass, e.g. `Match HPBT .308 175 gr` (manufacturer, name and `lotid` identify a bullet) |
| `mname` | no | manufacturer, only if it differs from the header |
| `lotid` | no | lot or version, to tell apart bullets with the same name |
| `caliber` | recommended | the bullet caliber in inch as the program's calibers use it: `.224`, `.243`, `.264`, `.277`, `.284`, `.308`, `.323`, `.338`, `.366`, `.375`, ... (the cartridge selection of a new load is filtered by it) |
| `gdia` | yes | bullet diameter [mm] (> 0) |
| `glen` | yes | overall length [mm] (> 0) |
| `gmass` | yes | mass [gr] (> 0) |
| `gtailtype` | recommended | base: `0` flat, `1` hollow base, `2` boat tail |
| `gtaildiaA` | for a boat tail | diameter where the taper starts [mm] (the bullet diameter, smaller for a rebated boat tail) |
| `gtaildiaB` | for a boat tail | base diameter [mm] |
| `gtailh` | for a boat tail | length of the taper (or the depth of the hollow base) [mm] |
| `gdepthmax` | no | maximum seating depth [mm] |
| `gpressure` | recommended | start pressure [bar], the gas pressure at which the bullet leaves the case and engraves: about 150 lead, 250 jacketed, 290 brass, 350 copper monolithic. 0 is not meant to be "none"; leave a typical value |
| `gfriction` | no | extra friction [bar] (negative for coated bullets) |
| `g1bc`, `g7bc` | no | ballistic coefficients (G1, G7) |
| `gUBCS` | no | UBCS code |
| `descr` | no | free text (construction, notes) |

Other columns of the projectile table are accepted but not needed (`origin`, `status`, ...); a key the program does not know makes
that bullet fail. A bullet with a problem is reported and skipped, the others are imported. `gValid` ("Validated") is ignored: it is
set by hand by a user who checked a bullet against a real one.

## Checks

The program checks every bullet (known keys, numbers where numbers belong, name present, diameter / length / mass above 0) and
shows the problems with the bullet's position in the file before anything is imported. Bullets that the database already has (same
manufacturer, name and lot) are left out.

Programs can write such a file with `csr::projectdata::toBulletFile` and read it with `readBulletFile`
(`src/core/projectdata.h`); `scripts/data/bulletdata/bul_to_json.py` converts QuickLOAD bullet files to the record format.
