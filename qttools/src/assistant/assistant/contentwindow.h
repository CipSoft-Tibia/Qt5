/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CONTENTWINDOW_H
#define CONTENTWINDOW_H

#include <QtCore/QUrl>
#include <QtCore/QModelIndex>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class QHelpEngine;
class QHelpContentItem;
class QHelpContentWidget;

class ContentWindow : public QWidget
{
    Q_OBJECT

public:
    ContentWindow();
    ~ContentWindow() override;

    bool syncToContent(const QUrl &url);
    void expandToDepth(int depth);

signals:
    void linkActivated(const QUrl &link);
    void escapePressed();

private slots:
    void showContextMenu(const QPoint &pos);
    void expandTOC();
    void itemClicked(const QModelIndex &index);

private:
    void focusInEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;

    QHelpContentWidget * const m_contentWidget;
    int m_expandDepth;
};

QT_END_NAMESPACE

#endif // CONTENTWINDOW_H
