#include "fieldnames.h"

#include <QCoreApplication>

namespace grt {

namespace {

// the descriptions the original program writes into its files (for lupdate; translated at run time)
[[maybe_unused]] constexpr const char *kDescriptions[] = {
    QT_TRANSLATE_NOOP("grt::fields", "G1 ballistic coefficient"),
    QT_TRANSLATE_NOOP("grt::fields", "G7 ballistic coefficient"),
    QT_TRANSLATE_NOOP("grt::fields", "Sebert factor"),
    QT_TRANSLATE_NOOP("grt::fields", "UBCS-Code (Universal Bullet Classification Scheme)"),
    QT_TRANSLATE_NOOP("grt::fields", "barrel length"),
    QT_TRANSLATE_NOOP("grt::fields", "bulk density"),
    QT_TRANSLATE_NOOP("grt::fields", "bullet jump distance"),
    QT_TRANSLATE_NOOP("grt::fields", "bullet seating depth"),
    QT_TRANSLATE_NOOP("grt::fields", "burn-up limit z1"),
    QT_TRANSLATE_NOOP("grt::fields", "burn-up limit z2"),
    QT_TRANSLATE_NOOP("grt::fields", "caliber name"),
    QT_TRANSLATE_NOOP("grt::fields", "cartridge overall length"),
    QT_TRANSLATE_NOOP("grt::fields", "cartridge test method"),
    QT_TRANSLATE_NOOP("grt::fields", "case length"),
    QT_TRANSLATE_NOOP("grt::fields", "case volume"),
    QT_TRANSLATE_NOOP("grt::fields", "cold temperature coefficient"),
    QT_TRANSLATE_NOOP("grt::fields", "covolume of the powder gases"),
    QT_TRANSLATE_NOOP("grt::fields", "effektive area"),
    QT_TRANSLATE_NOOP("grt::fields", "groove caliber"),
    QT_TRANSLATE_NOOP("grt::fields", "gun name"),
    QT_TRANSLATE_NOOP("grt::fields", "gun weight"),
    QT_TRANSLATE_NOOP("grt::fields", "hot temperature coefficient"),
    QT_TRANSLATE_NOOP("grt::fields", "initial gas pressure"),
    QT_TRANSLATE_NOOP("grt::fields", "ladder step propellant mass"),
    QT_TRANSLATE_NOOP("grt::fields", "ladder steps (1 to 10, below 2: disabled)"),
    QT_TRANSLATE_NOOP("grt::fields", "material density"),
    QT_TRANSLATE_NOOP("grt::fields", "maximum allowed pressure"),
    QT_TRANSLATE_NOOP("grt::fields", "prog/deg coefficient"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile base type"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile diameter"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile lot-id"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile manufacturer"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile material and gas sealing/losses"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile name"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile overall length"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile product name"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile tail diameter A"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile tail diameter B"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile tail length"),
    QT_TRANSLATE_NOOP("grt::fields", "projectile weight"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant detail picture uuid"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant lot-id"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant manufacturer"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant mass"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant picture uuid"),
    QT_TRANSLATE_NOOP("grt::fields", "propellant product name"),
    QT_TRANSLATE_NOOP("grt::fields", "ratio of the specific heat of the powder gases"),
    QT_TRANSLATE_NOOP("grt::fields", "selected ladder curve for display"),
    QT_TRANSLATE_NOOP("grt::fields", "special bullet resistance inside the barrel"),
    QT_TRANSLATE_NOOP("grt::fields", "specific explosive heat"),
    QT_TRANSLATE_NOOP("grt::fields", "technical atmosphere pressure"),
    QT_TRANSLATE_NOOP("grt::fields", "temperature normal"),
    QT_TRANSLATE_NOOP("grt::fields", "true if shotshell"),
    QT_TRANSLATE_NOOP("grt::fields", "twist length"),
    QT_TRANSLATE_NOOP("grt::fields", "unlocked bullet jump"),
    QT_TRANSLATE_NOOP("grt::fields", "unlocked friction"),
    QT_TRANSLATE_NOOP("grt::fields", "vivacity coefficient"),
};

}  // namespace

QString translatedDescription(const QString &descr)
{
    if (descr.isEmpty()) return descr;
    return QCoreApplication::translate("grt::fields", descr.toUtf8().constData());
}

}  // namespace grt
