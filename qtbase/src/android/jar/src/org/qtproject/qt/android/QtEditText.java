// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.graphics.Canvas;
import android.text.InputType;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.KeyEvent;

import org.qtproject.qt.android.QtInputConnection.QtInputConnectionListener;

public class QtEditText extends View
{
    int m_initialCapsMode = 0;
    int m_imeOptions = 0;
    int m_inputType = InputType.TYPE_CLASS_TEXT;
    boolean m_optionsChanged = false;
    QtInputConnection m_inputConnection = null;

    // input method hints - must be kept in sync with QTDIR/src/corelib/global/qnamespace.h
    private final int ImhHiddenText = 0x1;
    private final int ImhSensitiveData = 0x2;
    private final int ImhNoAutoUppercase = 0x4;
    private final int ImhPreferNumbers = 0x8;
    private final int ImhPreferUppercase = 0x10;
    private final int ImhPreferLowercase = 0x20;
    private final int ImhNoPredictiveText = 0x40;

    private final int ImhDate = 0x80;
    private final int ImhTime = 0x100;

    private final int ImhPreferLatin = 0x200;

    private final int ImhMultiLine = 0x400;

    private final int ImhDigitsOnly = 0x10000;
    private final int ImhFormattedNumbersOnly = 0x20000;
    private final int ImhUppercaseOnly = 0x40000;
    private final int ImhLowercaseOnly = 0x80000;
    private final int ImhDialableCharactersOnly = 0x100000;
    private final int ImhEmailCharactersOnly = 0x200000;
    private final int ImhUrlCharactersOnly = 0x400000;
    private final int ImhLatinOnly = 0x800000;

    private final QtInputConnectionListener m_qtInputConnectionListener;

    public QtEditText(Context context, QtInputConnectionListener listener)
    {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
        m_qtInputConnectionListener = listener;
    }

    private void setImeOptions(int imeOptions)
    {
        if (m_imeOptions == imeOptions)
            return;
        m_imeOptions = imeOptions;
        m_optionsChanged = true;
    }

    private void setInitialCapsMode(int initialCapsMode)
    {
        if (m_initialCapsMode == initialCapsMode)
            return;
        m_initialCapsMode = initialCapsMode;
        m_optionsChanged = true;
    }


    private void setInputType(int inputType)
    {
        if (m_inputType == inputType)
            return;
        m_inputType = inputType;
        m_optionsChanged = true;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
        outAttrs.inputType = m_inputType;
        outAttrs.imeOptions = m_imeOptions;
        outAttrs.initialCapsMode = m_initialCapsMode;
        m_inputConnection = new QtInputConnection(this,m_qtInputConnectionListener);
        return m_inputConnection;
    }

    @Override
    public boolean onCheckIsTextEditor ()
    {
        return true;
    }

    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event)
    {
        if (null != m_inputConnection)
            m_inputConnection.restartImmInput();

        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // DEBUG CODE
        // canvas.drawARGB(127, 255, 0, 255);
        super.onDraw(canvas);
    }


    public void setEditTextOptions(int enterKeyType, int inputHints)
    {
        int initialCapsMode = 0;
        int imeOptions = imeOptionsFromEnterKeyType(enterKeyType);
        int inputType = android.text.InputType.TYPE_CLASS_TEXT;

        if ((inputHints & (ImhPreferNumbers | ImhDigitsOnly | ImhFormattedNumbersOnly)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_NUMBER;
            if ((inputHints & ImhFormattedNumbersOnly) != 0) {
                inputType |= (android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL
                        | android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);
            }

            if ((inputHints & ImhHiddenText) != 0)
                inputType |= android.text.InputType.TYPE_NUMBER_VARIATION_PASSWORD;
        } else if ((inputHints & ImhDialableCharactersOnly) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_PHONE;
        } else if ((inputHints & (ImhDate | ImhTime)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_DATETIME;
            if ((inputHints & (ImhDate | ImhTime)) != (ImhDate | ImhTime)) {
                if ((inputHints & ImhDate) != 0)
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_DATE;
                else
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_TIME;
            } // else {  TYPE_DATETIME_VARIATION_NORMAL(0) }
        } else { // CLASS_TEXT
            if ((inputHints & ImhHiddenText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
            } else if ((inputHints & ImhSensitiveData) != 0 ||
                    isDisablePredictiveTextWorkaround(inputHints)) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
            } else if ((inputHints & ImhUrlCharactersOnly) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_URI;
                if (enterKeyType == 0) // not explicitly overridden
                    imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
            } else if ((inputHints & ImhEmailCharactersOnly) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
            }

            if ((inputHints & ImhMultiLine) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;
                // Clear imeOptions for Multi-Line Type
                // User should be able to insert new line in such case
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;
            }
            if ((inputHints & (ImhNoPredictiveText | ImhSensitiveData | ImhHiddenText)) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;

            if ((inputHints & ImhUppercaseOnly) != 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_CHARACTERS;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            } else if ((inputHints & ImhLowercaseOnly) == 0
                    && (inputHints & ImhNoAutoUppercase) == 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_SENTENCES;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            }
        }

        if (enterKeyType == 0 && (inputHints & ImhMultiLine) != 0)
            imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;

        setInitialCapsMode(initialCapsMode);
        setImeOptions(imeOptions);
        setInputType(inputType);
    }

    private int imeOptionsFromEnterKeyType(int enterKeyType)
    {
        int imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;

        // enter key type - must be kept in sync with QTDIR/src/corelib/global/qnamespace.h
        switch (enterKeyType) {
            case 0: // EnterKeyDefault
                break;
            case 1: // EnterKeyReturn
                imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;
                break;
            case 2: // EnterKeyDone
                break;
            case 3: // EnterKeyGo
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
                break;
            case 4: // EnterKeySend
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_SEND;
                break;
            case 5: // EnterKeySearch
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_SEARCH;
                break;
            case 6: // EnterKeyNext
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_NEXT;
                break;
            case 7: // EnterKeyPrevious
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_PREVIOUS;
                break;
        }
        return imeOptions;
    }

    private boolean isDisablePredictiveTextWorkaround(int inputHints)
    {
        return (inputHints & ImhNoPredictiveText) != 0 &&
                System.getenv("QT_ANDROID_ENABLE_WORKAROUND_TO_DISABLE_PREDICTIVE_TEXT") != null;
    }
}
