// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Kevin Krammer <kevin.krammer@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QSet>

class tst_PropertyRequirements : public QObject
{
    Q_OBJECT
public:
    tst_PropertyRequirements();

private slots:
    void constantOrNotifyableMain();
    void constantOrNotifyableFull();

private:
    typedef QList<QQmlType> Failures;

    /*!
        All properties that do not pass the check and the affected QML types

        Key: Property name formatted as C++-class-name::property-name
        Value: List of QQmlType which have the property
     */
    typedef QHash<QString, Failures> FailuresByProperty;

    enum TestDepth {
        MainTypeOnly,     //! Check only the properties of the main C++ type for each QML type
        WithSuperClasses  //! Check the super classes for each C++ type as well
    };

    void testAllQmlTypes(TestDepth testDepth, FailuresByProperty &failuresByProperty);
    void testQmlType(TestDepth testDepth, const QQmlType &qmlType, FailuresByProperty &failuresByProperty);
};


tst_PropertyRequirements::tst_PropertyRequirements()
{
}

void tst_PropertyRequirements::constantOrNotifyableMain()
{
    FailuresByProperty failuresByProperty;

    // test
    testAllQmlTypes(MainTypeOnly, failuresByProperty);

    // report
    QHash<QString, QStringList> occurrences;
    for (auto it = failuresByProperty.constBegin(); it != failuresByProperty.constEnd(); ++it) {

        occurrences[it.value().at(0).qmlTypeName()].append(it.key());
    }

    QStringList messages;
    for (auto it = occurrences.constBegin(); it != occurrences.constEnd(); ++it) {
        const QString occurrencePattern("%1:\n\t%2");

        QStringList properties = it.value();
        properties.sort();
        messages.append(occurrencePattern.arg(it.key(), properties.join("\n\t")));
    }
    messages.sort();

    qWarning() << "\nThe following QML Types have properties which are neither CONSTANT nor NOTIFYable:\n"
        << qPrintable(messages.join("\n"));

    // TODO enable once technical debt is fixes
    // QCOMPARE(failuresByProperty.count(), 0);
}

void tst_PropertyRequirements::constantOrNotifyableFull()
{
    FailuresByProperty failuresByProperty;

    // test
    testAllQmlTypes(WithSuperClasses, failuresByProperty);

    // report
    QStringList messages;
    for (auto it = failuresByProperty.constBegin(); it != failuresByProperty.constEnd(); ++it) {

        QSet<QString> occurrences;
        for (const QQmlType &qmlType : it.value()) {
            static const QString occurrencePattern("%1 (%2)");

            occurrences.insert(occurrencePattern.arg(qmlType.metaObject()->className(),
                                                     qmlType.qmlTypeName()));

        }

        static const QString messagePattern("\nProperty %1 neither CONSTANT nor NOTIFYable. Affected types:\n\t%2");
        QStringList occurrencesList = occurrences.values();
        occurrencesList.sort();
        messages.append(messagePattern.arg(it.key(), occurrencesList.join("\n\t")));

    }
    messages.sort();

    qWarning() << qPrintable(messages.join("\n"));

    // TODO enable once technical debt is fixes
    // QCOMPARE(failuresByProperty.count(), 0);
}

void tst_PropertyRequirements::testAllQmlTypes(TestDepth testDepth, FailuresByProperty &failuresByProperty)
{
    const QVector<QByteArray> qmlData {
        "import QtQml 2.2\nQtObject {}",
        "import QtQml.Models 2.2\nListModel {}",
        "import QtQuick 2.5\nItem {}",
        "import QtQuick.Window 2.2\nWindow {}",
        "import QtQuick.Dialogs 1.2\nDialog {}",
        "import QtQuick.Layouts 1.2\nGridLayout {}",
        "import QtQuick.Controls 2.2\nButton {}",
        "import QtQuick.Templates 2.2\nButton {}"
    };

    QQmlEngine engine;
    QSet<QString> seenTypes;

    for (const QByteArray &data : qmlData) {
        QQmlComponent component(&engine);
        component.setData(data, QUrl());

        for (const QQmlType &qmlType : QQmlMetaType::qmlTypes()) {
            if (!seenTypes.contains(qmlType.qmlTypeName())) {
                testQmlType(testDepth, qmlType, failuresByProperty);
            }
        }
        const auto &typeNameList = QQmlMetaType::qmlTypeNames();
        seenTypes.unite(QSet<QString>(typeNameList.cbegin(), typeNameList.cend()));
    }
}

void tst_PropertyRequirements::testQmlType(TestDepth testDepth, const QQmlType &qmlType, FailuresByProperty &failuresByProperty)
{
    QList<const QMetaObject*> inheritanceHierarchy;

    const QMetaObject *mo = qmlType.metaObject();
    while (mo) {
        inheritanceHierarchy.prepend(mo);
        mo = mo->superClass();
    }

    // check if this type is derived from QObject and even can have signals
    // i.e. weed out the Q_GADGET classes
    if (inheritanceHierarchy.isEmpty()
        || inheritanceHierarchy.constFirst()->className() != QByteArrayLiteral("QObject")) {
        return;
    }

    if (testDepth == MainTypeOnly) {
        inheritanceHierarchy.clear();
        inheritanceHierarchy.append(qmlType.metaObject());
    }

    for (const QMetaObject *metaClass : std::as_const(inheritanceHierarchy)) {
        for (int idx = metaClass->propertyOffset(); idx < metaClass->propertyCount(); ++idx) {
            const QMetaProperty property = metaClass->property(idx);

            // needs to be either CONSTANT or have a NOTIFY signal
            if (!property.isConstant() && !property.hasNotifySignal()) {
                static const QString fullNamePattern("%1::%2");
                const QString fullPropertyName = fullNamePattern.arg(metaClass->className(), property.name());

                failuresByProperty[fullPropertyName].append(qmlType);
            }
        }
    }
}

QTEST_MAIN(tst_PropertyRequirements)

#include "tst_propertyrequirements.moc"
