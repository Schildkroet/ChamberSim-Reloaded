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

#include "fieldnames.h"

#include <QCoreApplication>

namespace csr
{

namespace
{

// the descriptions the original program writes into its files (for lupdate; translated at run time)
[[maybe_unused]] constexpr const char *kDescriptions[] = {
    QT_TRANSLATE_NOOP("csr::fields", "G1 ballistic coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "G7 ballistic coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "Sebert factor"),
    QT_TRANSLATE_NOOP("csr::fields", "UBCS-Code (Universal Bullet Classification Scheme)"),
    QT_TRANSLATE_NOOP("csr::fields", "barrel length"),
    QT_TRANSLATE_NOOP("csr::fields", "bulk density"),
    QT_TRANSLATE_NOOP("csr::fields", "bullet jump distance"),
    QT_TRANSLATE_NOOP("csr::fields", "bullet seating depth"),
    QT_TRANSLATE_NOOP("csr::fields", "burn-up limit z1"),
    QT_TRANSLATE_NOOP("csr::fields", "burn-up limit z2"),
    QT_TRANSLATE_NOOP("csr::fields", "caliber name"),
    QT_TRANSLATE_NOOP("csr::fields", "cartridge overall length"),
    QT_TRANSLATE_NOOP("csr::fields", "cartridge test method"),
    QT_TRANSLATE_NOOP("csr::fields", "case length"),
    QT_TRANSLATE_NOOP("csr::fields", "case volume"),
    QT_TRANSLATE_NOOP("csr::fields", "cold temperature coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "covolume of the powder gases"),
    QT_TRANSLATE_NOOP("csr::fields", "effektive area"),
    QT_TRANSLATE_NOOP("csr::fields", "groove caliber"),
    QT_TRANSLATE_NOOP("csr::fields", "gun name"),
    QT_TRANSLATE_NOOP("csr::fields", "gun weight"),
    QT_TRANSLATE_NOOP("csr::fields", "hot temperature coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "initial gas pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "ladder step propellant mass"),
    QT_TRANSLATE_NOOP("csr::fields", "ladder steps (1 to 10, below 2: disabled)"),
    QT_TRANSLATE_NOOP("csr::fields", "material density"),
    QT_TRANSLATE_NOOP("csr::fields", "maximum allowed pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "prog/deg coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile base type"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile diameter"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile lot-id"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile manufacturer"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile material and gas sealing/losses"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile name"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile overall length"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile product name"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail diameter A"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail diameter B"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail length"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile weight"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant detail picture uuid"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant lot-id"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant manufacturer"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant mass"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant picture uuid"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant product name"),
    QT_TRANSLATE_NOOP("csr::fields", "ratio of the specific heat of the powder gases"),
    QT_TRANSLATE_NOOP("csr::fields", "selected ladder curve for display"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell hull"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell primer"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell wad"),
    QT_TRANSLATE_NOOP("csr::fields", "special bullet resistance inside the barrel"),
    QT_TRANSLATE_NOOP("csr::fields", "specific explosive heat"),
    QT_TRANSLATE_NOOP("csr::fields", "technical atmosphere pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "temperature normal"),
    QT_TRANSLATE_NOOP("csr::fields", "true if shotshell"),
    QT_TRANSLATE_NOOP("csr::fields", "twist length"),
    QT_TRANSLATE_NOOP("csr::fields", "unlocked bullet jump"),
    QT_TRANSLATE_NOOP("csr::fields", "unlocked friction"),
    QT_TRANSLATE_NOOP("csr::fields", "vivacity coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: vivacity coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: prog/deg coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: burn-up limit z1"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: burn-up limit z2"),
};

}  // namespace

QString translatedDescription(const QString &descr)
{
    if (descr.isEmpty()) return descr;
    return QCoreApplication::translate("csr::fields", descr.toUtf8().constData());
}

}  // namespace csr
