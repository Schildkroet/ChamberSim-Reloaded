#include "about.h"

#include <QApplication>
#include <QFileInfo>
#include <QLocale>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <QSysInfo>

#ifndef GRT_VERSION
#define GRT_VERSION "0.1"
#endif

namespace about
{

QString html(const QString &databasePath, int calibers, int projectiles, int propellants)
{
    const QString version =
        QCoreApplication::applicationVersion().isEmpty() ? QStringLiteral(GRT_VERSION) : QCoreApplication::applicationVersion();
    // short blocks with headings; the legal text sits in a framed box, the technical details in small print
    QString text =
        QApplication::translate("about", "<h2>GRT Reloaded</h2>"
                                         "<p><b>Version %1</b><br>Interior ballistics for handloaders</p>"
                                         "<p>Choose a cartridge, bullet and powder, enter the charge, and see pressure, bullet energy "
                                         "and velocity along the barrel. Includes a load development ladder, reports and printouts.</p>"
                                         "<p>A Qt&nbsp;6 re-creation of the workflow of Gordon's Reloading Tool. It is an independent "
                                         "project and is not affiliated with or supported by the original program or its author.</p>")
            .arg(version.toHtmlEscaped());
    text += QApplication::translate("about", "<h3>Accuracy</h3>"
                                             "<p>The simulation was calibrated against 190 runs of the original program on 36 "
                                             "cartridges: typically about 1&nbsp;% in muzzle velocity and 2&nbsp;% in peak pressure. "
                                             "Unusual loads can deviate more.</p>");
    text += QApplication::translate("about", "<h3>Safety</h3>"
                                             "<ul>"
                                             "<li>The results are calculated and can differ considerably from reality.</li>"
                                             "<li>Compare every load with the current loading tables of the component manufacturers.</li>"
                                             "<li>Start low. Use at your own risk.</li>"
                                             "</ul>");
    text +=
        QApplication::translate("about", "<h3>Disclaimer</h3>"
                                         "<table border=\"1\" cellspacing=\"0\" cellpadding=\"8\" width=\"100%\"><tr><td>"
                                         "This software is provided \"as is\", without warranty of any kind, express or implied, "
                                         "including accuracy, correctness or fitness for a particular purpose.<br><br>"
                                         "The author and all contributors accept <b>no responsibility or liability</b> for any damage, "
                                         "injury or death, loss, or any other consequence arising from the use of this software or of its "
                                         "results, including damage to firearms or property.<br><br>"
                                         "Reloading ammunition is dangerous and is done entirely at your own responsibility."
                                         "</td></tr></table>");
    text += QApplication::translate("about", "<hr><p><small>Qt %1 (built with %2) · %3 · language %4</small></p>")
                .arg(QString::fromLatin1(qVersion()), QStringLiteral(QT_VERSION_STR), QSysInfo::prettyProductName().toHtmlEscaped(),
                     QLocale().name());
    if (!databasePath.isEmpty())
    {
        text += QApplication::translate("about", "<p><small>Database: %1 — %2 calibers, %3 projectiles, %4 propellants</small></p>")
                    .arg(QFileInfo(databasePath).fileName().toHtmlEscaped())
                    .arg(calibers)
                    .arg(projectiles)
                    .arg(propellants);
    }
    return text;
}

void show(QWidget *parent, const QString &databasePath, int calibers, int projectiles, int propellants)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("aboutDialog"));
    dialog.setWindowTitle(QApplication::translate("about", "About GRT Reloaded"));

    auto *icon = new QLabel;
    icon->setPixmap(QApplication::windowIcon().pixmap(64, 64));
    icon->setAlignment(Qt::AlignTop);
    auto *browser = new QTextBrowser;
    browser->setFrameShape(QFrame::NoFrame);
    browser->setOpenExternalLinks(true);
    browser->setHtml(html(databasePath, calibers, projectiles, propellants));
    browser->setStyleSheet(QStringLiteral("QTextBrowser { background: transparent; }"));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    auto *top = new QHBoxLayout;
    if (const QPixmap pm = QApplication::windowIcon().pixmap(64, 64); !pm.isNull()) top->addWidget(icon);
    else delete icon;
    top->addWidget(browser, 1);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(top, 1);
    layout->addWidget(buttons);

    // twice as wide as the old message box; the height fits the text (scrolls if the screen is smaller)
    const int width = qRound(800.0 * std::max(8.0, dialog.font().pointSizeF()) / 9.0);
    browser->document()->setTextWidth(width - 40);
    const int wanted = qRound(browser->document()->size().height()) + buttons->sizeHint().height() + 60;
    const int maxHeight = dialog.screen() ? dialog.screen()->availableGeometry().height() * 85 / 100 : wanted;
    dialog.resize(width, std::min(wanted, maxHeight));
    dialog.exec();
}

}  // namespace about
