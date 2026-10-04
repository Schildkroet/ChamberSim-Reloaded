#pragma once

#include "dbfields.h"

#include <QString>

namespace recordfields
{

// The unit a field is shown in (null: none), from the user's unit settings.
[[nodiscard]] const grt::units::Unit *unitOf(const grt::dbfields::Field &f);

// A stored number as shown to the user: the unit's usual decimals, more only where the value has them; no thousands
// separators (used in the input fields of the record dialog and in the record viewer).
[[nodiscard]] QString shownNumber(const grt::dbfields::Field &f, double stored);

}  // namespace recordfields
