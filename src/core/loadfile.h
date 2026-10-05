#pragma once

#include <QString>
#include <QStringView>

#include <optional>
#include <vector>

namespace grt
{

// Text values of the file format are percent-encoded (UTF-8); decode leaves text without '%' as it is.
[[nodiscard]] QString percentDecode(const QString &s);
[[nodiscard]] QString percentEncode(const QString &s);

// One <input .../> (or <var .../>, <config .../>) element.
// Strings are stored percent-decoded; they are re-encoded on write.
struct Entry
{
    QString name;
    QString value;
    QString unit;
    QString type;
    QString descr;
};

// An ordered list of entries with typed lookup helpers.
class EntryList
{
public:
    [[nodiscard]] const Entry *find(QStringView name) const noexcept;
    [[nodiscard]] QString text(QStringView name, const QString &fallback = {}) const;
    // The value as a finite number; `fallback` if missing, not a number, NaN or infinite.
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

// The two file formats of a load. *.grtreload is this program's own (<GRTReloaded format="1"><load>), *.grtload is the
// original program's (<GordonsReloadingTool><InnerBallistikInput>): both are read, the original's is only written as an export.
// This program's data that the original does not know (physical burn values, shotshell parts) has its own sections
// (<physical>, <shotshell>) in *.grtreload; the export leaves it out. In memory it is in the propellant / caliber entries.
enum class FileFormat
{
    Reload,
    Original,
};

inline constexpr auto kReloadSuffix = "grtreload";
inline constexpr auto kOriginalSuffix = "grtload";

// The format a file name stands for: *.grtload is the original's, everything else this program's.
[[nodiscard]] FileFormat formatForPath(const QString &path);

// In-memory form of a load file (*.grtreload, or the original's *.grtload).
struct LoadFile
{
    QString appVersion;
    FileFormat source = FileFormat::Reload;  // the format it was read from (a new load is in this program's)
    QString title;
    EntryList config;
    EntryList caliber;
    EntryList caliberFile;  // <caliberfile><var .../></caliberfile>, the full CIP record
    EntryList gun;
    EntryList projectile;
    EntryList propellant;
    QString appendixXml;  // notes, measurements, pictures: kept verbatim (inner XML)

    [[nodiscard]] static std::optional<LoadFile> read(const QString &path, QString *error = nullptr);
    [[nodiscard]] static std::optional<LoadFile> parse(const QByteArray &xml, QString *error = nullptr);

    // Writes the format the file name stands for (see formatForPath).
    [[nodiscard]] bool write(const QString &path, QString *error = nullptr) const;
    [[nodiscard]] QByteArray toXml(FileFormat format = FileFormat::Reload) const;
};

}  // namespace grt
