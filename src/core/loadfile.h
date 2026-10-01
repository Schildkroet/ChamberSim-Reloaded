#pragma once

#include <QString>
#include <QStringView>

#include <optional>
#include <vector>

namespace grt {

// One <input .../> (or <var .../>, <config .../>) element.
// Strings are stored percent-decoded; they are re-encoded on write.
struct Entry {
    QString name;
    QString value;
    QString unit;
    QString type;
    QString descr;
};

// An ordered list of entries with typed lookup helpers.
class EntryList {
public:
    [[nodiscard]] const Entry *find(QStringView name) const noexcept;
    [[nodiscard]] QString text(QStringView name, const QString &fallback = {}) const;
    [[nodiscard]] double number(QStringView name, double fallback = 0.0) const;
    [[nodiscard]] bool flag(QStringView name, bool fallback = false) const;

    // Updates the value if the entry exists, appends a new entry otherwise.
    void set(QStringView name, const QString &value);
    void set(QStringView name, double value);

    [[nodiscard]] const std::vector<Entry> &entries() const noexcept { return m_entries; }
    std::vector<Entry> &entries() noexcept { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

// In-memory form of a *.grtload file (<InnerBallistikInput>).
struct LoadFile {
    QString appVersion;
    QString title;
    EntryList config;
    EntryList caliber;
    EntryList caliberFile;  // <caliberfile><var .../></caliberfile>, the full CIP record
    EntryList gun;
    EntryList projectile;
    EntryList propellant;
    QString appendixXml;    // notes, measurements, pictures: kept verbatim (inner XML)

    [[nodiscard]] static std::optional<LoadFile> read(const QString &path, QString *error = nullptr);
    [[nodiscard]] static std::optional<LoadFile> parse(const QByteArray &xml, QString *error = nullptr);

    [[nodiscard]] bool write(const QString &path, QString *error = nullptr) const;
    [[nodiscard]] QByteArray toXml() const;
};

}  // namespace grt
