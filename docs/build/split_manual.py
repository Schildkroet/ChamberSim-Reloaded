#!/usr/bin/env python3
"""Split the generated manual into its top-level user-guide sections."""

from pathlib import Path
import re
import sys


SECTION_FILES = {
    "Introduction": "ug_intro.md",
    "Installation and first start": "ug_installation.md",
    "The main window": "ug_mainwindow.md",
    "The database": "ug_database.md",
    "Making, opening and saving loads": "ug_files.md",
    "Entering a load": "ug_newload.md",
    "Results": "ug_results.md",
    "Measurements": "ug_measurements.md",
    "Reports and printing": "ug_reporting.md",
    "Tools": "ug_tools.md",
    "Settings": "ug_settings.md",
    "Tips and troubleshooting": "ug_troubleshooting.md",
    "Glossary": "ug_glossary.md",
}

HEADING_RE = re.compile(r"^# (.+?)\s*$")
FRONTMATTER = "---\npdf: true\n---\n\n"


def split_sections(markdown: str) -> dict[str, str]:
    sections: dict[str, list[str]] = {}
    current_heading = None

    for line in markdown.splitlines(keepends=True):
        match = HEADING_RE.match(line.rstrip("\r\n"))
        if match:
            current_heading = match.group(1)
            if current_heading in sections:
                raise ValueError(f"Duplicate top-level section: {current_heading}")
            sections[current_heading] = [line]
        elif current_heading is not None:
            sections[current_heading].append(line)

    unexpected = sections.keys() - SECTION_FILES.keys()
    missing = SECTION_FILES.keys() - sections.keys()
    if unexpected or missing:
        details = []
        if unexpected:
            details.append("unexpected sections: " + ", ".join(sorted(unexpected)))
        if missing:
            details.append("missing sections: " + ", ".join(sorted(missing)))
        raise ValueError("; ".join(details))

    return {
        heading: "".join(content).rstrip("\r\n") + "\n"
        for heading, content in sections.items()
    }


def main() -> int:
    manual_dir = Path(__file__).resolve().parent.parent / "manual"
    source = manual_dir / "manual.md"

    try:
        sections = split_sections(source.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        print(f"Could not split {source}: {error}", file=sys.stderr)
        return 1

    for heading, filename in SECTION_FILES.items():
        content = FRONTMATTER + sections[heading]
        (manual_dir / filename).write_text(content, encoding="utf-8")

    print(f"Split {len(sections)} sections from {source.name}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
