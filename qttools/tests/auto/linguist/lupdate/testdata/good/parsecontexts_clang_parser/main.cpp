// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


























// IMPORTANT!!!! If you want to add testdata to this file,
// always add it to the end in order to not change the linenumbers of translations!!!
#include <QtCore>
#include <QtGui>
#include <QtWidgets/QMainWindow>
//
// Test namespace scoping
//

class D : public QObject {
    Q_OBJECT
    public:
    QString foo() {
        return tr("test", "D");
    }

};

namespace A {

    class C : public QObject {
        Q_OBJECT
        public:
        void foo();
    };

    void C::foo() {
        tr("Bla", "A::C");
    }

    void goo() {
        C::tr("Bla", "A::C");       // Is identical to the previous tr(), (same context, sourcetext and comment,
                                    // so it should not add another entry to the list of messages)
    }

    void goo2() {
        C::tr("Bla 2", "A::C");     //Should be in the same namespace as the previous tr()
    }

}


namespace X {

    class D : public QObject {
        Q_OBJECT
        public:

    };

    class E : public QObject {
        Q_OBJECT
        public:
        void foo() { D::tr("foo", "D"); }  // Note that this is X::D from 440 on
    };


    namespace Y {
        class E : public QObject {
            Q_OBJECT

        };

        class C : public QObject {
            Q_OBJECT
            void foo();
        };

        void C::foo() {
            tr("Bla", "X::Y::C");
        }

        void goo() {
            D::tr("Bla", "X::D");   //This should be assigned to the X::D context
        }

        void goo2() {
            E::tr("Bla", "X::Y::E");   //This should be assigned to the X::Y::E context
            Y::E::tr("Bla", "X::Y::E");   //This should be assigned to the X::Y::E context
        }

    }; // namespace Y

    class F : public QObject {
        Q_OBJECT
        inline void inlinefunc() {
            tr("inline function", "X::F");
        }
    };
} // namespace X

namespace ico {
    namespace foo {
        class A : public QObject {
            A();
        };

        A::A() {
            tr("myfoo", "ico::foo::A");
            QObject::tr("task 161186", "QObject");
        }
    }
}

namespace AA {
class C {};
}

/**
 * the context of a message should not be affected by any inherited classes
 *
 * Keep this disabled for now, but at a long-term range it should work.
 */
namespace Gui {
    class MainWindow : public QMainWindow,
                    public AA::C
    {
        Q_OBJECT
public:
        MainWindow()
        {
            tr("More bla", "Gui::MainWindow");
        }

    };
} //namespace Gui


namespace A1 {
    class AB : public QObject {
        Q_OBJECT
        public:

        friend class OtherClass;

        QString inlineFuncAfterFriendDeclaration() const {
            return tr("inlineFuncAfterFriendDeclaration", "A1::AB");
        }
    };
    class B : AB {
        Q_OBJECT
        public:
        QString foo() const { return tr("foo", "A1::B"); }
    };

    // This is valid C++ too....
    class V : virtual AB {
        Q_OBJECT
        public:
        QString bar() const { return tr("bar", "A1::V"); }
    };

    class W : virtual public AB {
        Q_OBJECT
        public:
        QString baz() const { return tr("baz", "A1::W"); }
    };
}

class ForwardDecl;


class B1 : public QObject {
};

class C1 : public QObject {
};

namespace A1 {

class B2 : public QObject {
};

}

void func1()
{
    B1::tr("test TRANSLATOR comment (1)", "B1");

}

using namespace A1;
/*
    TRANSLATOR A1::B2
*/
void func2()
{
    B2::tr("test TRANSLATOR comment (2)", "A1::B2");
    C1::tr("test TRANSLATOR comment (3)", "C1");
}

void func3()
{
    B2::tr("test TRANSLATOR comment (4)", "A1::B2");
}

/*
    TRANSLATOR B2
    This is a comment to the translator.
*/
void func4()
{
    B2::tr("test TRANSLATOR comment (5)", "A1::B2");
}

namespace A1 {
namespace B3 {
class C2 : public QObject {
QString foo();
};
}
}

namespace D1 = A1::B3;
using namespace D1;

// TRANSLATOR A1::B3::C2
QString C2::foo()
{
    return tr("test TRANSLATOR comment (6)", "A1::B3::C2"); // 4.4 screws up
}
namespace Fooish {
  struct toto {
  Q_DECLARE_TR_FUNCTIONS(Bears::And::Spiders)
    QString bar(); };
}

QString Fooish::toto::bar()
{
    return tr("whatever the context", "Bears::And::Spiders");
}


int main(int /*argc*/, char ** /*argv*/) {
    return 0;
}

//#include "main.moc"
