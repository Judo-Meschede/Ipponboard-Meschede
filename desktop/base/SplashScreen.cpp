// Copyright 2018 Florian Muecke. Original Ipponboard source under BSD-style license.
// Modifications: Ipponboard-Meschede.
#include "SplashScreen.h"
#include "ui_SplashScreen.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QResizeEvent>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif


namespace
{
#ifdef _WIN32
bool FetchSnapshotWindows(QByteArray& payload, QString& error)
{
    payload.clear();
    error.clear();

    HINTERNET session = WinHttpOpen(L"Ipponboard-Meschede/0.1.8",
                                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        error = QStringLiteral("WinHTTP konnte nicht initialisiert werden (%1)").arg(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);

    HINTERNET connect = WinHttpConnect(session, L"test-liga.paul-meschede.de",
                                       INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) {
        error = QStringLiteral("Serververbindung fehlgeschlagen (%1)").arg(GetLastError());
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(connect, L"GET", L"/api/sync/snapshot",
                                           nullptr, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           WINHTTP_FLAG_SECURE);
    if (!request) {
        error = QStringLiteral("HTTPS-Anfrage konnte nicht erstellt werden (%1)").arg(GetLastError());
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    bool ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(request, nullptr);

    if (!ok) {
        error = QStringLiteral("HTTPS-Abruf fehlgeschlagen (%1)").arg(GetLastError());
    } else {
        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        if (!WinHttpQueryHeaders(request,
                                 WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                                 WINHTTP_NO_HEADER_INDEX)) {
            error = QStringLiteral("HTTP-Status konnte nicht gelesen werden (%1)").arg(GetLastError());
            ok = false;
        } else if (status != 200) {
            error = QStringLiteral("Server antwortet mit HTTP %1").arg(status);
            ok = false;
        }
    }

    while (ok) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            error = QStringLiteral("Antwort konnte nicht gelesen werden (%1)").arg(GetLastError());
            ok = false;
            break;
        }
        if (available == 0) break;

        QByteArray chunk;
        chunk.resize(static_cast<int>(available));
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), available, &read)) {
            error = QStringLiteral("Antwort konnte nicht gelesen werden (%1)").arg(GetLastError());
            ok = false;
            break;
        }
        chunk.resize(static_cast<int>(read));
        payload.append(chunk);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    if (ok && payload.isEmpty()) {
        error = QStringLiteral("Server lieferte keine Daten");
        ok = false;
    }
    return ok;
}
#endif
}

SplashScreen::SplashScreen(Data const& data, QWidget* parent) : QDialog(parent), ui(new Ui::SplashScreen)
{
    Q_UNUSED(data);
    ui->setupUi(this);
    ui->label_info->setText(QStringLiteral("Ipponboard-Meschede V0.1.6"));
    setWindowFlags(Qt::Window);
    resize(1536, 982);

    ui->cardSingle->installEventFilter(this);
    ui->cardTeam->installEventFilter(this);
    ui->cardAdmin->installEventFilter(this);
    ApplyReferenceGeometry();
    MakeCardChildrenMouseTransparent(ui->cardSingle);
    MakeCardChildrenMouseTransparent(ui->cardTeam);
    MakeCardChildrenMouseTransparent(ui->cardAdmin);

    SyncMasterDataCache();
}

SplashScreen::~SplashScreen(){ delete ui; }
void SplashScreen::SetImageStyleSheet(QString const&){ }
void SplashScreen::changeEvent(QEvent* e){ QWidget::changeEvent(e); if(e->type()==QEvent::LanguageChange) ui->retranslateUi(this); }

void SplashScreen::MakeCardChildrenMouseTransparent(QWidget* card)
{
    const auto children = card->findChildren<QWidget*>();
    for (QWidget* child : children) child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

bool SplashScreen::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (watched == ui->cardSingle) { on_commandLinkButton_startSingleVersion_pressed(); return true; }
            if (watched == ui->cardTeam) { on_commandLinkButton_startTeamVersion_pressed(); return true; }
            if (watched == ui->cardAdmin) { on_commandLinkButton_admin_pressed(); return true; }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void SplashScreen::on_commandLinkButton_startSingleVersion_pressed(){ accept(); }
void SplashScreen::on_commandLinkButton_startTeamVersion_pressed(){ done(QDialog::Accepted + 1); }
void SplashScreen::on_commandLinkButton_admin_pressed(){ QDesktopServices::openUrl(QUrl(QStringLiteral("https://test-liga.paul-meschede.de/verwaltung"))); }
void SplashScreen::on_btnSync_clicked(){ SyncMasterDataCache(); }

void SplashScreen::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    ApplyReferenceGeometry();
}

void SplashScreen::ApplyReferenceGeometry()
{
    if (!ui) return;
    const double sx = width() / 1536.0;
    const double sy = height() / 982.0;
    const auto apply = [sx, sy](QWidget* w, int x, int y, int ww, int hh) {
        w->setGeometry(qRound(x * sx), qRound(y * sy), qRound(ww * sx), qRound(hh * sy));
    };

    apply(ui->backgroundLabel, 0, 0, 1536, 982);
    apply(ui->cardSingle, 93, 410, 441, 313);
    apply(ui->cardTeam, 557, 410, 423, 313);
    apply(ui->cardAdmin, 1002, 410, 438, 313);
    apply(ui->btnSettings, 1110, 0, 194, 70);
    apply(ui->btnHelp, 1304, 0, 93, 70);
    apply(ui->btnAbout, 1397, 0, 139, 70);
    apply(ui->label_serverStatus, 148, 800, 260, 24);
    apply(ui->label_lastUpdate, 148, 826, 280, 22);
    apply(ui->btnSync, 551, 850, 184, 34);
    apply(ui->label_cacheStatus, 898, 800, 230, 24);
    apply(ui->label_info, 1260, 800, 230, 24);

    const double fontScale = qMax(0.78, qMin(sx, sy));
    QFont f;
    f = ui->label_serverStatus->font(); f.setPixelSize(qRound(16 * fontScale)); f.setBold(true); ui->label_serverStatus->setFont(f);
    f = ui->label_lastUpdate->font(); f.setPixelSize(qRound(14 * fontScale)); ui->label_lastUpdate->setFont(f);
    f = ui->label_cacheStatus->font(); f.setPixelSize(qRound(14 * fontScale)); ui->label_cacheStatus->setFont(f);
    f = ui->label_info->font(); f.setPixelSize(qRound(14 * fontScale)); ui->label_info->setFont(f);
    f = ui->btnSync->font(); f.setPixelSize(qRound(13 * fontScale)); ui->btnSync->setFont(f);
}

void SplashScreen::SyncMasterDataCache()
{
    ui->btnSync->setEnabled(false);
    ui->btnSync->setText(QStringLiteral("Aktualisiere..."));
    ui->label_serverStatus->setStyleSheet(QStringLiteral("color:#f4b83f;font-weight:700;font-size:14px;"));
    ui->label_serverStatus->setText(QStringLiteral("Server wird geprüft ..."));
    QCoreApplication::processEvents();

    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    QString dataPath = dir.absoluteFilePath(QStringLiteral("../data"));
    QDir dataDir(dataPath);
    if (!dataDir.exists() && !QDir().mkpath(dataPath)) {
        dataPath = dir.absoluteFilePath(QStringLiteral("data"));
        QDir().mkpath(dataPath);
    }
    const QString cacheFile = QDir(dataPath).filePath(QStringLiteral("masterdata.json"));

    QByteArray payload;
    QString networkError;
    bool downloadOk = false;

#ifdef _WIN32
    downloadOk = FetchSnapshotWindows(payload, networkError);
#else
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(QStringLiteral("https://test-liga.paul-meschede.de/api/sync/snapshot")));
    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(5000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
        payload = reply->readAll();
        downloadOk = !payload.isEmpty();
        if (!downloadOk) networkError = QStringLiteral("Server lieferte keine Daten");
    } else {
        if (!reply->isFinished()) {
            reply->abort();
            networkError = QStringLiteral("Zeitüberschreitung beim Serverabruf");
        } else {
            networkError = reply->errorString();
        }
    }
    reply->deleteLater();
#endif

    bool updated = false;
    if (downloadOk) {
        QSaveFile out(cacheFile);
        if (out.open(QIODevice::WriteOnly) &&
            out.write(payload) == payload.size() &&
            out.commit()) {
            updated = true;
            ui->label_serverStatus->setStyleSheet(QStringLiteral("color:#39df69;font-weight:700;font-size:14px;"));
            ui->label_serverStatus->setText(QStringLiteral("Server verbunden"));
            ui->btnSync->setToolTip(QString());
        } else {
            ui->label_serverStatus->setStyleSheet(QStringLiteral("color:#f4b83f;font-weight:700;font-size:14px;"));
            ui->label_serverStatus->setText(QStringLiteral("Server verbunden - Speichern fehlgeschlagen"));
            ui->btnSync->setToolTip(QStringLiteral("Der lokale Datenordner konnte nicht beschrieben werden."));
        }
    } else {
        ui->label_serverStatus->setStyleSheet(QStringLiteral("color:#f4b83f;font-weight:700;font-size:14px;"));
        ui->label_serverStatus->setText(QFile::exists(cacheFile)
            ? QStringLiteral("Offline - lokaler Stand aktiv")
            : QStringLiteral("Offline - kein lokaler Datenstand"));
        ui->btnSync->setToolTip(networkError);
        ui->label_lastUpdate->setText(networkError.isEmpty()
            ? QStringLiteral("Serverabruf fehlgeschlagen")
            : QStringLiteral("Fehler: %1").arg(networkError.left(70)));
    }

    if (QFile::exists(cacheFile)) {
        const QFileInfo info(cacheFile);
        const QString stamp = info.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm"));
        ui->label_cacheStatus->setText(QStringLiteral("Letzter Stand: %1").arg(stamp));
        if (updated) ui->label_lastUpdate->setText(QStringLiteral("Letzte Aktualisierung: %1").arg(stamp));
    } else {
        ui->label_cacheStatus->setText(QStringLiteral("Noch kein lokaler Datenstand"));
        if (downloadOk) ui->label_lastUpdate->setText(QStringLiteral("Letzte Aktualisierung: -"));
    }

    ui->btnSync->setText(updated ? QStringLiteral("Daten aktuell") : QStringLiteral("Erneut versuchen"));
    ui->btnSync->setEnabled(true);
}
