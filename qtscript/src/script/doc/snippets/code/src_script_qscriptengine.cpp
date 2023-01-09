/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
QScriptEngine myEngine;
QScriptValue three = myEngine.evaluate("1 + 2");
//! [0]


//! [1]
QScriptValue fun = myEngine.evaluate("(function(a, b) { return a + b; })");
QScriptValueList args;
args << 1 << 2;
QScriptValue threeAgain = fun.call(QScriptValue(), args);
//! [1]


//! [2]
QString fileName = "helloworld.qs";
QFile scriptFile(fileName);
if (!scriptFile.open(QIODevice::ReadOnly))
    // handle error
QTextStream stream(&scriptFile);
QString contents = stream.readAll();
scriptFile.close();
myEngine.evaluate(contents, fileName);
//! [2]


//! [3]
myEngine.globalObject().setProperty("myNumber", 123);
...
QScriptValue myNumberPlusOne = myEngine.evaluate("myNumber + 1");
//! [3]


//! [4]
QScriptValue result = myEngine.evaluate(...);
if (myEngine.hasUncaughtException()) {
    int line = myEngine.uncaughtExceptionLineNumber();
    qDebug() << "uncaught exception at line" << line << ":" << result.toString();
}
//! [4]


//! [5]
QPushButton button;
QScriptValue scriptButton = myEngine.newQObject(&button);
myEngine.globalObject().setProperty("button", scriptButton);

myEngine.evaluate("button.checkable = true");

qDebug() << scriptButton.property("checkable").toBoolean();
scriptButton.property("show").call(); // call the show() slot
//! [5]


//! [6]
QScriptValue myAdd(QScriptContext *context, QScriptEngine *engine)
{
   QScriptValue a = context->argument(0);
   QScriptValue b = context->argument(1);
   return a.toNumber() + b.toNumber();
}
//! [6]


//! [7]
QScriptValue fun = myEngine.newFunction(myAdd);
myEngine.globalObject().setProperty("myAdd", fun);
//! [7]


//! [8]
QScriptValue result = myEngine.evaluate("myAdd(myNumber, 1)");
//! [8]


//! [9]
QScriptValue Foo(QScriptContext *context, QScriptEngine *engine)
{
    if (context->calledAsConstructor()) {
        // initialize the new object
        context->thisObject().setProperty("bar", ...);
        // ...
        // return a non-object value to indicate that the
        // thisObject() should be the result of the "new Foo()" expression
        return engine->undefinedValue();
    } else {
        // not called as "new Foo()", just "Foo()"
        // create our own object and return that one
        QScriptValue object = engine->newObject();
        object.setPrototype(context->callee().property("prototype"));
        object.setProperty("baz", ...);
        return object;
    }
}

...

QScriptValue fooProto = engine->newObject();
fooProto.setProperty("whatever", ...);
engine->globalObject().setProperty("Foo", engine->newFunction(Foo, fooProto));
//! [9]


//! [10]
class Bar { ... };

Q_DECLARE_METATYPE(Bar)

QScriptValue constructBar(QScriptContext *context, QScriptEngine *engine)
{
    Bar bar;
    // initialize from arguments in context, if desired
    ...
    return engine->toScriptValue(bar);
}

class BarPrototype : public QObject, public QScriptable
{
// provide the scriptable interface of this type using slots and properties
...
};

...

// create and register the Bar prototype and constructor in the engine
BarPrototype *barPrototypeObject = new BarPrototype(...);
QScriptValue barProto = engine->newQObject(barPrototypeObject);
engine->setDefaultPrototype(qMetaTypeId<Bar>, barProto);
QScriptValue barCtor = engine->newFunction(constructBar, barProto);
engine->globalObject().setProperty("Bar", barCtor);
//! [10]


//! [11]
static QScriptValue getSetFoo(QScriptContext *context, QScriptEngine *engine)
{
    QScriptValue callee = context->callee();
    if (context->argumentCount() == 1) // writing?
        callee.setProperty("value", context->argument(0));
    return callee.property("value");
}

....

QScriptValue object = engine.newObject();
object.setProperty("foo", engine.newFunction(getSetFoo),
    QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
//! [11]


//! [12]
QScriptValue object = engine.newObject();
object.setProperty("foo", engine.newFunction(getFoo), QScriptValue::PropertyGetter);
object.setProperty("foo", engine.newFunction(setFoo), QScriptValue::PropertySetter);
//! [12]


//! [13]
Q_SCRIPT_DECLARE_QMETAOBJECT(QLineEdit, QWidget*)

...

QScriptValue lineEditClass = engine.scriptValueFromQMetaObject<QLineEdit>();
engine.globalObject().setProperty("QLineEdit", lineEditClass);
//! [13]


//! [14]
if (hello && world)
    print("hello world");
//! [14]


//! [15]
if (hello &&
//! [15]


//! [16]
0 = 0
//! [16]


//! [17]
./test.js
//! [17]


//! [18]
foo["bar"]
//! [18]


//! [19]
QScriptEngine engine;
QScriptContext *context = engine.pushContext();
context->activationObject().setProperty("myArg", 123);
engine.evaluate("var tmp = myArg + 42");
...
engine.popContext();
//! [19]


//! [20]
struct MyStruct {
    int x;
    int y;
};
//! [20]


//! [21]
Q_DECLARE_METATYPE(MyStruct)
//! [21]


//! [22]
QScriptValue toScriptValue(QScriptEngine *engine, const MyStruct &s)
{
  QScriptValue obj = engine->newObject();
  obj.setProperty("x", s.x);
  obj.setProperty("y", s.y);
  return obj;
}

void fromScriptValue(const QScriptValue &obj, MyStruct &s)
{
  s.x = obj.property("x").toInt32();
  s.y = obj.property("y").toInt32();
}
//! [22]


//! [23]
qScriptRegisterMetaType(engine, toScriptValue, fromScriptValue);
//! [23]


//! [24]
MyStruct s = qscriptvalue_cast<MyStruct>(context->argument(0));
...
MyStruct s2;
s2.x = s.x + 10;
s2.y = s.y + 20;
QScriptValue v = engine->toScriptValue(s2);
//! [24]


//! [25]
QScriptValue createMyStruct(QScriptContext *, QScriptEngine *engine)
{
    MyStruct s;
    s.x = 123;
    s.y = 456;
    return engine->toScriptValue(s);
}
...
QScriptValue ctor = engine.newFunction(createMyStruct);
engine.globalObject().setProperty("MyStruct", ctor);
//! [25]


//! [26]
Q_DECLARE_METATYPE(QVector<int>)

...

qScriptRegisterSequenceMetaType<QVector<int> >(engine);
...
QVector<int> v = qscriptvalue_cast<QVector<int> >(engine->evaluate("[5, 1, 3, 2]"));
std::sort(v.begin(), v.end());
QScriptValue a = engine->toScriptValue(v);
qDebug() << a.toString(); // outputs "[1, 2, 3, 5]"
//! [26]

//! [27]
QScriptValue mySpecialQObjectConstructor(QScriptContext *context,
                                         QScriptEngine *engine)
{
    QObject *parent = context->argument(0).toQObject();
    QObject *object = new QObject(parent);
    return engine->newQObject(object, QScriptEngine::ScriptOwnership);
}

...

QScriptValue ctor = engine.newFunction(mySpecialQObjectConstructor);
QScriptValue metaObject = engine.newQMetaObject(&QObject::staticMetaObject, ctor);
engine.globalObject().setProperty("QObject", metaObject);

QScriptValue result = engine.evaluate("new QObject()");
//! [27]
