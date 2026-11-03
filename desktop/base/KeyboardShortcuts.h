// Portable, user-configurable keyboard layout shared by both scoreboard modes.
#ifndef IPPONBOARD_MESCHEDE_KEYBOARD_SHORTCUTS_H
#define IPPONBOARD_MESCHEDE_KEYBOARD_SHORTCUTS_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QSaveFile>
#include <QString>

namespace IpponboardKeyboard {
struct Command {
    const char* id;
    QString label;
    QKeySequence defaultKey;
    bool teamOnly;
};

inline const QList<Command>& commands() {
    static const QList<Command> definitions = {
        {"hajime", QStringLiteral("Hajime / Mate"), QKeySequence(Qt::Key_Space), false},
        {"hold_first", QStringLiteral("Osaekomi / Toketa – Weiß"), QKeySequence(Qt::Key_Left), false},
        {"hold_second", QStringLiteral("Osaekomi / Toketa – Blau"), QKeySequence(Qt::Key_Right), false},
        {"hold_reset", QStringLiteral("Haltezeit zurücksetzen"), QKeySequence(Qt::Key_Down), false},
        {"reset", QStringLiteral("Kampf vollständig zurücksetzen"), QKeySequence(Qt::CTRL | Qt::Key_Backspace), false},
        {"next", QStringLiteral("Nächster Kampf"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right), true},
        {"previous", QStringLiteral("Vorheriger Kampf"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Left), true},
        {"result", QStringLiteral("Zwischenstand anzeigen"), QKeySequence(Qt::Key_F4), true},
        {"first_ippon_plus", QStringLiteral("Weiß: Ippon +"), QKeySequence(Qt::Key_F5), false},
        {"first_wazaari_plus", QStringLiteral("Weiß: Waza-ari +"), QKeySequence(Qt::Key_F6), false},
        {"first_yuko_plus", QStringLiteral("Weiß: Yuko +"), QKeySequence(Qt::Key_F7), false},
        {"first_shido_plus", QStringLiteral("Weiß: Shido +"), QKeySequence(Qt::Key_F8), false},
        {"second_ippon_plus", QStringLiteral("Blau: Ippon +"), QKeySequence(Qt::Key_F9), false},
        {"second_wazaari_plus", QStringLiteral("Blau: Waza-ari +"), QKeySequence(Qt::Key_F10), false},
        {"second_yuko_plus", QStringLiteral("Blau: Yuko +"), QKeySequence(Qt::Key_F11), false},
        {"second_shido_plus", QStringLiteral("Blau: Shido +"), QKeySequence(Qt::Key_F12), false},
        {"first_ippon_minus", QStringLiteral("Weiß: Ippon zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F5), false},
        {"first_wazaari_minus", QStringLiteral("Weiß: Waza-ari zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F6), false},
        {"first_yuko_minus", QStringLiteral("Weiß: Yuko zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F7), false},
        {"first_shido_minus", QStringLiteral("Weiß: Shido zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F8), false},
        {"second_ippon_minus", QStringLiteral("Blau: Ippon zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F9), false},
        {"second_wazaari_minus", QStringLiteral("Blau: Waza-ari zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F10), false},
        {"second_yuko_minus", QStringLiteral("Blau: Yuko zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F11), false},
        {"second_shido_minus", QStringLiteral("Blau: Shido zurücknehmen"), QKeySequence(Qt::CTRL | Qt::Key_F12), false}
    };
    return definitions;
}

inline QString filePath() {
    // Never store USB preferences in the Windows user profile.
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(
        QStringLiteral("../data/keyboard-shortcuts.json"));
}

inline QMap<QString, QKeySequence> defaults() {
    QMap<QString, QKeySequence> result;
    for (const Command& command : commands())
        result.insert(QString::fromLatin1(command.id), command.defaultKey);
    return result;
}

inline QMap<QString, QKeySequence> load() {
    QMap<QString, QKeySequence> result = defaults();
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) return result;
    const QJsonDocument parsed = QJsonDocument::fromJson(file.readAll());
    if (!parsed.isObject()) return result;
    const QJsonObject root = parsed.object();
    if (root.value(QStringLiteral("schema")).toString() != QStringLiteral("ipponboard-keyboard-1"))
        return result;
    const QJsonObject bindings = root.value(QStringLiteral("bindings")).toObject();
    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        if (bindings.contains(it.key()) && bindings.value(it.key()).isString())
            result[it.key()] = QKeySequence::fromString(bindings.value(it.key()).toString(),
                                                       QKeySequence::PortableText);
    }
    return result;
}

inline bool save(const QMap<QString, QKeySequence>& bindings, QString& error) {
    error.clear();
    const QString output = filePath();
    if (!QDir().mkpath(QFileInfo(output).absolutePath())) {
        error = QStringLiteral("USB-Datenverzeichnis konnte nicht angelegt werden.");
        return false;
    }
    QJsonObject entries;
    for (const Command& command : commands()) {
        const QString id = QString::fromLatin1(command.id);
        entries.insert(id, bindings.value(id, command.defaultKey).toString(QKeySequence::PortableText));
    }
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("ipponboard-keyboard-1"));
    root.insert(QStringLiteral("bindings"), entries);
    QSaveFile file(output);
    if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) { error = file.errorString(); return false; }
    return true;
}
}

#endif
