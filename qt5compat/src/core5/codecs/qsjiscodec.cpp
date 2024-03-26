// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Most of the code here was originally written by Serika Kurusugawa
// a.k.a. Junji Takagi, and is included in Qt with the author's permission,
// and the grateful thanks of the Qt team.

/*! \class QSjisCodec
    \inmodule QtCore5Compat
    \reentrant
    \internal
*/

#include "qsjiscodec_p.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

/*!
  Creates a Shift-JIS codec. Note that this is done automatically by
  the QApplication, you do not need construct your own.
*/
QSjisCodec::QSjisCodec() : conv(QJpUnicodeConv::newConverter(QJpUnicodeConv::Default))
{
}


/*!
  Destroys the Shift-JIS codec.
*/
QSjisCodec::~QSjisCodec()
{
    delete (const QJpUnicodeConv*)conv;
    conv = 0;
}


QByteArray QSjisCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }
    int invalid = 0;

    int rlen = 2*len + 1;
    QByteArray rstr;
    rstr.resize(rlen);
    uchar* cursor = (uchar*)rstr.data();
    for (int i = 0; i < len; i++) {
        QChar ch = uc[i];
        uint j;
        if (ch.row() == 0x00 && ch.cell() < 0x80) {
            // ASCII
            *cursor++ = ch.cell();
        } else if ((j = conv->unicodeToJisx0201(ch.row(), ch.cell())) != 0) {
            // JIS X 0201 Latin or JIS X 0201 Kana
            *cursor++ = j;
        } else if ((j = conv->unicodeToSjis(ch.row(), ch.cell())) != 0) {
            // JIS X 0208
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToSjisibmvdc(ch.row(), ch.cell())) != 0) {
            // JIS X 0208 IBM VDC
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToCp932(ch.row(), ch.cell())) != 0) {
            // CP932 (for lead bytes 87, ee & ed)
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToJisx0212(ch.row(), ch.cell())) != 0) {
            // JIS X 0212 (can't be encoded in ShiftJIS !)
            *cursor++ = 0x81;        // white square
            *cursor++ = 0xa0;        // white square
        } else {
            // Error
            *cursor++ = replacement;
            ++invalid;
        }
    }
    rstr.resize(cursor - (const uchar*)rstr.constData());

    if (state) {
        state->invalidChars += invalid;
    }
    return rstr;
}

QString QSjisCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    uchar buf[1] = {0};
    int nbuf = 0;
    QChar replacement = QChar::ReplacementCharacter;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = QChar::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
    }
    int invalid = 0;
    uint u= 0;
    QString result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (ch < 0x80) {
                result += QValidChar(ch);
            } else if (IsKana(ch)) {
                // JIS X 0201 Latin or JIS X 0201 Kana
                u = conv->jisx0201ToUnicode(ch);
                result += QValidChar(u);
            } else if (IsSjisChar1(ch)) {
                // JIS X 0208
                buf[0] = ch;
                nbuf = 1;
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            break;
        case 1:
            // JIS X 0208
            if (IsSjisChar2(ch)) {
                if ((u = conv->sjisibmvdcToUnicode(buf[0], ch))) {
                    result += QValidChar(u);
                } else if ((u = conv->cp932ToUnicode(buf[0], ch))) {
                    result += QValidChar(u);
                }
                else if (IsUserDefinedChar1(buf[0])) {
                    result += QChar::ReplacementCharacter;
                } else {
                    u = conv->sjisToUnicode(buf[0], ch);
                    result += QValidChar(u);
                }
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
            break;
        }
    }

    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->invalidChars += invalid;
    }
    return result;
}


int QSjisCodec::_mibEnum()
{
    return 17;
}

QByteArray QSjisCodec::_name()
{
    return "Shift_JIS";
}

/*!
    Returns the codec's mime name.
*/
QList<QByteArray> QSjisCodec::_aliases()
{
    QList<QByteArray> list;
    list << "SJIS" // Qt 3 compat
         << "MS_Kanji";
    return list;
}

QT_END_NAMESPACE
