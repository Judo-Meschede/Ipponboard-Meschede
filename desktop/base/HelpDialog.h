#ifndef IPPONBOARD_MESCHEDE_HELP_DIALOG_H
#define IPPONBOARD_MESCHEDE_HELP_DIALOG_H
#include <QDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QVBoxLayout>
namespace Ipponboard {
inline void ShowIpponboardHelp(QWidget* parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Hilfe – Ipponboard-Meschede"));
    dialog.resize(1020, 720);

    QVBoxLayout page(&dialog);
    QHBoxLayout searchRow;
    QLineEdit search(&dialog);
    search.setPlaceholderText(QStringLiteral("In der Hilfe suchen"));
    QPushButton searchButton(QStringLiteral("Suchen"), &dialog);
    QLabel searchStatus(&dialog);
    searchRow.addWidget(&search, 1);
    searchRow.addWidget(&searchButton);
    searchRow.addWidget(&searchStatus);
    page.addLayout(&searchRow);

    QHBoxLayout mainRow;
    QListWidget topics(&dialog);
    topics.setMaximumWidth(220);
    topics.addItems({
        QStringLiteral("Start"),
        QStringLiteral("Anzeigetafel"),
        QStringLiteral("Kampfbedienung"),
        QStringLiteral("Einzelwettkämpfe"),
        QStringLiteral("Mannschaftskämpfe"),
        QStringLiteral("Stammdaten"),
        QStringLiteral("Bildschirme"),
        QStringLiteral("USB-Varianten"),
        QStringLiteral("Ergebnisrücktransport"),
        QStringLiteral("Probleme / Hilfe")
    });
    QTextBrowser browser(&dialog);
    browser.setOpenExternalLinks(false);
    QFile manual(QStringLiteral(":/res/hilfe.html"));
    if (manual.open(QIODevice::ReadOnly))
        browser.setHtml(QString::fromUtf8(manual.readAll()));
    else
        browser.setPlainText(QStringLiteral("Das integrierte Handbuch konnte nicht geladen werden."));
    mainRow.addWidget(&topics);
    mainRow.addWidget(&browser, 1);
    page.addLayout(&mainRow, 1);

    const QStringList anchors = {
        QStringLiteral("start"), QStringLiteral("oberflaeche"),
        QStringLiteral("kampf"), QStringLiteral("einzel"),
        QStringLiteral("mannschaft"), QStringLiteral("stamm"),
        QStringLiteral("anzeige"), QStringLiteral("usb"),
        QStringLiteral("ergebnisse"), QStringLiteral("probleme")
    };
    QObject::connect(&topics, &QListWidget::currentRowChanged, &dialog,
        [&](int row) {
            if (row >= 0 && row < anchors.size())
                browser.scrollToAnchor(anchors.at(row));
        });
    const auto findText = [&]() {
        const QString query = search.text().trimmed();
        if (query.isEmpty()) return;
        if (browser.find(query)) {
            searchStatus.clear();
        } else {
            browser.moveCursor(QTextCursor::Start);
            searchStatus.setText(browser.find(query)
                ? QString() : QStringLiteral("Kein weiterer Treffer"));
        }
    };
    QObject::connect(&searchButton, &QPushButton::clicked, &dialog, findText);
    QObject::connect(&search, &QLineEdit::returnPressed, &dialog, findText);
    QPushButton close(QStringLiteral("Schließen"), &dialog);
    QObject::connect(&close, &QPushButton::clicked, &dialog, &QDialog::accept);
    QHBoxLayout bottom;
    bottom.addStretch();
    bottom.addWidget(&close);
    page.addLayout(&bottom);
    dialog.exec();

}
}
#endif
