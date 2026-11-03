// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "SettingsDlg.h"
#include "KeyboardShortcuts.h"
#include "ui_SettingsDlg.h"
#ifdef _WIN32
#include "../gamepad/gamepad.h"
#endif
#include "../core/ControllerConfig.h"

#include <QStringList>
#include <QDesktopWidget>
#include <QFile>
#include <QDir>
#include <QScreen>
#include <QSound>
#include <QColorDialog>
#include <QDebug>
#include <QApplication>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QHeaderView>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QSet>

using namespace Ipponboard;

SettingsDlg::SettingsDlg(EditionType edition, QWidget* parent) :
	QDialog(parent),
	m_edition(edition),
	ui(new Ui::SettingsDlg)
{
	ui->setupUi(this);
    setWindowIcon(QApplication::windowIcon());

    // Preserve the original settings signals and fields, but give the
    // administration dialog an independent Ipponboard-Meschede visual identity.
    setWindowTitle(QStringLiteral("Ipponboard-Meschede · Einstellungen"));
    setMinimumSize(800, 620);
    resize(920, 760);
    setStyleSheet(QStringLiteral(R"qss(
        QDialog { background:#0b1017; color:#e7ecf2; font: 11pt 'Segoe UI'; }
        QWidget#tab_general, QWidget#tab_keys, QWidget#tab { background:#0b1017; }
        QTabWidget::pane { border:0; background:#0b1017; top:0; }
        QTabBar::tab { background:#202b38; color:#c9d1db; border:1px solid #334253;
                       padding:13px 23px; margin-right:7px; border-radius:7px; }
        QTabBar::tab:selected { color:white; background:#9b1922; border-color:#c62a35; }
        QTabBar::tab:hover:!selected { background:#334354; }
        QGroupBox { background:#17202b; color:#f3f5f8;
                    border:1px solid #384758; border-radius:10px;
                    margin-top:16px; padding:16px 12px 12px; font-weight:600; }
        QGroupBox::title { subcontrol-origin:margin; subcontrol-position:top left;
                           left:17px; padding:0 9px; color:#f16b75; }
        QLabel { color:#d5dce6; }
        QLineEdit, QComboBox, QFontComboBox, QKeySequenceEdit {
            background:#263240; color:#f7f8f9; border:1px solid #4b6073;
            border-radius:6px; padding:7px; min-height:25px; }
        QComboBox:disabled, QLineEdit:disabled { color:#82909c; background:#202832; }
        QComboBox::drop-down { border:none; width:26px; }
        QComboBox QAbstractItemView { background:#253241; color:white; selection-background-color:#a51d2b; }
        QTableWidget { background:#18232e; color:#e7ebf0; alternate-background-color:#202c38;
                       gridline-color:#35475b; border:1px solid #405066; border-radius:8px; }
        QTableWidget::item:selected { background:#77222c; }
        QHeaderView::section { background:#243546; color:#fafafa;
                               padding:11px; border:0; border-right:1px solid #435466; }
        QPushButton, QToolButton {
            background:#273747; color:white; border:1px solid #4a6279;
            border-radius:7px; padding:8px 13px; min-height:24px; }
        QPushButton:hover, QToolButton:hover { background:#364e63; }
        QPushButton:pressed { background:#a51a26; }
        QPushButton:disabled { background:#1b2732; color:#778795; }
        QCheckBox { spacing:7px; color:#e4e8ed; }
        QCheckBox::indicator { width:16px; height:16px; }
        QScrollBar:vertical { background:#17202b; width:12px; }
        QScrollBar::handle:vertical { background:#516375; border-radius:5px; min-height:20px; }
    )qss"));
    ui->tab_extended->setTabText(ui->tab_extended->indexOf(ui->tab_general), QStringLiteral("Anzeige & Design"));
    ui->tab_extended->setTabText(ui->tab_extended->indexOf(ui->tab_keys), QStringLiteral("Tastatur / Numpad"));
    ui->tab_extended->setTabText(ui->tab_extended->indexOf(ui->tab), QStringLiteral("Erweitert"));
    ui->tab_extended->setTabIcon(ui->tab_extended->indexOf(ui->tab_general), QIcon());
    ui->tab_extended->setTabIcon(ui->tab_extended->indexOf(ui->tab_keys), QIcon());
    ui->tab_extended->setTabIcon(ui->tab_extended->indexOf(ui->tab), QIcon());
    // Gamepad input continues to work with its existing configuration.
    ui->tab_extended->removeTab(ui->tab_extended->indexOf(ui->tab_gamepad));
    ui->groupBox_2nd_view->setTitle(QStringLiteral("01   EXTERNE ANZEIGE"));
    ui->groupBox_sound->setTitle(QStringLiteral("02   MATTE & SIGNAL"));
    ui->groupBox_info_header->setTitle(QStringLiteral("03   SCHRIFT & FARBGEBUNG"));
    if (auto* generalLayout = qobject_cast<QVBoxLayout*>(ui->tab_general->layout())) {
        auto* header = new QLabel(
            QStringLiteral("<span style='font-size:22px;font-weight:700;color:#ffffff'>"
                "Anzeigetafel gestalten</span><br>"
                "<span style='font-size:12px;color:#a9b8c6'>"
                "Bildschirm, Beschriftung, Tonsignal und Farbschema für den Wettkampfbetrieb.</span>"),
            ui->tab_general);
        header->setTextFormat(Qt::RichText);
        header->setMinimumHeight(65);
        generalLayout->insertWidget(0, header);
        generalLayout->setSpacing(12);
    }
    ui->label_5->setText(QStringLiteral(
        "Tastenkombination anklicken und die neue Taste am Gerät drücken. "
        "Alle Änderungen werden lokal auf dem USB-Datenträger gespeichert."));
    ui->label_5->setWordWrap(true);

	if (m_edition == EditionType::Team)
	{
		ui->comboBox_next->setEnabled(true);
		ui->comboBox_prev->setEnabled(true);
		ui->comboBox_pause->setEnabled(true);

		ui->comboBox_mat->setEnabled(false);
		ui->label_home->setEnabled(true);
		ui->label_guest->setEnabled(true);
		ui->lineEdit_labelHome->setEnabled(true);
		ui->lineEdit_labelGuest->setEnabled(true);
	}
	else
	{
		ui->comboBox_mat->setEnabled(true);
		ui->label_home->setEnabled(false);
		ui->label_guest->setEnabled(false);
		ui->lineEdit_labelHome->setEnabled(false);
		ui->lineEdit_labelGuest->setEnabled(false);
	}

	//TODO: remove comment
	//ui->tabWidget->removeTab(ui->tabWidget->count() - 1);

	ui->text_text_sample->SetText("  Ipponboard   ");
	ui->text_color_first->SetText(tr("FIRST FIGHTER"));
	ui->text_color_second->SetText(tr("SECOND FIGHTER"));

#ifdef _WIN32
    // build button text map
	m_buttonTexts[FMlib::Gamepad::eButton1] = "button 1";
	m_buttonTexts[FMlib::Gamepad::eButton2] = "button 2";
	m_buttonTexts[FMlib::Gamepad::eButton3] = "button 3";
	m_buttonTexts[FMlib::Gamepad::eButton4] = "button 4";
	m_buttonTexts[FMlib::Gamepad::eButton5] = "button 5";
	m_buttonTexts[FMlib::Gamepad::eButton6] = "button 6";
	m_buttonTexts[FMlib::Gamepad::eButton7] = "button 7";
	m_buttonTexts[FMlib::Gamepad::eButton8] = "button 8";
	m_buttonTexts[FMlib::Gamepad::eButton9] = "button 9";
	m_buttonTexts[FMlib::Gamepad::eButton10] = "button 10";
	m_buttonTexts[FMlib::Gamepad::eButton11] = "button 11";
	m_buttonTexts[FMlib::Gamepad::eButton12] = "button 12";
	// ! Assure that pov values do not overlap with regular buttons! !
	m_buttonTexts[FMlib::Gamepad::eButton_pov_fwd] = "POV fwd";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_back] = "POV back";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_left] = "POV left";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_right] = "POV right";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_right_fwd] = "POV right fwd";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_right_back] = "POV right back";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_left_back] = "POV left back";
	m_buttonTexts[FMlib::Gamepad::eButton_pov_left_fwd] = "POV left fwd";
#endif
	QStringList buttons;
	ButtonTextMap::const_iterator iter = m_buttonTexts.begin();

	while (iter != m_buttonTexts.end())
	{
		buttons.push_back(iter->second);
		++iter;
	}

	ui->comboBox_next->addItems(buttons);
	ui->comboBox_prev->addItems(buttons);
	ui->comboBox_pause->addItems(buttons);
	ui->comboBox_reset->addItems(buttons);
	ui->comboBox_reset_2->addItems(buttons);
	ui->comboBox_reset_hold_first->addItems(buttons);
	ui->comboBox_reset_hold_second->addItems(buttons);
	ui->comboBox_hajime_mate->addItems(buttons);
	ui->comboBox_first_holding->addItems(buttons);
	ui->comboBox_second_holding->addItems(buttons);
	ui->comboBox_hansokumake_first->addItems(buttons);
	ui->comboBox_hansokumake_second->addItems(buttons);

	// num screens
    int numScreens = QGuiApplication::screens().count();

    for (int i(1); i <= numScreens; ++i)
	{
        QRect res = QGuiApplication::screens().at(i - 1)->availableGeometry();

		ui->comboBox_screen->addItem(
			QString("%1 (%2x%3)").arg(
				QString::number(i),
				QString::number(res.width()),
				QString::number(res.height())
			));
	}

	QDir dir(QDir::currentPath() + "/sounds");
	QStringList nameFilters;
	nameFilters << "*.wav";
	dir.setNameFilters(nameFilters);
	QStringList sounds(dir.entryList(QDir::Files));
	ui->comboBox_sound_time_ends->addItems(sounds);


    // Editable keyboard layout, including genuine numeric keypad key sequences.
    const auto currentKeys = IpponboardKeyboard::load();
    auto* keyboardTable = ui->tableWidget_keys;
    keyboardTable->clear();
    keyboardTable->setRowCount(IpponboardKeyboard::commands().size());
    keyboardTable->setColumnCount(2);
    keyboardTable->setHorizontalHeaderLabels({
        QStringLiteral("AKTION AUF DER ANZEIGETAFEL"),
        QStringLiteral("TASTENKOMBINATION") });
    keyboardTable->setAlternatingRowColors(true);
    keyboardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    keyboardTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    keyboardTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    keyboardTable->verticalHeader()->setDefaultSectionSize(43);
    int keyboardRow = 0;
    for (const auto& command : IpponboardKeyboard::commands()) {
        const QString id = QString::fromLatin1(command.id);
        auto* label = new QTableWidgetItem(command.label);
        label->setFlags(label->flags() & ~Qt::ItemIsEditable);
        label->setData(Qt::UserRole, id);
        keyboardTable->setItem(keyboardRow, 0, label);
        auto* sequence = new QKeySequenceEdit(currentKeys.value(id, command.defaultKey), keyboardTable);
        sequence->setToolTip(QStringLiteral("Hier klicken und die gewünschte Taste drücken (auch Numpad)."));
        keyboardTable->setCellWidget(keyboardRow, 1, sequence);
        ++keyboardRow;
    }
    auto* defaultsButton = new QPushButton(QStringLiteral("Standardbelegung wiederherstellen"), ui->tab_keys);
    defaultsButton->setObjectName(QStringLiteral("keyboardDefaultsButton"));
    if (auto* keyboardLayout = qobject_cast<QVBoxLayout*>(ui->tab_keys->layout()))
        keyboardLayout->insertWidget(0, defaultsButton);
    connect(defaultsButton, &QPushButton::clicked, this, [keyboardTable]() {
        const auto defaults = IpponboardKeyboard::defaults();
        for (int row=0; row<keyboardTable->rowCount(); ++row) {
            const QString id = keyboardTable->item(row,0)->data(Qt::UserRole).toString();
            auto* editor = qobject_cast<QKeySequenceEdit*>(keyboardTable->cellWidget(row,1));
            if (editor) editor->setKeySequence(defaults.value(id));
        }
    });

	ui->tableWidget_keys->verticalHeader()->setVisible(false);

	// NOTE: This is nasty workaround for the standard buttons not
	// beeing translated (separate translator would be required)
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

SettingsDlg::~SettingsDlg()
{
	delete ui;
}


void SettingsDlg::SetScreensSettings(int screen, const QSize&)
{
	Q_ASSERT(screen < ui->comboBox_screen->count());
	ui->comboBox_screen->setCurrentIndex(screen);

	// Ipponboard-Meschede always uses a true fullscreen mat display.
	ui->checkBox_secondary_view_custom_size->setChecked(false);
	ui->checkBox_secondary_view_custom_size->setEnabled(false);
	ui->lineEdit_fixedsize_width->setText("0");
	ui->lineEdit_fixedsize_height->setText("0");
	ui->lineEdit_fixedsize_width->setEnabled(false);
	ui->lineEdit_fixedsize_height->setEnabled(false);
	ui->label_screen_width->setEnabled(false);
	ui->label_screen_height->setEnabled(false);
}

void SettingsDlg::SetInfoHeaderSettings(const QFont& font,
										const QColor& color,
										const QColor& background)
{
	ui->fontComboBox_infoHeader->setCurrentFont(font);
	ui->text_text_sample->SetFontAndColor(font, color, background);
	ui->checkBox_text_bold->setChecked(font.bold());
	ui->checkBox_text_italic->setChecked(font.italic());
}

void SettingsDlg::SetFighterNameFont(const QFont& font)
{
	ui->fontComboBox_fighters->setCurrentFont(font);
	ui->checkBox_fighters_bold->setChecked(font.bold());
	ui->checkBox_fighters_italic->setChecked(font.italic());
	ui->text_color_first->SetFont(font);
	ui->text_color_second->SetFont(font);
}

void SettingsDlg::SetTextColorsFirst(const QColor& color, const QColor& background)
{
	ui->text_color_first->SetColor(color, background);
}

void SettingsDlg::SetTextColorsSecond(const QColor& color, const QColor& background)
{
	ui->text_color_second->SetColor(color, background);
}

void SettingsDlg::SetLabels(QString const& mat, QString const& home, QString const& guest)
{
	const int index = ui->comboBox_mat->findText(mat);

	if (-1 != index)
	{
		ui->comboBox_mat->setCurrentIndex(index);
	}
	else
	{
		ui->comboBox_mat->setEditText(mat);
	}

	ui->text_text_sample->SetText(mat);

	ui->lineEdit_labelHome->setText(home);
	ui->lineEdit_labelGuest->setText(guest);
}

void SettingsDlg::SetGongFile(const QString& path)
{
	int pos = path.lastIndexOf('/', -1);
	QString filename;

	if (-1 != pos)
		filename = path.right(path.length() - pos - 1);
	else
		filename = path;

	const int index = ui->comboBox_sound_time_ends->findText(filename);

	if (-1 != index)
		ui->comboBox_sound_time_ends->setCurrentIndex(index);
}

int SettingsDlg::GetSelectedScreen() const
{
	return ui->comboBox_screen->currentIndex();
}

QSize SettingsDlg::GetSize() const
{
	return QSize();
}

QFont SettingsDlg::GetInfoHeaderFont() const
{
	QFont f = ui->fontComboBox_infoHeader->currentFont();
	f.setBold(ui->checkBox_text_bold->isChecked());
	f.setItalic(ui->checkBox_text_italic->isChecked());
	return f;
}

QFont SettingsDlg::GetFighterNameFont() const
{
	QFont f = ui->fontComboBox_fighters->currentFont();
	f.setBold(ui->checkBox_fighters_bold->isChecked());
	f.setItalic(ui->checkBox_fighters_italic->isChecked());
	return f;
}

QColor SettingsDlg::GetInfoTextColor() const
{
	return ui->text_text_sample->GetColor();
}

QColor SettingsDlg::GetTextColorFirst() const
{
	return ui->text_color_first->GetColor();
}

QColor SettingsDlg::GetTextColorSecond() const
{
	return ui->text_color_second->GetColor();
}

QColor SettingsDlg::GetInfoTextBgColor() const
{
	return ui->text_text_sample->GetBgColor();
}
QColor SettingsDlg::GetTextBgColorFirst() const
{
	return ui->text_color_first->GetBgColor();
}
QColor SettingsDlg::GetTextBgColorSecond() const
{
	return ui->text_color_second->GetBgColor();
}

QString SettingsDlg::GetMatLabel() const
{
	return ui->comboBox_mat->currentText();
}

QString SettingsDlg::GetHomeLabel() const
{
	return ui->lineEdit_labelHome->text();
}

QString SettingsDlg::GetGuestLabel() const
{
	return ui->lineEdit_labelGuest->text();
}

QString SettingsDlg::GetGongFile() const
{
	QString path = QDir::currentPath() + "/sounds/";
	return path + ui->comboBox_sound_time_ends->currentText();
}

void SettingsDlg::on_buttonBox_accepted()
{
    QMap<QString, QKeySequence> bindings;
    QMap<QString, QString> occupied;
    for (int row=0; row<ui->tableWidget_keys->rowCount(); ++row) {
        const auto* item = ui->tableWidget_keys->item(row,0);
        const auto* editor = qobject_cast<QKeySequenceEdit*>(ui->tableWidget_keys->cellWidget(row,1));
        if (!item || !editor) continue;
        const QKeySequence sequence = editor->keySequence();
        const QString id = item->data(Qt::UserRole).toString();
        if (sequence.count() > 1) {
            QMessageBox::warning(this, QStringLiteral("Tastatur"), QStringLiteral(
                "Pro Funktion ist nur eine Tastenkombination zulässig."));
            return;
        }
        const QString key = sequence.toString(QKeySequence::PortableText);
        if (!key.isEmpty()) {
            const QList<QKeySequence> reserved = {
                QKeySequence(Qt::Key_F1), QKeySequence(Qt::ALT | Qt::Key_F4),
                QKeySequence(Qt::CTRL | Qt::Key_O), QKeySequence(Qt::CTRL | Qt::Key_S),
                QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S)
            };
            if (reserved.contains(sequence)) {
                QMessageBox::warning(this, QStringLiteral("Tastatur"),
                    QStringLiteral("Diese Taste ist für die Hilfe, Dateiaktionen oder Windows reserviert: %1").arg(key));
                return;
            }
            if (occupied.contains(key)) {
                QMessageBox::warning(this, QStringLiteral("Doppelte Tastenbelegung"),
                    QStringLiteral("%1 ist bereits für „%2“ belegt.").arg(key, occupied.value(key)));
                return;
            }
            occupied.insert(key, item->text());
        }
        bindings.insert(id, sequence);
    }
    QString error;
    if (!IpponboardKeyboard::save(bindings, error)) {
        QMessageBox::critical(this, QStringLiteral("Tastatur konnte nicht gespeichert werden"), error);
        return;
    }
    accept();
}

void SettingsDlg::on_buttonBox_rejected()
{
	reject();
}

void SettingsDlg::SetControllerConfig(const ControllerConfig* pConfig)
{
	Q_ASSERT(pConfig);

	if (pConfig)
	{
		set_button_value(ui->comboBox_hajime_mate, pConfig->button_hajime_mate);
		set_button_value(ui->comboBox_reset, pConfig->button_reset);
		set_button_value(ui->comboBox_reset_2, pConfig->button_reset_2);
		set_button_value(ui->comboBox_next, pConfig->button_next);
		set_button_value(ui->comboBox_prev, pConfig->button_prev);
		set_button_value(ui->comboBox_pause, pConfig->button_pause);
		set_button_value(ui->comboBox_first_holding, pConfig->button_osaekomi_toketa_first);
		set_button_value(ui->comboBox_second_holding, pConfig->button_osaekomi_toketa_second);
		set_button_value(ui->comboBox_reset_hold_first, pConfig->button_reset_hold_first);
		set_button_value(ui->comboBox_reset_hold_second, pConfig->button_reset_hold_second);
		set_button_value(ui->comboBox_hansokumake_first, pConfig->button_hansokumake_first);
		set_button_value(ui->comboBox_hansokumake_second, pConfig->button_hansokumake_second);

		ui->checkBox_invert_x_axis->setChecked(pConfig->axis_inverted_X);
		ui->checkBox_invert_y_axis->setChecked(pConfig->axis_inverted_Y);
		ui->checkBox_invert_r_axis->setChecked(pConfig->axis_inverted_R);
		ui->checkBox_invert_z_axis->setChecked(pConfig->axis_inverted_Z);
	}
}

void SettingsDlg::GetControllerConfig(ControllerConfig* pConfig)
{
	Q_ASSERT(pConfig);

	if (pConfig)
	{
		pConfig->button_hajime_mate =
			get_button_from_text(ui->comboBox_hajime_mate->currentText());

		pConfig->button_reset =
			get_button_from_text(ui->comboBox_reset->currentText());

		pConfig->button_reset_2 =
			get_button_from_text(ui->comboBox_reset_2->currentText());

		pConfig->button_next =
			get_button_from_text(ui->comboBox_next->currentText());

		pConfig->button_prev =
			get_button_from_text(ui->comboBox_prev->currentText());

		pConfig->button_pause =
			get_button_from_text(ui->comboBox_pause->currentText());

		pConfig->button_osaekomi_toketa_first =
			get_button_from_text(ui->comboBox_first_holding->currentText());

		pConfig->button_osaekomi_toketa_second =
			get_button_from_text(ui->comboBox_second_holding->currentText());

		pConfig->button_reset_hold_first =
			get_button_from_text(ui->comboBox_reset_hold_first->currentText());

		pConfig->button_reset_hold_second =
			get_button_from_text(ui->comboBox_reset_hold_second->currentText());

		pConfig->button_hansokumake_first =
			get_button_from_text(ui->comboBox_hansokumake_first->currentText());

		pConfig->button_hansokumake_second =
			get_button_from_text(ui->comboBox_hansokumake_second->currentText());

		pConfig->axis_inverted_X = ui->checkBox_invert_x_axis->isChecked();
		pConfig->axis_inverted_Y = ui->checkBox_invert_y_axis->isChecked();
		pConfig->axis_inverted_R = ui->checkBox_invert_r_axis->isChecked();
		pConfig->axis_inverted_Z = ui->checkBox_invert_z_axis->isChecked();
	}
}


//---------------------------------------------------------
void SettingsDlg::changeEvent(QEvent* e)
//---------------------------------------------------------
{
	QDialog::changeEvent(e);

	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;

	default:
		break;
	}
}

void Ipponboard::SettingsDlg::on_toolButton_play_gong_pressed()
{
	QString path = QDir::currentPath() + "/sounds/" +
				   ui->comboBox_sound_time_ends->currentText();
	qDebug() << "Playing sound from file:" << path;
	QSound::play(path);
}

void Ipponboard::SettingsDlg::on_fontComboBox_infoHeader_currentFontChanged(
	QFont f)
{
	f.setBold(ui->checkBox_text_bold->isChecked());
	f.setItalic(ui->checkBox_text_italic->isChecked());
	ui->text_text_sample->SetFont(f);
}

void Ipponboard::SettingsDlg::on_fontComboBox_fighters_currentFontChanged(
	QFont f)
{
	f.setBold(ui->checkBox_fighters_bold->isChecked());
	f.setItalic(ui->checkBox_fighters_italic->isChecked());
	ui->text_color_first->SetFont(f);
	ui->text_color_second->SetFont(f);
}

void Ipponboard::SettingsDlg::on_toolButton_text_color_pressed()
{
	QColor col(ui->text_text_sample->GetColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_text_sample->SetColor(col);
}

void Ipponboard::SettingsDlg::on_toolButton_text_background_pressed()
{
	QColor col(ui->text_text_sample->GetBgColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_text_sample->SetColor(ui->text_text_sample->GetColor(), col);
}

void Ipponboard::SettingsDlg::on_toolButton_text_color_first_pressed()
{
	QColor col(ui->text_color_first->GetColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_color_first->SetColor(col);
}

void Ipponboard::SettingsDlg::on_toolButton_text_background_first_pressed()
{
	QColor col(ui->text_color_first->GetBgColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_color_first->SetColor(ui->text_color_first->GetColor(), col);
}

void Ipponboard::SettingsDlg::on_toolButton_text_color_second_pressed()
{
	QColor col(ui->text_color_second->GetColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_color_second->SetColor(col);
}

void Ipponboard::SettingsDlg::on_toolButton_text_background_second_pressed()
{
	QColor col(ui->text_color_second->GetBgColor());
	col = QColorDialog::getColor(col, this);

	if (col.isValid())
		ui->text_color_second->SetColor(ui->text_color_second->GetColor(), col);
}

void Ipponboard::SettingsDlg::on_checkBox_text_bold_toggled(bool checked)
{
	QFont f = ui->text_text_sample->font();
	f.setBold(checked);
	ui->text_text_sample->SetFont(f);

}

void Ipponboard::SettingsDlg::on_checkBox_text_italic_toggled(bool checked)
{
	QFont f = ui->text_text_sample->font();
	f.setItalic(checked);
	ui->text_text_sample->SetFont(f);
}

void Ipponboard::SettingsDlg::on_checkBox_fighters_bold_toggled(bool checked)
{
	QFont f = ui->text_color_first->font();
	f.setBold(checked);
	ui->text_color_first->SetFont(f);
	ui->text_color_second->SetFont(f);
}

void Ipponboard::SettingsDlg::on_checkBox_fighters_italic_toggled(bool checked)
{
	QFont f = ui->text_color_first->font();
	f.setItalic(checked);
	ui->text_color_first->SetFont(f);
	ui->text_color_second->SetFont(f);
}

void Ipponboard::SettingsDlg::on_comboBox_mat_editTextChanged(QString text)
{
	ui->text_text_sample->SetText(text);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int SettingsDlg::get_button_from_text(const QString& text) const
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
	ButtonTextMap::const_iterator iter = m_buttonTexts.begin();

	while (iter != m_buttonTexts.end())
	{
		if (iter->second == text)
			return iter->first;

		++iter;
	}

	Q_ASSERT(!"element should be in list");
	return -1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Ipponboard::SettingsDlg::set_button_value(QComboBox* pCombo, int buttonId)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
	int index = pCombo->findText(m_buttonTexts[buttonId]);
	pCombo->setCurrentIndex(index);
}

void Ipponboard::SettingsDlg::on_checkBox_secondary_view_custom_size_toggled(bool checked)
{
	ui->lineEdit_fixedsize_width->setEnabled(checked);
	ui->lineEdit_fixedsize_height->setEnabled(checked);
	ui->label_screen_width->setEnabled(checked);
	ui->label_screen_height->setEnabled(checked);

	if (!checked)
	{
		ui->lineEdit_fixedsize_width->setText("0");
		ui->lineEdit_fixedsize_height->setText("0");
	}
}

