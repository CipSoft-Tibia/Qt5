/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTranslator>
#include <QQmlContext>
#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>
#include "../../shared/util.h"

class tst_qqmltranslation : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmltranslation() {}

private slots:
    void translation_data();
    void translation();
    void idTranslation();
    void translationChange();
    void preferJSContext();
};

void tst_qqmltranslation::translation_data()
{
    QTest::addColumn<QString>("translation");
    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<bool>("verifyCompiledData");

    QTest::newRow("qml") << QStringLiteral("qml_fr") << testFileUrl("translation.qml") << true;
    QTest::newRow("qrc") << QStringLiteral(":/qml_fr.qm") << QUrl("qrc:/translation.qml") << true;
    QTest::newRow("js") << QStringLiteral("qml_fr") << testFileUrl("jstranslation.qml") << false;
}

void tst_qqmltranslation::translation()
{
    QFETCH(QString, translation);
    QFETCH(QUrl, testFile);
    QFETCH(bool, verifyCompiledData);

    QTranslator translator;
    translator.load(translation, dataDirectory());
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFile);
    QObject *object = component.create();
    QVERIFY(object != nullptr);

    if (verifyCompiledData) {
        QQmlContext *context = qmlContext(object);
        QQmlEnginePrivate *engine = QQmlEnginePrivate::get(context->engine());
        QQmlRefPointer<QQmlTypeData> typeData = engine->typeLoader.getType(context->baseUrl());
        QV4::CompiledData::CompilationUnit *compilationUnit = typeData->compilationUnit();
        QVERIFY(compilationUnit);

        QSet<QString> compiledTranslations;
        compiledTranslations << QStringLiteral("basic")
                             << QStringLiteral("disambiguation")
                             << QStringLiteral("singular") << QStringLiteral("plural");

        const QV4::CompiledData::Object *rootObject
                = compilationUnit->qmlData->objectAt(/*root object*/0);
        const QV4::CompiledData::Binding *binding = rootObject->bindingTable();
        for (quint32 i = 0; i < rootObject->nBindings; ++i, ++binding) {
            const QString propertyName = compilationUnit->stringAt(binding->propertyNameIndex);

            const bool expectCompiledTranslation = compiledTranslations.contains(propertyName);

            if (expectCompiledTranslation) {
                if (binding->type != QV4::CompiledData::Binding::Type_Translation)
                    qDebug() << "binding for property" << propertyName << "is not a compiled translation";
                QCOMPARE(quint32(binding->type), quint32(QV4::CompiledData::Binding::Type_Translation));
            } else {
                if (binding->type == QV4::CompiledData::Binding::Type_Translation)
                    qDebug() << "binding for property" << propertyName << "is not supposed to be a compiled translation";
                QVERIFY(binding->type != QV4::CompiledData::Binding::Type_Translation);
            }
        }
    }

    QCOMPARE(object->property("basic").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("basic2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("basic3").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("disambiguation").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("disambiguation2").toString(), QString::fromUtf8("\xc3\xa0 plus tard"));
    QCOMPARE(object->property("disambiguation3").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("noop").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("noop2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("singular").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("singular2").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("plural").toString(), QLatin1String("2 canards"));
    QCOMPARE(object->property("plural2").toString(), QLatin1String("2 canards"));

    QCoreApplication::removeTranslator(&translator);
    delete object;
}

void tst_qqmltranslation::idTranslation()
{
    QTranslator translator;
    translator.load(QLatin1String("qmlid_fr"), dataDirectory());
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("idtranslation.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);

    {
        QQmlContext *context = qmlContext(object);
        QQmlEnginePrivate *engine = QQmlEnginePrivate::get(context->engine());
        QQmlRefPointer<QQmlTypeData> typeData = engine->typeLoader.getType(context->baseUrl());
        QV4::CompiledData::CompilationUnit *compilationUnit = typeData->compilationUnit();
        QVERIFY(compilationUnit);

        const QV4::CompiledData::Object *rootObject
                = compilationUnit->qmlData->objectAt(/*root object*/0);
        const QV4::CompiledData::Binding *binding = rootObject->bindingTable();
        for (quint32 i = 0; i < rootObject->nBindings; ++i, ++binding) {
            const QString propertyName = compilationUnit->stringAt(binding->propertyNameIndex);
            if (propertyName == "idTranslation") {
                if (binding->type != QV4::CompiledData::Binding::Type_TranslationById)
                    qDebug() << "binding for property" << propertyName << "is not a compiled translation";
                QCOMPARE(quint32(binding->type), quint32(QV4::CompiledData::Binding::Type_TranslationById));
            } else {
                QVERIFY(binding->type != QV4::CompiledData::Binding::Type_Translation);
            }
        }
    }

    QCOMPARE(object->property("idTranslation").toString(), QLatin1String("bonjour tout le monde"));
    QCOMPARE(object->property("idTranslation2").toString(), QLatin1String("bonjour tout le monde"));
    QCOMPARE(object->property("idTranslation3").toString(), QLatin1String("bonjour tout le monde"));

    QCoreApplication::removeTranslator(&translator);
    delete object;
}

class DummyTranslator : public QTranslator
{
    Q_OBJECT

    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override
    {
        Q_UNUSED(context);
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);
        if (!qstrcmp(sourceText, "translate me"))
            return QString::fromUtf8("xxx");
        if (!qstrcmp(sourceText, "English in mylibrary") && !qstrcmp(context, "mylibrary"))
            return QString::fromUtf8("Deutsch in mylibrary");
        if (!qstrcmp(sourceText, "English in translation") && !qstrcmp(context, "nested_js_translation"))
            return QString::fromUtf8("Deutsch in Setzung");
        return QString();
    }

    bool isEmpty() const override
    {
        return false;
    }
};

void tst_qqmltranslation::translationChange()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("translationChange.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("baseProperty").toString(), QString::fromUtf8("do not translate"));
    QCOMPARE(object->property("text1").toString(), QString::fromUtf8("translate me"));
    QCOMPARE(object->property("text2").toString(), QString::fromUtf8("translate me"));
    QCOMPARE(object->property("text3").toString(), QString::fromUtf8("translate me"));
    QCOMPARE(object->property("fromListModel").toString(), QString::fromUtf8("translate me"));

    DummyTranslator translator;
    QCoreApplication::installTranslator(&translator);

    QEvent ev(QEvent::LanguageChange);
    QCoreApplication::sendEvent(&engine, &ev);

    QCOMPARE(object->property("baseProperty").toString(), QString::fromUtf8("do not translate"));
    QCOMPARE(object->property("text1").toString(), QString::fromUtf8("xxx"));
    QCOMPARE(object->property("text2").toString(), QString::fromUtf8("xxx"));
    QCOMPARE(object->property("text3").toString(), QString::fromUtf8("xxx"));
    QCOMPARE(object->property("fromListModel").toString(), QString::fromUtf8("xxx"));

    QCoreApplication::removeTranslator(&translator);
}

void tst_qqmltranslation::preferJSContext()
{
    DummyTranslator translator;
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("preferjs.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("german1").toString(),
             QStringLiteral("Deutsch in Setzung"));
    QCOMPARE(object->property("german2").toString(),
             QStringLiteral("Deutsch in mylibrary"));

    QCoreApplication::removeTranslator(&translator);
}

QTEST_MAIN(tst_qqmltranslation)

#include "tst_qqmltranslation.moc"
