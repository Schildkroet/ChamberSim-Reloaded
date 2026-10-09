#pragma once

#include "solver.h"

#include <QString>
#include <QVariantMap>

#include <optional>
#include <vector>

namespace csr
{

// One result file written by the reference runner (tools/GRTRef): the full
// curves and the summary values of the original program for one load.
struct ReferenceRun
{
    QString job;
    bool stale{};  // the runner could not confirm it read the right tab
    std::vector<SimPoint> points;
    QVariantMap summary;  // raw strings, e.g. "MaxPressure" -> "2290.07 bar"

    // Leading number of a summary value ("2290.07 bar" -> 2290.07), or `fallback`.
    [[nodiscard]] double number(const QString &key, double fallback = 0.0) const;
};

[[nodiscard]] std::optional<ReferenceRun> readReferenceRun(const QString &jsonPath, QString *error = nullptr);

}  // namespace csr
