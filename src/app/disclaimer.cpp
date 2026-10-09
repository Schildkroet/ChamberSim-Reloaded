#include "disclaimer.h"

#include "log.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

namespace disclaimer
{

namespace
{
constexpr auto kHideKey = "ui/hideDisclaimer";

// the translation context "Disclaimer"
struct Disclaimer
{
    Q_DECLARE_TR_FUNCTIONS(Disclaimer)
};
}  // namespace

bool wanted()
{
    return !QSettings().value(QLatin1String(kHideKey), false).toBool();
}

std::unique_ptr<QMessageBox> create(QWidget *parent)
{
    auto box = std::make_unique<QMessageBox>(parent);
    box->setIcon(QMessageBox::Warning);
    box->setWindowTitle(Disclaimer::tr("Safety notice"));
    box->setTextFormat(Qt::RichText);
    box->setText(Disclaimer::tr("<b>Handloading ammunition is dangerous.</b> A wrong load can destroy a firearm and cause severe injury or "
                                "death."));
    box->setInformativeText(Disclaimer::tr(
        "ChamberSim Reloaded simulates interior ballistics. Its results are estimates and can be far from reality: use the program only "
        "if you know what you are doing.<br><br>Never use a simulated load as a recipe. Always start from the current load data "
        "of the powder and bullet maker, start low, work up carefully and watch for pressure signs.<br><br>The program is "
        "provided as it is, without any warranty. The authors are not responsible for any damage, injury or loss that results "
        "from its use."));
    box->addButton(Disclaimer::tr("I understand"), QMessageBox::AcceptRole);
    QPushButton *quit = box->addButton(Disclaimer::tr("Quit"), QMessageBox::RejectRole);
    box->setEscapeButton(quit);
    box->setCheckBox(new QCheckBox(Disclaimer::tr("Do not show this again")));
    return box;
}

bool confirm(QWidget *parent)
{
    if (!wanted()) return true;
    const auto box = create(parent);
    box->exec();
    const bool accepted = box->buttonRole(box->clickedButton()) == QMessageBox::AcceptRole;
    if (accepted && box->checkBox()->isChecked()) QSettings().setValue(QLatin1String(kHideKey), true);
    LOG_Info() << "Safety notice:" << (accepted ? "accepted" : "quit");
    return accepted;
}

}  // namespace disclaimer
