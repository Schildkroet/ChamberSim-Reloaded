import html
import os
import posixpath
import re
from datetime import datetime, timezone
from urllib.parse import quote

import markdown

from mkdocs.config import Config
from mkdocs.structure.files import File, Files, InclusionLevel
from mkdocs.structure.nav import Section, get_navigation
from mkdocs.structure.pages import Page

PDF_ONLY_ENV = "MKDOCS_EXPORTER_PDF_ONLY"
FRONT_MATTER_RE = re.compile(
    r"\A---[ \t]*\r?\n.*?\r?\n---[ \t]*(?:\r?\n|$)", re.DOTALL
)
FENCE_RE = re.compile(r"^ {0,3}(?P<fence>`{3,}|~{3,})")
HEADING_RE = re.compile(r"^( {0,3})(#{1,6})(?=\s|$)")


def _markdown_parser() -> markdown.Markdown:
    return markdown.Markdown(
        extensions=[
            "admonition",
            "attr_list",
            "fenced_code",
            "md_in_html",
            "meta",
            "tables",
            "toc",
            "pymdownx.highlight",
            "pymdownx.superfences",
            "pymdownx.arithmatex",
        ],
        extension_configs={"toc": {"toc_depth": "2"}},
    )


def _page_markdown(page: Page) -> tuple[str, markdown.Markdown]:
    with open(page.file.abs_src_path, encoding="utf-8") as source:
        content = source.read()

    parser = _markdown_parser()
    parser.convert(content)
    return content, parser


def _is_pdf_page(page: Page) -> bool:
    _, parser = _page_markdown(page)
    pdf_values = parser.Meta.get("pdf", ["false"])
    is_pdf_page = bool(pdf_values) and pdf_values[0].strip().lower() == "true"
    return is_pdf_page


def _page_toc(page: Page, include_non_pdf: bool = False) -> tuple[bool, str]:
    _, parser = _page_markdown(page)
    pdf_values = parser.Meta.get("pdf", ["false"])
    is_pdf_page = bool(pdf_values) and pdf_values[0].strip().lower() == "true"
    if not is_pdf_page and not include_non_pdf:
        return False, ""

    page_url = posixpath.relpath(page.file.url, "generated_site_toc/")
    page_url = quote(page_url, safe="/")
    toc = parser.toc.replace('href="#', f'href="{page_url}#')
    return True, toc


def _nav_pages(items: list) -> list[Page]:
    pages = []
    for item in items:
        if isinstance(item, Page):
            pages.append(item)
        elif isinstance(item, Section):
            pages.extend(_nav_pages(item.children))
    return pages


def _section_page_paths(items: list, section_title: str) -> set[str]:
    page_paths = set()
    for item in items:
        if isinstance(item, Page):
            continue
        if isinstance(item, Section):
            if item.title == section_title:
                page_paths.update(
                    page.file.src_path for page in _nav_pages(item.children)
                )
            else:
                page_paths.update(_section_page_paths(item.children, section_title))
    return page_paths


def _demote_headings(content: str) -> str:
    lines = content.splitlines()
    output = []
    fence_character = None
    fence_length = 0

    for line in lines:
        fence = FENCE_RE.match(line)
        if fence_character is not None:
            output.append(line)
            if (
                fence
                and fence.group("fence")[0] == fence_character
                and len(fence.group("fence")) >= fence_length
            ):
                fence_character = None
                fence_length = 0
            continue

        if fence:
            fence_character = fence.group("fence")[0]
            fence_length = len(fence.group("fence"))
            output.append(line)
            continue

        heading = HEADING_RE.match(line)
        if heading and len(heading.group(2)) < 6:
            line = (
                heading.group(1)
                + "#"
                + heading.group(2)
                + line[heading.end() :]
            )
        output.append(line)

    return "\n".join(output)


def _merged_manual(pages: list[Page], always_include_paths: set[str]) -> str:
    chapters = []
    for page in pages:
        if page.file.src_path not in always_include_paths and not _is_pdf_page(page):
            continue

        content, _ = _page_markdown(page)
        content = FRONT_MATTER_RE.sub("", content, count=1)
        content = _demote_headings(content).strip()
        if content:
            chapters.append(content)

    if not chapters:
        raise ValueError("Cannot generate the PDF manual: no PDF pages found.")

    chapter_markdown = "\n\n<div class=\"page-break\"></div>\n\n".join(chapters)
    toc_parser = _markdown_parser()
    toc_parser.convert(chapter_markdown)
    return (
        "---\npdf: true\n---\n\n"
        f'<div class="pdf-toc">{toc_parser.toc}</div>\n\n'
        '<div class="page-break"></div>\n\n'
        f"{chapter_markdown}\n"
    )


def _nav_to_html(items: list, include_non_pdf: bool = False) -> tuple[str, int]:
    output = []
    included_pages = 0

    for item in items:
        if isinstance(item, Page):
            include, page_toc = _page_toc(item, include_non_pdf)
            if not include:
                continue

            title = item.title or item.file.src_path
            page_url = quote(
                posixpath.relpath(item.file.url, "generated_site_toc/"),
                safe="/",
            )
            output.append(
                f'<li><a href="{html.escape(page_url, quote=True)}">'
                f"{html.escape(title)}</a>{page_toc}</li>"
            )
            included_pages += 1
        elif isinstance(item, Section):
            children, child_pages = _nav_to_html(
                item.children,
                include_non_pdf or item.title == "User Guide",
            )
            if child_pages:
                output.append(
                    f"<li>{html.escape(item.title or '')}{children}</li>"
                )
                included_pages += child_pages

    if not output:
        return "", 0

    return "<ul>\n" + "\n".join(output) + "\n</ul>", included_pages


def on_files(files: Files, config: Config) -> Files:
    nav = get_navigation(files, config)
    pages = _nav_pages(nav)

    if os.environ.get(PDF_ONLY_ENV, "").lower() in {"1", "true", "yes"}:
        user_guide_pages = _section_page_paths(nav, "User Guide")
        manual_file = File.generated(
            config,
            "generated_manual.md",
            content=_merged_manual(pages, user_guide_pages),
            inclusion=InclusionLevel.INCLUDED,
        )
        files.append(manual_file)
        config.nav[:] = [
            {"ChamberSim Reloaded User Manual": "generated_manual.md"}
        ]
        return files

    toc, included_pages = _nav_to_html(nav)
    if not included_pages:
        raise ValueError("Cannot generate the PDF table of contents: no PDF pages found.")

    content = f"---\npdf: true\n---\n\n# Contents\n\n{toc}\n"
    toc_file = File.generated(
        config,
        "generated_site_toc.md",
        content=content,
        inclusion=InclusionLevel.INCLUDED,
    )
    files.append(toc_file)
    config.nav.insert(0, {"": "generated_site_toc.md"})
    return files


def on_page_markdown(markdown_text: str, page: Page, **kwargs) -> str:
    if os.environ.get(PDF_ONLY_ENV, "").lower() in {"1", "true", "yes"}:
        if page.file.src_path != "generated_manual.md":
            page.meta["pdf"] = False
        else:
            timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
            markdown_text = markdown_text.replace("{{ GENERATED_AT }}", timestamp)
    return markdown_text
