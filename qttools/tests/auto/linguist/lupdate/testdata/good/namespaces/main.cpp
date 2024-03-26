// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


























#include <QtCore>

class Class : public QObject
{
    Q_OBJECT

    class SubClass
    {
        void f()
        {
            tr("nested class context");
        }
    };

    void f()
    {
        tr("just class context");
    }
};

namespace Outer {

class Class : public QObject { Q_OBJECT };

namespace Middle1 {

class Class : public QObject { Q_OBJECT };

namespace Inner1 {

class Class : public QObject { Q_OBJECT };

}

namespace I = Inner1;

class Something;
class Different;

}

namespace Middle2 {

class Class : public QObject { Q_OBJECT };

namespace Inner2 {

class Class : public QObject { Q_OBJECT };

namespace IO = Middle2;

}

namespace I = Inner2;

}

namespace MI = Middle1::Inner1;

namespace O = ::Outer;

class Middle1::Different : QObject {
Q_OBJECT
    void f() {
        tr("different namespaced class def");
    }
};

}

namespace O = Outer;
namespace OM = Outer::Middle1;
namespace OMI = Outer::Middle1::I;

int main()
{
    Class::tr("outestmost class");
    Outer::Class::tr("outer class");
    Outer::MI::Class::tr("innermost one");
    OMI::Class::tr("innermost two");
    O::Middle1::I::Class::tr("innermost three");
    O::Middle2::I::Class::tr("innermost three b");
    OM::I::Class::tr("innermost four");
    return 0;
}

class OM::Something : QObject {
Q_OBJECT
    void f() {
        tr("namespaced class def");
    }
    void g() {
        tr("namespaced class def 2");
    }
};

// QTBUG-8360
namespace A {

void foo()
{
    using namespace A;
}

QString goo()
{
    return QObject::tr("Bla");
}

}


namespace AA {
namespace B {

using namespace AA;

namespace C {

class Test : public QObject {
    Q_OBJECT
};

}

}

using namespace B;
using namespace C;

void goo()
{
    AA::Test::tr("howdy?");
}

}


namespace A1 {
namespace B {

class Test : public QObject {
    Q_OBJECT
};

using namespace A1;

void foo()
{
    B::B::B::Test::tr("yeeee-ha!");
}

}
}


//#include "main.moc"
