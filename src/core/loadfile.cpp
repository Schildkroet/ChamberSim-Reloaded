#include "loadfile.h"

#include "numeric.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <functional>
#include <utility>

using namespace Qt::StringLiterals;

namespace csr
{

QString percentDecode(const QString &s)
{
    return s.contains(u'%') ? QUrl::fromPercentEncoding(s.toUtf8()) : s;
}

QString percentEncode(const QString &s)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(s));
}

namespace
{

[[nodiscard]] bool isTextual(const Entry &e)
{
    return e.type.isEmpty() || e.type == u"string";
}

Entry readEntry(const QXmlStreamAttributes &a)
{
    return {.name = a.value(u"name"_s).toString(),
            .value = percentDecode(a.value(u"value"_s).toString()),
            .unit = a.value(u"unit"_s).toString(),
            .type = a.value(u"type"_s).toString(),
            .descr = a.value(u"descr"_s).toString()};
}

void writeEntry(QXmlStreamWriter &w, const QString &tag, const Entry &e)
{
    w.writeEmptyElement(tag);
    w.writeAttribute(u"name"_s, e.name);
    w.writeAttribute(u"value"_s, isTextual(e) ? percentEncode(e.value) : e.value);
    if (!e.type.isEmpty() || !e.unit.isEmpty() || !e.descr.isEmpty())
    {
        w.writeAttribute(u"unit"_s, e.unit);
        w.writeAttribute(u"type"_s, e.type);
        w.writeAttribute(u"descr"_s, e.descr);
    }
}

// Copies the remaining content of the current element (without its end tag) to the writer.
void copyChildren(QXmlStreamReader &r, QXmlStreamWriter &w)
{
    int depth = 0;
    while (!r.atEnd())
    {
        switch (r.readNext())
        {
        case QXmlStreamReader::StartElement:
            w.writeStartElement(r.name().toString());
            w.writeAttributes(r.attributes());
            ++depth;
            break;
        case QXmlStreamReader::EndElement:
            if (depth == 0) return;
            w.writeEndElement();
            --depth;
            break;
        case QXmlStreamReader::Characters:
            if (!r.isWhitespace()) w.writeCharacters(r.text().toString());
            break;
        default: break;
        }
    }
}

void readList(QXmlStreamReader &r, EntryList &list, QStringView childTag)
{
    while (r.readNextStartElement())
    {
        if (r.name() == childTag) list.entries().push_back(readEntry(r.attributes()));
        r.skipCurrentElement();
    }
}

// The data this program adds to the original's sections: the physical model's burn values of the propellant (physBa, physA0, ...) and
// the parts of a shotshell (shothull, shotwad, shotprimer). In a *.csrload they have sections of their own, the original's
// export leaves them out.
bool isPhysical(const Entry &e)
{
    return e.name.startsWith(u"phys");
}

bool isShotPart(const Entry &e)
{
    return e.name == u"shothull" || e.name == u"shotwad" || e.name == u"shotprimer";
}

// The entries of `list` for which `wanted` is true, as <tag> with <childTag> children; no element when there are none and `optional`.
template <class Pred>
void writeList(QXmlStreamWriter &w, const QString &tag, const EntryList &list, const QString &childTag, Pred wanted, bool optional = false)
{
    if (optional && std::ranges::none_of(list.entries(), wanted)) return;
    w.writeStartElement(tag);
    for (const Entry &e : list.entries())
        if (wanted(e)) writeEntry(w, childTag, e);
    w.writeEndElement();
}

void writeList(QXmlStreamWriter &w, const QString &tag, const EntryList &list, const QString &childTag)
{
    writeList(w, tag, list, childTag, [](const Entry &) { return true; });
}

}  // namespace

// ---- EntryList ------------------------------------------------------------

const Entry *EntryList::find(QStringView name) const noexcept
{
    const auto it = std::ranges::find_if(m_entries, [&](const Entry &e) { return e.name == name; });
    return it == m_entries.end() ? nullptr : &*it;
}

Entry *EntryList::find(QStringView name) noexcept
{
    return const_cast<Entry *>(std::as_const(*this).find(name));
}

QString EntryList::text(QStringView name, const QString &fallback) const
{
    const Entry *e = find(name);
    return e ? e->value : fallback;
}

double EntryList::number(QStringView name, double fallback) const
{
    const Entry *e = find(name);
    if (!e) return fallback;
    return parseFinite(e->value).value_or(fallback);
}

bool EntryList::flag(QStringView name, bool fallback) const
{
    const Entry *e = find(name);
    if (!e) return fallback;
    return e->value.compare(u"true", Qt::CaseInsensitive) == 0 || e->value == u"1";
}

void EntryList::set(QStringView name, const QString &value)
{
    if (Entry *e = find(name)) e->value = value;
    else m_entries.push_back({.name = name.toString(), .value = value, .unit = {}, .type = {}, .descr = {}});
}

void EntryList::set(QStringView name, double value)
{
    set(name, QString::number(value, 'g', 17));
}

// ---- LoadFile -------------------------------------------------------------

std::optional<LoadFile> LoadFile::read(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = f.errorString();
        return std::nullopt;
    }
    return parse(f.readAll(), error);
}

std::optional<LoadFile> LoadFile::parse(const QByteArray &xml, QString *error)
{
    QXmlStreamReader r(xml);
    LoadFile lf;
    bool found = false;

    if (!r.readNextStartElement() || (r.name() != u"ChamberSimReloaded" && r.name() != u"GordonsReloadingTool"))
    {
        if (error) *error = QCoreApplication::translate("csr::LoadFile", "Not a ChamberSim Reloaded or GRT load file");
        return std::nullopt;
    }
    lf.source = r.name() == u"ChamberSimReloaded" ? FileFormat::Reload : FileFormat::Original;
    const QStringView loadTag = lf.source == FileFormat::Reload ? u"load" : u"InnerBallistikInput";
    lf.appVersion = r.attributes().value(u"version"_s).toString();

    while (r.readNextStartElement())
    {
        if (r.name() != loadTag)
        {
            r.skipCurrentElement();
            continue;
        }
        found = true;
        while (r.readNextStartElement())
        {
            const auto tag = r.name();
            if (tag == u"title")
            {
                lf.title = percentDecode(r.readElementText());
            }
            else if (tag == u"config")
            {
                lf.config.entries().push_back(readEntry(r.attributes()));
                r.skipCurrentElement();
            }
            else if (tag == u"caliber")
            {
                readList(r, lf.caliber, u"input");
            }
            else if (tag == u"caliberfile")
            {
                readList(r, lf.caliberFile, u"var");
            }
            else if (tag == u"gun")
            {
                readList(r, lf.gun, u"input");
            }
            else if (tag == u"projectile")
            {
                readList(r, lf.projectile, u"input");
            }
            else if (tag == u"propellant")
            {
                readList(r, lf.propellant, u"input");
            }
            else if (tag == u"shotshell")
            {
                readList(r, lf.caliber, u"input");
            }
            else if (tag == u"physical")
            {
                readList(r, lf.propellant, u"input");
            }
            else if (tag == u"appendix")
            {
                QByteArray inner;
                QXmlStreamWriter w(&inner);
                w.setAutoFormatting(true);
                w.setAutoFormattingIndent(2);
                copyChildren(r, w);
                lf.appendixXml = QString::fromUtf8(inner);
            }
            else
            {
                r.skipCurrentElement();
            }
        }
    }

    if (r.hasError())
    {
        if (error) *error = r.errorString();
        return std::nullopt;
    }
    if (!found)
    {
        if (error) *error = QCoreApplication::translate("csr::LoadFile", "The file contains no load data (<%1> is missing)").arg(loadTag);
        return std::nullopt;
    }
    return lf;
}

FileFormat formatForPath(const QString &path)
{
    return QFileInfo(path).suffix().compare(QLatin1String(kOriginalSuffix), Qt::CaseInsensitive) == 0 ? FileFormat::Original
                                                                                                      : FileFormat::Reload;
}

QByteArray LoadFile::toXml(FileFormat format) const
{
    const bool original = format == FileFormat::Original;
    QByteArray out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(2);
    w.writeStartDocument(u"1.0"_s, true);
    if (original)
    {
        w.writeStartElement(u"GordonsReloadingTool"_s);
        if (!appVersion.isEmpty()) w.writeAttribute(u"version"_s, appVersion);
    }
    else
    {
        w.writeStartElement(u"ChamberSimReloaded"_s);
        w.writeAttribute(u"format"_s, u"1"_s);
        if (const QString v = QCoreApplication::applicationVersion(); !v.isEmpty()) w.writeAttribute(u"version"_s, v);
    }
    w.writeStartElement(original ? u"InnerBallistikInput"_s : u"load"_s);
    w.writeTextElement(u"title"_s, percentEncode(title));
    for (const Entry &e : config.entries()) writeEntry(w, u"config"_s, e);
    writeList(w, u"caliber"_s, caliber, u"input"_s, std::not_fn(isShotPart));
    writeList(w, u"caliberfile"_s, caliberFile, u"var"_s);
    writeList(w, u"gun"_s, gun, u"input"_s);
    writeList(w, u"projectile"_s, projectile, u"input"_s);
    writeList(w, u"propellant"_s, propellant, u"input"_s, std::not_fn(isPhysical));
    if (!original)
    {
        writeList(w, u"shotshell"_s, caliber, u"input"_s, isShotPart, true);
        writeList(w, u"physical"_s, propellant, u"input"_s, isPhysical, true);
    }
    w.writeStartElement(u"appendix"_s);
    if (!appendixXml.isEmpty())
    {
        // appendix content is already well-formed XML: splice it in verbatim
        QXmlStreamReader ar(u"<a>"_s + appendixXml + u"</a>"_s);
        ar.readNextStartElement();
        copyChildren(ar, w);
    }
    w.writeEndElement();  // appendix
    w.writeEndElement();  // load
    w.writeEndElement();  // root
    w.writeEndDocument();
    return out;
}

bool LoadFile::write(const QString &path, QString *error) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(toXml(formatForPath(path))) < 0 || !f.commit())
    {
        if (error) *error = f.errorString();
        return false;
    }
    return true;
}

}  // namespace csr
