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
 * 16 Jan 2014:   Initial version based on \tools\linguist\linguist\formpreviewview.cpp
 * 28 Jan 2014:   Open tab pages and show selected strings in combo boxes. 
 *
 */

#include "stdafx.h"
#include "QtUILoader.h"

#include <QtWidgets\QAction>
#include <QtWidgets\QApplication>
#include <QtWidgets\QFontComboBox>
#include <QtWidgets\QFrame>
#include <QtWidgets\QGridLayout>
#include <QtWidgets\QListWidget>
#include <QtWidgets\QMdiArea>
#include <QtWidgets\QMdiSubWindow>
#include <QtWidgets\QMenu>
#include <QtWidgets\QTableWidget>
#include <QtWidgets\QTabWidget>
#include <QtWidgets\QToolBox>
#include <QtWidgets\QTreeWidget>

#include <QtUITools\quiloader.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace QtViewer; 
using namespace System; 
using namespace System::Runtime::InteropServices;

#if defined(Q_CC_SUN) || defined(Q_CC_HPACC) || defined(Q_CC_XLC)
int qHash(const QUiTranslatableStringValue &tsv)
#else
static int qHash(const QUiTranslatableStringValue &tsv)
#endif
{
  return qHash(tsv.value()) ^ qHash(tsv.comment());
}

static bool operator==(const QUiTranslatableStringValue &tsv1, const QUiTranslatableStringValue &tsv2)
{
  return tsv1.value() == tsv2.value() && tsv1.comment() == tsv2.comment();
}

std::wstring MarshalString(String^ s)
{
  using namespace System::Runtime::InteropServices;
  std::wstring ret;
  IntPtr p = Marshal::StringToHGlobalUni(s);
  if (p.ToPointer())
  {
      ret.assign((wchar_t const*)p.ToPointer());
      Marshal::FreeHGlobal(p);
  }
  return ret;
}

QString SystemStringToQt(System::String^ str)
{
  std::wstring t = MarshalString(str);    
  QString r = QString::fromUtf16((const ushort*)t.c_str());
  return r;
}

String^ QtToSystemString(QString * str)
{
  wchar_t * wcar = (wchar_t *)str->utf16();
  String^ r = gcnew String(wcar);
  return r;
}

#define INSERT_TARGET(_tsv, _type, _target, _prop) \
{ \
  target.type = _type; \
  target.target._target; \
  target.prop._prop; \
  (*targets)[qvariant_cast<QUiTranslatableStringValue>(_tsv)].append(target); \
} 

static void registerTreeItem(QTreeWidgetItem *item, TargetsHash *targets)
{
  const QUiItemRolePair *irs = qUiItemRoles;

  int cnt = item->columnCount();
  for (int i = 0; i < cnt; ++i) {
    for (unsigned j = 0; irs[j].shadowRole >= 0; j++) {
      QVariant v = item->data(i, irs[j].shadowRole);
      if (v.isValid()) {
        TranslatableEntry target;
        target.prop.treeIndex.column = i;
        INSERT_TARGET(v, TranslatableTreeWidgetItem, treeWidgetItem = item, treeIndex.index = j);
      }
    }
  }

  cnt = item->childCount();
  for (int j = 0; j < cnt; ++j)
    registerTreeItem(item->child(j), targets);
}

#define REGISTER_ITEM_CORE(item, propType, targetName) \
  const QUiItemRolePair *irs = qUiItemRoles; \
  for (unsigned j = 0; irs[j].shadowRole >= 0; j++) { \
  QVariant v = item->data(irs[j].shadowRole); \
  if (v.isValid()) \
  INSERT_TARGET(v, propType, targetName = item, index = j); \
  }

static void registerListItem(QListWidgetItem *item, TargetsHash *targets)
{
  TranslatableEntry target;
  REGISTER_ITEM_CORE(item, TranslatableListWidgetItem, listWidgetItem);
}

static void registerTableItem(QTableWidgetItem *item, TargetsHash *targets)
{
  if (!item)
    return;

  TranslatableEntry target;
  REGISTER_ITEM_CORE(item, TranslatableTableWidgetItem, tableWidgetItem);
}

#define REGISTER_SUBWIDGET_PROP(mainWidget, propType, propName) \
  do { \
  QVariant v = mainWidget->widget(i)->property(propName); \
  if (v.isValid()) \
  INSERT_TARGET(v, propType, object = mainWidget, index = i); \
  } while (0)

static QtUILoader::QtUILoader() 
{
  if(!m_app) 
  {
    int cargs = 0; 
    m_app = new QApplication(cargs, NULL);
  }
}

static void destroyTargets(TargetsHash *targets)
{
    for (TargetsHash::Iterator it = targets->begin(), end = targets->end(); it != end; ++it)
        foreach (const TranslatableEntry &target, *it)
            if (target.type == TranslatableProperty)
                delete target.prop.name;
    targets->clear();
}

QtUILoader::~QtUILoader() 
{
  if(m_targets) 
  {
    destroyTargets(m_targets); 
  }
  if(m_form) 
  {
    delete m_form; 
  }
  if(m_tabInfoTable) 
  {
    m_tabInfoTable->clear();     
  }
  if(m_mainWindow) 
  {
    delete m_mainWindow; 
  }
}

UIWindowInfo^ QtUILoader::Render(String^ path) 
{
  QString fileName = SystemStringToQt(path); 
  QFile file(fileName);

  static QUiLoader *uiLoader;
  if(!uiLoader) 
  {		 
    uiLoader = new QUiLoader(NULL);
    uiLoader->setLanguageChangeEnabled(true);
    uiLoader->setTranslationEnabled(false);
  }

  m_form = uiLoader->load(&file);		
  if(!m_form) 
  {
    // Could not load form 
    return nullptr; 
  }

  m_targets = new TargetsHash(); 
  m_tabInfoTable = new TabInfoTable(); 

  buildTargets(m_form, m_targets, m_tabInfoTable); 
  
  m_form->setWindowFlags(Qt::Widget);
  m_form->setWindowModality(Qt::NonModal);
  m_form->setFocusPolicy(Qt::NoFocus);  
  m_form->show(); 

  m_mainWindow = new QMainWindow(NULL);
  m_mainWindow->setGeometry(0, 0, m_form->width(), m_form->height()); 
  m_mainWindow->setCentralWidget(m_form); 
  QString windowTitle = m_form->windowTitle(); 
  if(windowTitle.isEmpty()) 
    windowTitle = QString("Form"); 

  m_mainWindow->setWindowTitle(windowTitle); 
  m_mainWindow->show(); 

  UIWindowInfo^ info = gcnew UIWindowInfo(); 
  info->Handle = (IntPtr) (int) m_mainWindow->effectiveWinId();

  return info; 
}

static void registerTabWidgetChildren(QTabWidget *tabw, QObject* child, int index, TabInfoTable* tabInfoTable) 
{
  TabInfo info; 
  info.tabIndex = index; 
  info.tabWidget = tabw; 
  tabInfoTable->insert(child, info);
  foreach (QObject *co, child->children())
  {
    registerTabWidgetChildren(tabw, co, index, tabInfoTable);
  }
}

void QtUILoader::buildTargets(QObject *o, TargetsHash *targets, TabInfoTable* tabInfoTable)
{
  TranslatableEntry target;

  foreach (const QByteArray &prop, o->dynamicPropertyNames()) {
    if (prop.startsWith(PROP_GENERIC_PREFIX)) {
      const QByteArray propName = prop.mid(sizeof(PROP_GENERIC_PREFIX) - 1);
      INSERT_TARGET(o->property(prop),
        TranslatableProperty, object = o, name = qstrdup(propName.data()));
    }
  }

  if (QTabWidget *tabw = qobject_cast<QTabWidget*>(o)) {
    const int cnt = tabw->count();
    for (int i = 0; i < cnt; ++i) {
      registerTabWidgetChildren(tabw, tabw->widget(i), i, m_tabInfoTable); 
      REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageText, PROP_TABPAGETEXT);
      REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageToolTip, PROP_TABPAGETOOLTIP);
      REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageWhatsThis, PROP_TABPAGEWHATSTHIS);
    }
  } else if (QToolBox *toolw = qobject_cast<QToolBox*>(o)) {
    const int cnt = toolw->count();
    for (int i = 0; i < cnt; ++i) {
      REGISTER_SUBWIDGET_PROP(toolw, TranslatableToolItemText, PROP_TOOLITEMTEXT);
      REGISTER_SUBWIDGET_PROP(toolw, TranslatableToolItemToolTip, PROP_TOOLITEMTOOLTIP);
    }
  } else if (QComboBox *combow = qobject_cast<QComboBox*>(o)) {
    if (!qobject_cast<QFontComboBox*>(o)) {
      const int cnt = combow->count();
      for (int i = 0; i < cnt; ++i) {
        const QVariant v = combow->itemData(i, Qt::DisplayPropertyRole);
        if (v.isValid())
          INSERT_TARGET(v, TranslatableComboBoxItem, comboBox = combow, index = i);
      }
    }
  } else if (QListWidget *listw = qobject_cast<QListWidget*>(o)) {
    const int cnt = listw->count();
    for (int i = 0; i < cnt; ++i)
      registerListItem(listw->item(i), targets);
  } else if (QTableWidget *tablew = qobject_cast<QTableWidget*>(o)) {
    const int row_cnt = tablew->rowCount();
    const int col_cnt = tablew->columnCount();
    for (int j = 0; j < col_cnt; ++j)
      registerTableItem(tablew->verticalHeaderItem(j), targets);
    for (int i = 0; i < row_cnt; ++i) {
      registerTableItem(tablew->horizontalHeaderItem(i), targets);
      for (int j = 0; j < col_cnt; ++j)
        registerTableItem(tablew->item(i, j), targets);
    }
  } else if (QTreeWidget *treew = qobject_cast<QTreeWidget*>(o)) {
    if (QTreeWidgetItem *item = treew->headerItem())
      registerTreeItem(item, targets);
    const int cnt = treew->topLevelItemCount();
    for (int i = 0; i < cnt; ++i)
      registerTreeItem(treew->topLevelItem(i), targets);
  }

  foreach (QObject *co, o->children())
    buildTargets(co, targets, tabInfoTable);
}


static void retranslateTarget(const TranslatableEntry &target, const QString &text)
{
    switch (target.type) {
    case TranslatableProperty:
        target.target.object->setProperty(target.prop.name, text);
        break;
#ifndef QT_NO_TABWIDGET
    case TranslatableTabPageText:
        target.target.tabWidget->setTabText(target.prop.index, text);        
        break;
# ifndef QT_NO_TOOLTIP
    case TranslatableTabPageToolTip:
        target.target.tabWidget->setTabToolTip(target.prop.index, text);
        break;
# endif
# ifndef QT_NO_WHATSTHIS
    case TranslatableTabPageWhatsThis:
        target.target.tabWidget->setTabWhatsThis(target.prop.index, text);
        break;
# endif
#endif // QT_NO_TABWIDGET
#ifndef QT_NO_TOOLBOX
    case TranslatableToolItemText:
        target.target.toolBox->setItemText(target.prop.index, text);
        break;
# ifndef QT_NO_TOOLTIP
    case TranslatableToolItemToolTip:
        target.target.toolBox->setItemToolTip(target.prop.index, text);
        break;
# endif
#endif // QT_NO_TOOLBOX
#ifndef QT_NO_COMBOBOX
    case TranslatableComboBoxItem:
        target.target.comboBox->setItemText(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_LISTWIDGET
    case TranslatableListWidgetItem:
        target.target.listWidgetItem->setData(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_TABLEWIDGET
    case TranslatableTableWidgetItem:
        target.target.tableWidgetItem->setData(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_TREEWIDGET
    case TranslatableTreeWidgetItem:
        target.target.treeWidgetItem->setData(target.prop.treeIndex.column, target.prop.treeIndex.index, text);
        break;
#endif
    }
}

void QtUILoader::Translate(String^ source, String^ comment, String^ translation) 
{  
  if(!m_targets) 
  {
    // Render was not called yet, no hashtable 
    return; 
  }

  QString qs_source = SystemStringToQt(source);   
  QString qs_comment = SystemStringToQt(comment); 
  QString qs_translation = SystemStringToQt(translation); 
  
  QUiTranslatableStringValue tsv; 
  tsv.setValue(qs_source.toUtf8()); 
  tsv.setComment(qs_comment.toUtf8());   

  if(m_targets->contains(tsv)) 
  {
    QList<TranslatableEntry> targets = m_targets->value(tsv);  

    for(int i=0; i < targets.length(); i++) 
    {
      TranslatableEntry entry = targets.at(i);       
      retranslateTarget(entry, qs_translation); 
    }
  }    

}

void QtUILoader::Refresh() 
{
  if(m_form) 
  {
    HWND handle = (HWND) m_form->effectiveWinId(); 
    RedrawWindow(handle, NULL, NULL, RDW_INVALIDATE);      
  }
}


//TODO
HitTestResult^ QtUILoader::HitTest(int x, int y) 
{
  if(!m_app) 
  {
    return nullptr; 
  }
    
  QPoint pos = QCursor::pos(); 
  QWidget* widget = m_form->childAt(m_form->mapFromGlobal(pos)); 
  if(widget) 
  {
    HitTestResult^ hrs = gcnew HitTestResult(); 

    // Check if this widget is in our hashtable         
    QPoint parentPos = widget->mapFrom(m_form, QPoint(widget->x(), widget->y())); 

    hrs->WidgetRectangle.X = parentPos.x(); 
    hrs->WidgetRectangle.Y = parentPos.y(); 
    hrs->WidgetRectangle.Height = widget->height(); 
    hrs->WidgetRectangle.Width = widget->width(); 

    return hrs; 
  }
  else
  {
    return nullptr; 
  }
}

List<System::Drawing::Rectangle>^ QtUILoader::GetWidgetBounds(String^ source, String^ comment)
{
  if(!m_targets) 
  {
    // Render was not called yet, no hashtable 
    return nullptr; 
  }
  
  List<System::Drawing::Rectangle>^ list = gcnew List<System::Drawing::Rectangle>();   

  QString qs_source = SystemStringToQt(source);   
  QString qs_comment = SystemStringToQt(comment);   
  
  QUiTranslatableStringValue tsv; 
  tsv.setValue(qs_source.toUtf8()); 
  tsv.setComment(qs_comment.toUtf8());   
  
  HWND handle = (HWND)m_form->effectiveWinId();
  
  RECT windowRect; 
  RECT clientRect; 

  ::GetWindowRect(handle, &windowRect); 
  ::GetClientRect(handle, &clientRect); 

  // Qt coords are relative to (0,0) of the client area. 
  // The click panel covers the whole window. Therefore, we need to add the size of the window borders to the coords. 
  int deltaX = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left); 
  int deltaY = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top); 

  QPoint global00 = m_form->mapToGlobal(QPoint(0,0)); 

  if(m_targets->contains(tsv)) 
  {
    QList<TranslatableEntry> targets = m_targets->value(tsv);  

    for(int i=0; i < targets.length(); i++) 
    {
      TranslatableEntry entry = targets.at(i);       

      QWidget* widget = NULL; 

      if(entry.type == TranslatableTreeWidgetItem) 
      {
        // If the target is a QTreeWidgetItem, we select the parent control
        // QTreeWidgetItem's do not inherit from QObject. 
        QTreeWidgetItem* twi = (QTreeWidgetItem*) entry.target.object; 
        widget = twi->treeWidget(); 
      }
      else 
      {
        widget = qobject_cast<QWidget*>(entry.target.object);
      }

      if(!widget) 
        return nullptr; 

      // Coordinates are relative to the UIWindowHost window.
      QPoint widgetPos = widget->mapToGlobal(QPoint(0,0)); 

      int x = widgetPos.x() - global00.x() + (deltaX / 2) - 3; 
      int y = widgetPos.y() - global00.y() + (deltaY - (deltaX / 2)) - 3; 
      int h = widget->height() + 6;                 
      int w = widget->width() + 6; 

      list->Add(System::Drawing::Rectangle(x, y, w, h));       

      switch(entry.type) 
      {
        case TranslatableTabPageText: 
          // change active tab page
          entry.target.tabWidget->setCurrentIndex(entry.prop.index); 
          break; 
        case TranslatableComboBoxItem: 
          // show combo box item 
          entry.target.comboBox->setCurrentIndex(entry.prop.index); 
          break; 
      }
            
      if(m_tabInfoTable->contains(entry.target.object))
      {
        // if this widget is a child of a tab page, open the tab page. 
        TabInfo tabInfo = m_tabInfoTable->value(entry.target.object); 
        tabInfo.tabWidget->setCurrentIndex(tabInfo.tabIndex); 
      }
    }
  }    

  return list; 
}