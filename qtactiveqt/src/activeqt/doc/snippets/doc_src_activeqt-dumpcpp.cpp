// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
Outlook::Application *outlook = new Outlook::Application;
//! [0]


//! [1]
Outlook::_NameSpace *session = outlook->Session();
//! [1]


//! [2]
Outlook::NameSpace *session = outlook->Session();
//! [2]


//! [3]
Outlook::_NameSpace *tmp = outlook->Session();
Outlook::NameSpace *session = new Outlook::NameSpace(tmp);
delete tmp; // or any other use of tmp: segfault
//! [3]


//! [4]
Outlook::NameSpace *session = new Outlook::NameSpace(outlook->Session());
//! [4]
