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
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPrintPreviewDialog>
#include <QRegularExpression>
#include <QImage>
#include <QPainter>
#include <QPrinter>
#include <QSettings>
#include <QSaveFile>
#include <QSplashScreen>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QUuid>
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
	constexpr auto RecoveryQueueFileName = "competition-sync-queue.json";
	constexpr auto LastSessionFileName = "last-competition-session.json";

	QString runtime_data_dir()
	{
		static const QString result = []()
		{
			const QString portable = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime"));
			if (QDir().mkpath(portable))
			{
				const QString testPath = QDir(portable).filePath(QStringLiteral(".write-test"));
				QSaveFile test(testPath);
				if (test.open(QIODevice::WriteOnly))
				{
					test.write("ok");
					if (test.commit())
					{
						QFile::remove(testPath);
						return portable;
					}
				}
			}

			const QString fallback = QDir(fm::GetAppDataDir()).filePath(QStringLiteral("runtime"));
			QDir().mkpath(fallback);
			return fallback;
		}();
		return result;
	}

	QString safe_runtime_id(QString value)
	{
		value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
		return value.isEmpty() ? QStringLiteral("_") : value;
	}

	bool write_json_atomic(const QString& filePath, const QJsonDocument& document)
	{
		QDir().mkpath(QFileInfo(filePath).absolutePath());
		QSaveFile file(filePath);
		if (!file.open(QIODevice::WriteOnly))
			return false;
		if (file.write(document.toJson(QJsonDocument::Indented)) <= 0)
			return false;
		return file.commit();
	}

	QJsonDocument read_json_document(const QString& filePath)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
			return QJsonDocument();
		QJsonParseError error;
		const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
		return error.error == QJsonParseError::NoError ? doc : QJsonDocument();
	}

	QString recovery_api_path(const QString& competitionDayId, const QString& matId)
	{
		return QStringLiteral("/api/competition-recovery/%1/%2")
			.arg(QString::fromLatin1(QUrl::toPercentEncoding(competitionDayId)),
				 QString::fromLatin1(QUrl::toPercentEncoding(matId)));
	}

#ifdef _WIN32
	bool http_json_request(const wchar_t* method, const QString& requestPath, const QByteArray& payload,
		QByteArray& response, DWORD& status)
	{
		response.clear();
		status = 0;
		HINTERNET session = WinHttpOpen(L"Ipponboard-Meschede", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!session)
			return false;
		WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);

		HINTERNET connect = WinHttpConnect(session, L"test-liga.paul-meschede.de", INTERNET_DEFAULT_HTTPS_PORT, 0);
		const std::wstring path = requestPath.toStdWString();
		HINTERNET request = connect ? WinHttpOpenRequest(connect, method, path.c_str(), nullptr,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;

		const wchar_t* headers = payload.isEmpty() ? WINHTTP_NO_ADDITIONAL_HEADERS : L"Content-Type: application/json\r\n";
		const DWORD headerLength = payload.isEmpty() ? 0 : static_cast<DWORD>(-1L);
		bool ok = request && WinHttpSendRequest(request, headers, headerLength,
			payload.isEmpty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(payload.constData()),
			static_cast<DWORD>(payload.size()), static_cast<DWORD>(payload.size()), 0)
			&& WinHttpReceiveResponse(request, nullptr);

		DWORD statusSize = sizeof(status);
		if (ok)
			ok = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

		if (ok)
		{
			for (;;)
			{
				DWORD available = 0;
				if (!WinHttpQueryDataAvailable(request, &available))
				{
					ok = false;
					break;
				}
				if (available == 0)
					break;
				QByteArray chunk(static_cast<int>(available), Qt::Uninitialized);
				DWORD read = 0;
				if (!WinHttpReadData(request, chunk.data(), available, &read))
				{
					ok = false;
					break;
				}
				chunk.resize(static_cast<int>(read));
				response.append(chunk);
			}
		}

		if (request) WinHttpCloseHandle(request);
		if (connect) WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return ok;
	}
#endif
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
	, m_masterClubs()
	, m_masterTeams()
	, m_masterFighters()
	, m_masterCompetitionDays()
	, m_masterTournamentModes()
	, m_masterRuleSets()
	, m_currentCompetitionDayId()
	, m_currentMatId()
	, m_competitionDayTeamIds()
	, m_restoringCompetitionState(false)
	, m_usingMasterData(false)
	, m_modes()
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
	// Each name cell resolves its roster live from the currently selected team.
	// The column is the single source of truth: name1 = Home, name2 = Guest.
	m_fighterDelegateHomeRound1 = new ComboBoxDelegate(this);
	m_fighterDelegateHomeRound2 = new ComboBoxDelegate(this);

	const auto liveRosterProvider = [this](const QModelIndex& index)
	{
		const bool isGuest = index.column() == TournamentModel::eCol_name2;
		const QComboBox* teamBox = isGuest ? m_pUi->comboBox_club_guest : m_pUi->comboBox_club_home;
		const QString teamId = teamBox->currentData().toString();
		const QStringList allNames = FighterNamesForTeam_(teamId);
		const QStringList allIds = FighterIdsForTeam_(teamId);

		// A fighter may be used only once per team and round. The other round is independent.
		// Keep the fighter of the currently edited row available so an existing selection can be retained.
		QStringList usedIds;
		if (index.model())
		{
			for (int row = 0; row < index.model()->rowCount(); ++row)
			{
				if (row == index.row())
					continue;
				const QModelIndex other = index.model()->index(row, index.column());
				const QString usedId = index.model()->data(other, Qt::UserRole).toString();
				if (!usedId.isEmpty() && !usedIds.contains(usedId))
					usedIds.append(usedId);
			}
		}

		QStringList names;
		QStringList ids;
		for (int i = 0; i < allIds.size() && i < allNames.size(); ++i)
		{
			if (usedIds.contains(allIds.at(i)))
				continue;
			names.append(allNames.at(i));
			ids.append(allIds.at(i));
		}

		// Special valid lineup choice: no athlete entered for this team/weight class.
		// This entry is intentionally reusable.
		names.prepend(QStringLiteral("_n.A."));
		ids.prepend(QString());
		return std::make_pair(names, ids);
	};

	m_fighterDelegateHomeRound1->SetItemProvider(liveRosterProvider);
	m_fighterDelegateHomeRound2->SetItemProvider(liveRosterProvider);

	m_pUi->tableView_tournament_list1->setItemDelegateForColumn(TournamentModel::eCol_name1, m_fighterDelegateHomeRound1);
	m_pUi->tableView_tournament_list1->setItemDelegateForColumn(TournamentModel::eCol_name2, m_fighterDelegateHomeRound1);
	m_pUi->tableView_tournament_list2->setItemDelegateForColumn(TournamentModel::eCol_name1, m_fighterDelegateHomeRound2);
	m_pUi->tableView_tournament_list2->setItemDelegateForColumn(TournamentModel::eCol_name2, m_fighterDelegateHomeRound2);

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

	if (!RestoreLastCompetitionSession_())
		load_autosave_if_available();

	QTimer::singleShot(1000, this, [this]() { FlushRecoveryQueue_(); });
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
	m_masterCompetitionDays = master.value(QStringLiteral("competitionDays")).toArray();
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
			const QString id = team.value(QStringLiteral("id")).toString();
			if (!m_competitionDayTeamIds.isEmpty() && !m_competitionDayTeamIds.contains(id))
				continue;
			const QString name = team.value(QStringLiteral("name")).toString();
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

QString MainWindowTeam::TerminalId_() const
{
	const QString path = QDir(runtime_data_dir()).filePath(QStringLiteral("terminal-id.txt"));
	QFile file(path);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		const QString existing = QString::fromUtf8(file.readAll()).trimmed();
		if (!existing.isEmpty())
			return existing;
	}

	const QString created = QUuid::createUuid().toString(QUuid::WithoutBraces);
	QSaveFile out(path);
	if (out.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		out.write(created.toUtf8());
		out.commit();
	}
	return created;
}

QString MainWindowTeam::CompetitionStateFilePath_(const QString& competitionDayId, const QString& matId) const
{
	const QString dir = QDir(runtime_data_dir()).filePath(QStringLiteral("competition-states"));
	QDir().mkpath(dir);
	return QDir(dir).filePath(safe_runtime_id(competitionDayId) + QStringLiteral("__") + safe_runtime_id(matId) + QStringLiteral(".json"));
}

QJsonDocument MainWindowTeam::BuildRecoverySnapshot_() const
{
	QJsonParseError error;
	const QJsonDocument raw = QJsonDocument::fromJson(GetTournamentAsJson_(), &error);
	if (error.error != QJsonParseError::NoError || !raw.isObject())
		return QJsonDocument();

	QJsonObject root = raw.object();
	QJsonArray rounds = root.value(QStringLiteral("Rounds")).toArray();
	for (int roundIndex = 0; roundIndex < rounds.size(); ++roundIndex)
	{
		QJsonArray round = rounds.at(roundIndex).toArray();
		for (int fightIndex = 0; fightIndex < round.size(); ++fightIndex)
		{
			QJsonObject fight = round.at(fightIndex).toObject();
			if (!fight.value(QStringLiteral("IsSaved")).toBool())
			{
				fight.insert(QStringLiteral("SecondsElapsed"), 0);
				fight.insert(QStringLiteral("IsGoldenScore"), false);
				for (const QString& fighterKey : { QStringLiteral("FirstFighter"), QStringLiteral("SecondFighter") })
				{
					QJsonObject fighter = fight.value(fighterKey).toObject();
					fighter.insert(QStringLiteral("Ippon"), 0);
					fighter.insert(QStringLiteral("Wazaari"), 0);
					fighter.insert(QStringLiteral("Yuko"), 0);
					fighter.insert(QStringLiteral("Shido"), 0);
					fighter.insert(QStringLiteral("Hansokumake"), 0);
					fight.insert(fighterKey, fighter);
				}
			}
			round.replace(fightIndex, fight);
		}
		rounds.replace(roundIndex, round);
	}
	root.insert(QStringLiteral("Rounds"), rounds);
	return QJsonDocument(root);
}

bool MainWindowTeam::UploadRecoveryEvent_(const QJsonObject& event, QJsonObject* response) const
{
#ifdef _WIN32
	const QString competitionDayId = event.value(QStringLiteral("competitionDayId")).toString();
	const QString matId = event.value(QStringLiteral("matId")).toString();
	if (competitionDayId.isEmpty() || matId.isEmpty())
		return false;

	QByteArray body;
	DWORD status = 0;
	const QByteArray payload = QJsonDocument(event).toJson(QJsonDocument::Compact);
	if (!http_json_request(L"PUT", recovery_api_path(competitionDayId, matId), payload, body, status))
		return false;
	if (status < 200 || status >= 300)
		return false;

	if (response)
	{
		QJsonParseError error;
		const QJsonDocument doc = QJsonDocument::fromJson(body, &error);
		if (error.error == QJsonParseError::NoError && doc.isObject())
			*response = doc.object();
	}
	return true;
#else
	Q_UNUSED(event);
	Q_UNUSED(response);
	return false;
#endif
}

bool MainWindowTeam::DownloadRecoveryState_(const QString& competitionDayId, const QString& matId, QJsonObject& recovery) const
{
	recovery = QJsonObject();
#ifdef _WIN32
	QByteArray body;
	DWORD status = 0;
	if (!http_json_request(L"GET", recovery_api_path(competitionDayId, matId), QByteArray(), body, status))
		return false;
	if (status == 404)
		return false;
	if (status < 200 || status >= 300)
		return false;

	QJsonParseError error;
	const QJsonDocument doc = QJsonDocument::fromJson(body, &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject())
		return false;
	recovery = doc.object().value(QStringLiteral("recovery")).toObject();
	return !recovery.isEmpty();
#else
	Q_UNUSED(competitionDayId);
	Q_UNUSED(matId);
	return false;
#endif
}

void MainWindowTeam::FlushRecoveryQueue_()
{
	const QString queuePath = QDir(runtime_data_dir()).filePath(QString::fromLatin1(RecoveryQueueFileName));
	const QJsonDocument doc = read_json_document(queuePath);
	if (!doc.isArray() || doc.array().isEmpty())
		return;

	const QJsonArray queue = doc.array();
	QJsonArray remaining;
	bool blocked = false;
	for (int i = 0; i < queue.size(); ++i)
	{
		const QJsonObject event = queue.at(i).toObject();
		if (!blocked && UploadRecoveryEvent_(event))
			continue;

		blocked = true;
		remaining.append(event);
	}

	if (remaining.isEmpty())
		QFile::remove(queuePath);
	else
		write_json_atomic(queuePath, QJsonDocument(remaining));
}

void MainWindowTeam::PersistCompetitionRecovery_(int completedRound, int completedFight, const QString& reason)
{
	if (m_restoringCompetitionState || m_currentCompetitionDayId.isEmpty() || m_currentMatId.isEmpty())
		return;
	if (completedRound < 0 || completedRound >= m_pController->GetRoundCount() ||
		completedFight < 0 || completedFight >= m_pController->GetFightCount())
		return;
	if (!m_pController->GetFight(completedRound, completedFight).is_saved)
		return;

	const QJsonDocument snapshot = BuildRecoverySnapshot_();
	if (!snapshot.isObject())
		return;

	const QString updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
	QJsonObject local;
	local.insert(QStringLiteral("schema"), QStringLiteral("ipponboard-competition-recovery-1"));
	local.insert(QStringLiteral("competitionDayId"), m_currentCompetitionDayId);
	local.insert(QStringLiteral("matId"), m_currentMatId);
	local.insert(QStringLiteral("terminalId"), TerminalId_());
	local.insert(QStringLiteral("updatedAt"), updatedAt);
	local.insert(QStringLiteral("snapshot"), snapshot.object());
	write_json_atomic(CompetitionStateFilePath_(m_currentCompetitionDayId, m_currentMatId), QJsonDocument(local));

	QJsonObject event;
	event.insert(QStringLiteral("competitionDayId"), m_currentCompetitionDayId);
	event.insert(QStringLiteral("matId"), m_currentMatId);
	event.insert(QStringLiteral("terminalId"), TerminalId_());
	event.insert(QStringLiteral("eventRound"), completedRound);
	event.insert(QStringLiteral("eventFight"), completedFight);
	event.insert(QStringLiteral("reason"), reason);
	event.insert(QStringLiteral("clientUpdatedAt"), updatedAt);
	event.insert(QStringLiteral("snapshot"), snapshot.object());

	FlushRecoveryQueue_();
	if (UploadRecoveryEvent_(event))
		return;

	const QString queuePath = QDir(runtime_data_dir()).filePath(QString::fromLatin1(RecoveryQueueFileName));
	QJsonDocument queueDoc = read_json_document(queuePath);
	QJsonArray queue = queueDoc.isArray() ? queueDoc.array() : QJsonArray();
	bool replaced = false;
	for (int i = 0; i < queue.size(); ++i)
	{
		const QJsonObject queued = queue.at(i).toObject();
		if (queued.value(QStringLiteral("competitionDayId")).toString() == m_currentCompetitionDayId &&
			queued.value(QStringLiteral("matId")).toString() == m_currentMatId &&
			queued.value(QStringLiteral("eventRound")).toInt(-1) == completedRound &&
			queued.value(QStringLiteral("eventFight")).toInt(-1) == completedFight)
		{
			queue.replace(i, event);
			replaced = true;
			break;
		}
	}
	if (!replaced)
		queue.append(event);
	write_json_atomic(queuePath, QJsonDocument(queue));
}

bool MainWindowTeam::RestoreCompetitionState_(const QString& competitionDayId, const QString& matId, bool showMessage)
{
	const QJsonDocument localDoc = read_json_document(CompetitionStateFilePath_(competitionDayId, matId));
	const QJsonObject local = localDoc.isObject() ? localDoc.object() : QJsonObject();
	const QJsonObject localSnapshot = local.value(QStringLiteral("snapshot")).toObject();

	bool localPending = false;
	const QJsonDocument queueDoc = read_json_document(QDir(runtime_data_dir()).filePath(QString::fromLatin1(RecoveryQueueFileName)));
	if (queueDoc.isArray())
	{
		for (const QJsonValue& value : queueDoc.array())
		{
			const QJsonObject event = value.toObject();
			if (event.value(QStringLiteral("competitionDayId")).toString() == competitionDayId &&
				event.value(QStringLiteral("matId")).toString() == matId)
			{
				localPending = true;
				break;
			}
		}
	}

	QJsonObject serverRecovery;
	const bool hasServer = DownloadRecoveryState_(competitionDayId, matId, serverRecovery);
	const QJsonObject serverSnapshot = serverRecovery.value(QStringLiteral("snapshot")).toObject();

	QJsonObject selected;
	QString source;
	if (localPending && !localSnapshot.isEmpty())
	{
		selected = localSnapshot;
		source = QStringLiteral("lokal, noch nicht vollständig synchronisiert");
	}
	else if (hasServer && !serverSnapshot.isEmpty())
	{
		selected = serverSnapshot;
		source = QStringLiteral("Server");
	}
	else if (!localSnapshot.isEmpty())
	{
		selected = localSnapshot;
		source = QStringLiteral("lokal");
	}

	if (selected.isEmpty())
		return false;

	QJsonDocument tournamentDoc(selected);
	m_restoringCompetitionState = true;
	int result = LoadTournamentFromJson_(tournamentDoc);
	if (result == 1)
		result = LoadTournamentFromJson_(tournamentDoc, true);
	m_restoringCompetitionState = false;
	if (result != 0)
		return false;

	if (source == QStringLiteral("Server"))
	{
		QJsonObject cached;
		cached.insert(QStringLiteral("schema"), QStringLiteral("ipponboard-competition-recovery-1"));
		cached.insert(QStringLiteral("competitionDayId"), competitionDayId);
		cached.insert(QStringLiteral("matId"), matId);
		cached.insert(QStringLiteral("terminalId"), TerminalId_());
		cached.insert(QStringLiteral("updatedAt"), serverRecovery.value(QStringLiteral("updatedAt")).toString());
		cached.insert(QStringLiteral("snapshot"), selected);
		write_json_atomic(CompetitionStateFilePath_(competitionDayId, matId), QJsonDocument(cached));
	}

	if (showMessage)
		QMessageBox::information(this, QStringLiteral("Kampftag geladen"),
			QStringLiteral("Der letzte gespeicherte Stand wurde aus %1 wiederhergestellt.").arg(source));
	return true;
}

void MainWindowTeam::SaveLastCompetitionSession_() const
{
	if (m_currentCompetitionDayId.isEmpty() || m_currentMatId.isEmpty())
		return;
	QJsonObject session;
	session.insert(QStringLiteral("competitionDayId"), m_currentCompetitionDayId);
	session.insert(QStringLiteral("matId"), m_currentMatId);
	session.insert(QStringLiteral("updatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
	write_json_atomic(QDir(runtime_data_dir()).filePath(QString::fromLatin1(LastSessionFileName)), QJsonDocument(session));
}

void MainWindowTeam::ClearCompetitionDayFilter_()
{
	m_currentCompetitionDayId.clear();
	m_currentMatId.clear();
	m_competitionDayTeamIds.clear();
	QFile::remove(QDir(runtime_data_dir()).filePath(QString::fromLatin1(LastSessionFileName)));
	setWindowTitle(QStringLiteral("Ipponboard-Meschede v%1").arg(QApplication::applicationVersion()));
}

bool MainWindowTeam::ApplyCompetitionDay_(const QJsonObject& day, const QString& matId, const QString& matName,
	bool confirmDiscard, bool showRestoreMessage)
{
	QStringList teamIds;
	for (const QJsonValue& value : day.value(QStringLiteral("teamIds")).toArray())
	{
		const QString id = value.toString();
		if (!id.isEmpty() && !teamIds.contains(id))
			teamIds.append(id);
	}
	if (teamIds.size() < 2)
	{
		if (confirmDiscard)
			QMessageBox::warning(this, QStringLiteral("Kampftag laden"),
				QStringLiteral("Dieser Kampftag enthält weniger als zwei teilnehmende Mannschaften."));
		return false;
	}

	if (confirmDiscard && QMessageBox::question(
		this,
		QStringLiteral("Kampftag laden"),
		QStringLiteral("Der aktuelle Turnierstand wird verworfen und der Kampftag geladen. Fortfahren?"),
		QMessageBox::Yes,
		QMessageBox::No) == QMessageBox::No)
	{
		return false;
	}

	m_currentCompetitionDayId = day.value(QStringLiteral("id")).toString();
	m_currentMatId = matId;
	m_competitionDayTeamIds = teamIds;
	update_club_views();

	const QString modeId = day.value(QStringLiteral("tournamentModeId")).toString();
	const int modeIndex = modeId.isEmpty() ? -1 : m_pUi->comboBox_mode->findData(modeId);
	if (modeIndex >= 0 && modeIndex != m_pUi->comboBox_mode->currentIndex())
		m_pUi->comboBox_mode->setCurrentIndex(modeIndex);
	else
		on_comboBox_mode_currentIndexChanged(m_pUi->comboBox_mode->currentIndex());

	const QString hostClubId = day.value(QStringLiteral("hostClubId")).toString();
	const int hostIndex = m_pUi->comboBox_club_host->findData(hostClubId);
	if (hostIndex >= 0)
		m_pUi->comboBox_club_host->setCurrentIndex(hostIndex);

	const QDate date = QDate::fromString(day.value(QStringLiteral("date")).toString(), Qt::ISODate);
	if (date.isValid())
		m_pUi->dateEdit->setDate(date);
	m_pUi->lineEdit_location->setText(day.value(QStringLiteral("location")).toString());

	const QString dayName = day.value(QStringLiteral("name")).toString();
	setWindowTitle(QStringLiteral("Ipponboard-Meschede v%1 — %2 / %3")
		.arg(QApplication::applicationVersion(), dayName, matName));

	UpdateTeamFighterDelegates_();
	update_score_screen();
	SaveLastCompetitionSession_();
	RestoreCompetitionState_(m_currentCompetitionDayId, m_currentMatId, showRestoreMessage);
	setWindowTitle(QStringLiteral("Ipponboard-Meschede v%1 — %2 / %3")
		.arg(QApplication::applicationVersion(), dayName, matName));
	FlushRecoveryQueue_();
	return true;
}

bool MainWindowTeam::RestoreLastCompetitionSession_()
{
	const QJsonDocument doc = read_json_document(QDir(runtime_data_dir()).filePath(QString::fromLatin1(LastSessionFileName)));
	if (!doc.isObject())
		return false;
	const QJsonObject session = doc.object();
	const QString dayId = session.value(QStringLiteral("competitionDayId")).toString();
	const QString matId = session.value(QStringLiteral("matId")).toString();
	if (dayId.isEmpty() || matId.isEmpty())
		return false;

	QJsonObject day;
	for (const QJsonValue& value : m_masterCompetitionDays)
	{
		const QJsonObject candidate = value.toObject();
		if (candidate.value(QStringLiteral("id")).toString() == dayId)
		{
			day = candidate;
			break;
		}
	}
	if (day.isEmpty())
		return false;

	QString matName = matId;
	for (const QJsonValue& value : day.value(QStringLiteral("mats")).toArray())
	{
		const QJsonObject mat = value.toObject();
		if (mat.value(QStringLiteral("id")).toString() == matId)
		{
			matName = mat.value(QStringLiteral("name")).toString(matId);
			break;
		}
	}
	if (matName == matId && matId.startsWith(QStringLiteral("mat-")))
		matName = QStringLiteral("Matte %1").arg(matId.mid(4));

	return ApplyCompetitionDay_(day, matId, matName, false, false);
}

bool MainWindowTeam::LoadCompetitionDay_()
{
	LoadMasterDataCache_();

	QVector<QJsonObject> days;
	QStringList labels;
	for (const QJsonValue& value : m_masterCompetitionDays)
	{
		const QJsonObject day = value.toObject();
		const QString id = day.value(QStringLiteral("id")).toString();
		const QString name = day.value(QStringLiteral("name")).toString();
		if (id.isEmpty() || name.isEmpty())
			continue;

		const QString date = day.value(QStringLiteral("date")).toString();
		const QString status = day.value(QStringLiteral("status")).toString();
		QString label = date.isEmpty() ? name : date + QStringLiteral(" – ") + name;
		if (status == QStringLiteral("closed"))
			label += QStringLiteral(" [abgeschlossen]");
		days.append(day);
		labels.append(label);
	}

	if (days.isEmpty())
	{
		QMessageBox::information(this,
			QStringLiteral("Kampftag laden"),
			QStringLiteral("Im synchronisierten Datenstand sind noch keine Kampftage vorhanden."));
		return false;
	}

	bool ok = false;
	const QString selectedLabel = QInputDialog::getItem(
		this,
		QStringLiteral("Kampftag laden"),
		QStringLiteral("Kampftag:"),
		labels,
		0,
		false,
		&ok);
	if (!ok)
		return false;

	const int selectedIndex = labels.indexOf(selectedLabel);
	if (selectedIndex < 0 || selectedIndex >= days.size())
		return false;
	const QJsonObject day = days.at(selectedIndex);

	QString matId = QStringLiteral("mat-1");
	QString matName = QStringLiteral("Matte 1");
	QStringList matLabels;
	QVector<QJsonObject> mats;
	for (const QJsonValue& value : day.value(QStringLiteral("mats")).toArray())
	{
		const QJsonObject mat = value.toObject();
		if (mat.value(QStringLiteral("id")).toString().isEmpty())
			continue;
		mats.append(mat);
		matLabels.append(mat.value(QStringLiteral("name")).toString(
			QStringLiteral("Matte %1").arg(mats.size())));
	}
	if (mats.isEmpty())
	{
		const int count = qMax(1, day.value(QStringLiteral("matCount")).toInt(1));
		for (int i = 0; i < count; ++i)
		{
			QJsonObject mat;
			mat.insert(QStringLiteral("id"), QStringLiteral("mat-%1").arg(i + 1));
			mat.insert(QStringLiteral("name"), QStringLiteral("Matte %1").arg(i + 1));
			mats.append(mat);
			matLabels.append(mat.value(QStringLiteral("name")).toString());
		}
	}

	if (mats.size() > 1)
	{
		const QString selectedMat = QInputDialog::getItem(
			this,
			QStringLiteral("Matte wählen"),
			QStringLiteral("Matte:"),
			matLabels,
			0,
			false,
			&ok);
		if (!ok)
			return false;
		const int matIndex = matLabels.indexOf(selectedMat);
		if (matIndex < 0 || matIndex >= mats.size())
			return false;
		matId = mats.at(matIndex).value(QStringLiteral("id")).toString();
		matName = mats.at(matIndex).value(QStringLiteral("name")).toString(selectedMat);
	}
	else
	{
		matId = mats.first().value(QStringLiteral("id")).toString(matId);
		matName = mats.first().value(QStringLiteral("name")).toString(matName);
	}

	return ApplyCompetitionDay_(day, matId, matName, true, true);
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
	QMessageBox source(this);
	source.setWindowTitle(QStringLiteral("Turnier laden"));
	source.setText(QStringLiteral("Was möchten Sie laden?"));
	QAbstractButton* competitionDayButton = source.addButton(QStringLiteral("Kampftag"), QMessageBox::AcceptRole);
	QAbstractButton* fileButton = source.addButton(QStringLiteral("Lokale Turnierdatei"), QMessageBox::ActionRole);
	source.addButton(QMessageBox::Cancel);
	source.exec();

	if (source.clickedButton() == competitionDayButton)
	{
		LoadCompetitionDay_();
		return;
	}
	if (source.clickedButton() != fileButton)
		return;

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

	// A local tournament file is independent from any previously loaded Kampftag filter.
	ClearCompetitionDayFilter_();

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
}

void MainWindowTeam::on_actionPrint_triggered()
{
	if (!IsExactNwjv5Template_())
		WriteScoreToHtml_();

	QPrinter printer(QPrinter::HighResolution);
    //TODO: fix margins (actual header margin is too big)
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);
    printer.setFullPage(IsExactNwjv5Template_());
	QPrintPreviewDialog preview(&printer, this);
	connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(Print(QPrinter*)));
	preview.exec();
}

void MainWindowTeam::on_actionExport_triggered()
{
	// The exact 5er PDF is painted directly from measured PDF coordinates.
	// HTML is still generated for HTML export and all other templates.
	if (!IsExactNwjv5Template_())
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
			if (IsExactNwjv5Template_())
				WriteScoreToHtml_();
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
            printer.setFullPage(true);
            printer.setPageOrientation(QPageLayout::Landscape);
            printer.setOutputFormat(QPrinter::PdfFormat);
            printer.setPageSize(QPageSize(QPageSize::A4));
            printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);
			printer.setOutputFileName(fileName);
			if (IsExactNwjv5Template_())
			{
				PrintExactNwjv5_(&printer);
			}
			else
			{
				QTextEdit edit(m_htmlScore, this);
				edit.document()->print(&printer);
			}
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


bool MainWindowTeam::IsExactNwjv5Template_() const
{
	const QString templateFile = get_template_file(m_currentMode);
	return templateFile.endsWith(QStringLiteral("list_output_nwjv_5_hinundrueck.html"), Qt::CaseInsensitive);
}

void MainWindowTeam::PrintExactNwjv5_(QPrinter* p)
{
	if (!p)
		return;

	QPainter painter;
	if (!painter.begin(p))
		return;

	// The official NWJV source is A4 landscape, 842 x 595 PDF points.
	// Everything below uses this exact coordinate system. The viewport maps it
	// to the physical A4 page without HTML layout or browser scaling.
	const QRect page = p->pageRect(QPrinter::DevicePixel).toRect();
	painter.setViewport(page);
	painter.setWindow(QRect(0, 0, 842, 595));
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::TextAntialiasing, true);

	const QColor black(0, 0, 0);
	const QColor blue(79, 134, 207);
	const QColor dotted(90, 90, 90);
	const QPen majorPen(black, 2.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	const QPen thinPen(black, 1.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	const QPen dottedPen(dotted, 0.65, Qt::DotLine, Qt::SquareCap, Qt::MiterJoin);

	auto font = [](int px, bool bold = false)
	{
		QFont f(QStringLiteral("Arial"));
		f.setPixelSize(px);
		f.setBold(bold);
		return f;
	};
	auto drawCentered = [&](const QRectF& r, const QString& text, int px, bool bold = false, const QColor& color = QColor(0,0,0))
	{
		painter.save();
		painter.setPen(color);
		painter.setFont(font(px, bold));
		painter.drawText(r, Qt::AlignCenter | Qt::TextSingleLine, text);
		painter.restore();
	};
	auto drawLeft = [&](const QRectF& r, const QString& text, int px, bool bold = false, const QColor& color = QColor(0,0,0))
	{
		painter.save();
		painter.setPen(color);
		painter.setFont(font(px, bold));
		painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
		painter.restore();
	};
	auto drawCenteredWrapped = [&](const QRectF& r, const QString& text, int px, bool bold = false, const QColor& color = QColor(0,0,0))
	{
		painter.save();
		painter.setPen(color);
		painter.setFont(font(px, bold));
		painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, text);
		painter.restore();
	};
	auto drawVertical = [&](const QRectF& r, const QString& text, int px, bool bold, const QColor& color)
	{
		painter.save();
		painter.setPen(color);
		painter.setFont(font(px, bold));
		const QPointF c = r.center();
		painter.translate(c);
		painter.rotate(-90.0);
		const QRectF rr(-r.height()/2.0, -r.width()/2.0, r.height(), r.width());
		painter.drawText(rr, Qt::AlignCenter | Qt::TextSingleLine, text);
		painter.restore();
	};
	auto drawVerticalMultiline = [&](const QRectF& r, const QString& text, int px, bool bold, const QColor& color)
	{
		painter.save();
		painter.setPen(color);
		painter.setFont(font(px, bold));
		const QPointF c = r.center();
		painter.translate(c);
		painter.rotate(-90.0);
		const QRectF rr(-r.height()/2.0, -r.width()/2.0, r.height(), r.width());
		painter.drawText(rr, Qt::AlignCenter, text);
		painter.restore();
	};

	// --- fixed official NWJV form geometry ---
	painter.fillRect(QRectF(0,0,842,595), Qt::white);

	// NWJV logo: reuse the embedded official logo from the existing NWJV template.
	{
		const QString htmlPath = fm::GetSettingsFilePath("templates/list_output_nwjv_5_hinundrueck.html");
		QFile logoSource(htmlPath);
		if (logoSource.open(QFile::ReadOnly))
		{
			const QString html = QString::fromUtf8(logoSource.readAll());
			const QRegularExpression rx(QStringLiteral("data:image/jpeg;base64,([^\\\"]+)"));
			const QRegularExpressionMatch m = rx.match(html);
			if (m.hasMatch())
			{
				const QImage logo = QImage::fromData(QByteArray::fromBase64(m.captured(1).toLatin1()), "JPG");
				if (!logo.isNull())
					painter.drawImage(QRectF(14.0, 35.65385, 94.75, 52.15386), logo);
			}
		}
	}

	// Header texts exactly on the official form.
	drawLeft(QRectF(133, 16.0, 190, 13), QStringLiteral("Mannschaftsliste mit Unterbewertung"), 10, true);
	drawLeft(QRectF(357, 16.0, 28, 13), QStringLiteral("Art:"), 10, true);
	drawLeft(QRectF(585, 16.0, 28, 13), QStringLiteral("Ort:"), 10, true);
	drawLeft(QRectF(725, 16.0, 25, 13), QStringLiteral("am"), 10, true);

	// Upper WEISS / NWJV / BLAU frame.
	painter.setPen(majorPen);
	painter.drawRect(QRectF(129.5, 40.5, 656.5, 51.5));
	painter.drawLine(QPointF(422.0,40.5), QPointF(422.0,92.0));
	painter.drawLine(QPointF(498.0,40.5), QPointF(498.0,92.0));
	drawCenteredWrapped(QRectF(140, 46, 202, 40), m_pUi->comboBox_club_home->currentText(), 15, true, black);
	drawCentered(QRectF(350, 54, 71, 24), QStringLiteral("WEISS"), 16, true, black);
	drawCenteredWrapped(QRectF(506, 46, 200, 40), m_pUi->comboBox_club_guest->currentText(), 15, true, black);
	drawCentered(QRectF(714, 54, 72, 24), QStringLiteral("BLAU"), 16, true, blue);
	drawCentered(QRectF(424, 44, 73, 14), QStringLiteral("Nordrhein-"), 10, false, black);
	drawCentered(QRectF(424, 61, 73, 14), QStringLiteral("Westfälischer"), 10, false, black);
	drawCentered(QRectF(424, 77, 73, 14), QStringLiteral("Judo-Verband"), 10, false, black);

	// Exact column coordinates measured from the supplied official PDF.
	const QVector<qreal> x = {14,54,214,242,270,298,326,354,388,422,582,610,638,666,694,722,756,786,826};
	const qreal yTeamTop=92, yTeamBottom=110, yHeaderBottom=167;
	const QVector<qreal> rowBottom1 = {197,227,257,287,317};
	const qreal yHinSumBottom=332;
	const QVector<qreal> rowBottom2 = {362,392,422,452,482};

	// Team header row and main grid.
	painter.setPen(majorPen);
	painter.drawLine(QPointF(54,yTeamTop), QPointF(786,yTeamTop));
	painter.drawLine(QPointF(54,yTeamBottom), QPointF(786,yTeamBottom));
	painter.drawLine(QPointF(14,yTeamBottom), QPointF(826,yTeamBottom));
	for (qreal xv : x)
		painter.drawLine(QPointF(xv, yTeamBottom), QPointF(xv, 317));

	// Team-row boundaries only between the official operator groups.
	// Internal score-column lines must not cut through + / - / =.
	for(qreal xv : QVector<qreal>{54,214,298,354,422,582,666,722,786})
		painter.drawLine(QPointF(xv,yTeamTop), QPointF(xv,yTeamBottom));

	// Header bottom and first-round rows.
	painter.drawLine(QPointF(14,yHeaderBottom), QPointF(826,yHeaderBottom));
	for(qreal yv : rowBottom1)
		painter.drawLine(QPointF(14,yv), QPointF(826,yv));

	// Dotted score separators (official form).
	painter.setPen(dottedPen);
	for(qreal xv : QVector<qreal>{242,270,298,326,610,638,666,694})
	{
		painter.drawLine(QPointF(xv,yHeaderBottom), QPointF(xv,317));
	}

	// Re-draw major score boundaries over dotted grid.
	painter.setPen(majorPen);
	for(qreal xv : QVector<qreal>{14,54,214,354,388,422,582,722,756,786,826})
		painter.drawLine(QPointF(xv,yTeamBottom), QPointF(xv,317));

	// First-round summary line and second round.
	painter.drawLine(QPointF(14,332), QPointF(826,332));
	for(qreal xv : x)
		painter.drawLine(QPointF(xv,332), QPointF(xv,482));
	for(qreal yv : rowBottom2)
		painter.drawLine(QPointF(14,yv), QPointF(826,yv));
	painter.setPen(dottedPen);
	for(qreal xv : QVector<qreal>{242,270,298,326,610,638,666,694})
		painter.drawLine(QPointF(xv,332), QPointF(xv,482));
	painter.setPen(majorPen);
	for(qreal xv : QVector<qreal>{14,54,214,354,388,422,582,722,756,786,826})
		painter.drawLine(QPointF(xv,332), QPointF(xv,482));

	// Operator labels. Club names are shown in the large WEISS/BLAU header fields.
	drawCentered(QRectF(214,92,84,18), QStringLiteral("+"), 17, true, black);
	drawCentered(QRectF(298,92,56,18), QStringLiteral("-"), 17, true, black);
	drawCentered(QRectF(354,92,68,18), QStringLiteral("="), 17, true, black);
	drawCentered(QRectF(582,92,84,18), QStringLiteral("+"), 17, true, blue);
	drawCentered(QRectF(666,92,56,18), QStringLiteral("-"), 17, true, blue);
	drawCentered(QRectF(722,92,64,18), QStringLiteral("="), 17, true, blue);

	// Column headings.
	drawCentered(QRectF(14,145,40,22), QStringLiteral("kg"), 16, true, black);
	drawCentered(QRectF(54,145,160,22), QStringLiteral("Judoka"), 16, true, black);
	drawVertical(QRectF(214,111,28,56), QStringLiteral("Yuko"), 10, false, black);
	drawVertical(QRectF(242,111,28,56), QStringLiteral("Waza-ari"), 10, false, black);
	drawVertical(QRectF(270,111,28,56), QStringLiteral("Ippon"), 10, false, black);
	drawVertical(QRectF(298,111,28,56), QStringLiteral("Shido"), 10, false, black);
	drawVerticalMultiline(QRectF(326,111,28,56), QStringLiteral("Hansoku-\nmake"), 10, false, black);
	drawVertical(QRectF(354,111,34,56), QStringLiteral("SIEG"), 10, false, black);
	drawVerticalMultiline(QRectF(388,111,34,56), QStringLiteral("Unterbe-\nwertung"), 10, false, black);

	drawCentered(QRectF(422,145,160,22), QStringLiteral("Judoka"), 16, true, blue);
	drawVertical(QRectF(582,111,28,56), QStringLiteral("Yuko"), 10, false, blue);
	drawVertical(QRectF(610,111,28,56), QStringLiteral("Waza-ari"), 10, false, blue);
	drawVertical(QRectF(638,111,28,56), QStringLiteral("Ippon"), 10, false, blue);
	drawVertical(QRectF(666,111,28,56), QStringLiteral("Shido"), 10, false, blue);
	drawVerticalMultiline(QRectF(694,111,28,56), QStringLiteral("Hansoku-\nmake"), 10, false, blue);
	drawVertical(QRectF(722,111,34,56), QStringLiteral("SIEG"), 10, false, blue);
	drawVerticalMultiline(QRectF(756,111,30,56), QStringLiteral("Unterbe-\nwertung"), 10, false, blue);
	drawVertical(QRectF(786,111,40,56), QStringLiteral("Wettkampfzeit"), 10, false, black);

	// Summary fields and footer fixed labels.
	drawLeft(QRectF(302,316,52,16), QStringLiteral("Hinrunde"), 9, false, black);
	drawLeft(QRectF(302,482,52,15), QStringLiteral("Rückrunde"), 9, false, black);
	drawLeft(QRectF(310,497,44,15), QStringLiteral("Hinrunde"), 9, false, black);
	drawLeft(QRectF(329,512,25,15), QStringLiteral("Total"), 9, false, black);

	// Summary boxes.
	painter.setPen(majorPen);
	for(const QRectF& r : QVector<QRectF>{
		QRectF(354,317,34,15),QRectF(388,317,34,15),QRectF(722,317,34,15),QRectF(756,317,30,15),
		QRectF(354,482,34,15),QRectF(388,482,34,15),QRectF(722,482,34,15),QRectF(756,482,30,15),
		QRectF(354,497,34,15),QRectF(388,497,34,15),QRectF(722,497,34,15),QRectF(756,497,30,15),
		QRectF(354,512,34,15),QRectF(388,512,34,15),QRectF(722,512,34,15),QRectF(756,512,30,15)})
		painter.drawRect(r);

	// Signature lines and note.
	painter.setPen(thinPen);
	painter.drawLine(QPointF(35,543), QPointF(167,543));
	painter.drawLine(QPointF(191,543), QPointF(323,543));
	painter.drawLine(QPointF(504,543), QPointF(690,543));
	drawCentered(QRectF(35,545,132,16), QStringLiteral("Listenführung"), 10, false, black);
	drawCentered(QRectF(191,545,132,16), QStringLiteral("Kampfrichter"), 10, false, black);
	drawCentered(QRectF(504,545,186,16), QStringLiteral("Sportl. Leitung"), 10, false, black);
	drawLeft(QRectF(17,564,390,18), QStringLiteral("Anmerkung: Ein Tausch von Judoka bei der Rückrunde ist möglich, aber kein Muss!"), 10, false, black);

	// --- variable content ---
	const QString modeText = get_full_mode_title(m_currentMode);
	drawLeft(QRectF(380,14,198,17), modeText, 9, false, black);
	drawLeft(QRectF(610,14,108,17), m_pUi->lineEdit_location->text(), 9, false, black);
	drawLeft(QRectF(744,14,80,17), m_pUi->dateEdit->text(), 9, false, black);

	auto scoreText = [](const Fight& fight, int value)
	{
		if (!fight.is_saved || value == 0)
			return QString();
		return QString::number(value);
	};
	auto resultText = scoreText;
	auto timeText = [](const Fight& fight)
	{
		return !fight.is_saved ? QString() : fight.GetTotalTimeElapsedString();
	};

	const QVector<qreal> rowTopFirst = {167,197,227,257,287};
	const QVector<qreal> rowTopSecond = {332,362,392,422,452};

	auto drawFight = [&](const Fight& fight, qreal y)
	{
		const qreal h=30.0;
		drawCentered(QRectF(14,y,40,h), fight.weight, 8, false, black);
		drawLeft(QRectF(58,y,152,h), fight.fighters[static_cast<int>(FighterEnum::First)].name, 8, false, black);
		const auto& s1=fight.GetScore1();
		drawCentered(QRectF(214,y,28,h), scoreText(fight,s1.Yuko()), 8, false, black);
		drawCentered(QRectF(242,y,28,h), scoreText(fight,s1.Wazaari()), 8, false, black);
		drawCentered(QRectF(270,y,28,h), scoreText(fight,s1.Ippon()), 8, false, black);
		drawCentered(QRectF(298,y,28,h), scoreText(fight,s1.Shido()), 8, false, black);
		drawCentered(QRectF(326,y,28,h), scoreText(fight,s1.Hansokumake()), 8, false, black);
		drawCentered(QRectF(354,y,34,h), resultText(fight,fight.HasWon(FighterEnum::First)), 8, false, black);
		drawCentered(QRectF(388,y,34,h), resultText(fight,fight.GetScorePoints(FighterEnum::First)), 8, false, black);

		drawLeft(QRectF(426,y,152,h), fight.fighters[static_cast<int>(FighterEnum::Second)].name, 8, false, black);
		const auto& s2=fight.GetScore2();
		drawCentered(QRectF(582,y,28,h), scoreText(fight,s2.Yuko()), 8, false, black);
		drawCentered(QRectF(610,y,28,h), scoreText(fight,s2.Wazaari()), 8, false, black);
		drawCentered(QRectF(638,y,28,h), scoreText(fight,s2.Ippon()), 8, false, black);
		drawCentered(QRectF(666,y,28,h), scoreText(fight,s2.Shido()), 8, false, black);
		drawCentered(QRectF(694,y,28,h), scoreText(fight,s2.Hansokumake()), 8, false, black);
		drawCentered(QRectF(722,y,34,h), resultText(fight,fight.HasWon(FighterEnum::Second)), 8, false, black);
		drawCentered(QRectF(756,y,30,h), resultText(fight,fight.GetScorePoints(FighterEnum::Second)), 8, false, black);
		drawCentered(QRectF(786,y,40,h), timeText(fight), 8, false, black);
	};

	const int fights = qMin(5, m_pController->GetFightCount());
	for(int i=0;i<fights;++i)
		drawFight(m_pController->GetFight(0,i), rowTopFirst.at(i));
	if(m_pController->GetRoundCount()>1)
		for(int i=0;i<fights;++i)
			drawFight(m_pController->GetFight(1,i), rowTopSecond.at(i));

	const auto wins1=m_pController->GetTournamentScoreModel(0)->GetTotalWins();
	const auto score1=m_pController->GetTournamentScoreModel(0)->GetTotalScore();
	const auto wins2=m_pController->GetRoundCount()>1?m_pController->GetTournamentScoreModel(1)->GetTotalWins():std::make_pair<unsigned,unsigned>(0,0);
	const auto score2=m_pController->GetRoundCount()>1?m_pController->GetTournamentScoreModel(1)->GetTotalScore():std::make_pair<unsigned,unsigned>(0,0);
	const auto totalWins=std::make_pair(wins1.first+wins2.first,wins1.second+wins2.second);
	const auto totalScore=std::make_pair(score1.first+score2.first,score1.second+score2.second);

	auto summary = [&](qreal y, const std::pair<unsigned,unsigned>& wins, const std::pair<unsigned,unsigned>& score)
	{
		painter.fillRect(QRectF(355,y+1,32,13),Qt::white);
		painter.fillRect(QRectF(389,y+1,32,13),Qt::white);
		painter.fillRect(QRectF(723,y+1,32,13),Qt::white);
		painter.fillRect(QRectF(757,y+1,28,13),Qt::white);
		drawCentered(QRectF(354,y,34,15),wins.first==0?QString():QString::number(wins.first),9,true,black);
		drawCentered(QRectF(388,y,34,15),score.first==0?QString():QString::number(score.first),9,true,black);
		drawCentered(QRectF(722,y,34,15),wins.second==0?QString():QString::number(wins.second),9,true,black);
		drawCentered(QRectF(756,y,30,15),score.second==0?QString():QString::number(score.second),9,true,black);
	};
	summary(317,wins1,score1);
	summary(482,wins2,score2);
	summary(497,wins1,score1);
	summary(512,totalWins,totalScore);

	painter.end();
}

void MainWindowTeam::Print(QPrinter* p)
{
	if (IsExactNwjv5Template_())
	{
		PrintExactNwjv5_(p);
		return;
	}

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
