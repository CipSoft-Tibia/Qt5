// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INPLACE_EDITOR_H
#define INPLACE_EDITOR_H

#include <textpropertyeditor_p.h>
#include <shared_enums_p.h>

#include "inplace_widget_helper.h"
#include <qdesigner_utils_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE


class QDesignerFormWindowInterface;

namespace qdesigner_internal {

class InPlaceEditor: public TextPropertyEditor
{
    Q_OBJECT
public:
    InPlaceEditor(QWidget *widget,
                  TextPropertyValidationMode validationMode,
                  QDesignerFormWindowInterface *fw,
                  const QString& text,
                  const QRect& r);
private:
    InPlaceWidgetHelper m_InPlaceWidgetHelper;
};

// Base class for inline editor helpers to be embedded into a task menu.
// Inline-edits a property on a multi-selection.
// To use it for a particular widget/property, overwrite the method
// returning the edit area.

class TaskMenuInlineEditor : public QObject
{
    Q_OBJECT

public slots:
    void editText();

private slots:
    void updateText(const QString &text);
    void updateSelection();

protected:
    TaskMenuInlineEditor(QWidget *w, TextPropertyValidationMode vm, const QString &property, QObject *parent);
    // Overwrite to return the area for the inline editor.
    virtual QRect editRectangle() const = 0;
    QWidget *widget() const { return m_widget; }

private:
    const TextPropertyValidationMode m_vm;
    const QString m_property;
    QWidget *m_widget;
    QPointer<QDesignerFormWindowInterface> m_formWindow;
    QPointer<InPlaceEditor> m_editor;
    bool m_managed;
    qdesigner_internal::PropertySheetStringValue m_value;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // INPLACE_EDITOR_H
