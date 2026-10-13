/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "projectdata.h"

#include "dates.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace csr::projectdata
{

namespace
{

bool isNumeric(const QString &sqlType)
{
    return sqlType == u"FLOAT" || sqlType == u"INTEGER";
}

QString typeOf(const Schema &schema, const QString &column)
{
    for (const auto &[name, type] : schema)
        if (name == column) return type;
    return {};
}

// Values the simulation needs: they must be > 0.
QStringList positiveColumns(Table t)
{
    switch (t)
    {
    case Table::Caliber: return {u"L3"_s, u"V"_s};
    case Table::Projectile: return {u"gdia"_s, u"glen"_s, u"gmass"_s};
    case Table::Propellant: return {u"Ba"_s, u"Qex"_s, u"k"_s, u"pc"_s, u"pcd"_s};
    }
    return {};
}

}  // namespace

QString folderName(Table t)
{
    switch (t)
    {
    case Table::Caliber: return u"calibers"_s;
    case Table::Projectile: return u"projectiles"_s;
    case Table::Propellant: return u"propellants"_s;
    }
    return {};
}

QStringList check(Table t, const QVariantMap &record, const Schema &schema)
{
    QStringList problems;
    for (auto it = record.begin(); it != record.end(); ++it)
    {
        const QString type = typeOf(schema, it.key());
        if (type.isEmpty())
        {
            problems << u"unknown column \"%1\""_s.arg(it.key());
            continue;
        }
        if (it.key() == u"id" || it.key() == u"geloescht")
        {
            problems << u"\"%1\" must not be in the file"_s.arg(it.key());
            continue;
        }
        const bool number =
            it.value().typeId() == QMetaType::Double || it.value().typeId() == QMetaType::Int || it.value().typeId() == QMetaType::LongLong;
        if (isNumeric(type) && !number) problems << u"\"%1\" must be a number"_s.arg(it.key());
        if (!isNumeric(type) && number) problems << u"\"%1\" must be text"_s.arg(it.key());
        if (type.compare(u"DATE"_s, Qt::CaseInsensitive) == 0 && !number)
        {
            const auto date = normalizedDate(it.value());
            if (!date || *date != it.value().toString().trimmed())
                problems << u"\"%1\" must be a date in the form yyyy-MM-dd (or empty)"_s.arg(it.key());
        }
    }
    const QString name = t == Table::Caliber ? u"cipname"_s : u"pname"_s;
    if (record.value(name).toString().trimmed().isEmpty()) problems << u"\"%1\" (the name) is missing"_s.arg(name);
    for (const QString &c : positiveColumns(t))
        if (!(record.value(c).toDouble() > 0.0)) problems << u"\"%1\" must be greater than 0"_s.arg(c);
    if (t == Table::Propellant)
    {
        const double z1 = record.value(u"z1"_s).toDouble(), z2 = record.value(u"z2"_s).toDouble();
        if (!(z1 > 0.0 && z1 < z2 && z2 < 1.0)) problems << u"the transition limits need 0 < z1 < z2 < 1"_s;
        if (const double q = record.value(u"Qlty"_s).toDouble(); q < 0.0 || q > 1.0)
            problems << u"\"Qlty\" (quality) must be between 0 and 1"_s;
    }
    return problems;
}

Loaded load(const QString &dataDir, Table t, const Schema &schema)
{
    Loaded out;
    const QDir dir(dataDir + u'/' + folderName(t));
    if (!dir.exists()) return out;
    const QStringList files = dir.entryList({u"*.json"_s}, QDir::Files, QDir::Name);
    for (const QString &f : files)
    {
        QFile file(dir.filePath(f));
        const QString where = folderName(t) + u'/' + f;
        if (!file.open(QIODevice::ReadOnly))
        {
            out.errors << where + u": cannot be read"_s;
            continue;
        }
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        if (err.error != QJsonParseError::NoError || (!doc.isObject() && !doc.isArray()))
        {
            out.errors << where + u": not a JSON object or array ("_s + err.errorString() + u')';
            continue;
        }
        // one record, or an array of records (the projectiles: one file per manufacturer)
        const QJsonArray items = doc.isArray() ? doc.array() : QJsonArray{doc.object()};
        for (qsizetype i = 0; i < items.size(); ++i)
        {
            // in an array: "projectiles/acme.json #2 (B2)"
            QString at = doc.isArray() ? where + u" #"_s + QString::number(i + 1) : where;
            if (!items.at(i).isObject())
            {
                out.errors << at + u": not a JSON object"_s;
                continue;
            }
            const QVariantMap record = items.at(i).toObject().toVariantMap();
            const QString name = record.value(t == Table::Caliber ? u"cipname"_s : u"pname"_s).toString();
            if (doc.isArray() && !name.isEmpty()) at += u" ("_s + name + u')';
            const QStringList problems = check(t, record, schema);
            if (!problems.isEmpty())
            {
                for (const QString &p : problems) out.errors << at + u": "_s + p;
                continue;
            }
            out.records.push_back(record);
        }
    }
    return out;
}

QByteArray toJson(const QVariantMap &record, const Schema &schema)
{
    // JSON text of one value ("..." with escapes, or a number)
    const auto encode = [](const QJsonValue &v) { return QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact).mid(1).chopped(1); };
    // written line by line to keep the schema order (QJsonObject would sort the keys)
    QByteArrayList lines;
    for (const auto &[column, type] : schema)
    {
        if (column == u"id" || column == u"geloescht" || !record.contains(column)) continue;
        const QVariant v = record.value(column);
        lines << "  " + encode(column) + ": " + encode(isNumeric(type) ? QJsonValue(v.toDouble()) : QJsonValue(v.toString()));
    }
    return "{\n" + lines.join(",\n") + "\n}\n";
}

QString fileName(Table t, const QVariantMap &record)
{
    QString base = t == Table::Caliber ? record.value(u"cipname"_s).toString()
                                       : record.value(u"mname"_s).toString() + u' ' + record.value(u"pname"_s).toString() + u' ' +
                                             record.value(u"lotid"_s).toString();
    base = base.normalized(QString::NormalizationForm_KD).toLower();
    QString slug;
    for (const QChar c : base)
    {
        if (c.isLetterOrNumber() && c.unicode() < 128) slug += c;
        else if (!slug.isEmpty() && !slug.endsWith(u'-')) slug += u'-';
    }
    while (slug.endsWith(u'-')) slug.chop(1);
    return (slug.isEmpty() ? u"record"_s : slug) + u".json"_s;
}

}  // namespace csr::projectdata
