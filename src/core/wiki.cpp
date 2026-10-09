#include "wiki.h"

#include <QHash>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace csr::report
{

namespace
{

// value * num / den for image sizes, in double (template sizes like 99999x99999 overflow int); 0 if den <= 0
int scaled(int value, int num, int den)
{
    if (den <= 0) return 0;
    return static_cast<int>(std::clamp(static_cast<double>(value) * num / den, 0.0, 100'000.0));
}

QString colorOf(const QString &name)
{
    static const QHash<QString, QString> names{{u"red"_s, u"#e02020"_s},
                                               {u"lightred"_s, u"#ff8080"_s},
                                               {u"darkred"_s, u"#800000"_s},
                                               {u"green"_s, u"#20a020"_s},
                                               {u"lightgreen"_s, u"#80ff80"_s},
                                               {u"darkgreen"_s, u"#006000"_s},
                                               {u"blue"_s, u"#2040e0"_s},
                                               {u"lightblue"_s, u"#8099ff"_s},
                                               {u"darkblue"_s, u"#000080"_s},
                                               {u"orange"_s, u"#ff8000"_s},
                                               {u"lightorange"_s, u"#ffc080"_s},
                                               {u"darkorange"_s, u"#b05000"_s},
                                               {u"grey"_s, u"#808080"_s},
                                               {u"lightgrey"_s, u"#c0c0c0"_s},
                                               {u"darkgrey"_s, u"#404040"_s},
                                               {u"yellow"_s, u"#e0e000"_s},
                                               {u"lightyellow"_s, u"#ffff90"_s},
                                               {u"darkyellow"_s, u"#808000"_s},
                                               {u"magenta"_s, u"#e000e0"_s},
                                               {u"lightmagenta"_s, u"#ff90ff"_s},
                                               {u"darkmagenta"_s, u"#800080"_s},
                                               {u"cyan"_s, u"#00c0c0"_s},
                                               {u"lightcyan"_s, u"#90ffff"_s},
                                               {u"darkcyan"_s, u"#006060"_s},
                                               {u"black"_s, u"#000000"_s},
                                               {u"white"_s, u"#ffffff"_s},
                                               // names of the program's color scheme
                                               {u"color_diagram_pressure"_s, u"#d05050"_s},
                                               {u"color_diagram_energy"_s, u"#20a090"_s},
                                               {u"color_diagram_velocity"_s, u"#5060d0"_s},
                                               {u"color_diagram_range"_s, u"#a06020"_s},
                                               {u"color_window_infotext"_s, u"#606060"_s}};
    const QString n = name.trimmed().toLower();
    if (const auto it = names.constFind(n); it != names.constEnd()) return it.value();
    static const QRegularExpression hex(u"^#[0-9a-f]{6}$"_s, QRegularExpression::CaseInsensitiveOption);
    return hex.match(n).hasMatch() ? n : QString();
}

QString escape(const QString &s)
{
    return s.toHtmlEscaped();
}

// ---- images ----
QString imageHtml(const QString &inner, const ImageResolver &images, int pageWidth)
{
    // inner is the text between {{ and }}: [spaces]name[?params][spaces]
    const bool alignRight = inner.startsWith(u' ');
    const bool alignLeft = inner.endsWith(u' ');
    const QString body = inner.trimmed();
    const qsizetype q = body.indexOf(u'?');
    const QString name = q < 0 ? body : body.left(q);
    const QString params = q < 0 ? QString() : body.mid(q + 1);

    const ImageSource image = images ? images(name) : ImageSource();
    if (image.src.isEmpty()) return u"<i>[%1]</i>"_s.arg(escape(name));

    // requested size: "80" (width in px), "15%" (of the page width), "100x50", or none (natural, at most page width)
    int width = image.width, height = image.height;
    QString link;
    for (const QString &part : params.split(u'&', Qt::SkipEmptyParts))
    {
        static const QRegularExpression whRe(u"^(\\d+)x(\\d+)$"_s), wRe(u"^(\\d+)$"_s), pRe(u"^(\\d+)%$"_s);
        if (const auto m = whRe.match(part); m.hasMatch())
        {
            width = m.captured(1).toInt();
            height = m.captured(2).toInt();
        }
        else if (const auto m2 = wRe.match(part); m2.hasMatch())
        {
            const int w = m2.captured(1).toInt();
            height = scaled(height, w, width);
            width = w;
        }
        else if (const auto m3 = pRe.match(part); m3.hasMatch())
        {
            const int w = scaled(pageWidth, m3.captured(1).toInt(), 100);
            height = scaled(height, w, width);
            width = w;
        }
        else if (part.startsWith(u"link=")) link = part.mid(5);
    }
    if (width > pageWidth && pageWidth > 0)
    {  // never wider than the page
        height = scaled(height, pageWidth, width);
        width = pageWidth;
    }
    QString size;
    if (width > 0) size += u" width=\"%1\""_s.arg(width);
    if (height > 0) size += u" height=\"%1\""_s.arg(height);
    QString align;
    if (alignLeft && !alignRight) align = u" align=\"left\""_s;
    else if (alignRight && !alignLeft) align = u" align=\"right\""_s;
    QString img = u"<img src=\"%1\"%2%3>"_s.arg(image.src, size, align);
    if (!link.isEmpty()) img = u"<a href=\"%1\">%2</a>"_s.arg(escape(link), img);
    return img;
}

// ---- inline formatting ----
QString inlineHtml(const QString &raw, const ImageResolver &images, int pageWidth)
{
    // images first (they contain characters that the later rules would touch)
    QStringList imageHtmls;
    QString text = raw;
    {
        static const QRegularExpression re(u"\\{\\{(.+?)\\}\\}"_s);
        QString out;
        qsizetype pos = 0;
        for (auto it = re.globalMatch(text); it.hasNext();)
        {
            const auto m = it.next();
            out += text.mid(pos, m.capturedStart() - pos);
            out += QChar(static_cast<char16_t>(0xE000 + imageHtmls.size()));  // private-use placeholder
            imageHtmls << imageHtml(m.captured(1), images, pageWidth);
            pos = m.capturedEnd();
        }
        text = out + text.mid(pos);
    }
    // monospace spans keep their content literal
    QStringList monoSpans;
    {
        static const QRegularExpression re(u"''(.+?)''"_s);
        QString out;
        qsizetype pos = 0;
        for (auto it = re.globalMatch(text); it.hasNext();)
        {
            const auto m = it.next();
            out += text.mid(pos, m.capturedStart() - pos);
            out += QChar(static_cast<char16_t>(0xE100 + monoSpans.size()));
            monoSpans << u"<tt>%1</tt>"_s.arg(escape(m.captured(1)));
            pos = m.capturedEnd();
        }
        text = out + text.mid(pos);
    }
    // links [[target|text]]
    QStringList links;
    {
        static const QRegularExpression re(u"\\[\\[(.+?)\\]\\]"_s);
        QString out;
        qsizetype pos = 0;
        for (auto it = re.globalMatch(text); it.hasNext();)
        {
            const auto m = it.next();
            out += text.mid(pos, m.capturedStart() - pos);
            out += QChar(static_cast<char16_t>(0xE200 + links.size()));
            const QString target = m.captured(1).section(u'|', 0, 0).trimmed();
            const QString label = m.captured(1).contains(u'|') ? m.captured(1).section(u'|', 1) : target;
            const QString href = target.contains(u'@') && !target.contains(u':') ? u"mailto:"_s + target : target;
            links << u"<a href=\"%1\">%2</a>"_s.arg(escape(href), escape(label));
            pos = m.capturedEnd();
        }
        text = out + text.mid(pos);
    }

    text = escape(text);

    // tags: <color X> </color> <size N%> <size=N> </size>
    static const QRegularExpression colorOpen(u"&lt;color\\s+([^&]+?)&gt;"_s), sizeOpen(u"&lt;size[ =]\\s*(\\d+)(%?)&gt;"_s);
    text.replace(u"&lt;/color&gt;"_s, u"</span>"_s);
    text.replace(u"&lt;/size&gt;"_s, u"</span>"_s);
    {
        QString out;
        qsizetype pos = 0;
        for (auto it = colorOpen.globalMatch(text); it.hasNext();)
        {
            const auto m = it.next();
            out += text.mid(pos, m.capturedStart() - pos);
            const QString c = colorOf(m.captured(1));
            out += c.isEmpty() ? u"<span>"_s : u"<span style=\"color:%1\">"_s.arg(c);
            pos = m.capturedEnd();
        }
        text = out + text.mid(pos);
        out.clear();
        pos = 0;
        for (auto it = sizeOpen.globalMatch(text); it.hasNext();)
        {
            const auto m = it.next();
            out += text.mid(pos, m.capturedStart() - pos);
            const double n = m.captured(1).toDouble();
            const double pt = m.captured(2) == u"%" ? 10.0 * n / 100.0 : n;  // percentages refer to a 10 pt base
            out += u"<span style=\"font-size:%1pt\">"_s.arg(pt);
            pos = m.capturedEnd();
        }
        text = out + text.mid(pos);
    }

    // emphasis (no nesting problems for the combinations the manual shows)
    static const QRegularExpression bold(u"\\*\\*(.+?)\\*\\*"_s), italic(u"(?<!:)//(.+?)//"_s), underline(u"__(.+?)__"_s);
    text.replace(bold, u"<b>\\1</b>"_s);
    text.replace(italic, u"<i>\\1</i>"_s);
    text.replace(underline, u"<u>\\1</u>"_s);

    // forced line break: two backslashes at the end of the line or followed by whitespace
    static const QRegularExpression br(u"\\\\\\\\(?=\\s|$)"_s);
    text.replace(br, u"<br>"_s);

    // typography
    text.replace(u"&lt;-&gt;"_s, u"↔"_s).replace(u"-&gt;"_s, u"→"_s).replace(u"&lt;-"_s, u"←"_s);
    text.replace(u"&lt;=&gt;"_s, u"⇔"_s).replace(u"=&gt;"_s, u"⇒"_s).replace(u"&lt;="_s, u"⇐"_s);
    text.replace(u"&gt;&gt;"_s, u"»"_s).replace(u"&lt;&lt;"_s, u"«"_s);
    text.replace(u"---"_s, u"—"_s).replace(u"--"_s, u"–"_s);
    text.replace(u"(c)"_s, u"©"_s).replace(u"(tm)"_s, u"™"_s).replace(u"(r)"_s, u"®"_s);

    // footnotes ((text)) as a small inline note
    static const QRegularExpression foot(u"\\(\\((.+?)\\)\\)"_s);
    text.replace(foot, u"<small>(\\1)</small>"_s);

    // put the protected pieces back
    for (int i = 0; i < imageHtmls.size(); ++i) text.replace(QChar(static_cast<char16_t>(0xE000 + i)), imageHtmls[i]);
    for (int i = 0; i < monoSpans.size(); ++i) text.replace(QChar(static_cast<char16_t>(0xE100 + i)), monoSpans[i]);
    for (int i = 0; i < links.size(); ++i) text.replace(QChar(static_cast<char16_t>(0xE200 + i)), links[i]);
    return text;
}

// ---- tables ----
QString tableHtml(const QStringList &rows, const ImageResolver &images, int pageWidth)
{
    QString html = u"<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">\n"_s;
    for (const QString &row : rows)
    {
        const QString line = row.trimmed();
        // cells are delimited by ^ and |; the delimiter before a cell decides header or normal
        struct Cell
        {
            bool header;
            QString text;
            int span;
        };
        QList<Cell> cells;
        QString current;
        bool header = line.startsWith(u'^');
        for (qsizetype i = 1; i < line.size(); ++i)
        {
            const QChar c = line[i];
            if (c == u'^' || c == u'|')
            {  // every delimiter closes the cell before it
                cells.push_back({header, current, 1});
                current.clear();
                header = c == u'^';
            }
            else
            {
                current += c;
            }
        }
        if (!current.trimmed().isEmpty()) cells.push_back({header, current, 1});  // a row without closing delimiter
        // an empty cell (two delimiters in a row) extends the cell before it
        QList<Cell> merged;
        for (const Cell &c : cells)
        {
            if (c.text.isEmpty() && !merged.isEmpty()) ++merged.last().span;
            else merged.push_back(c);
        }
        html += u"<tr>"_s;
        for (const Cell &c : merged)
        {
            const QString tag = c.header ? u"th"_s : u"td"_s;
            html += u"<%1%2>%3</%1>"_s.arg(tag, c.span > 1 ? u" colspan=\"%1\""_s.arg(c.span) : QString(),
                                           inlineHtml(c.text.trimmed(), images, pageWidth));
        }
        html += u"</tr>\n"_s;
    }
    return html + u"</table>\n"_s;
}

}  // namespace

QString wikiToHtml(const QString &source, const ImageResolver &images, int pageWidth)
{
    QString text = source;
    text.replace(u"\r\n"_s, u"\n"_s).replace(u'\r', u'\n');
    const QStringList lines = text.split(u'\n');

    QString html;
    QStringList paragraph;
    const auto flushParagraph = [&]
    {
        if (paragraph.isEmpty()) return;
        html += u"<p>%1</p>\n"_s.arg(inlineHtml(paragraph.join(u'\n'), images, pageWidth).replace(u'\n', u' '));
        paragraph.clear();
    };

    static const QRegularExpression headRe(u"^\\s*(={2,6})\\s*(.+?)\\s*=*\\s*$"_s), listRe(u"^(\\s{2,})([*-])\\s+(.*)$"_s);
    QList<int> listDepth;  // open <ul> levels
    const auto closeLists = [&](int toDepth = 0)
    {
        while (static_cast<int>(listDepth.size()) > toDepth)
        {
            html += u"</ul>\n"_s;
            listDepth.removeLast();
        }
    };

    for (qsizetype i = 0; i < lines.size(); ++i)
    {
        const QString &line = lines[i];
        const QString trimmed = line.trimmed();

        if (trimmed.startsWith(u"<code>"))
        {  // literal block
            flushParagraph();
            closeLists();
            QStringList code;
            QString first = trimmed.mid(6);
            if (first.contains(u"</code>"))
            {
                code << first.left(first.indexOf(u"</code>"));
            }
            else
            {
                if (!first.isEmpty()) code << first;
                for (++i; i < lines.size() && !lines[i].contains(u"</code>"); ++i) code << lines[i];
                if (i < lines.size()) code << lines[i].left(lines[i].indexOf(u"</code>"));
            }
            while (!code.isEmpty() && code.first().trimmed().isEmpty()) code.removeFirst();
            while (!code.isEmpty() && code.last().trimmed().isEmpty()) code.removeLast();
            html += u"<pre>%1</pre>\n"_s.arg(escape(code.join(u'\n')));
            continue;
        }
        if (trimmed.isEmpty())
        {
            flushParagraph();
            closeLists();
            continue;
        }
        if (trimmed == u"<pagebreak>")
        {  // a page break for printing (not part of the original's markup)
            flushParagraph();
            closeLists();
            html += u"<p style=\"page-break-before:always\">&nbsp;</p>\n"_s;
            continue;
        }
        if (trimmed == u"----" || (trimmed.size() >= 4 && trimmed.count(u'-') == trimmed.size()))
        {
            flushParagraph();
            closeLists();
            html += u"<hr>\n"_s;
            continue;
        }
        if (const auto m = headRe.match(line); m.hasMatch() && line.trimmed().startsWith(u'=') && m.captured(1).size() >= 2)
        {
            flushParagraph();
            closeLists();
            const int level = std::clamp(7 - static_cast<int>(m.captured(1).size()), 1, 5);  // ====== is level 1
            html += u"<h%1>%2</h%1>\n"_s.arg(level).arg(inlineHtml(m.captured(2), images, pageWidth));
            continue;
        }
        if (trimmed.startsWith(u'^') || trimmed.startsWith(u'|'))
        {  // table: collect consecutive rows
            flushParagraph();
            closeLists();
            QStringList rows;
            for (; i < lines.size(); ++i)
            {
                const QString t = lines[i].trimmed();
                if (!(t.startsWith(u'^') || t.startsWith(u'|'))) break;
                rows << t;
            }
            --i;
            html += tableHtml(rows, images, pageWidth);
            continue;
        }
        if (const auto m = listRe.match(line); m.hasMatch() && m.captured(2) == u"*")
        {
            flushParagraph();
            const int depth = static_cast<int>(m.captured(1).size() / 2);
            if (listDepth.isEmpty() || listDepth.last() < depth)
            {
                listDepth.push_back(depth);
                html += u"<ul>\n"_s;
            }
            else
            {
                while (!listDepth.isEmpty() && listDepth.last() > depth)
                {
                    html += u"</ul>\n"_s;
                    listDepth.removeLast();
                }
            }
            html += u"<li>%1</li>\n"_s.arg(inlineHtml(m.captured(3), images, pageWidth));
            continue;
        }
        closeLists();
        paragraph << line;
    }
    flushParagraph();
    closeLists();
    return html;
}

}  // namespace csr::report
