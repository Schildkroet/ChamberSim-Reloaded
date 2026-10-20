import re
import sys
from pathlib import Path


ALERT = re.compile(r"^> \[!(NOTE|WARNING)\]\s*$")
ADMONITION_TITLES = {"NOTE": ("note", "Note"), "WARNING": ("warning", "Safety")}
IMAGE_SRC = re.compile(r'(<img\b[^>]*?\bsrc=["\'])(?:\./)?images/', re.IGNORECASE)


def convert_alerts(markdown: str) -> str:
    lines = markdown.splitlines()
    converted = []
    index = 0

    while index < len(lines):
        match = ALERT.fullmatch(lines[index])
        if match is None:
            converted.append(lines[index])
            index += 1
            continue

        alert_type = match.group(1)
        admonition, title = ADMONITION_TITLES[alert_type]
        body = []
        index += 1
        while index < len(lines) and lines[index].startswith(">"):
            line = lines[index][1:]
            body.append(line[1:] if line.startswith(" ") else line)
            index += 1

        converted.extend((f'!!! {admonition} "{title}"', ""))
        converted.extend(f"    {line}" for line in body)

    result = "\n".join(converted)
    return result + ("\n" if markdown.endswith(("\n", "\r")) else "")


def fix_image_paths(markdown: str) -> str:
    return IMAGE_SRC.sub(r"\1../images/", markdown)


def main() -> None:
    for path in Path(sys.argv[1]).rglob("*.md"):
        original = path.read_text(encoding="utf-8")
        converted = fix_image_paths(convert_alerts(original))
        if converted != original:
            path.write_text(converted, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
