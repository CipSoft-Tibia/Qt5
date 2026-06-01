// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequicktextedit_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickTextEdit::QAccessibleQuickTextEdit(QQuickTextEdit *textEdit)
    : QAccessibleQuickItem(textEdit)
{
}

void QAccessibleQuickTextEdit::removeSelection(int selectionIndex)
{
    if (selectionCount() == 1 && selectionIndex == 0) {
        const int cursorPos = textEdit()->cursorPosition();
        textEdit()->select(cursorPos, cursorPos);
    }
}

void QAccessibleQuickTextEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex == 0)
        textEdit()->select(startOffset, endOffset);
}

#endif // accessibility

QT_END_NAMESPACE
