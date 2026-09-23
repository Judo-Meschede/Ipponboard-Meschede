// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.


#ifndef __BASE_COMBOBOXDELEGATE_H
#define __BASE_COMBOBOXDELEGATE_H

#include "../util/helpers.hpp"

#include <QItemDelegate>
#include <QStringList>
#include <functional>
#include <utility>

// forwards
class QComboBox;

class ComboBoxDelegate : public QItemDelegate
{
public:
	using ItemProvider = std::function<std::pair<QStringList, QStringList>(const QModelIndex&)>;
	ComboBoxDelegate(QObject* parent);

	virtual QWidget* createEditor(
		QWidget* parent,
		const QStyleOptionViewItem& /* option */,
		const QModelIndex& /* index */) const override;

	virtual void setEditorData(
		QWidget* editor,
		const QModelIndex& index) const override;

	virtual void setModelData(
		QWidget* editor,
		QAbstractItemModel* model,
		const QModelIndex& index) const override;

	virtual void updateEditorGeometry(
		QWidget* editor,
		const QStyleOptionViewItem& option,
		const QModelIndex& /* index */) const override;

	void SetItems(QStringList const& items, QStringList const& itemIds = QStringList());
	void SetItemProvider(ItemProvider provider);

private:
	QStringList m_items;
	QStringList m_itemIds;
	ItemProvider m_itemProvider;
};

#endif // __BASE_COMBOBOXDELEGATE_H
