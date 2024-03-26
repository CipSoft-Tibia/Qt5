// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/* Copyright 1985, 1987, 1990, 1998  The Open Group

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the names of the authors or their
   institutions shall not be used in advertising or otherwise to promote the
   sale, use or other dealings in this Software without prior written
   authorization from the authors.



   Copyright © 2009 Dan Nicholson

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice (including the next
   paragraph) shall be included in all copies or substantial portions of the
   Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/*
 XConvertCase was copied from src/3rdparty/xkbcommon/src/keysym.c
 The following code modifications were applied:

 XConvertCase() was renamed to xkbcommon_XConvertCase(), to not confuse it
 with Xlib's XConvertCase().

 UCSConvertCase() was renamed to qt_UCSConvertCase() and function's body was
 replaced to use Qt APIs for doing case conversion, which should give us better
 results instead of using the less complete version from keysym.c
*/

#include "qxkbcommon_p.h"

#include <QtCore/QChar>

QT_BEGIN_NAMESPACE

static void qt_UCSConvertCase(uint32_t code, xkb_keysym_t *lower, xkb_keysym_t *upper)
{
    *lower = QChar::toLower(code);
    *upper = QChar::toUpper(code);
}

void QXkbCommon::xkbcommon_XConvertCase(xkb_keysym_t sym, xkb_keysym_t *lower, xkb_keysym_t *upper)
{
    /* Latin 1 keysym */
    if (sym < 0x100) {
        qt_UCSConvertCase(sym, lower, upper);
        return;
    }

    /* Unicode keysym */
    if ((sym & 0xff000000) == 0x01000000) {
        qt_UCSConvertCase((sym & 0x00ffffff), lower, upper);
        *upper |= 0x01000000;
        *lower |= 0x01000000;
        return;
    }

    /* Legacy keysym */

    *lower = sym;
    *upper = sym;

    switch (sym >> 8) {
    case 1: /* Latin 2 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym == XKB_KEY_Aogonek)
            *lower = XKB_KEY_aogonek;
        else if (sym >= XKB_KEY_Lstroke && sym <= XKB_KEY_Sacute)
            *lower += (XKB_KEY_lstroke - XKB_KEY_Lstroke);
        else if (sym >= XKB_KEY_Scaron && sym <= XKB_KEY_Zacute)
            *lower += (XKB_KEY_scaron - XKB_KEY_Scaron);
        else if (sym >= XKB_KEY_Zcaron && sym <= XKB_KEY_Zabovedot)
            *lower += (XKB_KEY_zcaron - XKB_KEY_Zcaron);
        else if (sym == XKB_KEY_aogonek)
            *upper = XKB_KEY_Aogonek;
        else if (sym >= XKB_KEY_lstroke && sym <= XKB_KEY_sacute)
            *upper -= (XKB_KEY_lstroke - XKB_KEY_Lstroke);
        else if (sym >= XKB_KEY_scaron && sym <= XKB_KEY_zacute)
            *upper -= (XKB_KEY_scaron - XKB_KEY_Scaron);
        else if (sym >= XKB_KEY_zcaron && sym <= XKB_KEY_zabovedot)
            *upper -= (XKB_KEY_zcaron - XKB_KEY_Zcaron);
        else if (sym >= XKB_KEY_Racute && sym <= XKB_KEY_Tcedilla)
            *lower += (XKB_KEY_racute - XKB_KEY_Racute);
        else if (sym >= XKB_KEY_racute && sym <= XKB_KEY_tcedilla)
            *upper -= (XKB_KEY_racute - XKB_KEY_Racute);
        break;
    case 2: /* Latin 3 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XKB_KEY_Hstroke && sym <= XKB_KEY_Hcircumflex)
            *lower += (XKB_KEY_hstroke - XKB_KEY_Hstroke);
        else if (sym >= XKB_KEY_Gbreve && sym <= XKB_KEY_Jcircumflex)
            *lower += (XKB_KEY_gbreve - XKB_KEY_Gbreve);
        else if (sym >= XKB_KEY_hstroke && sym <= XKB_KEY_hcircumflex)
            *upper -= (XKB_KEY_hstroke - XKB_KEY_Hstroke);
        else if (sym >= XKB_KEY_gbreve && sym <= XKB_KEY_jcircumflex)
            *upper -= (XKB_KEY_gbreve - XKB_KEY_Gbreve);
        else if (sym >= XKB_KEY_Cabovedot && sym <= XKB_KEY_Scircumflex)
            *lower += (XKB_KEY_cabovedot - XKB_KEY_Cabovedot);
        else if (sym >= XKB_KEY_cabovedot && sym <= XKB_KEY_scircumflex)
            *upper -= (XKB_KEY_cabovedot - XKB_KEY_Cabovedot);
        break;
    case 3: /* Latin 4 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XKB_KEY_Rcedilla && sym <= XKB_KEY_Tslash)
            *lower += (XKB_KEY_rcedilla - XKB_KEY_Rcedilla);
        else if (sym >= XKB_KEY_rcedilla && sym <= XKB_KEY_tslash)
            *upper -= (XKB_KEY_rcedilla - XKB_KEY_Rcedilla);
        else if (sym == XKB_KEY_ENG)
            *lower = XKB_KEY_eng;
        else if (sym == XKB_KEY_eng)
            *upper = XKB_KEY_ENG;
        else if (sym >= XKB_KEY_Amacron && sym <= XKB_KEY_Umacron)
            *lower += (XKB_KEY_amacron - XKB_KEY_Amacron);
        else if (sym >= XKB_KEY_amacron && sym <= XKB_KEY_umacron)
            *upper -= (XKB_KEY_amacron - XKB_KEY_Amacron);
        break;
    case 6: /* Cyrillic */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XKB_KEY_Serbian_DJE && sym <= XKB_KEY_Serbian_DZE)
            *lower -= (XKB_KEY_Serbian_DJE - XKB_KEY_Serbian_dje);
        else if (sym >= XKB_KEY_Serbian_dje && sym <= XKB_KEY_Serbian_dze)
            *upper += (XKB_KEY_Serbian_DJE - XKB_KEY_Serbian_dje);
        else if (sym >= XKB_KEY_Cyrillic_YU && sym <= XKB_KEY_Cyrillic_HARDSIGN)
            *lower -= (XKB_KEY_Cyrillic_YU - XKB_KEY_Cyrillic_yu);
        else if (sym >= XKB_KEY_Cyrillic_yu && sym <= XKB_KEY_Cyrillic_hardsign)
            *upper += (XKB_KEY_Cyrillic_YU - XKB_KEY_Cyrillic_yu);
        break;
    case 7: /* Greek */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XKB_KEY_Greek_ALPHAaccent && sym <= XKB_KEY_Greek_OMEGAaccent)
            *lower += (XKB_KEY_Greek_alphaaccent - XKB_KEY_Greek_ALPHAaccent);
        else if (sym >= XKB_KEY_Greek_alphaaccent && sym <= XKB_KEY_Greek_omegaaccent &&
             sym != XKB_KEY_Greek_iotaaccentdieresis &&
             sym != XKB_KEY_Greek_upsilonaccentdieresis)
            *upper -= (XKB_KEY_Greek_alphaaccent - XKB_KEY_Greek_ALPHAaccent);
        else if (sym >= XKB_KEY_Greek_ALPHA && sym <= XKB_KEY_Greek_OMEGA)
            *lower += (XKB_KEY_Greek_alpha - XKB_KEY_Greek_ALPHA);
        else if (sym >= XKB_KEY_Greek_alpha && sym <= XKB_KEY_Greek_omega &&
             sym != XKB_KEY_Greek_finalsmallsigma)
            *upper -= (XKB_KEY_Greek_alpha - XKB_KEY_Greek_ALPHA);
        break;
    case 0x13: /* Latin 9 */
        if (sym == XKB_KEY_OE)
            *lower = XKB_KEY_oe;
        else if (sym == XKB_KEY_oe)
            *upper = XKB_KEY_OE;
        else if (sym == XKB_KEY_Ydiaeresis)
            *lower = XKB_KEY_ydiaeresis;
        break;
    }
}

QT_END_NAMESPACE
