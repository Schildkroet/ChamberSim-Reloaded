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

#include "about.h"

#include <QApplication>
#include <QDateTime>
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

#ifndef CSR_VERSION
#define CSR_VERSION "0.1"
#endif

namespace about
{

namespace
{

QString countsText(const Counts &c)
{
    return QApplication::translate("about", "%Ln caliber(s)", nullptr, c.calibers) + QStringLiteral(", ") +
           QApplication::translate("about", "%Ln projectile(s)", nullptr, c.projectiles) + QStringLiteral(", ") +
           QApplication::translate("about", "%Ln propellant(s)", nullptr, c.propellants);
}

// compile date and time of this file (__DATE__ is always "Mmm dd yyyy" in English), shown as ISO 8601
QString buildDate()
{
    const QString stamp = QString::fromLatin1(__DATE__ " " __TIME__).simplified();
    const QDateTime time = QLocale::c().toDateTime(stamp, QStringLiteral("MMM d yyyy HH:mm:ss"));
    return time.isValid() ? time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) : stamp;
}

}  // namespace

QString html(const std::optional<DatabaseInfo> &database)
{
    const QString version =
        QCoreApplication::applicationVersion().isEmpty() ? QStringLiteral(CSR_VERSION) : QCoreApplication::applicationVersion();
    // short blocks with headings; the legal text sits in a framed box, the technical details in small print
    QString text =
        QApplication::translate("about", "<h2>ChamberSim Reloaded</h2>"
                                         "<p><b>Version %1</b><br>Interior ballistics for handloaders</p>"
                                         "<p>Choose a cartridge, bullet and powder, enter the charge, and see pressure, bullet energy "
                                         "and velocity along the barrel.</p>"
                                         "<p>A Qt&nbsp;6 re-creation of the workflow of Gordon's Reloading Tool. It is an independent "
                                         "project and is not affiliated with or supported by the original program or its author.</p>")
            .arg(version.toHtmlEscaped());
    text += QStringLiteral(
                "<p>%1 <a href=\"https://github.com/Schildkroet/ChamberSim-Reloaded\">github.com/Schildkroet/ChamberSim-Reloaded</a></p>")
                .arg(QApplication::translate("about", "Source code and issue tracker:"));
    text += QApplication::translate("about",
                                    "<h3>Features</h3>"
                                    "<ul>"
                                    "<li>Load development ladder; reports with print preview and PDF export</li>"
                                    "<li>Measurements: chronograph data (import of Garmin Xero, MagnetoSpeed and Labradar files) compared "
                                    "with the simulation; the powder's model values can be fitted to your measurements</li>"
                                    "<li>Built-in database of calibers, projectiles and propellants, your own records, and "
                                    "optionally the database of the original program</li>"
                                    "<li>Cartridge and chamber drawings with the C.I.P. dimensions</li>"
                                    "<li>Metric, imperial or the original's units; light and dark theme; several languages</li>"
                                    "</ul>");
    text += QApplication::translate("about", "<h3>Accuracy</h3>"
                                             "<p>The default <b>Physical Model</b> was fitted to the published load data of powder "
                                             "manufacturers. On cartridges not used for the fit it typically deviates about 2&nbsp;% in "
                                             "muzzle velocity and 7.5&nbsp;% in peak pressure from the published values (median); single "
                                             "loads, and pistol and shotgun powders, can deviate much more.</p>"
                                             "<p>The <b>GRT Original</b> model (File &gt; Settings &gt; Solver) reproduces the original "
                                             "program: on average 0.8&nbsp;% in muzzle velocity and 2.8&nbsp;% in peak pressure over 349 "
                                             "reference runs.</p>");
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
    text +=
        QApplication::translate("about", "<h3>Copyright and license</h3>"
                                         "<p>Copyright &copy; 2026 Schildkroet</p>"
                                         "<p><b>This is free software.</b> You should not have paid for it. If someone sold it to you, "
                                         "you were cheated: ask for your money back and get the program from the link above.</p>"
                                         "<p>ChamberSim Reloaded is free software: you can redistribute it and/or modify it under the "
                                         "terms of the GNU General Public License as published by the Free Software Foundation, either "
                                         "version 3 of the License, or (at your option) any later version. It is distributed in the hope "
                                         "that it will be useful, but without any warranty; see the "
                                         "<a href=\"https://www.gnu.org/licenses/\">GNU General Public License</a> for details.</p>"
                                         "<p>The records of the built-in database are licensed under "
                                         "<a href=\"https://creativecommons.org/licenses/by-sa/4.0/\">CC BY-SA 4.0</a>.</p>");
    text += QApplication::translate("about", "<hr><p><small>Qt %1 (built with %2) · %3 · language %4</small></p>")
                .arg(QString::fromLatin1(qVersion()), QStringLiteral(QT_VERSION_STR), QSysInfo::prettyProductName().toHtmlEscaped(),
                     QLocale().name());
    text += QApplication::translate("about", "<p><small>Build date: %1</small></p>").arg(buildDate());
    if (database)
    {
        const QString original =
            database->originalPath.isEmpty()
                ? QApplication::translate("about", "Original program's database: none chosen")
                : QApplication::translate("about", "Original program's database (%1): %2")
                      .arg(QFileInfo(database->originalPath).fileName().toHtmlEscaped(), countsText(database->original));
        text += QApplication::translate("about", "<p><small>Database<br>Project records: %1<br>Own records: %2<br>%3</small></p>")
                    .arg(countsText(database->project), countsText(database->own), original);
    }
    return text;
}

void show(QWidget *parent, const std::optional<DatabaseInfo> &database)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("aboutDialog"));
    dialog.setWindowTitle(QApplication::translate("about", "About ChamberSim Reloaded"));

    auto *icon = new QLabel;
    icon->setPixmap(QApplication::windowIcon().pixmap(64, 64));
    icon->setAlignment(Qt::AlignTop);
    auto *browser = new QTextBrowser;
    browser->setFrameShape(QFrame::NoFrame);
    browser->setOpenExternalLinks(true);
    browser->setHtml(html(database));
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
