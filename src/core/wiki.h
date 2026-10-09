#pragma once

#include <QString>

#include <functional>

namespace csr::report
{

// What an image name ({{name}}) resolves to: something an <img src=...> understands (e.g. a data: URI) and,
// if known, the natural size in pixels. An empty `src` shows a text placeholder instead.
struct ImageSource
{
    QString src;
    int width{0};
    int height{0};
    ImageSource() = default;
    ImageSource(QString source, int w = 0, int h = 0)
        : src(std::move(source))
        , width(w)
        , height(h)
    {
    }
};
using ImageResolver = std::function<ImageSource(const QString &name)>;

// Converts the report markup (a DokuWiki-like language, see the original's "Syntax" page) to HTML for
// Qt's rich text engine. Supported: headlines (= ... ======), **bold**, //italic//, __underline__,
// ''monospace'', forced line breaks (\\), paragraphs, lists (two spaces + "*", nested by indentation),
// tables (^ headers, | cells, empty cell = column span), horizontal rule (----), <code> blocks,
// [[url|text]] links, {{image?width|W%|WxH|link}} images (spaces left/right of the name align it),
// <color X> and <size N%|=N> tags, a few typographic replacements (-> => (c) ...), and a line with only
// <pagebreak> starts a new printed page.
// Everything else is escaped, so a template cannot inject markup.
// `pageWidth` is the width available to the text in layout pixels: Qt's rich text does not understand
// percentage widths, so sizes like ?50% are turned into pixels of it (keeping the aspect ratio).
[[nodiscard]] QString wikiToHtml(const QString &text, const ImageResolver &images = {}, int pageWidth = 720);

}  // namespace csr::report
