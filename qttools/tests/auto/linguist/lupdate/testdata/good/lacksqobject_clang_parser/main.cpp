// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


























// IMPORTANT!!!! If you want to add testdata to this file,
// always add it to the end in order to not change the linenumbers of translations!!!

#define QTCORE <QtCore>
#include QTCORE // Hidden from lupdate, but compiles

//
// Test 'lacks Q_OBJECT' reporting on namespace scopes
//

class B : public QObject {
    //Q_OBJECT
    void foo();
};

void B::foo() {
    tr("Bla", "::B");
}


class C : public QObject {
    //Q_OBJECT
    void foo() {
        tr("Bla", "::C");
    }
};


namespace nsB {

    class B : public QObject {
        //Q_OBJECT
        void foo();
    };

    void B::foo() {
        tr("Bla", "nsB::B");
    }

    class C : public QObject {
        //Q_OBJECT
        void foo() {
            tr("Bla", "nsB::C");
        }
    };
}

