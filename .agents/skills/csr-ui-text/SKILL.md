---
name: csr-ui-text
description: "Use when adding or changing any user-visible text in ChamberSim Reloaded: a label, tooltip, message box, menu entry, report text or input-panel field name, or when updating or adding a translation (.ts file)."
---

# UI text and translations

Long form: `translations/README.md`.

## Writing strings

- American English (`en_US`) is the source language and the default. Write the source text in US English.
- Every user-visible string goes through `tr()` (QObject classes) or `QCoreApplication::translate("Context", "...")`
  (in `src/core`, which has no widgets). Strings in tables: `QT_TR_NOOP("...")`, translated later with `tr(var)`.
- Plurals: `tr("%n entries", nullptr, count)`; the `en_US.ts` carries the English plural forms.
- Whole sentences with `%1`/`%2` placeholders, never sentences glued from fragments: translators reorder.
- File formats, XML/JSON keys, units inside `.csrload`/`.grtload`/`.db` stay untranslated and use `.` decimals.
- Property names in panels are capitalised (Title Case); labels readable, not abbreviations nobody knows.
- Input-panel labels come from the load files' `descr` attribute; their translations are in the context
  `grt::fields` (`src/core/fieldnames.cpp`): there the translation is the finished label, a part in brackets becomes
  the tooltip.

## After changing strings

```bash
cmake --build --preset debug --target update_translations
```

Run it after every string change: the `.ts` files are not always up to date. It keeps existing translations and marks
changed ones unfinished. Commit the updated `.ts` files with the change.

Languages: `de_DE`, `fr_FR`, `it_IT`, `es_ES`, `sv_SE`, `nb_NO` (`CSR_LANGUAGES` in `CMakeLists.txt`). Translate new
strings in the languages you can do well; leave the rest unfinished rather than guessing. Check a translated UI
headless with `--language de_DE` (German is usually the longest: labels must still fit).

## Adding a language

`CSR_LANGUAGES` in `CMakeLists.txt`, copy `ChamberSimReloaded_en_US.ts` with the new `language="xx_YY"` header, run
`update_translations`, translate. The language appears in View > Language by itself (`src/app/translation.cpp`).
