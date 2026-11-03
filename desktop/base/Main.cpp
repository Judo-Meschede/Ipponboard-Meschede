// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "MainWindow.h"
#include "MainWindowTeam.h"
//#include "../core/TournamentMode.h"
#include "SplashScreen.h"
#include "versioninfo.h"
#include "OnlineVersionChecker.h"
#include "../util/path_helpers.h"

#include <QApplication>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QTranslator>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QLocale>
//#include <QtextCodec>
#include <QCommandLineParser>
#include <QDebug>
#include <QTextStream>

#ifndef _WIN32
#ifndef NO_ERROR
#define NO_ERROR 0
#endif
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER 1
#endif
#endif

void LangNotFound(const QString& fileName)
{
	QMessageBox::critical(nullptr,
						  QCoreApplication::applicationName(),
						  "Unable to read language file: " + fileName +
						  "\nThe default language is being used.");
}

void SetTranslation(QApplication& app, QTranslator& translator, QString const& langStr)
{
    Q_UNUSED(app);

	if (langStr == QString("en"))
	{
		return; // default
	}

	if (langStr == QString("de") || langStr == QString("nl"))
	{
		const QString& langPath = QCoreApplication::applicationDirPath() + QString("/lang");

		if (translator.load(langStr, langPath))
		{
			app.installTranslator(&translator);
		}
		else
		{
			LangNotFound(langStr);
		}
	}
}

int ShowSplashScreen()
{
    SplashScreen::Data splashData;
    splashData.text = QStringLiteral("Ipponboard-Meschede – Judo-Anzeigetafel für Einzel- und Mannschaftswettkämpfe.");
    splashData.info = QCoreApplication::applicationName()
        + QStringLiteral(" v") + QCoreApplication::applicationVersion()
        + QStringLiteral("\nBuild: ") + QString::fromLatin1(VersionInfo::Date);
    SplashScreen splash(splashData);
    const int result = splash.exec();
    return result == QDialog::Rejected ? 0 : result;
}

void CustomMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &msg)
{
    Q_UNUSED(context)
    QString logLevel;
    switch (type)
    {
    case QtDebugMsg:
        logLevel = "DBUG";
        break;
    case QtInfoMsg:
        logLevel = "INFO";
        break;
    case QtWarningMsg:
        logLevel = "WARN";
        break;
    case QtCriticalMsg:
        logLevel = "CRIT";
        break;
    case QtFatalMsg:
        logLevel = "FATL";
        break;
    }

    QString logMsg = QString("%1 %2 %3").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")).arg(logLevel).arg(msg);

    QFile logFile(QCoreApplication::applicationName() + ".log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream out(&logFile);
        out << logMsg << Qt::endl;
    }
}

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

    // Open the log file and truncate existing content
    QFile logFile(QCoreApplication::applicationName() + ".log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        logFile.close(); // Close file after truncating
    }

    qInstallMessageHandler(CustomMessageHandler);

	QCoreApplication::setApplicationVersion(VersionInfo::VersionStr);
    QCoreApplication::setOrganizationName(QString::fromUtf8("SSV Meschede Judo"));
	QCoreApplication::setOrganizationDomain("paul-meschede.de");
	QCoreApplication::setApplicationName("Ipponboard-Meschede");
    const QString emblemFile = QDir(QCoreApplication::applicationDirPath())
        .absoluteFilePath(QStringLiteral("app-icon.png"));
    if (QFile::exists(emblemFile))
        a.setWindowIcon(QIcon(emblemFile));

    qInfo() << QCoreApplication::applicationName() << QCoreApplication::applicationVersion();

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption modeOption(QStringList() << "mode", "Selects starting mode: single|team|ask.", "mode", "ask");
    parser.addOption(modeOption);
    parser.process(a);
    QString mode = parser.value(modeOption);
    QStringList validModes { "single", "team", "ask" };
    if (!validModes.contains(mode))
    {
        parser.showHelp(ERROR_INVALID_PARAMETER);
    }

	// read language code
	QString langStr = QLocale::system().name();
	langStr.truncate(langStr.lastIndexOf('_'));

	qInfo() << "Reading settings from: " << fm::GetSettingsFilePath("Ipponboard-Meschede.ini");
	QSettings settings(fm::GetSettingsFilePath("Ipponboard-Meschede.ini"), QSettings::IniFormat, &a);
	settings.beginGroup(str_tag_Main);

	if (settings.contains(str_tag_Language))
	{
		langStr = settings.value(str_tag_Language).toString();
	}

	settings.endGroup();

	QTranslator translator;  // Note: this object needs to remain in scope.
	SetTranslation(a, translator, langStr);

    // USB export packages always start offline. Even allowed synchronization
    // requires an explicit user action; never check remote releases at startup.
    const QString usbProfile = QDir(QCoreApplication::applicationDirPath())
        .absoluteFilePath(QStringLiteral("../data/usb-profile.json"));
    OnlineVersionChecker::OnlineVersion onlineVersion{};
    if (!QFile::exists(usbProfile))
        onlineVersion = OnlineVersionChecker::CheckOnlineVersion();
	if (onlineVersion.state == OnlineVersionChecker::State::NewerAvailable)
	{
		// format message
		QString changes = QString(": <br><br><tt>%1</tt><br>").arg(QCoreApplication::tr("en") == "de" ? onlineVersion.changes_de : onlineVersion.changes_en);
		changes.replace("\n", "<br>");

		QString msg = QString("<p>%1 %2</p>")
			.arg(QCoreApplication::tr("Version %1 available (currently using: %2)").arg(QString("<b>%1</b>").arg(onlineVersion.version)).arg(QCoreApplication::applicationVersion()))
			.arg(changes);

		msg += QString("<p>%1</p>").arg(QCoreApplication::tr("Do you want to download it or visit the project homepage?"));

		// show message box
		auto result = QMessageBox::information(
			nullptr,
			QCoreApplication::tr("Ipponboard - New Version Available"),
			msg,
			QCoreApplication::tr("Download"),
			QCoreApplication::tr("Visit Homepage"),
			QCoreApplication::tr("Cancel"),
			0, 2);

		if (result == 0) // download
		{
			return QDesktopServices::openUrl(QUrl(onlineVersion.downloadUrl));
		}
		else if (result == 1) // visit homepage
		{
			return QDesktopServices::openUrl(QUrl(onlineVersion.infoUrl));
		}
	}	

    int dlgResult {0};

    if (mode == "ask")
    {
        dlgResult = ShowSplashScreen();
    }
    else if (mode == "single")
    {
        dlgResult = QDialog::Accepted;
    }
    else if (mode == "team")
    {
        dlgResult = QDialog::Accepted + 1;
    }
    if (dlgResult == 0)
    {
        return NO_ERROR;
    }

    std::unique_ptr<MainWindowBase> pMainWnd {nullptr};

	if (dlgResult == QDialog::Accepted + 1)
	{
        pMainWnd = std::make_unique<MainWindowTeam>();
	}
	else
	{
		pMainWnd = std::make_unique<MainWindow>();
	}

	try
	{
		pMainWnd->Init();
	}
	catch (std::exception const&)
	{
        return NO_ERROR;
	}

	pMainWnd->show();

	return a.exec();
}
