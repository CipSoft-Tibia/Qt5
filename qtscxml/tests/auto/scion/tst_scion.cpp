// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QJsonDocument>

#include <QtScxml/qscxmlcompiler.h>
#include <QtScxml/qscxmlstatemachine.h>

#include <functional>

#include "scxml/scion.h"
#include "scxml/compiled_tests.h"

Q_DECLARE_METATYPE(std::function<QScxmlStateMachine *()>);

enum { SpyWaitTime = 12000 };

static QSet<QString> testFailOnRun = QSet<QString>()
        // Currently we do not support loading data as XML content inside the <data> tag.
        << QLatin1String("w3c-ecma/test557.txml")
        // The following test uses the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441a.txml")
        // The following test needs manual inspection of the result. However, note that we do not support the undocumented "exmode" attribute.
        << QLatin1String("w3c-ecma/test441b.txml")
        // The following test needs manual inspection of the result.
        // The following test needs manual inspection of the result. However, note that we do not support multiple identical keys for event data.
        << QLatin1String("w3c-ecma/test178.txml")
        // We do not support the optional basic http event i/o processor.
        << QLatin1String("w3c-ecma/test201.txml")
        << QLatin1String("w3c-ecma/test230.txml")
        << QLatin1String("w3c-ecma/test250.txml")
        << QLatin1String("w3c-ecma/test307.txml")
        // Qt does not support forcing initial states that are not marked as such.
        << QLatin1String("w3c-ecma/test413.txml") // FIXME: verify initial state setting...
        << QLatin1String("w3c-ecma/test576.txml") // FIXME: verify initial state setting...
        // Scion apparently sets <data> values without a src/expr attribute to 0. We set it to undefined, as specified in B.2.1.
        << QLatin1String("w3c-ecma/test456.txml") // replaced by modified_test456
        // FIXME: qscxmlc fails on improper scxml file, currently no way of testing it properly for compiled case
        << QLatin1String("w3c-ecma/test301.txml")
        // FIXME: Currently we do not support nested scxml as a child of assign.
        << QLatin1String("w3c-ecma/test530.txml")
        ;

class MySignalSpy: public QSignalSpy
{
public:
    explicit MySignalSpy(const QObject *obj, const char *aSignal)
        : QSignalSpy(obj, aSignal)
    {}

    bool fastWait()
    {
        const int increment = SpyWaitTime / 20;
        for (int total = 0; total < SpyWaitTime; total += increment) {
            if (this->wait(increment))
                return true;
        }
        return false;
    }
};

class DynamicLoader: public QScxmlCompiler::Loader
{
public:
    DynamicLoader();
    QByteArray load(const QString &name,
                    const QString &baseDir,
                    QStringList *errors) override final;

};

DynamicLoader::DynamicLoader()
    : Loader()
{}

QByteArray DynamicLoader::load(const QString &name,
                               const QString &baseDir,
                               QStringList *errors)
{
    QStringList errs;
    QByteArray contents;
    QUrl url(name);
    if (!url.isLocalFile() && !url.isRelative())
        errs << QStringLiteral("src attribute is not a local file (%1)").arg(name);
    QFileInfo fInfo(url.isLocalFile() ? url.toLocalFile() : name);
    if (fInfo.isRelative())
        fInfo = QFileInfo(QDir(baseDir).filePath(fInfo.filePath()));
    fInfo = QFileInfo(QLatin1String(":/") + fInfo.filePath()); // take it from resources

    if (!fInfo.exists()) {
        errs << QStringLiteral("src attribute resolves to non existing file (%1)")
                 .arg(fInfo.absoluteFilePath());
    } else {
        QFile f(fInfo.absoluteFilePath());
        if (f.open(QFile::ReadOnly))
            contents = f.readAll();
        else
            errs << QStringLiteral("Failure opening file %1: %2")
                               .arg(fInfo.absoluteFilePath(), f.errorString());
    }
    if (errors)
        *errors = errs;

    return contents;
}


class TestScion: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void dynamic_data();
    void dynamic();
    void compiled_data();
    void compiled();

private:
    void generateData();
    bool runTest(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription);
};

void TestScion::initTestCase()
{
}

enum TestStatus {
    TestIsOk,
    TestFailsOnRun
};
Q_DECLARE_METATYPE(TestStatus)

void TestScion::generateData()
{
    QTest::addColumn<QString>("scxml");
    QTest::addColumn<QString>("json");
    QTest::addColumn<TestStatus>("testStatus");
    QTest::addColumn<std::function<QScxmlStateMachine *()>>("creator");

    const int nrOfTests = sizeof(testBases) / sizeof(const char *);
    for (int i = 0; i < nrOfTests; ++i) {
        TestStatus testStatus;
        QString base = QString::fromUtf8(testBases[i]);
        if (testFailOnRun.contains(base))
            testStatus = TestFailsOnRun;
        else
            testStatus = TestIsOk;
        QTest::newRow(testBases[i]) << base + QLatin1String(".scxml")
                                    << base + QLatin1String(".json")
                                    << testStatus
                                    << creators[i];
    }
}

void TestScion::dynamic_data()
{
    generateData();
}

void TestScion::dynamic()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(TestStatus, testStatus);
    QFETCH(std::function<QScxmlStateMachine *()>, creator);

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();
    QVERIFY(testDescription.isObject());

    QFile scxmlFile(QLatin1String(":/") + scxml);
    QVERIFY(scxmlFile.open(QIODevice::ReadOnly));
    QXmlStreamReader xmlReader(&scxmlFile);
    QScxmlCompiler compiler(&xmlReader);
    compiler.setFileName(scxml);
    DynamicLoader loader;
    compiler.setLoader(&loader);
    QScopedPointer<QScxmlStateMachine> stateMachine(compiler.compile());
    QVERIFY(compiler.errors().isEmpty());
    scxmlFile.close();

    QVERIFY(stateMachine != nullptr);
    stateMachine->setLoader(&loader);

    const bool runResult = runTest(stateMachine.data(), testDescription.object());
    if (runResult == false && testStatus == TestFailsOnRun)
        QEXPECT_FAIL("", "This is expected to fail on run", Abort);

    QVERIFY(runResult);
    QCoreApplication::processEvents(); // flush any pending events
}

static QStringList getStates(const QJsonObject &obj, const QString &key)
{
    QStringList states;
    auto jsonStates = obj.value(key).toArray();
    for (int i = 0, ei = jsonStates.size(); i != ei; ++i) {
        QString state = jsonStates.at(i).toString();
        Q_ASSERT(!state.isEmpty());
        states.append(state);
    }
    std::sort(states.begin(), states.end());
    return states;
}

void TestScion::compiled_data()
{
    generateData();
}

void TestScion::compiled()
{
    QFETCH(QString, scxml);
    QFETCH(QString, json);
    QFETCH(TestStatus, testStatus);
    QFETCH(std::function<QScxmlStateMachine *()>, creator);

    QFile jsonFile(QLatin1String(":/") + json);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    auto testDescription = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

    QScopedPointer<QScxmlStateMachine> stateMachine(creator());
    if (stateMachine == nullptr && testStatus == TestFailsOnRun) {
        QEXPECT_FAIL("", "This is expected to fail", Abort);
    }
    QVERIFY(stateMachine != nullptr);
    DynamicLoader loader;
    stateMachine->setLoader(&loader);

    const bool runResult = runTest(stateMachine.data(), testDescription.object());
    if (runResult == false && testStatus == TestFailsOnRun)
        QEXPECT_FAIL("", "This is expected to fail on run", Abort);

    QVERIFY(runResult);

    QCoreApplication::processEvents(); // flush any pending events
}

static bool verifyStates(QScxmlStateMachine *stateMachine, const QJsonObject &stateDescription, const QString &key, int counter)
{
    const auto errorMessage = [&key, &counter](const QStringList& current,
                                     const QStringList& expected, const QLatin1String& details) {
        qWarning("Incorrect %s (%d)!", qPrintable(key), counter);
        qWarning() << "Current configuration:" << current;
        qWarning() << "Expected configuration:" << expected;
        qWarning() << "Failed state read was done with:" << details;
    };
    using namespace Qt::StringLiterals;

    // Verify that activeStateNames() matches the expectation
    auto current = stateMachine->activeStateNames();
    std::sort(current.begin(), current.end());
    auto expected = getStates(stateDescription, key);
    if (current != expected) {
        errorMessage(current, expected, "activeStateNames()"_L1);
        return false;
    }

    for (const auto& s : expected) {
        // Verify that isActive(stateName) matches the expectation
        if (!stateMachine->isActive(s)) {
            errorMessage(current, expected, "isActive()"_L1);
            return false;
        }
        // Verify that the metaobject matches the expectation
        const auto mo = stateMachine->metaObject();
        if (!mo->property(mo->indexOfProperty(s.toLocal8Bit())).read(stateMachine).toBool()) {
            errorMessage(current, expected, "metaobject read"_L1);
            return false;
        }
    }
    return true;
}

static bool playEvent(QScxmlStateMachine *stateMachine, const QJsonObject &eventDescription, int counter)
{
    if (!stateMachine->isRunning()) {
        qWarning() << "State machine stopped running!";
        return false;
    }

    Q_ASSERT(eventDescription.contains(QLatin1String("event")));
    auto event = eventDescription.value(QLatin1String("event")).toObject();
    auto eventName = event.value(QLatin1String("name")).toString();
    Q_ASSERT(!eventName.isEmpty());
    QScxmlEvent::EventType type = QScxmlEvent::ExternalEvent;
    if (event.contains(QLatin1String("type"))) {
        QString typeStr = event.value(QLatin1String("type")).toString();
        if (typeStr.compare(QLatin1String("external"), Qt::CaseInsensitive) == 0)
            type = QScxmlEvent::InternalEvent;
        else if (typeStr.compare(QLatin1String("platform"), Qt::CaseInsensitive) == 0)
            type = QScxmlEvent::PlatformEvent;
        else {
            qWarning() << "unexpected event type in " << eventDescription;
            return false;
        }
    }
    QVariant data;
    // remove ifs and rely on defaults?
    if (event.contains(QLatin1String("data"))) {
        data = event.value(QLatin1String("data")).toVariant();
    }
    QString sendid;
    if (event.contains(QLatin1String("sendid")))
        sendid = event.value(QLatin1String("sendid")).toString();
    QString origin;
    if (event.contains(QLatin1String("origin")))
        origin = event.value(QLatin1String("origin")).toString();
    QString origintype;
    if (event.contains(QLatin1String("origintype")))
        origintype = event.value(QLatin1String("origintype")).toString();
    QString invokeid;
    if (event.contains(QLatin1String("invokeid")))
        invokeid = event.value(QLatin1String("invokeid")).toString();
    QScxmlEvent *e = new QScxmlEvent;
    e->setName(eventName);
    e->setEventType(type);
    e->setData(data);
    e->setSendId(sendid);
    e->setOrigin(origin);
    e->setOriginType(origintype);
    e->setInvokeId(invokeid);
    int delay = 0;
    if (eventDescription.contains(QLatin1String("after"))) {
        delay = eventDescription.value(QLatin1String("after")).toInt();
        Q_ASSERT(delay > 0);
        e->setDelay(delay);
    }

    QTimer trigger;
    trigger.setSingleShot(true);
    stateMachine->connectToEvent(eventName, &trigger, [&stateMachine, &trigger](const QScxmlEvent &) {
        QObject::connect(stateMachine, &QScxmlStateMachine::reachedStableState,
                         &trigger, QOverload<>::of(&QTimer::start));
    });
    MySignalSpy triggerSpy(&trigger, SIGNAL(timeout()));

    // Create a signal spy for each state we expect to be (de)activated in next transition.
    using namespace Qt::StringLiterals;
    const auto expectedActive = getStates(eventDescription, QLatin1String("nextConfiguration"));
    std::list<std::unique_ptr<QSignalSpy>> stateSpies;
    const auto mo = stateMachine->metaObject();

    for (const auto& s : stateMachine->stateNames()) {
        if (expectedActive.contains(s) == stateMachine->isActive(s))
            continue;
        // The state is expected to undergo an activation change, create a signal spy
        const auto changedSignal = s + "Changed(bool)"_L1;
        auto signalIndex = mo->indexOfSignal(changedSignal.toUtf8().constData());
        stateSpies.push_back(std::make_unique<QSignalSpy>(stateMachine, mo->method(signalIndex)));
    }

    stateMachine->submitEvent(e);

    if (!triggerSpy.fastWait()) {
        qWarning() << "State machine did not reach a stable state.";
    } else if (verifyStates(stateMachine, eventDescription, QLatin1String("nextConfiguration"), counter)) {
        for (const auto& spy : stateSpies) {
            if (spy->size() != 1) {
                qWarning() << "Signal error for:" << spy->signal() << ", size:" << spy->size();
                return false;
            }
        }
        return true;
    }

    if (delay > 0)
        qWarning("... after sending delayed event %s with a delay of %dms.", qPrintable(eventName), delay);
    else
        qWarning("... after sending event %s.", qPrintable(eventName));
    return false;
}

static bool playEvents(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription)
{
    auto jsonEvents = testDescription.value(QLatin1String("events"));
    Q_ASSERT(!jsonEvents.isNull());
    auto eventsArray = jsonEvents.toArray();
    for (int i = 0, ei = eventsArray.size(); i != ei; ++i) {
        if (!playEvent(stateMachine, eventsArray.at(i).toObject(), i + 1))
            return false;
    }
    return true;
}

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug debug, const QScxmlEvent &event)
{
    QJsonObject obj;
    obj.insert(QLatin1String("name"), event.name());
    obj.insert(QLatin1String("type"), event.eventType());
    obj.insert(QLatin1String("data"), QJsonValue::fromVariant(event.data()));
    obj.insert(QLatin1String("sendid"), event.sendId());
    obj.insert(QLatin1String("origin"), event.origin());
    obj.insert(QLatin1String("originType"), event.originType());
    obj.insert(QLatin1String("invokeid"), event.invokeId());
    return debug << obj;
}

QDebug operator<<(QDebug debug, const QList<QScxmlEvent> &events)
{
    return QtPrivate::printSequentialContainer(debug, "QList", events);
}
QT_END_NAMESPACE

static int verifyEvent(const QList<QScxmlEvent> &receivedEvents, const QJsonObject &event,
                       int position) {
    QScxmlEvent::EventType eventType = QScxmlEvent::ExternalEvent;
    const bool verifyEventType = event.contains(QLatin1String("type"));
    if (verifyEventType) {
        QString typeStr = event.value(QLatin1String("type")).toString();
        if (typeStr.compare(QLatin1String("external"), Qt::CaseInsensitive) == 0)
            eventType = QScxmlEvent::InternalEvent;
        else if (typeStr.compare(QLatin1String("platform"), Qt::CaseInsensitive) == 0)
            eventType = QScxmlEvent::PlatformEvent;
        else {
            qWarning() << "unexpected event type in " << event;
            return -1;
        }
    }

    const bool verifyName = event.contains(QLatin1String("name"));
    const QString name = verifyName ? event.value(QLatin1String("name")).toString() : QString();

    const bool verifyData = event.contains(QLatin1String("data"));
    const QVariant data = verifyData ? event.value(QLatin1String("data")).toVariant() : QVariant();
    const bool verifySendId = event.contains(QLatin1String("sendid"));
    const QString sendId = verifySendId ? event.value(QLatin1String("sendid")).toString()
                                        : QString();
    const bool verifyOrigin = event.contains(QLatin1String("origin"));
    const QString origin = verifyOrigin ? event.value(QLatin1String("origin")).toString()
                                        : QString();
    const bool verifyOriginType = event.contains(QLatin1String("originType"));
    const QString originType = verifyOriginType
            ? event.value(QLatin1String("origintype")).toString()
            : QString();
    const bool verifyInvokeId = event.contains(QLatin1String("invokeid"));
    const QString invokeId = verifyInvokeId ? event.value(QLatin1String("invokeid")).toString()
                                            : QString();

    while (position < receivedEvents.size()) {
        const QScxmlEvent &receivedEvent = receivedEvents[position];
        if ((verifyName && receivedEvent.name() != name)
                || (verifyEventType && receivedEvent.eventType() != eventType)
                || (verifyData && receivedEvent.data() != data)
                || (verifySendId && receivedEvent.sendId() != sendId)
                || (verifyOrigin && receivedEvent.origin() != origin)
                || (verifyOriginType && receivedEvent.originType() != originType)
                || (verifyInvokeId && receivedEvent.invokeId() != invokeId)) {
            ++position;
        } else {
            return position;
        }
    }

    qWarning("Did not receive expected event:");
    qWarning() << event;

    return -1; // nothing found
}

static bool verifyEvents(const QList<QScxmlEvent> &receivedEvents,
                         const QJsonObject &testDescription)
{
    auto jsonEvents = testDescription.value(QLatin1String("expectedEvents"));
    if (jsonEvents.isNull())
        return true;

    auto eventsArray = jsonEvents.toArray();

    int position = 0;
    for (int i = 0, ei = eventsArray.size(); i != ei; ++i) {
        position = verifyEvent(receivedEvents, eventsArray.at(i).toObject(), position);
        if (position < 0) {
            qWarning("received events:");
            qWarning() << receivedEvents;
            qWarning("expected events");
            qWarning() << eventsArray;
            return false;
        } else {
            ++position; // Don't use the same event twice.
        }
    }
    return true;
}

bool TestScion::runTest(QScxmlStateMachine *stateMachine, const QJsonObject &testDescription)
{
    MySignalSpy stableStateSpy(stateMachine, SIGNAL(reachedStableState()));
    MySignalSpy finishedSpy(stateMachine, SIGNAL(finished()));

    QList<QScxmlEvent> receivedEvents;
    stateMachine->connectToEvent(QLatin1String("*"), this, [&](const QScxmlEvent &event) {
        receivedEvents.append(event);
    });

    if (!stateMachine->init() && stateMachine->name() != QStringLiteral("test487")) {
        // test487 relies on a failing init to see if an error event gets posted.
        qWarning() << "init failed";
        return false;
    }
    stateMachine->start();

    if (testDescription.contains(QLatin1String("events"))
            && !testDescription.value(QLatin1String("events")).toArray().isEmpty()) {
        if (!stableStateSpy.fastWait()) {
            qWarning() << "Failed to reach stable initial state!";
            return false;
        }

        if (!verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration"), 0))
            return false;

        return playEvents(stateMachine, testDescription);
    } else {
        // Wait for all events (delayed or otherwise) to propagate.
        if (stateMachine->isRunning()) {
            finishedSpy.fastWait(); // Some tests don't have a final state, so don't check for the
                                    // result
        }
        if (!verifyEvents(receivedEvents, testDescription))
            return false;
        return verifyStates(stateMachine, testDescription, QLatin1String("initialConfiguration"), 0);
    }
}

QTEST_GUILESS_MAIN(TestScion)
#include "tst_scion.moc"
