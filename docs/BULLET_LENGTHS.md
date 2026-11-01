# Bullet lengths – recalculation of the estimated values

`scripts/data/bulletdata/recalculate_lengths.py` recalculated the length of the **1343** projectile records
whose length was only estimated (of 2747 in the project database; the other 1403 have a length
published by the maker, measured on a product photo or taken from the maker's bullet data). The table at the end
lists every record.

1 record(s) were left alone because their mass cannot belong to a bullet of that diameter:

| Maker | Projectile | Diameter (mm) | Mass (gr) | Old length (mm) |
|---|---|---|---|---|
| PPU | SJHP B-580 | 10.88 | 24 | 1.8 |

## Method

The earlier estimate (`estimate_length.py`) divides the mass by an effective density that depends only on a coarse
class and the caliber. The new model describes the bullet's construction more closely:

```
length = exp(b0 + b1·ln(diameter) + b2·ln(mass) + b3·ln(G1 form factor) + Σ shape terms)
```

| Term | What it captures |
|---|---|
| diameter, mass | the swept volume; a longer bullet has a lower effective density |
| material | monolithic copper/brass (8.9 g/cm³) is lighter than a lead core (11.3 g/cm³); jacketed lead in between |
| nose | wadcutter, semi-wadcutter, flat/truncated, round nose, hollow point, polymer tip: how much of the nose cylinder is empty |
| base | boat tail (removes volume), flat base, hollow base |
| relief grooves | Barnes TSX/TTSX/LRX, Hasler, Sax KJG and similar bullets are longer for their mass |
| partition | dual-core bullets such as the Nosler Partition |
| G1 form factor i1 = SD/BC | the published ballistic coefficient measures the ogive: a low i1 means a long, slender nose. Used for the 305 records that have a BC; a G7 BC is converted to i1 where no G1 BC is present, and match bullets without any BC get the typical form factor of match bullets of the same construction |

The coefficients were fitted to the 1403 records with a given length (a ridge regression of ln(length),
robust against the few wrong photo measurements). The accuracy below is a grouped 5-fold cross validation: a fold holds
out whole product lines (maker and first word of the bullet name), so no bullet is predicted from a sibling of its own
line, and the figures apply to bullets the model has not seen.

## Accuracy

Median and 90th percentile of the absolute length error, and the 10th/90th percentile of the signed error (the
range that the model is expected to stay in):

| | n | median error | 90 % of the errors below | 10 % … 90 % range |
|---|---|---|---|---|
| previous estimate | 1403 | 6.7 % | 20.5 % | -20.0 … +10.2 % |
| new model | 1403 | 4.7 % | 13.4 % | -10.0 … +9.3 % |

The previous estimate is a fixed formula, so its figures are not cross-validated; they are its errors on the same
records, some of which were used for its own calibration, so they flatter it. Split by construction and by whether a
ballistic coefficient is available:

| Construction | BC | n | median error | 10 % … 90 % range |
|---|---|---|---|---|
| copper | BC | 87 | 4.3 % | -10.3 … +7.9 % |
| copper | no BC | 176 | 5.3 % | -11.2 … +12.3 % |
| lead | BC | 16 | 5.8 % | -19.8 … +3.5 % |
| lead | no BC | 48 | 6.1 % | -8.1 … +10.6 % |
| fmj | BC | 29 | 4.4 % | -8.0 … +10.4 % |
| fmj | no BC | 61 | 8.5 % | -15.0 … +10.3 % |
| jacket | BC | 581 | 4.2 % | -7.8 … +8.0 % |
| jacket | no BC | 405 | 5.4 % | -11.3 … +10.1 % |

The main residual uncertainty is the ogive of bullets whose maker publishes no ballistic coefficient (Sierra, PPU,
Sellier & Bellot, RWS, Federal, Winchester, Remington, Barnes): two bullets of the same mass and construction can differ
by several millimetres in nose length. For those the shape class is all the model can use.

## Result

- 679 records change by at most 5 %, 268 by more than 10 %, 70 by more than 20 %.
- median change +2.3 %, largest increase +54.1 %, largest decrease -23.4 %.
- average length 27.1 mm before, 27.8 mm after.

### By maker

| Maker | Records | median change | largest change |
|---|---|---|---|
| ACP | 4 | -10.5 % | -11.9 % |
| Alsa | 4 | -6.1 % | -7.1 % |
| BA | 1 | -1.0 % | -1.0 % |
| Balleurope | 3 | -6.9 % | -8.5 % |
| Ballistix Academy | 105 | -1.2 % | +22.1 % |
| Barnes | 128 | +3.7 % | +32.7 % |
| Berger | 2 | +2.9 % | +4.8 % |
| Bloodsport | 1 | -7.9 % | -7.9 % |
| Brenneke | 11 | -0.6 % | +13.3 % |
| Buffalo Arms | 1 | +3.5 % | +3.5 % |
| Cam Pro | 3 | +8.4 % | +9.4 % |
| CloverBullets | 9 | +14.9 % | +20.0 % |
| CopperBear | 3 | -3.8 % | -9.6 % |
| Cutting Edge | 8 | +38.4 % | +54.1 % |
| DK Bullets | 8 | +11.0 % | +18.1 % |
| Degol | 93 | +0.9 % | +24.1 % |
| Federal | 30 | +4.9 % | +37.2 % |
| Fiocchi | 3 | +10.5 % | +11.7 % |
| Fox | 18 | +0.2 % | +12.0 % |
| Frontier | 4 | +4.2 % | +10.1 % |
| GECO | 16 | -6.5 % | -11.0 % |
| GM | 17 | +24.5 % | +33.0 % |
| GPA | 10 | -5.6 % | -10.2 % |
| H&N | 1 | -2.3 % | -2.3 % |
| Hornady | 195 | +6.3 % | +30.7 % |
| LOS | 12 | +16.3 % | +53.2 % |
| Lapua | 34 | +0.9 % | +29.5 % |
| Leader | 5 | -8.1 % | -11.1 % |
| Magtech | 4 | +2.7 % | -16.7 % |
| Missouri | 1 | +7.4 % | +7.4 % |
| Norma | 59 | -0.2 % | +25.4 % |
| North Fork | 2 | +0.4 % | +0.9 % |
| Nosler | 8 | +6.5 % | +11.0 % |
| PPU | 94 | +4.0 % | -15.2 % |
| RWS | 54 | -0.0 % | +13.4 % |
| Remington | 11 | -2.8 % | -6.6 % |
| Sako | 53 | -0.1 % | +27.7 % |
| Sax | 22 | -3.0 % | -23.4 % |
| Sellier & Bellot | 105 | +2.7 % | +12.2 % |
| Shooting Technology | 5 | -4.7 % | -8.0 % |
| Sierra | 129 | +2.4 % | +16.0 % |
| Speer | 14 | +1.3 % | +6.8 % |
| Swift | 2 | +7.0 % | +12.2 % |
| Ve-Loads | 2 | +5.2 % | +13.0 % |
| Wi-La-Tech | 14 | +6.3 % | +12.8 % |
| Winchester | 34 | -0.4 % | +11.1 % |
| Woodleigh | 1 | -5.6 % | -5.6 % |

### Changes larger than 20 %

These records are worth a look; a large change means that the old estimate and the construction disagree strongly
(usually a long ogive or a short wadcutter).

| Maker | Projectile | Old (mm) | New (mm) | Change | Construction |
|---|---|---|---|---|---|
| Cutting Edge | Single Feed LZR (Lazer Tipped Hollow Point) 0.416 550 gr | 36.8 | 56.7 | +54.1 % | monolithic copper/brass, polymer tip |
| LOS | RNFP verk. Blei 0.401 180 gr | 11.8 | 18.1 | +53.2 % | lead |
| LOS | RNFP verk. Blei 0.358 180 gr | 14.8 | 22.6 | +52.5 % | lead |
| LOS | HP verk. Blei 0.358 180 gr | 14.8 | 21.5 | +45.1 % | lead, hollow point |
| Cutting Edge | Safari Raptor Hohlspitz 0.375 275 gr | 23.5 | 33.7 | +43.5 % | monolithic copper/brass, hollow point |
| Cutting Edge | MTAC (Match/Tactical) 0.277 125 gr | 23.7 | 33.1 | +39.6 % | monolithic copper/brass, match |
| Cutting Edge | MTAC (Match/Tactical) 0.308 132 gr | 20.4 | 28.4 | +39.2 % | monolithic copper/brass, match |
| Cutting Edge | MTAC (Match/Tactical) 0.308 155 gr | 24.0 | 33.0 | +37.5 % | monolithic copper/brass, match |
| Federal | Trophy Bonded Sledgehammer Solid PB474TBSH500 | 31.7 | 43.5 | +37.2 % | monolithic copper/brass, polymer tip, boat tail |
| Federal | Trophy Bonded Sledgehammer Solid PB458TBSH500 | 34.4 | 46.5 | +35.2 % | monolithic copper/brass, polymer tip, boat tail |
| Cutting Edge | Flat Base Raptor Hohlspitz 0.308 130 gr | 17.6 | 23.8 | +35.1 % | monolithic copper/brass, hollow point, flat base |
| GM | Splinter Tip Messingjagd 0.406 188 gr | 17.2 | 22.9 | +33.0 % | monolithic copper/brass, polymer tip |
| Barnes | SOLID 30713 | 29.0 | 38.5 | +32.7 % | monolithic copper/brass |
| Cutting Edge | MTH (Match / Tactical / Hunting) 0.338 265 gr | 34.4 | 45.5 | +32.2 % | monolithic copper/brass, match |
| Federal | Trophy Bonded Sledgehammer Solid PB416TBSH400 | 34.5 | 45.4 | +31.7 % | monolithic copper/brass, polymer tip, boat tail |
| LOS | CEB Bleifrei 0.308 150 gr | 22.5 | 29.4 | +30.8 % | monolithic copper/brass |
| Hornady | NTX 22000 | 10.4 | 13.6 | +30.7 % | monolithic copper/brass, polymer tip, boat tail |
| GM | Splinter Crown Messingjagd 0.375 185 gr | 20.3 | 26.3 | +29.6 % | monolithic copper/brass, polymer tip |
| Lapua | Naturalis N566 | 15.2 | 19.7 | +29.5 % | monolithic copper/brass, boat tail |
| GM | Splinter Crown Messingjagd 0.366 154 gr | 17.9 | 23.2 | +29.5 % | monolithic copper/brass, polymer tip |
| GM | Splinter Tip Messingjagd 0.366 154 gr | 17.9 | 23.2 | +29.5 % | monolithic copper/brass, polymer tip |
| Federal | Trophy Bonded Sledgehammer Solid PB375TBSH300 | 32.9 | 42.5 | +29.2 % | monolithic copper/brass, polymer tip, boat tail |
| Hornady | ECX 33272 | 25.8 | 33.3 | +29.1 % | monolithic copper/brass, polymer tip, boat tail |
| Barnes | SOLID 30694 | 28.5 | 36.4 | +27.7 % | monolithic copper/brass |
| GM | Splinter Tip Messingjagd 0.338 131 gr | 18.2 | 23.2 | +27.7 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade Pro 0.366 201 gr | 23.3 | 29.8 | +27.7 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade Pro 0.309 162 gr | 27.7 | 35.2 | +26.9 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade 0.375 270 gr | 29.6 | 37.5 | +26.7 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade 0.366 230 gr | 26.7 | 33.8 | +26.5 % | monolithic copper/brass, polymer tip |
| GM | Splinter Crown Messingjagd 0.323 127 gr | 19.6 | 24.7 | +26.1 % | monolithic copper/brass, polymer tip |
| GM | Splinter Tip Messingjagd 0.323 127 gr | 19.6 | 24.7 | +26.1 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade Solid 0.376 261 gr | 28.5 | 35.8 | +25.6 % | monolithic copper/brass, polymer tip |
| Norma | GTX 10657401 | 24.2 | 30.4 | +25.4 % | monolithic copper/brass, polymer tip, boat tail |
| Sako | Powerhead Blade Pro 0.309 174 gr | 29.7 | 37.1 | +25.1 % | monolithic copper/brass, polymer tip |
| Lapua | Lead-free copper N509 | 23.2 | 28.9 | +24.6 % | monolithic copper/brass |
| GM | Splinter Crown Messingjagd 0.308 121 gr | 20.8 | 25.9 | +24.5 % | monolithic copper/brass, polymer tip |
| GM | Splinter Tip Messingjagd 0.308 121 gr | 20.8 | 25.9 | +24.5 % | monolithic copper/brass, polymer tip |
| Barnes | SOLID 30649 | 31.7 | 39.4 | +24.1 % | monolithic copper/brass |
| Degol | Starkmantel Verbundkern Rundkopf 0.700 1000 gr | 22.6 | 28.1 | +24.1 % | jacketed lead core, round nose, flat base |
| Sako | Powerhead Blade 0.309 162 gr | 27.7 | 34.4 | +24.1 % | monolithic copper/brass, polymer tip |
| GM | Murmel Messingjagd 0.406 190 gr | 17.3 | 21.5 | +24.1 % | monolithic copper/brass |
| Hornady | ECX 35614 | 29.0 | 36.0 | +24.1 % | monolithic copper/brass, polymer tip, boat tail |
| Hornady | NTX 17016 | 9.7 | 12.0 | +23.6 % | monolithic copper/brass, polymer tip, boat tail |
| Sax | KSG LM-105 0.338 293 gr | 53.0 | 40.6 | -23.4 % | jacketed lead core |
| Sako | Powerhead Blade 0.323 180 gr | 27.8 | 34.3 | +23.3 % | monolithic copper/brass, polymer tip |
| Lapua | Naturalis N560 | 28.4 | 35.0 | +23.1 % | monolithic copper/brass, boat tail |
| Sako | Powerhead Blade Pro 0.285 140 gr | 28.7 | 35.3 | +23.1 % | monolithic copper/brass, polymer tip |
| Lapua | Naturalis N558 | 27.2 | 33.4 | +22.7 % | monolithic copper/brass, boat tail |
| GM | Splinter Crown Messingjagd 0.284 104 gr | 21.5 | 26.3 | +22.4 % | monolithic copper/brass, polymer tip |
| GM | Splinter Tip Messingjagd 0.284 104 gr | 21.5 | 26.3 | +22.4 % | monolithic copper/brass, polymer tip |
| Sako | Powerhead Blade Pro 0.278 120 gr | 26.0 | 31.7 | +22.1 % | monolithic copper/brass, polymer tip |
| Ballistix Academy | Teilmantel Open Tip 0.702 900 gr | 21.9 | 26.7 | +22.1 % | jacketed lead core, hollow point |
| Sako | Powerhead Blade 0.308 168 gr | 28.9 | 35.2 | +21.9 % | monolithic copper/brass, polymer tip |
| Ballistix Academy | Teilmantel Open Tip 0.700 900 gr | 22.1 | 26.9 | +21.6 % | jacketed lead core, hollow point |
| GM | Splinter Messingjagd 0.277 102 gr | 22.3 | 27.1 | +21.3 % | monolithic copper/brass, polymer tip |
| Degol | Teilmantel Spitz 0.510 700 gr | 34.1 | 41.4 | +21.3 % | jacketed lead core |
| Hornady | FTX 0.452 225 gr | 16.0 | 19.4 | +21.2 % | jacketed lead core, polymer tip |
| Federal | Trophy Bonded Bear Claw PB474TBBC500 | 31.7 | 38.4 | +21.1 % | jacketed lead core, polymer tip, boat tail |
| Ballistix Academy | Teilmantel Open Tip 0.700 1000 gr | 24.5 | 29.7 | +21.1 % | jacketed lead core, hollow point |
| Norma | GTX 10665681 | 36.5 | 44.2 | +21.1 % | monolithic copper/brass, polymer tip, boat tail |
| GM | Splinter Messingjagd 0.222 37 gr | 13.3 | 16.1 | +20.9 % | monolithic copper/brass, polymer tip |
| Ballistix Academy | Teilmantel Open Tip 0.702 1000 gr | 24.4 | 29.5 | +20.9 % | jacketed lead core, hollow point |
| Norma | GTX 10665091 | 33.2 | 40.1 | +20.8 % | monolithic copper/brass, polymer tip, boat tail |
| Sako | Powerhead Blade Pro 0.265 123 gr | 29.7 | 35.8 | +20.7 % | monolithic copper/brass, polymer tip |
| Hornady | ECX 26112 | 29.3 | 35.3 | +20.4 % | monolithic copper/brass, polymer tip, boat tail |
| Degol | Teilmantel Hohlspitz Boattail 0.458 600 gr | 54.0 | 43.1 | -20.2 % | jacketed lead core, hollow point, boat tail |
| GM | Splinter Messingjagd 0.264 91 gr | 22.2 | 26.7 | +20.2 % | monolithic copper/brass, polymer tip |
| Barnes | SOLID 30536 | 33.3 | 40.0 | +20.1 % | monolithic copper/brass |
| Degol | Starkmantel Halbspitz PP 0.458 400 gr | 25.1 | 30.2 | +20.1 % | jacketed lead core |
| CloverBullets | Messing Jagd 0.366 160 gr | 18.6 | 22.3 | +20.0 % | monolithic copper/brass |

## All recalculated bullets

`BC` marks the records whose ballistic coefficient took part in the estimate, `range` is the 10–90 % error range
of the model for that construction (see *Accuracy*).

| Maker | Projectile | Dia (mm) | Mass (gr) | Old (mm) | New (mm) | Change | Construction | BC | range |
|---|---|---|---|---|---|---|---|---|---|
| ACP | Copper-CNC Sidewinder Kupfer Hohlspitz 0.257 100 gr | 6.53 | 100 | 29.1 | 27.2 | -6.6 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| ACP | Sidewinder bleifreie Hohlspitz 0.375 285 gr | 9.52 | 285 | 39.0 | 34.9 | -10.6 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| ACP | Sidewinder bleifreie Hohlspitz 0.416 300 gr | 10.57 | 300 | 33.3 | 29.9 | -10.4 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| ACP | Sidewinder bleifreie Hohlspitz 0.416 400 gr | 10.57 | 400 | 44.4 | 39.1 | -11.9 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Alsa | Pro Vollmantel Rundkopf 0.355 115 gr | 9.02 | 115 | 14.0 | 13.3 | -5.1 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Alsa | Pro Vollmantel Rundkopf 0.355 124 gr | 9.02 | 124 | 15.1 | 14.3 | -5.6 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Alsa | Pro Vollmantel Rundkopf 0.355 140 gr | 9.02 | 140 | 17.1 | 16.0 | -6.6 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Alsa | Pro Vollmantel Rundkopf 0.355 147 gr | 9.02 | 147 | 18.0 | 16.7 | -7.1 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| BA | Blei Rundkopf (wie aus der Kokille) 0.300 80 gr | 7.62 | 80 | 13.5 | 13.4 | -1.0 % | lead, round nose, flat base |  | -8…+11 % |
| Balleurope | Blei Semi Rundkopf 0.309 150 gr | 7.85 | 150 | 23.9 | 22.8 | -4.8 % | lead, round nose, flat base |  | -8…+11 % |
| Balleurope | Blei Semi Rundkopf 0.459 300 gr | 11.66 | 300 | 21.7 | 20.2 | -6.9 % | lead, round nose, flat base |  | -8…+11 % |
| Balleurope | Blei Semi Rundkopf 0.459 400 gr | 11.66 | 400 | 28.9 | 26.5 | -8.5 % | lead, round nose, flat base |  | -8…+11 % |
| Ballistix Academy | SWC Baseguard Rundkopf Blei 0.441 260 gr | 11.20 | 260 | 20.1 | 20.6 | +2.3 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| Ballistix Academy | SWC Baseguard Rundkopf Blei 0.441 370 gr | 11.20 | 370 | 28.7 | 28.6 | -0.3 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| Ballistix Academy | SWC Baseguard Rundkopf Blei 0.446 260 gr | 11.33 | 260 | 19.7 | 20.1 | +2.0 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| Ballistix Academy | SWC Baseguard Rundkopf Blei 0.446 370 gr | 11.33 | 370 | 29.4 | 28.0 | -4.8 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| Ballistix Academy | Starkmantel 1,25 Open Tip 0.323 150 gr | 8.20 | 150 | 22.9 | 22.5 | -1.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.220 53 gr | 5.59 | 53 | 19.2 | 17.9 | -6.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.222 58 gr | 5.64 | 58 | 20.6 | 19.1 | -7.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.224 58 gr | 5.69 | 58 | 20.2 | 18.8 | -6.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.225 58 gr | 5.71 | 58 | 20.0 | 18.7 | -6.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.226 58 gr | 5.74 | 58 | 19.8 | 18.5 | -6.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.262 159 gr | 6.65 | 159 | 39.0 | 35.8 | -8.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.264 140 gr | 6.71 | 140 | 33.8 | 31.2 | -7.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.264 160 gr | 6.71 | 160 | 38.6 | 35.4 | -8.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.266 160 gr | 6.76 | 160 | 38.0 | 34.9 | -8.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.268 160 gr | 6.81 | 160 | 37.3 | 34.4 | -7.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 110 gr | 7.82 | 110 | 18.7 | 18.5 | -1.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 120 gr | 7.82 | 120 | 20.4 | 20.1 | -1.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 180 gr | 7.82 | 180 | 30.7 | 29.3 | -4.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 190 gr | 7.82 | 190 | 32.4 | 30.9 | -4.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 200 gr | 7.82 | 200 | 34.1 | 32.4 | -5.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 220 gr | 7.82 | 220 | 37.5 | 35.4 | -5.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 240 gr | 7.82 | 240 | 40.9 | 38.4 | -6.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.308 90 gr | 7.82 | 90 | 15.3 | 15.3 | +0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.309 120 gr | 7.85 | 120 | 20.3 | 19.9 | -2.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.309 170 gr | 7.85 | 170 | 28.8 | 27.6 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.309 180 gr | 7.85 | 180 | 30.4 | 29.1 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.309 200 gr | 7.85 | 200 | 33.8 | 32.1 | -4.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 110 gr | 7.92 | 110 | 18.2 | 18.0 | -0.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 130 gr | 7.92 | 130 | 21.5 | 21.1 | -1.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 174 gr | 7.92 | 174 | 28.8 | 27.7 | -3.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 190 gr | 7.92 | 190 | 31.4 | 30.1 | -4.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 200 gr | 7.92 | 200 | 33.1 | 31.6 | -4.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.312 90 gr | 7.92 | 90 | 14.9 | 14.9 | +0.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.318 200 gr | 8.08 | 200 | 31.7 | 30.4 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.318 225 gr | 8.08 | 225 | 35.7 | 33.9 | -5.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.318 240 gr | 8.08 | 240 | 38.0 | 36.0 | -5.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.321 200 gr | 8.15 | 200 | 31.0 | 29.9 | -3.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.321 220 gr | 8.15 | 220 | 34.1 | 32.7 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.321 244 gr | 8.15 | 244 | 37.8 | 36.0 | -4.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.321 260 gr | 8.15 | 260 | 40.3 | 38.2 | -5.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.323 120 gr | 8.20 | 120 | 18.4 | 18.3 | -0.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.323 150 gr | 8.20 | 150 | 22.9 | 22.5 | -1.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.323 200 gr | 8.20 | 200 | 30.6 | 29.5 | -3.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.323 227 gr | 8.20 | 227 | 34.7 | 33.2 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.327 160 gr | 8.31 | 160 | 23.8 | 23.3 | -1.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.327 200 gr | 8.31 | 200 | 29.7 | 28.8 | -3.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.327 227 gr | 8.31 | 227 | 33.8 | 32.4 | -4.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.329 160 gr | 8.36 | 160 | 23.5 | 23.1 | -1.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.329 200 gr | 8.36 | 200 | 29.3 | 28.4 | -2.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.329 227 gr | 8.36 | 227 | 33.3 | 32.0 | -3.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.331 160 gr | 8.41 | 160 | 23.1 | 22.8 | -1.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.331 227 gr | 8.41 | 227 | 32.8 | 31.7 | -3.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.338 240 gr | 8.59 | 240 | 33.1 | 32.0 | -3.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.340 240 gr | 8.64 | 240 | 32.6 | 31.6 | -2.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.342 240 gr | 8.69 | 240 | 32.2 | 31.3 | -2.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.350 120 gr | 8.89 | 120 | 15.3 | 15.6 | +2.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.350 190 gr | 8.89 | 190 | 24.2 | 24.0 | -0.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.350 80 gr | 8.89 | 80 | 10.2 | 10.7 | +4.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.352 120 gr | 8.94 | 120 | 15.1 | 15.5 | +2.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.352 190 gr | 8.94 | 190 | 23.9 | 23.8 | -0.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.352 80 gr | 8.94 | 80 | 10.0 | 10.6 | +5.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.355 120 gr | 9.02 | 120 | 14.8 | 15.2 | +2.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.355 190 gr | 9.02 | 190 | 23.4 | 23.4 | -0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.355 80 gr | 9.02 | 80 | 9.9 | 10.4 | +4.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.356 120 gr | 9.04 | 120 | 14.7 | 15.1 | +2.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.356 190 gr | 9.04 | 190 | 23.3 | 23.3 | -0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.356 80 gr | 9.04 | 80 | 9.8 | 10.3 | +5.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.357 120 gr | 9.07 | 120 | 14.6 | 15.0 | +2.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.357 190 gr | 9.07 | 190 | 23.1 | 23.1 | +0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.357 80 gr | 9.07 | 80 | 9.7 | 10.3 | +5.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.358 300 gr | 9.09 | 300 | 36.2 | 35.3 | -2.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.360 300 gr | 9.14 | 300 | 35.8 | 35.0 | -2.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.362 300 gr | 9.19 | 300 | 35.3 | 34.6 | -2.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.364 300 gr | 9.25 | 300 | 34.9 | 34.2 | -2.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.366 300 gr | 9.30 | 300 | 34.4 | 33.8 | -1.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.402 280 gr | 10.21 | 280 | 25.9 | 26.4 | +2.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.404 280 gr | 10.26 | 280 | 25.6 | 26.2 | +2.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.406 280 gr | 10.31 | 280 | 25.3 | 25.9 | +2.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.408 280 gr | 10.36 | 280 | 25.0 | 25.7 | +2.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.411 280 gr | 10.44 | 280 | 24.6 | 25.3 | +2.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.413 340 gr | 10.49 | 340 | 29.5 | 30.1 | +2.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.414 340 gr | 10.52 | 340 | 29.3 | 29.9 | +2.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.510 650 gr | 12.95 | 650 | 34.3 | 36.7 | +6.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.512 650 gr | 13.00 | 650 | 34.0 | 36.4 | +7.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.513 650 gr | 13.03 | 650 | 33.8 | 36.2 | +7.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.514 650 gr | 13.06 | 650 | 33.7 | 36.1 | +7.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.586 1000 gr | 14.88 | 1000 | 37.8 | 41.9 | +11.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.586 900 gr | 14.88 | 900 | 34.0 | 38.0 | +11.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.588 1000 gr | 14.94 | 1000 | 37.5 | 41.6 | +11.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.588 900 gr | 14.94 | 900 | 33.8 | 37.7 | +11.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.590 1000 gr | 14.99 | 1000 | 37.2 | 41.3 | +11.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.590 900 gr | 14.99 | 900 | 33.5 | 37.5 | +11.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.700 1000 gr | 17.78 | 1000 | 24.5 | 29.7 | +21.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.700 900 gr | 17.78 | 900 | 22.1 | 26.9 | +21.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.702 1000 gr | 17.83 | 1000 | 24.4 | 29.5 | +20.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip 0.702 900 gr | 17.83 | 900 | 21.9 | 26.7 | +22.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | Teilmantel Open Tip Stufenheck 0.510 650 gr | 12.95 | 650 | 34.3 | 36.7 | +6.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Ballistix Academy | ULD 0.308 168 gr | 7.82 | 168 | 28.6 | 29.7 | +3.8 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.308 180 gr | 7.82 | 180 | 30.7 | 31.7 | +3.2 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.308 200 gr | 7.82 | 200 | 34.1 | 35.0 | +2.5 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.510 740 gr | 12.95 | 740 | 39.1 | 44.7 | +14.4 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.510 760 gr | 12.95 | 760 | 40.1 | 45.9 | +14.4 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.510 780 gr | 12.95 | 780 | 41.2 | 47.0 | +14.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.510 800 gr | 12.95 | 800 | 42.2 | 48.1 | +14.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| Ballistix Academy | ULD 0.510 820 gr | 12.95 | 820 | 43.3 | 49.3 | +13.8 % | jacketed lead core, boat tail |  | -11…+10 % |
| Barnes | BUSTER 30672 | 12.70 | 400 | 22.4 | 23.1 | +3.2 % | jacketed lead core, flat nose |  | -11…+10 % |
| Barnes | CAL 32471 | 5.69 | 65 | 24.9 | 22.0 | -11.6 % | jacketed lead core |  | -11…+10 % |
| Barnes | CAL 32472 | 6.53 | 117 | 34.0 | 29.2 | -14.0 % | jacketed lead core |  | -11…+10 % |
| Barnes | HPFB Varminator 0.224 40 gr | 5.69 | 40 | 14.1 | 13.3 | -5.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | HPFB Varminator 0.243 58 gr | 6.17 | 58 | 17.0 | 16.1 | -5.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | HPFB Varminator 0.243 72 gr | 6.17 | 72 | 21.1 | 19.7 | -6.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | Kupfervoll Rundkopf 0.366 286 gr | 9.30 | 286 | 41.1 | 34.6 | -15.8 % | monolithic copper/brass, round nose, flat base |  | -11…+12 % |
| Barnes | Kupfervoll Rundkopf 0.416 400 gr | 10.57 | 400 | 44.4 | 37.0 | -16.7 % | monolithic copper/brass, round nose, flat base |  | -11…+12 % |
| Barnes | LRX 30228 | 6.71 | 127 | 35.0 | 36.9 | +5.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 30376 | 7.82 | 212 | 42.9 | 44.3 | +3.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 30479 | 6.53 | 101 | 29.4 | 31.4 | +6.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 30483 | 9.52 | 270 | 36.9 | 37.9 | +2.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 30862 | 6.17 | 95 | 30.9 | 33.1 | +7.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 30881 | 5.69 | 77 | 29.5 | 31.8 | +7.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX 31150 | 8.59 | 250 | 42.1 | 43.1 | +2.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.277 129 gr | 7.04 | 129 | 32.3 | 34.1 | +5.6 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.284 139 gr | 7.21 | 139 | 33.1 | 34.9 | +5.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.284 145 gr | 7.21 | 145 | 34.6 | 36.3 | +5.0 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.284 160 gr | 7.21 | 160 | 38.1 | 39.9 | +4.6 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.308 175 gr | 7.82 | 175 | 35.5 | 37.0 | +4.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.308 200 gr | 7.82 | 200 | 40.5 | 42.0 | +3.6 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.308 208 gr | 7.82 | 208 | 42.2 | 43.5 | +3.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.338 265 gr | 8.59 | 265 | 44.6 | 45.5 | +2.0 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | LRX Long-Range BT 0.338 280 gr | 8.59 | 280 | 47.1 | 47.9 | +1.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | MPG 30249 | 7.04 | 85 | 18.6 | 18.7 | +0.6 % | jacketed lead core |  | -11…+10 % |
| Barnes | MPG 30331 | 7.82 | 150 | 25.8 | 26.0 | +0.8 % | jacketed lead core |  | -11…+10 % |
| Barnes | MPG 30388 | 7.87 | 108 | 18.3 | 18.9 | +3.1 % | jacketed lead core |  | -11…+10 % |
| Barnes | Match Burner BT 0.243 105 gr | 6.17 | 105 | 31.5 | 33.8 | +7.3 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner BT 0.243 112 gr | 6.17 | 112 | 33.6 | 35.9 | +6.9 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner BT 0.264 140 gr | 6.71 | 140 | 35.8 | 37.6 | +5.1 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner BT 0.264 145 gr | 6.71 | 145 | 37.1 | 38.9 | +4.8 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner HP 0.224 85 gr | 5.69 | 85 | 29.8 | 29.3 | -1.8 % | jacketed lead core, hollow point, match |  | -11…+10 % |
| Barnes | Match Burner HP 0.264 120 gr | 6.71 | 120 | 30.7 | 29.4 | -4.4 % | jacketed lead core, hollow point, match |  | -11…+10 % |
| Barnes | Match Burner HP BT 0.224 69 gr | 5.69 | 69 | 24.2 | 25.4 | +5.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner HP BT 0.308 155 gr | 7.82 | 155 | 29.5 | 29.3 | -0.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Barnes | Match Burner HP BT 0.308 175 gr | 7.82 | 175 | 33.3 | 32.8 | -1.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Barnes | ORIGINAL 30360 | 7.82 | 190 | 32.7 | 32.4 | -0.8 % | jacketed lead core |  | -11…+10 % |
| Barnes | ORIGINAL 30497 | 9.52 | 255 | 28.0 | 29.2 | +4.2 % | jacketed lead core |  | -11…+10 % |
| Barnes | ORIGINAL 30498 | 9.58 | 255 | 27.6 | 28.8 | +4.4 % | jacketed lead core |  | -11…+10 % |
| Barnes | Original FNSP 0.348 220 gr | 8.84 | 220 | 28.7 | 26.7 | -7.0 % | jacketed lead core, flat nose |  | -11…+10 % |
| Barnes | Original FNSP 0.348 250 gr | 8.84 | 250 | 32.6 | 30.1 | -7.7 % | jacketed lead core, flat nose |  | -11…+10 % |
| Barnes | Original FNSP 0.510 300 gr | 12.95 | 300 | 16.0 | 17.0 | +6.3 % | jacketed lead core, flat nose |  | -11…+10 % |
| Barnes | Original FNSP 0.510 450 gr | 12.95 | 450 | 24.0 | 24.9 | +3.6 % | jacketed lead core, flat nose |  | -11…+10 % |
| Barnes | SOLID 30536 | 10.72 | 400 | 33.3 | 40.0 | +20.1 % | monolithic copper/brass |  | -11…+12 % |
| Barnes | SOLID 30649 | 12.04 | 500 | 31.7 | 39.4 | +24.1 % | monolithic copper/brass |  | -11…+12 % |
| Barnes | SOLID 30694 | 12.95 | 535 | 28.5 | 36.4 | +27.7 % | monolithic copper/brass |  | -11…+12 % |
| Barnes | SOLID 30713 | 14.81 | 750 | 29.0 | 38.5 | +32.7 % | monolithic copper/brass |  | -11…+12 % |
| Barnes | TAC-TX 31142 | 6.71 | 115 | 31.8 | 31.7 | -0.2 % | monolithic copper/brass, boat tail |  | -11…+12 % |
| Barnes | TAC-XP HP 0.355 115 gr | 9.02 | 115 | 17.5 | 16.5 | -5.5 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Barnes | TAC-XP HP 0.355 95 gr | 9.02 | 95 | 14.5 | 13.8 | -4.7 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Barnes | TAC-XP HP 0.40 125 gr | 10.16 | 125 | 15.0 | 14.2 | -5.4 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Barnes | TAC-XP HP 0.40 140 gr | 10.16 | 140 | 16.8 | 15.8 | -6.1 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Barnes | TSX 30204 | 5.69 | 78 | 29.8 | 32.2 | +8.0 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX 30224 | 6.53 | 115 | 33.4 | 34.5 | +3.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 30391 | 7.87 | 123 | 24.7 | 26.3 | +6.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX 30393 | 7.90 | 150 | 29.8 | 30.6 | +2.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 30417 | 8.59 | 285 | 48.0 | 48.7 | +1.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX 30457 | 9.09 | 225 | 33.8 | 34.1 | +0.8 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 30514 | 10.41 | 400 | 45.7 | 44.9 | -1.8 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 30516 | 10.44 | 300 | 34.2 | 34.1 | -0.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 30688 | 12.83 | 525 | 39.6 | 38.6 | -2.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 31112 | 9.09 | 180 | 27.1 | 27.6 | +1.9 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 32078 | 9.02 | 170 | 26.0 | 26.6 | +2.2 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX 32473 | 10.16 | 190 | 22.9 | 23.4 | +2.2 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.224 70 gr | 5.69 | 70 | 26.8 | 29.1 | +8.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.284 120 gr | 7.21 | 120 | 28.6 | 30.4 | +6.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.284 140 gr | 7.21 | 140 | 33.4 | 35.2 | +5.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.284 150 gr | 7.21 | 150 | 35.8 | 37.5 | +4.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.308 130 gr | 7.82 | 130 | 26.4 | 28.0 | +6.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.308 165 gr | 7.82 | 165 | 33.4 | 35.0 | +4.9 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.308 168 gr | 7.82 | 168 | 34.1 | 35.6 | +4.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.323 180 gr | 8.20 | 180 | 33.2 | 34.7 | +4.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.323 200 gr | 8.20 | 200 | 36.9 | 38.3 | +3.7 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.338 185 gr | 8.59 | 185 | 31.1 | 32.5 | +4.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX BT 0.338 210 gr | 8.59 | 210 | 35.3 | 36.6 | +3.7 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.284 160 gr | 7.21 | 160 | 38.1 | 37.7 | -1.1 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.284 175 gr | 7.21 | 175 | 41.7 | 41.0 | -1.7 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.338 225 gr | 8.59 | 225 | 37.9 | 36.9 | -2.6 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.366 286 gr | 9.30 | 286 | 41.1 | 39.6 | -3.6 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.375 235 gr | 9.52 | 235 | 32.1 | 31.5 | -1.9 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.375 300 gr | 9.52 | 300 | 41.0 | 39.6 | -3.4 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.375 350 gr | 9.52 | 350 | 47.9 | 45.7 | -4.5 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.416 300 gr | 10.57 | 300 | 33.3 | 32.3 | -3.0 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.416 400 gr | 10.57 | 400 | 44.4 | 42.3 | -4.7 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.458 350 gr | 11.63 | 350 | 32.1 | 31.0 | -3.4 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.458 450 gr | 11.63 | 450 | 41.3 | 39.2 | -5.0 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX FB 0.458 500 gr | 11.63 | 500 | 45.8 | 43.3 | -5.4 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX Flachkopf 0.458 250 gr | 11.63 | 250 | 22.9 | 20.6 | -10.1 % | monolithic copper/brass, flat nose, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX HPBT 0.264 120 gr | 6.71 | 120 | 33.1 | 36.2 | +9.3 % | monolithic copper/brass, hollow point, boat tail, relief grooves, match |  | -11…+12 % |
| Barnes | TSX HPBT 0.277 110 gr | 7.04 | 110 | 27.6 | 30.4 | +10.1 % | monolithic copper/brass, hollow point, boat tail, relief grooves, match |  | -11…+12 % |
| Barnes | TSX HPBT 0.277 130 gr | 7.04 | 130 | 32.6 | 35.5 | +9.0 % | monolithic copper/brass, hollow point, boat tail, relief grooves, match |  | -11…+12 % |
| Barnes | TSX HPBT 0.277 140 gr | 7.04 | 140 | 35.1 | 38.1 | +8.5 % | monolithic copper/brass, hollow point, boat tail, relief grooves, match |  | -11…+12 % |
| Barnes | TSX HPFB 0.277 150 gr | 7.04 | 150 | 37.6 | 38.3 | +1.8 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Barnes | TSX Triple Schock FB 0.224 45 gr | 5.69 | 45 | 17.2 | 18.2 | +5.6 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TSX-BT 0.308 180 gr | 7.82 | 180 | 36.5 | 38.0 | +4.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TSX-FB 0.308 200 gr | 7.82 | 200 | 40.5 | 39.7 | -2.0 % | monolithic copper/brass, flat base, relief grooves |  | -11…+12 % |
| Barnes | TTSX 0.257 80 gr | 6.53 | 80 | 23.3 | 25.2 | +8.2 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX 0.277 110 gr | 7.04 | 110 | 27.6 | 29.4 | +6.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX 0.277 130 gr | 7.04 | 130 | 32.6 | 34.4 | +5.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX 30461 | 9.09 | 200 | 30.0 | 31.3 | +4.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX 30996 | 11.46 | 250 | 23.6 | 24.6 | +4.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.264 100 gr | 6.71 | 100 | 27.6 | 29.5 | +6.9 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.264 120 gr | 6.71 | 120 | 33.1 | 35.0 | +5.7 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.277 130 gr | 7.04 | 130 | 32.6 | 34.4 | +5.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.284 120 gr | 7.21 | 120 | 28.6 | 30.4 | +6.4 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.284 140 gr | 7.21 | 140 | 33.4 | 35.2 | +5.3 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.284 150 gr | 7.21 | 150 | 35.8 | 37.5 | +4.8 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.308 130 gr | 7.82 | 130 | 26.4 | 28.0 | +6.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.308 165 gr | 7.82 | 165 | 33.4 | 35.0 | +4.9 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.308 168 gr | 7.82 | 168 | 34.1 | 35.6 | +4.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.308 180 gr | 7.82 | 180 | 36.5 | 38.0 | +4.1 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.338 185 gr | 8.59 | 185 | 31.1 | 32.5 | +4.5 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.338 210 gr | 8.59 | 210 | 35.3 | 36.6 | +3.7 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX BT 0.338 225 gr | 8.59 | 225 | 37.9 | 39.0 | +3.0 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | TTSX-BT 0.277 95 gr | 7.04 | 95 | 23.8 | 25.6 | +7.6 % | monolithic copper/brass, boat tail, relief grooves |  | -11…+12 % |
| Barnes | VARMINATOR 30092 | 5.18 | 32 | 13.5 | 12.9 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | Varmint Grenade 0.204 26 gr | 5.18 | 26 | 11.2 | 10.6 | -5.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | Varmint Grenade 0.243 62 gr | 6.17 | 62 | 18.2 | 17.1 | -6.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | Varmint Grenade FRAN 0.224 30 gr | 5.69 | 30 | 10.5 | 10.1 | -3.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | Varmint Grenade FRAN HP 0.224 36 gr | 5.69 | 36 | 12.3 | 12.0 | -2.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Barnes | X Pistole 0.429 225 gr | 10.90 | 225 | 18.1 | 19.9 | +10.1 % | jacketed lead core |  | -11…+10 % |
| Barnes | X Pistole 0.451 200 gr | 11.46 | 200 | 14.3 | 16.2 | +13.3 % | jacketed lead core |  | -11…+10 % |
| Barnes | X Pistole 0.451 225 gr | 11.46 | 225 | 16.1 | 18.1 | +12.3 % | jacketed lead core |  | -11…+10 % |
| Barnes | X Pistole 0.500 275 gr | 12.70 | 275 | 15.4 | 17.9 | +16.1 % | jacketed lead core |  | -11…+10 % |
| Barnes | X Pistole 0.500 325 gr | 12.70 | 325 | 18.2 | 20.9 | +14.9 % | jacketed lead core |  | -11…+10 % |
| Barnes | X Pistole 0.500 375 gr | 12.70 | 375 | 21.0 | 23.9 | +13.9 % | jacketed lead core |  | -11…+10 % |
| Barnes | XPB 30562 | 11.46 | 250 | 17.8 | 20.0 | +12.2 % | jacketed lead core |  | -11…+10 % |
| Barnes | XPB 30659 | 12.06 | 275 | 17.4 | 19.8 | +13.6 % | jacketed lead core |  | -11…+10 % |
| Berger | HPBT (Target) 0.264 120 gr | 6.71 | 120 | 30.7 | 31.0 | +1.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Berger | Long Range Hybrid Target 0.270 150 gr | 6.86 | 150 | 36.7 | 38.4 | +4.8 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| Bloodsport | X Kupfer Hohlspitz 0.277 130 gr | 7.04 | 130 | 32.6 | 30.0 | -7.9 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Brenneke | Basic Nature 0.308 139 gr | 7.82 | 139 | 24.7 | 24.2 | -2.0 % | jacketed lead core |  | -11…+10 % |
| Brenneke | TAG 0.323 175 gr | 8.20 | 175 | 27.9 | 27.4 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Brenneke | TAG 0.366 225 gr | 9.30 | 225 | 26.9 | 27.1 | +0.9 % | jacketed lead core |  | -11…+10 % |
| Brenneke | TIG 0.284 177 gr | 7.21 | 177 | 37.7 | 35.6 | -5.7 % | jacketed lead core |  | -11…+10 % |
| Brenneke | TIG 0.308 150 gr | 7.82 | 150 | 26.6 | 26.0 | -2.3 % | jacketed lead core |  | -11…+10 % |
| Brenneke | TOG 0.277 150 gr | 7.04 | 150 | 33.8 | 34.3 | +1.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Brenneke | TOG 0.338 225 gr | 8.59 | 225 | 32.3 | 34.1 | +5.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Brenneke | TOG 0.366 247 gr | 9.30 | 247 | 29.5 | 31.9 | +8.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Brenneke | TOG 0.413 232 gr | 10.49 | 232 | 21.0 | 23.8 | +13.3 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Brenneke | TUG 0.366 293 gr | 9.30 | 293 | 35.0 | 34.8 | -0.6 % | jacketed lead core |  | -11…+10 % |
| Brenneke | Teilmantel 0.284 145 gr | 7.21 | 145 | 30.9 | 29.5 | -4.6 % | jacketed lead core |  | -11…+10 % |
| Buffalo Arms | Hartblei 7 mm Nambu 0.280 60 gr | 7.11 | 60 | 12.8 | 13.2 | +3.5 % | jacketed lead core |  | -11…+10 % |
| Cam Pro | Balleurope Vollmantel BT Crimprille 0.224 55 gr | 5.69 | 55 | 17.0 | 18.6 | +9.4 % | full metal jacket, boat tail |  | -15…+10 % |
| Cam Pro | Balleurope Vollmantel BT Crimprille 0.224 62 gr | 5.69 | 62 | 19.2 | 20.8 | +8.4 % | full metal jacket, boat tail |  | -15…+10 % |
| Cam Pro | Balleurope Vollmantel BT Crimprille 0.308 147 gr | 7.82 | 147 | 23.9 | 25.2 | +5.5 % | full metal jacket, boat tail |  | -15…+10 % |
| CloverBullets | HTLR Tombak Jagd- / Scheiben 0.308 150 gr | 7.82 | 150 | 25.8 | 29.4 | +14.1 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Messing Jagd 0.284 120 gr | 7.21 | 120 | 24.8 | 28.0 | +12.8 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Messing Jagd 0.308 135 gr | 7.82 | 135 | 23.2 | 26.7 | +14.9 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Messing Jagd 0.323 145 gr | 8.20 | 145 | 22.4 | 26.0 | +16.1 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Messing Jagd 0.366 160 gr | 9.30 | 160 | 18.6 | 22.3 | +20.0 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Tombak Jagd 0.284 125 gr | 7.21 | 125 | 25.9 | 29.1 | +12.2 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Tombak Jagd 0.308 140 gr | 7.82 | 140 | 24.1 | 27.6 | +14.5 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Tombak Jagd 0.323 145 gr | 8.20 | 145 | 22.4 | 26.0 | +16.1 % | monolithic copper/brass |  | -11…+12 % |
| CloverBullets | Tombak Jagd 0.366 170 gr | 9.30 | 170 | 19.7 | 23.6 | +20.0 % | monolithic copper/brass |  | -11…+12 % |
| CopperBear | Kupferhohlspitz 0.224 40 gr | 5.69 | 40 | 14.2 | 15.0 | +5.9 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| CopperBear | Kupferhohlspitz 0.338 260 gr | 8.59 | 260 | 40.6 | 39.1 | -3.8 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| CopperBear | Kupferhohlspitz 0.458 280 gr | 11.63 | 280 | 25.7 | 23.2 | -9.6 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Cutting Edge | Flat Base Raptor Hohlspitz 0.308 130 gr | 7.82 | 130 | 17.6 | 23.8 | +35.1 % | monolithic copper/brass, hollow point, flat base |  | -11…+12 % |
| Cutting Edge | MTAC (Match/Tactical) 0.277 125 gr | 7.04 | 125 | 23.7 | 33.1 | +39.6 % | monolithic copper/brass, match |  | -11…+12 % |
| Cutting Edge | MTAC (Match/Tactical) 0.308 132 gr | 7.82 | 132 | 20.4 | 28.4 | +39.2 % | monolithic copper/brass, match |  | -11…+12 % |
| Cutting Edge | MTAC (Match/Tactical) 0.308 155 gr | 7.82 | 155 | 24.0 | 33.0 | +37.5 % | monolithic copper/brass, match |  | -11…+12 % |
| Cutting Edge | MTH (Match / Tactical / Hunting) 0.338 265 gr | 8.59 | 265 | 34.4 | 45.5 | +32.2 % | monolithic copper/brass, match |  | -11…+12 % |
| Cutting Edge | Safari Raptor Hohlspitz 0.375 275 gr | 9.52 | 275 | 23.5 | 33.7 | +43.5 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Cutting Edge | Single Feed LZR (Lazer Tipped Hollow Point) 0.416 550 gr | 10.57 | 550 | 36.8 | 56.7 | +54.1 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Cutting Edge | Single Feed MTAC (Match/Tactical) 0.510 1002 gr | 12.95 | 1002 | 60.6 | 71.3 | +17.6 % | monolithic copper/brass, match |  | -11…+12 % |
| DK Bullets | GTE CU HV FB Kupferhohlspitz 0.308 110 gr | 7.82 | 110 | 18.4 | 20.3 | +10.4 % | monolithic copper/brass, hollow point, flat base |  | -11…+12 % |
| DK Bullets | GTE CU SV BT Kupferhohlspitz Subsonic 0.458 500 gr | 11.63 | 500 | 37.8 | 41.1 | +8.7 % | monolithic copper/brass, hollow point, boat tail |  | -11…+12 % |
| DK Bullets | GTE CU SV BT Subsonic Kupferhohlspitz 0.458 420 gr | 11.63 | 420 | 31.7 | 34.9 | +10.1 % | monolithic copper/brass, hollow point, boat tail |  | -11…+12 % |
| DK Bullets | Hunter HPBT 0.224 55 gr | 5.69 | 55 | 17.4 | 20.5 | +18.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| DK Bullets | Hunter HPBT 0.277 128 gr | 7.04 | 128 | 26.4 | 30.0 | +13.6 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| DK Bullets | Hunter HPBT 0.312 150 gr | 7.92 | 150 | 24.4 | 27.7 | +13.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| DK Bullets | Hunter HPBT 0.323 196 gr | 8.20 | 196 | 29.8 | 33.3 | +11.6 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| DK Bullets | Hunter HPBT 0.375 276 gr | 9.52 | 276 | 31.1 | 34.3 | +10.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Degol | Hohlspitz mit Boattail 0.416 420 gr | 10.57 | 420 | 45.2 | 37.1 | -17.9 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| Degol | Rundkopf Lion 0.474 500 gr | 12.04 | 500 | 28.9 | 31.2 | +8.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Rundkopf Lion 0.510 570 gr | 12.95 | 570 | 27.7 | 30.7 | +10.7 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Rundkopf Lion 0.510 600 gr | 12.95 | 600 | 29.2 | 32.2 | +10.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Rundkopf Vollmantel 0.510 600 gr | 12.95 | 600 | 31.5 | 31.0 | -1.7 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Starkmantel Flachkopf PP 0.430 265 gr | 10.92 | 265 | 19.3 | 20.5 | +6.1 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Halbspitz 0.458 600 gr | 11.63 | 600 | 37.7 | 44.1 | +17.0 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Halbspitz PP 0.458 400 gr | 11.63 | 400 | 25.1 | 30.2 | +20.1 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Halbspitz PP 0.458 500 gr | 11.63 | 500 | 31.4 | 37.2 | +18.4 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.375 350 gr | 9.52 | 350 | 35.0 | 35.3 | +0.9 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.411 300 gr | 10.44 | 300 | 24.3 | 25.5 | +5.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.411 400 gr | 10.44 | 400 | 32.4 | 33.4 | +3.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.416 410 gr | 10.57 | 410 | 32.3 | 33.4 | +3.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.423 300 gr | 10.74 | 300 | 22.7 | 24.2 | +6.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.423 347 gr | 10.74 | 347 | 26.3 | 27.7 | +5.3 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.423 400 gr | 10.74 | 400 | 30.3 | 31.6 | +4.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.458 480 gr | 11.63 | 480 | 30.1 | 32.2 | +6.9 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.458 500 gr | 11.63 | 500 | 31.4 | 33.4 | +6.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.458 600 gr | 11.63 | 600 | 37.7 | 39.7 | +5.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf 0.510 570 gr | 12.95 | 570 | 27.7 | 30.7 | +10.7 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Rundkopf PP 0.510 600 gr | 12.95 | 600 | 29.2 | 32.2 | +10.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Spitz PP 0.423 350 gr | 10.74 | 350 | 26.5 | 31.1 | +17.2 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Flachkopf 0.364 193 gr | 9.25 | 193 | 20.7 | 21.0 | +1.4 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Halbrund 0.264 156 gr | 6.71 | 156 | 34.7 | 36.3 | +4.7 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.227 77 gr | 5.77 | 77 | 27.3 | 23.9 | -12.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.243 70 gr | 6.17 | 70 | 18.8 | 19.2 | +2.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.257 125 gr | 6.53 | 125 | 33.5 | 29.6 | -11.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.264 140 gr | 6.71 | 140 | 35.3 | 31.2 | -11.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.277 130 gr | 7.04 | 130 | 29.4 | 26.5 | -9.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.285 130 gr | 7.24 | 130 | 27.5 | 25.1 | -8.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.318 150 gr | 8.08 | 150 | 24.7 | 23.2 | -6.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.338 300 gr | 8.59 | 300 | 42.9 | 39.5 | -8.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.356 200 gr | 9.04 | 200 | 25.3 | 24.4 | -3.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.358 200 gr | 9.09 | 200 | 25.0 | 24.2 | -3.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.366 320 gr | 9.30 | 320 | 38.0 | 35.9 | -5.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.375 245 gr | 9.52 | 245 | 27.5 | 26.7 | -2.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.375 350 gr | 9.52 | 350 | 39.3 | 37.3 | -5.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.413 270 gr | 10.49 | 270 | 24.1 | 24.2 | +0.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz 0.416 270 gr | 10.57 | 270 | 23.7 | 23.9 | +0.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz PP 0.356 225 gr | 9.04 | 225 | 28.5 | 27.3 | -4.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Hohlspitz PP 0.416 350 gr | 10.57 | 350 | 30.7 | 30.5 | -0.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Degol | Starkmantel Verbundkern PP 0.435 350 gr | 11.05 | 350 | 24.8 | 29.4 | +18.5 % | jacketed lead core |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.2615 120 gr | 6.64 | 120 | 27.3 | 26.1 | -4.6 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.264 160 gr | 6.71 | 160 | 35.6 | 33.4 | -6.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.318 175 gr | 8.08 | 175 | 25.6 | 25.3 | -1.0 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.323 175 gr | 8.20 | 175 | 24.7 | 24.6 | -0.3 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.338 300 gr | 8.59 | 300 | 38.1 | 37.3 | -2.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.356 250 gr | 9.04 | 250 | 28.2 | 28.5 | +1.0 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.358 250 gr | 9.09 | 250 | 27.8 | 28.2 | +1.3 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.375 245 gr | 9.52 | 245 | 24.5 | 25.3 | +3.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.375 270 gr | 9.52 | 270 | 27.0 | 27.7 | +2.5 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.375 300 gr | 9.52 | 300 | 30.0 | 30.5 | +1.8 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.435 410 gr | 11.05 | 410 | 29.1 | 30.6 | +5.3 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.440 330 gr | 11.18 | 330 | 22.8 | 24.4 | +7.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.468 480 gr | 11.89 | 480 | 28.6 | 30.8 | +7.7 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.585 750 gr | 14.86 | 750 | 26.3 | 30.4 | +15.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.620 900 gr | 15.75 | 900 | 27.4 | 32.2 | +17.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf 0.700 1000 gr | 17.78 | 1000 | 22.6 | 28.1 | +24.1 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf Hohlspitz 0.323 125 gr | 8.20 | 125 | 19.9 | 17.1 | -14.2 % | jacketed lead core, round nose, hollow point, flat base |  | -11…+10 % |
| Degol | Starkmantel Verbundkern Rundkopf Scharfrand 0.323 185 gr | 8.20 | 185 | 26.1 | 27.5 | +5.3 % | jacketed lead core, round nose, flat base, relief grooves |  | -11…+10 % |
| Degol | Teilmantel Flachkopf 0.500 350 gr | 12.70 | 350 | 17.9 | 19.8 | +10.7 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Hohlspitz Boattail 0.458 600 gr | 11.63 | 600 | 54.0 | 43.1 | -20.2 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| Degol | Teilmantel Rundkopf 0.423 350 gr | 10.74 | 350 | 26.5 | 27.9 | +5.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Rundkopf 0.510 700 gr | 12.95 | 700 | 34.1 | 37.2 | +9.0 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Rundkopf Lion 0.375 350 gr | 9.52 | 350 | 35.0 | 35.3 | +0.9 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Rundkopf Lion 0.458 480 gr | 11.63 | 480 | 30.1 | 32.2 | +6.9 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Rundkopf Lion 0.458 500 gr | 11.63 | 500 | 31.4 | 33.4 | +6.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Degol | Teilmantel Spitz 0.510 700 gr | 12.95 | 700 | 34.1 | 41.4 | +21.3 % | jacketed lead core |  | -11…+10 % |
| Degol | Vollmantel 0.264 160 gr | 6.71 | 160 | 31.7 | 35.8 | +12.9 % | full metal jacket |  | -15…+10 % |
| Degol | Vollmantel 0.323 196 gr | 8.20 | 196 | 25.9 | 29.3 | +13.2 % | full metal jacket |  | -15…+10 % |
| Degol | Vollmantel Flachkopf 0.452 260 gr | 11.48 | 260 | 17.4 | 17.6 | +0.9 % | full metal jacket, flat nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Flachkopf 0.500 275 gr | 12.70 | 275 | 15.0 | 15.2 | +1.4 % | full metal jacket, flat nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Flachkopf 0.500 350 gr | 12.70 | 350 | 19.1 | 19.1 | -0.1 % | full metal jacket, flat nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Flachkopf 0.500 375 gr | 12.70 | 375 | 20.5 | 20.3 | -0.7 % | full metal jacket, flat nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Flachkopf 0.500 500 gr | 12.70 | 500 | 27.3 | 26.7 | -2.4 % | full metal jacket, flat nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.318 196 gr | 8.08 | 196 | 26.7 | 27.1 | +1.6 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.366 285 gr | 9.30 | 285 | 29.2 | 29.3 | +0.4 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.366 320 gr | 9.30 | 320 | 32.8 | 32.7 | -0.3 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.375 300 gr | 9.52 | 300 | 29.3 | 29.4 | +0.4 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.375 350 gr | 9.52 | 350 | 34.2 | 34.0 | -0.6 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.411 400 gr | 10.44 | 400 | 32.5 | 32.2 | -1.0 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.416 410 gr | 10.57 | 410 | 32.5 | 32.2 | -1.0 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.423 347 gr | 10.74 | 347 | 26.6 | 26.7 | +0.2 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.423 400 gr | 10.74 | 400 | 30.6 | 30.5 | -0.4 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.435 410 gr | 11.05 | 410 | 29.7 | 29.5 | -0.7 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.458 480 gr | 11.63 | 480 | 31.3 | 31.0 | -1.1 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.458 500 gr | 11.63 | 500 | 32.6 | 32.2 | -1.3 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.458 600 gr | 11.63 | 600 | 39.1 | 38.2 | -2.4 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.468 480 gr | 11.89 | 480 | 29.9 | 29.7 | -0.8 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.505 525 gr | 12.83 | 525 | 28.1 | 27.8 | -1.0 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.510 535 gr | 12.95 | 535 | 28.0 | 27.8 | -0.7 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.510 570 gr | 12.95 | 570 | 29.9 | 29.5 | -1.3 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Degol | Vollmantel Rundkopf 0.700 1000 gr | 17.78 | 1000 | 27.6 | 27.0 | -2.2 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Federal | Fusion 0.338 225 gr | 8.59 | 225 | 31.3 | 31.7 | +1.2 % | jacketed lead core |  | -11…+10 % |
| Federal | Fusion FB224F1 | 5.69 | 90 | 31.5 | 30.7 | -2.6 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB264F2 | 6.71 | 140 | 35.8 | 33.7 | -5.9 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB277F2 | 7.04 | 130 | 30.3 | 28.6 | -5.5 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB277F4 | 7.04 | 150 | 35.0 | 32.7 | -6.4 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB284F1 | 7.21 | 140 | 31.1 | 29.3 | -5.8 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB284F3 | 7.21 | 160 | 35.6 | 33.2 | -6.7 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB284F4 | 7.21 | 175 | 38.9 | 36.1 | -7.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB308F1 | 7.82 | 150 | 28.6 | 26.7 | -6.7 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Fusion FB308F4 | 7.82 | 180 | 34.3 | 31.7 | -7.7 % | jacketed lead core, boat tail |  | -11…+10 % |
| Federal | Terminal Ascent PB264TA1 | 6.71 | 130 | 33.2 | 33.8 | +1.9 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Terminal Ascent PB277TA1 | 7.04 | 136 | 31.7 | 32.1 | +1.4 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Terminal Ascent PB284TA1 | 7.21 | 155 | 34.5 | 34.7 | +0.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Terminal Ascent PB308TA1 | 7.82 | 175 | 33.3 | 33.2 | -0.3 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Terminal Ascent PB308TA2 | 7.82 | 200 | 38.1 | 37.6 | -1.3 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Bear Claw PB375TBBC250 | 9.52 | 250 | 27.4 | 31.6 | +15.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Bear Claw PB375TBBC300 | 9.52 | 300 | 32.9 | 37.5 | +14.1 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Bear Claw PB416TBBC400 | 10.57 | 400 | 34.5 | 40.1 | +16.3 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Bear Claw PB474TBBC500 | 12.04 | 500 | 31.7 | 38.4 | +21.1 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Sledgehammer Solid PB375TBSH300 | 9.52 | 300 | 32.9 | 42.5 | +29.2 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Federal | Trophy Bonded Sledgehammer Solid PB416TBSH400 | 10.57 | 400 | 34.5 | 45.4 | +31.7 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Federal | Trophy Bonded Sledgehammer Solid PB458TBSH500 | 11.63 | 500 | 34.4 | 46.5 | +35.2 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Federal | Trophy Bonded Sledgehammer Solid PB474TBSH500 | 12.04 | 500 | 31.7 | 43.5 | +37.2 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Federal | Trophy Bonded Tip PB277TT130 | 7.04 | 130 | 28.4 | 30.8 | +8.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB277TT140 | 7.04 | 140 | 30.6 | 33.0 | +7.9 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB284TT140 | 7.21 | 140 | 29.0 | 31.5 | +8.7 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB284TT160 | 7.21 | 160 | 33.1 | 35.7 | +8.0 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB308TT165 | 7.82 | 165 | 28.4 | 31.4 | +10.6 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB308TT180 | 7.82 | 180 | 31.0 | 34.1 | +9.9 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Federal | Trophy Bonded Tip PB338TT200 | 8.59 | 200 | 27.9 | 31.3 | +12.3 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Fiocchi | Blei RK 0.251 50 gr | 6.38 | 50 | 12.1 | 13.5 | +11.7 % | lead |  | -8…+11 % |
| Fiocchi | SJSP 0.430 240 gr | 10.92 | 240 | 19.1 | 21.1 | +10.5 % | jacketed lead core |  | -11…+10 % |
| Fiocchi | VM 0.251 50 gr | 6.38 | 50 | 13.6 | 13.8 | +1.3 % | jacketed lead core |  | -11…+10 % |
| Fox | Classic Hunter Bleifrei 0.264 139 gr | 6.71 | 139 | 37.7 | 36.9 | -2.1 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.306 155 gr | 7.77 | 155 | 31.3 | 30.7 | -1.8 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.308 110 gr | 7.82 | 110 | 21.9 | 22.0 | +0.5 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.308 165 gr | 7.82 | 165 | 32.8 | 32.2 | -1.9 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.311 123 gr | 7.90 | 123 | 24.0 | 24.0 | -0.2 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.311 155 gr | 7.90 | 155 | 30.3 | 29.8 | -1.8 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.318 150 gr | 8.08 | 150 | 28.0 | 27.6 | -1.3 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Bleifrei 0.413 200 gr | 10.49 | 200 | 22.1 | 21.8 | -1.4 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Tipless Bleifrei 0.228 50 gr | 5.79 | 50 | 18.2 | 18.8 | +3.5 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Tipless Bleifrei 0.243 100 gr | 6.17 | 100 | 32.0 | 31.9 | -0.3 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Classic Hunter Tipless Bleifrei 0.366 200 gr | 9.30 | 200 | 28.2 | 27.5 | -2.4 % | monolithic copper/brass |  | -11…+12 % |
| Fox | Target Bleifrei 0.224 55 gr | 5.69 | 55 | 20.7 | 23.2 | +12.0 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.243 80 gr | 6.17 | 80 | 25.6 | 28.1 | +9.9 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.284 130 gr | 7.21 | 130 | 30.4 | 32.8 | +7.8 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.308 150 gr | 7.82 | 150 | 29.9 | 32.0 | +7.1 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.323 160 gr | 8.20 | 160 | 29.0 | 31.0 | +6.9 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.338 185 gr | 8.59 | 185 | 30.6 | 32.5 | +6.1 % | monolithic copper/brass, match |  | -11…+12 % |
| Fox | Target Bleifrei 0.366 220 gr | 9.30 | 220 | 31.0 | 32.7 | +5.6 % | monolithic copper/brass, match |  | -11…+12 % |
| Frontier | Spartan Premium Hunter 0.284 155 gr | 7.21 | 155 | 38.1 | 35.5 | -6.7 % | monolithic copper/brass |  | -11…+12 % |
| Frontier | Spartan Xiphos Hohlspitz 0.355 90 gr | 9.02 | 90 | 12.8 | 13.1 | +2.6 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Frontier | Spartan Xiphos Hohlspitz 0.40 120 gr | 10.16 | 120 | 12.9 | 13.7 | +5.8 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| Frontier | Spartan Xiphos Hohlspitz 0.451 150 gr | 11.46 | 150 | 12.1 | 13.3 | +10.1 % | monolithic copper/brass, hollow point |  | -11…+12 % |
| GECO | Express 0.284 155 gr | 7.21 | 155 | 34.4 | 31.4 | -8.7 % | jacketed lead core |  | -11…+10 % |
| GECO | Express 0.308 165 gr | 7.82 | 165 | 30.5 | 28.4 | -6.8 % | jacketed lead core |  | -11…+10 % |
| GECO | Hexagon 0.355 124 gr | 9.02 | 124 | 16.6 | 16.5 | -0.8 % | jacketed lead core |  | -11…+10 % |
| GECO | Plus 0.264 156 gr | 6.71 | 156 | 40.8 | 36.3 | -11.0 % | jacketed lead core |  | -11…+10 % |
| GECO | Plus Teilmantel 0.277 150 gr | 7.04 | 150 | 35.2 | 31.9 | -9.4 % | jacketed lead core |  | -11…+10 % |
| GECO | Plus Teilmantel 0.284 170 gr | 7.21 | 170 | 37.7 | 34.2 | -9.2 % | jacketed lead core |  | -11…+10 % |
| GECO | Plus Teilmantel 0.308 170 gr | 7.82 | 170 | 31.4 | 29.2 | -6.9 % | jacketed lead core |  | -11…+10 % |
| GECO | Plus Teilmantel 0.323 196 gr | 8.20 | 196 | 32.5 | 30.5 | -6.3 % | jacketed lead core |  | -11…+10 % |
| GECO | TMR 0.366 255 gr | 9.30 | 255 | 31.7 | 30.5 | -3.7 % | jacketed lead core |  | -11…+10 % |
| GECO | Teilmantel 0.243 105 gr | 6.17 | 105 | 33.0 | 29.5 | -10.6 % | jacketed lead core |  | -11…+10 % |
| GECO | Teilmantel 0.284 165 gr | 7.21 | 165 | 36.6 | 33.3 | -9.0 % | jacketed lead core |  | -11…+10 % |
| GECO | Vollmantel Rundkopf 0.355 115 gr | 9.02 | 115 | 12.9 | 13.3 | +3.0 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| GECO | Vollmantel Rundkopf 0.355 124 gr | 9.02 | 124 | 13.9 | 14.3 | +2.6 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| GECO | Zero 0.308 136 gr | 7.82 | 136 | 25.1 | 23.7 | -5.5 % | jacketed lead core |  | -11…+10 % |
| GECO | Zero 0.323 139 gr | 8.20 | 139 | 23.0 | 22.1 | -4.0 % | jacketed lead core |  | -11…+10 % |
| GECO | Zero 0.366 188 gr | 9.30 | 188 | 23.4 | 22.9 | -2.0 % | jacketed lead core |  | -11…+10 % |
| GM | Murmel Messingjagd 0.406 190 gr | 10.31 | 190 | 17.3 | 21.5 | +24.1 % | monolithic copper/brass |  | -11…+12 % |
| GM | Splinter Crown Messingjagd 0.284 104 gr | 7.21 | 104 | 21.5 | 26.3 | +22.4 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Crown Messingjagd 0.308 121 gr | 7.82 | 121 | 20.8 | 25.9 | +24.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Crown Messingjagd 0.323 127 gr | 8.20 | 127 | 19.6 | 24.7 | +26.1 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Crown Messingjagd 0.366 154 gr | 9.30 | 154 | 17.9 | 23.2 | +29.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Crown Messingjagd 0.375 185 gr | 9.52 | 185 | 20.3 | 26.3 | +29.6 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Messingjagd 0.222 37 gr | 5.64 | 37 | 13.3 | 16.1 | +20.9 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Messingjagd 0.224 47 gr | 5.69 | 47 | 16.5 | 19.8 | +19.9 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Messingjagd 0.243 71 gr | 6.17 | 71 | 20.8 | 24.9 | +19.7 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Messingjagd 0.264 91 gr | 6.71 | 91 | 22.2 | 26.7 | +20.2 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Messingjagd 0.277 102 gr | 7.04 | 102 | 22.3 | 27.1 | +21.3 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.284 104 gr | 7.21 | 104 | 21.5 | 26.3 | +22.4 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.308 121 gr | 7.82 | 121 | 20.8 | 25.9 | +24.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.323 127 gr | 8.20 | 127 | 19.6 | 24.7 | +26.1 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.338 131 gr | 8.59 | 131 | 18.2 | 23.2 | +27.7 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.366 154 gr | 9.30 | 154 | 17.9 | 23.2 | +29.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GM | Splinter Tip Messingjagd 0.406 188 gr | 10.31 | 188 | 17.2 | 22.9 | +33.0 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| GPA | Classic 0.224 39 gr | 5.69 | 39 | 14.8 | 13.6 | -7.9 % | jacketed lead core |  | -11…+10 % |
| GPA | Classic 0.224 63 gr | 5.69 | 63 | 23.8 | 21.4 | -10.2 % | jacketed lead core |  | -11…+10 % |
| GPA | Classic 0.270 106 gr | 6.86 | 106 | 26.4 | 24.2 | -8.3 % | jacketed lead core |  | -11…+10 % |
| GPA | Classic 0.366 188 gr | 9.30 | 188 | 23.5 | 22.9 | -2.4 % | jacketed lead core |  | -11…+10 % |
| GPA | Kupfer Sport 0.408 369 gr | 10.36 | 369 | 42.6 | 39.6 | -7.0 % | monolithic copper/brass |  | -11…+12 % |
| GPA | Kupfer Sport 0.408 469 gr | 10.36 | 469 | 50.4 | 49.6 | -1.5 % | monolithic copper/brass |  | -11…+12 % |
| GPA | Messing Sport 0.338 209 gr | 8.59 | 209 | 31.3 | 33.5 | +6.9 % | monolithic copper/brass |  | -11…+12 % |
| GPA | Plus 0.308 142 gr | 7.82 | 142 | 26.3 | 24.7 | -6.1 % | jacketed lead core |  | -11…+10 % |
| GPA | Plus 0.323 154 gr | 8.20 | 154 | 25.6 | 24.3 | -5.1 % | jacketed lead core |  | -11…+10 % |
| GPA | Plus 0.366 188 gr | 9.30 | 188 | 23.5 | 22.9 | -2.4 % | jacketed lead core |  | -11…+10 % |
| H&N | RN HS .264 140 gr | 6.71 | 140 | 30.2 | 29.5 | -2.3 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Hornady | 40047B | 10.16 | 180 | 17.3 | 17.8 | +2.7 % | jacketed lead core | yes | -8…+8 % |
| Hornady | A-Tip Match 22799 | 5.69 | 76 | 26.6 | 29.1 | +9.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 2286 | 5.69 | 90 | 31.5 | 35.1 | +11.4 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 24531 | 6.17 | 110 | 33.0 | 36.1 | +9.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 245312 | 6.17 | 110 | 33.0 | 36.1 | +9.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 24568 | 6.17 | 120 | 36.0 | 39.4 | +9.4 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 2562 | 6.53 | 138 | 37.1 | 40.2 | +8.4 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 2836 | 7.21 | 166 | 36.9 | 39.2 | +6.4 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 28506 | 7.21 | 190 | 42.2 | 45.2 | +7.2 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 30741 | 7.82 | 177 | 33.7 | 35.2 | +4.6 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 33389 | 8.59 | 300 | 47.9 | 48.8 | +1.9 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 3729 | 9.52 | 390 | 51.3 | 51.7 | +0.8 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | A-Tip Match 41697 | 10.57 | 500 | 54.2 | 52.9 | -2.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | AEROMATCH 22771 | 5.69 | 69 | 24.2 | 23.4 | -3.2 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 227711 | 5.69 | 69 | 24.2 | 23.4 | -3.2 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 227771 | 5.69 | 77 | 27.0 | 26.0 | -3.7 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 24573 | 6.17 | 105 | 31.5 | 31.0 | -1.7 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 245731 | 6.17 | 105 | 31.5 | 31.0 | -1.7 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 26303 | 6.71 | 140 | 35.8 | 34.5 | -3.7 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 263031 | 6.71 | 140 | 35.8 | 34.5 | -3.7 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 30507 | 7.82 | 168 | 32.0 | 29.4 | -8.0 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 305071 | 7.82 | 168 | 32.0 | 29.4 | -8.0 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 30719 | 7.82 | 175 | 33.3 | 30.9 | -7.3 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | AEROMATCH 307191 | 7.82 | 175 | 33.3 | 30.9 | -7.3 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | CX 22402 | 5.69 | 50 | 19.1 | 20.6 | +7.8 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 22765 | 5.69 | 65 | 24.9 | 28.6 | +15.0 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 22813 | 5.69 | 70 | 26.8 | 30.7 | +14.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 243704 | 6.17 | 80 | 26.1 | 28.2 | +8.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 24444 | 6.17 | 90 | 29.4 | 33.1 | +12.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 254104 | 6.53 | 90 | 26.2 | 28.2 | +7.6 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 2543 | 6.53 | 112 | 32.6 | 37.0 | +13.6 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 261104 | 6.71 | 120 | 33.1 | 36.2 | +9.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 26111 | 6.71 | 90 | 24.8 | 28.1 | +13.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 26178 | 6.71 | 130 | 35.8 | 39.4 | +10.0 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 271904 | 7.04 | 100 | 25.0 | 26.9 | +7.6 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 273704 | 7.04 | 130 | 32.6 | 35.3 | +8.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 28284 | 7.21 | 150 | 35.7 | 38.7 | +8.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 2837 | 7.21 | 160 | 38.1 | 42.5 | +11.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 301914 | 7.82 | 110 | 22.3 | 25.1 | +12.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 301934 | 7.82 | 180 | 36.5 | 39.3 | +7.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 303704 | 7.82 | 150 | 30.4 | 33.5 | +10.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 304704 | 7.82 | 165 | 33.4 | 36.4 | +8.9 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 30738 | 7.82 | 190 | 38.5 | 42.3 | +9.9 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 332014 | 8.59 | 225 | 37.9 | 40.4 | +6.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 332704 | 8.59 | 185 | 31.2 | 33.7 | +8.0 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 33283 | 8.59 | 240 | 40.4 | 44.0 | +8.9 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | CX 37084 | 9.52 | 250 | 34.2 | 36.0 | +5.3 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | DGH Dangerous Game Handgun 35548 | 9.02 | 138 | 17.2 | 17.6 | +2.5 % | jacketed lead core | yes | -8…+8 % |
| Hornady | DGH Dangerous Game Handgun 3576 | 9.07 | 165 | 20.3 | 20.3 | +0.1 % | jacketed lead core | yes | -8…+8 % |
| Hornady | DGH Dangerous Game Handgun 40071 | 10.16 | 200 | 18.9 | 19.6 | +3.5 % | jacketed lead core | yes | -8…+8 % |
| Hornady | DGH Dangerous Game Handgun 45208 | 11.48 | 300 | 21.3 | 21.9 | +2.9 % | jacketed lead core | yes | -8…+8 % |
| Hornady | DGH Dangerous Game Handgun 50109 | 12.70 | 500 | 28.0 | 31.3 | +11.9 % | jacketed lead core |  | -11…+10 % |
| Hornady | DGS Dangerous Game Solid 4507 | 11.63 | 500 | 34.4 | 37.4 | +8.6 % | monolithic copper/brass, flat base | yes | -10…+8 % |
| Hornady | DGS Vollmantel 0.505 525 gr | 12.83 | 525 | 31.4 | 34.0 | +8.4 % | monolithic copper/brass, flat base |  | -11…+12 % |
| Hornady | DGX Bonded 37214 | 9.52 | 300 | 32.9 | 30.3 | -7.8 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 41044 | 10.41 | 400 | 35.7 | 33.7 | -5.6 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 41694 | 10.57 | 400 | 34.5 | 32.8 | -5.0 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 42404 | 10.74 | 400 | 33.2 | 31.9 | -4.0 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 45034 | 11.63 | 480 | 33.0 | 31.8 | -3.7 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 45054 | 11.63 | 500 | 34.4 | 33.0 | -4.1 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 47474 | 12.04 | 500 | 31.7 | 31.1 | -1.9 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Bonded 51504 | 12.95 | 570 | 30.4 | 30.7 | +0.9 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | DGX Verbundkern 0.505 525 gr | 12.83 | 525 | 28.7 | 31.2 | +8.8 % | jacketed lead core, flat base |  | -11…+10 % |
| Hornady | ECX 224034 | 5.69 | 50 | 17.6 | 20.6 | +17.0 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 26112 | 6.71 | 120 | 29.3 | 35.3 | +20.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 263364 | 6.71 | 140 | 34.1 | 38.7 | +13.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 28214 | 7.21 | 150 | 31.0 | 36.0 | +16.2 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 301944 | 7.82 | 125 | 21.5 | 25.5 | +18.5 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 30372 | 7.82 | 150 | 25.8 | 30.3 | +17.6 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 304714 | 7.82 | 165 | 28.4 | 33.3 | +17.3 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 32314 | 8.20 | 180 | 27.8 | 33.1 | +19.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 33272 | 8.59 | 185 | 25.8 | 33.3 | +29.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ECX 35614 | 9.30 | 250 | 29.0 | 36.0 | +24.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 22791B | 5.69 | 75 | 26.3 | 29.4 | +11.7 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 22834 | 5.69 | 88 | 30.8 | 34.1 | +10.7 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 22834B | 5.69 | 88 | 30.8 | 34.1 | +10.7 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 245301 | 6.17 | 109 | 32.7 | 35.7 | +9.2 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 24561B | 6.17 | 108 | 32.4 | 35.0 | +7.9 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 2561 | 6.53 | 134 | 36.1 | 38.8 | +7.6 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 263311 | 6.71 | 140 | 35.8 | 41.0 | +14.4 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Hornady | ELD Match Extremely Low Drag Match 26331B | 6.71 | 140 | 35.8 | 38.5 | +7.4 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 263331 | 6.71 | 147 | 37.6 | 42.9 | +14.1 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Hornady | ELD Match Extremely Low Drag Match 26333B | 6.71 | 147 | 37.6 | 40.4 | +7.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 30506B | 7.82 | 168 | 32.0 | 33.4 | +4.5 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 30713 | 7.82 | 178 | 33.9 | 35.2 | +3.9 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD Match Extremely Low Drag Match 30951 | 7.82 | 195 | 37.1 | 38.2 | +3.0 % | jacketed lead core, polymer tip, boat tail, match | yes | -8…+8 % |
| Hornady | ELD-VT 22762 | 5.69 | 62 | 21.8 | 24.7 | +13.2 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-VT 22772 | 5.69 | 69 | 24.3 | 27.1 | +11.6 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-VT 25412 | 6.53 | 95 | 24.6 | 27.9 | +13.5 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-VT 26103 | 6.71 | 100 | 24.4 | 27.9 | +14.5 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-VT 30620 | 7.82 | 174 | 30.0 | 34.9 | +16.3 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X 0.257 128 gr | 6.53 | 128 | 33.1 | 35.1 | +6.2 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 22833 | 5.69 | 80 | 28.1 | 31.1 | +10.7 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 2441 | 6.17 | 90 | 26.4 | 29.1 | +10.2 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 24550 | 6.17 | 103 | 30.2 | 33.5 | +10.9 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 25418 | 6.53 | 110 | 28.5 | 31.7 | +11.1 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 27356 | 7.04 | 145 | 31.7 | 35.5 | +12.1 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | ELD-X Extremely Low Drag - eXpanding 33371 | 8.59 | 270 | 37.6 | 44.1 | +17.2 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | FMJ 35527B | 9.02 | 100 | 12.2 | 12.8 | +4.6 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 355571 | 9.02 | 115 | 14.1 | 14.0 | -0.9 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 35557B | 9.02 | 115 | 14.1 | 14.0 | -0.9 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 355771 | 9.02 | 124 | 15.2 | 14.9 | -2.0 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 35577B | 9.02 | 124 | 15.2 | 14.9 | -2.0 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 35597B | 9.02 | 147 | 17.9 | 18.1 | +0.9 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 400471 | 10.16 | 180 | 17.3 | 17.1 | -1.2 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 451771 | 11.46 | 230 | 17.3 | 17.0 | -2.0 % | full metal jacket | yes | -8…+10 % |
| Hornady | FMJ 45177B | 11.46 | 230 | 17.3 | 17.0 | -2.0 % | full metal jacket | yes | -8…+10 % |
| Hornady | FTX .308 Marlin 0.308 160 gr | 7.82 | 160 | 27.5 | 29.7 | +8.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | FTX 0.308 160 gr | 7.82 | 160 | 27.5 | 29.7 | +8.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | FTX 0.430 265 gr | 10.92 | 265 | 21.1 | 24.9 | +18.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | FTX 0.452 225 gr | 11.48 | 225 | 16.0 | 19.4 | +21.2 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | FTX 3502 | 9.02 | 165 | 20.5 | 22.7 | +10.6 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 355281 | 9.02 | 115 | 14.3 | 14.3 | +0.2 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 35528B | 9.02 | 115 | 14.3 | 14.3 | +0.2 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 355721 | 9.04 | 125 | 15.5 | 15.7 | +1.5 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 35572B | 9.04 | 125 | 15.5 | 15.7 | +1.5 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 400421 | 10.16 | 180 | 17.0 | 17.4 | +2.3 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 40042B | 10.16 | 180 | 17.0 | 17.4 | +2.3 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 40061B | 10.16 | 200 | 18.9 | 19.5 | +3.0 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 45105B | 11.46 | 185 | 13.2 | 14.2 | +7.8 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 45159B | 11.46 | 200 | 14.3 | 15.3 | +7.1 % | jacketed lead core | yes | -8…+8 % |
| Hornady | HAP Hornady Action Pistol 45161B | 11.46 | 230 | 16.4 | 17.7 | +7.8 % | jacketed lead core | yes | -8…+8 % |
| Hornady | InterLock 2620 | 6.71 | 129 | 31.4 | 29.9 | -4.7 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 2640 | 6.71 | 160 | 39.0 | 33.0 | -15.4 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 2740 | 7.04 | 150 | 32.8 | 31.3 | -4.5 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 2850 | 7.21 | 175 | 36.2 | 34.0 | -6.1 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3070 | 7.82 | 180 | 31.0 | 30.0 | -3.1 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3120 | 7.92 | 150 | 25.1 | 24.9 | -0.9 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3140B | 7.87 | 123 | 20.9 | 20.3 | -2.7 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3501 | 9.02 | 170 | 21.2 | 20.5 | -3.5 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3520 | 9.09 | 250 | 30.5 | 29.8 | -2.3 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 3711 | 9.52 | 270 | 29.6 | 29.4 | -0.7 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | InterLock 45204 | 11.48 | 245 | 17.4 | 17.6 | +1.3 % | jacketed lead core, flat base | yes | -8…+8 % |
| Hornady | Lead Bullets 10428 | 9.09 | 158 | 18.8 | 18.0 | -4.3 % | lead | yes | -20…+4 % |
| Hornady | Lead Bullets 12458 | 11.53 | 255 | 18.8 | 17.3 | -8.1 % | lead | yes | -20…+4 % |
| Hornady | Match 2249B | 5.69 | 52 | 18.2 | 17.9 | -1.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2249C | 5.69 | 52 | 18.2 | 17.9 | -1.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2250B | 5.69 | 53 | 18.6 | 18.0 | -3.3 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 22785 | 5.69 | 68 | 23.8 | 23.6 | -0.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2278B | 5.69 | 68 | 23.8 | 23.6 | -0.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2278C | 5.69 | 68 | 23.8 | 23.6 | -0.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 22796 | 5.69 | 75 | 26.3 | 25.9 | -1.5 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2279B | 5.69 | 75 | 26.3 | 25.9 | -1.5 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2279C | 5.69 | 75 | 26.3 | 25.9 | -1.5 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 2458 | 6.17 | 105 | 31.5 | 30.9 | -1.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 24585 | 6.17 | 105 | 31.5 | 30.9 | -1.9 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 3039B | 7.82 | 155 | 29.5 | 27.3 | -7.5 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 3039C | 7.82 | 155 | 29.5 | 27.3 | -7.5 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 305016 | 7.82 | 168 | 32.0 | 29.6 | -7.6 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 30501B | 7.82 | 168 | 32.0 | 29.6 | -7.6 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 30715 | 7.82 | 178 | 33.9 | 31.7 | -6.4 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | Match 30715B | 7.82 | 178 | 33.9 | 31.7 | -6.4 % | jacketed lead core, match | yes | -8…+8 % |
| Hornady | MonoFlex 2446 | 6.17 | 90 | 26.4 | 27.5 | +4.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | Monoflex für .30-30 0.308 140 gr | 7.82 | 140 | 24.1 | 26.2 | +8.8 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Hornady | NTX 17016 | 4.37 | 15.5 | 9.7 | 12.0 | +23.6 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | NTX 22000 | 5.18 | 24 | 10.4 | 13.6 | +30.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Hornady | SST 0.270 140 gr | 6.86 | 140 | 32.5 | 34.7 | +6.9 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Hornady | SST 0.270 150 gr | 6.86 | 150 | 34.8 | 37.1 | +6.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Hornady | SST Super Shock Tip 26173 | 6.71 | 123 | 31.4 | 33.5 | +6.8 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 26202 | 6.71 | 129 | 31.4 | 34.5 | +9.9 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 2716 | 7.04 | 120 | 26.3 | 29.3 | +11.3 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 27302 | 7.04 | 130 | 28.4 | 31.9 | +12.1 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 27352 | 7.04 | 140 | 30.6 | 34.1 | +11.6 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 28202 | 7.21 | 139 | 28.8 | 32.6 | +13.0 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 28302 | 7.21 | 154 | 31.9 | 35.7 | +11.9 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 30702 | 7.82 | 180 | 31.0 | 34.8 | +12.4 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | SST Super Shock Tip 3142 | 7.87 | 123 | 20.9 | 23.7 | +13.5 % | jacketed lead core, polymer tip, boat tail | yes | -8…+8 % |
| Hornady | Sub-X 30718 | 7.82 | 175 | 30.1 | 28.7 | -4.8 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Sub-X 33394 | 8.59 | 307 | 42.8 | 39.5 | -7.7 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Sub-X 3503 | 9.07 | 250 | 30.7 | 29.2 | -5.0 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Sub-X 45031 | 11.48 | 395 | 28.0 | 28.9 | +3.4 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Sub-X 45052 | 11.63 | 410 | 28.2 | 28.9 | +2.6 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Traditional & FMJ 2263B | 5.69 | 55 | 17.0 | 18.3 | +7.5 % | full metal jacket | yes | -8…+10 % |
| Hornady | Traditional & FMJ 22671 | 5.69 | 55 | 17.0 | 18.6 | +9.6 % | full metal jacket, boat tail | yes | -8…+10 % |
| Hornady | Traditional & FMJ 22672 | 5.69 | 55 | 17.0 | 18.6 | +9.6 % | full metal jacket, boat tail | yes | -8…+10 % |
| Hornady | Traditional & FMJ 22760B | 5.69 | 62 | 19.1 | 20.3 | +6.3 % | full metal jacket | yes | -8…+10 % |
| Hornady | Traditional & FMJ 3017B | 7.82 | 110 | 17.9 | 17.7 | -1.2 % | full metal jacket | yes | -8…+10 % |
| Hornady | Traditional & FMJ 30196 | 7.82 | 125 | 20.4 | 20.6 | +1.0 % | full metal jacket | yes | -8…+10 % |
| Hornady | Traditional & FMJ 30371 | 7.82 | 150 | 24.4 | 26.2 | +7.6 % | full metal jacket, boat tail | yes | -8…+10 % |
| Hornady | Traditional & FMJ 3037B | 7.82 | 150 | 24.4 | 26.2 | +7.6 % | full metal jacket, boat tail | yes | -8…+10 % |
| Hornady | Traditional & FMJ 3147B | 7.87 | 123 | 19.7 | 20.3 | +3.2 % | full metal jacket | yes | -8…+10 % |
| Hornady | V-MAX 21710 | 4.37 | 20 | 12.5 | 13.6 | +8.5 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 2207 | 5.63 | 60 | 21.6 | 22.6 | +4.8 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22272 | 5.69 | 55 | 19.3 | 20.4 | +5.9 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22416 | 5.69 | 40 | 14.1 | 15.3 | +8.8 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22606 | 5.18 | 40 | 17.3 | 18.8 | +8.6 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22616 | 5.69 | 50 | 17.6 | 18.8 | +6.9 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22716 | 5.69 | 55 | 19.3 | 20.4 | +5.9 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | V-MAX 22721 | 7.04 | 110 | 24.1 | 26.3 | +9.2 % | jacketed lead core, polymer tip | yes | -8…+8 % |
| Hornady | Varmint 2240 | 5.69 | 50 | 17.6 | 17.2 | -2.5 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Varmint 2266 | 5.69 | 55 | 19.3 | 19.3 | -0.2 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Hornady | Varmint 2266B | 5.69 | 55 | 19.3 | 18.8 | -2.8 % | jacketed lead core | yes | -8…+8 % |
| Hornady | Vollmantel FMJ/BT (.3105") 0.303 174 gr | 7.70 | 174 | 29.3 | 30.4 | +3.9 % | full metal jacket, boat tail |  | -15…+10 % |
| Hornady | XTP Mag 45235 | 11.48 | 300 | 20.3 | 20.2 | -0.3 % | jacketed lead core, hollow point, flat base | yes | -8…+8 % |
| Hornady | XTP eXtreme Terminal Performance 45230 | 11.48 | 300 | 20.3 | 19.9 | -2.0 % | jacketed lead core, hollow point, flat base | yes | -8…+8 % |
| LOS | CEB Bleifrei 0.308 150 gr | 7.82 | 150 | 22.5 | 29.4 | +30.8 % | monolithic copper/brass |  | -11…+12 % |
| LOS | FP 0.356 145 gr | 9.04 | 145 | 15.4 | 17.3 | +12.3 % | jacketed lead core, flat nose |  | -11…+10 % |
| LOS | HP verk. Blei 0.358 180 gr | 9.09 | 180 | 14.8 | 21.5 | +45.1 % | lead, hollow point |  | -8…+11 % |
| LOS | Hunter 0.308 155 gr | 7.82 | 155 | 23.0 | 26.8 | +16.6 % | jacketed lead core |  | -11…+10 % |
| LOS | RN 0.356 100 gr | 9.04 | 100 | 10.7 | 12.1 | +12.7 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| LOS | RNFP verk. Blei 0.358 180 gr | 9.09 | 180 | 14.8 | 22.6 | +52.5 % | lead |  | -8…+11 % |
| LOS | RNFP verk. Blei 0.401 180 gr | 10.19 | 180 | 11.8 | 18.1 | +53.2 % | lead |  | -8…+11 % |
| LOS | Tactic 0.224 52 gr | 5.69 | 52 | 15.7 | 17.9 | +13.7 % | jacketed lead core |  | -11…+10 % |
| LOS | Tactic 0.308 150 gr | 7.82 | 150 | 22.3 | 26.0 | +16.6 % | jacketed lead core |  | -11…+10 % |
| LOS | Tactic 0.308 165 gr | 7.82 | 165 | 24.5 | 28.4 | +16.0 % | jacketed lead core |  | -11…+10 % |
| LOS | Vollmantel Rundkopf 0.365 93 gr | 9.27 | 93 | 10.7 | 10.3 | -3.5 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| LOS | Vollmantel Rundkopf 0.451 230 gr | 11.46 | 230 | 17.6 | 16.0 | -9.2 % | full metal jacket, round nose, flat base |  | -15…+10 % |
| Lapua | 7.83 mm (.308'') B416 | 7.83 | 200 | 32.5 | 35.5 | +9.2 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Lapua | Cutting Edge, FMJ S496 | 6.71 | 100 | 22.2 | 25.2 | +13.4 % | monolithic copper/brass | yes | -10…+8 % |
| Lapua | Cutting Edge, Open Tip G477 | 7.83 | 100 | 17.2 | 18.2 | +5.8 % | monolithic copper/brass, hollow point | yes | -10…+8 % |
| Lapua | D166 D166 | 7.87 | 200 | 32.1 | 34.4 | +7.2 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Lapua | D46 D46 | 7.83 | 185 | 30.0 | 33.3 | +10.9 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Lapua | FMJ Boat Tail B343 | 6.71 | 144 | 31.9 | 35.1 | +10.1 % | full metal jacket, boat tail | yes | -8…+10 % |
| Lapua | FMJ Boat Tail, Lock Base B476 | 7.83 | 170 | 27.6 | 29.9 | +8.4 % | full metal jacket, boat tail | yes | -8…+10 % |
| Lapua | FMJ S569 | 5.69 | 55 | 17.0 | 18.3 | +7.5 % | full metal jacket | yes | -8…+10 % |
| Lapua | Lead-free copper N509 | 6.17 | 90 | 23.2 | 28.9 | +24.6 % | monolithic copper/brass |  | -11…+12 % |
| Lapua | Lock Base B408 | 8.60 | 250 | 33.6 | 37.4 | +11.2 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Lapua | MaxRange Target GB582 | 6.71 | 144 | 35.0 | 35.8 | +2.3 % | jacketed lead core, match | yes | -8…+8 % |
| Lapua | Naturalis N558 | 7.83 | 170 | 27.2 | 33.4 | +22.7 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Lapua | Naturalis N560 | 9.30 | 250 | 28.4 | 35.0 | +23.1 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Lapua | Naturalis N564 | 7.21 | 156 | 29.5 | 34.4 | +16.6 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Lapua | Naturalis N566 | 5.69 | 50 | 15.2 | 19.7 | +29.5 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Lapua | OT G573 Hohlspitz 0.323 120 gr | 8.20 | 120 | 17.9 | 18.3 | +2.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Lapua | Open Tip G573 | 8.20 | 180 | 27.9 | 26.7 | -4.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Lapua | Open Tip G574 | 9.30 | 185 | 21.5 | 21.5 | -0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| Lapua | Open Tip G580 | 7.83 | 100 | 17.2 | 16.5 | -4.2 % | jacketed lead core, hollow point | yes | -8…+8 % |
| Lapua | Scenar-L GB542 | 6.18 | 105 | 31.4 | 31.1 | -1.0 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB543 | 6.18 | 90 | 26.9 | 27.2 | +1.1 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB544 | 5.69 | 69 | 24.2 | 24.4 | +0.6 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB545 | 5.69 | 77 | 27.0 | 27.1 | +0.2 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB546 | 6.71 | 136 | 34.7 | 34.0 | -1.9 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB547 | 6.71 | 120 | 30.6 | 30.4 | -0.6 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB550 | 7.83 | 175 | 33.3 | 31.7 | -4.8 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB551 | 7.83 | 220 | 41.8 | 39.6 | -5.3 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB552 | 7.83 | 155 | 29.5 | 28.5 | -3.3 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB553 | 7.21 | 150 | 33.4 | 32.5 | -2.7 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Scenar-L GB554 | 7.21 | 180 | 40.0 | 38.8 | -2.9 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Lapua | Soft Point E539 | 5.69 | 55 | 19.3 | 18.1 | -6.4 % | jacketed lead core | yes | -8…+8 % |
| Lapua | TRX TrueRange Expanding N583 | 6.71 | 120 | 29.2 | 27.5 | -5.7 % | jacketed lead core, hollow point | yes | -8…+8 % |
| Lapua | TRX TrueRange Expanding N584 | 7.83 | 165 | 28.3 | 28.0 | -0.9 % | jacketed lead core, hollow point | yes | -8…+8 % |
| Lapua | TRX TrueRange Expanding N585 | 7.83 | 150 | 25.8 | 25.1 | -2.9 % | jacketed lead core, hollow point | yes | -8…+8 % |
| Leader | LJG-SX Messing Hohlspitz 0.285 104 gr | 7.24 | 104 | 27.5 | 24.4 | -11.1 % | monolithic copper/brass, hollow point, relief grooves |  | -11…+12 % |
| Leader | LJG-SX Messing Hohlspitz 0.308 120 gr | 7.82 | 120 | 26.5 | 24.1 | -9.2 % | monolithic copper/brass, hollow point, relief grooves |  | -11…+12 % |
| Leader | LJG-SX Messing Hohlspitz 0.323 126 gr | 8.20 | 126 | 25.0 | 23.0 | -8.1 % | monolithic copper/brass, hollow point, relief grooves |  | -11…+12 % |
| Leader | LJG-SX Messing Hohlspitz 0.338 131 gr | 8.59 | 131 | 23.4 | 21.8 | -7.0 % | monolithic copper/brass, hollow point, relief grooves |  | -11…+12 % |
| Leader | LJG-SX Messing Hohlspitz 0.366 154 gr | 9.30 | 154 | 22.8 | 21.7 | -4.8 % | monolithic copper/brass, hollow point, relief grooves |  | -11…+12 % |
| Magtech | LSWC 0.452 200 gr | 11.48 | 200 | 15.1 | 15.3 | +1.5 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| Magtech | Teilmantel Flachkopf 0.357 158 gr | 9.07 | 158 | 21.7 | 18.1 | -16.7 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Magtech | Vollmantel 0.25 50 gr | 6.35 | 50 | 12.5 | 13.4 | +7.1 % | full metal jacket |  | -15…+10 % |
| Magtech | Vollmantel 0.307 110 gr | 7.80 | 110 | 18.1 | 18.8 | +3.9 % | full metal jacket |  | -15…+10 % |
| Missouri | Bullet Co. TCFP IDP #7 Blei 0.401 140 gr | 10.19 | 140 | 13.3 | 14.3 | +7.4 % | lead |  | -8…+11 % |
| Norma | Alaska 20665321 | 6.71 | 156 | 38.0 | 33.2 | -12.7 % | jacketed lead core | yes | -8…+8 % |
| Norma | Alaska 20676481 | 7.82 | 180 | 31.0 | 28.6 | -7.8 % | jacketed lead core | yes | -8…+8 % |
| Norma | Alaska 20693161 | 9.30 | 285 | 33.1 | 32.6 | -1.5 % | jacketed lead core | yes | -8…+8 % |
| Norma | Bondstrike 20665651 | 6.71 | 143 | 36.5 | 36.2 | -0.8 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Norma | Bondstrike 20670171 | 7.21 | 165 | 36.7 | 36.1 | -1.8 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Norma | Bondstrike 20676871 | 7.82 | 180 | 34.3 | 33.7 | -1.9 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Norma | Diamond Line 10660162 | 6.17 | 105 | 31.5 | 31.6 | +0.4 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Norma | Diamond Line 10665081 | 6.71 | 130 | 33.2 | 32.9 | -0.9 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Norma | Ecostrike 20665701 | 6.71 | 120 | 30.7 | 33.0 | +7.6 % | monolithic copper/brass, boat tail |  | -11…+12 % |
| Norma | Ecostrike 20676941 | 7.82 | 150 | 28.6 | 30.6 | +7.0 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Norma | Ecostrike 20677071 | 7.82 | 165 | 31.4 | 33.7 | +7.3 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Norma | Ecostrike 20680121 | 8.20 | 160 | 27.9 | 29.5 | +5.7 % | monolithic copper/brass, boat tail | yes | -10…+8 % |
| Norma | FMJ 20665141 | 6.71 | 120 | 26.7 | 27.9 | +4.4 % | full metal jacket | yes | -8…+10 % |
| Norma | FMJ 20676511 | 7.82 | 150 | 24.4 | 25.8 | +5.8 % | full metal jacket | yes | -8…+10 % |
| Norma | FMJ 20676512 | 7.82 | 150 | 24.4 | 25.8 | +5.8 % | full metal jacket | yes | -8…+10 % |
| Norma | FMJ 20680141 | 8.20 | 123 | 18.1 | 18.1 | -0.2 % | full metal jacket | yes | -8…+10 % |
| Norma | FMJ 20693061 | 9.30 | 232 | 26.7 | 25.6 | -4.2 % | full metal jacket | yes | -8…+10 % |
| Norma | GTX 10657401 | 5.69 | 69 | 24.2 | 30.4 | +25.4 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10665091 | 6.71 | 130 | 33.2 | 40.1 | +20.8 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10665201 | 6.71 | 100 | 25.6 | 30.4 | +18.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10665681 | 6.71 | 143 | 36.5 | 44.2 | +21.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10676861 | 7.82 | 155 | 29.5 | 33.9 | +14.9 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10677031 | 7.82 | 168 | 32.0 | 36.8 | +15.1 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 10677061 | 7.82 | 175 | 33.3 | 39.4 | +18.3 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 20665201 | 6.71 | 100 | 25.6 | 30.4 | +18.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 20665202 | 6.71 | 100 | 25.6 | 30.4 | +18.7 % | monolithic copper/brass, polymer tip, boat tail | yes | -10…+8 % |
| Norma | GTX 20680401 | 8.20 | 180 | 31.3 | 35.2 | +12.4 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Norma | GTX 20693271 | 9.30 | 250 | 34.4 | 37.5 | +9.0 % | monolithic copper/brass, polymer tip, boat tail |  | -11…+12 % |
| Norma | Golden Target GTX HPBT Match 0.338 250 gr | 8.59 | 250 | 39.9 | 45.2 | +13.4 % | monolithic copper/brass, polymer tip, boat tail, match |  | -11…+12 % |
| Norma | Oryx 0.277 150 gr | 7.04 | 150 | 32.8 | 31.9 | -2.8 % | jacketed lead core |  | -11…+10 % |
| Norma | Oryx 0.338 230 gr | 8.59 | 230 | 32.0 | 32.3 | +1.1 % | jacketed lead core |  | -11…+10 % |
| Norma | Oryx 0.358 250 gr | 9.09 | 250 | 30.5 | 31.3 | +2.7 % | jacketed lead core |  | -11…+10 % |
| Norma | Oryx 0.375 300 gr | 9.52 | 300 | 32.9 | 34.0 | +3.3 % | jacketed lead core |  | -11…+10 % |
| Norma | Oryx 20657131 | 5.69 | 55 | 19.3 | 18.1 | -6.4 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20660501 | 6.17 | 100 | 29.3 | 26.7 | -9.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20665241 | 6.71 | 156 | 38.0 | 34.4 | -9.4 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20670041 | 7.21 | 156 | 32.3 | 30.4 | -6.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20670051 | 7.21 | 170 | 35.2 | 32.4 | -8.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20676161 | 7.82 | 165 | 28.4 | 27.8 | -2.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20676391 | 7.82 | 200 | 34.4 | 32.4 | -5.8 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20676441 | 7.82 | 180 | 31.0 | 30.1 | -3.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20680011 | 8.20 | 196 | 30.3 | 29.4 | -2.9 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20693071 | 9.30 | 232 | 26.9 | 26.4 | -1.7 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20693081 | 9.30 | 286 | 33.2 | 32.6 | -1.9 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx 20693121 | 9.30 | 325 | 37.7 | 36.4 | -3.4 % | jacketed lead core | yes | -8…+8 % |
| Norma | Oryx Bonded SP 0.338 250 gr | 8.59 | 250 | 34.8 | 35.0 | +0.5 % | jacketed lead core |  | -11…+10 % |
| Norma | Soft Point 20657011 | 5.69 | 50 | 17.6 | 16.8 | -4.7 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 0.277 140 gr | 7.04 | 140 | 30.6 | 29.9 | -2.3 % | jacketed lead core |  | -11…+10 % |
| Norma | Tipstrike 20657001 | 5.69 | 55 | 19.3 | 18.9 | -2.2 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 20660141 | 6.17 | 76 | 22.3 | 22.3 | +0.1 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 20670141 | 7.21 | 160 | 33.1 | 33.1 | +0.1 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 20676891 | 7.82 | 170 | 29.3 | 29.9 | +2.0 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 20681001 | 8.20 | 180 | 27.8 | 27.9 | +0.4 % | jacketed lead core | yes | -8…+8 % |
| Norma | Tipstrike 20694001 | 9.30 | 255 | 29.6 | 30.6 | +3.5 % | jacketed lead core | yes | -8…+8 % |
| Norma | Vulkan 20665351 | 6.71 | 156 | 38.0 | 34.5 | -9.2 % | jacketed lead core | yes | -8…+8 % |
| Norma | Vulkan 20670061 | 7.21 | 170 | 35.2 | 32.8 | -6.8 % | jacketed lead core | yes | -8…+8 % |
| Norma | Vulkan 20676531 | 7.82 | 180 | 31.0 | 29.5 | -4.8 % | jacketed lead core | yes | -8…+8 % |
| Norma | Vulkan 20680201 | 8.20 | 196 | 30.3 | 29.6 | -2.2 % | jacketed lead core | yes | -8…+8 % |
| Norma | Vulkan 20693051 | 9.30 | 232 | 26.9 | 26.6 | -1.1 % | jacketed lead core | yes | -8…+8 % |
| North Fork | Technologies FPS Voll 0.308 200 gr | 7.82 | 200 | 38.2 | 38.6 | +0.9 % | monolithic copper/brass |  | -11…+12 % |
| North Fork | Technologies SS Verbundkern 0.284 160 gr | 7.21 | 160 | 36.7 | 36.6 | -0.2 % | monolithic copper/brass |  | -11…+12 % |
| Nosler | AccuBond 0.277 110 gr | 7.04 | 110 | 24.1 | 26.3 | +9.3 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Nosler | AccuBond Long Range Spitzer 0.277 165 gr | 7.04 | 165 | 36.1 | 38.5 | +6.7 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Nosler | Ballistic Tip 0.270 150 gr | 6.86 | 150 | 34.8 | 37.1 | +6.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Nosler | Ballistic Tip 0.277 170 gr | 7.04 | 170 | 37.2 | 39.6 | +6.5 % | jacketed lead core, polymer tip, boat tail |  | -11…+10 % |
| Nosler | E-Tip bleifrei 0.277 85 gr | 7.04 | 85 | 21.3 | 21.2 | -0.5 % | monolithic copper/brass |  | -11…+12 % |
| Nosler | HPBT RDF 0.243 115 gr | 6.17 | 115 | 34.5 | 35.1 | +1.6 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Nosler | JHP Sporting Handgun Hohlspitz 0.429 200 gr | 10.90 | 200 | 15.3 | 17.0 | +11.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Nosler | Varmageddon Polymerspitze 0.264 90 gr | 6.71 | 90 | 21.9 | 23.3 | +6.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| PPU | FMJ B-014 | 9.03 | 94 | 11.4 | 12.2 | +7.1 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-015 | 7.82 | 71 | 11.5 | 12.4 | +7.9 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-016 | 6.35 | 50 | 12.4 | 13.4 | +7.9 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-102 | 9.26 | 93 | 10.8 | 11.5 | +6.5 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-164 | 9.11 | 108 | 12.9 | 13.7 | +5.9 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-180 | 11.43 | 230 | 17.4 | 17.9 | +2.7 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-214 | 11.43 | 200 | 15.2 | 15.7 | +3.1 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-347 | 9.03 | 145 | 17.6 | 18.3 | +4.1 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-354 | 9.00 | 130 | 15.9 | 16.6 | +4.7 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-398 | 9.00 | 95 | 11.6 | 12.4 | +6.9 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-457 | 9.00 | 158 | 19.3 | 20.0 | +3.6 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ B-613 | 9.03 | 130 | 15.8 | 16.5 | +4.7 % | full metal jacket |  | -15…+10 % |
| PPU | FMJ BT B-141 | 7.21 | 175 | 33.5 | 34.8 | +3.8 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-265 | 9.52 | 301 | 32.9 | 33.7 | +2.4 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-417 | 8.31 | 200 | 28.8 | 29.9 | +3.8 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-421 | 8.59 | 249 | 33.6 | 34.4 | +2.5 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-424 | 8.20 | 123 | 18.1 | 19.5 | +7.5 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-436 | 7.82 | 150 | 24.4 | 25.7 | +5.3 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-471 | 8.20 | 252 | 37.3 | 38.1 | +2.2 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-484 | 6.71 | 110 | 24.4 | 25.9 | +6.0 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-564 | 8.18 | 200 | 29.8 | 30.8 | +3.5 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-603 | 7.04 | 115 | 23.2 | 24.6 | +5.9 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT B-605 | 6.81 | 139 | 29.9 | 31.3 | +4.7 % | full metal jacket, boat tail |  | -15…+10 % |
| PPU | FMJ BT match B-348 | 8.18 | 198 | 29.5 | 33.2 | +12.6 % | full metal jacket, boat tail, match |  | -15…+10 % |
| PPU | FMJ BT match B-498 | 8.20 | 250 | 36.9 | 41.1 | +11.5 % | full metal jacket, boat tail, match |  | -15…+10 % |
| PPU | FMJ BT match B-500 | 7.82 | 175 | 28.5 | 32.3 | +13.3 % | full metal jacket, boat tail, match |  | -15…+10 % |
| PPU | FMJ BT match B-529 | 7.90 | 182 | 29.0 | 32.8 | +13.2 % | full metal jacket, boat tail, match |  | -15…+10 % |
| PPU | FMJ BT match B-541 | 8.18 | 200 | 29.8 | 33.5 | +12.5 % | full metal jacket, boat tail, match |  | -15…+10 % |
| PPU | FMS B-111 | 9.03 | 158 | 19.6 | 20.6 | +5.3 % | jacketed lead core |  | -11…+10 % |
| PPU | FPJ B-191 | 9.00 | 125 | 15.6 | 16.0 | +2.9 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-221 | 10.88 | 180 | 14.5 | 15.6 | +7.7 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-222 | 10.88 | 240 | 19.3 | 20.5 | +6.0 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-339 | 9.03 | 158 | 19.6 | 19.9 | +1.3 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-444 | 9.52 | 301 | 33.0 | 32.8 | -0.6 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-470 | 7.80 | 98 | 17.0 | 16.9 | -0.8 % | full metal jacket |  | -15…+10 % |
| PPU | FPJ B-614 | 10.15 | 165 | 15.6 | 16.5 | +5.6 % | full metal jacket |  | -15…+10 % |
| PPU | GROM 0.323 185 gr | 8.20 | 185 | 28.6 | 28.9 | +0.9 % | jacketed lead core |  | -11…+10 % |
| PPU | HP BT B-423 | 9.65 | 250 | 32.1 | 27.2 | -15.2 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT B-485 | 6.71 | 120 | 30.7 | 27.7 | -9.7 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT B-535 | 5.69 | 69 | 24.2 | 22.7 | -6.0 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT B-592 | 7.82 | 175 | 33.3 | 29.3 | -11.9 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT B-604 | 7.04 | 115 | 26.8 | 24.3 | -9.4 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT B-616 | 5.69 | 55 | 19.3 | 18.4 | -4.8 % | jacketed lead core, hollow point, boat tail |  | -11…+10 % |
| PPU | HP BT match B-399 | 5.69 | 69 | 24.2 | 25.4 | +5.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-495 | 5.69 | 75 | 26.3 | 27.5 | +4.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-496 | 7.82 | 168 | 32.0 | 31.6 | -1.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-497 | 7.82 | 190 | 36.2 | 35.4 | -2.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-499 | 7.21 | 150 | 33.3 | 33.2 | -0.2 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-540 | 6.71 | 120 | 30.7 | 31.0 | +1.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-583 | 8.59 | 250 | 39.9 | 38.2 | -4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-593 | 7.82 | 175 | 33.3 | 32.8 | -1.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | HP BT match B-625 | 7.82 | 155 | 29.5 | 29.3 | -0.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| PPU | JHP B-145 | 9.03 | 94 | 11.2 | 12.1 | +7.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-154 | 10.88 | 180 | 13.8 | 15.4 | +11.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-156 | 10.88 | 240 | 18.4 | 20.2 | +9.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-163 | 9.00 | 147 | 17.7 | 18.5 | +4.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-171 | 9.00 | 124 | 14.9 | 15.7 | +5.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-172 | 9.26 | 95 | 10.7 | 11.6 | +8.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-174 | 7.79 | 71 | 11.9 | 12.3 | +3.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | JHP B-175 | 7.82 | 71 | 11.8 | 12.3 | +3.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | LRN B-081 | 7.94 | 98 | 15.1 | 16.6 | +10.0 % | lead |  | -8…+11 % |
| PPU | LRN B-089 | 9.12 | 145 | 16.9 | 18.3 | +8.4 % | lead |  | -8…+11 % |
| PPU | LRN B-192 | 9.04 | 124 | 14.7 | 16.1 | +9.4 % | lead |  | -8…+11 % |
| PPU | LRN B-202 | 10.45 | 230 | 20.4 | 21.7 | +6.2 % | lead |  | -8…+11 % |
| PPU | SJ FP B-448 | 11.63 | 405 | 27.9 | 27.8 | -0.5 % | jacketed lead core, flat nose |  | -11…+10 % |
| PPU | SJFP B-320 | 10.88 | 300 | 24.1 | 24.9 | +3.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | SJFP B-338 | 9.03 | 158 | 19.6 | 19.6 | +0.1 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | SJFP B-359 | 10.88 | 240 | 19.3 | 20.2 | +4.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | SJHP B-135 | 9.03 | 158 | 18.8 | 19.6 | +4.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | SJHP B-181 | 11.43 | 185 | 12.7 | 14.4 | +13.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| PPU | SJSP B-329 | 9.00 | 130 | 16.2 | 17.3 | +6.8 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-132 | 5.69 | 55 | 19.3 | 18.8 | -2.5 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-462 | 8.18 | 139 | 21.6 | 22.2 | +2.7 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-478 | 7.21 | 139 | 28.8 | 28.3 | -1.6 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-483 | 6.71 | 123 | 30.0 | 29.1 | -3.1 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-558 | 7.21 | 173 | 35.8 | 34.8 | -2.8 % | jacketed lead core |  | -11…+10 % |
| PPU | SP B-561 | 8.38 | 208 | 30.6 | 30.9 | +0.9 % | jacketed lead core |  | -11…+10 % |
| PPU | SP RN B-267 | 9.52 | 300 | 32.9 | 30.5 | -7.2 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| PPU | SP RN B-352 | 6.81 | 123 | 29.0 | 25.4 | -12.5 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| PPU | SWC B-140 | 9.08 | 158 | 19.5 | 19.4 | -0.6 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| PPU | SWC B-326 | 9.08 | 158 | 19.5 | 19.4 | -0.6 % | lead, semi-wadcutter, flat base |  | -8…+11 % |
| PPU | SWC HP B-136 | 9.08 | 158 | 19.5 | 18.4 | -5.5 % | lead, semi-wadcutter, hollow point, flat base |  | -8…+11 % |
| PPU | Solid match B-429 | 12.70 | 725 | 56.0 | 50.9 | -9.1 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| PPU | Solid match B-594 | 7.82 | 156 | 29.7 | 30.9 | +4.1 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| PPU | TMJ B-044 | 9.00 | 115 | 14.1 | 14.8 | +5.3 % | full metal jacket |  | -15…+10 % |
| PPU | TMJ B-297 | 9.00 | 124 | 15.2 | 15.9 | +4.8 % | full metal jacket |  | -15…+10 % |
| PPU | TMJ B-303 | 11.43 | 230 | 17.4 | 17.9 | +2.7 % | full metal jacket |  | -15…+10 % |
| PPU | TMJ B-332 | 9.03 | 158 | 19.2 | 19.9 | +3.5 % | full metal jacket |  | -15…+10 % |
| PPU | TMJ B-455 | 9.00 | 147 | 18.0 | 18.7 | +3.8 % | full metal jacket |  | -15…+10 % |
| PPU | TMJ B-574 | 7.82 | 71 | 11.5 | 12.4 | +7.9 % | full metal jacket |  | -15…+10 % |
| PPU | Vollmantel 0.308 93 gr | 7.82 | 93 | 15.1 | 16.0 | +5.8 % | full metal jacket |  | -15…+10 % |
| PPU | WC B-017 | 9.08 | 158 | 18.2 | 19.9 | +9.1 % | lead, wadcutter, flat base |  | -8…+11 % |
| PPU | WC B-053 | 9.08 | 148 | 17.0 | 18.7 | +9.9 % | lead, wadcutter, flat base |  | -8…+11 % |
| PPU | WC B-298 | 7.94 | 98 | 14.7 | 16.5 | +12.0 % | lead, wadcutter, flat base |  | -8…+11 % |
| RWS | Doppelkern Dk 2146002 | 9.30 | 225 | 26.1 | 27.1 | +4.0 % | jacketed lead core |  | -11…+10 % |
| RWS | Doppelkern Dk 2146045 | 7.82 | 165 | 28.4 | 28.4 | +0.1 % | jacketed lead core |  | -11…+10 % |
| RWS | Doppelkern Dk 2311463 | 6.71 | 140 | 34.1 | 32.8 | -3.8 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution 2315959 | 7.82 | 184 | 31.7 | 31.5 | -0.7 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution 2315960 | 7.21 | 159 | 32.9 | 32.2 | -2.3 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution 2317411 | 8.20 | 200 | 30.9 | 31.0 | +0.5 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution 2317412 | 9.30 | 291 | 33.8 | 34.6 | +2.2 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution 2416959 | 7.04 | 154 | 33.7 | 32.7 | -3.0 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2318520 | 7.21 | 127 | 26.3 | 26.0 | -1.0 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2318522 | 8.20 | 139 | 21.5 | 22.1 | +2.6 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2318523 | 9.30 | 184 | 21.3 | 22.5 | +5.5 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2407446 | 6.71 | 93 | 22.7 | 22.4 | -1.5 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2416960 | 7.04 | 96 | 21.0 | 21.0 | -0.1 % | jacketed lead core |  | -11…+10 % |
| RWS | Evolution Green 2418119 | 7.82 | 139 | 23.9 | 24.2 | +1.3 % | jacketed lead core |  | -11…+10 % |
| RWS | HIT 0.284 140 gr | 7.21 | 140 | 29.0 | 28.5 | -1.6 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 0.366 250 gr | 9.30 | 250 | 29.0 | 30.0 | +3.3 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2416965 | 10.49 | 200 | 17.5 | 19.2 | +9.9 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2422791 | 7.82 | 150 | 25.8 | 26.0 | +0.8 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2426874 | 7.82 | 165 | 28.4 | 28.4 | +0.1 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2426876 | 8.20 | 160 | 24.7 | 25.2 | +2.0 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2434115 | 6.71 | 120 | 29.3 | 28.4 | -3.1 % | jacketed lead core |  | -11…+10 % |
| RWS | Hit 2434116 | 7.21 | 130 | 26.9 | 26.6 | -1.0 % | jacketed lead core |  | -11…+10 % |
| RWS | Id Classic 2145480 | 7.82 | 150 | 25.8 | 28.0 | +8.4 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | KS 0.366 247 gr | 9.30 | 247 | 28.7 | 29.6 | +3.2 % | jacketed lead core |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146231 | 5.69 | 74 | 26.0 | 22.6 | -13.0 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146258 | 7.21 | 162 | 33.5 | 29.8 | -11.1 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146290 | 7.21 | 123 | 25.4 | 23.0 | -9.4 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146304 | 7.82 | 150 | 25.8 | 23.7 | -8.3 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146371 | 7.82 | 165 | 28.4 | 25.9 | -8.9 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146444 | 8.20 | 180 | 27.8 | 25.6 | -7.9 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146452 | 8.53 | 247 | 34.9 | 31.9 | -8.6 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Kegelspitz Ks 2146460 | 6.71 | 127 | 31.0 | 27.3 | -12.1 % | jacketed lead core, flat nose |  | -11…+10 % |
| RWS | Match Jagd 2145782 | 5.69 | 46 | 16.1 | 18.3 | +13.4 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| RWS | Match Jagd 2146843 | 5.69 | 46 | 16.1 | 18.3 | +13.4 % | jacketed lead core, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2313343 | 5.69 | 52 | 18.2 | 19.5 | +7.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2416966 | 7.82 | 168 | 32.0 | 31.6 | -1.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2418598 | 7.82 | 155 | 29.5 | 29.3 | -0.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2418599 | 7.82 | 175 | 33.3 | 32.8 | -1.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2418600 | 6.71 | 130 | 33.2 | 33.4 | +0.7 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2421464 | 7.82 | 190 | 36.2 | 35.4 | -2.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2421466 | 8.59 | 250 | 39.9 | 38.2 | -4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2421639 | 5.69 | 69 | 24.2 | 25.4 | +5.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Scorion Hpbt-Match 2426660 | 6.71 | 143 | 36.5 | 36.5 | +0.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| RWS | Speed Tip Professional 2411035 | 7.82 | 165 | 28.4 | 30.6 | +7.7 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Speed Tip Professional 2411037 | 7.21 | 150 | 31.0 | 32.8 | +5.7 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Speed Tip Professional 2411038 | 8.59 | 250 | 34.8 | 37.6 | +8.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Speed Tip Professional 2416962 | 8.20 | 180 | 27.8 | 30.3 | +8.9 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Speed Tip Professional 2416967 | 7.04 | 140 | 30.6 | 32.2 | +5.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Speed Tip Professional 2423741 | 6.71 | 93 | 22.7 | 24.1 | +6.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| RWS | Tmr 2145774 | 9.30 | 285 | 39.2 | 34.8 | -11.2 % | jacketed lead core, boat tail |  | -11…+10 % |
| RWS | Tms 2145642 | 5.79 | 71 | 24.0 | 23.7 | -1.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| RWS | Tms 2145790 | 5.69 | 63 | 22.1 | 22.0 | -0.7 % | jacketed lead core, boat tail |  | -11…+10 % |
| RWS | Tms 2145960 | 5.69 | 55 | 19.3 | 19.3 | +0.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| RWS | Uni Classic 2145499 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Remington | Core-Lokt 29-22733 | 6.53 | 100 | 25.9 | 24.5 | -5.4 % | jacketed lead core, flat base |  | -11…+10 % |
| Remington | Core-Lokt 29-22756 | 7.21 | 150 | 31.0 | 29.6 | -4.6 % | jacketed lead core, flat base |  | -11…+10 % |
| Remington | Core-Lokt 29-22772 | 7.82 | 150 | 25.8 | 25.2 | -2.2 % | jacketed lead core, flat base |  | -11…+10 % |
| Remington | Core-Lokt 29-22788 | 7.82 | 180 | 31.0 | 30.0 | -3.4 % | jacketed lead core, flat base |  | -11…+10 % |
| Remington | Core-Lokt 29-22920 | 6.17 | 100 | 29.3 | 27.4 | -6.6 % | jacketed lead core, flat base |  | -11…+10 % |
| Remington | Remington Component 29-22742 | 7.04 | 115 | 25.2 | 24.9 | -1.4 % | jacketed lead core |  | -11…+10 % |
| Remington | Remington Component 29-22930 | 6.17 | 80 | 23.5 | 22.9 | -2.8 % | jacketed lead core |  | -11…+10 % |
| Remington | Remington Component 29-22958 | 5.66 | 50 | 17.7 | 17.4 | -1.7 % | jacketed lead core |  | -11…+10 % |
| Remington | Remington Component 29-22960 | 5.66 | 55 | 19.5 | 19.0 | -2.5 % | jacketed lead core |  | -11…+10 % |
| Remington | Remington Component 29-22962 | 5.66 | 55 | 19.5 | 19.0 | -2.5 % | jacketed lead core |  | -11…+10 % |
| Remington | Remington Component 29-22968 | 5.66 | 62 | 22.0 | 21.3 | -3.3 % | jacketed lead core |  | -11…+10 % |
| Sako | Gamehead 0.265 140 gr | 6.73 | 140 | 33.8 | 33.0 | -2.4 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead 0.285 120 gr | 7.24 | 120 | 24.6 | 24.5 | -0.3 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead 0.285 140 gr | 7.24 | 140 | 28.7 | 28.5 | -0.8 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead 0.312 123 gr | 7.92 | 123 | 20.6 | 20.3 | -1.3 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead 0.324 150 gr | 8.23 | 150 | 23.0 | 23.7 | +3.1 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead 0.324 174 gr | 8.23 | 174 | 26.7 | 27.2 | +1.7 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Pro 0.264 130 gr | 6.71 | 130 | 31.7 | 31.6 | -0.4 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Pro 0.265 130 gr | 6.73 | 130 | 31.4 | 31.4 | +0.0 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Pro 0.265 140 gr | 6.73 | 140 | 33.8 | 33.8 | -0.1 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Pro 0.285 150 gr | 7.24 | 150 | 30.8 | 31.6 | +2.7 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Varmint 0.224 40 gr | 5.69 | 40 | 14.1 | 14.2 | +0.8 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Varmint 0.224 49 gr | 5.69 | 49 | 17.2 | 17.3 | +0.5 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Varmint 0.243 69 gr | 6.17 | 69 | 20.2 | 20.5 | +1.7 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Varmint Rx 0.224 52 gr | 5.69 | 52 | 18.3 | 16.9 | -7.8 % | jacketed lead core | yes | -8…+8 % |
| Sako | Gamehead Varmint Rx 0.309 130 gr | 7.85 | 130 | 22.2 | 21.4 | -3.4 % | jacketed lead core | yes | -8…+8 % |
| Sako | Hammerhead 0.285 170 gr | 7.24 | 170 | 37.5 | 33.7 | -10.1 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Hammerhead 0.309 181 gr | 7.85 | 181 | 34.3 | 28.6 | -16.6 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Hammerhead 0.309 201 gr | 7.85 | 201 | 38.1 | 33.0 | -13.4 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Hammerhead 0.324 201 gr | 8.23 | 201 | 34.8 | 29.4 | -15.6 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Hammerhead Teilmantel 0.365 285 gr | 9.27 | 285 | 33.3 | 34.1 | +2.4 % | jacketed lead core |  | -11…+10 % |
| Sako | Powerhead Blade 0.243 80 gr | 6.17 | 80 | 23.5 | 27.8 | +18.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Sako | Powerhead Blade 0.264 120 gr | 6.71 | 120 | 29.3 | 34.2 | +16.6 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade 0.265 120 gr | 6.73 | 120 | 29.0 | 34.0 | +17.2 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade 0.285 140 gr | 7.24 | 140 | 28.7 | 34.2 | +19.3 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade 0.308 168 gr | 7.82 | 168 | 28.9 | 35.2 | +21.9 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Sako | Powerhead Blade 0.309 162 gr | 7.85 | 162 | 27.7 | 34.4 | +24.1 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade 0.323 180 gr | 8.20 | 180 | 27.8 | 34.3 | +23.3 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Sako | Powerhead Blade 0.366 230 gr | 9.30 | 230 | 26.7 | 33.8 | +26.5 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Sako | Powerhead Blade 0.375 270 gr | 9.52 | 270 | 29.6 | 37.5 | +26.7 % | monolithic copper/brass, polymer tip |  | -11…+12 % |
| Sako | Powerhead Blade Pro 0.265 123 gr | 6.73 | 123 | 29.7 | 35.8 | +20.7 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Pro 0.278 120 gr | 7.06 | 120 | 26.0 | 31.7 | +22.1 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Pro 0.285 140 gr | 7.24 | 140 | 28.7 | 35.3 | +23.1 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Pro 0.309 162 gr | 7.85 | 162 | 27.7 | 35.2 | +26.9 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Pro 0.309 174 gr | 7.85 | 174 | 29.7 | 37.1 | +25.1 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Pro 0.366 201 gr | 9.30 | 201 | 23.3 | 29.8 | +27.7 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Solid 0.224 40 gr | 5.69 | 40 | 14.1 | 15.5 | +10.2 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Powerhead Blade Solid 0.376 261 gr | 9.55 | 261 | 28.5 | 35.8 | +25.6 % | monolithic copper/brass, polymer tip | yes | -10…+8 % |
| Sako | Speedhead 0.224 49 gr | 5.69 | 49 | 17.2 | 17.0 | -1.1 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Speedhead 0.264 144 gr | 6.71 | 144 | 36.8 | 35.5 | -3.6 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Speedhead 0.265 144 gr | 6.73 | 144 | 36.5 | 35.3 | -3.3 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Speedhead 0.309 123 gr | 7.85 | 123 | 23.3 | 21.9 | -6.1 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Speedhead FMJ 0.284 78 gr | 7.21 | 78 | 14.9 | 15.9 | +6.5 % | full metal jacket |  | -15…+10 % |
| Sako | Super Hammerhead 0.264 140 gr | 6.71 | 140 | 35.8 | 33.7 | -6.0 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Super Hammerhead 0.265 140 gr | 6.73 | 140 | 35.5 | 33.5 | -5.7 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Super Hammerhead 0.278 130 gr | 7.06 | 130 | 30.1 | 28.8 | -4.2 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Super Hammerhead 0.309 150 gr | 7.85 | 150 | 28.4 | 25.4 | -10.6 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Super Hammerhead 0.309 181 gr | 7.85 | 181 | 34.3 | 30.6 | -10.7 % | jacketed lead core, boat tail | yes | -8…+8 % |
| Sako | Trg Defense 0.224 63 gr | 5.69 | 63 | 22.1 | 20.0 | -9.5 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sako | Trg Defense 0.309 221 gr | 7.85 | 221 | 41.8 | 39.2 | -6.3 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sako | Trg Precision 0.264 136 gr | 6.71 | 136 | 34.8 | 33.8 | -2.7 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sako | Trg Precision 0.265 136 gr | 6.73 | 136 | 34.5 | 33.7 | -2.4 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sako | Trg Precision 0.265 140 gr | 6.73 | 140 | 35.5 | 34.5 | -2.7 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sako | Trg Precision 0.309 174 gr | 7.85 | 174 | 32.9 | 31.2 | -5.1 % | jacketed lead core, boat tail, match | yes | -8…+8 % |
| Sax | KJG-BCS 0.375 215 gr | 9.52 | 215 | 30.6 | 29.8 | -2.5 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-BCS 0.416 245 gr | 10.57 | 245 | 27.4 | 27.5 | +0.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-BCS 0.458 292 gr | 11.63 | 292 | 26.1 | 26.9 | +3.2 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-BCS 0.577 573 gr | 14.66 | 573 | 29.5 | 32.3 | +9.5 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-BCS 0.600 636 gr | 15.24 | 636 | 29.8 | 33.0 | +10.9 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-BCS-Solid 0.500 525 gr | 12.70 | 525 | 38.1 | 39.4 | +3.3 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-HS 0.224 37 gr | 5.69 | 37 | 16.9 | 15.6 | -7.9 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-HSR 0.224 46 gr | 5.69 | 46 | 21.0 | 19.1 | -9.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-S (Graubünden) 0.413 201 gr | 10.49 | 201 | 22.9 | 23.2 | +1.3 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-SR 0.359 154 gr | 9.12 | 154 | 24.2 | 23.7 | -2.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-SR 0.366 154 gr | 9.30 | 154 | 23.2 | 22.8 | -1.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KJG-SR 0.375 154 gr | 9.52 | 154 | 21.9 | 21.8 | -0.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | KSG LM-105 0.338 293 gr | 8.59 | 293 | 53.0 | 40.6 | -23.4 % | jacketed lead core |  | -11…+10 % |
| Sax | MJG-HSX Bleifrei 0.375 185 gr | 9.52 | 185 | 27.1 | 25.9 | -4.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.285 105 gr | 7.24 | 105 | 26.7 | 25.9 | -2.9 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.308 121 gr | 7.82 | 121 | 26.3 | 25.5 | -3.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.318 132 gr | 8.08 | 132 | 26.9 | 26.0 | -3.5 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.323 127 gr | 8.20 | 127 | 25.1 | 24.3 | -3.1 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.338 130 gr | 8.59 | 130 | 23.5 | 22.7 | -3.3 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.366 159 gr | 9.30 | 159 | 24.5 | 23.5 | -4.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MJG-SX Bleifrei 0.375 161 gr | 9.52 | 161 | 23.6 | 22.7 | -3.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Sax | MSG FK SO (.50 BMG) 0.50 803 gr | 12.70 | 803 | 58.2 | 48.9 | -16.1 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | EDGE V338892 | 6.71 | 120 | 29.3 | 28.4 | -3.1 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | EDGE V339342 | 7.21 | 150 | 31.0 | 30.4 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | EDGE V339532 | 8.20 | 180 | 27.8 | 28.1 | +1.2 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | EDGE V339592 | 7.82 | 165 | 28.4 | 28.4 | +0.1 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | EDGE V339642 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | FMJ V318622 | 9.07 | 158 | 19.1 | 19.7 | +3.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V318722 | 9.07 | 158 | 19.1 | 19.7 | +3.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V318822 | 10.16 | 180 | 17.3 | 17.8 | +3.2 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V318842 | 10.16 | 165 | 15.8 | 16.4 | +4.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V318872 | 11.48 | 230 | 17.3 | 17.7 | +2.4 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319022 | 6.38 | 50 | 12.3 | 13.3 | +7.8 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319072 | 7.85 | 73 | 11.8 | 12.6 | +7.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319122 | 9.02 | 92 | 11.2 | 12.0 | +7.0 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319172 | 9.02 | 115 | 14.1 | 14.8 | +4.8 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319222 | 9.02 | 124 | 15.2 | 15.9 | +4.3 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319322 | 9.02 | 140 | 17.1 | 17.8 | +3.9 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319342 | 9.02 | 150 | 18.4 | 19.0 | +3.0 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319372 | 9.22 | 95 | 11.1 | 11.8 | +6.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319542 | 11.53 | 230 | 17.1 | 17.6 | +2.7 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V319622 | 7.82 | 85 | 13.9 | 14.7 | +5.7 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338022 | 5.69 | 45 | 13.9 | 15.0 | +8.0 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338122 | 5.69 | 50 | 15.5 | 16.6 | +6.9 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338142 | 5.69 | 55 | 17.0 | 18.1 | +6.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338322 | 5.74 | 70 | 21.2 | 22.3 | +5.4 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338632 | 7.77 | 174 | 28.7 | 29.1 | +1.5 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338682 | 9.30 | 232 | 26.7 | 26.9 | +0.7 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338782 | 7.82 | 150 | 24.4 | 25.0 | +2.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338822 | 7.92 | 180 | 28.5 | 29.0 | +1.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V338922 | 7.82 | 147 | 23.9 | 24.6 | +2.7 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339022 | 7.82 | 180 | 29.2 | 29.7 | +1.7 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339052 | 7.82 | 200 | 32.6 | 32.8 | +0.5 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339102 | 7.82 | 124 | 20.2 | 20.9 | +3.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339132 | 7.04 | 110 | 22.2 | 22.9 | +3.4 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339372 | 8.20 | 196 | 28.9 | 29.3 | +1.5 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339622 | 6.71 | 140 | 31.1 | 31.6 | +1.6 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339802 | 7.82 | 110 | 17.9 | 18.7 | +4.5 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339922 | 7.92 | 124 | 19.6 | 20.4 | +4.2 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | FMJ V339972 | 7.21 | 140 | 26.8 | 27.5 | +2.5 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | HPBT V338332 | 5.69 | 69 | 24.2 | 25.4 | +5.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V338482 | 7.82 | 168 | 32.0 | 31.6 | -1.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V338702 | 7.82 | 180 | 34.3 | 33.7 | -1.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V338712 | 7.82 | 175 | 33.3 | 32.8 | -1.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V339152 | 5.69 | 77 | 27.0 | 28.2 | +4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V339162 | 5.69 | 52 | 18.2 | 19.5 | +7.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V339172 | 6.71 | 142 | 36.3 | 36.3 | +0.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V339432 | 8.59 | 250 | 39.9 | 38.2 | -4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | HPBT V339442 | 8.59 | 300 | 47.9 | 45.3 | -5.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sellier & Bellot | JHP V318032 | 11.48 | 255 | 17.3 | 19.3 | +11.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | JHP V318792 | 10.16 | 180 | 16.3 | 17.6 | +8.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | JHP V318912 | 11.48 | 230 | 15.6 | 17.5 | +12.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | JHP V318972 | 11.48 | 230 | 15.6 | 17.5 | +12.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | JHP V319402 | 9.02 | 124 | 14.8 | 15.7 | +5.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | JHP V319422 | 9.02 | 115 | 13.8 | 14.6 | +5.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | LFN V318642 | 9.07 | 158 | 17.6 | 18.3 | +3.8 % | lead, flat nose |  | -8…+11 % |
| Sellier & Bellot | LFN V319532 | 11.53 | 250 | 17.2 | 17.6 | +2.4 % | lead, flat nose |  | -8…+11 % |
| Sellier & Bellot | LRN V318372 | 9.02 | 124 | 14.8 | 16.2 | +9.2 % | lead |  | -8…+11 % |
| Sellier & Bellot | LRN V318522 | 9.07 | 158 | 18.6 | 20.1 | +7.9 % | lead |  | -8…+11 % |
| Sellier & Bellot | LRN V319482 | 7.98 | 100 | 15.2 | 16.8 | +10.2 % | lead |  | -8…+11 % |
| Sellier & Bellot | SJHP V318742 | 9.07 | 158 | 18.7 | 19.5 | +4.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sellier & Bellot | SP V318672 | 9.07 | 158 | 19.4 | 20.5 | +5.4 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V318772 | 9.07 | 158 | 19.4 | 20.5 | +5.4 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V319272 | 9.02 | 124 | 15.4 | 16.5 | +7.0 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V319522 | 10.92 | 240 | 19.1 | 21.1 | +10.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V319932 | 9.02 | 100 | 12.4 | 13.5 | +8.6 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338072 | 5.69 | 45 | 15.8 | 15.6 | -1.3 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338172 | 5.69 | 50 | 17.6 | 17.2 | -2.2 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338272 | 5.69 | 55 | 19.3 | 18.8 | -2.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338372 | 5.74 | 70 | 24.1 | 23.2 | -3.7 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338422 | 6.17 | 100 | 29.3 | 28.2 | -3.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338442 | 6.71 | 156 | 38.0 | 36.3 | -4.4 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338472 | 6.53 | 117 | 30.3 | 29.2 | -3.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338522 | 6.71 | 131 | 31.9 | 30.8 | -3.4 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338572 | 6.71 | 140 | 34.1 | 32.8 | -3.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338582 | 11.63 | 405 | 27.9 | 30.5 | +9.3 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338622 | 7.04 | 150 | 32.8 | 31.9 | -2.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338672 | 7.21 | 140 | 29.0 | 28.5 | -1.6 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338872 | 7.92 | 150 | 25.1 | 25.4 | +1.0 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V338942 | 7.82 | 110 | 18.9 | 19.4 | +2.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339072 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339272 | 7.92 | 124 | 20.7 | 21.2 | +2.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339292 | 7.04 | 130 | 28.4 | 27.9 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339322 | 8.08 | 196 | 31.4 | 31.3 | -0.2 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339522 | 9.32 | 193 | 22.3 | 23.4 | +5.0 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339572 | 9.30 | 285 | 33.1 | 33.9 | +2.4 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339582 | 7.82 | 150 | 25.8 | 26.0 | +0.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SP V339772 | 7.92 | 180 | 30.1 | 30.1 | -0.0 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SPCE V338732 | 7.21 | 173 | 35.8 | 34.8 | -2.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SPCE V338972 | 7.82 | 150 | 25.8 | 26.0 | +0.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SPCE V339422 | 8.20 | 196 | 30.3 | 30.5 | +0.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | SPCE V339722 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TFMJ V318532 | 9.07 | 158 | 19.1 | 19.7 | +3.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TFMJ V318922 | 9.07 | 158 | 19.1 | 19.7 | +3.1 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TFMJ V318952 | 10.16 | 180 | 17.3 | 17.8 | +3.2 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TFMJ V319572 | 11.48 | 230 | 17.3 | 17.7 | +2.4 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TFMJ V319802 | 9.02 | 115 | 14.1 | 14.8 | +4.8 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TFMJ V319842 | 9.02 | 124 | 15.2 | 15.9 | +4.3 % | full metal jacket |  | -15…+10 % |
| Sellier & Bellot | TXRG V338432 | 6.71 | 120 | 29.3 | 28.4 | -3.1 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339082 | 7.82 | 110 | 18.9 | 19.4 | +2.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339332 | 7.21 | 150 | 31.0 | 30.4 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339382 | 7.82 | 165 | 28.4 | 28.4 | +0.1 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339392 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339952 | 9.30 | 250 | 29.0 | 30.0 | +3.3 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | TXRG V339992 | 8.20 | 180 | 27.8 | 28.1 | +1.2 % | jacketed lead core |  | -11…+10 % |
| Sellier & Bellot | WC V318572 | 9.07 | 148 | 17.1 | 18.7 | +9.4 % | lead, wadcutter, flat base |  | -8…+11 % |
| Sellier & Bellot | WC V319472 | 7.98 | 100 | 14.9 | 16.6 | +11.5 % | lead, wadcutter, flat base |  | -8…+11 % |
| Shooting Technology | HPHB Kegelspitz verkupfert 0.356 100 gr | 9.04 | 100 | 12.4 | 12.0 | -3.5 % | lead, flat nose |  | -8…+11 % |
| Shooting Technology | HPHB TC Kegelstumpf Hohlspitz verkupfert 0.356 116 gr | 9.04 | 116 | 13.8 | 12.7 | -8.0 % | lead, flat nose, hollow point, flat base |  | -8…+11 % |
| Shooting Technology | Kegelstumpf verkupfert 0.356 116 gr | 9.04 | 116 | 14.3 | 13.8 | -3.8 % | lead, flat nose |  | -8…+11 % |
| Shooting Technology | Rundkopf verkupfert 0.356 100 gr | 9.04 | 100 | 12.4 | 11.8 | -4.7 % | lead, round nose, flat base |  | -8…+11 % |
| Shooting Technology | Rundkopf verkupfert 0.356 116 gr | 9.04 | 116 | 14.3 | 13.6 | -5.0 % | lead, round nose, flat base |  | -8…+11 % |
| Sierra | FMJ TOURNAMENT MASTER 8815 | 11.46 | 230 | 17.3 | 17.8 | +2.7 % | full metal jacket |  | -15…+10 % |
| Sierra | FMJBT GAMEKING 2115 | 7.82 | 150 | 24.4 | 25.7 | +5.3 % | full metal jacket, boat tail |  | -15…+10 % |
| Sierra | FMJBT GameKing 1355 | 5.69 | 55 | 17.0 | 18.6 | +9.4 % | full metal jacket, boat tail |  | -15…+10 % |
| Sierra | FN PRO-HUNTER 2000 | 7.82 | 150 | 25.8 | 23.0 | -10.9 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Sierra | FN PRO-HUNTER 2010 | 7.82 | 170 | 29.3 | 25.8 | -11.8 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Sierra | FN PRO-HUNTER 2755 | 9.09 | 155 | 18.9 | 17.7 | -6.4 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Sierra | FN PRO-HUNTER 2900 | 9.52 | 200 | 21.9 | 20.5 | -6.3 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Sierra | FN PRO-HUNTER 8760 | 11.46 | 260 | 18.5 | 18.3 | -1.0 % | jacketed lead core, flat nose, flat base |  | -11…+10 % |
| Sierra | HP VarmintKing 2116 | 7.82 | 115 | 19.1 | 19.3 | +0.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | HP VarmintKing 2136 | 7.82 | 150 | 24.9 | 24.7 | -0.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | HPBT GAMEKING 2236 | 7.82 | 205 | 39.1 | 38.0 | -2.7 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT GameKing 1728 | 6.71 | 130 | 33.2 | 33.4 | +0.7 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING 2210 | 7.82 | 190 | 36.2 | 35.4 | -2.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING 2230 | 7.82 | 200 | 38.1 | 37.2 | -2.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING 2240 | 7.82 | 220 | 41.9 | 40.6 | -3.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING 9300T | 8.59 | 300 | 47.9 | 45.3 | -5.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING X 2175 | 7.82 | 175 | 33.3 | 32.8 | -1.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MATCHKING X 2232 | 7.82 | 200 | 38.1 | 37.2 | -2.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing  - Cannelure 9377GT | 5.69 | 77 | 27.0 | 28.2 | +4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing 1711C | 6.71 | 100 | 25.6 | 26.1 | +2.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing 1725 | 6.71 | 120 | 30.7 | 31.0 | +1.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing 1744 | 6.71 | 144 | 36.8 | 36.8 | -0.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing 9377T | 5.69 | 77 | 27.0 | 28.2 | +4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing X 1571 | 6.17 | 107 | 32.1 | 32.8 | +2.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing X 1743 | 6.71 | 142 | 36.3 | 36.3 | +0.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing X 1834 | 7.04 | 135 | 31.5 | 31.5 | +0.1 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing X 1976 | 7.21 | 175 | 38.9 | 38.4 | -1.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT MatchKing X 9380 | 5.69 | 77 | 27.0 | 28.2 | +4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT-CN MATCHKING 9631 | 6.53 | 131 | 35.2 | 35.5 | +0.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN  MatchKing 1981 | 7.21 | 180 | 40.0 | 39.4 | -1.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN - PALMA MATCHKING 2156 | 7.82 | 155 | 29.5 | 29.3 | -0.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 2231 | 7.82 | 200 | 38.1 | 37.2 | -2.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 2237 | 7.82 | 215 | 41.0 | 39.8 | -3.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 2251 | 7.82 | 230 | 43.8 | 42.4 | -3.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 2269 | 7.82 | 169 | 32.2 | 31.7 | -1.4 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 2277 | 7.82 | 177 | 33.7 | 33.1 | -1.6 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 9240T | 7.82 | 210 | 40.0 | 38.9 | -2.7 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MATCHKING 9350T | 9.52 | 350 | 46.0 | 42.9 | -6.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1396 | 5.69 | 95 | 33.3 | 34.3 | +3.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1565 | 6.17 | 105 | 31.5 | 32.2 | +2.2 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1575 | 6.17 | 110 | 33.0 | 33.6 | +1.9 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1729T | 6.71 | 130 | 33.2 | 33.4 | +0.7 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1755 | 6.71 | 150 | 38.3 | 38.2 | -0.2 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 1997 | 7.21 | 197 | 43.8 | 42.9 | -2.0 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 6MM95GRHPBTC | 6.17 | 95 | 28.5 | 29.3 | +2.8 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 9290T | 5.69 | 90 | 31.5 | 32.6 | +3.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HPBT\u002fCN MatchKing 9390T | 5.69 | 80 | 28.0 | 29.2 | +4.3 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HP\u002fCN MATCHKING 2121 | 7.82 | 125 | 23.8 | 23.9 | +0.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | HP\u002fFN PRO-HUNTER 8900 | 11.63 | 300 | 19.7 | 21.3 | +7.9 % | jacketed lead core, hollow point, flat base |  | -11…+10 % |
| Sierra | HP\u002fFN PRO-HUNTER 8950 | 11.63 | 400 | 26.2 | 27.8 | +6.3 % | jacketed lead core, hollow point, flat base |  | -11…+10 % |
| Sierra | HP\u002fFN Pro-Hunter 2020 | 7.82 | 125 | 20.8 | 20.2 | -2.7 % | jacketed lead core, hollow point, flat base |  | -11…+10 % |
| Sierra | JHC SPORTS MASTER 8826C | 11.46 | 225 | 15.3 | 17.2 | +12.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP - SIG V-CROWN 9947 | 9.02 | 147 | 17.6 | 18.4 | +4.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP PRO-HUNTER 3100 | 10.16 | 200 | 18.1 | 18.9 | +4.4 % | jacketed lead core, hollow point, flat base |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 5350 | 12.95 | 350 | 17.7 | 20.5 | +16.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 8124 | 9.02 | 124 | 14.8 | 15.7 | +5.9 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 8147 | 9.02 | 147 | 17.6 | 18.4 | +4.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 8320 | 9.07 | 125 | 14.8 | 15.6 | +5.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 8325 | 9.07 | 140 | 16.5 | 17.4 | +5.3 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JHP SPORTS MASTER 8835 | 11.46 | 185 | 12.6 | 14.3 | +13.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | JSP SPORTS MASTER 5400 | 12.95 | 400 | 21.3 | 24.5 | +14.9 % | jacketed lead core |  | -11…+10 % |
| Sierra | JSP Sport Master Teilmantel 0.500 400 gr | 12.70 | 400 | 22.4 | 25.4 | +13.4 % | jacketed lead core |  | -11…+10 % |
| Sierra | MatchKing HPBT 0.270 135 gr | 6.86 | 135 | 33.0 | 33.2 | +0.5 % | jacketed lead core, hollow point, boat tail, match |  | -11…+10 % |
| Sierra | RN PRO-HUNTER 2100 | 7.82 | 110 | 18.9 | 17.5 | -7.6 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Sierra | RN PRO-HUNTER 2135 | 7.82 | 150 | 25.8 | 23.4 | -9.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Sierra | RN PRO-HUNTER 2170 | 7.82 | 180 | 31.0 | 27.7 | -10.5 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Sierra | RN PRO-HUNTER 2180 | 7.82 | 220 | 37.9 | 33.5 | -11.7 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Sierra | RN PRO-HUNTER 2800 | 9.09 | 200 | 24.4 | 22.8 | -6.4 % | jacketed lead core, round nose, flat base |  | -11…+10 % |
| Sierra | SBT GAMEKING 3000 | 9.52 | 300 | 39.4 | 34.9 | -11.5 % | jacketed lead core, boat tail |  | -11…+10 % |
| Sierra | SBT GameKing 1913 | 7.21 | 150 | 33.3 | 31.3 | -6.1 % | jacketed lead core, boat tail |  | -11…+10 % |
| Sierra | SBT GameKing 9362 | 5.69 | 62 | 21.7 | 21.6 | -0.3 % | jacketed lead core, boat tail |  | -11…+10 % |
| Sierra | SBT Pro-Hunter 1560 | 6.17 | 100 | 30.0 | 28.9 | -3.6 % | jacketed lead core, boat tail |  | -11…+10 % |
| Sierra | SBT VarmintKing 1515 | 6.17 | 80 | 24.0 | 23.5 | -2.2 % | jacketed lead core, boat tail |  | -11…+10 % |
| Sierra | SMP VarmintKing 1370 | 5.69 | 63 | 22.1 | 21.4 | -3.3 % | jacketed lead core |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2120 | 7.82 | 125 | 21.5 | 21.3 | -1.0 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2130 | 7.82 | 150 | 25.8 | 25.2 | -2.2 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2300 | 7.90 | 150 | 25.3 | 24.7 | -2.2 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2305 | 7.90 | 125 | 21.1 | 20.9 | -1.1 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2310 | 7.90 | 180 | 30.3 | 29.4 | -3.1 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2400 | 8.20 | 150 | 23.2 | 23.0 | -0.8 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT PRO-HUNTER 2410 | 8.20 | 175 | 27.0 | 26.6 | -1.5 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT Pro-Hunter 1540 | 6.17 | 100 | 29.3 | 27.4 | -6.6 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT Pro-Hunter 1720 | 6.71 | 120 | 29.3 | 27.6 | -5.9 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT Pro-Hunter 1830 | 7.04 | 130 | 28.4 | 27.1 | -4.7 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT Pro-Hunter 1900 | 7.21 | 120 | 24.8 | 24.0 | -3.3 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | SPT Pro-Hunter 1910 | 7.21 | 140 | 29.0 | 27.7 | -4.5 % | jacketed lead core, flat base |  | -11…+10 % |
| Sierra | Sport Master JHP Hohlspitz 0.500 350 gr | 12.70 | 350 | 18.6 | 21.3 | +14.6 % | jacketed lead core, hollow point |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4610GT | 7.82 | 210 | 36.2 | 38.3 | +5.9 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4625 | 7.82 | 125 | 21.5 | 23.6 | +9.6 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4665T | 7.82 | 165 | 28.4 | 30.6 | +7.7 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4680T | 7.82 | 180 | 31.0 | 33.2 | +7.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4681T | 7.82 | 180 | 31.0 | 33.2 | +7.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4682 | 7.82 | 180 | 31.0 | 33.2 | +7.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED GAMEKING 4695 | 7.82 | 195 | 33.6 | 35.8 | +6.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | TIPPED MATCHKING 7720 | 7.82 | 220 | 41.9 | 42.5 | +1.5 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | TIPPED MATCHKING 7768 | 7.82 | 168 | 32.0 | 33.0 | +3.2 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | TIPPED MATCHKING 7775 | 7.82 | 175 | 33.3 | 34.3 | +3.1 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | TIPPED MATCHKING 7795 | 7.82 | 195 | 37.1 | 38.0 | +2.4 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped GameKing - BGB 4540C | 7.21 | 140 | 29.0 | 30.7 | +5.9 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4062 | 5.69 | 64 | 22.5 | 23.3 | +3.8 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4100T | 6.17 | 90 | 26.4 | 27.5 | +4.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4110T | 6.17 | 100 | 29.3 | 30.3 | +3.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4210 | 6.53 | 110 | 28.5 | 29.7 | +4.2 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4320 | 6.71 | 120 | 29.3 | 30.6 | +4.3 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4330T | 6.71 | 130 | 31.7 | 32.9 | +3.9 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4340 | 6.71 | 140 | 34.1 | 35.3 | +3.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4345 | 6.71 | 145 | 35.3 | 36.5 | +3.4 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4440T | 7.04 | 140 | 30.6 | 32.2 | +5.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4475 | 7.04 | 175 | 38.3 | 39.6 | +3.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4540 | 7.21 | 140 | 29.0 | 30.7 | +5.9 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4550T | 7.21 | 150 | 31.0 | 32.8 | +5.7 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4565T | 7.21 | 165 | 34.1 | 35.8 | +5.0 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 4580 | 7.21 | 180 | 37.2 | 38.9 | +4.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 7171 | 5.69 | 69 | 24.3 | 25.1 | +3.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped GameKing 7331 | 6.53 | 131 | 33.9 | 35.0 | +3.2 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped MatchKing 7188 | 5.69 | 88 | 30.8 | 33.4 | +8.5 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7207 | 6.17 | 107 | 32.1 | 34.3 | +6.8 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7216 | 6.17 | 116 | 34.8 | 37.0 | +6.3 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7295 | 6.17 | 95 | 28.5 | 30.7 | +7.6 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7334 | 6.53 | 135 | 36.3 | 38.2 | +5.2 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7407 | 6.71 | 107 | 27.3 | 29.1 | +6.7 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7430 | 6.71 | 130 | 33.2 | 35.0 | +5.3 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7450 | 6.71 | 153 | 39.1 | 40.7 | +4.2 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped MatchKing 7685 | 7.21 | 185 | 41.1 | 42.3 | +3.0 % | jacketed lead core, polymer tip, boat tail, match |  | -11…+10 % |
| Sierra | Tipped VarmintKing 1036 | 5.18 | 36 | 15.6 | 16.3 | +4.7 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped VarmintKing 1039 | 5.18 | 39 | 16.8 | 17.6 | +4.8 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped VarmintKing 1785 | 6.71 | 105 | 25.6 | 27.0 | +5.3 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped VarmintKing 22CAL40GRTIP | 5.69 | 40 | 14.1 | 15.0 | +6.5 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Sierra | Tipped VarmintKing 7170 | 5.69 | 69 | 24.3 | 25.1 | +3.1 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Speer | Gold Dot BTSP 0.224 62 gr | 5.69 | 62 | 21.7 | 21.6 | -0.3 % | jacketed lead core, boat tail |  | -11…+10 % |
| Speer | Gold Dot BTSP 0.224 75 gr | 5.69 | 75 | 26.3 | 25.9 | -1.7 % | jacketed lead core, boat tail |  | -11…+10 % |
| Speer | Gold Dot Hohlspitz 0.40 200 gr | 10.16 | 200 | 18.1 | 18.9 | +4.4 % | jacketed lead core, hollow point, flat base |  | -11…+10 % |
| Speer | Gold Dot SP 0.264 120 gr | 6.71 | 120 | 29.3 | 27.6 | -5.9 % | jacketed lead core, flat base |  | -11…+10 % |
| Speer | Gold Dot SP 0.277 115 gr | 7.04 | 115 | 25.2 | 24.1 | -4.2 % | jacketed lead core, flat base |  | -11…+10 % |
| Speer | Grand Slam 0.375 285 gr | 9.52 | 285 | 31.3 | 31.4 | +0.5 % | jacketed lead core, flat base |  | -11…+10 % |
| Speer | Hot-Cor 0.323 150 gr | 8.20 | 150 | 23.2 | 23.7 | +2.2 % | jacketed lead core |  | -11…+10 % |
| Speer | Hot-Cor Semi Spitz 0.366 270 gr | 9.30 | 270 | 31.3 | 32.2 | +2.9 % | jacketed lead core |  | -11…+10 % |
| Speer | Hot-Cor Teilmantel Spitz 0.243 90 gr | 6.17 | 90 | 26.4 | 25.5 | -3.3 % | jacketed lead core |  | -11…+10 % |
| Speer | Impact Bonded Tipped Teilmantelverbundkern 0.308 190 gr | 7.82 | 190 | 32.7 | 34.9 | +6.8 % | jacketed lead core, polymer tip |  | -11…+10 % |
| Speer | Mag Tip 0.416 350 gr | 10.57 | 350 | 30.2 | 32.0 | +6.1 % | jacketed lead core |  | -11…+10 % |
| Speer | RNSP Plinker Plinker 0.308 100 gr | 7.82 | 100 | 17.2 | 17.8 | +3.3 % | jacketed lead core |  | -11…+10 % |
| Speer | TMJ Vollmantel 0.224 75 gr | 5.69 | 75 | 23.2 | 24.2 | +4.5 % | full metal jacket |  | -15…+10 % |
| Speer | Teilmantel 0.311 180 gr | 7.90 | 180 | 30.3 | 30.2 | -0.2 % | jacketed lead core |  | -11…+10 % |
| Swift | A-Frame 0.358 280 gr | 9.09 | 280 | 34.2 | 34.8 | +1.9 % | jacketed lead core |  | -11…+10 % |
| Swift | A-Frame 0.509 570 gr | 12.93 | 570 | 30.5 | 34.2 | +12.2 % | jacketed lead core |  | -11…+10 % |
| Ve-Loads | BMG Long Range Target Match Kupfer 0.510 813 gr | 12.95 | 813 | 60.1 | 58.6 | -2.5 % | monolithic copper/brass, match |  | -11…+12 % |
| Ve-Loads | Long Range Target Match Messing 0.284 141 gr | 7.21 | 141 | 31.3 | 35.4 | +13.0 % | monolithic copper/brass, match |  | -11…+12 % |
| Wi-La-Tech | DSG / NS Doppel-Scharfrand- bleifrei 0.308 154 gr | 7.82 | 154 | 30.1 | 32.0 | +6.2 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.224 46 gr | 5.69 | 46 | 17.0 | 19.1 | +12.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.243 65 gr | 6.17 | 65 | 20.4 | 22.6 | +10.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.264 100 gr | 6.71 | 100 | 26.6 | 28.7 | +8.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.284 110 gr | 7.21 | 110 | 25.3 | 27.3 | +7.9 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.308 130 gr | 7.82 | 130 | 25.4 | 27.3 | +7.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.338 165 gr | 8.59 | 165 | 26.7 | 28.4 | +6.4 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.356 180 gr | 9.04 | 180 | 26.3 | 27.9 | +6.2 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.364 165 gr | 9.25 | 165 | 23.9 | 24.6 | +3.0 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.366 165 gr | 9.30 | 165 | 23.7 | 24.4 | +2.8 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.366 180 gr | 9.30 | 180 | 24.9 | 26.4 | +6.1 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.416 303 gr | 10.57 | 303 | 32.4 | 33.6 | +3.6 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei 0.423 292 gr | 10.74 | 292 | 31.4 | 31.4 | +0.1 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Wi-La-Tech | DSG Doppel-Scharfrand- bleifrei Hornet 0.224 43 gr | 5.69 | 43 | 15.9 | 17.9 | +12.8 % | monolithic copper/brass, relief grooves |  | -11…+12 % |
| Winchester | BC MAX WB30BC195 | 7.82 | 195 | 37.1 | 34.1 | -8.0 % | jacketed lead core, boat tail |  | -11…+10 % |
| Winchester | BC MAX WB65BC140 | 6.71 | 140 | 35.8 | 33.7 | -5.9 % | jacketed lead core, boat tail |  | -11…+10 % |
| Winchester | FMJ - Flat Base WB9FB115 | 9.02 | 115 | 14.1 | 14.3 | +1.8 % | full metal jacket, flat base |  | -15…+10 % |
| Winchester | FMJ - Flat Base WB9MC124 | 9.02 | 124 | 15.2 | 15.4 | +1.3 % | full metal jacket, flat base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB38JHP110 | 9.07 | 110 | 13.2 | 12.3 | -6.8 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB38JHP125 | 9.07 | 125 | 15.1 | 13.9 | -8.1 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB40JHP180 | 10.16 | 180 | 17.3 | 15.7 | -9.5 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB44HSP240 | 10.92 | 240 | 19.9 | 17.8 | -10.4 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB45JHP230 | 11.46 | 230 | 17.3 | 15.6 | -9.8 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB9JHP115 | 9.02 | 115 | 14.1 | 13.0 | -8.0 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | FMJ - Hollow Base WB9JHP147 | 9.02 | 147 | 17.9 | 16.3 | -8.8 % | full metal jacket, hollow point, hollow base |  | -15…+10 % |
| Winchester | Full Metal Jacket WB380MC95 | 9.04 | 95 | 11.5 | 12.3 | +6.9 % | full metal jacket |  | -15…+10 % |
| Winchester | Full Metal Jacket WB38MC130 | 9.04 | 130 | 15.8 | 16.5 | +4.5 % | full metal jacket |  | -15…+10 % |
| Winchester | Full Metal Jacket WB45MC230 | 11.46 | 230 | 17.3 | 17.8 | +2.7 % | full metal jacket |  | -15…+10 % |
| Winchester | Full Metal Jacket WB556MC55 | 5.69 | 55 | 17.0 | 18.1 | +6.6 % | full metal jacket |  | -15…+10 % |
| Winchester | Full Metal Jacket WB762MC147 | 7.82 | 147 | 23.9 | 24.6 | +2.7 % | full metal jacket |  | -15…+10 % |
| Winchester | Hollow Point WB22HP46 | 5.69 | 46 | 15.8 | 15.1 | -4.2 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | JHP - Notched WB9MC115 | 9.02 | 115 | 13.8 | 14.6 | +5.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Pointed Soft Point WB223PSP55 | 5.69 | 55 | 19.3 | 18.8 | -2.5 % | jacketed lead core |  | -11…+10 % |
| Winchester | Pointed Soft Point WB243PSP80 | 6.17 | 80 | 23.5 | 22.9 | -2.8 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB223PP64 | 5.69 | 64 | 22.5 | 21.7 | -3.6 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB270PP130 | 7.04 | 130 | 28.4 | 27.9 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB308SP180 | 7.82 | 180 | 31.0 | 30.8 | -0.5 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB30PP150 | 7.82 | 150 | 25.8 | 26.0 | +0.8 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB762PP123 | 7.87 | 123 | 20.9 | 21.3 | +2.0 % | jacketed lead core |  | -11…+10 % |
| Winchester | Power-Point WB7PP150 | 7.21 | 150 | 31.0 | 30.4 | -1.8 % | jacketed lead core |  | -11…+10 % |
| Winchester | Silvertip Hollow Point WB357ST145 | 9.07 | 145 | 17.1 | 17.9 | +5.0 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Silvertip Hollow Point WB40ST155 | 10.16 | 155 | 14.0 | 15.3 | +9.5 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Silvertip Hollow Point WB40ST175 | 10.16 | 175 | 15.8 | 17.2 | +8.7 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Silvertip Hollow Point WB9ST115 | 9.02 | 115 | 13.8 | 14.6 | +5.8 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Silvertip Hollow Point WB9ST147 | 9.02 | 147 | 17.6 | 18.4 | +4.4 % | jacketed lead core, hollow point |  | -11…+10 % |
| Winchester | Soft Point WB44SP200 | 10.82 | 200 | 16.3 | 18.1 | +11.1 % | jacketed lead core |  | -11…+10 % |
| Winchester | Truncated Cone WB40TC165 | 10.16 | 165 | 15.6 | 15.6 | -0.3 % | jacketed lead core, flat nose |  | -11…+10 % |
| Winchester | Truncated Cone WB40TC180 | 10.16 | 180 | 17.0 | 16.9 | -0.7 % | jacketed lead core, flat nose |  | -11…+10 % |
| Woodleigh | Weldcore Teilmantel Rundkopf 0.413 400 gr | 10.49 | 400 | 35.1 | 33.1 | -5.6 % | jacketed lead core, round nose, flat base |  | -11…+10 % |

## Applying the result

`recalculate_lengths.py --apply` writes `glen` and a note that says how the value was obtained into the records.
Without `--apply` only this report is written. The script is self-contained (Python standard library only) and
deterministic, so the table can be reproduced at any time.
