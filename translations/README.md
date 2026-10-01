# Translations

American English (`en_US`) is the **source language and the default**. Source strings in the code are
English; no translation file is needed to run in English. `GRTReloaded_en_US.ts` only carries the
English *plural forms* (`%n entry` / `%n entries`).

## Adding a language (e.g. German)

1. In `CMakeLists.txt` change `set(GRT_LANGUAGES en_US)` to `set(GRT_LANGUAGES en_US de_DE)`.
2. Create the file once: copy `GRTReloaded_en_US.ts` to `GRTReloaded_de_DE.ts` and set
   `language="de_DE"` in its `<TS ...>` header.
3. Run the CMake target **`update_translations`** (Qt Creator: Build > Run CMake, then build that target;
   or `cmake --build <build-dir> --target update_translations`). This extracts all strings.
4. Translate with Qt Linguist (`linguist translations/GRTReloaded_de_DE.ts`).
5. Build as usual. `.qm` files are generated (target `release_translations`) and embedded under
   `:/i18n`; the language shows up in **View > Language** automatically.

After changing user-visible strings in code, run `update_translations` again (it keeps existing
translations and marks changed ones unfinished).

## How the language is chosen

`translation::install()` (src/app/translation.cpp) picks, in order: the `--language` command-line
option, the saved choice from View > Language, the system UI languages. The first one with a
translation wins (`de_AT` falls back to `de_DE`); otherwise `en_US`. The default `QLocale` follows the
chosen language, so number formats match. The language menu applies after a restart.

## Rules for code

* Every user-visible string goes through `tr()` (in `QObject` classes) or
  `QCoreApplication::translate("Context", "...")` (in `src/core`). Write the source text in US English.
* Use `QT_TR_NOOP("...")` for strings stored in tables and translated later with `tr(var)`.
* Plurals: `tr("%n entries", nullptr, count)`; add both forms in the language files.
* Do not build sentences from fragments; use `%1`/`%2` placeholders so translators can reorder.
* File formats, keys, units inside `.grtload`/`.db` files stay untranslated and use `.` decimals.
