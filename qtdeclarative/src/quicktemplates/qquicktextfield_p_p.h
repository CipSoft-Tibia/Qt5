// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTEXTFIELD_P_P_H
#define QQUICKTEXTFIELD_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/private/qlazilyallocated_p.h>
#include <QtQuick/private/qquicktextinput_p_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickTemplates2/private/qquickpresshandler_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include <QtQuickTemplates2/private/qquicktextfield_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickTextFieldPrivate : public QQuickTextInputPrivate, public QQuickItemChangeListener
#if QT_CONFIG(accessibility)
    , public QAccessible::ActivationObserver
#endif
{
public:
    Q_DECLARE_PUBLIC(QQuickTextField)

    QQuickTextFieldPrivate();
    ~QQuickTextFieldPrivate();

    static QQuickTextFieldPrivate *get(QQuickTextField *item) {
        return static_cast<QQuickTextFieldPrivate *>(QObjectPrivate::get(item)); }

    inline QMarginsF getInset() const { return QMarginsF(getLeftInset(), getTopInset(), getRightInset(), getBottomInset()); }
    inline qreal getTopInset() const { return extra.isAllocated() ? extra->topInset : 0; }
    inline qreal getLeftInset() const { return extra.isAllocated() ? extra->leftInset : 0; }
    inline qreal getRightInset() const { return extra.isAllocated() ? extra->rightInset : 0; }
    inline qreal getBottomInset() const { return extra.isAllocated() ? extra->bottomInset : 0; }

    void setTopInset(qreal value, bool reset = false);
    void setLeftInset(qreal value, bool reset = false);
    void setRightInset(qreal value, bool reset = false);
    void setBottomInset(qreal value, bool reset = false);

    void resizeBackground();

    void resolveFont();
    void inheritFont(const QFont &font);
    void updateFont(const QFont &font);
    inline void setFont_helper(const QFont &font) {
        if (sourceFont.resolveMask() == font.resolveMask() && sourceFont == font)
            return;
        updateFont(font);
    }

#if QT_CONFIG(quicktemplates2_hover)
    void updateHoverEnabled(bool h, bool e);
#endif

    qreal getImplicitWidth() const override;
    qreal getImplicitHeight() const override;

    void implicitWidthChanged() override;
    void implicitHeightChanged() override;

    void readOnlyChanged(bool isReadOnly);
    void echoModeChanged(QQuickTextField::EchoMode echoMode);

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
#endif

    void cancelBackground();
    void executeBackground(bool complete = false);

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    QPalette defaultPalette() const override;

#if QT_CONFIG(quicktemplates2_hover)
    bool hovered = false;
    bool explicitHoverEnabled = false;
#endif

    struct ExtraData {
        bool hasTopInset = false;
        bool hasLeftInset = false;
        bool hasRightInset = false;
        bool hasBottomInset = false;
        bool hasBackgroundWidth = false;
        bool hasBackgroundHeight = false;
        qreal topInset = 0;
        qreal leftInset = 0;
        qreal rightInset = 0;
        qreal bottomInset = 0;
        QFont requestedFont;
    };
    QLazilyAllocated<ExtraData> extra;

    bool resizingBackground = false;
    QQuickDeferredPointer<QQuickItem> background;
    QString placeholder;
    QColor placeholderColor;
    Qt::FocusReason focusReason = Qt::OtherFocusReason;
    QQuickPressHandler pressHandler;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTFIELD_P_P_H
