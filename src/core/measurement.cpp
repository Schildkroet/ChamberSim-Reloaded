#include "measurement.h"

#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace grt {

namespace {

QString decode(const QString &s)
{
    return s.contains(u'%') ? QUrl::fromPercentEncoding(s.toUtf8()) : s;
}

QString encode(const QString &s)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(s));
}

QString number(double v)
{
    return QString::number(v, 'g', 17);
}

RawAttributes rawAttributes(const QXmlStreamAttributes &a)
{
    RawAttributes out;
    out.reserve(static_cast<std::size_t>(a.size()));
    for (const QXmlStreamAttribute &x : a) out.emplace_back(x.qualifiedName().toString(), x.value().toString());
    return out;
}

QString attribute(const RawAttributes &a, QStringView name)
{
    const auto it = std::ranges::find_if(a, [&](const auto &p) { return p.first == name; });
    return it == a.end() ? QString() : it->second;
}

// Writes the attributes in their original order; `own` replaces values (and is appended where missing).
void writeAttributes(QXmlStreamWriter &w, const RawAttributes &raw, const RawAttributes &own)
{
    for (const auto &[name, value] : raw) {
        const auto it = std::ranges::find_if(own, [&](const auto &p) { return p.first == name; });
        w.writeAttribute(name, it == own.end() ? value : it->second);
    }
    for (const auto &[name, value] : own)
        if (std::ranges::none_of(raw, [&](const auto &p) { return p.first == name; })) w.writeAttribute(name, value);
}

// Copies the element the reader is positioned on (a StartElement) with everything inside it.
void copyElement(QXmlStreamReader &r, QXmlStreamWriter &w)
{
    int depth = 0;
    do {
        if (r.isStartElement()) ++depth;
        if (r.isEndElement()) --depth;
        w.writeCurrentToken(r);
        if (depth == 0) break;
    } while (!r.atEnd() && (r.readNext(), true));
}

MeasuredCharge readCharge(QXmlStreamReader &r)
{
    MeasuredCharge c;
    c.attributes = rawAttributes(r.attributes());
    c.name = decode(attribute(c.attributes, u"name"));
    c.note = decode(attribute(c.attributes, u"note"));
    c.charge_kg = attribute(c.attributes, u"value").toDouble();
    QXmlStreamWriter extra(&c.extraXml);
    while (r.readNextStartElement()) {
        if (r.name() == u"shot") {
            const RawAttributes a = rawAttributes(r.attributes());
            c.shots.push_back({.name = decode(attribute(a, u"name")),
                               .velocity_ms = attribute(a, u"velocity").toDouble(),
                               .pressure_bar = attribute(a, u"pressure").toDouble()});
            r.skipCurrentElement();
        } else {
            copyElement(r, extra);
        }
    }
    return c;
}

void writeBlock(QXmlStreamWriter &w, const Measurement &m)
{
    w.writeStartElement(u"Measurement"_s);
    writeAttributes(w, m.attributes, {{u"title"_s, encode(m.title)}});
    for (const MeasuredCharge &c : m.charges) {
        w.writeStartElement(u"charge"_s);
        writeAttributes(w, c.attributes, {{u"name"_s, encode(c.name)}, {u"note"_s, encode(c.note)}, {u"value"_s, number(c.charge_kg)}});
        for (const Shot &s : c.shots) {
            w.writeEmptyElement(u"shot"_s);
            w.writeAttribute(u"name"_s, encode(s.name));
            w.writeAttribute(u"velocity"_s, number(s.velocity_ms));
            w.writeAttribute(u"pressure"_s, number(s.pressure_bar));
        }
        if (!c.extraXml.isEmpty()) {
            QXmlStreamReader er(u"<x>"_s + c.extraXml + u"</x>"_s);
            er.readNextStartElement();  // <x>
            while (er.readNextStartElement()) copyElement(er, w);
        }
        w.writeEndElement();
    }
    w.writeEndElement();
}

}  // namespace

MeasuredCharge MeasuredCharge::create(const QString &name, double charge_g)
{
    MeasuredCharge c;
    c.name = name;
    c.setCharge_g(charge_g);
    c.attributes = {{u"name"_s, {}}, {u"showinreport"_s, u"true"_s}, {u"expanded"_s, u"true"_s},
                    {u"expandedstats"_s, u"false"_s}, {u"note"_s, {}}, {u"value"_s, {}}, {u"menu"_s, {}},
                    {u"menuvalue"_s, {}}, {u"menuunit"_s, u"m"_s}, {u"source"_s, {}}, {u"sourceoptions"_s, u"%7B%7D"_s}};
    return c;
}

ShotStatistics statistics(std::span<const double> values)
{
    ShotStatistics s;
    double sum = 0.0;
    for (double v : values) {
        if (!(v > 0.0)) continue;
        s.min = s.count == 0 ? v : std::min(s.min, v);
        s.max = s.count == 0 ? v : std::max(s.max, v);
        sum += v;
        ++s.count;
    }
    if (s.count == 0) return s;
    s.mean = sum / s.count;
    if (s.count > 1) {
        double sq = 0.0;
        for (double v : values)
            if (v > 0.0) sq += (v - s.mean) * (v - s.mean);
        s.sd = std::sqrt(sq / (s.count - 1));
    }
    return s;
}

ShotStatistics velocityStatistics(const MeasuredCharge &charge)
{
    std::vector<double> v;
    for (const Shot &s : charge.shots) v.push_back(s.velocity_ms);
    return statistics(v);
}

ShotStatistics pressureStatistics(const MeasuredCharge &charge)
{
    std::vector<double> v;
    for (const Shot &s : charge.shots) v.push_back(s.pressure_bar);
    return statistics(v);
}

std::optional<Measurement> readMeasurement(const QString &appendixXml)
{
    QXmlStreamReader r(u"<a>"_s + appendixXml + u"</a>"_s);
    if (!r.readNextStartElement()) return std::nullopt;
    while (r.readNextStartElement()) {
        if (r.name() != u"Measurement") {
            r.skipCurrentElement();
            continue;
        }
        Measurement m;
        m.attributes = rawAttributes(r.attributes());
        m.title = decode(attribute(m.attributes, u"title"));
        while (r.readNextStartElement()) {
            if (r.name() == u"charge") m.charges.push_back(readCharge(r));
            else r.skipCurrentElement();
        }
        return m;
    }
    return std::nullopt;
}

QString writeMeasurement(const QString &appendixXml, const Measurement &m)
{
    QString out;
    QXmlStreamWriter w(&out);
    QXmlStreamReader r(u"<a>"_s + appendixXml + u"</a>"_s);
    bool replaced = false;
    int lastIndex = 0;
    int depth = 0;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartDocument() || r.isEndDocument()) continue;
        if (r.isStartElement()) {
            if (++depth == 1) continue;  // the wrapper
            if (depth == 2) lastIndex = std::max(lastIndex, r.attributes().value(u"index"_s).toInt());
            if (depth == 2 && !replaced && r.name() == u"Measurement") {
                writeBlock(w, m);
                r.skipCurrentElement();
                --depth;
                replaced = true;
                continue;
            }
        } else if (r.isEndElement()) {
            if (--depth == 0) continue;
        }
        w.writeCurrentToken(r);
    }
    if (!replaced) {
        Measurement added = m;
        if (attribute(added.attributes, u"index").isEmpty())
            added.attributes.insert(added.attributes.begin(), {u"index"_s, QString::number(lastIndex + 1)});
        if (attribute(added.attributes, u"showinreport").isEmpty()) added.attributes.emplace_back(u"showinreport"_s, u"true"_s);
        if (!out.trimmed().isEmpty() && !out.endsWith(u'\n')) out += u'\n';
        writeBlock(w, added);
    }
    return out;
}

}  // namespace grt
