// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "ComboBoxDelegate.h"
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

ComboBoxDelegate::ComboBoxDelegate(QObject* parent)
	: QItemDelegate(parent)
	, m_items()
	, m_itemIds()
{
}

QWidget* ComboBoxDelegate::createEditor(
	QWidget* parent,
	const QStyleOptionViewItem& /*option*/,
	const QModelIndex& index) const
{
	QComboBox* editor = new QComboBox(parent);
	editor->setEditable(true);
	editor->setInsertPolicy(QComboBox::NoInsert);
	editor->setMaxVisibleItems(18);

	for (int i = 0; i < m_items.size(); ++i)
	{
		const QVariant id = i < m_itemIds.size() ? QVariant(m_itemIds.at(i)) : QVariant();
		editor->addItem(m_items.at(i), id);
	}

	if (QCompleter* completer = editor->completer())
	{
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
		completer->setCompletionMode(QCompleter::PopupCompletion);
	}

	const QString currentText = index.model()->data(index, Qt::DisplayRole).toString();
	editor->setCurrentIndex(-1);
	editor->setEditText(currentText);
	if (editor->lineEdit())
		editor->lineEdit()->selectAll();

	return editor;
}

void ComboBoxDelegate::setEditorData(
	QWidget* editor,
	const QModelIndex& index) const
{
	QComboBox* comboBox = static_cast<QComboBox*>(editor);
	const QString value = index.model()->data(index, Qt::EditRole).toString();
	comboBox->setCurrentIndex(-1);
	comboBox->setEditText(value);
	if (comboBox->lineEdit())
		comboBox->lineEdit()->selectAll();
}

void ComboBoxDelegate::setModelData(
	QWidget* editor,
	QAbstractItemModel* model,
	const QModelIndex& index) const
{
	QComboBox* comboBox = static_cast<QComboBox*>(editor);
	const QString text = comboBox->currentText().trimmed();
	model->setData(index, text, Qt::EditRole);

	const int exactIndex = comboBox->findText(text, Qt::MatchExactly);
	if (exactIndex >= 0 && exactIndex < m_itemIds.size() && !m_itemIds.at(exactIndex).isEmpty())
	{
		model->setData(index, m_itemIds.at(exactIndex), Qt::UserRole);
	}
	else
	{
		model->setData(index, QString(), Qt::UserRole);
	}
}

void ComboBoxDelegate::updateEditorGeometry(
	QWidget* editor,
	const QStyleOptionViewItem& option,
	const QModelIndex& /*index*/) const
{
	editor->setGeometry(option.rect);
}

void ComboBoxDelegate::SetItems(QStringList const& items, QStringList const& itemIds)
{
	m_items = items;
	m_itemIds = itemIds;
}
