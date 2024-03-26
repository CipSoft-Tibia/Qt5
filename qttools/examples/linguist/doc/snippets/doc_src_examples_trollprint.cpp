// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
twoSidedEnabledRadio = new QRadioButton(tr("Enabled", "two-sided"));
twoSidedDisabledRadio = new QRadioButton(tr("Disabled", "two-sided"));
//! [0]


//! [1]
colorsEnabledRadio = new QRadioButton(tr("Enabled", "colors"), colors);
colorsDisabledRadio = new QRadioButton(tr("Disabled", "colors"), colors);
//! [1]


//! [2]
/*
   TRANSLATOR MainWindow

   In this application the whole application is a MainWindow.
   Choose Help|About from the menu bar to see some text
   belonging to MainWindow.

   ...
*/
//! [2]


//! [3]
/*
   TRANSLATOR ZClientErrorDialog

   Choose Client|Edit to reach the Client Edit dialog, then choose
   Client Specification from the drop down list at the top and pick
   client Bartel Leendert van der Waerden. Now check the Profile
   checkbox and then click the Start Processing button. You should
   now see a pop up window with the text "Error: Name too long!".
   This window is a ZClientErrorDialog.
*/
//! [3]
