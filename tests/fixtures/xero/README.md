# Garmin Xero sample files

Exports of the Garmin Xero C1 Pro and of apps that handle them, used by `CoreTest::chronographFiles`.

* `garmin-xero.fit`, `garmin-xero.csv` – made for this project (own values).
* All other files come from the test assets of <https://github.com/useyournose/xeroconv>
  (`src/_tests/assets`, MIT License, Copyright (c) 2024 useyournose; see `LICENSE-xeroconv`), unchanged:
  * `06-18-2024_15-47-11.fit` (10 shots), `06-18-2024_15-50-06.fit` (15), `06-18-2024_15-56-35.fit` (22),
    `08-10-2026_10-44-31.fit` (5) – shot session FIT files of the device;
  * `broken.fit` – not a FIT file (15 bytes);
  * `Hornady_VMAX_64gr__2024-10-04_18-41-13.csv`, `Shotview_a.csv` – German-language CSV of the Xero app (quoted decimal commas,
    byte order mark in front of the header);
  * `Shotview_i.csv` – English CSV in fps with a PM time stamp; `Shotview_i_de.csv` – German headers, unquoted numbers.
