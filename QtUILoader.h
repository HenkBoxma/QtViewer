/*
 * NOTE: This library is based on the Qt Linguist 5.1 (LGPL 2.1) sources.
 *  Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 * 
 * Copyright (C) 2014 CPA Software Engineering UG 
 * 
 * Website: http://www.cpawelzik.com
 * E-Mail:  christopher@cpawelzik.com
 *
 * This file is part of QtViewer.
 *
 * QtViewer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * QtViewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with QtViewer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Change history
 * ==============
 *
 * 16 Jan 2014:   Initial version based on \tools\linguist\linguist\formpreviewview.h
 *
 */

#pragma once

#include <QtUiTools\QUiLoader>
#include <QtCore\QFile>
#include <QtWidgets\QApplication>
#include <QtWidgets\QWidget>
#include <QtWidgets\QTabWidget>
#include <QtWidgets\QMainWindow>
#include <QtCore\QHash>
#include <QtCore\QList>

#include <QtUiTools\quiloader_p.h>

#include "UIWindowInfo.h"
#include "HitTestResult.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;

class QComboBox;
class QListWidgetItem;
class QGridLayout;
class QMdiArea;
class QMdiSubWindow;
class QToolBox;
class QTableWidgetItem;
class QTreeWidgetItem;

const QUiItemRolePair qUiItemRoles[] = {
    { Qt::DisplayRole, Qt::DisplayPropertyRole },
#ifndef QT_NO_TOOLTIP
    { Qt::ToolTipRole, Qt::ToolTipPropertyRole },
#endif
#ifndef QT_NO_STATUSTIP
    { Qt::StatusTipRole, Qt::StatusTipPropertyRole },
#endif
#ifndef QT_NO_WHATSTHIS
    { Qt::WhatsThisRole, Qt::WhatsThisPropertyRole },
#endif
    { -1 , -1 }
};

enum TranslatableEntryType {
    TranslatableProperty,
    TranslatableToolItemText,
    TranslatableToolItemToolTip,
    TranslatableTabPageText,
    TranslatableTabPageToolTip,
    TranslatableTabPageWhatsThis,
    TranslatableListWidgetItem,
    TranslatableTableWidgetItem,
    TranslatableTreeWidgetItem,
    TranslatableComboBoxItem
};

struct TranslatableEntry {
    TranslatableEntryType type;
    union {
        QObject *object;
        QComboBox *comboBox;
        QTabWidget *tabWidget;
        QToolBox *toolBox;
        QListWidgetItem *listWidgetItem;
        QTableWidgetItem *tableWidgetItem;
        QTreeWidgetItem *treeWidgetItem;
    } target;
    union {
        char *name;
        int index;
        struct {
            short index; // Known to be below 1000
            short column;
        } treeIndex;
    } prop;
};

typedef QHash<QUiTranslatableStringValue, QList<TranslatableEntry> > TargetsHash;

// ================= New code ===================

struct TabInfo {
  QTabWidget *tabWidget; 
  int tabIndex; 
}; 

typedef QHash<QObject*, TabInfo> TabInfoTable; 

namespace QtViewer {
	
	public ref class QtUILoader
	{	
	public:
		UIWindowInfo^ Render(String^ xml); 
    void Translate(String^ source, String^ comment, String^ translation);     
    void Refresh();
    HitTestResult^ HitTest(int x, int y); 
    List<System::Drawing::Rectangle>^ GetWidgetBounds(String^ source, String^ comment);
	private: 
    ~QtUILoader(); 
    static QtUILoader(); 		
    void buildTargets(QObject *o, TargetsHash *targets, TabInfoTable *tabInfoTable);

		QWidget* m_form; 
    QMainWindow* m_mainWindow; 
		static QApplication* m_app;         
    TargetsHash* m_targets; 
    TabInfoTable* m_tabInfoTable; 
	};
}
