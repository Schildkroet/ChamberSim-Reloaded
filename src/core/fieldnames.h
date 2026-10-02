#pragma once

#include <QString>

namespace grt
{

// The label of a load-file field in the user's language. The files describe their fields in English ("case volume",
// attribute descr); the known descriptions are translated (context "grt::fields"). Returns the description itself
// when there is no translation (English, or a field this program does not know).
[[nodiscard]] QString translatedDescription(const QString &descr);

}  // namespace grt
