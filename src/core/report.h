#pragma once

#include "loadfile.h"
#include "solver.h"
#include "units.h"
#include "wiki.h"

#include <QByteArray>
#include <QHash>
#include <QString>

#include <functional>

namespace grt::report {

// A value a report can show: a number (in the file's base unit, with its quantity), text, or a PNG image.
struct Value {
    enum class Kind { Number, Text, Image };
    Kind kind{Kind::Text};
    double number{0.0};
    units::Quantity quantity{units::Quantity::None};
    bool percent{false};  // the number is a fraction (0..1) that is shown as a percentage
    QString text;
    QByteArray png;

    [[nodiscard]] static Value ofNumber(double base, units::Quantity q = units::Quantity::None);
    [[nodiscard]] static Value ofFraction(double fraction);
    [[nodiscard]] static Value ofText(QString text);
    [[nodiscard]] static Value ofImage(QByteArray png);
};

// All values of a report, addressed by "source.property" (case-insensitive).
class Data {
public:
    void set(const QString &key, Value value) { m_values.insert(key.toLower(), std::move(value)); }
    [[nodiscard]] const Value *find(const QString &key) const
    {
        const auto it = m_values.constFind(key.toLower());
        return it == m_values.constEnd() ? nullptr : &it.value();
    }
    [[nodiscard]] qsizetype size() const noexcept { return m_values.size(); }
    [[nodiscard]] QStringList keys() const { return m_values.keys(); }

private:
    QHash<QString, Value> m_values;
};

struct RenderOptions {
    // Unit for automatic formatting (no format string). Default: the base unit.
    std::function<const units::Unit &(units::Quantity)> unitFor;
    // Width of the page in layout pixels; images given in percent are sized against it.
    int pageWidth{720};
};

// Replaces the placeholders ~~source.property~~ and ~~source.property("format")~~ in the text:
//  * without a format string numbers are shown in the user's unit with its symbol ("2287 bar");
//  * with a format string the number is in the file's *base* unit (mm, g, bar, m/s, mm2, cm3, ...) and the
//    unit text is whatever the format contains ("#.000 mm²"), so a template means the same in every unit system;
//  * images become {{name}} and a format string is taken as their size parameters ("?40%");
//  * values that do not exist are shown in red as [source.property?].
[[nodiscard]] QString expandPragmas(const QString &text, const Data &data, const RenderOptions &options = {});

// The complete report as HTML: pragmas expanded, markup converted, images embedded as data: URIs.
[[nodiscard]] QString renderReport(const QString &templateText, const Data &data, const RenderOptions &options = {});

struct FileInfo {
    QString path;  // may be empty for an unsaved load
};

// Fills the sources "file", "work", "result" and "caliber" from a load and its (focus) result. Images
// are added by the caller (they need widgets to render).
[[nodiscard]] Data buildData(const LoadFile &load, const SimInput &input, const SimResult &result, const FileInfo &file);

// The report used when the user has not written one.
[[nodiscard]] QString defaultTemplate();

}  // namespace grt::report
