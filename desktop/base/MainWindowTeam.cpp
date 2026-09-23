// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "MainWindowTeam.h"
#include "ui_MainWindowTeam.h"

#include "ScoreScreen.h"
#include "../base/ComboBoxDelegate.h"
#include "../base/ClubManager.h"
#include "../base/ClubManagerDlg.h"
#include "../base/FighterManagerDlg.h"
#include "ModeManagerDlg.h"
#include "../base/View.h"
#include "../base/versioninfo.h"
#include "../core/Controller.h"
#include "../core/ControllerConfig.h"
#include "../core/TournamentModel.h"
#include "TournamentSerialization.h"

#ifdef _WIN32
#include "../gamepad/gamepad.h"
#include <windows.h>
#include <winhttp.h>
#endif

#include "../util/path_helpers.h"
#include "../Widgets/ScaledImage.h"

#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QSignalBlocker>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QSaveFile>
#include <QSplashScreen>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <algorithm>


namespace StrTags
{
static const char* const mode = "Mode";
static const char* const host = "Host";
}

using namespace FMlib;
using namespace Ipponboard;

namespace
{
	bool initialized = false;
	constexpr auto SaveDateFormat = "dd.MM.yyyy";
}

MainWindowTeam::MainWindowTeam(QWidget* parent)
	: MainWindowBase(parent)
	, m_pUi(new Ui::MainWindowTeam)
	, m_pScoreScreen()
	, m_pClubManager()
	, m_htmlScore()
	, m_currentMode()
	, m_host()
	, m_FighterNamesHome()
	, m_FighterNamesGuest()
	, m_FighterIdsHome()
	, m_FighterIdsGuest()
	, m_fighterDelegateHomeRound1(nullptr)
	, m_fighterDelegateHomeRound2(nullptr)
	, m_fighterDelegateGuestRound1(nullptr)
	, m_fighterDelegateGuestRound2(nullptr)
	, m_masterClubs()
	, m_masterTeams()
	, m_masterFighters()
	, m_masterTournamentModes()
	, m_masterRuleSets()
	, m_usingMasterData(false)
	, m_modes()
	, m_modernSetupRoot(nullptr)
	, m_modernHomeRound1(nullptr)
	, m_modernGuestRound1(nullptr)
	, m_modernHomeRound2(nullptr)
	, m_modernGuestRound2(nullptr)
	, m_modernHomeTeamLabel(nullptr)
	, m_modernGuestTeamLabel(nullptr)
	, m_modernResultHomeHeader(nullptr)
	, m_modernResultGuestHeader(nullptr)
	, m_modernResultR1Home(nullptr)
	, m_modernResultR1Guest(nullptr)
	, m_modernResultR2Home(nullptr)
	, m_modernResultR2Guest(nullptr)
	, m_modernResultTotalHome(nullptr)
	, m_modernResultTotalGuest(nullptr)
	, m_modernResultWinner(nullptr)
	, m_modernStatusLabel(nullptr)
{
	m_pUi->setupUi(this);
}

MainWindowTeam::~MainWindowTeam()
{}


void MainWindowTeam::LoadModes(Ipponboard::TournamentMode::List modes, QString selectedMode)
{
	m_pUi->comboBox_mode->clear();
	m_modes.swap(modes);

	for (auto const & mode : m_modes)
	{
		m_pUi->comboBox_mode->addItem(mode.Description(), QVariant(mode.id));
	}
	
	initialized = true;

	auto index = m_pUi->comboBox_mode->findData(QVariant(selectedMode));
	index = index == -1 ? 0 : index;

	if (index != m_pUi->comboBox_mode->currentIndex())
	{
		m_pUi->comboBox_mode->setCurrentIndex(index);
	}
	else
	{
		// re-trigger event, so that dependent controls are updated
		on_comboBox_mode_currentIndexChanged(index);
	}
}


void MainWindowTeam::Init()
{
	m_pClubManager.reset(new Ipponboard::ClubManager());
	m_pScoreScreen.reset(new Ipponboard::ScoreScreen());

	MainWindowBase::Init();

	// set default background
	m_pScoreScreen->setStyleSheet(m_pUi->frame_primary_view->styleSheet());

	// Load the last synchronized server snapshot first. Tournament modes are global system data.
	LoadMasterDataCache_();
	RegisterRuleSetsFromMasterData_();

	QString errMsg;
	Ipponboard::TournamentMode::List modes;
	if (!LoadModesFromMasterData_(modes))
	{
		// Only fallback for a first/offline start before the server has any global modes.
		if (!Ipponboard::TournamentMode::ReadModes(MainWindowTeam::ModeConfigurationFileName(), modes, errMsg))
		{
			QMessageBox::critical(nullptr,
				QCoreApplication::tr("Error reading mode configurations"), errMsg);
			throw std::runtime_error("Initialization failed!");
		}
	}
	LoadModes(modes, m_currentMode);

	//
	// setup data
	//
	m_pUi->dateEdit->setDate(QDate::currentDate());
	update_club_views();

	//m_pUi->comboBox_club_guest->setCurrentIndex(0);

	// Fighter selection is driven by the selected teams' cached rosters.
	// Four completely independent live editors. Each editor has one fixed side.
	m_fighterDelegateHomeRound1 = new ComboBoxDelegate(this);
	m_fighterDelegateHomeRound2 = new ComboBoxDelegate(this);
	m_fighterDelegateGuestRound1 = new ComboBoxDelegate(this);
	m_fighterDelegateGuestRound2 = new ComboBoxDelegate(this);

	const auto homeRosterProvider = [this](const QModelIndex&)
	{
		const QString teamId = m_pUi->comboBox_club_home->currentData().toString();
		return std::make_pair(FighterNamesForTeam_(teamId), FighterIdsForTeam_(teamId));
	};
	const auto guestRosterProvider = [this](const QModelIndex&)
	{
		const QString teamId = m_pUi->comboBox_club_guest->currentData().toString();
		return std::make_pair(FighterNamesForTeam_(teamId), FighterIdsForTeam_(teamId));
	};

	m_fighterDelegateHomeRound1->SetItemProvider(homeRosterProvider);
	m_fighterDelegateHomeRound2->SetItemProvider(homeRosterProvider);
	m_fighterDelegateGuestRound1->SetItemProvider(guestRosterProvider);
	m_fighterDelegateGuestRound2->SetItemProvider(guestRosterProvider);

	m_pUi->tableView_tournament_list1->setItemDelegateForColumn(TournamentModel::eCol_name1, m_fighterDelegateHomeRound1);
	m_pUi->tableView_tournament_list1->setItemDelegateForColumn(TournamentModel::eCol_name2, m_fighterDelegateGuestRound1);
	m_pUi->tableView_tournament_list2->setItemDelegateForColumn(TournamentModel::eCol_name1, m_fighterDelegateHomeRound2);
	m_pUi->tableView_tournament_list2->setItemDelegateForColumn(TournamentModel::eCol_name2, m_fighterDelegateGuestRound2);

	const auto enableOneClickFighterSelection = [this](QTableView* table)
	{
		connect(table, &QTableView::clicked, this, [table](const QModelIndex& index)
		{
			if (index.column() != TournamentModel::eCol_name1 &&
				index.column() != TournamentModel::eCol_name2)
				return;
			table->edit(index);
			QTimer::singleShot(0, table, [table]()
			{
				QComboBox* combo = qobject_cast<QComboBox*>(QApplication::focusWidget());
				if (combo && table->isAncestorOf(combo))
					combo->showPopup();
			});
		});
	};
	enableOneClickFighterSelection(m_pUi->tableView_tournament_list1);
	enableOneClickFighterSelection(m_pUi->tableView_tournament_list2);
	BuildModernTeamSetupUi_();
	// make name columns auto-resizable
    m_pUi->tableView_tournament_list1->horizontalHeader()->setSectionResizeMode(TournamentModel::eCol_name1, QHeaderView::Stretch);
    m_pUi->tableView_tournament_list1->horizontalHeader()->setSectionResizeMode(TournamentModel::eCol_name2, QHeaderView::Stretch);

	// TEMP: hide weight cotrol
//	m_pUi->label_weight->hide();
//	m_pUi->lineEdit_weights->hide();
//	m_pUi->toolButton_weights->hide();
//	m_pUi->gridLayout_main->removeItem(m_pUi->horizontalSpacer_4);
//	delete m_pUi->horizontalSpacer_4;

	//update_weights("-66;-73;-81;-90;+90");
	//FIXME: check why this has not been in branch

	m_pUi->actionAutoAdjustPoints->setChecked(m_pController->IsAutoAdjustPoints());

	UpdateFightNumber_();
	UpdateButtonText_();

	//m_pUi->button_pause->click();	// we start with pause!

	load_autosave_if_available();
}



void MainWindowTeam::UpdateGoldenScoreView()
{
	m_pUi->button_golden_score->setEnabled(m_pController->GetRules()->IsOption_OpenEndGoldenScore());
	m_pUi->button_golden_score->setChecked(m_pController->IsGoldenScore());
}

void MainWindowTeam::closeEvent(QCloseEvent* event)
{
	MainWindowBase::closeEvent(event);

	if (m_pScoreScreen)
	{
		m_pScoreScreen->close();
	}
}

void MainWindowTeam::keyPressEvent(QKeyEvent* event)
{
	const bool isCtrlPressed = event->modifiers().testFlag(Qt::ControlModifier);
	const bool isAltPressed = event->modifiers().testFlag(Qt::AltModifier);

	if (event->matches(QKeySequence::SaveAs))
	{
		on_actionSave_As_triggered();
	}
	else if (event->matches(QKeySequence::Open))
	{
		on_actionLoad_triggered();
	}

	//FIXME: copy and paste handling should be part of the table class!
	if (m_pUi->tabWidget->currentWidget() == m_pUi->tab_view)
	{
		switch (event->key())
		{
		case Qt::Key_Left:
			if (isCtrlPressed && isAltPressed)
			{
				m_pUi->button_prev->click();
				qDebug() << "Button [ Prev ] was triggered by keyboard";
			}
			else
			{
				MainWindowBase::keyPressEvent(event);
			}

			break;

		case Qt::Key_Right:
			if (isCtrlPressed && isAltPressed)
			{
				m_pUi->button_next->click();
				qDebug() << "Button [ Next ] was triggered by keyboard";
			}
			else
			{
				MainWindowBase::keyPressEvent(event);
			}

			break;

		case Qt::Key_F4:
			m_pUi->button_pause->click();
			qDebug() << "Button [ ResultScreen ] was triggered by keyboard";
			break;

		default:
			MainWindowBase::keyPressEvent(event);
			break;
		}
	}
	else if (m_pUi->tabWidget->currentWidget() == m_pUi->tab_score_table)
	{
		if (event->matches(QKeySequence::Copy))
		{
			if (QApplication::focusWidget() == m_pUi->tableView_tournament_list1)
			{
				slot_copy_cell_content_list1();
			}
			else if (QApplication::focusWidget() == m_pUi->tableView_tournament_list2)
			{
				slot_copy_cell_content_list2();
			}
		}
		else if (event->matches(QKeySequence::Paste))
		{
			if (QApplication::focusWidget() == m_pUi->tableView_tournament_list1)
			{
				slot_paste_cell_content_list1();
			}
			else if (QApplication::focusWidget() == m_pUi->tableView_tournament_list2)
			{
				slot_paste_cell_content_list2();
			}
		}
		else if (event->matches(QKeySequence::Delete))
		{
			if (QApplication::focusWidget() == m_pUi->tableView_tournament_list1)
			{
				slot_clear_cell_content_list1();
			}
			else if (QApplication::focusWidget() == m_pUi->tableView_tournament_list2)
			{
				slot_clear_cell_content_list2();
			}
		}
		else
		{
			MainWindowBase::keyPressEvent(event);
		}
	}
	else
	{
		//TODO: handle view keys
		//FIXME: handling should be part of the view class!
		//switch (event->key())
		//{
		//default:
		MainWindowBase::keyPressEvent(event);
		//    break;
		//}
	}
}

QStringList MainWindowTeam::get_list_templates()
{
	QDir dir(TournamentMode::str_TemplateDirName);
	QStringList filters;
	filters.append("*.html");
	return dir.entryList(filters, QDir::Files, QDir::Name);
}

void MainWindowTeam::write_specific_settings(QSettings& settings)
{
	settings.beginGroup(EditionNameShort());
	{
		settings.remove("");
		settings.setValue(StrTags::mode, m_currentMode);
		settings.setValue(StrTags::host, m_host);
		settings.setValue(str_tag_LabelHome, m_pController->GetHomeLabel());
		settings.setValue(str_tag_LabelGuest, m_pController->GetGuestLabel());
	}
	settings.endGroup();
}

void MainWindowTeam::read_specific_settings(QSettings& settings)
{
	settings.beginGroup(EditionNameShort());
	{
		m_currentMode = settings.value(StrTags::mode, "").toString();
		m_host = settings.value(StrTags::host, "").toString();

		m_pController->SetLabels(
			settings.value(str_tag_LabelHome, tr("Home")).toString(),
			settings.value(str_tag_LabelGuest, tr("Guest")).toString());
	}
	settings.endGroup();
}

void MainWindowTeam::on_actionManageFighters_triggered()
{
	MainWindowBase::on_actionManageFighters_triggered();

	FighterManagerDlg dlg(m_fighterManager, this);
	dlg.exec();
}

void MainWindowTeam::update_info_text_color(const QColor& color, const QColor& bgColor)
{
	MainWindowBase::update_info_text_color(color, bgColor);
	//m_pScoreScreen->SetInfoTextColor(color, bgColor);
}

void MainWindowTeam::update_text_color_first(const QColor& color, const QColor& bgColor)
{
	MainWindowBase::update_text_color_first(color, bgColor);
	m_pScoreScreen->SetTextColorFirst(color, bgColor);
}

void MainWindowTeam::update_text_color_second(const QColor& color, const QColor& bgColor)
{
	MainWindowBase::update_text_color_second(color, bgColor);
	m_pScoreScreen->SetTextColorSecond(color, bgColor);
}

void MainWindowTeam::update_fighter_name_font(const QFont& font)
{
	MainWindowBase::update_fighter_name_font(font);
	m_pScoreScreen->SetTextFont(font);
}

void MainWindowTeam::update_views()
{
	MainWindowBase::update_views();
	update_score_screen(); // TODO: should be an IView!

	UpdateFightNumber_();
	UpdateButtonText_();
}

bool MainWindowTeam::LoadMasterDataCache_()
{
	QString appDir = QCoreApplication::applicationDirPath();
	QString cacheFile = QDir(appDir).absoluteFilePath(QStringLiteral("../data/masterdata.json"));
	if (!QFile::exists(cacheFile))
	{
		cacheFile = QDir(appDir).absoluteFilePath(QStringLiteral("data/masterdata.json"));
	}
	QFile file(cacheFile);
	if (!file.open(QIODevice::ReadOnly))
	{
		m_usingMasterData = false;
		return false;
	}
	QJsonParseError error;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject())
	{
		m_usingMasterData = false;
		return false;
	}
	const QJsonObject root = doc.object();
	const QJsonObject master = root.value(QStringLiteral("masterdata")).toObject();
	if (master.isEmpty())
	{
		m_usingMasterData = false;
		return false;
	}
	m_masterClubs = master.value(QStringLiteral("clubs")).toArray();
	m_masterTeams = master.value(QStringLiteral("teams")).toArray();
	m_masterFighters = master.value(QStringLiteral("fighters")).toArray();
	m_masterTournamentModes = master.value(QStringLiteral("tournamentModes")).toArray();
	m_masterRuleSets = master.value(QStringLiteral("ruleSets")).toArray();
	m_usingMasterData = !m_masterTeams.isEmpty();
	return m_usingMasterData;
}

void MainWindowTeam::RegisterRuleSetsFromMasterData_()
{
	RulesFactory::ClearDefinitions();
	for (const QJsonValue& value : m_masterRuleSets)
	{
		const QJsonObject o = value.toObject();
		if (o.value(QStringLiteral("status")).toString() == QStringLiteral("inactive"))
			continue;

		RulesDefinition d;
		d.name = o.value(QStringLiteral("name")).toString();
		if (d.name.isEmpty()) continue;
		d.hasYuko = o.value(QStringLiteral("hasYuko")).toBool(true);
		d.awaseteIppon = o.value(QStringLiteral("awaseteIppon")).toBool(true);
		d.openEndGoldenScore = o.value(QStringLiteral("openEndGoldenScore")).toBool(true);
		d.shidoAddsPoint = o.value(QStringLiteral("shidoAddsPoint")).toBool(false);
		d.shidoScoreCounts = o.value(QStringLiteral("shidoScoreCounts")).toBool(false);
		d.maxShidoCount = o.value(QStringLiteral("maxShidoCount")).toInt(2);
		d.maxWazaariCount = o.value(QStringLiteral("maxWazaariCount")).toInt(2);
		d.osaekomiYukoSeconds = o.value(QStringLiteral("osaekomiYukoSeconds")).toInt(5);
		d.osaekomiWazaariSeconds = o.value(QStringLiteral("osaekomiWazaariSeconds")).toInt(10);
		d.osaekomiIpponSeconds = o.value(QStringLiteral("osaekomiIpponSeconds")).toInt(20);
		d.ipponTeamPoints = o.value(QStringLiteral("ipponTeamPoints")).toInt(10);
		d.wazaariTeamPoints = o.value(QStringLiteral("wazaariTeamPoints")).toInt(7);
		d.yukoTeamPoints = o.value(QStringLiteral("yukoTeamPoints")).toInt(5);
		d.shidoTeamPoints = o.value(QStringLiteral("shidoTeamPoints")).toInt(1);
		d.ipponLabel = o.value(QStringLiteral("ipponLabel")).toString(QStringLiteral("Ippon"));
		d.wazaariLabel = o.value(QStringLiteral("wazaariLabel")).toString(QStringLiteral("Waza-ari"));
		d.yukoLabel = o.value(QStringLiteral("yukoLabel")).toString(QStringLiteral("Yuko"));
		d.shidoLabel = o.value(QStringLiteral("shidoLabel")).toString(QStringLiteral("Shido"));
		d.hansokumakeLabel = o.value(QStringLiteral("hansokumakeLabel")).toString(QStringLiteral("Hansoku-make"));
		RulesFactory::RegisterDefinition(d);
	}
}

bool MainWindowTeam::LoadModesFromMasterData_(Ipponboard::TournamentMode::List& modes) const
{
	if (m_masterTournamentModes.isEmpty())
		return false;

	Ipponboard::TournamentMode::List loaded;
	for (const QJsonValue& value : m_masterTournamentModes)
	{
		const QJsonObject o = value.toObject();
		Ipponboard::TournamentMode mode;
		mode.id = o.value(QStringLiteral("id")).toString();
		mode.title = o.value(QStringLiteral("title")).toString();
		mode.subTitle = o.value(QStringLiteral("subTitle")).toString();
		mode.weights = o.value(QStringLiteral("weights")).toString();
		mode.listTemplate = o.value(QStringLiteral("listTemplate")).toString();
		mode.options = o.value(QStringLiteral("options")).toString();
		mode.rules = o.value(QStringLiteral("rules")).toString(mode.rules);
		mode.nRounds = o.value(QStringLiteral("nRounds")).toInt(1);
		mode.fightTimeInSeconds = o.value(QStringLiteral("fightTimeInSeconds")).toInt(240);
		mode.weightsAreDoubled = o.value(QStringLiteral("weightsAreDoubled")).toBool(false);
		const QString overrides = o.value(QStringLiteral("fightTimeOverrides")).toString();
		if (!overrides.isEmpty())
			Ipponboard::TournamentMode::ExtractFightTimeOverrides(overrides, mode.fightTimeOverrides);

		if (!mode.id.isEmpty() && !mode.title.isEmpty() && !mode.weights.isEmpty() && !mode.listTemplate.isEmpty())
			loaded.push_back(mode);
	}
	if (loaded.empty())
		return false;

	std::sort(begin(loaded), end(loaded));
	modes.swap(loaded);
	return true;
}

static QJsonArray TournamentModesToJson(const Ipponboard::TournamentMode::List& modes)
{
	QJsonArray items;
	for (const auto& mode : modes)
	{
		QJsonObject o;
		o.insert(QStringLiteral("id"), mode.id);
		o.insert(QStringLiteral("title"), mode.title);
		o.insert(QStringLiteral("subTitle"), mode.subTitle);
		o.insert(QStringLiteral("weights"), mode.weights);
		o.insert(QStringLiteral("listTemplate"), mode.listTemplate);
		o.insert(QStringLiteral("options"), mode.options);
		o.insert(QStringLiteral("rules"), mode.rules);
		o.insert(QStringLiteral("nRounds"), mode.nRounds);
		o.insert(QStringLiteral("fightTimeInSeconds"), mode.fightTimeInSeconds);
		o.insert(QStringLiteral("weightsAreDoubled"), mode.weightsAreDoubled);
		o.insert(QStringLiteral("fightTimeOverrides"), mode.GetFightTimeOverridesString());
		items.append(o);
	}
	return items;
}

bool MainWindowTeam::UploadTournamentModes_(const Ipponboard::TournamentMode::List& modes, QString& errorMsg)
{
	errorMsg.clear();
	QJsonObject root;
	root.insert(QStringLiteral("tournamentModes"), TournamentModesToJson(modes));
	const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

#ifdef _WIN32
	HINTERNET session = WinHttpOpen(L"Ipponboard-Meschede/0.2.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) { errorMsg = QStringLiteral("Server-Sync konnte nicht gestartet werden."); return false; }
	WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);
	HINTERNET connect = WinHttpConnect(session, L"test-liga.paul-meschede.de", INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET request = connect ? WinHttpOpenRequest(connect, L"PUT", L"/api/masterdata/tournamentModes",
		nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
	bool ok = request && WinHttpSendRequest(request, L"Content-Type: application/json\r\n", -1L,
		(LPVOID)payload.constData(), static_cast<DWORD>(payload.size()), static_cast<DWORD>(payload.size()), 0)
		&& WinHttpReceiveResponse(request, nullptr);
	DWORD status = 0, size = sizeof(status);
	if (ok) ok = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status >= 200 && status < 300;
	if (request) WinHttpCloseHandle(request);
	if (connect) WinHttpCloseHandle(connect);
	WinHttpCloseHandle(session);
	if (!ok) errorMsg = status ? QStringLiteral("Server lehnt die Modusänderung ab (HTTP %1).").arg(status)
		: QStringLiteral("Server nicht erreichbar. Globale Modi wurden nicht gespeichert.");
	return ok;
#else
	errorMsg = QStringLiteral("Globale Modus-Synchronisation ist auf dieser Plattform noch nicht implementiert.");
	return false;
#endif
}

void MainWindowTeam::SaveTournamentModesToCache_(const Ipponboard::TournamentMode::List& modes)
{
	QString appDir = QCoreApplication::applicationDirPath();
	QString cacheFile = QDir(appDir).absoluteFilePath(QStringLiteral("../data/masterdata.json"));
	if (!QFile::exists(cacheFile))
		cacheFile = QDir(appDir).absoluteFilePath(QStringLiteral("data/masterdata.json"));

	QFile file(cacheFile);
	if (!file.open(QIODevice::ReadOnly))
		return;
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	file.close();
	if (error.error != QJsonParseError::NoError || !doc.isObject())
		return;

	QJsonObject root = doc.object();
	QJsonObject master = root.value(QStringLiteral("masterdata")).toObject();
	master.insert(QStringLiteral("tournamentModes"), TournamentModesToJson(modes));
	root.insert(QStringLiteral("masterdata"), master);

	QSaveFile out(cacheFile);
	if (out.open(QIODevice::WriteOnly))
	{
		out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
		out.commit();
	}
}

QStringList MainWindowTeam::FighterNamesForTeam_(const QString& teamId) const
{
	QStringList result;
	const QStringList ids = FighterIdsForTeam_(teamId);
	for (const QString& fighterId : ids)
	{
		for (const QJsonValue& value : m_masterFighters)
		{
			const QJsonObject fighter = value.toObject();
			if (fighter.value(QStringLiteral("id")).toString() != fighterId)
				continue;
			const QString name = (fighter.value(QStringLiteral("firstName")).toString() + QStringLiteral(" ") +
				fighter.value(QStringLiteral("lastName")).toString()).trimmed();
			result.append(name);
			break;
		}
	}
	return result;
}

QStringList MainWindowTeam::FighterIdsForTeam_(const QString& teamId) const
{
	QStringList result;
	QJsonArray fighterIds;
	for (const QJsonValue& value : m_masterTeams)
	{
		const QJsonObject team = value.toObject();
		if (team.value(QStringLiteral("id")).toString() == teamId)
		{
			fighterIds = team.value(QStringLiteral("fighterIds")).toArray();
			break;
		}
	}
	for (const QJsonValue& idValue : fighterIds)
	{
		const QString fighterId = idValue.toString();
		for (const QJsonValue& value : m_masterFighters)
		{
			const QJsonObject fighter = value.toObject();
			if (fighter.value(QStringLiteral("id")).toString() != fighterId)
				continue;
			if (fighter.value(QStringLiteral("status")).toString() != QStringLiteral("inactive"))
				result.append(fighterId);
			break;
		}
	}
	return result;
}

void MainWindowTeam::UpdateTeamFighterDelegates_()
{
	if (!m_usingMasterData)
		return;
	const QString homeId = m_pUi->comboBox_club_home->currentData().toString();
	const QString guestId = m_pUi->comboBox_club_guest->currentData().toString();
	m_FighterIdsHome = FighterIdsForTeam_(homeId);
	m_FighterIdsGuest = FighterIdsForTeam_(guestId);
	m_FighterNamesHome = FighterNamesForTeam_(homeId);
	m_FighterNamesGuest = FighterNamesForTeam_(guestId);
	// Editors resolve their list live when opened. No cached list is pushed into delegates here.
}

void MainWindowTeam::update_club_views()
{
	QString oldHost = m_host;
	const QString oldHomeId = m_pUi->comboBox_club_home->currentData().toString();
	const QString oldGuestId = m_pUi->comboBox_club_guest->currentData().toString();

	m_pUi->comboBox_club_host->clear();
	m_pUi->comboBox_club_home->clear();
	m_pUi->comboBox_club_guest->clear();

	if (m_usingMasterData)
	{
		for (const QJsonValue& value : m_masterClubs)
		{
			const QJsonObject club = value.toObject();
			if (club.value(QStringLiteral("status")).toString() == QStringLiteral("inactive"))
				continue;
			m_pUi->comboBox_club_host->addItem(club.value(QStringLiteral("name")).toString(),
				club.value(QStringLiteral("id")).toString());
		}
		for (const QJsonValue& value : m_masterTeams)
		{
			const QJsonObject team = value.toObject();
			if (team.value(QStringLiteral("status")).toString() == QStringLiteral("inactive"))
				continue;
			const QString name = team.value(QStringLiteral("name")).toString();
			const QString id = team.value(QStringLiteral("id")).toString();
			if (!name.isEmpty() && !id.isEmpty())
			{
				m_pUi->comboBox_club_home->addItem(name, id);
				m_pUi->comboBox_club_guest->addItem(name, id);
			}
		}
	}
	else
	{
		for (int i = 0; i < m_pClubManager->ClubCount(); ++i)
		{
			Ipponboard::Club club;
			m_pClubManager->GetClub(i, club);
			QIcon icon(club.logoFile);
			m_pUi->comboBox_club_host->addItem(icon, club.name);
			m_pUi->comboBox_club_home->addItem(icon, club.name);
			m_pUi->comboBox_club_guest->addItem(icon, club.name);
		}
	}

	m_host = oldHost;
	int hostIndex = m_pUi->comboBox_club_host->findText(m_host);
	if (hostIndex < 0)
		hostIndex = 0;
	m_pUi->comboBox_club_host->setCurrentIndex(hostIndex);

	if (m_usingMasterData)
	{
		int homeIndex = m_pUi->comboBox_club_home->findData(oldHomeId);
		if (homeIndex < 0)
			homeIndex = 0;
		m_pUi->comboBox_club_home->setCurrentIndex(homeIndex);

		int guestIndex = m_pUi->comboBox_club_guest->findData(oldGuestId);
		if (guestIndex < 0)
			guestIndex = m_pUi->comboBox_club_guest->count() > 1 ? 1 : 0;
		m_pUi->comboBox_club_guest->setCurrentIndex(guestIndex);
	}
	else
	{
		m_pUi->comboBox_club_home->setCurrentIndex(hostIndex);
	}

	m_pUi->lineEdit_location->setText(m_pClubManager->GetAddress(m_host));
	UpdateTeamFighterDelegates_();
}


void MainWindowTeam::BuildModernTeamSetupUi_()
{
	if (m_modernSetupRoot)
		return;

	// Hide the legacy list UI. The underlying models/controllers remain active.
	const QList<QWidget*> legacyWidgets = {
		m_pUi->tableView_tournament_list1, m_pUi->tableView_tournament_list2,
		m_pUi->label_intermediate_result, m_pUi->lineEdit_wins_intermediate,
		m_pUi->lineEdit_score_intermediate, m_pUi->label_2, m_pUi->label_3,
		m_pUi->pushButton_copySwitched, m_pUi->label_final_score,
		m_pUi->lineEdit_wins, m_pUi->lineEdit_score, m_pUi->label_final_wins,
		m_pUi->label_final_sub_score, m_pUi->toolButton_weights,
		m_pUi->toolButton_team_home, m_pUi->toolButton_team_guest,
		m_pUi->label_home, m_pUi->label_guest, m_pUi->label_date,
		m_pUi->label_location, m_pUi->label_host, m_pUi->label_mode, m_pUi->line
	};
	for (QWidget* w : legacyWidgets) if (w) w->hide();

	m_pUi->gridLayout_main->setContentsMargins(0,0,0,0);
	m_pUi->gridLayout_main->setSpacing(0);

	m_modernSetupRoot = new QWidget(m_pUi->tab_score_table);
	m_modernSetupRoot->setObjectName(QStringLiteral("modernTeamSetupRoot"));
	auto* outer = new QVBoxLayout(m_modernSetupRoot);
	outer->setContentsMargins(12, 10, 12, 10);
	outer->setSpacing(10);

	auto* scroll = new QScrollArea(m_modernSetupRoot);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	auto* content = new QWidget(scroll);
	content->setObjectName(QStringLiteral("modernContent"));
	auto* contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(0,0,0,0);
	contentLayout->setSpacing(10);

	auto* titleRow = new QHBoxLayout();
	auto* back = new QLabel(QStringLiteral("‹  Zur Übersicht"), content);
	back->setObjectName(QStringLiteral("modernBack"));
	auto* title = new QLabel(QStringLiteral("Mannschaftskampf bearbeiten"), content);
	title->setObjectName(QStringLiteral("modernTitle"));
	titleRow->addWidget(back);
	titleRow->addSpacing(18);
	titleRow->addWidget(title);
	titleRow->addStretch();
	contentLayout->addLayout(titleRow);

	auto* details = new QFrame(content);
	details->setObjectName(QStringLiteral("detailsCard"));
	auto* dg = new QGridLayout(details);
	dg->setContentsMargins(14,12,14,12);
	dg->setHorizontalSpacing(12);
	dg->setVerticalSpacing(9);

	auto addField = [dg](int row, int col, const QString& label, QWidget* widget)
	{
		auto* l = new QLabel(label);
		l->setObjectName(QStringLiteral("fieldLabel"));
		dg->addWidget(l,row,col);
		dg->addWidget(widget,row,col+1);
	};

	addField(0,0,QStringLiteral("Wettkampf / Liga"),m_pUi->comboBox_mode);
	addField(1,0,QStringLiteral("Datum"),m_pUi->dateEdit);
	addField(0,2,QStringLiteral("Ort"),m_pUi->lineEdit_location);
	addField(1,2,QStringLiteral("Ausrichter"),m_pUi->comboBox_club_host);
	addField(0,4,QStringLiteral("Heimmannschaft"),m_pUi->comboBox_club_home);
	addField(1,4,QStringLiteral("Gastmannschaft"),m_pUi->comboBox_club_guest);
	for(int c=1;c<=5;c+=2) dg->setColumnStretch(c,1);
	contentLayout->addWidget(details);

	auto buildLineupTable = [this](QWidget* parent, bool guest)->QTableWidget*
	{
		auto* t = new QTableWidget(parent);
		t->setColumnCount(4);
		t->setHorizontalHeaderLabels({QStringLiteral("#"),QStringLiteral("Gewicht"),QStringLiteral("Kämpfer"),QStringLiteral("Jahrgang")});
		t->verticalHeader()->hide();
		t->horizontalHeader()->setStretchLastSection(false);
		t->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
		t->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
		t->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
		t->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
		t->setSelectionMode(QAbstractItemView::NoSelection);
		t->setEditTriggers(QAbstractItemView::NoEditTriggers);
		t->setFocusPolicy(Qt::NoFocus);
		t->setShowGrid(true);
		t->setAlternatingRowColors(false);
		t->setObjectName(guest ? QStringLiteral("guestLineupTable") : QStringLiteral("homeLineupTable"));
		return t;
	};

	auto buildTeamCard = [&](bool guest)->QFrame*
	{
		auto* card = new QFrame(content);
		card->setObjectName(guest ? QStringLiteral("guestCard") : QStringLiteral("homeCard"));
		auto* vl = new QVBoxLayout(card);
		vl->setContentsMargins(10,10,10,10);
		vl->setSpacing(7);

		auto* head = new QHBoxLayout();
		auto* badge = new QLabel(guest ? QStringLiteral("BLAU") : QStringLiteral("WEISS"), card);
		badge->setObjectName(guest ? QStringLiteral("guestBadge") : QStringLiteral("homeBadge"));
		auto* names = new QVBoxLayout();
		QLabel*& nameLabel = guest ? m_modernGuestTeamLabel : m_modernHomeTeamLabel;
		nameLabel = new QLabel(card);
		nameLabel->setObjectName(QStringLiteral("teamName"));
		auto* role = new QLabel(guest ? QStringLiteral("Gastmannschaft") : QStringLiteral("Heimmannschaft"), card);
		role->setObjectName(QStringLiteral("teamRole"));
		names->addWidget(nameLabel);
		names->addWidget(role);
		auto* rosterBtn = new QPushButton(QStringLiteral("Kader anzeigen"),card);
		rosterBtn->setObjectName(QStringLiteral("secondaryButton"));
		connect(rosterBtn,&QPushButton::clicked,this,[this,guest](){ShowModernRoster_(guest?FighterEnum::Second:FighterEnum::First);});
		head->addWidget(badge);
		head->addLayout(names);
		head->addStretch();
		head->addWidget(rosterBtn);
		vl->addLayout(head);

		auto addRound = [&](const QString& titleText, int round)
		{
			auto* roundTitle = new QLabel(titleText,card);
			roundTitle->setObjectName(guest ? QStringLiteral("guestRoundTitle") : QStringLiteral("homeRoundTitle"));
			vl->addWidget(roundTitle);
			QTableWidget* table = buildLineupTable(card,guest);
			if(!guest && round==0) m_modernHomeRound1=table;
			if(!guest && round==1) m_modernHomeRound2=table;
			if(guest && round==0) m_modernGuestRound1=table;
			if(guest && round==1) m_modernGuestRound2=table;
			vl->addWidget(table);
		};
		addRound(QStringLiteral("⌄  Hinrunde (1. Durchgang)"),0);
		addRound(QStringLiteral("⌄  Rückrunde (2. Durchgang)"),1);

		auto* buttons = new QHBoxLayout();
		auto* copyBtn = new QPushButton(QStringLiteral("Hinrunde in Rückrunde übernehmen"),card);
		auto* clearBtn = new QPushButton(QStringLiteral("Aufstellung leeren"),card);
		copyBtn->setObjectName(QStringLiteral("secondaryButton"));
		clearBtn->setObjectName(QStringLiteral("secondaryButton"));
		connect(copyBtn,&QPushButton::clicked,this,[this,guest](){CopyModernLineup_(guest?FighterEnum::Second:FighterEnum::First);});
		connect(clearBtn,&QPushButton::clicked,this,[this,guest](){ClearModernLineup_(guest?FighterEnum::Second:FighterEnum::First);});
		buttons->addWidget(copyBtn);
		buttons->addWidget(clearBtn);
		vl->addLayout(buttons);
		return card;
	};

	auto* teams = new QHBoxLayout();
	teams->setSpacing(10);
	teams->addWidget(buildTeamCard(false),1);
	teams->addWidget(buildTeamCard(true),1);
	contentLayout->addLayout(teams);

	auto* result = new QFrame(content);
	result->setObjectName(QStringLiteral("resultCard"));
	auto* rg = new QGridLayout(result);
	rg->setContentsMargins(14,10,14,12);
	rg->setHorizontalSpacing(12);
	rg->setVerticalSpacing(7);
	auto* rt = new QLabel(QStringLiteral("3. Ergebnis"),result);
	rt->setObjectName(QStringLiteral("sectionTitle"));
	rg->addWidget(rt,0,0,1,5);

	m_modernResultHomeHeader = new QLabel(result);
	m_modernResultGuestHeader = new QLabel(result);
	m_modernResultHomeHeader->setAlignment(Qt::AlignCenter);
	m_modernResultGuestHeader->setAlignment(Qt::AlignCenter);
	rg->addWidget(new QLabel(QStringLiteral("Durchgang"),result),1,0);
	rg->addWidget(m_modernResultHomeHeader,1,1);
	rg->addWidget(new QLabel(QStringLiteral(":"),result),1,2);
	rg->addWidget(m_modernResultGuestHeader,1,3);
	rg->addWidget(new QLabel(QStringLiteral("Sieger Durchgang"),result),1,4);

	auto makeScore = [result](){ auto* l=new QLabel(QStringLiteral("0"),result); l->setAlignment(Qt::AlignCenter); l->setObjectName(QStringLiteral("scoreBox")); return l; };
	m_modernResultR1Home=makeScore(); m_modernResultR1Guest=makeScore();
	m_modernResultR2Home=makeScore(); m_modernResultR2Guest=makeScore();
	m_modernResultTotalHome=makeScore(); m_modernResultTotalGuest=makeScore();
	auto* winnerR1=new QLabel(QStringLiteral("–"),result);
	auto* winnerR2=new QLabel(QStringLiteral("–"),result);
	m_modernResultWinner=new QLabel(QStringLiteral("–"),result);
	m_modernResultWinner->setObjectName(QStringLiteral("winnerLabel"));

	rg->addWidget(new QLabel(QStringLiteral("Hinrunde (1. Durchgang)"),result),2,0);
	rg->addWidget(m_modernResultR1Home,2,1); rg->addWidget(new QLabel(QStringLiteral(":"),result),2,2); rg->addWidget(m_modernResultR1Guest,2,3); rg->addWidget(winnerR1,2,4);
	rg->addWidget(new QLabel(QStringLiteral("Rückrunde (2. Durchgang)"),result),3,0);
	rg->addWidget(m_modernResultR2Home,3,1); rg->addWidget(new QLabel(QStringLiteral(":"),result),3,2); rg->addWidget(m_modernResultR2Guest,3,3); rg->addWidget(winnerR2,3,4);
	auto* totalLabel = new QLabel(QStringLiteral("Gesamtergebnis"),result); totalLabel->setObjectName(QStringLiteral("totalLabel"));
	rg->addWidget(totalLabel,4,0); rg->addWidget(m_modernResultTotalHome,4,1); rg->addWidget(new QLabel(QStringLiteral(":"),result),4,2); rg->addWidget(m_modernResultTotalGuest,4,3); rg->addWidget(m_modernResultWinner,4,4);

	// Round winner labels follow the same score sources.
	connect(m_pUi->lineEdit_wins_intermediate,&QLineEdit::textChanged,this,[this,winnerR1,winnerR2](){
		UpdateModernResults_();
		const auto parse=[](const QString&s){QStringList p=s.split(':');return p.size()==2?QPair<int,int>(p[0].trimmed().toInt(),p[1].trimmed().toInt()):QPair<int,int>(0,0);};
		auto r1=parse(m_pUi->lineEdit_wins_intermediate->text()), total=parse(m_pUi->lineEdit_wins->text());
		auto setWinner=[this](QLabel* l,int a,int b){l->setText(a==b?QStringLiteral("–"):(a>b?m_pUi->comboBox_club_home->currentText():m_pUi->comboBox_club_guest->currentText()));};
		setWinner(winnerR1,r1.first,r1.second);
		setWinner(winnerR2,total.first-r1.first,total.second-r1.second);
	});
	connect(m_pUi->lineEdit_wins,&QLineEdit::textChanged,this,[this,winnerR1,winnerR2](){
		UpdateModernResults_();
		const auto parse=[](const QString&s){QStringList p=s.split(':');return p.size()==2?QPair<int,int>(p[0].trimmed().toInt(),p[1].trimmed().toInt()):QPair<int,int>(0,0);};
		auto r1=parse(m_pUi->lineEdit_wins_intermediate->text()), total=parse(m_pUi->lineEdit_wins->text());
		auto setWinner=[this](QLabel* l,int a,int b){l->setText(a==b?QStringLiteral("–"):(a>b?m_pUi->comboBox_club_home->currentText():m_pUi->comboBox_club_guest->currentText()));};
		setWinner(winnerR1,r1.first,r1.second);
		setWinner(winnerR2,total.first-r1.first,total.second-r1.second);
	});
	contentLayout->addWidget(result);

	auto* bottom = new QHBoxLayout();
	auto* statusCard = new QFrame(content);
	statusCard->setObjectName(QStringLiteral("statusCard"));
	auto* sl = new QHBoxLayout(statusCard);
	sl->setContentsMargins(12,8,12,8);
	auto* st = new QLabel(QStringLiteral("4. Status"),statusCard); st->setObjectName(QStringLiteral("sectionTitle"));
	m_modernStatusLabel = new QLabel(QStringLiteral("●  In Bearbeitung"),statusCard); m_modernStatusLabel->setObjectName(QStringLiteral("statusPill"));
	sl->addWidget(st); sl->addWidget(m_modernStatusLabel); sl->addStretch();

	auto* actionCard = new QFrame(content);
	actionCard->setObjectName(QStringLiteral("statusCard"));
	auto* al = new QHBoxLayout(actionCard);
	al->setContentsMargins(12,8,12,8);
	auto* at = new QLabel(QStringLiteral("5. Aktionen"),actionCard); at->setObjectName(QStringLiteral("sectionTitle"));
	auto* save = new QPushButton(QStringLiteral("Speichern"),actionCard); save->setObjectName(QStringLiteral("secondaryButton"));
	auto* start = new QPushButton(QStringLiteral("▶  Wettkampf starten"),actionCard); start->setObjectName(QStringLiteral("startButton"));
	connect(save,&QPushButton::clicked,this,[this](){
		SaveTournamentToFile_(fm::GetAppConfigFilePath(TournamentSerialization::AutoSaveFilename));
		if(m_modernStatusLabel)m_modernStatusLabel->setText(QStringLiteral("●  Gespeichert"));
	});
	connect(start,&QPushButton::clicked,this,[this](){m_pUi->tabWidget->setCurrentWidget(m_pUi->tab_view);});
	al->addWidget(at); al->addStretch(); al->addWidget(save); al->addWidget(start);
	bottom->addWidget(statusCard,1);
	bottom->addWidget(actionCard,1);
	contentLayout->addLayout(bottom);
	contentLayout->addStretch();

	scroll->setWidget(content);
	outer->addWidget(scroll);
	m_pUi->verticalLayout_7->insertWidget(0,m_modernSetupRoot,1);

	m_pUi->menuBar->setStyleSheet(QStringLiteral(
		"QMenuBar{background:#102d49;color:white;padding:4px 8px;} QMenuBar::item{padding:6px 12px;background:transparent;} QMenuBar::item:selected{background:#1d4669;}"));
	m_pUi->tabWidget->setStyleSheet(QStringLiteral(
		"QTabBar::tab{padding:10px 8px;} QTabBar::tab:selected{font-weight:700;}"));

	m_modernSetupRoot->setStyleSheet(QStringLiteral(
		"#modernContent{background:#f4f7fb;color:#10243b;}"
		"#modernTitle{font-size:20px;font-weight:800;color:#10243b;} #modernBack{color:#234c75;font-weight:600;}"
		"#detailsCard,#resultCard,#statusCard{background:white;border:1px solid #d8e0e8;border-radius:7px;}"
		"#fieldLabel{font-weight:700;color:#1c3046;} QComboBox,QDateEdit,QLineEdit{min-height:30px;border:1px solid #c7d1dc;border-radius:4px;background:white;padding:0 8px;}"
		"#homeCard{background:white;border:1px solid #d8e0e8;border-radius:8px;} #guestCard{background:#eaf4ff;border:1px solid #4c9be8;border-radius:8px;}"
		"#homeBadge{background:white;border:2px solid #d6dce3;border-radius:5px;padding:10px 8px;font-weight:900;color:#1b2c42;}"
		"#guestBadge{background:#0b5da8;border:2px solid #0b5da8;border-radius:5px;padding:10px 8px;font-weight:900;color:white;}"
		"#teamName{font-size:17px;font-weight:800;} #teamRole{color:#53677d;} #homeRoundTitle{font-weight:800;padding:6px;background:#f7f8fa;border:1px solid #e0e4e9;}"
		"#guestRoundTitle{font-weight:800;padding:6px;background:#0b5da8;color:white;border:1px solid #0b5da8;}"
		"QTableWidget{background:white;border:1px solid #d9e0e7;gridline-color:#e1e6eb;} QHeaderView::section{background:#f1f4f7;color:#18304a;font-weight:800;border:0;border-right:1px solid #d9e0e7;padding:5px;}"
		"#guestLineupTable QHeaderView::section{background:#0b5da8;color:white;border-right:1px solid #3a7db9;} #guestLineupTable{border:1px solid #5a9bd5;}"
		"#secondaryButton{min-height:30px;border:1px solid #aebdcb;border-radius:4px;background:#f8fafc;color:#173653;padding:0 12px;} #secondaryButton:hover{background:#edf3f8;}"
		"#sectionTitle{font-size:16px;font-weight:800;} #scoreBox{border:1px solid #b8c9d8;border-radius:4px;background:#f5f9fd;padding:5px 16px;font-weight:800;font-size:16px;}"
		"#totalLabel,#winnerLabel{font-weight:900;} #statusPill{background:#fff1c9;border:1px solid #efc45c;border-radius:12px;padding:4px 10px;color:#8a5a00;font-weight:700;}"
		"#startButton{min-height:34px;border:0;border-radius:4px;background:#149447;color:white;font-weight:800;padding:0 18px;} #startButton:hover{background:#117c3d;}"
	));

	RefreshModernTeamSetupUi_();
}

QString MainWindowTeam::FighterYear_(const QString& fighterId) const
{
	for (const QJsonValue& value : m_masterFighters)
	{
		const QJsonObject f = value.toObject();
		if (f.value(QStringLiteral("id")).toString() != fighterId) continue;
		const QString birth = f.value(QStringLiteral("birthDate")).toString();
		if (birth.size() >= 4) return birth.left(4);
		break;
	}
	return QStringLiteral("–");
}

void MainWindowTeam::PopulateModernLineupTable_(QTableWidget* table, int round, Ipponboard::FighterEnum side)
{
	if (!table || round < 0 || round >= m_pController->GetRoundCount()) return;
	auto* model = m_pController->GetTournamentScoreModel(round).get();
	if (!model) return;

	QSignalBlocker blocker(table);
	const int rows = model->rowCount(QModelIndex());
	table->setRowCount(rows);
	table->setMinimumHeight(35 + rows * 34);
	table->setMaximumHeight(35 + rows * 34);

	const bool guest = side == FighterEnum::Second;
	const int nameColumn = guest ? TournamentModel::eCol_name2 : TournamentModel::eCol_name1;
	const QComboBox* teamBox = guest ? m_pUi->comboBox_club_guest : m_pUi->comboBox_club_home;
	const QString teamId = teamBox->currentData().toString();
	const QStringList names = FighterNamesForTeam_(teamId);
	const QStringList ids = FighterIdsForTeam_(teamId);

	for (int row=0; row<rows; ++row)
	{
		auto* no = new QTableWidgetItem(QString::number(row+1)); no->setTextAlignment(Qt::AlignCenter);
		auto* weight = new QTableWidgetItem(model->data(model->index(row,TournamentModel::eCol_weight),Qt::DisplayRole).toString()); weight->setTextAlignment(Qt::AlignCenter);
		table->setItem(row,0,no);
		table->setItem(row,1,weight);

		auto* combo = new QComboBox(table);
		combo->setEditable(true);
		combo->setInsertPolicy(QComboBox::NoInsert);
		combo->addItem(QStringLiteral("– nicht aufgestellt –"),QStringLiteral("__NOT_SET__"));
		for(int i=0;i<names.size();++i) combo->addItem(names.at(i), i<ids.size()?ids.at(i):QString());
		if(QCompleter* completer=combo->completer())
		{
			completer->setCaseSensitivity(Qt::CaseInsensitive);
			completer->setFilterMode(Qt::MatchContains);
			completer->setCompletionMode(QCompleter::PopupCompletion);
		}

		const QModelIndex mi = model->index(row,nameColumn);
		const QString currentName = model->data(mi,Qt::DisplayRole).toString().trimmed();
		const QString currentId = model->data(mi,Qt::UserRole).toString();
		int pos = currentId.isEmpty() ? -1 : combo->findData(currentId);
		if(pos>=0) combo->setCurrentIndex(pos);
		else if(currentName.isEmpty() || currentName==QStringLiteral("--")) combo->setCurrentIndex(0);
		else {combo->setCurrentIndex(-1);combo->setEditText(currentName);}
		table->setCellWidget(row,2,combo);

		auto* year = new QTableWidgetItem(currentId.isEmpty()?QStringLiteral("–"):FighterYear_(currentId));
		year->setTextAlignment(Qt::AlignCenter);
		table->setItem(row,3,year);

		auto commit = [this,table,model,mi,combo,row]()
		{
			QString id;
			QString text = combo->currentText().trimmed();
			const int selected = combo->currentIndex();
			if(selected>=0) id=combo->itemData(selected).toString();
			if(id==QStringLiteral("__NOT_SET__") || text==QStringLiteral("– nicht aufgestellt –"))
			{
				model->setData(mi,QString(),Qt::EditRole);
				model->setData(mi,QString(),Qt::UserRole);
				if(auto* y=table->item(row,3)) y->setText(QStringLiteral("–"));
				return;
			}
			if(selected<0 || id.isEmpty())
			{
				model->setData(mi,text,Qt::EditRole);
				model->setData(mi,QString(),Qt::UserRole);
				if(auto* y=table->item(row,3)) y->setText(QStringLiteral("–"));
			}
			else
			{
				model->setData(mi,text,Qt::EditRole);
				model->setData(mi,id,Qt::UserRole);
				if(auto* y=table->item(row,3)) y->setText(FighterYear_(id));
			}
		};
		connect(combo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[commit](int){commit();});
		if(combo->lineEdit()) connect(combo->lineEdit(),&QLineEdit::editingFinished,this,commit);
	}
}

void MainWindowTeam::RefreshModernTeamSetupUi_()
{
	if (!m_modernSetupRoot) return;
	const QString home=m_pUi->comboBox_club_home->currentText();
	const QString guest=m_pUi->comboBox_club_guest->currentText();
	if(m_modernHomeTeamLabel)m_modernHomeTeamLabel->setText(home);
	if(m_modernGuestTeamLabel)m_modernGuestTeamLabel->setText(guest);
	if(m_modernResultHomeHeader)m_modernResultHomeHeader->setText(home+QStringLiteral(" (Heim)"));
	if(m_modernResultGuestHeader)m_modernResultGuestHeader->setText(guest+QStringLiteral(" (Gast)"));

	PopulateModernLineupTable_(m_modernHomeRound1,0,FighterEnum::First);
	PopulateModernLineupTable_(m_modernGuestRound1,0,FighterEnum::Second);
	if(m_pController->GetRoundCount()>1)
	{
		m_modernHomeRound2->show(); m_modernGuestRound2->show();
		PopulateModernLineupTable_(m_modernHomeRound2,1,FighterEnum::First);
		PopulateModernLineupTable_(m_modernGuestRound2,1,FighterEnum::Second);
	}
	else
	{
		if(m_modernHomeRound2)m_modernHomeRound2->hide();
		if(m_modernGuestRound2)m_modernGuestRound2->hide();
	}
	UpdateModernResults_();
}

void MainWindowTeam::CopyModernLineup_(Ipponboard::FighterEnum side)
{
	if(m_pController->GetRoundCount()<2)return;
	auto* src=m_pController->GetTournamentScoreModel(0).get();
	auto* dst=m_pController->GetTournamentScoreModel(1).get();
	const int col=side==FighterEnum::Second?TournamentModel::eCol_name2:TournamentModel::eCol_name1;
	const int rows=std::min(src->rowCount(QModelIndex()),dst->rowCount(QModelIndex()));
	for(int r=0;r<rows;++r)
	{
		const QModelIndex s=src->index(r,col), d=dst->index(r,col);
		dst->setData(d,src->data(s,Qt::DisplayRole),Qt::EditRole);
		dst->setData(d,src->data(s,Qt::UserRole),Qt::UserRole);
	}
	RefreshModernTeamSetupUi_();
}

void MainWindowTeam::ClearModernLineup_(Ipponboard::FighterEnum side)
{
	const int col=side==FighterEnum::Second?TournamentModel::eCol_name2:TournamentModel::eCol_name1;
	for(int round=0;round<m_pController->GetRoundCount();++round)
	{
		auto* model=m_pController->GetTournamentScoreModel(round).get();
		for(int r=0;r<model->rowCount(QModelIndex());++r)
		{
			const QModelIndex i=model->index(r,col);
			model->setData(i,QString(),Qt::EditRole);
			model->setData(i,QString(),Qt::UserRole);
		}
	}
	RefreshModernTeamSetupUi_();
}

void MainWindowTeam::ShowModernRoster_(Ipponboard::FighterEnum side)
{
	const bool guest=side==FighterEnum::Second;
	const QComboBox* box=guest?m_pUi->comboBox_club_guest:m_pUi->comboBox_club_home;
	const QStringList names=FighterNamesForTeam_(box->currentData().toString());
	QMessageBox::information(this,QStringLiteral("Mannschaftskader"),
		names.isEmpty()?QStringLiteral("Kein Kader hinterlegt."):names.join(QStringLiteral("\n")));
}

void MainWindowTeam::UpdateModernResults_()
{
	if(!m_modernResultR1Home)return;
	const auto parse=[](const QString&s){QStringList p=s.split(':');return p.size()==2?QPair<int,int>(p[0].trimmed().toInt(),p[1].trimmed().toInt()):QPair<int,int>(0,0);};
	const auto r1=parse(m_pUi->lineEdit_wins_intermediate->text());
	const auto total=parse(m_pUi->lineEdit_wins->text());
	const int r2h=std::max(0,total.first-r1.first), r2g=std::max(0,total.second-r1.second);
	m_modernResultR1Home->setText(QString::number(r1.first));
	m_modernResultR1Guest->setText(QString::number(r1.second));
	m_modernResultR2Home->setText(QString::number(r2h));
	m_modernResultR2Guest->setText(QString::number(r2g));
	m_modernResultTotalHome->setText(QString::number(total.first));
	m_modernResultTotalGuest->setText(QString::number(total.second));
	const QString home=m_pUi->comboBox_club_home->currentText(), guest=m_pUi->comboBox_club_guest->currentText();
	m_modernResultWinner->setText(total.first==total.second?QStringLiteral("–"):(total.first>total.second?home:guest));
}

void MainWindowTeam::UpdateFightNumber_()
{
	const int currentFight = m_pController->GetCurrentFight() + 1;

	const bool isSaved = m_pController->GetFight(
							 m_pController->GetCurrentRound(),
							 m_pController->GetCurrentFight()).is_saved;

	QString formatStr("%1 / %2");

	if (isSaved)
	{
		formatStr.append(tr(" (saved)"));
	}

	m_pUi->label_fight->setText(
		formatStr
		.arg(QString::number(currentFight))
		.arg(QString::number(m_pController->GetFightCount())));

	const int currentRound = m_pController->GetCurrentRound();

	if (currentRound == 0)
	{
		m_pUi->widget_currentRound->UpdateImage(":res/images/one_blue.png");
	}
	else
	{
		m_pUi->widget_currentRound->UpdateImage(":res/images/two_green.png");
	}
}

void MainWindowTeam::attach_primary_view()
{
	auto widget = dynamic_cast<QWidget*>(m_pPrimaryView.get());

	if (widget)
	{
		m_pUi->verticalLayout_3->insertWidget(0, widget, 0);
	}
}

void MainWindowTeam::retranslate_Ui()
{
	m_pUi->retranslateUi(this);
}

void MainWindowTeam::ui_check_language_items()
{
	m_pUi->actionLang_Deutsch->setChecked("de" == m_Language);
	m_pUi->actionLang_English->setChecked("en" == m_Language);
	m_pUi->actionLang_Dutch->setChecked("nl" == m_Language);

	// don't forget second implementation!
}

void MainWindowTeam::ui_check_show_secondary_view(bool checked) const
{
	m_pUi->actionShow_SecondaryView->setChecked(checked);
}

void MainWindowTeam::UpdateButtonText_()
{
	const bool isSaved = m_pController->GetFight(
							 m_pController->GetCurrentRound(),
							 m_pController->GetCurrentFight()).is_saved;

	const bool isLastFight =
		m_pController->GetCurrentFight() ==
		m_pController->GetFightCount() - 1
		&& m_pController->GetCurrentRound() ==
		m_pController->GetRoundCount() - 1;

	const bool isFirstFight = m_pController->GetCurrentFight() == 0
							  && m_pController->GetCurrentRound() == 0;

	QString textSave = tr("Save");
	QString textNext = tr("Next");

	m_pUi->button_next->setEnabled(true);
	m_pUi->button_prev->setEnabled(!isFirstFight);

	if (isLastFight)
	{
		m_pUi->button_next->setText(textSave);

		if (isSaved)
		{
			m_pUi->button_next->setEnabled(false);
		}
	}
	else
	{
		m_pUi->button_next->setText(textNext);
	}
}

void MainWindowTeam::update_score_screen()
{
	const QString home = m_pUi->comboBox_club_home->currentText();
	const QString guest = m_pUi->comboBox_club_guest->currentText();
	m_pScoreScreen->SetClubs(home, guest);
	const QString logo_home = m_pClubManager->GetLogo(home);
	const QString logo_guest = m_pClubManager->GetLogo(guest);
	m_pScoreScreen->SetLogos(logo_home, logo_guest);
	const int score_first = m_pController->GetTeamScore(Ipponboard::FighterEnum::First);
	const int score_second = m_pController->GetTeamScore(Ipponboard::FighterEnum::Second);
	m_pScoreScreen->SetScore(score_first, score_second);

	m_pScoreScreen->update();
}

QString MainWindowTeam::GetRoundDataAsHtml(const Fight& fight, int fightNo)
{
	// little helper to hide initial zeros for early print outs
	auto getNum = [&](int val)
	{
		return (!fight.is_saved && val == 0) ? QString() : QString::number(val);
	};

	auto getTime = [&](QString const & timeStr)
	{
		return !fight.is_saved ? QString() : timeStr;
	};

	auto first = FighterEnum::First;
	auto second = FighterEnum::Second;
	auto const& score_first = fight.GetScore1();
	auto const& score_second = fight.GetScore2();

	QString roundData("<tr>");

	roundData.append("<td><center>" + QString::number(fightNo + 1) + "</center></td>"); // number
	roundData.append("<td><center>" + fight.weight + "</center></td>"); // weight
	roundData.append("<td><center>" + fight.fighters[first].name + "</center></td>"); // name
	roundData.append("<td><center>" + getNum(score_first.Ippon()) + "</center></td>"); // I
	roundData.append("<td><center>" + getNum(score_first.Wazaari()) + "</center></td>"); // W
	roundData.append("<td><center>" + getNum(score_first.Yuko()) + "</center></td>"); // Y
	roundData.append("<td><center>" + getNum(score_first.Shido()) + "</center></td>"); // S
	roundData.append("<td><center>" + getNum(score_first.Hansokumake()) + "</center></td>"); // H
	roundData.append("<td><center>" + getNum(fight.HasWon(first)) + "</center></td>"); // won
	roundData.append("<td><center>" + getNum(fight.GetScorePoints(first)) + "</center></td>"); // score
	roundData.append("<td><center>" + fight.fighters[second].name + "</center></td>"); // name
	roundData.append("<td><center>" + getNum(score_second.Ippon()) + "</center></td>"); // I
	roundData.append("<td><center>" + getNum(score_second.Wazaari()) + "</center></td>"); // W
	roundData.append("<td><center>" + getNum(score_second.Yuko()) + "</center></td>"); // Y
	roundData.append("<td><center>" + getNum(score_second.Shido()) + "</center></td>"); // S
	roundData.append("<td><center>" + getNum(score_second.Hansokumake()) + "</center></td>"); // H
	roundData.append("<td><center>" + getNum(fight.HasWon(second)) + "</center></td>"); // won
	roundData.append("<td><center>" + getNum(fight.GetScorePoints(second)) + "</center></td>"); // score
	roundData.append("<td><center>" + getTime(fight.GetTimeRemainingString()) + "</center></td>"); // time
	roundData.append("<td><center>" + getTime(fight.GetTotalTimeElapsedString()) + "</center></td>"); // time
	roundData.append("</tr>\n");

	return roundData;
}

QString MainWindowTeam::GetRoundDataAsNwjvHtml(const Fight& fight)
{
	auto getNum = [&](int val)
	{
		return (!fight.is_saved && val == 0) ? QString() : QString::number(val);
	};
	auto getTime = [&](QString const& timeStr)
	{
		return !fight.is_saved ? QString() : timeStr;
	};

	auto first = FighterEnum::First;
	auto second = FighterEnum::Second;
	auto const& score_first = fight.GetScore1();
	auto const& score_second = fight.GetScore2();

	QString row("<tr>");
	row.append("<td><center>" + fight.weight + "</center></td>");
	row.append("<td class=\"namecell\"><center>" + fight.fighters[first].name + "</center></td>");
	row.append("<td><center>" + getNum(score_first.Yuko()) + "</center></td>");
	row.append("<td><center>" + getNum(score_first.Wazaari()) + "</center></td>");
	row.append("<td><center>" + getNum(score_first.Ippon()) + "</center></td>");
	row.append("<td><center>" + getNum(score_first.Shido()) + "</center></td>");
	row.append("<td><center>" + getNum(score_first.Hansokumake()) + "</center></td>");
	row.append("<td><center>" + getNum(fight.HasWon(first)) + "</center></td>");
	row.append("<td><center>" + getNum(fight.GetScorePoints(first)) + "</center></td>");
	row.append("<td class=\"namecell\"><center>" + fight.fighters[second].name + "</center></td>");
	row.append("<td><center>" + getNum(score_second.Yuko()) + "</center></td>");
	row.append("<td><center>" + getNum(score_second.Wazaari()) + "</center></td>");
	row.append("<td><center>" + getNum(score_second.Ippon()) + "</center></td>");
	row.append("<td><center>" + getNum(score_second.Shido()) + "</center></td>");
	row.append("<td><center>" + getNum(score_second.Hansokumake()) + "</center></td>");
	row.append("<td><center>" + getNum(fight.HasWon(second)) + "</center></td>");
	row.append("<td><center>" + getNum(fight.GetScorePoints(second)) + "</center></td>");
	row.append("<td><center>" + getTime(fight.GetTotalTimeElapsedString()) + "</center></td>");
	row.append("</tr>\n");
	return row;
}

void MainWindowTeam::WriteScoreToHtml_()
{
	QString modeText = get_full_mode_title(m_currentMode);
    QString templateFile = get_template_file(m_currentMode);
	const QString filePath(fm::GetSettingsFilePath(templateFile.toStdString().c_str()));

	QFile file(filePath);

	if (!file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(this, tr("File open error"),
							  tr("File could not be opened: ") + file.fileName());
		return;
	}

	QTextStream ts(&file);

	m_htmlScore = ts.readAll();
	file.close();

	m_htmlScore.replace("%TITLE%", modeText);

	m_htmlScore.replace("%HOST%", m_pUi->comboBox_club_host->currentText());
	m_htmlScore.replace("%DATE%", m_pUi->dateEdit->text());
	m_htmlScore.replace("%LOCATION%", m_pUi->lineEdit_location->text());
	m_htmlScore.replace("%HOME%", m_pUi->comboBox_club_home->currentText());
	m_htmlScore.replace("%GUEST%", m_pUi->comboBox_club_guest->currentText());

	// intermediate score
	auto wins1st = m_pController->GetTournamentScoreModel(0)->GetTotalWins();
	m_htmlScore.replace("%WINS_HOME%", QString::number(wins1st.first));
	m_htmlScore.replace("%WINS_GUEST%", QString::number(wins1st.second));
	auto score1st = m_pController->GetTournamentScoreModel(0)->GetTotalScore();
	m_htmlScore.replace("%SCORE_HOME%", QString::number(score1st.first));
	m_htmlScore.replace("%SCORE_GUEST%", QString::number(score1st.second));

	// final score
	auto wins2nd = m_pController->GetRoundCount() > 1 ?
                       m_pController->GetTournamentScoreModel(1)->GetTotalWins() : std::make_pair<unsigned int, unsigned int>(0, 0);
	auto score2nd = m_pController->GetRoundCount() > 1 ?
                    m_pController->GetTournamentScoreModel(1)->GetTotalScore() : std::make_pair<unsigned int, unsigned int>(0, 0);
	m_htmlScore.replace("%SECOND_WINS_HOME%", QString::number(wins2nd.first));
	m_htmlScore.replace("%SECOND_WINS_GUEST%", QString::number(wins2nd.second));
	m_htmlScore.replace("%SECOND_SCORE_HOME%", QString::number(score2nd.first));
	m_htmlScore.replace("%SECOND_SCORE_GUEST%", QString::number(score2nd.second));
	auto totalWins = std::make_pair(wins1st.first + wins2nd.first, wins1st.second + wins2nd.second);
	auto totalScore = std::make_pair(score1st.first + score2nd.first, score1st.second + score2nd.second);

	m_htmlScore.replace("%TOTAL_WINS_HOME%", QString::number(totalWins.first));
	m_htmlScore.replace("%TOTAL_WINS_GUEST%", QString::number(totalWins.second));
	m_htmlScore.replace("%TOTAL_SCORE_HOME%", QString::number(totalScore.first));
	m_htmlScore.replace("%TOTAL_SCORE_GUEST%", QString::number(totalScore.second));

	QString winner = tr("tie");

	if (totalWins.first > totalWins.second)
	{
		winner = m_pUi->comboBox_club_home->currentText();
	}
	else if (totalWins.first < totalWins.second)
	{
		winner = m_pUi->comboBox_club_guest->currentText();
	}

	m_htmlScore.replace("%WINNER%", winner);

	// first round
	QString scoreData;

	for (int fightNo(0); fightNo < m_pController->GetFightCount(); ++fightNo)
	{
		const auto& fight = m_pController->GetFight(0, fightNo);
		scoreData.append(GetRoundDataAsHtml(fight, fightNo));
	}

	m_htmlScore.replace("%FIRST_ROUND%", scoreData);

	// second round
	scoreData.clear();

	for (int roundNo(1); roundNo < m_pController->GetRoundCount(); ++roundNo)
	{
		for (int fightNo(0); fightNo < m_pController->GetFightCount(); ++fightNo)
		{
			const auto& fight = m_pController->GetFight(roundNo, fightNo);
			scoreData.append(GetRoundDataAsHtml(fight, fightNo + m_pController->GetFightCount()));
		}
	}

	m_htmlScore.replace("%SECOND_ROUND%", scoreData);

	QString nwjvFirstRound;
	for (int fightNo(0); fightNo < m_pController->GetFightCount(); ++fightNo)
	{
		nwjvFirstRound.append(GetRoundDataAsNwjvHtml(m_pController->GetFight(0, fightNo)));
	}
	m_htmlScore.replace("%NWJV_FIRST_ROUND%", nwjvFirstRound);

	QString nwjvSecondRound;
	for (int roundNo(1); roundNo < m_pController->GetRoundCount(); ++roundNo)
	{
		for (int fightNo(0); fightNo < m_pController->GetFightCount(); ++fightNo)
		{
			nwjvSecondRound.append(GetRoundDataAsNwjvHtml(m_pController->GetFight(roundNo, fightNo)));
		}
	}
	m_htmlScore.replace("%NWJV_SECOND_ROUND%", nwjvSecondRound);

const QString copyright = tr("List generated with Ipponboard v") +
							  QApplication::applicationVersion() +
							  ", &copy; " + QApplication::organizationName() + ", 2010-" + VersionInfo::CopyrightYear;
    m_htmlScore.replace("</body>", "<br/><small><center>" + copyright + "</center></small></body>");
}

TournamentSerialization::TournamentSaveData MainWindowTeam::CollectTournamentSaveData_() const
{
	TournamentSerialization::TournamentSaveData saveData;
	saveData.fileVersion = QString::fromLatin1(TournamentSerialization::TournamentSaveFileVersion);
	saveData.host = m_pUi->comboBox_club_host->currentText();
	saveData.hostClubId = m_pUi->comboBox_club_host->currentData().toString();
	saveData.date = m_pUi->dateEdit->text();
	saveData.location = m_pUi->lineEdit_location->text();
	saveData.home = m_pUi->comboBox_club_home->currentText();
	saveData.homeTeamId = m_pUi->comboBox_club_home->currentData().toString();
	saveData.guest = m_pUi->comboBox_club_guest->currentText();
	saveData.guestTeamId = m_pUi->comboBox_club_guest->currentData().toString();
	saveData.currentRound = m_pController->GetCurrentRound();
	saveData.currentFight = m_pController->GetCurrentFight();
	saveData.infoTextFg = MainWindowBase::m_pPrimaryView->GetInfoTextColor().rgb();
	saveData.infoTextBg = MainWindowBase::m_pPrimaryView->GetInfoTextBgColor().rgb();
	saveData.firstFg = MainWindowBase::m_pPrimaryView->GetTextColorFirst().rgb();
	saveData.firstBg = MainWindowBase::m_pPrimaryView->GetTextBgColorFirst().rgb();
	saveData.secondFg = MainWindowBase::m_pPrimaryView->GetTextColorSecond().rgb();
	saveData.secondBg = MainWindowBase::m_pPrimaryView->GetTextBgColorSecond().rgb();

	for (const auto& mode : m_modes)
	{
		if (mode.id == m_currentMode)
		{
			saveData.mode = mode;
			break;
		}
	}

	const auto roundCount = m_pController->GetRoundCount();
	const auto fightsPerRound = m_pController->GetFightCount();
	saveData.rounds.resize(roundCount);
	for (int roundIndex = 0; roundIndex < roundCount; ++roundIndex)
	{
		auto& round = saveData.rounds[roundIndex];
		round.reserve(fightsPerRound);
		for (int fightIndex = 0; fightIndex < fightsPerRound; ++fightIndex)
		{
			round.push_back(m_pController->GetFight(roundIndex, fightIndex));
		}
	}

	return saveData;
}

QByteArray MainWindowTeam::GetTournamentAsJson_() const
{
	return TournamentSerialization::ToJson(CollectTournamentSaveData_()).toJson(QJsonDocument::Indented);
}

int MainWindowTeam::LoadTournamentFromJson_(QJsonDocument& doc, bool loadWithIncompatibleVersion)
{
	TournamentSerialization::TournamentSaveData saveData;
	const auto result = TournamentSerialization::CreateFromJson(
		doc,
		TournamentSerialization::TournamentSaveFileVersion, 
		loadWithIncompatibleVersion, 
		saveData);

	if (result != 0)
	{
		return result;
	}

	if (!m_usingMasterData)
	{
		if (m_pUi->comboBox_club_host->findText(saveData.host) == -1)
			m_pClubManager->AddClub(Club(saveData.host, "clubs\\default.png"));
		if (m_pUi->comboBox_club_home->findText(saveData.home) == -1)
			m_pClubManager->AddClub(Club(saveData.home, "clubs\\default.png"));
		if (m_pUi->comboBox_club_guest->findText(saveData.guest) == -1)
			m_pClubManager->AddClub(Club(saveData.guest, "clubs\\default.png"));
	}

	update_club_views();

	int hostIndex = !saveData.hostClubId.isEmpty() ? m_pUi->comboBox_club_host->findData(saveData.hostClubId) : -1;
	if (hostIndex < 0) hostIndex = m_pUi->comboBox_club_host->findText(saveData.host);
	if (hostIndex >= 0) m_pUi->comboBox_club_host->setCurrentIndex(hostIndex);

	m_pUi->dateEdit->setDate(QDate::fromString(saveData.date, SaveDateFormat));
	m_pUi->lineEdit_location->setText(saveData.location);

	int homeIndex = !saveData.homeTeamId.isEmpty() ? m_pUi->comboBox_club_home->findData(saveData.homeTeamId) : -1;
	if (homeIndex < 0) homeIndex = m_pUi->comboBox_club_home->findText(saveData.home);
	if (homeIndex >= 0) m_pUi->comboBox_club_home->setCurrentIndex(homeIndex);

	int guestIndex = !saveData.guestTeamId.isEmpty() ? m_pUi->comboBox_club_guest->findData(saveData.guestTeamId) : -1;
	if (guestIndex < 0) guestIndex = m_pUi->comboBox_club_guest->findText(saveData.guest);
	if (guestIndex >= 0) m_pUi->comboBox_club_guest->setCurrentIndex(guestIndex);

	UpdateTeamFighterDelegates_();

	const auto existingMode = std::find_if(
		m_modes.begin(),
		m_modes.end(),
		[&](const TournamentMode& mode) { return mode.id == saveData.mode.id; });

	const auto overridesString = saveData.mode.GetFightTimeOverridesString();
	const auto matchesExisting = existingMode != m_modes.end() &&
		existingMode->title == saveData.mode.title &&
		existingMode->subTitle == saveData.mode.subTitle &&
		existingMode->listTemplate == saveData.mode.listTemplate &&
		existingMode->weights == saveData.mode.weights &&
		existingMode->weightsAreDoubled == saveData.mode.weightsAreDoubled &&
		existingMode->nRounds == saveData.mode.nRounds &&
		existingMode->fightTimeInSeconds == saveData.mode.fightTimeInSeconds &&
		existingMode->GetFightTimeOverridesString() == overridesString &&
		existingMode->rules == saveData.mode.rules &&
		existingMode->options == saveData.mode.options;

	if (matchesExisting)
	{
		auto index = m_pUi->comboBox_mode->findData(QVariant(existingMode->id));
		index = index == -1 ? 0 : index;
		m_pUi->comboBox_mode->setCurrentIndex(index);
	}
	else
	{
		TournamentMode newMode = saveData.mode;
		m_modes.push_back(newMode);
		m_pUi->comboBox_mode->addItem(newMode.Description(), QVariant(newMode.id));
		m_pUi->comboBox_mode->setCurrentIndex(m_pUi->comboBox_mode->findData(QVariant(newMode.id)));
	}

	for (std::size_t roundIndex = 0; roundIndex < saveData.rounds.size(); ++roundIndex)
	{
		const auto& round = saveData.rounds[roundIndex];
		for (std::size_t fightIndex = 0; fightIndex < round.size(); ++fightIndex)
		{
			m_pController->SetFight(static_cast<int>(roundIndex), static_cast<int>(fightIndex), round[fightIndex]);
		}
	}

	m_pController->SetCurrentRound(saveData.currentRound);
	m_pController->SetCurrentFight(saveData.currentFight);

	MainWindowBase::update_info_text_color(QColor::fromRgba(saveData.infoTextFg), QColor::fromRgba(saveData.infoTextBg));
	MainWindowBase::update_text_color_first(QColor::fromRgba(saveData.firstFg), QColor::fromRgba(saveData.firstBg));
	MainWindowBase::update_text_color_second(QColor::fromRgba(saveData.secondFg), QColor::fromRgba(saveData.secondBg));

	return 0;
}


QString MainWindowTeam::SaveTournamentToFile_(QString const& filename)

{
	QFile file = QFile(filename);
	auto jsonDoc = GetTournamentAsJson_();
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(jsonDoc) > 0)
	{
		file.flush();
		file.close();
		qDebug() << "Saved tournament successfully to" << filename;
		return QString();
	}
	else
	{
		QString errorMsg = tr("The tournament could not be saved to %1").arg(filename);
		qWarning() << errorMsg;
		return errorMsg;
	}
}

void MainWindowTeam::load_autosave_if_available()
{
	const auto autoSavePath = fm::GetAppConfigFilePath(TournamentSerialization::AutoSaveFilename);
	QJsonDocument document;
	QString errorMessage;
	const auto status = TournamentSerialization::ReadSaveFile(
		autoSavePath,
		document,
		&errorMessage);

	if (status == TournamentSerialization::ReadSaveFileStatus::FileNotFound)
	{
		return;
	}

	if (status != TournamentSerialization::ReadSaveFileStatus::Success)
	{
		if (errorMessage.isEmpty())
		{
			qWarning() << "Autosave file could not be read:" << autoSavePath;
		}
		else
		{
			qWarning() << "Autosave file could not be read:" << errorMessage;
		}
		return;
	}

	auto loadResult = LoadTournamentFromJson_(document);
	if (loadResult == 1)
	{
		loadResult = LoadTournamentFromJson_(document, true);
	}

	if (loadResult == 0)
	{
		qInfo() << "Loaded autosave from" << autoSavePath;
		return;
	}

	qWarning() << "Autosave document could not be applied";
}

void MainWindowTeam::on_actionNew_triggered()
{
	if (QMessageBox::question(
		this,
		tr("Discard tournament?"),
		tr("This will discard any unsaved changes from your current tournament. Proceed?"),
		QMessageBox::Yes,
		QMessageBox::No) == QMessageBox::No) return;

	on_comboBox_mode_currentIndexChanged(m_pUi->comboBox_mode->currentIndex());
}

void MainWindowTeam::on_actionSave_As_triggered()
{
	const auto initialFileName = QString("%1-%2_vs_%3.json")
		.arg(QDate::currentDate().toString(Qt::ISODate))
		.arg(m_pUi->comboBox_club_home->currentText())
		.arg(m_pUi->comboBox_club_guest->currentText());
	
	QString initialPath = fm::GetAppConfigFilePath(initialFileName);
	
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save tournament as..."),
		initialPath,
		tr("JSON File (*.json)"));

	if (fileName == "") return;

	const QString& errorMsg = SaveTournamentToFile_(fileName);

	if (errorMsg.isEmpty())
	{
		QMessageBox::information(this, tr("Saved!"), tr("The match was saved successfully!"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error!"), errorMsg);
	}
}

void MainWindowTeam::on_actionLoad_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Load tournament from..."),
		fm::GetAppConfigDir(),
		tr("JSON File (*.json)"));

	if (fileName == "") return;

	if (QMessageBox::question(
		this,
		tr("Discard tournament?"),
        tr("Loading a tournament file will discard any unsaved changes from your current tournament. Proceed?"),
		QMessageBox::Yes,
		QMessageBox::No) == QMessageBox::No) return;

	QFile file = QFile(fileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, tr("Error!"), tr("File could not be opened!"));
		return;
	}
	
	QString val = file.readAll();
	file.close();

	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8(), &err);
	if (err.error)
	{
		QMessageBox::warning(this, tr("Error while parsing JSON!"), err.errorString());
		return;
	}
	int result = LoadTournamentFromJson_(doc);

	if (result == 1)
	{
		if (QMessageBox::question(
			this,
			tr("File Version mismatch"),
			tr("This file was saved with a newer version of Ipponboard and may not be compatible with this version. Do you want to try to load this file anyway? This could lead to a corrupted Tournament State!"),
			QMessageBox::Yes,
			QMessageBox::No) == QMessageBox::Yes)
		{
			result = LoadTournamentFromJson_(doc, true);
		}
		else
		{
			return;
		}
	}

	if (result == 0)
	{
		QMessageBox::information(this, tr("Success!"), tr("The match was loaded successfully!"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error!"), tr("Error while parsing data!"));
		on_comboBox_mode_currentIndexChanged(m_pUi->comboBox_mode->currentIndex());
	}
}

void MainWindowTeam::on_actionReset_Scores_triggered()
{
	if (QMessageBox::warning(
				this,
				tr("Reset Scores"),
				tr("Really reset complete score table?"),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		m_pController->ClearFightsAndResetTimers();
	}

	UpdateFightNumber_();
	UpdateButtonText_();
}

bool MainWindowTeam::EvaluateSpecificInput(const Gamepad* pGamepad)
{
#ifdef _WIN32
    // back
	if (pGamepad->WasPressed(Gamepad::EButton(m_controllerCfg.button_prev)))
	{
		on_button_prev_clicked();
		// TODO: check: is UpdateViews_(); necessary here?
		// --> handle update views outside of this function
		return true;
	}
	// next
	else if (pGamepad->WasPressed(Gamepad::EButton(m_controllerCfg.button_next)))
	{
		on_button_next_clicked();
		// TODO: check: is UpdateViews_(); necessary here?
		// --> handle update views outside of this function
		return true;
	}
#endif

	return false;
}

void MainWindowTeam::on_tabWidget_currentChanged(int /*index*/)
{
	update_views();
}

void MainWindowTeam::on_actionManageModes_triggered()
{
	QStringList templates = get_list_templates();

	ModeManagerDlg dlg(m_modes, templates, m_currentMode, this);

	if (dlg.exec() == QDialog::Accepted)
	{
		QString errMsg;

		if (!UploadTournamentModes_(dlg.Result(), errMsg))
		{
			QMessageBox::critical(this,
				QStringLiteral("Globale Wettkampfmodi"),
				errMsg + QStringLiteral("\n\nDie Änderung wurde nicht als globale Systemeinstellung übernommen."));
			return;
		}

		m_masterTournamentModes = TournamentModesToJson(dlg.Result());
		SaveTournamentModesToCache_(dlg.Result());
		LoadModes(dlg.Result(), m_currentMode); // global server state accepted
	}
}

void MainWindowTeam::on_actionManage_Clubs_triggered()
{
	ClubManagerDlg dlg(m_pClubManager, this);
	dlg.exec();
}

void MainWindowTeam::on_actionLoad_Demo_Data_triggered()
{
	//const QString modeBayernliga("bayernliga_m");

	//auto iter = std::find_if(begin(m_modes), end(m_modes),
	//	[&](TournamentMode const& mode)
	//{
	//	return mode.name == modeBayernliga;
	//});

	//if (iter == end(m_modes) || m_pUi->comboBox_mode->findText(iter->Description()) < 0)
	//{
	//	QMessageBox::critical(this, tr("Load demo data error"),
	//		tr("Tournament mode settings for [%1] could not be found.").arg(modeBayernliga));

	//	return;
	//}

	//int modeIndex = m_pUi->comboBox_mode->findText(iter->Description());
	//m_pUi->comboBox_mode->setCurrentIndex(modeIndex);

	//m_pController->ClearFights();																				//  Y  W  I  S  H  Y  W  I  S  H
	//m_pController->InitTournament(*iter);
	//update_weights("-90;+90;-73;-66;-81");
	//m_pController->SetFight(0, 0, "-90", "Sven Hölzl", "TG Eierstatt", "Oliver Salz", "TSV Brunnstadt",			3, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(0, 1, "-90", "Max Grünert", "TG Eierstatt", "Marc Schälzig", "TSV Brunnstadt",			3, 2, 0, 0, 0, 0, 0, 0, 1, 0);
	//m_pController->SetFight(0, 2, "+90", "Lukas Neumaier", "TG Eierstatt", "Daniel Nusenstein", "TSV Brunnstadt",	0, 0, 0, 1, 0, 0, 0, 1, 1, 0);
	//m_pController->SetFight(0, 3, "+90", "Hans Neumeier", "TG Eierstatt", "Anderas Mader", "TSV Brunnstadt",			1, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(0, 4, "-73", "Bogdan Mahl", "TG Eierstatt", "Christopher Benka", "TSV Brunnstadt"	,		2, 0, 1, 1, 0, 0, 0, 0, 3, 0);
	//m_pController->SetFight(0, 5, "-73", "Peter Sellmaier", "TG Eierstatt", "Jan-Michael Köbinger", "TSV Brunnstadt",		0, 1, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(0, 6, "-66", "Thomas Keil", "TG Eierstatt", "Arthur Sichelstein", "TSV Brunnstadt",			2, 1, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(0, 7, "-66", "Werner Bogner", "TG Eierstatt", "Thomas Schamberger", "TSV Brunnstadt",		0, 0, 1, 0, 0, 2, 0, 0, 0, 0);
	//m_pController->SetFight(0, 8, "-81", "Hans Schmieder", "TG Eierstatt", "Gerhard Westerner", "TSV Brunnstadt",	0, 1, 1, 1, 0, 1, 0, 0, 0, 0);
	//m_pController->SetFight(0, 9, "-81", "Axel Neumaier", "TG Eierstatt", "Georg Beier", "TSV Brunnstadt",			1, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	////  Y  W  I  S  H  Y  W  I  S  H
	//m_pController->SetFight(1, 0, "-90", "Sven Hölzl", "TG Eierstatt", "Marc Schälzig", "TSV Brunnstadt",		0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(1, 1, "-90", "Max Grunert", "TG Eierstatt", "Florian Kütz", "TSV Brunnstadt",		0, 1, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(1, 2, "+90", "Lukas Neumaier", "TG Eierstatt", "Andreas Mader", "TSV Brunnstadt",	1, 2, 0, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(1, 3, "+90", "Hans Neumaier", "TG Eierstatt", "Daniel Nusenstein", "TSV Brunnstadt",	0, 0, 0, 2, 0, 0, 0, 1, 2, 0);
	//m_pController->SetFight(1, 4, "-73", "Christian Feigl", "TG Eierstatt", "Jan-Michael Köbinger", "TSV Brunnstadt",	2, 1, 0, 1, 0, 0, 0, 0, 1, 0);
	//m_pController->SetFight(1, 5, "-73", "Peter Sellmaier", "TG Eierstatt", "Christopher Beier", "TSV Brunnstadt",	0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	//m_pController->SetFight(1, 6, "-66", "Adam Herzog", "TG Eierstatt", "Thomas Schamberger", "TSV Brunnstadt",		0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
	//m_pController->SetFight(1, 7, "-66", "Maxim Selwitschka", "TG Eierstatt", "Jonas Alwetter", "TSV Brunnstadt",	0, 1, 1, 0, 0, 1, 0, 0, 0, 0);
	//m_pController->SetFight(1, 8, "-81", "Piotr Makaritsch", "TG Eierstatt", "Georg Beier", "TSV Brunnstadt",		0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
	//m_pController->SetFight(1, 9, "-81", "Axel Neumaier", "TG Eierstatt", "Gerhard Westerner", "TSV Brunnstadt",	0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
	////m_pController->SetCurrentFight(0);

	//m_pUi->tableView_tournament_list1->viewport()->update();
	//m_pUi->tableView_tournament_list2->viewport()->update();
}

void MainWindowTeam::on_button_pause_clicked()
{
	if (m_pScoreScreen->isVisible())
	{
		m_pScoreScreen->hide();
		m_pUi->button_pause->setText(tr("Show results"));
	}
	else
	{
		update_score_screen();
		update_screen_visibility(m_pScoreScreen.get());

		m_pUi->button_pause->setText(tr("Hide results"));
	}
}

void MainWindowTeam::on_button_prev_clicked()
{
	//if (0 == m_pController->GetCurrentFightIndex())
	//	return;

	m_pController->PrevFight();
	//m_pController->SetCurrentFight(m_pController->GetCurrentFightIndex() - 1);
	
	SaveTournamentToFile_(fm::GetAppConfigFilePath(TournamentSerialization::AutoSaveFilename)); // autosave
}

void MainWindowTeam::on_button_next_clicked()
{
	/*
	if (m_pController->GetCurrentFightIndex() == m_pController->GetFightCount() - 1)
	{
		m_pController->SetCurrentFight(m_pController->GetCurrentFightIndex());
	}
	else
	{
		m_pController->SetCurrentFight(m_pController->GetCurrentFightIndex() + 1);
	}
	*/
	m_pController->NextFight();

	// reset osaekomi view (to reset active colors of previous fight)
    m_pController->DoAction(eAction_ResetOsaeKomi, FighterEnum::Nobody, true /*doRevoke*/);

	SaveTournamentToFile_(fm::GetAppConfigFilePath(TournamentSerialization::AutoSaveFilename)); // autosave
}

void MainWindowTeam::on_comboBox_mode_currentIndexChanged(int i)
{
	if (!initialized)
	{
		return;
	}

	m_currentMode = m_pUi->comboBox_mode->itemData(i).toString();
	QString modeDescription = m_pUi->comboBox_mode->currentText();

	// FIXME2014: use this???
	//m_pController->SetOption(eOption_Use2013Rules, true);

	auto iter = std::find_if(begin(m_modes), end(m_modes), [&](TournamentMode const & mode)
	{
		return mode.id == m_currentMode;
	});

	if (iter != end(m_modes))
	{
		m_pController->InitTournament(*iter);
		update_weights(iter->weights); // TODO: don't set weights twice

		// disable "copy & switch" button if no duplicate weight classes are used (issue #42)
		if (iter->weightsAreDoubled)
		{
			m_pUi->pushButton_copySwitched->show();
		}
		else
		{
			m_pUi->pushButton_copySwitched->hide();
		}
	}
	else
	{
		Q_ASSERT("invalid mode");
	}

	// update table views
	m_pUi->tableView_tournament_list1->setModel(m_pController->GetTournamentScoreModel(0).get());
	m_pUi->tableView_tournament_list1->resizeColumnsToContents();

	m_pController->GetTournamentScoreModel(0)->SetExternalDisplays(
		m_pUi->lineEdit_wins_intermediate,
		m_pUi->lineEdit_score_intermediate);

	m_pUi->tableView_tournament_list1->selectRow(0);

	if (m_pController->GetRoundCount() == 1)
	{
		m_pUi->label_intermediate_result->setText(m_pUi->label_final_score->text()); //TODO: make this better!
		m_pUi->tableView_tournament_list2->hide();
		m_pUi->pushButton_copySwitched->hide();
		m_pUi->label_final_score->hide();
		m_pUi->label_final_wins->hide();
		m_pUi->label_final_sub_score->hide();
		m_pUi->lineEdit_score->hide();
		m_pUi->lineEdit_wins->hide();
	}
	else
	{
		m_pUi->tableView_tournament_list2->setModel(m_pController->GetTournamentScoreModel(1).get());
		m_pUi->tableView_tournament_list2->resizeColumnsToContents();
		m_pController->GetTournamentScoreModel(1)->SetExternalDisplays(
			m_pUi->lineEdit_wins,
			m_pUi->lineEdit_score);

		m_pController->GetTournamentScoreModel(1)->SetIntermediateModel(
			m_pController->GetTournamentScoreModel(0).get());

		m_pUi->tableView_tournament_list2->selectRow(0);

		m_pUi->tableView_tournament_list2->horizontalHeader()->setSectionResizeMode(TournamentModel::eCol_name1, QHeaderView::Stretch);
		m_pUi->tableView_tournament_list2->horizontalHeader()->setSectionResizeMode(TournamentModel::eCol_name2, QHeaderView::Stretch);

		m_pUi->tableView_tournament_list2->show();
		m_pUi->label_final_score->show();
		m_pUi->label_final_wins->show();
		m_pUi->label_final_sub_score->show();
		m_pUi->lineEdit_score->show();
		m_pUi->lineEdit_wins->show();
	}

	// set mode text as mat label
	m_MatLabel = modeDescription;
	m_pPrimaryView->SetMat(modeDescription);
	m_pSecondaryView->SetMat(modeDescription);

	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();

	UpdateFightNumber_();
	RefreshModernTeamSetupUi_();
}

void MainWindowTeam::on_comboBox_club_host_currentIndexChanged(const QString& s)
{
	m_host = s;

	// set location from host
	m_pUi->lineEdit_location->setText(m_pClubManager->GetAddress(m_host));
}

void MainWindowTeam::on_comboBox_club_home_currentIndexChanged(const QString& s)
{
	m_pController->SetClub(Ipponboard::FighterEnum::First, s);
	UpdateTeamFighterDelegates_();

#if 0
	ComboBoxDelegate* pCbx = dynamic_cast<ComboBoxDelegate*>
							 (m_pUi->tableView_tournament_list1->itemDelegateForColumn(TournamentModel::eCol_name1));

	if (pCbx)
	{
		pCbx->SetItems(m_fighterManager.GetClubFighterNames(s));
	}

#endif
	//UpdateViews_(); --> already done by controller
	update_score_screen();
	RefreshModernTeamSetupUi_();
}

void MainWindowTeam::on_comboBox_club_guest_currentIndexChanged(const QString& s)
{
	m_pController->SetClub(Ipponboard::FighterEnum::Second, s);
	UpdateTeamFighterDelegates_();
#if 0
	ComboBoxDelegate* pCbx = dynamic_cast<ComboBoxDelegate*>
							 (m_pUi->tableView_tournament_list1->itemDelegateForColumn(TournamentModel::eCol_name2));

	if (pCbx)
	{
		pCbx->SetItems(m_fighterManager.GetClubFighterNames(s));
	}

#endif
	//UpdateViews_(); --> already done by controller
	update_score_screen();
	RefreshModernTeamSetupUi_();
}

void MainWindowTeam::on_actionPrint_triggered()
{
	WriteScoreToHtml_();

	QPrinter printer(QPrinter::HighResolution);
    //TODO: fix margins (actual header margin is too big)
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);
    //printer.setFullPage(true);
	QPrintPreviewDialog preview(&printer, this);
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(Print(QPrinter*)));
	preview.exec();
}

void MainWindowTeam::on_actionExport_triggered()
{
	WriteScoreToHtml_();

	// save file to...
	QString selectedFilter;
	QString dateStr(m_pUi->dateEdit->text());
	dateStr.replace('.', '-');
	QString fileName = QFileDialog::getSaveFileName(this,
					   tr("Export file to..."),
					   tr("ScoreList_") + dateStr,
					   tr("PDF File (*.pdf);;HTML File (*.html)"),
					   &selectedFilter);

	if (!fileName.isEmpty())
	{
		// set wait cursor
		QApplication::setOverrideCursor(Qt::WaitCursor);

		if (fileName.endsWith(".html"))
		{
			QFile html(fileName);

			if (html.open(QFile::WriteOnly))
			{
				QTextStream ts(&html);
				ts << m_htmlScore;
				ts.flush();
				html.close();
			}
		}
		else
		{
            QPrinter printer(QPrinter::HighResolution);
            //TODO: fix margins? (printable area is somehow smaller than with Qt4...)
            //TODO: use QPdfWriter?
            printer.setFullPage(true);
            printer.setPageOrientation(QPageLayout::Landscape);
            printer.setOutputFormat(QPrinter::PdfFormat);
            printer.setPageSize(QPageSize(QPageSize::A4));
            printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);
			printer.setOutputFileName(fileName);
			QTextEdit edit(m_htmlScore, this);
			edit.document()->print(&printer);
		}

		QApplication::restoreOverrideCursor();
	}
}

void MainWindowTeam::on_button_golden_score_toggled(bool toggled)
{
	m_pController->SetGoldenScore(toggled);
}

void MainWindowTeam::on_toolButton_weights_pressed()
{
	bool ok(false);
	const QString weights = QInputDialog::getText(
								this,
								tr("Set Weights"),
								tr("Set weights (separated by ';'):"),
								QLineEdit::Normal,
								m_weights,
								&ok);

	if (ok)
	{
		if (m_pController->GetFightCount() / 2 - 1 != weights.count(';')
				&& m_pController->GetFightCount() - 1 != weights.count(';'))
		{
			QMessageBox::critical(this, "Wrong values",
								  tr("You need to specify %1 weight classes separated by ';'!")
								  .arg(QString::number(m_pController->GetFightCount())));
			on_toolButton_weights_pressed();
		}
		else
		{
			update_weights(weights);
		}
	}
}

void MainWindowTeam::on_toolButton_team_home_pressed()
{
#if 0
	MainWindowBase::on_actionManageFighters_triggered();
	const QString club = m_pUi->comboBox_club_home->currentText();

	FighterManagerDlg dlg(m_fighterManager, this);
	dlg.SetFilter(FighterManagerDlg::eColumn_club, club);
	dlg.exec();

	ComboBoxDelegate* pCbx = dynamic_cast<ComboBoxDelegate*>(
								 m_pUi->tableView_tournament_list1->
								 itemDelegateForColumn(TournamentModel::eCol_name1));

	if (pCbx)
	{
		pCbx->SetItems(m_fighterManager.GetClubFighterNames(club));
	}

#endif
}

void MainWindowTeam::on_toolButton_team_guest_pressed()
{
#if 0
	MainWindowBase::on_actionManageFighters_triggered();
	const QString club = m_pUi->comboBox_club_guest->currentText();

	FighterManagerDlg dlg(m_fighterManager, this);
	dlg.SetFilter(FighterManagerDlg::eColumn_club, club);
	dlg.exec();

	auto pCbx = dynamic_cast<ComboBoxDelegate*>(
					m_pUi->tableView_tournament_list2->
					itemDelegateForColumn(TournamentModel::eCol_name2));

	if (pCbx)
	{
		pCbx->SetItems(m_fighterManager.GetClubFighterNames(club));
	}

#endif
}

void MainWindowTeam::update_weights(QString const& weightString)
{
	m_weights = weightString;
	m_pController->SetWeights(weightString.split(';'));
}

void MainWindowTeam::on_pushButton_copySwitched_pressed()
{
	m_pController->CopyAndSwitchGuestFighters();
}

void MainWindowTeam::on_actionSet_Round_Time_triggered()
{
	bool ok(false);

	auto timeStr = QInputDialog::getText(
					   this,
					   tr("Set Value"),
					   tr("Set value to (m:ss):"),
					   QLineEdit::Normal,
					   m_pController->GetFightTimeString(),
					   &ok);

	if (ok)
	{
		m_pController->SetRoundTime(timeStr);
	}
}

void MainWindowTeam::on_actionScore_Screen_triggered()
{
	m_pUi->tabWidget->setCurrentWidget(m_pUi->tab_score_table);
}

void MainWindowTeam::on_actionScore_Control_triggered()
{
	m_pUi->tabWidget->setCurrentWidget(m_pUi->tab_view);
}

void MainWindowTeam::on_tableView_customContextMenuRequested(
	QTableView* pTableView,
	QPoint const& pos,
	const char* copySlot,
	const char* pasteSlot,
	const char* clearSlot)
{
	QMenu menu;
	QModelIndex index = pTableView->indexAt(pos);
	index = index.sibling(index.row(), 0);

	QModelIndexList selection =
		pTableView->selectionModel()->selectedIndexes();

	if (selection.empty())
	{
		Q_ASSERT(!"empty selection");
		return;
	}

	// Do not allow copy if different columns are selected
	bool copyAllowed(true);

	if (selection.size() > 1)
	{
		for (int i(0); i < selection.size() - 1; ++i)
		{
			if (selection[i].column() != selection[i + 1].column())
			{
				copyAllowed = false;
				break;
			}
		}
	}

	// Paste is only allowed for the name cells
	// and if the clipboard is not empty
	const bool pasteAllowed = (selection[0].column() == TournamentModel::eCol_name1
							   || selection[0].column() == TournamentModel::eCol_name2)
							  && !QApplication::clipboard()->text().isEmpty();

	const bool clearAllowed = copyAllowed;

	if (index.isValid())
	{
		QIcon copyIcon(":/res/icons/copy_cells.png");
		QIcon pasteIcon(":/res/icons/paste.png");
		QIcon clearIcon(":/res/icons/clear_cells.png");
		QAction* pAction = nullptr;
		pAction = menu.addAction(copyIcon, tr("Copy"), this, copySlot, QKeySequence::Copy);
		pAction->setDisabled(!copyAllowed);

		pAction = menu.addAction(pasteIcon, tr("Paste"), this, pasteSlot, QKeySequence::Paste);
		pAction->setDisabled(!pasteAllowed);

		pAction = menu.addAction(clearIcon, tr("Clear"), this, clearSlot, QKeySequence::Delete);
		pAction->setDisabled(!clearAllowed);

		menu.exec(QCursor::pos());
	}
}

void MainWindowTeam::on_tableView_tournament_list1_customContextMenuRequested(QPoint const& pos)
{
	on_tableView_customContextMenuRequested(
		m_pUi->tableView_tournament_list1,
		pos,
		SLOT(slot_copy_cell_content_list1()),
		SLOT(slot_paste_cell_content_list1()),
		SLOT(slot_clear_cell_content_list1()));
}

void MainWindowTeam::on_tableView_tournament_list2_customContextMenuRequested(QPoint const& pos)
{
	on_tableView_customContextMenuRequested(
		m_pUi->tableView_tournament_list2,
		pos,
		SLOT(slot_copy_cell_content_list2()),
		SLOT(slot_paste_cell_content_list2()),
		SLOT(slot_clear_cell_content_list2()));
}

void MainWindowTeam::copy_cell_content(QTableView* pTableView)
{
	QModelIndexList selection = pTableView->selectionModel()->selectedIndexes();
	std::sort(selection.begin(), selection.end());

	// Copy is only allowed for single column selection
	for (int i(0); i < selection.size() - 1; ++i)
	{
		if (selection[i].column() != selection[i + 1].column())
		{
			QApplication::clipboard()->clear();
			return;
		}
	}

	QString selectedText;

	for (const QModelIndex& index : selection)
	{
        auto text = pTableView->model()->data(index, Qt::DisplayRole);
        selectedText += text.toString() + '\n';
	}

	if (!selectedText.isEmpty())
	{
		selectedText.truncate(selectedText.lastIndexOf('\n'));  // remove last '\n'
		QApplication::clipboard()->setText(selectedText);
	}
}

void MainWindowTeam::paste_cell_content(QTableView* pTableView)
{
	if (QApplication::clipboard()->text().isEmpty())
	{
		QMessageBox::warning(this, QApplication::applicationName(),
							 tr("There is nothing to paste!"));
		return;
	}

	auto lines = QApplication::clipboard()->text().split('\n');

	QModelIndexList selection = pTableView->selectionModel()->selectedIndexes();

	if (selection.empty())
	{
		QMessageBox::critical(this, QApplication::applicationName(),
							  tr("Can not paste into an empty selection!"));
		return;
	}

	std::sort(selection.begin(), selection.end());

	if (lines.size() < selection.size())
	{
		QMessageBox::critical(this, QApplication::applicationName(),
							  tr("There is too few data for the selection in the clipboard!"));
		return;
	}

	if (lines.size() > selection.size())
	{
		// extend selection to maximum possible
		QModelIndex index = selection.back();
		const int nRows = pTableView->model()->rowCount();

		while (index.row() < nRows &&
				index.isValid() &&
				lines.size() > selection.size())
		{
			index = pTableView->model()->index(
						index.row() + 1, index.column());
			selection.push_back(index);
			pTableView->selectionModel()->select(index, QItemSelectionModel::Select);
		}

		if (lines.size() < selection.size())
		{
			QMessageBox::warning(this, QApplication::applicationName(),
								 tr("There is more data available in the clipboard as could be pasted!"));
		}
	}

	auto lineNo = 0;

	for (QModelIndex index : selection)
	{
		if (index.column() == TournamentModel::eCol_name1 ||
				index.column() == TournamentModel::eCol_name2)
		{
			pTableView->model()->setData(
				index, lines[lineNo], Qt::EditRole);
			++lineNo;
		}
	}
}

void MainWindowTeam::clear_cell_content(QTableView* pTableView)
{
	QModelIndexList selection = pTableView->selectionModel()->selectedIndexes();
	std::sort(selection.begin(), selection.end());

	// Clear is only allowed for single column selection
	for (int i(0); i < selection.size() - 1; ++i)
	{
		if (selection[i].column() != selection[i + 1].column())
		{
			QApplication::clipboard()->clear();
			return;
		}
	}

	for (const QModelIndex& index : selection)
	{
		pTableView->model()->setData(index, "", Qt::EditRole);
	}
}

void MainWindowTeam::slot_copy_cell_content_list1()
{
	copy_cell_content(m_pUi->tableView_tournament_list1);
}

void MainWindowTeam::slot_copy_cell_content_list2()
{
	copy_cell_content(m_pUi->tableView_tournament_list2);
}

void MainWindowTeam::slot_paste_cell_content_list1()
{
	paste_cell_content(m_pUi->tableView_tournament_list1);
}

void MainWindowTeam::slot_paste_cell_content_list2()
{
	paste_cell_content(m_pUi->tableView_tournament_list2);
}

void MainWindowTeam::slot_clear_cell_content_list1()
{
	clear_cell_content(m_pUi->tableView_tournament_list1);
}

void MainWindowTeam::slot_clear_cell_content_list2()
{
	clear_cell_content(m_pUi->tableView_tournament_list2);
}

void MainWindowTeam::Print(QPrinter* p)
{
	QTextEdit e(m_htmlScore, this);
	e.document()->print(p);
}

QString MainWindowTeam::get_template_file(QString const& modeId) const
{
	// TODO: use binary seach as the container is already sorted
	auto iter = std::find_if(begin(m_modes), end(m_modes), [&](TournamentMode const & m)
	{
		return m.id == modeId;
	});

	if (iter != end(m_modes))
	{
		return QString("%1/%2").arg(TournamentMode::str_TemplateDirName, iter->listTemplate);
	}

	return QString();
}

QString MainWindowTeam::get_full_mode_title(QString const& modeId) const
{
	QString year(QString::number(QDate::currentDate().year()));

	// TODO: use binary seach as the container is already sorted
	auto iter = std::find_if(begin(m_modes), end(m_modes), [&](TournamentMode const & tm)
	{
		return tm.id == modeId;
	});

	if (iter != end(m_modes))
	{
		if (iter->subTitle.isEmpty())
		{
			return QString("%1 %2").arg(iter->title, year);
		}
		else
		{
			return QString("%1 %2 - %3").arg(iter->title, year, iter->subTitle);
		}
	}

	return tr("Ipponboard fight list %1").arg(year);
}
