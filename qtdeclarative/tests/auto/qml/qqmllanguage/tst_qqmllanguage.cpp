// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlincubator.h>
#include <QtCore/qiterable.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qrandom.h>
#include <QtGui/qevent.h>
#include <QSignalSpy>
#include <QFont>
#include <QQmlFileSelector>
#include <QFileSelector>
#include <QEasingCurve>
#include <QScopeGuard>

#include <private/qqmlproperty_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qv4debugging_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <QtQml/private/qqmlexpression_p.h>

#include "testtypes.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/testhttpserver_p.h>

#include <deque>

#if defined(Q_OS_MAC)
#include <unistd.h>
#endif

using namespace Qt::StringLiterals;

DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static inline bool isCaseSensitiveFileSystem(const QString &path) {
    Q_UNUSED(path);
#if defined(Q_OS_MAC)
    return pathconf(path.toLatin1().constData(), _PC_CASE_SENSITIVE);
#elif defined(Q_OS_WIN)
    return false;
#else
    return true;
#endif
}

/*
This test case covers QML language issues.  This covers everything that does not
involve evaluating ECMAScript expressions and bindings.

Evaluation of expressions and bindings is covered in qmlecmascript
*/
class tst_qqmllanguage : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qqmllanguage();

private slots:
    void initTestCase() override;
    void cleanupTestCase();

    void errors_data();
    void errors();

    void insertedSemicolon_data();
    void insertedSemicolon();

    void simpleObject();
    void simpleContainer();
    void interfaceProperty();
    void interfaceQList();
    void assignObjectToSignal();
    void assignObjectToVariant();
    void assignLiteralSignalProperty();
    void assignQmlComponent();
    void assignValueTypes();
    void assignTypeExtremes();
    void assignCompositeToType();
    void assignLiteralToVar();
    void assignLiteralToJSValue();
    void assignNullStrings();
    void bindJSValueToVar();
    void bindJSValueToVariant();
    void bindJSValueToType();
    void bindTypeToJSValue();
    void customParserTypes();
    void customParserTypeInInlineComponent();
    void rootAsQmlComponent();
    void rootItemIsComponent();
    void inlineQmlComponents();
    void idProperty();
    void autoNotifyConnection();
    void assignSignal();
    void assignSignalFunctionExpression();
    void overrideSignal_data();
    void overrideSignal();
    void dynamicProperties();
    void dynamicPropertiesNested();
    void listProperties();
    void listPropertiesInheritanceNoCrash();
    void badListItemType();
    void dynamicObjectProperties();
    void dynamicSignalsAndSlots();
    void simpleBindings();
    void noDoubleEvaluationForFlushedBindings_data();
    void noDoubleEvaluationForFlushedBindings();
    void autoComponentCreation();
    void autoComponentCreationInGroupProperty();
    void propertyValueSource();
    void requiredProperty();
    void requiredPropertyFromCpp_data();
    void requiredPropertyFromCpp();
    void attachedProperties();
    void dynamicObjects();
    void valueTypes();
    void cppnamespace();
    void aliasProperties();
    void aliasPropertiesAndSignals();
    void aliasPropertyChangeSignals();
    void qtbug_89822();
    void componentCompositeType();
    void i18n();
    void i18n_data();
    void onCompleted();
    void onDestruction();
    void scriptString();
    void scriptStringJs();
    void scriptStringWithoutSourceCode();
    void scriptStringComparison();
    void defaultPropertyListOrder();
    void defaultPropertyWithInitializer_data();
    void defaultPropertyWithInitializer();
    void declaredPropertyValues();
    void dontDoubleCallClassBegin();
    void reservedWords_data();
    void reservedWords();
    void inlineAssignmentsOverrideBindings();
    void nestedComponentRoots();
    void registrationOrder();
    void readonly();
    void readonlyObjectProperties();
    void receivers();
    void registeredCompositeType();
    void registeredCompositeTypeWithEnum();
    void registeredCompositeTypeWithAttachedProperty();
    void implicitImportsLast();

    void basicRemote_data();
    void basicRemote();
    void importsBuiltin_data();
    void importsBuiltin();
    void importsLocal_data();
    void importsLocal();
    void importsRemote_data();
    void importsRemote();
    void importsInstalled_data();
    void importsInstalled();
    void importsInstalledRemote_data();
    void importsInstalledRemote();
    void importsPath_data();
    void importsPath();
    void importsOrder_data();
    void importsOrder();
    void importIncorrectCase();
    void importJs_data();
    void importJs();
    void importJsModule_data();
    void importJsModule();
    void explicitSelfImport();
    void importInternalType();

    void qmlAttachedPropertiesObjectMethod();
    void customOnProperty();
    void variantNotify();

    void revisions();
    void revisionOverloads();

    void subclassedUncreateableRevision_data();
    void subclassedUncreateableRevision();

    void subclassedExtendedUncreateableRevision_data();
    void subclassedExtendedUncreateableRevision();

    void uncreatableTypesAsProperties();

    void propertyInit();
    void remoteLoadCrash();
    void signalWithDefaultArg();
    void signalParameterTypes();
    void functionParameterTypes();

    // regression tests for crashes
    void crash1();
    void crash2();

    void globalEnums();
    void lowercaseEnumRuntime_data();
    void lowercaseEnumRuntime();
    void lowercaseEnumCompileTime_data();
    void lowercaseEnumCompileTime();
    void scopedEnum();
    void scopedEnumsWithNameClash();
    void scopedEnumsWithResolvedNameClash();
    void enumNoScopeLeak();
    void qmlEnums();
    void literals_data();
    void literals();

    void objectDeletionNotify_data();
    void objectDeletionNotify();

    void scopedProperties();

    void deepProperty();

    void groupAssignmentFailure();

    void compositeSingletonProperties();
    void compositeSingletonSameEngine();
    void compositeSingletonDifferentEngine();
    void compositeSingletonNonTypeError();
    void compositeSingletonQualifiedNamespace();
    void compositeSingletonModule();
    void compositeSingletonModuleVersioned();
    void compositeSingletonModuleQualified();
    void compositeSingletonInstantiateError();
    void compositeSingletonDynamicPropertyError();
    void compositeSingletonDynamicSignalAndJavaScriptPragma();
    void compositeSingletonQmlRegisterTypeError();
    void compositeSingletonQmldirNoPragmaError();
    void compositeSingletonQmlDirError();
    void compositeSingletonRemote();
    void compositeSingletonSelectors();
    void compositeSingletonRegistered();
    void compositeSingletonCircular();
    void compositeSingletonRequiredProperties();
    void compositeSingletonRequiredProperties_data();

    void singletonsHaveContextAndEngine();

    void customParserBindingScopes();
    void customParserEvaluateEnum();
    void customParserProperties();
    void customParserWithExtendedObject();
    void nestedCustomParsers();

    void preservePropertyCacheOnGroupObjects();
    void propertyCacheInSync();

    void rootObjectInCreationNotForSubObjects();
    void lazyDeferredSubObject();
    void deferredProperties();
    void executeDeferredPropertiesOnce();
    void deferredProperties_extra();

    void noChildEvents();

    void earlyIdObjectAccess();

    void deleteSingletons();

    void arrayBuffer_data();
    void arrayBuffer();

    void defaultListProperty();
    void namespacedPropertyTypes();

    void qmlTypeCanBeResolvedByName_data();
    void qmlTypeCanBeResolvedByName();

    void instanceof_data();
    void instanceof();

    void concurrentLoadQmlDir();

    void accessDeletedObject();

    void lowercaseTypeNames();

    void thisInQmlScope();

    void valueTypeGroupPropertiesInBehavior();

    void retrieveQmlTypeId();

    void polymorphicFunctionLookup();
    void anchorsToParentInPropertyChanges();

    void typeWrapperToVariant();

    void extendedForeignTypes();
    void foreignTypeSingletons();

    void inlineComponent();
    void inlineComponent_data();
    void inlineComponentReferenceCycle_data();
    void inlineComponentReferenceCycle();
    void nestedInlineComponentNotAllowed();
    void inlineComponentStaticTypeResolution();
    void inlineComponentInSingleton();
    void nonExistingInlineComponent_data();
    void nonExistingInlineComponent();
    void inlineComponentFoundBeforeOtherImports();
    void inlineComponentDuplicateNameError();
    void inlineComponentWithAliasInstantiatedWithNewProperties();
    void inlineComponentWithImplicitComponent();

    void selfReference();
    void selfReferencingSingleton();

    void listContainingDeletedObject();
    void overrideSingleton();
    void revisionedPropertyOfAttachedObjectProperty();

    void arrayToContainer();
    void qualifiedScopeInCustomParser();
    void accessNullPointerPropertyCache();
    void bareInlineComponent();

    void checkUncreatableNoReason();

    void checkURLtoURLObject();
    void registerValueTypes();
    void extendedNamespace();
    void extendedNamespaceByObject();
    void extendedByAttachedType();
    void factorySingleton();
    void extendedSingleton();
    void qtbug_85932();
    void qtbug_86482();

    void multiExtension();
    void multiExtensionExtra();
    void multiExtensionIndirect();
    void multiExtensionQmlTypes();
    void extensionSpecial();
    void extensionRevision();
    void extendedGroupProperty();
    void invalidInlineComponent();
    void warnOnInjectedParameters();
#if QT_CONFIG(wheelevent)
    void warnOnInjectedParametersFromCppSignal();
#endif

    void qtbug_85615();

    void hangOnWarning();

    void groupPropertyFromNonExposedBaseClass();

    void listEnumConversion();
    void deepInlineComponentScriptBinding();

    void propertyObserverOnReadonly();
    void valueTypeWithEnum();
    void enumsFromRelatedTypes();

    void propertyAndAliasMustHaveDistinctNames_data();
    void propertyAndAliasMustHaveDistinctNames();

    void variantListConversion();
    void thisInArrowFunction();

    void jittedAsCast();
    void propertyNecromancy();
    void generalizedGroupedProperty();

    void groupedAttachedProperty_data();
    void groupedAttachedProperty();

    void ambiguousContainingType();
    void objectAsBroken();
    void customValueTypes();
    void valueTypeList();
    void componentMix();
    void uncreatableAttached();
    void resetGadgetProperty();
    void leakingAttributesQmlAttached();
    void leakingAttributesQmlSingleton();
    void leakingAttributesQmlForeign();
    void attachedOwnProperties();
    void bindableOnly();
    void v4SequenceMethods();
    void v4SequenceMethodsWithParams_data();
    void v4SequenceMethodsWithParams();
    void jsFunctionOverridesImport();
    void bindingAliasToComponentUrl();
    void badGroupedProperty();
    void functionInGroupedProperty();
    void signalInlineComponentArg();
    void functionSignatureEnforcement();
    void importPrecedence();
    void nullIsNull();
    void multiRequired();
    void isNullOrUndefined();

    void objectAndGadgetMethodCallsRejectThisObject();
    void objectAndGadgetMethodCallsAcceptThisObject();
    void asValueType();

    void longConversion();

    void enumPropsManyUnderylingTypes();

    void typedEnums_data();
    void typedEnums();

    void objectMethodClone();
    void unregisteredValueTypeConversion();
    void retainThis();

    void variantObjectList();
    void jitExceptions();

    void attachedInCtor();
    void byteArrayConversion();
    void propertySignalNames_data();
    void propertySignalNames();
    void signalNames_data();
    void signalNames();

    void callMethodOfAttachedDerived();

    void multiVersionSingletons();
    void typeAnnotationCycle();
    void corpseInQmlList();
    void objectInQmlListAndGc();
    void asCastToInlineComponent();
    void deepAliasOnICOrReadonly();

    void optionalChainCallOnNullProperty();

    void ambiguousComponents();

    void writeNumberToEnumAlias();
    void badInlineComponentAnnotation();
    void manuallyCallSignalHandler();
    void overrideDefaultProperty();
    void enumScopes();

    void typedObjectList();

    void nestedVectors();

    void overrideInnerBinding();

private:
    QQmlEngine engine;
    QStringList defaultImportPathList;

    void testType(const QString& qml, const QString& type, const QString& error, bool partialMatch = false);

    // When calling into JavaScript, the specific type of the return value can differ if that return
    // value is a number. This is not only the case for non-integral numbers, or numbers that do not
    // fit into the (signed) integer range, but it also depends on which optimizations are run. So,
    // to check if the return value is of a number type, use this method instead of checking against
    // a specific userType.
    static bool isJSNumberType(int userType)
    {
        return userType == QMetaType::Int || userType == QMetaType::UInt
                || userType == QMetaType::Double;
    }

    void getSingletonInstance(QQmlEngine& engine, const char* fileName, const char* propertyName, QObject** result /* out */);
    void getSingletonInstance(QObject* o, const char* propertyName, QObject** result /* out */);
};

#define DETERMINE_ERRORS(errorfile,expected,actual)\
    QList<QByteArray> expected; \
    QList<QByteArray> actual; \
    do { \
        QFile file(testFile(errorfile)); \
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); \
        QByteArray data = file.readAll(); \
        file.close(); \
        expected = data.split('\n'); \
        expected.removeAll(QByteArray("")); \
        QList<QQmlError> errors = component.errors(); \
        for (int ii = 0; ii < errors.count(); ++ii) { \
            const QQmlError &error = errors.at(ii); \
            QByteArray errorStr = QByteArray::number(error.line()) + ':' +  \
                                  QByteArray::number(error.column()) + ':' + \
                                  error.description().toUtf8(); \
            actual << errorStr; \
        } \
    } while (false);

#define VERIFY_ERRORS(errorfile) \
    if (!errorfile) { \
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty()) \
            qWarning() << "Unexpected Errors:" << component.errors(); \
        QVERIFY2(!component.isError(), qPrintable(component.errorString())); \
        QVERIFY(component.errors().isEmpty()); \
    } else { \
        DETERMINE_ERRORS(errorfile,expected,actual);\
        if (qgetenv("DEBUG") != "" && expected != actual) \
            qWarning() << "Expected:" << expected << "Actual:" << actual;  \
        if (qgetenv("QDECLARATIVELANGUAGE_UPDATEERRORS") != "" && expected != actual) {\
            QFile file(testFile(errorfile)); \
            QVERIFY(file.open(QIODevice::WriteOnly)); \
            for (int ii = 0; ii < actual.count(); ++ii) { \
                file.write(actual.at(ii)); file.write("\n"); \
            } \
            file.close(); \
        } else { \
            QCOMPARE(actual, expected); \
        } \
    }

void tst_qqmllanguage::cleanupTestCase()
{
    if (dataDirectoryUrl().scheme() != QLatin1String("qrc"))
        QVERIFY(QFile::remove(testFile(QString::fromUtf8("I18nType\303\201\303\242\303\243\303\244\303\245.qml"))));
}

void tst_qqmllanguage::insertedSemicolon_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");
    QTest::addColumn<bool>("create");

    QTest::newRow("insertedSemicolon.1") << "insertedSemicolon.1.qml" << "insertedSemicolon.1.errors.txt" << false;
}

void tst_qqmllanguage::insertedSemicolon()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);
    QFETCH(bool, create);

    QQmlComponent component(&engine, testFileUrl(file));

    std::unique_ptr<QObject> object;

    if(create) {
        object.reset(component.create());
        QVERIFY(object.get());
    }

    VERIFY_ERRORS(errorFile.toLatin1().constData());
}

void tst_qqmllanguage::errors_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");
    QTest::addColumn<bool>("create");

    QTest::newRow("nonexistantProperty.1") << "nonexistantProperty.1.qml" << "nonexistantProperty.1.errors.txt" << false;
    QTest::newRow("nonexistantProperty.2") << "nonexistantProperty.2.qml" << "nonexistantProperty.2.errors.txt" << false;
    QTest::newRow("nonexistantProperty.3") << "nonexistantProperty.3.qml" << "nonexistantProperty.3.errors.txt" << false;
    QTest::newRow("nonexistantProperty.4") << "nonexistantProperty.4.qml" << "nonexistantProperty.4.errors.txt" << false;
    QTest::newRow("nonexistantProperty.5") << "nonexistantProperty.5.qml" << "nonexistantProperty.5.errors.txt" << false;
    QTest::newRow("nonexistantProperty.6") << "nonexistantProperty.6.qml" << "nonexistantProperty.6.errors.txt" << false;
    QTest::newRow("nonexistantProperty.7") << "nonexistantProperty.7.qml" << "nonexistantProperty.7.errors.txt" << false;
    QTest::newRow("nonexistantProperty.8") << "nonexistantProperty.8.qml" << "nonexistantProperty.8.errors.txt" << false;

    QTest::newRow("wrongType (string for int)") << "wrongType.1.qml" << "wrongType.1.errors.txt" << false;
    QTest::newRow("wrongType (int for bool)") << "wrongType.2.qml" << "wrongType.2.errors.txt" << false;
    QTest::newRow("wrongType (bad rect)") << "wrongType.3.qml" << "wrongType.3.errors.txt" << false;

    QTest::newRow("wrongType (invalid enum)") << "wrongType.4.qml" << "wrongType.4.errors.txt" << false;
    QTest::newRow("wrongType (int for uint)") << "wrongType.5.qml" << "wrongType.5.errors.txt" << false;
    QTest::newRow("wrongType (string for real)") << "wrongType.6.qml" << "wrongType.6.errors.txt" << false;
    QTest::newRow("wrongType (int for color)") << "wrongType.7.qml" << "wrongType.7.errors.txt" << false;
    QTest::newRow("wrongType (int for date)") << "wrongType.8.qml" << "wrongType.8.errors.txt" << false;
    QTest::newRow("wrongType (int for time)") << "wrongType.9.qml" << "wrongType.9.errors.txt" << false;
    QTest::newRow("wrongType (int for datetime)") << "wrongType.10.qml" << "wrongType.10.errors.txt" << false;
    QTest::newRow("wrongType (string for point)") << "wrongType.11.qml" << "wrongType.11.errors.txt" << false;
    QTest::newRow("wrongType (color for size)") << "wrongType.12.qml" << "wrongType.12.errors.txt" << false;
    QTest::newRow("wrongType (number string for int)") << "wrongType.13.qml" << "wrongType.13.errors.txt" << false;
    QTest::newRow("wrongType (int for string)") << "wrongType.14.qml" << "wrongType.14.errors.txt" << false;
    QTest::newRow("wrongType (int for url)") << "wrongType.15.qml" << "wrongType.15.errors.txt" << false;
    QTest::newRow("wrongType (invalid object)") << "wrongType.16.qml" << "wrongType.16.errors.txt" << false;
    QTest::newRow("wrongType (int for enum)") << "wrongType.17.qml" << "wrongType.17.errors.txt" << false;

    QTest::newRow("readOnly.1") << "readOnly.1.qml" << "readOnly.1.errors.txt" << false;
    QTest::newRow("readOnly.2") << "readOnly.2.qml" << "readOnly.2.errors.txt" << false;
    QTest::newRow("readOnly.3") << "readOnly.3.qml" << "readOnly.3.errors.txt" << false;
    QTest::newRow("readOnly.4") << "readOnly.4.qml" << "readOnly.4.errors.txt" << false;
    QTest::newRow("readOnly.5") << "readOnly.5.qml" << "readOnly.5.errors.txt" << false;

    QTest::newRow("listAssignment.1") << "listAssignment.1.qml" << "listAssignment.1.errors.txt" << false;
    QTest::newRow("listAssignment.2") << "listAssignment.2.qml" << "listAssignment.2.errors.txt" << false;
    QTest::newRow("listAssignment.3") << "listAssignment.3.qml" << "listAssignment.3.errors.txt" << false;

    QTest::newRow("invalidID.1") << "invalidID.qml" << "invalidID.errors.txt" << false;
    QTest::newRow("invalidID.2") << "invalidID.2.qml" << "invalidID.2.errors.txt" << false;
    QTest::newRow("invalidID.3") << "invalidID.3.qml" << "invalidID.3.errors.txt" << false;
    QTest::newRow("invalidID.4") << "invalidID.4.qml" << "invalidID.4.errors.txt" << false;
    QTest::newRow("invalidID.5") << "invalidID.5.qml" << "invalidID.5.errors.txt" << false;
    QTest::newRow("invalidID.6") << "invalidID.6.qml" << "invalidID.6.errors.txt" << false;
    QTest::newRow("invalidID.7") << "invalidID.7.qml" << "invalidID.7.errors.txt" << false;
    QTest::newRow("invalidID.8") << "invalidID.8.qml" << "invalidID.8.errors.txt" << false;
    QTest::newRow("invalidID.9") << "invalidID.9.qml" << "invalidID.9.errors.txt" << false;
    QTest::newRow("invalidID.10") << "invalidID.10.qml" << "invalidID.10.errors.txt" << false;

    QTest::newRow("scriptString.1") << "scriptString.1.qml" << "scriptString.1.errors.txt" << false;
    QTest::newRow("scriptString.2") << "scriptString.2.qml" << "scriptString.2.errors.txt" << false;

    QTest::newRow("unsupportedProperty") << "unsupportedProperty.qml" << "unsupportedProperty.errors.txt" << false;
    QTest::newRow("nullDotProperty") << "nullDotProperty.qml" << "nullDotProperty.errors.txt" << true;
    QTest::newRow("fakeDotProperty") << "fakeDotProperty.qml" << "fakeDotProperty.errors.txt" << false;
    QTest::newRow("duplicateIDs") << "duplicateIDs.qml" << "duplicateIDs.errors.txt" << false;
    QTest::newRow("unregisteredObject") << "unregisteredObject.qml" << "unregisteredObject.errors.txt" << false;
    QTest::newRow("empty") << "empty.qml" << "empty.errors.txt" << false;
    QTest::newRow("missingObject") << "missingObject.qml" << "missingObject.errors.txt" << false;
    QTest::newRow("failingComponent") << "failingComponentTest.qml" << "failingComponent.errors.txt" << false;
    QTest::newRow("missingSignal") << "missingSignal.qml" << "missingSignal.errors.txt" << false;
    QTest::newRow("missingSignal2") << "missingSignal.2.qml" << "missingSignal.2.errors.txt" << false;
    QTest::newRow("finalOverride") << "finalOverride.qml" << "finalOverride.errors.txt" << false;
    QTest::newRow("customParserIdNotAllowed") << "customParserIdNotAllowed.qml" << "customParserIdNotAllowed.errors.txt" << false;

    QTest::newRow("nullishCoalescing_LHS_Or") << "nullishCoalescing_LHS_Or.qml" << "nullishCoalescing_LHS_Or.errors.txt" << false;
    QTest::newRow("nullishCoalescing_LHS_And") << "nullishCoalescing_LHS_And.qml" << "nullishCoalescing_LHS_And.errors.txt" << false;
    QTest::newRow("nullishCoalescing_RHS_Or") << "nullishCoalescing_RHS_Or.qml" << "nullishCoalescing_RHS_Or.errors.txt" << false;
    QTest::newRow("nullishCoalescing_RHS_And") << "nullishCoalescing_RHS_And.qml" << "nullishCoalescing_RHS_And.errors.txt" << false;

    QTest::newRow("questionDotEOF") << "questionDotEOF.qml" << "questionDotEOF.errors.txt" << false;
    QTest::newRow("optionalChaining.LHS") << "optionalChaining.LHS.qml" << "optionalChaining.LHS.errors.txt" << false;


    QTest::newRow("invalidGroupedProperty.1") << "invalidGroupedProperty.1.qml" << "invalidGroupedProperty.1.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.2") << "invalidGroupedProperty.2.qml" << "invalidGroupedProperty.2.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.3") << "invalidGroupedProperty.3.qml" << "invalidGroupedProperty.3.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.4") << "invalidGroupedProperty.4.qml" << "invalidGroupedProperty.4.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.5") << "invalidGroupedProperty.5.qml" << "invalidGroupedProperty.5.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.6") << "invalidGroupedProperty.6.qml" << "invalidGroupedProperty.6.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.7") << "invalidGroupedProperty.7.qml" << "invalidGroupedProperty.7.errors.txt" << true;
    QTest::newRow("invalidGroupedProperty.8") << "invalidGroupedProperty.8.qml" << "invalidGroupedProperty.8.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.9") << "invalidGroupedProperty.9.qml" << "invalidGroupedProperty.9.errors.txt" << false;
    QTest::newRow("invalidGroupedProperty.10") << "invalidGroupedProperty.10.qml" << "invalidGroupedProperty.10.errors.txt" << false;

    QTest::newRow("importNamespaceConflict") << "importNamespaceConflict.qml" << "importNamespaceConflict.errors.txt" << false;
    QTest::newRow("importVersionMissing (builtin)") << "importVersionMissingBuiltIn.qml" << "importVersionMissingBuiltIn.errors.txt" << false;
    QTest::newRow("importVersionMissing (installed)") << "importVersionMissingInstalled.qml" << "importVersionMissingInstalled.errors.txt" << false;
    QTest::newRow("importNonExist (installed)") << "importNonExist.qml" << "importNonExist.errors.txt" << false;
    QTest::newRow("importNonExistOlder (installed)") << "importNonExistOlder.qml" << "importNonExistOlder.errors.txt" << false;
    QTest::newRow("importNewerVersion (installed)") << "importNewerVersion.qml" << "importNewerVersion.errors.txt" << false;
    QTest::newRow("invalidImportID") << "invalidImportID.qml" << "invalidImportID.errors.txt" << false;
    QTest::newRow("importFile") << "importFile.qml" << "importFile.errors.txt" << false;

    QTest::newRow("signal.1") << "signal.1.qml" << "signal.1.errors.txt" << false;
    QTest::newRow("signal.2") << "signal.2.qml" << "signal.2.errors.txt" << false;
    QTest::newRow("signal.3") << "signal.3.qml" << "signal.3.errors.txt" << false;
    QTest::newRow("signal.4") << "signal.4.qml" << "signal.4.errors.txt" << false;
    QTest::newRow("signal.5") << "signal.5.qml" << "signal.5.errors.txt" << false;
    QTest::newRow("signal.6") << "signal.6.qml" << "signal.6.errors.txt" << false;

    QTest::newRow("method.1") << "method.1.qml" << "method.1.errors.txt" << false;

    QTest::newRow("property.1") << "property.1.qml" << "property.1.errors.txt" << false;
    QTest::newRow("property.2") << "property.2.qml" << "property.2.errors.txt" << false;
    QTest::newRow("property.3") << "property.3.qml" << "property.3.errors.txt" << false;
    QTest::newRow("property.4") << "property.4.qml" << "property.4.errors.txt" << false;
    QTest::newRow("property.6") << "property.6.qml" << "property.6.errors.txt" << false;
    QTest::newRow("property.7") << "property.7.qml" << "property.7.errors.txt" << false;

    QTest::newRow("importScript.1") << "importscript.1.qml" << "importscript.1.errors.txt" << false;

    QTest::newRow("Component.1") << "component.1.qml" << "component.1.errors.txt" << false;
    QTest::newRow("Component.2") << "component.2.qml" << "component.2.errors.txt" << false;
    QTest::newRow("Component.3") << "component.3.qml" << "component.3.errors.txt" << false;
    QTest::newRow("Component.4") << "component.4.qml" << "component.4.errors.txt" << false;
    QTest::newRow("Component.5") << "component.5.qml" << "component.5.errors.txt" << false;
    QTest::newRow("Component.6") << "component.6.qml" << "component.6.errors.txt" << false;
    QTest::newRow("Component.7") << "component.7.qml" << "component.7.errors.txt" << false;
    QTest::newRow("Component.8") << "component.8.qml" << "component.8.errors.txt" << false;
    QTest::newRow("Component.9") << "component.9.qml" << "component.9.errors.txt" << false;

    QTest::newRow("MultiSet.1") << "multiSet.1.qml" << "multiSet.1.errors.txt" << false;
    QTest::newRow("MultiSet.2") << "multiSet.2.qml" << "multiSet.2.errors.txt" << false;
    QTest::newRow("MultiSet.3") << "multiSet.3.qml" << "multiSet.3.errors.txt" << false;
    QTest::newRow("MultiSet.4") << "multiSet.4.qml" << "multiSet.4.errors.txt" << false;
    QTest::newRow("MultiSet.5") << "multiSet.5.qml" << "multiSet.5.errors.txt" << false;
    QTest::newRow("MultiSet.6") << "multiSet.6.qml" << "multiSet.6.errors.txt" << false;
    QTest::newRow("MultiSet.7") << "multiSet.7.qml" << "multiSet.7.errors.txt" << false;
    QTest::newRow("MultiSet.8") << "multiSet.8.qml" << "multiSet.8.errors.txt" << false;
    QTest::newRow("MultiSet.9") << "multiSet.9.qml" << "multiSet.9.errors.txt" << false;
    QTest::newRow("MultiSet.10") << "multiSet.10.qml" << "multiSet.10.errors.txt" << false;
    QTest::newRow("MultiSet.11") << "multiSet.11.qml" << "multiSet.11.errors.txt" << false;

    QTest::newRow("dynamicMeta.1") << "dynamicMeta.1.qml" << "dynamicMeta.1.errors.txt" << false;
    QTest::newRow("dynamicMeta.2") << "dynamicMeta.2.qml" << "dynamicMeta.2.errors.txt" << false;
    QTest::newRow("dynamicMeta.3") << "dynamicMeta.3.qml" << "dynamicMeta.3.errors.txt" << false;
    QTest::newRow("dynamicMeta.4") << "dynamicMeta.4.qml" << "dynamicMeta.4.errors.txt" << false;
    QTest::newRow("dynamicMeta.5") << "dynamicMeta.5.qml" << "dynamicMeta.5.errors.txt" << false;

    QTest::newRow("invalidAlias.1") << "invalidAlias.1.qml" << "invalidAlias.1.errors.txt" << false;
    QTest::newRow("invalidAlias.2") << "invalidAlias.2.qml" << "invalidAlias.2.errors.txt" << false;
    QTest::newRow("invalidAlias.3") << "invalidAlias.3.qml" << "invalidAlias.3.errors.txt" << false;
    QTest::newRow("invalidAlias.4") << "invalidAlias.4.qml" << "invalidAlias.4.errors.txt" << false;
    QTest::newRow("invalidAlias.5") << "invalidAlias.5.qml" << "invalidAlias.5.errors.txt" << false;
    QTest::newRow("invalidAlias.6") << "invalidAlias.6.qml" << "invalidAlias.6.errors.txt" << false;
    QTest::newRow("invalidAlias.7") << "invalidAlias.7.qml" << "invalidAlias.7.errors.txt" << false;
    QTest::newRow("invalidAlias.8") << "invalidAlias.8.qml" << "invalidAlias.8.errors.txt" << false;
    QTest::newRow("invalidAlias.9") << "invalidAlias.9.qml" << "invalidAlias.9.errors.txt" << false;
    QTest::newRow("invalidAlias.10") << "invalidAlias.10.qml" << "invalidAlias.10.errors.txt" << false;
    QTest::newRow("invalidAlias.11") << "invalidAlias.11.qml" << "invalidAlias.11.errors.txt" << false;
    QTest::newRow("invalidAlias.12") << "invalidAlias.12.qml" << "invalidAlias.12.errors.txt" << false;
    QTest::newRow("invalidAlias.13") << "invalidAlias.13.qml" << "invalidAlias.13.errors.txt" << false;

    QTest::newRow("invalidAttachedProperty.1") << "invalidAttachedProperty.1.qml" << "invalidAttachedProperty.1.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.2") << "invalidAttachedProperty.2.qml" << "invalidAttachedProperty.2.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.3") << "invalidAttachedProperty.3.qml" << "invalidAttachedProperty.3.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.4") << "invalidAttachedProperty.4.qml" << "invalidAttachedProperty.4.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.5") << "invalidAttachedProperty.5.qml" << "invalidAttachedProperty.5.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.6") << "invalidAttachedProperty.6.qml" << "invalidAttachedProperty.6.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.7") << "invalidAttachedProperty.7.qml" << "invalidAttachedProperty.7.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.8") << "invalidAttachedProperty.8.qml" << "invalidAttachedProperty.8.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.9") << "invalidAttachedProperty.9.qml" << "invalidAttachedProperty.9.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.10") << "invalidAttachedProperty.10.qml" << "invalidAttachedProperty.10.errors.txt" << false;
    QTest::newRow("invalidAttachedProperty.11") << "invalidAttachedProperty.11.qml" << "invalidAttachedProperty.11.errors.txt" << false;

    QTest::newRow("assignValueToSignal") << "assignValueToSignal.qml" << "assignValueToSignal.errors.txt" << false;
    QTest::newRow("emptySignal") << "emptySignal.qml" << "emptySignal.errors.txt" << false;

    QTest::newRow("nestedErrors") << "nestedErrors.qml" << "nestedErrors.errors.txt" << false;
    QTest::newRow("defaultGrouped") << "defaultGrouped.qml" << "defaultGrouped.errors.txt" << false;
    QTest::newRow("doubleSignal") << "doubleSignal.qml" << "doubleSignal.errors.txt" << false;
    QTest::newRow("missingValueTypeProperty") << "missingValueTypeProperty.qml" << "missingValueTypeProperty.errors.txt" << false;
    QTest::newRow("objectValueTypeProperty") << "objectValueTypeProperty.qml" << "objectValueTypeProperty.errors.txt" << false;
    QTest::newRow("enumTypes") << "enumTypes.qml" << "enumTypes.errors.txt" << false;
    QTest::newRow("noCreation") << "noCreation.qml" << "noCreation.errors.txt" << false;
    QTest::newRow("destroyedSignal") << "destroyedSignal.qml" << "destroyedSignal.errors.txt" << false;
    QTest::newRow("assignToNamespace") << "assignToNamespace.qml" << "assignToNamespace.errors.txt" << false;
    QTest::newRow("invalidOn") << "invalidOn.qml" << "invalidOn.errors.txt" << false;
    QTest::newRow("invalidProperty") << "invalidProperty.qml" << "invalidProperty.errors.txt" << false;
    QTest::newRow("nonScriptableProperty") << "nonScriptableProperty.qml" << "nonScriptableProperty.errors.txt" << false;
    QTest::newRow("notAvailable") << "notAvailable.qml" << "notAvailable.errors.txt" << false;
    QTest::newRow("singularProperty") << "singularProperty.qml" << "singularProperty.errors.txt" << false;
    QTest::newRow("singularProperty.2") << "singularProperty.2.qml" << "singularProperty.2.errors.txt" << false;

    QTest::newRow("scopedEnumList") << "scopedEnumList.qml" << "scopedEnumList.errors.txt" << false;
    QTest::newRow("lowercase enum value") << "lowercaseQmlEnum.1.qml" << "lowercaseQmlEnum.1.errors.txt" << false;
    QTest::newRow("lowercase enum type") << "lowercaseQmlEnum.2.qml" << "lowercaseQmlEnum.2.errors.txt" << false;
    QTest::newRow("string enum value") << "invalidQmlEnumValue.1.qml" << "invalidQmlEnumValue.1.errors.txt" << false;
    QTest::newRow("identifier enum type") << "invalidQmlEnumValue.2.qml" << "invalidQmlEnumValue.2.errors.txt" << false;
    QTest::newRow("enum value too large") << "invalidQmlEnumValue.3.qml" << "invalidQmlEnumValue.3.errors.txt" << false;
    QTest::newRow("non-integer enum value") << "invalidQmlEnumValue.4.qml" << "invalidQmlEnumValue.4.errors.txt" << false;

    const QString expectedError = isCaseSensitiveFileSystem(dataDirectory()) ?
        QStringLiteral("incorrectCase.errors.sensitive.txt") :
        QStringLiteral("incorrectCase.errors.insensitive.txt");
    QTest::newRow("incorrectCase") << "incorrectCase.qml" << expectedError << false;

    QTest::newRow("metaobjectRevision.1") << "metaobjectRevision.1.qml" << "metaobjectRevision.1.errors.txt" << false;
    QTest::newRow("metaobjectRevision.2") << "metaobjectRevision.2.qml" << "metaobjectRevision.2.errors.txt" << false;
    QTest::newRow("metaobjectRevision.3") << "metaobjectRevision.3.qml" << "metaobjectRevision.3.errors.txt" << false;

    QTest::newRow("invalidRoot.1") << "invalidRoot.1.qml" << "invalidRoot.1.errors.txt" << false;
    QTest::newRow("invalidRoot.2") << "invalidRoot.2.qml" << "invalidRoot.2.errors.txt" << false;
    QTest::newRow("invalidRoot.3") << "invalidRoot.3.qml" << "invalidRoot.3.errors.txt" << false;
    QTest::newRow("invalidRoot.4") << "invalidRoot.4.qml" << "invalidRoot.4.errors.txt" << false;

    QTest::newRow("invalidTypeName.1") << "invalidTypeName.1.qml" << "invalidTypeName.1.errors.txt" << false;
    QTest::newRow("invalidTypeName.2") << "invalidTypeName.2.qml" << "invalidTypeName.2.errors.txt" << false;
    QTest::newRow("invalidTypeName.3") << "invalidTypeName.3.qml" << "invalidTypeName.3.errors.txt" << false;
    QTest::newRow("invalidTypeName.4") << "invalidTypeName.4.qml" << "invalidTypeName.4.errors.txt" << false;

    QTest::newRow("Major version isolation") << "majorVersionIsolation.qml" << "majorVersionIsolation.errors.txt" << false;

    QTest::newRow("badCompositeRegistration.1") << "badCompositeRegistration.1.qml" << "badCompositeRegistration.1.errors.txt" << false;
    QTest::newRow("badCompositeRegistration.2") << "badCompositeRegistration.2.qml" << "badCompositeRegistration.2.errors.txt" << false;

    QTest::newRow("assignComponentToWrongType") << "assignComponentToWrongType.qml" << "assignComponentToWrongType.errors.txt" << false;
    QTest::newRow("cyclicAlias") << "cyclicAlias.qml" << "cyclicAlias.errors.txt" << false;

    QTest::newRow("fuzzed.1") << "fuzzed.1.qml" << "fuzzed.1.errors.txt" << false;
    QTest::newRow("fuzzed.2") << "fuzzed.2.qml" << "fuzzed.2.errors.txt" << false;
    QTest::newRow("fuzzed.3") << "fuzzed.3.qml" << "fuzzed.3.errors.txt" << false;

    QTest::newRow("bareQmlImport") << "bareQmlImport.qml" << "bareQmlImport.errors.txt" << false;

    QTest::newRow("typeAnnotations.2") << "typeAnnotations.2.qml" << "typeAnnotations.2.errors.txt" << false;

    QTest::newRow("propertyUnknownType") << "propertyUnknownType.qml" << "propertyUnknownType.errors.txt" << false;

    QTest::newRow("selfInstantiation") << "SelfInstantiation.qml" << "SelfInstantiation.errors.txt" << false;
}

void tst_qqmllanguage::errors()
{
#ifdef Q_OS_ANDROID
    if (qstrcmp(QTest::currentDataTag(), "fuzzed.2") == 0) {
        QSKIP("Gives different errors on Android");
        /* Only gives one error on Android:

            qrc:/data/fuzzed.2.qml:1:1: "
            import"
            ^
        So, it seems to complain about the first import (which is understandable)
        */
    }
#endif
    QFETCH(QString, file);
    QFETCH(QString, errorFile);
    QFETCH(bool, create);

    QQmlComponent component(&engine, testFileUrl(file));
    QTRY_VERIFY(!component.isLoading());

    QScopedPointer<QObject> object;

    if (create) {
        object.reset(component.create());
        QVERIFY(object.isNull());
    }

    VERIFY_ERRORS(errorFile.toLatin1().constData());
}

void tst_qqmllanguage::simpleObject()
{
    QQmlComponent component(&engine, testFileUrl("simpleObject.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

void tst_qqmllanguage::simpleContainer()
{
    QQmlComponent component(&engine, testFileUrl("simpleContainer.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyContainer> container(qobject_cast<MyContainer*>(component.create()));
    QVERIFY(container != nullptr);
    QCOMPARE(container->getChildren()->size(),2);
}

void tst_qqmllanguage::interfaceProperty()
{
    QQmlComponent component(&engine, testFileUrl("interfaceProperty.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject*>(component.create()));
    QVERIFY(object != nullptr);
    QVERIFY(object->interface());
    QCOMPARE(object->interface()->id, 913);
}

void tst_qqmllanguage::interfaceQList()
{
    QQmlComponent component(&engine, testFileUrl("interfaceQList.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyContainer> container(qobject_cast<MyContainer*>(component.create()));
    QVERIFY(container != nullptr);
    QCOMPARE(container->getQListInterfaces()->size(), 2);
    for(int ii = 0; ii < 2; ++ii)
        QCOMPARE(container->getQListInterfaces()->at(ii)->id, 913);
}

void tst_qqmllanguage::assignObjectToSignal()
{
    QQmlComponent component(&engine, testFileUrl("assignObjectToSignal.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);
    QTest::ignoreMessage(QtWarningMsg, "MyQmlObject::basicSlot");
    emit object->basicSignal();
}

void tst_qqmllanguage::assignObjectToVariant()
{
    QQmlComponent component(&engine, testFileUrl("assignObjectToVariant.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVariant v = object->property("a");
    QVERIFY(v.typeId() == qMetaTypeId<QObject *>());
}

void tst_qqmllanguage::assignLiteralSignalProperty()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralSignalProperty.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->onLiteralSignal(), 10);
}

// Test is an external component can be loaded and assigned (to a qlist)
void tst_qqmllanguage::assignQmlComponent()
{
    QQmlComponent component(&engine, testFileUrl("assignQmlComponent.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyContainer> object(qobject_cast<MyContainer *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->getChildren()->size(), 1);
    QObject *child = object->getChildren()->at(0);
    QCOMPARE(child->property("x"), QVariant(10));
    QCOMPARE(child->property("y"), QVariant(11));
}

// Test literal assignment to all the value types
void tst_qqmllanguage::assignValueTypes()
{
    QQmlComponent component(&engine, testFileUrl("assignValueTypes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->flagProperty(), MyTypeObject::FlagVal1 | MyTypeObject::FlagVal3);
    QCOMPARE(object->enumProperty(), MyTypeObject::EnumVal2);
    QCOMPARE(object->qtEnumProperty(), Qt::RichText);
    QCOMPARE(object->mirroredEnumProperty(), MyTypeObject::MirroredEnumVal3);
    QCOMPARE(object->relatedEnumProperty(), MyEnumContainer::RelatedValue);
    QCOMPARE(object->stringProperty(), QString("Hello World!"));
    QCOMPARE(object->uintProperty(), uint(10));
    QCOMPARE(object->intProperty(), -19);
    QCOMPARE((float)object->realProperty(), float(23.2));
    QCOMPARE((float)object->doubleProperty(), float(-19.7));
    QCOMPARE((float)object->floatProperty(), float(8.5));
    QCOMPARE(object->colorProperty(), QColor("red"));
    QCOMPARE(object->dateProperty(), QDate(1982, 11, 25));
    QCOMPARE(object->timeProperty(), QTime(11, 11, 32));
    QCOMPARE(object->dateTimeProperty(), QDateTime(QDate(2009, 5, 12), QTime(13, 22, 1)));
    QCOMPARE(object->pointProperty(), QPoint(99,13));
    QCOMPARE(object->pointFProperty(), QPointF(-10.1, 12.3));
    QCOMPARE(object->sizeProperty(), QSize(99, 13));
    QCOMPARE(object->sizeFProperty(), QSizeF(0.1, 0.2));
    QCOMPARE(object->rectProperty(), QRect(9, 7, 100, 200));
    QCOMPARE(object->rectFProperty(), QRectF(1000.1, -10.9, 400, 90.99));
    QCOMPARE(object->boolProperty(), true);
    QCOMPARE(object->variantProperty(), QVariant("Hello World!"));
    QCOMPARE(object->vectorProperty(), QVector3D(10, 1, 2.2f));
    QCOMPARE(object->vector2Property(), QVector2D(2, 3));
    QCOMPARE(object->vector4Property(), QVector4D(10, 1, 2.2f, 2.3f));
    const QUrl encoded = QUrl::fromEncoded("main.qml?with%3cencoded%3edata", QUrl::TolerantMode);
    QCOMPARE(object->urlProperty(), encoded);
    QVERIFY(object->objectProperty() != nullptr);
    MyTypeObject *child = qobject_cast<MyTypeObject *>(object->objectProperty());
    QVERIFY(child != nullptr);
    QCOMPARE(child->intProperty(), 8);

    //these used to go via script. Ensure they no longer do
    QCOMPARE(object->property("qtEnumTriggeredChange").toBool(), false);
    QCOMPARE(object->property("mirroredEnumTriggeredChange").toBool(), false);
}

// Test edge case type assignments
void tst_qqmllanguage::assignTypeExtremes()
{
    QQmlComponent component(&engine, testFileUrl("assignTypeExtremes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->uintProperty(), 0xEE6B2800);
    QCOMPARE(object->intProperty(), -0x77359400);
}

// Test that a composite type can assign to a property of its base type
void tst_qqmllanguage::assignCompositeToType()
{
    QQmlComponent component(&engine, testFileUrl("assignCompositeToType.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

// Test that literals are stored correctly in "var" properties
// Note that behaviour differs from "variant" properties in that
// no conversion from "special strings" to QVariants is performed.
void tst_qqmllanguage::assignLiteralToVar()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralToVar.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QVERIFY(isJSNumberType(object->property("test1").typeId()));
    QCOMPARE(object->property("test2").typeId(), (int)QMetaType::Double);
    QCOMPARE(object->property("test3").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test4").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test5").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test6").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test7").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test8").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test9").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test10").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test11").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test12").typeId(), QMetaType::QColor);
    QCOMPARE(object->property("test13").typeId(), QMetaType::QRectF);
    QCOMPARE(object->property("test14").typeId(), QMetaType::QPointF);
    QCOMPARE(object->property("test15").typeId(), QMetaType::QSizeF);
    QCOMPARE(object->property("test16").typeId(), QMetaType::QVector3D);
    QVERIFY(isJSNumberType(object->property("variantTest1Bound").typeId()));
    QVERIFY(isJSNumberType(object->property("test1Bound").typeId()));

    QCOMPARE(object->property("test1"), QVariant(5));
    QCOMPARE(object->property("test2"), QVariant((double)1.7));
    QCOMPARE(object->property("test3"), QVariant(QString(QLatin1String("Hello world!"))));
    QCOMPARE(object->property("test4"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test5"), QVariant(QString(QLatin1String("10,10,10x10"))));
    QCOMPARE(object->property("test6"), QVariant(QString(QLatin1String("10,10"))));
    QCOMPARE(object->property("test7"), QVariant(QString(QLatin1String("10x10"))));
    QCOMPARE(object->property("test8"), QVariant(QString(QLatin1String("100,100,100"))));
    QCOMPARE(object->property("test9"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test10"), QVariant(bool(true)));
    QCOMPARE(object->property("test11"), QVariant(bool(false)));
    QCOMPARE(object->property("test12"), QVariant(QColor::fromRgbF(0.2f, 0.3f, 0.4f, 0.5f)));
    QCOMPARE(object->property("test13"), QVariant(QRectF(10, 10, 10, 10)));
    QCOMPARE(object->property("test14"), QVariant(QPointF(10, 10)));
    QCOMPARE(object->property("test15"), QVariant(QSizeF(10, 10)));
    QCOMPARE(object->property("test16"), QVariant(QVector3D(100, 100, 100)));
    QCOMPARE(object->property("variantTest1Bound"), QVariant(9));
    QCOMPARE(object->property("test1Bound"), QVariant(11));
}

void tst_qqmllanguage::assignLiteralToJSValue()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralToJSValue.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test1");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(5));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test2");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(1.7));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test3");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("Hello world!")));
    }{
        MyQmlObject *object = root->findChild<MyQmlObject *>("test4");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("#FF008800")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test5");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("10,10,10x10")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test6");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("10,10")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test7");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("10x10")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test8");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("100,100,100")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test9");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("#FF008800")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test10");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isBool());
        QCOMPARE(value.toBool(), true);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test11");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isBool());
        QCOMPARE(value.toBool(), false);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test20");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isCallable());
        QCOMPARE(value.call(QList<QJSValue> () << QJSValue(4)).toInt(), 12);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test21");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isUndefined());
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test22");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNull());
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test1Bound");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(9));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test20Bound");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(27));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("test23");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isQObject());
        QCOMPARE(value.toQObject()->objectName(), "blah");
    }
}

void tst_qqmllanguage::assignNullStrings()
{
    QQmlComponent component(&engine, testFileUrl("assignNullStrings.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);
    QVERIFY(object->stringProperty().isNull());
    QVERIFY(object->byteArrayProperty().isNull());
    QMetaObject::invokeMethod(object.data(), "assignNullStringsFromJs", Qt::DirectConnection);
    QVERIFY(object->stringProperty().isNull());
    QVERIFY(object->byteArrayProperty().isNull());
}

void tst_qqmllanguage::bindJSValueToVar()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralToJSValue.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    QObject *object = root->findChild<QObject *>("varProperties");

    QVERIFY(isJSNumberType(object->property("test1").typeId()));
    QVERIFY(isJSNumberType(object->property("test2").typeId()));
    QCOMPARE(object->property("test3").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test4").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test5").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test6").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test7").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test8").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test9").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test10").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test11").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test12").typeId(), QMetaType::QColor);
    QCOMPARE(object->property("test13").typeId(), QMetaType::QRectF);
    QCOMPARE(object->property("test14").typeId(), QMetaType::QPointF);
    QCOMPARE(object->property("test15").typeId(), QMetaType::QSizeF);
    QCOMPARE(object->property("test16").typeId(), QMetaType::QVector3D);
    QVERIFY(isJSNumberType(object->property("test1Bound").typeId()));
    QVERIFY(isJSNumberType(object->property("test20Bound").typeId()));

    QCOMPARE(object->property("test1"), QVariant(5));
    QCOMPARE(object->property("test2"), QVariant((double)1.7));
    QCOMPARE(object->property("test3"), QVariant(QString(QLatin1String("Hello world!"))));
    QCOMPARE(object->property("test4"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test5"), QVariant(QString(QLatin1String("10,10,10x10"))));
    QCOMPARE(object->property("test6"), QVariant(QString(QLatin1String("10,10"))));
    QCOMPARE(object->property("test7"), QVariant(QString(QLatin1String("10x10"))));
    QCOMPARE(object->property("test8"), QVariant(QString(QLatin1String("100,100,100"))));
    QCOMPARE(object->property("test9"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test10"), QVariant(bool(true)));
    QCOMPARE(object->property("test11"), QVariant(bool(false)));
    QCOMPARE(object->property("test12"), QVariant(QColor::fromRgbF(0.2f, 0.3f, 0.4f, 0.5f)));
    QCOMPARE(object->property("test13"), QVariant(QRectF(10, 10, 10, 10)));
    QCOMPARE(object->property("test14"), QVariant(QPointF(10, 10)));
    QCOMPARE(object->property("test15"), QVariant(QSizeF(10, 10)));
    QCOMPARE(object->property("test16"), QVariant(QVector3D(100, 100, 100)));
    QCOMPARE(object->property("test1Bound"), QVariant(9));
    QCOMPARE(object->property("test20Bound"), QVariant(27));
}

void tst_qqmllanguage::bindJSValueToVariant()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralToJSValue.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    QObject *object = root->findChild<QObject *>("variantProperties");

    QVERIFY(isJSNumberType(object->property("test1").typeId()));
    QVERIFY(isJSNumberType(object->property("test2").typeId()));
    QCOMPARE(object->property("test3").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test4").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test5").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test6").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test7").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test8").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test9").typeId(), QMetaType::QString);
    QCOMPARE(object->property("test10").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test11").typeId(), QMetaType::Bool);
    QCOMPARE(object->property("test12").typeId(), QMetaType::QColor);
    QCOMPARE(object->property("test13").typeId(), QMetaType::QRectF);
    QCOMPARE(object->property("test14").typeId(), QMetaType::QPointF);
    QCOMPARE(object->property("test15").typeId(), QMetaType::QSizeF);
    QCOMPARE(object->property("test16").typeId(), QMetaType::QVector3D);
    QVERIFY(isJSNumberType(object->property("test1Bound").typeId()));
    QVERIFY(isJSNumberType(object->property("test20Bound").typeId()));

    QCOMPARE(object->property("test1"), QVariant(5));
    QCOMPARE(object->property("test2"), QVariant((double)1.7));
    QCOMPARE(object->property("test3"), QVariant(QString(QLatin1String("Hello world!"))));
    QCOMPARE(object->property("test4"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test5"), QVariant(QString(QLatin1String("10,10,10x10"))));
    QCOMPARE(object->property("test6"), QVariant(QString(QLatin1String("10,10"))));
    QCOMPARE(object->property("test7"), QVariant(QString(QLatin1String("10x10"))));
    QCOMPARE(object->property("test8"), QVariant(QString(QLatin1String("100,100,100"))));
    QCOMPARE(object->property("test9"), QVariant(QString(QLatin1String("#FF008800"))));
    QCOMPARE(object->property("test10"), QVariant(bool(true)));
    QCOMPARE(object->property("test11"), QVariant(bool(false)));
    QCOMPARE(object->property("test12"), QVariant(QColor::fromRgbF(0.2f, 0.3f, 0.4f, 0.5f)));
    QCOMPARE(object->property("test13"), QVariant(QRectF(10, 10, 10, 10)));
    QCOMPARE(object->property("test14"), QVariant(QPointF(10, 10)));
    QCOMPARE(object->property("test15"), QVariant(QSizeF(10, 10)));
    QCOMPARE(object->property("test16"), QVariant(QVector3D(100, 100, 100)));
    QCOMPARE(object->property("test1Bound"), QVariant(9));
    QCOMPARE(object->property("test20Bound"), QVariant(27));
}

void tst_qqmllanguage::bindJSValueToType()
{
    QQmlComponent component(&engine, testFileUrl("assignLiteralToJSValue.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    {
        MyTypeObject *object = root->findChild<MyTypeObject *>("typedProperties");

        QCOMPARE(object->intProperty(), 5);
        QCOMPARE(object->doubleProperty(), double(1.7));
        QCOMPARE(object->stringProperty(), QString(QLatin1String("Hello world!")));
        QCOMPARE(object->boolProperty(), true);
        QCOMPARE(object->colorProperty(), QColor::fromRgbF(0.2f, 0.3f, 0.4f, 0.5f));
        QCOMPARE(object->rectFProperty(), QRectF(10, 10, 10, 10));
        QCOMPARE(object->pointFProperty(), QPointF(10, 10));
        QCOMPARE(object->sizeFProperty(), QSizeF(10, 10));
        QCOMPARE(object->vectorProperty(), QVector3D(100, 100, 100));
    } {
        MyTypeObject *object = root->findChild<MyTypeObject *>("stringProperties");

        QCOMPARE(object->intProperty(), 1);
        QCOMPARE(object->doubleProperty(), double(1.7));
        QCOMPARE(object->stringProperty(), QString(QLatin1String("Hello world!")));
        QCOMPARE(object->boolProperty(), true);
        QCOMPARE(object->colorProperty(), QColor::fromRgb(0x00, 0x88, 0x00, 0xFF));
        QCOMPARE(object->rectFProperty(), QRectF(10, 10, 10, 10));
        QCOMPARE(object->pointFProperty(), QPointF(10, 10));
        QCOMPARE(object->sizeFProperty(), QSizeF(10, 10));
        QCOMPARE(object->vectorProperty(), QVector3D(100, 100, 100));
    }
}

void tst_qqmllanguage::bindTypeToJSValue()
{
    QQmlComponent component(&engine, testFileUrl("bindTypeToJSValue.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    {
        MyQmlObject *object = root->findChild<MyQmlObject *>("flagProperty");
        QVERIFY(object);
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(MyTypeObject::FlagVal1 | MyTypeObject::FlagVal3));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("enumProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(MyTypeObject::EnumVal2));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("stringProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("Hello World!")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("uintProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(10));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("intProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(-19));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("realProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(23.2));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("doubleProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(-19.7));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("floatProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isNumber());
        QCOMPARE(value.toNumber(), qreal(8.5));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("colorProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("r")).toNumber(), qreal(1.0));
        QCOMPARE(value.property(QLatin1String("g")).toNumber(), qreal(0.0));
        QCOMPARE(value.property(QLatin1String("b")).toNumber(), qreal(0.0));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("dateProperty");
        QJSValue value = object->qjsvalue();
        QCOMPARE(value.toDateTime().isValid(), true);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("timeProperty");
        QJSValue value = object->qjsvalue();
        QCOMPARE(value.toDateTime().isValid(), true);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("dateTimeProperty");
        QJSValue value = object->qjsvalue();
        QCOMPARE(value.toDateTime().isValid(), true);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("pointProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(99));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(13));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("pointFProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(-10.1));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(12.3));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("rectProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(9));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(7));
        QCOMPARE(value.property(QLatin1String("width")).toNumber(), qreal(100));
        QCOMPARE(value.property(QLatin1String("height")).toNumber(), qreal(200));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("rectFProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(1000.1));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(-10.9));
        QCOMPARE(value.property(QLatin1String("width")).toNumber(), qreal(400));
        QCOMPARE(value.property(QLatin1String("height")).toNumber(), qreal(90.99));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("boolProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isBool());
        QCOMPARE(value.toBool(), true);
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("variantProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("Hello World!")));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("vectorProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(10.0f));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(1.0f));
        QCOMPARE(value.property(QLatin1String("z")).toNumber(), qreal(2.2f));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("vector4Property");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isObject());
        QCOMPARE(value.property(QLatin1String("x")).toNumber(), qreal(10.0f));
        QCOMPARE(value.property(QLatin1String("y")).toNumber(), qreal(1.0f));
        QCOMPARE(value.property(QLatin1String("z")).toNumber(), qreal(2.2f));
        QCOMPARE(value.property(QLatin1String("w")).toNumber(), qreal(2.3f));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("urlProperty");
        QJSValue value = object->qjsvalue();
        const QUrl encoded = QUrl::fromEncoded("main.qml?with%3cencoded%3edata", QUrl::TolerantMode);
        QCOMPARE(value.toString(), encoded.toString());
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("objectProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isQObject());
        QVERIFY(qobject_cast<MyTypeObject *>(value.toQObject()));
    } {
        MyQmlObject *object = root->findChild<MyQmlObject *>("varProperty");
        QJSValue value = object->qjsvalue();
        QVERIFY(value.isString());
        QCOMPARE(value.toString(), QString(QLatin1String("Hello World!")));
    }
}

// Tests that custom parser types can be instantiated
void tst_qqmllanguage::customParserTypes()
{
    QQmlComponent component(&engine, testFileUrl("customParserTypes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("count"), QVariant(2));
}

// Tests that custom pursor types can be instantiated in ICs
void tst_qqmllanguage::customParserTypeInInlineComponent()
{
    QQmlComponent component(&engine, testFileUrl("customParserTypeInIC.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("count"), 2);
}

// Tests that the root item can be a custom component
void tst_qqmllanguage::rootAsQmlComponent()
{
    QQmlComponent component(&engine, testFileUrl("rootAsQmlComponent.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyContainer> object(qobject_cast<MyContainer *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("x"), QVariant(11));
    QCOMPARE(object->getChildren()->size(), 2);
}

void tst_qqmllanguage::rootItemIsComponent()
{
    QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(
                    ".*/rootItemIsComponent\\.qml:3:1: Using a Component as the root of "
                    "a QML document is deprecated: types defined in qml documents are "
                    "automatically wrapped into Components when needed\\."));
    QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(
                    ".*/EvilComponentType\\.qml:3:1: Using a Component as the root of a "
                    "QML document is deprecated: types defined in qml documents are automatically "
                    "wrapped into Components when needed\\."));
    QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(".*/rootItemIsComponent\\.qml:7:36: Using a Component as the root "
                               "of an inline component is deprecated: inline components are "
                               "automatically wrapped into Components when needed\\."));
    QQmlComponent component(&engine, testFileUrl("rootItemIsComponent.qml"));
}

// Tests that components can be specified inline
void tst_qqmllanguage::inlineQmlComponents()
{
    QQmlComponent component(&engine, testFileUrl("inlineQmlComponents.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyContainer> object(qobject_cast<MyContainer *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->getChildren()->size(), 1);
    QQmlComponent *comp = qobject_cast<QQmlComponent *>(object->getChildren()->at(0));
    QVERIFY(comp != nullptr);
    QScopedPointer<MyQmlObject> compObject(qobject_cast<MyQmlObject *>(comp->create()));
    QVERIFY(compObject != nullptr);
    QCOMPARE(compObject->value(), 11);
}

// Tests that types that have an id property have it set
void tst_qqmllanguage::idProperty()
{
    {
        QQmlComponent component(&engine, testFileUrl("idProperty.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<MyContainer> object(qobject_cast<MyContainer *>(component.create()));
        QVERIFY(object != nullptr);
        QCOMPARE(object->getChildren()->size(), 2);
        MyTypeObject *child =
                qobject_cast<MyTypeObject *>(object->getChildren()->at(0));
        QVERIFY(child != nullptr);
        QCOMPARE(child->id(), QString("myObjectId"));
        QCOMPARE(object->property("object"), QVariant::fromValue((QObject *)child));

        child =
                qobject_cast<MyTypeObject *>(object->getChildren()->at(1));
        QVERIFY(child != nullptr);
        QCOMPARE(child->id(), QString("name.with.dots"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("idPropertyMismatch.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());
        QQmlContext *ctx = qmlContext(root.data());
        QVERIFY(ctx);
        QCOMPARE(ctx->nameForObject(root.data()), QStringLiteral("root"));
        QCOMPARE(ctx->objectForName(QStringLiteral("root")), root.data());
    }
}

// Tests automatic connection to notify signals if "onBlahChanged" syntax is used
// even if the notify signal for "blah" is not called "blahChanged"
void tst_qqmllanguage::autoNotifyConnection()
{
    QQmlComponent component(&engine, testFileUrl("autoNotifyConnection.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);
    QMetaProperty prop = object->metaObject()->property(object->metaObject()->indexOfProperty("receivedNotify"));
    QVERIFY(prop.isValid());

    QCOMPARE(prop.read(object.data()), QVariant::fromValue(false));
    object->setPropertyWithNotify(1);
    QCOMPARE(prop.read(object.data()), QVariant::fromValue(true));
}

// Tests that signals can be assigned to
void tst_qqmllanguage::assignSignal()
{
    QQmlComponent component(&engine, testFileUrl("assignSignal.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);
    QTest::ignoreMessage(QtWarningMsg, "MyQmlObject::basicSlot");
    emit object->basicSignal();
    QTest::ignoreMessage(QtWarningMsg, "MyQmlObject::basicSlotWithArgs(9)");
    emit object->basicParameterizedSignal(9);
}

void tst_qqmllanguage::assignSignalFunctionExpression()
{
    QQmlComponent component(&engine, testFileUrl("assignSignalFunctionExpression.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);
    QTest::ignoreMessage(QtWarningMsg, "MyQmlObject::basicSlot");
    emit object->basicSignal();
    QTest::ignoreMessage(QtWarningMsg, "MyQmlObject::basicSlotWithArgs(9)");
    emit object->basicParameterizedSignal(9);
}

void tst_qqmllanguage::overrideSignal_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("override signal with signal") << "overrideSignal.1.qml" << "overrideSignal.1.errors.txt";
    QTest::newRow("override signal with method") << "overrideSignal.2.qml" << "overrideSignal.2.errors.txt";
    QTest::newRow("override signal with property") << "overrideSignal.3.qml" << "";
    QTest::newRow("override signal of alias property with signal") << "overrideSignal.4.qml" << "overrideSignal.4.errors.txt";
    QTest::newRow("override signal of superclass with signal") << "overrideSignal.5.qml" << "overrideSignal.5.errors.txt";
    QTest::newRow("override builtin signal with signal") << "overrideSignal.6.qml" << "overrideSignal.6.errors.txt";
}

void tst_qqmllanguage::overrideSignal()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QQmlComponent component(&engine, testFileUrl(file));
    if (errorFile.isEmpty()) {
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QVERIFY(object->property("success").toBool());
    } else {
        VERIFY_ERRORS(errorFile.toLatin1().constData());
    }
}

// Tests the creation and assignment of dynamic properties
void tst_qqmllanguage::dynamicProperties()
{
    QQmlComponent component(&engine, testFileUrl("dynamicProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QCOMPARE(object->property("intProperty"), QVariant(10));
    QCOMPARE(object->property("boolProperty"), QVariant(false));
    QCOMPARE(object->property("doubleProperty"), QVariant(-10.1));
    QCOMPARE(object->property("realProperty"), QVariant((qreal)-19.9));
    QCOMPARE(object->property("stringProperty"), QVariant("Hello World!"));
    QCOMPARE(object->property("urlProperty"), QVariant(QUrl("main.qml")));
    QCOMPARE(object->property("colorProperty"), QVariant(QColor("red")));
    QVariant date = object->property("dateProperty");
    if (!date.convert(QMetaType(QMetaType::QDate)))
        QFAIL("could not convert to date");
    QCOMPARE(date, QVariant(QDate(1945, 9, 2)));
    QCOMPARE(object->property("varProperty"), QVariant("Hello World!"));
}

// Test that nested types can use dynamic properties
void tst_qqmllanguage::dynamicPropertiesNested()
{
    QQmlComponent component(&engine, testFileUrl("dynamicPropertiesNested.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("super_a").toInt(), 11); // Overridden
    QCOMPARE(object->property("super_c").toInt(), 14); // Inherited
    QCOMPARE(object->property("a").toInt(), 13); // New
    QCOMPARE(object->property("b").toInt(), 12); // New
}

// Tests the creation and assignment to dynamic list properties
void tst_qqmllanguage::listProperties()
{
    QQmlComponent component(&engine, testFileUrl("listProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test").toInt(), 2);
}

// Tests that initializing list properties of a base class does not crash
// (QTBUG-82171)
void tst_qqmllanguage::listPropertiesInheritanceNoCrash()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("listPropertiesChild.qml"));
    QScopedPointer<QObject> object(component.create()); // should not crash
    QVERIFY(object != nullptr);
}

void tst_qqmllanguage::badListItemType()
{
    QQmlComponent component(&engine, testFileUrl("badListItemType.qml"));
    QVERIFY(component.isError());
    VERIFY_ERRORS("badListItemType.errors.txt");
}

// Tests the creation and assignment of dynamic object properties
// ### Not complete
void tst_qqmllanguage::dynamicObjectProperties()
{
    {
    QQmlComponent component(&engine, testFileUrl("dynamicObjectProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("objectProperty"), QVariant::fromValue((QObject*)nullptr));
    QVERIFY(object->property("objectProperty2") != QVariant::fromValue((QObject*)nullptr));
    }
    {
    QQmlComponent component(&engine, testFileUrl("dynamicObjectProperties.2.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QVERIFY(object->property("objectProperty") != QVariant::fromValue((QObject*)nullptr));
    }
}

// Tests the declaration of dynamic signals and slots
void tst_qqmllanguage::dynamicSignalsAndSlots()
{
    QTest::ignoreMessage(QtDebugMsg, "1921");

    QQmlComponent component(&engine, testFileUrl("dynamicSignalsAndSlots.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->metaObject()->indexOfMethod("signal1()") != -1);
    QVERIFY(object->metaObject()->indexOfMethod("signal2()") != -1);
    QVERIFY(object->metaObject()->indexOfMethod("slot1()") != -1);
    QVERIFY(object->metaObject()->indexOfMethod("slot2()") != -1);

    QCOMPARE(object->property("test").toInt(), 0);
    QMetaObject::invokeMethod(object.data(), "slot3", Qt::DirectConnection, Q_ARG(QVariant, QVariant(10)));
    QCOMPARE(object->property("test").toInt(), 10);
}

void tst_qqmllanguage::simpleBindings()
{
    QQmlComponent component(&engine, testFileUrl("simpleBindings.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value1"), QVariant(10));
    QCOMPARE(object->property("value2"), QVariant(10));
    QCOMPARE(object->property("value3"), QVariant(21));
    QCOMPARE(object->property("value4"), QVariant(10));
    QCOMPARE(object->property("objectProperty"), QVariant::fromValue(object.data()));
}

class EvaluationCounter : public QObject
{
    Q_OBJECT
public:
    int counter = 0;
    Q_INVOKABLE void increaseEvaluationCounter() { ++counter; }
};

void tst_qqmllanguage::noDoubleEvaluationForFlushedBindings_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::newRow("order1") << QString("noDoubleEvaluationForFlushedBindings.qml");
    QTest::newRow("order2") << QString("noDoubleEvaluationForFlushedBindings.2.qml");
}

void tst_qqmllanguage::noDoubleEvaluationForFlushedBindings()
{
    QFETCH(QString, fileName);
    QQmlEngine engine;

    EvaluationCounter stats;
    engine.rootContext()->setContextProperty("stats", &stats);

    QQmlComponent component(&engine, testFileUrl(fileName));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(stats.counter, 1);
}

void tst_qqmllanguage::autoComponentCreation()
{
    {
        QQmlComponent component(&engine, testFileUrl("autoComponentCreation.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
        QVERIFY(object != nullptr);
        QVERIFY(object->componentProperty() != nullptr);
        QScopedPointer<MyTypeObject> child(qobject_cast<MyTypeObject *>(object->componentProperty()->create()));
        QVERIFY(child != nullptr);
        QCOMPARE(child->realProperty(), qreal(9));
    }
    {
        QQmlComponent component(&engine, testFileUrl("autoComponentCreation.2.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
        QVERIFY(object != nullptr);
        QVERIFY(object->componentProperty() != nullptr);
        QScopedPointer<MyTypeObject> child(qobject_cast<MyTypeObject *>(object->componentProperty()->create()));
        QVERIFY(child != nullptr);
        QCOMPARE(child->realProperty(), qreal(9));
    }
}

void tst_qqmllanguage::autoComponentCreationInGroupProperty()
{
    QQmlComponent component(&engine, testFileUrl("autoComponentCreationInGroupProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);
    QVERIFY(object->componentProperty() != nullptr);
    QScopedPointer<MyTypeObject> child(qobject_cast<MyTypeObject *>(object->componentProperty()->create()));
    QVERIFY(child != nullptr);
    QCOMPARE(child->realProperty(), qreal(9));
}

void tst_qqmllanguage::propertyValueSource()
{
    {
    QQmlComponent component(&engine, testFileUrl("propertyValueSource.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);

    QList<QObject *> valueSources;
    const QObjectList allChildren = object->findChildren<QObject*>();
    for (QObject *child : allChildren) {
        if (qobject_cast<QQmlPropertyValueSource *>(child))
            valueSources.append(child);
    }

    QCOMPARE(valueSources.size(), 1);
    MyPropertyValueSource *valueSource =
        qobject_cast<MyPropertyValueSource *>(valueSources.at(0));
    QVERIFY(valueSource != nullptr);
    QCOMPARE(valueSource->prop.object(), qobject_cast<QObject*>(object.data()));
    QCOMPARE(valueSource->prop.name(), QString(QLatin1String("intProperty")));
    }

    {
    QQmlComponent component(&engine, testFileUrl("propertyValueSource.2.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);

    QList<QObject *> valueSources;
    const QObjectList allChildren = object->findChildren<QObject*>();
    for (QObject *child : allChildren) {
        if (qobject_cast<QQmlPropertyValueSource *>(child))
            valueSources.append(child);
    }

    QCOMPARE(valueSources.size(), 1);
    MyPropertyValueSource *valueSource =
        qobject_cast<MyPropertyValueSource *>(valueSources.at(0));
    QVERIFY(valueSource != nullptr);
    QCOMPARE(valueSource->prop.object(), qobject_cast<QObject*>(object.data()));
    QCOMPARE(valueSource->prop.name(), QString(QLatin1String("intProperty")));
    }
}

void tst_qqmllanguage::requiredProperty()
{
    QQmlEngine engine;
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.1.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.2.qml"));
        QVERIFY(!component.errors().empty());
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.4.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!component.errors().empty());
        QVERIFY(component.errorString().contains("Required property objectName was not initialized"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.3.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!component.errors().empty());
        QVERIFY(component.errorString().contains("Required property i was not initialized"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.5.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!component.errors().empty());
        QVERIFY(component.errorString().contains("Required property i was not initialized"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.6.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);
    }
    {
        QQmlComponent component(&engine, testFileUrl("requiredProperties.7.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!component.errors().empty());
        QVERIFY(component.errorString().contains("Property blub was marked as required but does not exist"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("RequiredListPropertiesUser.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);
    }
}

class MyClassWithRequiredProperty : public QObject
{
public:
    Q_OBJECT
    Q_PROPERTY(int test MEMBER m_test REQUIRED NOTIFY testChanged)
    Q_SIGNAL void testChanged();
private:
    int m_test;
};

class ChildClassWithoutOwnRequired : public MyClassWithRequiredProperty
{
public:
    Q_OBJECT
    Q_PROPERTY(int test2 MEMBER m_test2 NOTIFY test2Changed)
    Q_SIGNAL void test2Changed();
private:
    int m_test2;
};

class ChildClassWithOwnRequired : public MyClassWithRequiredProperty
{
public:
    Q_OBJECT
    Q_PROPERTY(int test2 MEMBER m_test2 REQUIRED NOTIFY test2Changed)
    Q_SIGNAL void test2Changed();
private:
    int m_test2;
};

void tst_qqmllanguage::requiredPropertyFromCpp_data()
{
    qmlRegisterType<MyClassWithRequiredProperty>("example.org", 1, 0, "MyClass");
    qmlRegisterType<ChildClassWithoutOwnRequired>("example.org", 1, 0, "Child");
    qmlRegisterType<ChildClassWithOwnRequired>("example.org", 1, 0, "Child2");


    QTest::addColumn<QUrl>("setFile");
    QTest::addColumn<QUrl>("notSetFile");
    QTest::addColumn<QString>("errorMessage");
    QTest::addColumn<int>("expectedValue");

    QTest::addRow("direct") << testFileUrl("cppRequiredProperty.qml") << testFileUrl("cppRequiredPropertyNotSet.qml") << QString(":4 Required property test was not initialized\n") << 42;
    QTest::addRow("in parent") << testFileUrl("cppRequiredPropertyInParent.qml") << testFileUrl("cppRequiredPropertyInParentNotSet.qml") << QString(":4 Required property test was not initialized\n") << 42;
    QTest::addRow("in child and parent") << testFileUrl("cppRequiredPropertyInChildAndParent.qml") << testFileUrl("cppRequiredPropertyInChildAndParentNotSet.qml") << QString(":4 Required property test2 was not initialized\n") << 18;
}

void tst_qqmllanguage::requiredPropertyFromCpp()
{
    QQmlEngine engine;
    QFETCH(QUrl, setFile);
    QFETCH(QUrl, notSetFile);
    QFETCH(QString, errorMessage);
    QFETCH(int, expectedValue);
    {
        QQmlComponent comp(&engine, notSetFile);
        QScopedPointer<QObject> o { comp.create() };
        QVERIFY(o.isNull());
        QVERIFY(comp.isError());
        QCOMPARE(comp.errorString(), notSetFile.toString() + errorMessage);
    }
    {
        QQmlComponent comp(&engine, setFile);
        QScopedPointer<QObject> o { comp.create() };
        QVERIFY(!o.isNull());
        QCOMPARE(o->property("test").toInt(), expectedValue);
    }
}

void tst_qqmllanguage::attachedProperties()
{
    QQmlComponent component(&engine, testFileUrl("attachedProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QObject *attached = qmlAttachedPropertiesObject<MyQmlObject>(object.data());
    QVERIFY(attached != nullptr);
    QCOMPARE(attached->property("value"), QVariant(10));
    QCOMPARE(attached->property("value2"), QVariant(13));

    {
        QQmlComponent component(&engine, testFileUrl("attachedPropertyDerived.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QCOMPARE(MyQmlObjectWithAttachedCounter::attachedCount, 1);
    }
}

// Tests non-static object properties
void tst_qqmllanguage::dynamicObjects()
{
    QQmlComponent component(&engine, testFileUrl("dynamicObject.1.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

void tst_qqmllanguage::valueTypes()
{
    QQmlComponent component(&engine, testFileUrl("valueTypes.qml"));
    VERIFY_ERRORS(0);

    const auto bindingLoopRegex = QRegularExpression(".*QML MyTypeObject: Binding loop detected for property \"rectProperty.width\".*");
    QTest::ignoreMessage(QtWarningMsg, bindingLoopRegex);
    QTest::ignoreMessage(QtWarningMsg, bindingLoopRegex);

    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
    QVERIFY(object != nullptr);


    QCOMPARE(object->rectProperty(), QRect(10, 11, 12, 13));
    QCOMPARE(object->rectProperty2(), QRect(10, 11, 12, 13));
    QCOMPARE(object->intProperty(), 10);
    object->doAction();
    QCOMPARE(object->rectProperty(), QRect(12, 11, 14, 13));
    QCOMPARE(object->rectProperty2(), QRect(12, 11, 14, 13));
    QCOMPARE(object->intProperty(), 12);

    // ###
#if 0
    QQmlProperty p(object, "rectProperty.x");
    QCOMPARE(p.read(), QVariant(12));
    p.write(13);
    QCOMPARE(p.read(), QVariant(13));

    quint32 r = QQmlPropertyPrivate::saveValueType(p.coreIndex(), p.valueTypeCoreIndex());
    QQmlProperty p2;
    QQmlPropertyPrivate::restore(p2, r, object);
    QCOMPARE(p2.read(), QVariant(13));
#endif
}

void tst_qqmllanguage::cppnamespace()
{
    QScopedPointer<QObject> object;

    auto create = [&](const char *file) {
        QQmlComponent component(&engine, testFileUrl(file));
        VERIFY_ERRORS(0);
        object.reset(component.create());
        QVERIFY(object != nullptr);
    };

    auto createAndCheck = [&](const char *file) {
        create(file);
        return !QTest::currentTestFailed();
    };

    QVERIFY(createAndCheck("cppnamespace.qml"));
    QCOMPARE(object->property("intProperty").toInt(),
             (int)MyNamespace::MyOtherNSEnum::OtherKey2);

    QVERIFY(createAndCheck("cppstaticnamespace.qml"));
    QCOMPARE(object->property("intProperty").toInt(),
             (int)MyStaticNamespace::MyOtherNSEnum::OtherKey2);

    QVERIFY(createAndCheck("cppnamespace.2.qml"));
    QVERIFY(createAndCheck("cppstaticnamespace.2.qml"));
}

void tst_qqmllanguage::aliasProperties()
{
    // Simple "int" alias
    {
        QQmlComponent component(&engine, testFileUrl("alias.1.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Read through alias
        QCOMPARE(object->property("valueAlias").toInt(), 10);
        object->setProperty("value", QVariant(13));
        QCOMPARE(object->property("valueAlias").toInt(), 13);

        // Write through alias
        object->setProperty("valueAlias", QVariant(19));
        QCOMPARE(object->property("valueAlias").toInt(), 19);
        QCOMPARE(object->property("value").toInt(), 19);
    }

    // Complex object alias
    {
        QQmlComponent component(&engine, testFileUrl("alias.2.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Read through alias
        MyQmlObject *v =
            qvariant_cast<MyQmlObject *>(object->property("aliasObject"));
        QVERIFY(v != nullptr);
        QCOMPARE(v->value(), 10);

        // Write through alias
        MyQmlObject *v2 = new MyQmlObject();
        v2->setParent(object.data());
        object->setProperty("aliasObject", QVariant::fromValue(v2));
        MyQmlObject *v3 =
            qvariant_cast<MyQmlObject *>(object->property("aliasObject"));
        QVERIFY(v3 != nullptr);
        QCOMPARE(v3, v2);
    }

    // Nested aliases
    {
        QQmlComponent component(&engine, testFileUrl("alias.3.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("value").toInt(), 1892);
        QCOMPARE(object->property("value2").toInt(), 1892);

        object->setProperty("value", QVariant(1313));
        QCOMPARE(object->property("value").toInt(), 1313);
        QCOMPARE(object->property("value2").toInt(), 1313);

        object->setProperty("value2", QVariant(8080));
        QCOMPARE(object->property("value").toInt(), 8080);
        QCOMPARE(object->property("value2").toInt(), 8080);
    }

    // Enum aliases
    {
        QQmlComponent component(&engine, testFileUrl("alias.4.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("enumAlias").toInt(), 1);
    }

    // Id aliases
    {
        QQmlComponent component(&engine, testFileUrl("alias.5.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QVariant v = object->property("otherAlias");
        QCOMPARE(v.typeId(), qMetaTypeId<MyQmlObject *>());
        MyQmlObject *o = qvariant_cast<MyQmlObject*>(v);
        QCOMPARE(o->value(), 10);

        delete o; //intentional delete

        v = object->property("otherAlias");
        QCOMPARE(v.typeId(), qMetaTypeId<MyQmlObject *>());
        o = qvariant_cast<MyQmlObject*>(v);
        QVERIFY(!o);
    }

    // Nested aliases - this used to cause a crash
    {
        QQmlComponent component(&engine, testFileUrl("alias.6.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("a").toInt(), 1923);
    }

    // Ptr Alias Cleanup - check that aliases to ptr types return 0
    // if the object aliased to is removed
    {
        QQmlComponent component(&engine, testFileUrl("alias.7.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QObject *object1 = qvariant_cast<QObject *>(object->property("object"));
        QVERIFY(object1 != nullptr);
        QObject *object2 = qvariant_cast<QObject *>(object1->property("object"));
        QVERIFY(object2 != nullptr);

        QObject *alias = qvariant_cast<QObject *>(object->property("aliasedObject"));
        QCOMPARE(alias, object2);

        delete object1; //intentional delete

        QObject *alias2 = object.data(); // "Random" start value
        int status = -1;
        void *a[] = { &alias2, nullptr, &status };
        QMetaObject::metacall(object.data(), QMetaObject::ReadProperty,
                              object->metaObject()->indexOfProperty("aliasedObject"), a);
        QVERIFY(!alias2);
    }

    // Simple composite type
    {
        QQmlComponent component(&engine, testFileUrl("alias.8.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("value").toInt(), 10);
    }

    // Complex composite type
    {
        QQmlComponent component(&engine, testFileUrl("alias.9.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("value").toInt(), 10);
    }

    // Valuetype alias
    // Simple "int" alias
    {
        QQmlComponent component(&engine, testFileUrl("alias.10.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Read through alias
        QCOMPARE(object->property("valueAlias").toRect(), QRect(10, 11, 9, 8));
        object->setProperty("rectProperty", QVariant(QRect(33, 12, 99, 100)));
        QCOMPARE(object->property("valueAlias").toRect(), QRect(33, 12, 99, 100));

        // Write through alias
        object->setProperty("valueAlias", QVariant(QRect(3, 3, 4, 9)));
        QCOMPARE(object->property("valueAlias").toRect(), QRect(3, 3, 4, 9));
        QCOMPARE(object->property("rectProperty").toRect(), QRect(3, 3, 4, 9));
    }

    // Valuetype sub-alias
    {
        QQmlComponent component(&engine, testFileUrl("alias.11.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Read through alias
        QCOMPARE(object->property("aliasProperty").toInt(), 19);
        object->setProperty("rectProperty", QVariant(QRect(33, 8, 102, 111)));
        QCOMPARE(object->property("aliasProperty").toInt(), 33);

        // Write through alias
        object->setProperty("aliasProperty", QVariant(4));
        QCOMPARE(object->property("aliasProperty").toInt(), 4);
        QCOMPARE(object->property("rectProperty").toRect(), QRect(4, 8, 102, 111));
    }

    // Nested aliases with a qml file
    {
        QQmlComponent component(&engine, testFileUrl("alias.12.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QPointer<QObject> subObject = qvariant_cast<QObject*>(object->property("referencingSubObject"));
        QVERIFY(!subObject.isNull());

        QVERIFY(subObject->property("success").toBool());
    }

    // Nested aliases with a qml file with reverse ordering
    {
        // This is known to fail at the moment.
        QQmlComponent component(&engine, testFileUrl("alias.13.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QPointer<QObject> subObject = qvariant_cast<QObject*>(object->property("referencingSubObject"));
        QVERIFY(!subObject.isNull());

        QVERIFY(subObject->property("success").toBool());
    }

    // "Nested" aliases within an object that require iterative resolution
    {
        QQmlComponent component(&engine, testFileUrl("alias.14.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QPointer<QObject> subObject = qvariant_cast<QObject*>(object->property("referencingSubObject"));
        QVERIFY(!subObject.isNull());

        QVERIFY(subObject->property("success").toBool());
    }

    // Property bindings on group properties that are actually aliases (QTBUG-51043)
    {
        QQmlComponent component(&engine, testFileUrl("alias.15.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QPointer<QObject> subItem = qvariant_cast<QObject*>(object->property("symbol"));
        QVERIFY(!subItem.isNull());

        QCOMPARE(subItem->property("y").toInt(), 1);
    }

    // Nested property bindings on group properties that are actually aliases (QTBUG-94983)
    {
        QQmlComponent component(&engine, testFileUrl("alias.15a.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QPointer<QObject> subItem = qvariant_cast<QObject*>(object->property("symbol"));
        QVERIFY(!subItem.isNull());

        QPointer<QObject> subSubItem = qvariant_cast<QObject*>(subItem->property("layer"));

        QCOMPARE(subSubItem->property("enabled").value<bool>(), true);
    }

    // Alias to sub-object with binding (QTBUG-57041)
    {
        // This is shold *not* crash.
        QQmlComponent component(&engine, testFileUrl("alias.16.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
    }

    // Alias to grouped property
    {
        QQmlComponent component(&engine, testFileUrl("alias.17.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
        QVERIFY(object->property("success").toBool());
    }

    // Alias to grouped property updates
    {
        QQmlComponent component(&engine, testFileUrl("alias.17.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
        QObject *aliasUser = object->findChild<QObject*>(QLatin1String("aliasUser"));
        QVERIFY(aliasUser);
        QQmlProperty checkValueProp(object.get(), "checkValue");
        QVERIFY(checkValueProp.isValid());
        checkValueProp.write(777);
        QCOMPARE(object->property("checkValue").toInt(), 777);
        QCOMPARE(aliasUser->property("topMargin").toInt(), 777);
    }

    // Write to alias to grouped property
    {
        QQmlComponent component(&engine, testFileUrl("alias.17.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
        QObject *aliasUser = object->findChild<QObject*>(QLatin1String("aliasUser"));
        QVERIFY(aliasUser);
        QQmlProperty topMarginProp {aliasUser, "topMargin"};
        QVERIFY(topMarginProp.isValid());
        topMarginProp.write(777);
        QObject *myItem = object->findChild<QObject*>(QLatin1String("myItem"));
        QVERIFY(myItem);
        auto anchors = myItem->property("anchors").value<QObject*>();
        QVERIFY(anchors);
        QCOMPARE(anchors->property("topMargin").toInt(), 777);
    }

    // Binding to alias to grouped property gets updated
    {
        QQmlComponent component(&engine, testFileUrl("alias.17.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
        QObject *aliasUser = object->findChild<QObject*>(QLatin1String("aliasUser"));
        QVERIFY(aliasUser);
        QQmlProperty topMarginProp {aliasUser, "topMargin"};
        QVERIFY(topMarginProp.isValid());
        topMarginProp.write(20);
        QObject *myText = object->findChild<QObject*>(QLatin1String("myText"));
        QVERIFY(myText);
        auto text = myText->property("text").toString();
        QCOMPARE(text, "alias:\n20");
    }

    {
        QQmlComponent component(&engine, testFileUrl("alias.18.qml"));
        VERIFY_ERRORS("alias.18.errors.txt");
    }

    // Binding on deep alias
    {
        QQmlComponent component(&engine, testFileUrl("alias.19.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
        QCOMPARE(object->property("height").toInt(), 960);
    }
}

// QTBUG-13374 Test that alias properties and signals can coexist
void tst_qqmllanguage::aliasPropertiesAndSignals()
{
    QQmlComponent component(&engine, testFileUrl("aliasPropertiesAndSignals.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);
    QCOMPARE(o->property("test").toBool(), true);
}

void tst_qqmllanguage::qtbug_89822()
{
    QQmlComponent component(&engine, testFileUrl("qtbug_89822.qml"));
    VERIFY_ERRORS("qtbug_89822.errors.txt");
}

// Test that the root element in a composite type can be a Component
void tst_qqmllanguage::componentCompositeType()
{
    QQmlComponent component(&engine, testFileUrl("componentCompositeType.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

class TestType : public QObject {
    Q_OBJECT
public:
    TestType(QObject *p=nullptr) : QObject(p) {}
};

class TestType2 : public QObject {
    Q_OBJECT
public:
    TestType2(QObject *p=nullptr) : QObject(p) {}
};

void tst_qqmllanguage::i18n_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("stringProperty");
    QTest::newRow("i18nStrings") << "i18nStrings.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245 (5 accented 'a' letters)");
    QTest::newRow("i18nDeclaredPropertyNames") << "i18nDeclaredPropertyNames.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245: 10");
    QTest::newRow("i18nDeclaredPropertyUse") << "i18nDeclaredPropertyUse.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245: 15");
    QTest::newRow("i18nScript") << "i18nScript.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245: 20");
    QTest::newRow("i18nType") << "i18nType.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245: 30");
    QTest::newRow("i18nNameSpace") << "i18nNameSpace.qml" << QString::fromUtf8("Test \303\241\303\242\303\243\303\244\303\245: 40");
}

void tst_qqmllanguage::i18n()
{
    QFETCH(QString, file);
    QFETCH(QString, stringProperty);
    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(0);
    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->stringProperty(), stringProperty);
}

// Check that the Component::onCompleted attached property works
void tst_qqmllanguage::onCompleted()
{
    QQmlComponent component(&engine, testFileUrl("onCompleted.qml"));
    VERIFY_ERRORS(0);
    QTest::ignoreMessage(QtDebugMsg, "Completed 6 10");
    QTest::ignoreMessage(QtDebugMsg, "Completed 6 10");
    QTest::ignoreMessage(QtDebugMsg, "Completed 10 11");
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

// Check that the Component::onDestruction attached property works
void tst_qqmllanguage::onDestruction()
{
    QQmlComponent component(&engine, testFileUrl("onDestruction.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QTest::ignoreMessage(QtDebugMsg, "Destruction 6 10");
    QTest::ignoreMessage(QtDebugMsg, "Destruction 6 10");
    QTest::ignoreMessage(QtDebugMsg, "Destruction 10 11");
}

// Check that assignments to QQmlScriptString properties work
void tst_qqmllanguage::scriptString()
{
    {
        QQmlComponent component(&engine, testFileUrl("scriptString.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        QVERIFY(!object->scriptProperty().isEmpty());
        QCOMPARE(object->scriptProperty().stringLiteral(), QString());
        bool ok;
        QCOMPARE(object->scriptProperty().numberLiteral(&ok), qreal(0.));
        QCOMPARE(ok, false);

        const QQmlScriptStringPrivate *scriptPrivate = QQmlScriptStringPrivate::get(object->scriptProperty());
        QVERIFY(scriptPrivate != nullptr);
        QCOMPARE(scriptPrivate->script, QString("foo + bar"));
        QCOMPARE(scriptPrivate->scope, qobject_cast<QObject*>(object.data()));
        QCOMPARE(scriptPrivate->context, qmlContext(object.data()));

        QVERIFY(object->grouped() != nullptr);
        const QQmlScriptStringPrivate *groupedPrivate = QQmlScriptStringPrivate::get(object->grouped()->script());
        QCOMPARE(groupedPrivate->script, QString("console.log(1921)"));
        QCOMPARE(groupedPrivate->scope, qobject_cast<QObject*>(object.data()));
        QCOMPARE(groupedPrivate->context, qmlContext(object.data()));
    }

    {
        QQmlComponent component(&engine, testFileUrl("scriptString2.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        QCOMPARE(object->scriptProperty().stringLiteral(), QString("hello\\n\\\"world\\\""));
    }

    {
        QQmlComponent component(&engine, testFileUrl("scriptString3.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        bool ok;
        QCOMPARE(object->scriptProperty().numberLiteral(&ok), qreal(12.345));
        QCOMPARE(ok, true);

    }

    {
        QQmlComponent component(&engine, testFileUrl("scriptString4.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        bool ok;
        QCOMPARE(object->scriptProperty().booleanLiteral(&ok), true);
        QCOMPARE(ok, true);
    }

    {
        QQmlComponent component(&engine, testFileUrl("scriptString5.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        QCOMPARE(object->scriptProperty().isNullLiteral(), true);
    }

    {
        QQmlComponent component(&engine, testFileUrl("scriptString6.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        QCOMPARE(object->scriptProperty().isUndefinedLiteral(), true);
    }
    {
        QQmlComponent component(&engine, testFileUrl("scriptString7.qml"));
        VERIFY_ERRORS(0);

        QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
        QVERIFY(object != nullptr);
        QQmlScriptString ss = object->scriptProperty();

        {
            QQmlExpression expr(ss, /*context*/nullptr, object.data());
            QCOMPARE(expr.evaluate().toInt(), int(100));
        }

        {
            SimpleObjectWithCustomParser testScope;
            QVERIFY(testScope.metaObject()->indexOfProperty("intProperty") != object->metaObject()->indexOfProperty("intProperty"));

            testScope.setIntProperty(42);
            QQmlExpression expr(ss, /*context*/nullptr, &testScope);
            QCOMPARE(expr.evaluate().toInt(), int(42));
        }
    }
}

// Check that assignments to QQmlScriptString properties works also from within Javascript
void tst_qqmllanguage::scriptStringJs()
{
    QQmlComponent component(&engine, testFileUrl("scriptStringJs.qml"));
    VERIFY_ERRORS(0);

    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
    QVERIFY(object != nullptr);
    QQmlContext *context = QQmlEngine::contextForObject(object.data());
    QVERIFY(context != nullptr);
    bool ok;

    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("\" hello \\\" world \""));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QCOMPARE(object->scriptProperty().stringLiteral(), QString(" hello \\\" world "));
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && !ok);

    QJSValue inst = engine.newQObject(object.data());
    QJSValue func = engine.evaluate("(function(value) { this.scriptProperty = value })");

    func.callWithInstance(inst, QJSValueList() << "test a \"string ");
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("\"test a \\\"string \""));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QCOMPARE(object->scriptProperty().stringLiteral(), QString("test a \\\"string "));
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && !ok);

    func.callWithInstance(inst, QJSValueList() << QJSValue::UndefinedValue);
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("undefined"));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QVERIFY(object->scriptProperty().stringLiteral().isEmpty());
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && !ok);

    func.callWithInstance(inst, QJSValueList() << true);
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("true"));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QVERIFY(object->scriptProperty().stringLiteral().isEmpty());
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(object->scriptProperty().booleanLiteral(&ok) && ok);

    func.callWithInstance(inst, QJSValueList() << false);
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("false"));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QVERIFY(object->scriptProperty().stringLiteral().isEmpty());
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && ok);

    func.callWithInstance(inst, QJSValueList() << QJSValue::NullValue);
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("null"));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(object->scriptProperty().isNullLiteral());
    QVERIFY(object->scriptProperty().stringLiteral().isEmpty());
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 0.0 && !ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && !ok);

    func.callWithInstance(inst, QJSValueList() << 12.34);
    QCOMPARE(QQmlScriptStringPrivate::get(object->scriptProperty())->script, QString("12.34"));
    QVERIFY(!object->scriptProperty().isEmpty());
    QVERIFY(!object->scriptProperty().isUndefinedLiteral());
    QVERIFY(!object->scriptProperty().isNullLiteral());
    QVERIFY(object->scriptProperty().stringLiteral().isEmpty());
    QVERIFY(object->scriptProperty().numberLiteral(&ok) == 12.34 && ok);
    QVERIFY(!object->scriptProperty().booleanLiteral(&ok) && !ok);
}

struct FreeUnitData
{
    static void cleanup(const QV4::CompiledData::Unit *readOnlyQmlUnit)
    {
        if (readOnlyQmlUnit && !(readOnlyQmlUnit->flags & QV4::CompiledData::Unit::StaticData))
            free(const_cast<QV4::CompiledData::Unit *>(readOnlyQmlUnit));
    }
};

void tst_qqmllanguage::scriptStringWithoutSourceCode()
{
    QUrl url = testFileUrl("scriptString7.qml");
    QScopedPointer<const QV4::CompiledData::Unit, FreeUnitData> readOnlyQmlUnit;
    {
        QQmlEnginePrivate *eng = QQmlEnginePrivate::get(&engine);
        QQmlRefPointer<QQmlTypeData> td = eng->typeLoader.getType(url);
        Q_ASSERT(td);
        QVERIFY(!td->backupSourceCode().isValid());

        QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit = td->compilationUnit();
        readOnlyQmlUnit.reset(compilationUnit->unitData());
        Q_ASSERT(readOnlyQmlUnit);
        QV4::CompiledData::Unit *qmlUnit = reinterpret_cast<QV4::CompiledData::Unit *>(malloc(readOnlyQmlUnit->unitSize));
        memcpy(qmlUnit, readOnlyQmlUnit.data(), readOnlyQmlUnit->unitSize);

        qmlUnit->flags &= ~QV4::CompiledData::Unit::StaticData;
        compilationUnit->setUnitData(qmlUnit);

        const QV4::CompiledData::Object *rootObject = compilationUnit->objectAt(/*root object*/0);
        QCOMPARE(compilationUnit->stringAt(rootObject->inheritedTypeNameIndex), QString("MyTypeObject"));
        quint32 i;
        for (i = 0; i < rootObject->nBindings; ++i) {
            const QV4::CompiledData::Binding *binding = rootObject->bindingTable() + i;
            if (compilationUnit->stringAt(binding->propertyNameIndex) != QString("scriptProperty"))
                continue;
            QCOMPARE(compilationUnit->bindingValueAsScriptString(binding), QString("intProperty"));
            const_cast<QV4::CompiledData::Binding*>(binding)->stringIndex = 0; // empty string index
            QVERIFY(compilationUnit->bindingValueAsScriptString(binding).isEmpty());
            break;
        }
        QVERIFY(i < rootObject->nBindings);
    }
    QQmlComponent component(&engine, url);
    VERIFY_ERRORS(0);

    QScopedPointer<MyTypeObject> object(qobject_cast<MyTypeObject*>(component.create()));
    QVERIFY(object != nullptr);
    QQmlScriptString ss = object->scriptProperty();
    QVERIFY(!ss.isEmpty());
    QCOMPARE(ss.stringLiteral(), QString());
    bool ok;
    QCOMPARE(ss.numberLiteral(&ok), qreal(0.));
    QCOMPARE(ok, false);

    const QQmlScriptStringPrivate *scriptPrivate = QQmlScriptStringPrivate::get(ss);
    QVERIFY(scriptPrivate != nullptr);
    QVERIFY(scriptPrivate->script.isEmpty());
    QCOMPARE(scriptPrivate->scope, qobject_cast<QObject*>(object.data()));
    QCOMPARE(scriptPrivate->context, qmlContext(object.data()));

    {
        QQmlExpression expr(ss, /*context*/nullptr, object.data());
        QCOMPARE(expr.evaluate().toInt(), int(100));
    }
}

// Test the QQmlScriptString comparison operators. The script strings are considered
// equal if there evaluation would produce the same result.
void tst_qqmllanguage::scriptStringComparison()
{
    QQmlComponent component1(&engine, testFileUrl("scriptString.qml"));
    QVERIFY(!component1.isError() && component1.errors().isEmpty());
    QScopedPointer<MyTypeObject> object1(qobject_cast<MyTypeObject*>(component1.create()));
    QVERIFY(object1 != nullptr);

    QQmlComponent component2(&engine, testFileUrl("scriptString2.qml"));
    QVERIFY(!component2.isError() && component2.errors().isEmpty());
    QScopedPointer<MyTypeObject> object2(qobject_cast<MyTypeObject*>(component2.create()));
    QVERIFY(object2 != nullptr);

    QQmlComponent component3(&engine, testFileUrl("scriptString3.qml"));
    QVERIFY(!component3.isError() && component3.errors().isEmpty());
    QScopedPointer<MyTypeObject> object3(qobject_cast<MyTypeObject*>(component3.create()));
    QVERIFY(object3 != nullptr);

    //QJSValue inst1 = engine.newQObject(object1);
    QJSValue inst2 = engine.newQObject(object2.data());
    QJSValue inst3 = engine.newQObject(object3.data());
    QJSValue func = engine.evaluate("(function(value) { this.scriptProperty = value })");

    const QString s = "hello\\n\\\"world\\\"";
    const qreal n = 12.345;
    bool ok;

    QCOMPARE(object2->scriptProperty().stringLiteral(), s);
    QVERIFY(object3->scriptProperty().numberLiteral(&ok) == n && ok);
    QCOMPARE(object1->scriptProperty(), object1->scriptProperty());
    QCOMPARE(object2->scriptProperty(), object2->scriptProperty());
    QCOMPARE(object3->scriptProperty(), object3->scriptProperty());
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    QVERIFY(object1->scriptProperty() != object2->scriptProperty());
    QVERIFY(object1->scriptProperty() != object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << n);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << s);
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << s);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << QJSValue::UndefinedValue);
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << QJSValue::UndefinedValue);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << QJSValue::NullValue);
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << QJSValue::NullValue);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << false);
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << false);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    func.callWithInstance(inst2, QJSValueList() << true);
    QVERIFY(object2->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << true);
    QCOMPARE(object2->scriptProperty(), object3->scriptProperty());

    QVERIFY(object1->scriptProperty() != object2->scriptProperty());
    object2->setScriptProperty(object1->scriptProperty());
    QCOMPARE(object1->scriptProperty(), object2->scriptProperty());

    QVERIFY(object1->scriptProperty() != object3->scriptProperty());
    func.callWithInstance(inst3, QJSValueList() << engine.toScriptValue(object1->scriptProperty()));
    QCOMPARE(object1->scriptProperty(), object3->scriptProperty());

    // While this are two instances of the same object they are still considered different
    // because the (none literal) script string may access variables which have different
    // values in both instances and hence evaluated to different results.
    QScopedPointer<MyTypeObject> object1_2(qobject_cast<MyTypeObject*>(component1.create()));
    QVERIFY(object1_2 != nullptr);
    QVERIFY(object1->scriptProperty() != object1_2->scriptProperty());
}

// Check that default property assignments are correctly spliced into explicit
// property assignments
void tst_qqmllanguage::defaultPropertyListOrder()
{
    QQmlComponent component(&engine, testFileUrl("defaultPropertyListOrder.qml"));
    VERIFY_ERRORS(0);

    QScopedPointer<MyContainer> container(qobject_cast<MyContainer *>(component.create()));
    QVERIFY(container  != nullptr);

    QCOMPARE(container->getChildren()->size(), 6);
    QCOMPARE(container->getChildren()->at(0)->property("index"), QVariant(0));
    QCOMPARE(container->getChildren()->at(1)->property("index"), QVariant(1));
    QCOMPARE(container->getChildren()->at(2)->property("index"), QVariant(2));
    QCOMPARE(container->getChildren()->at(3)->property("index"), QVariant(3));
    QCOMPARE(container->getChildren()->at(4)->property("index"), QVariant(4));
    QCOMPARE(container->getChildren()->at(5)->property("index"), QVariant(5));
}

void tst_qqmllanguage::defaultPropertyWithInitializer_data()
{
    QTest::addColumn<QUrl>("file");
    QTest::addColumn<QString>("objectName");

    QTest::newRow("base") << testFileUrl("DefaultPropertyWithInitializer.qml") << u"default"_s;
    QTest::newRow("user") << testFileUrl("DefaultPropertyWithInitializerUser.qml") << u"changed"_s;
    QTest::newRow("list base") << testFileUrl("DefaultPropertyWithListInitializer.qml") << u"1"_s;
    QTest::newRow("list user") << testFileUrl("DefaultPropertyWithListInitializerUser.qml") << u"2"_s;
}

void tst_qqmllanguage::defaultPropertyWithInitializer()
{
    QFETCH(QUrl, file);
    QFETCH(QString, objectName);

    QQmlComponent component(&engine, file);
    VERIFY_ERRORS(0);

    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    auto entry = root->property("entry").value<QObject *>();
    QVERIFY(entry);
    QCOMPARE(entry->objectName(), objectName);
}

void tst_qqmllanguage::declaredPropertyValues()
{
    QQmlComponent component(&engine, testFileUrl("declaredPropertyValues.qml"));
    VERIFY_ERRORS(0);
}

void tst_qqmllanguage::dontDoubleCallClassBegin()
{
    QQmlComponent component(&engine, testFileUrl("dontDoubleCallClassBegin.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);

    MyParserStatus *o2 = qobject_cast<MyParserStatus *>(qvariant_cast<QObject *>(o->property("object")));
    QVERIFY(o2);
    QCOMPARE(o2->classBeginCount(), 1);
    QCOMPARE(o2->componentCompleteCount(), 1);
}

void tst_qqmllanguage::reservedWords_data()
{
    QTest::addColumn<QByteArray>("word");

    QTest::newRow("abstract") << QByteArray("abstract");
    QTest::newRow("as") << QByteArray("as");
    QTest::newRow("boolean") << QByteArray("boolean");
    QTest::newRow("break") << QByteArray("break");
    QTest::newRow("byte") << QByteArray("byte");
    QTest::newRow("case") << QByteArray("case");
    QTest::newRow("catch") << QByteArray("catch");
    QTest::newRow("char") << QByteArray("char");
    QTest::newRow("class") << QByteArray("class");
    QTest::newRow("continue") << QByteArray("continue");
    QTest::newRow("const") << QByteArray("const");
    QTest::newRow("debugger") << QByteArray("debugger");
    QTest::newRow("default") << QByteArray("default");
    QTest::newRow("delete") << QByteArray("delete");
    QTest::newRow("do") << QByteArray("do");
    QTest::newRow("double") << QByteArray("double");
    QTest::newRow("else") << QByteArray("else");
    QTest::newRow("enum") << QByteArray("enum");
    QTest::newRow("export") << QByteArray("export");
    QTest::newRow("extends") << QByteArray("extends");
    QTest::newRow("false") << QByteArray("false");
    QTest::newRow("final") << QByteArray("final");
    QTest::newRow("finally") << QByteArray("finally");
    QTest::newRow("float") << QByteArray("float");
    QTest::newRow("for") << QByteArray("for");
    QTest::newRow("function") << QByteArray("function");
    QTest::newRow("goto") << QByteArray("goto");
    QTest::newRow("if") << QByteArray("if");
    QTest::newRow("implements") << QByteArray("implements");
    QTest::newRow("import") << QByteArray("import");
    QTest::newRow("pragma") << QByteArray("pragma");
    QTest::newRow("in") << QByteArray("in");
    QTest::newRow("instanceof") << QByteArray("instanceof");
    QTest::newRow("int") << QByteArray("int");
    QTest::newRow("interface") << QByteArray("interface");
    QTest::newRow("long") << QByteArray("long");
    QTest::newRow("native") << QByteArray("native");
    QTest::newRow("new") << QByteArray("new");
    QTest::newRow("null") << QByteArray("null");
    QTest::newRow("package") << QByteArray("package");
    QTest::newRow("private") << QByteArray("private");
    QTest::newRow("protected") << QByteArray("protected");
    QTest::newRow("public") << QByteArray("public");
    QTest::newRow("return") << QByteArray("return");
    QTest::newRow("short") << QByteArray("short");
    QTest::newRow("static") << QByteArray("static");
    QTest::newRow("super") << QByteArray("super");
    QTest::newRow("switch") << QByteArray("switch");
    QTest::newRow("synchronized") << QByteArray("synchronized");
    QTest::newRow("this") << QByteArray("this");
    QTest::newRow("throw") << QByteArray("throw");
    QTest::newRow("throws") << QByteArray("throws");
    QTest::newRow("transient") << QByteArray("transient");
    QTest::newRow("true") << QByteArray("true");
    QTest::newRow("try") << QByteArray("try");
    QTest::newRow("typeof") << QByteArray("typeof");
    QTest::newRow("var") << QByteArray("var");
    QTest::newRow("void") << QByteArray("void");
    QTest::newRow("volatile") << QByteArray("volatile");
    QTest::newRow("while") << QByteArray("while");
    QTest::newRow("with") << QByteArray("with");
}

void tst_qqmllanguage::reservedWords()
{
    QFETCH(QByteArray, word);
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nQtObject { property string " + word + " }", QUrl());
    QCOMPARE(component.errorString(), QLatin1String(":2 Expected token `identifier'\n"));
}

// Check that first child of qml is of given type. Empty type insists on error.
void tst_qqmllanguage::testType(const QString& qml, const QString& type, const QString& expectederror, bool partialMatch)
{
    if (engine.importPathList() == defaultImportPathList)
        engine.addImportPath(testFile("lib"));

    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), testFileUrl("empty.qml")); // just a file for relative local imports

    QTRY_VERIFY(!component.isLoading());

    if (type.isEmpty()) {
        QVERIFY(component.isError());
        QString actualerror;
        const auto errors = component.errors();
        for (const QQmlError &e : errors) {
            if (!actualerror.isEmpty())
                actualerror.append("; ");
            actualerror.append(e.description());
        }
        QCOMPARE(actualerror.left(partialMatch ? expectederror.size(): -1),expectederror);
    } else {
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        const QMetaObject *meta = object->metaObject();
        for (; meta; meta = meta->superClass()) {
            const QString className(meta->className());
            if (!className.contains("_QMLTYPE_") && !className.contains("_QML_")) {
                QCOMPARE(className, type);
                break;
            }
        }
        QVERIFY(meta != nullptr);
    }

    engine.setImportPathList(defaultImportPathList);
}

// QTBUG-17276
void tst_qqmllanguage::inlineAssignmentsOverrideBindings()
{
    QQmlComponent component(&engine, testFileUrl("inlineAssignmentsOverrideBindings.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);
    QCOMPARE(o->property("test").toInt(), 11);
}

// QTBUG-19354
void tst_qqmllanguage::nestedComponentRoots()
{
    QQmlComponent component(&engine, testFileUrl("nestedComponentRoots.qml"));
}

// Import tests (QT-558)
void tst_qqmllanguage::importsBuiltin_data()
{
    // QT-610

    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");

    // import built-ins
    QTest::newRow("missing import")
        << "Test {}"
        << ""
        << "Test is not a type";
    QTest::newRow("not in version 0.0")
        << "import org.qtproject.Test 0.0\n"
           "Test {}"
        << ""
        << "Test is not a type";
    QTest::newRow("version not installed")
        << "import org.qtproject.Test 99.0\n"
           "Test {}"
        << ""
        << "module \"org.qtproject.Test\" version 99.0 is not installed";
    QTest::newRow("in version 0.0")
        << "import org.qtproject.Test 0.0\n"
           "TestTP {}"
        << "TestType"
        << "";
    QTest::newRow("qualified in version 0.0")
        << "import org.qtproject.Test 0.0 as T\n"
           "T.TestTP {}"
        << "TestType"
        << "";
    QTest::newRow("in version 1.0")
        << "import org.qtproject.Test 1.0\n"
           "Test {}"
        << "TestType"
        << "";
    QTest::newRow("qualified wrong")
        << "import org.qtproject.Test 1.0 as T\n" // QT-610
           "Test {}"
        << ""
        << "Test is not a type";
    QTest::newRow("qualified right")
        << "import org.qtproject.Test 1.0 as T\n"
           "T.Test {}"
        << "TestType"
        << "";
    QTest::newRow("qualified right but not in version 0.0")
        << "import org.qtproject.Test 0.0 as T\n"
           "T.Test {}"
        << ""
        << "T.Test is not a type";
    QTest::newRow("in version 1.1")
        << "import org.qtproject.Test 1.1\n"
           "Test {}"
        << "TestType"
        << "";
    QTest::newRow("in version 1.3")
        << "import org.qtproject.Test 1.3\n"
           "Test {}"
        << "TestType"
        << "";
    QTest::newRow("in version 1.5")
        << "import org.qtproject.Test 1.5\n"
           "Test {}"
        << "TestType"
        << "";
    QTest::newRow("changed in version 1.8")
        << "import org.qtproject.Test 1.8\n"
           "Test {}"
        << "TestType2"
        << "";
    QTest::newRow("in version 1.12")
        << "import org.qtproject.Test 1.12\n"
           "Test {}"
        << "TestType2"
        << "";
    QTest::newRow("old in version 1.9")
        << "import org.qtproject.Test 1.9\n"
           "OldTest {}"
        << "TestType"
        << "";
    QTest::newRow("old in version 1.11")
        << "import org.qtproject.Test 1.11\n"
           "OldTest {}"
        << "TestType"
        << "";
    QTest::newRow("multiversion 1")
        << "import org.qtproject.Test 1.11\n"
           "import org.qtproject.Test 1.12\n"
           "Test {}"
        << (!qmlCheckTypes()?"TestType2":"")
        << (!qmlCheckTypes()?"":"Test is ambiguous. Found in org/qtproject/Test/ in version 1.12 and 1.11");
    QTest::newRow("multiversion 2")
        << "import org.qtproject.Test 1.11\n"
           "import org.qtproject.Test 1.12\n"
           "OldTest {}"
        << (!qmlCheckTypes()?"TestType":"")
        << (!qmlCheckTypes()?"":"OldTest is ambiguous. Found in org/qtproject/Test/ in version 1.12 and 1.11");
    QTest::newRow("qualified multiversion 3")
        << "import org.qtproject.Test 1.0 as T0\n"
           "import org.qtproject.Test 1.8 as T8\n"
           "T0.Test {}"
        << "TestType"
        << "";
    QTest::newRow("qualified multiversion 4")
        << "import org.qtproject.Test 1.0 as T0\n"
           "import org.qtproject.Test 1.8 as T8\n"
           "T8.Test {}"
        << "TestType2"
        << "";
}

void tst_qqmllanguage::importsBuiltin()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);
    testType(qml,type,error);
}

void tst_qqmllanguage::importsLocal_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");

    // import locals
    QTest::newRow("local import")
        << "import \"subdir\"\n" // QT-613
           "Test {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("local import second")
        << "import QtQuick 2.0\nimport \"subdir\"\n"
           "Test {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("local import subsubdir")
        << "import QtQuick 2.0\nimport \"subdir/subsubdir\"\n"
           "SubTest {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("local import QTBUG-7721 A")
        << "subdir.Test {}" // no longer allowed (QTBUG-7721)
        << ""
        << "subdir.Test - subdir is neither a type nor a namespace";
    QTest::newRow("local import QTBUG-7721 B")
        << "import \"subdir\" as X\n"
           "X.subsubdir.SubTest {}" // no longer allowed (QTBUG-7721)
        << ""
        << "X.subsubdir.SubTest - subsubdir is not a type";
    QTest::newRow("local import as")
        << "import \"subdir\" as T\n"
           "T.Test {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("wrong local import as")
        << "import \"subdir\" as T\n"
           "Test {}"
        << ""
        << "Test is not a type";
    QTest::newRow("library precedence over local import")
        << "import \"subdir\"\n"
           "import org.qtproject.Test 1.0\n"
           "Test {}"
        << (!qmlCheckTypes()?"TestType":"")
        << (!qmlCheckTypes()?"":"Test is ambiguous. Found in org/qtproject/Test/ and in subdir/");

    if (dataDirectoryUrl().scheme() != QLatin1String("qrc")) {
        // file URL doesn't work with qrc scheme
        QTest::newRow("file URL survives percent-encoding")
            << "import \"" + QUrl::fromLocalFile(QDir::currentPath() + "/{subdir}").toString() + "\"\n"
               "Test {}"
            << "QQuickRectangle"
            << "";
    }
}

void tst_qqmllanguage::importsLocal()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);
    testType(qml,type,error);
}

void tst_qqmllanguage::basicRemote_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");

    QString serverdir = "/qtest/qml/qqmllanguage/";

    QTest::newRow("no need for qmldir") << QUrl(serverdir+"Test.qml") << "" << "";
    QTest::newRow("absent qmldir") << QUrl(serverdir+"/noqmldir/Test.qml") << "" << "";
    QTest::newRow("need qmldir") << QUrl(serverdir+"TestNamed.qml") << "" << "";
}

void tst_qqmllanguage::basicRemote()
{
    QFETCH(QUrl, url);
    QFETCH(QString, type);
    QFETCH(QString, error);

    ThreadedTestHTTPServer server(dataDirectory());

    url = server.baseUrl().resolved(url);

    QQmlComponent component(&engine, url);

    QTRY_VERIFY(!component.isLoading());

    if (error.isEmpty()) {
        if (component.isError())
            qDebug() << component.errors();
        QVERIFY(!component.isError());
    } else {
        QVERIFY(component.isError());
    }
}

void tst_qqmllanguage::importsRemote_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");

    QString serverdir = "{{ServerBaseUrl}}/qtest/qml/qqmllanguage";

    QTest::newRow("remote import") << "import \""+serverdir+"\"\nTest {}" << "QQuickRectangle"
        << "";
    QTest::newRow("remote import with subdir") << "import \""+serverdir+"\"\nTestSubDir {}" << "QQuickText"
        << "";
    QTest::newRow("remote import with local") << "import \""+serverdir+"\"\nTestLocal {}" << "QQuickImage"
        << "";
    QTest::newRow("remote import with qualifier") << "import \""+serverdir+"\" as NS\nNS.NamedLocal {}" << "QQuickImage"
        << "";
    QTest::newRow("wrong remote import with undeclared local") << "import \""+serverdir+"\"\nWrongTestLocal {}" << ""
        << "WrongTestLocal is not a type";
    QTest::newRow("wrong remote import of internal local") << "import \""+serverdir+"\"\nLocalInternal {}" << ""
        << "LocalInternal is not a type";
    QTest::newRow("wrong remote import of undeclared local") << "import \""+serverdir+"\"\nUndeclaredLocal {}" << ""
        << "UndeclaredLocal is not a type";
}

void tst_qqmllanguage::importsRemote()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);

    ThreadedTestHTTPServer server(dataDirectory());

    qml.replace(QStringLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString());

    testType(qml,type,error);
}

void tst_qqmllanguage::importsInstalled_data()
{
    // QT-610

    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");

    // import installed
    QTest::newRow("installed import 0")
        << "import org.qtproject.installedtest0 0.0\n"
           "InstalledTestTP {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("installed import 0 as TP")
        << "import org.qtproject.installedtest0 0.0 as TP\n"
           "TP.InstalledTestTP {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("installed import 1")
        << "import org.qtproject.installedtest 1.0\n"
           "InstalledTest {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("installed import 2")
        << "import org.qtproject.installedtest 1.3\n"
           "InstalledTest {}"
        << "QQuickRectangle"
        << "";
    QTest::newRow("installed import 3")
        << "import org.qtproject.installedtest 1.4\n"
           "InstalledTest {}"
        << "QQuickText"
        << "";
    QTest::newRow("installed import minor version not available") // QTBUG-11936
        << "import org.qtproject.installedtest 0.1\n"
           "InstalledTest {}"
        << ""
        << "module \"org.qtproject.installedtest\" version 0.1 is not installed";
    QTest::newRow("installed import minor version not available") // QTBUG-9627
        << "import org.qtproject.installedtest 1.10\n"
           "InstalledTest {}"
        << ""
        << "module \"org.qtproject.installedtest\" version 1.10 is not installed";
    QTest::newRow("installed import major version not available") // QTBUG-9627
        << "import org.qtproject.installedtest 9.0\n"
           "InstalledTest {}"
        << ""
        << "module \"org.qtproject.installedtest\" version 9.0 is not installed";
    QTest::newRow("installed import visibility") // QT-614
        << "import org.qtproject.installedtest 1.4\n"
           "PrivateType {}"
        << ""
        << "PrivateType is not a type";
    QTest::newRow("installed import version QML clash")
        << "import org.qtproject.installedtest1 1.0\n"
           "Test {}"
        << ""
        << "\"Test\" version 1.0 is defined more than once in module \"org.qtproject.installedtest1\"";
    QTest::newRow("installed import version JS clash")
        << "import org.qtproject.installedtest2 1.0\n"
           "Test {}"
        << ""
        << "\"Test\" version 1.0 is defined more than once in module \"org.qtproject.installedtest2\"";
}

void tst_qqmllanguage::importsInstalled()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);
    testType(qml,type,error);
}

void tst_qqmllanguage::importsInstalledRemote_data()
{
    // Repeat the tests for local installed data
    importsInstalled_data();
}

void tst_qqmllanguage::importsInstalledRemote()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);

    ThreadedTestHTTPServer server(dataDirectory());

    QString serverdir = server.urlString("/lib/");
    engine.setImportPathList(QStringList(defaultImportPathList) << serverdir);

    testType(qml,type,error);

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::importsPath_data()
{
    QTest::addColumn<QStringList>("importPath");
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("value");

    QTest::newRow("local takes priority normal")
        << (QStringList() << testFile("lib") << "{{ServerBaseUrl}}/lib2/")
        << "import testModule 1.0\n"
           "Test {}"
        << "foo";

    QTest::newRow("local takes priority reversed")
        << (QStringList() << "{{ServerBaseUrl}}/lib/" << testFile("lib2"))
        << "import testModule 1.0\n"
           "Test {}"
        << "bar";

    QTest::newRow("earlier takes priority 1")
        << (QStringList() << "{{ServerBaseUrl}}/lib/" << "{{ServerBaseUrl}}/lib2/")
        << "import testModule 1.0\n"
           "Test {}"
        << "foo";

    QTest::newRow("earlier takes priority 2")
        << (QStringList() << "{{ServerBaseUrl}}/lib2/" << "{{ServerBaseUrl}}/lib/")
        << "import testModule 1.0\n"
           "Test {}"
        << "bar";

    QTest::newRow("major version takes priority over unversioned")
        << (QStringList() << "{{ServerBaseUrl}}/lib/" << "{{ServerBaseUrl}}/lib3/")
        << "import testModule 1.0\n"
           "Test {}"
        << "baz";

    QTest::newRow("major version takes priority over minor")
        << (QStringList() << "{{ServerBaseUrl}}/lib4/" << "{{ServerBaseUrl}}/lib3/")
        << "import testModule 1.0\n"
           "Test {}"
        << "baz";

    QTest::newRow("minor version takes priority over unversioned")
        << (QStringList() << "{{ServerBaseUrl}}/lib/" << "{{ServerBaseUrl}}/lib4/")
        << "import testModule 1.0\n"
           "Test {}"
        << "qux";
}

void tst_qqmllanguage::importsPath()
{
    QFETCH(QStringList, importPath);
    QFETCH(QString, qml);
    QFETCH(QString, value);

    ThreadedTestHTTPServer server(dataDirectory());

    for (int i = 0; i < importPath.size(); ++i)
        importPath[i].replace(QStringLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString());

    engine.setImportPathList(QStringList(defaultImportPathList) << importPath);

    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), testFileUrl("empty.qml"));

    QTRY_VERIFY(component.isReady());
    VERIFY_ERRORS(0);

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("test").toString(), value);

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::importsOrder_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("error");
    QTest::addColumn<bool>("partialMatch");

    QTest::newRow("double import") <<
           "import org.qtproject.installedtest 1.4\n"
           "import org.qtproject.installedtest 1.4\n"
           "InstalledTest {}"
           << (!qmlCheckTypes()?"QQuickText":"")
           << (!qmlCheckTypes()?"":"InstalledTest is ambiguous. Found in lib/org/qtproject/installedtest/ in version 1.4 and 1.4")
           << false;
    QTest::newRow("installed import overrides 1") <<
           "import org.qtproject.installedtest 1.0\n"
           "import org.qtproject.installedtest 1.4\n"
           "InstalledTest {}"
           << (!qmlCheckTypes()?"QQuickText":"")
           << (!qmlCheckTypes()?"":"InstalledTest is ambiguous. Found in lib/org/qtproject/installedtest/ in version 1.4 and 1.0")
           << false;
    QTest::newRow("installed import overrides 2") <<
           "import org.qtproject.installedtest 1.4\n"
           "import org.qtproject.installedtest 1.0\n"
           "InstalledTest {}"
           << (!qmlCheckTypes()?"QQuickRectangle":"")
           << (!qmlCheckTypes()?"":"InstalledTest is ambiguous. Found in lib/org/qtproject/installedtest/ in version 1.0 and 1.4")
           << false;
    QTest::newRow("installed import re-overrides 1") <<
           "import org.qtproject.installedtest 1.4\n"
           "import org.qtproject.installedtest 1.0\n"
           "import org.qtproject.installedtest 1.4\n"
           "InstalledTest {}"
           << (!qmlCheckTypes()?"QQuickText":"")
           << (!qmlCheckTypes()?"":"InstalledTest is ambiguous. Found in lib/org/qtproject/installedtest/ in version 1.4 and 1.0")
           << false;
    QTest::newRow("installed import re-overrides 2") <<
           "import org.qtproject.installedtest 1.4\n"
           "import org.qtproject.installedtest 1.0\n"
           "import org.qtproject.installedtest 1.4\n"
           "import org.qtproject.installedtest 1.0\n"
           "InstalledTest {}"
           << (!qmlCheckTypes()?"QQuickRectangle":"")
           << (!qmlCheckTypes()?"":"InstalledTest is ambiguous. Found in lib/org/qtproject/installedtest/ in version 1.0 and 1.4")
           << false;
    QTest::newRow("installed import versus builtin 1") <<
           "import org.qtproject.installedtest 1.5\n"
           "import QtQuick 2.0\n"
           "Rectangle {}"
           << (!qmlCheckTypes()?"QQuickRectangle":"")
           << (!qmlCheckTypes()?"":"Rectangle is ambiguous. Found in file://")
           << true;
    QTest::newRow("installed import versus builtin 2") <<
           "import QtQuick 2.0\n"
           "import org.qtproject.installedtest 1.5\n"
           "Rectangle {}"
           << (!qmlCheckTypes()?"QQuickText":"")
           << (!qmlCheckTypes()?"":"Rectangle is ambiguous. Found in lib/org/qtproject/installedtest/ and in file://")
           << true;
    QTest::newRow("namespaces cannot be overridden by types 1") <<
           "import QtQuick 2.0 as Rectangle\n"
           "import org.qtproject.installedtest 1.5\n"
           "Rectangle {}"
           << ""
           << "Namespace Rectangle cannot be used as a type"
           << false;
    QTest::newRow("namespaces cannot be overridden by types 2") <<
           "import QtQuick 2.0 as Rectangle\n"
           "import org.qtproject.installedtest 1.5\n"
           "Rectangle.Image {}"
           << "QQuickImage"
           << ""
           << false;
    QTest::newRow("local last 1") <<
           "LocalLast {}"
           << "QQuickText"
           << ""
           << false;
    QTest::newRow("local last 2") <<
           "import org.qtproject.installedtest 1.0\n"
           "LocalLast {}"
           << (!qmlCheckTypes()?"QQuickRectangle":"")// i.e. from org.qtproject.installedtest, not data/LocalLast.qml
           << (!qmlCheckTypes()?"":"LocalLast is ambiguous. Found in lib/org/qtproject/installedtest/ and in ")
           << false;
    QTest::newRow("local last 3") << //Forces it to load the local qmldir to resolve types, but they shouldn't override anything
           "import org.qtproject.installedtest 1.0\n"
           "LocalLast {LocalLast2{}}"
           << (!qmlCheckTypes()?"QQuickRectangle":"")// i.e. from org.qtproject.installedtest, not data/LocalLast.qml
           << (!qmlCheckTypes()?"":"LocalLast is ambiguous. Found in lib/org/qtproject/installedtest/ and in ")
           << false;
}

void tst_qqmllanguage::importsOrder()
{
    QFETCH(QString, qml);
    QFETCH(QString, type);
    QFETCH(QString, error);
    QFETCH(bool, partialMatch);
    testType(qml,type,error,partialMatch);
}

void tst_qqmllanguage::importIncorrectCase()
{
    if (engine.importPathList() == defaultImportPathList)
        engine.addImportPath(testFile("lib"));

    // Load "importIncorrectCase.qml" using wrong case
    QQmlComponent component(&engine, testFileUrl("ImportIncorrectCase.qml"));

    QList<QQmlError> errors = component.errors();
    QCOMPARE(errors.size(), 1);

    const QString expectedError = isCaseSensitiveFileSystem(dataDirectory()) ?
        QStringLiteral("No such file or directory") :
        QStringLiteral("File name case mismatch");
    QCOMPARE(errors.at(0).description(), expectedError);

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::importJs_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");
    QTest::addColumn<bool>("performTest");

    QTest::newRow("defaultVersion")
        << "importJs.1.qml"
        << "importJs.1.errors.txt"
        << true;

    QTest::newRow("specifiedVersion")
        << "importJs.2.qml"
        << "importJs.2.errors.txt"
        << true;

    QTest::newRow("excludeExcessiveVersion")
        << "importJs.3.qml"
        << "importJs.3.errors.txt"
        << false;

    QTest::newRow("includeAppropriateVersion")
        << "importJs.4.qml"
        << "importJs.4.errors.txt"
        << true;

    QTest::newRow("noDefaultVersion")
        << "importJs.5.qml"
        << "importJs.5.errors.txt"
        << false;

    QTest::newRow("repeatImportFails")
        << "importJs.6.qml"
        << "importJs.6.errors.txt"
        << false;

    QTest::newRow("multipleVersionImportFails")
        << "importJs.7.qml"
        << "importJs.7.errors.txt"
        << false;

    QTest::newRow("namespacedImport")
        << "importJs.8.qml"
        << "importJs.8.errors.txt"
        << true;

    QTest::newRow("namespacedVersionedImport")
        << "importJs.9.qml"
        << "importJs.9.errors.txt"
        << true;

    QTest::newRow("namespacedRepeatImport")
        << "importJs.10.qml"
        << "importJs.10.errors.txt"
        << true;

    QTest::newRow("emptyScript")
        << "importJs.11.qml"
        << "importJs.11.errors.txt"
        << true;
}

void tst_qqmllanguage::importJs()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);
    QFETCH(bool, performTest);

    engine.setImportPathList(QStringList(defaultImportPathList) << testFile("lib"));

    QQmlComponent component(&engine, testFileUrl(file));

    {
        DETERMINE_ERRORS(errorFile,expected,actual);
        QCOMPARE(actual.size(), expected.size());
        for (int i = 0; i < expected.size(); ++i)
        {
            const int compareLen = qMin(expected.at(i).size(), actual.at(i).size());
            QCOMPARE(actual.at(i).left(compareLen), expected.at(i).left(compareLen));
        }
    }

    if (performTest) {
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QCOMPARE(object->property("test").toBool(),true);
    }

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::importJsModule_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("plainImport")
        << "importJsModule.1.qml";

    QTest::newRow("ImportQmlStyle")
        << "importJsModule.2.qml";

    QTest::newRow("plainImportWithCycle")
        << "importJsModule.3.qml";
}

void tst_qqmllanguage::importJsModule()
{
    QFETCH(QString, file);

    engine.setImportPathList(QStringList(defaultImportPathList) << testFile("lib"));
    auto importPathGuard = qScopeGuard([this]{
        engine.setImportPathList(defaultImportPathList);
    });

    QQmlComponent component(&engine, testFileUrl(file));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("test").toBool(),true);
}

void tst_qqmllanguage::explicitSelfImport()
{
    engine.setImportPathList(QStringList(defaultImportPathList) << testFile("lib"));

    QQmlComponent component(&engine, testFileUrl("mixedModuleWithSelfImport.qml"));
    QVERIFY(component.errors().size() == 0);

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::importInternalType()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());

    {
        QQmlComponent component(&engine);
        component.setData("import modulewithinternaltypes 1.0\nPublicType{}", QUrl());
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVERIFY(obj->property("myInternalType").value<QObject*>() != 0);
    }
    {
        QQmlComponent component(&engine);
        component.setData("import modulewithinternaltypes 1.0\nPublicTypeWithExplicitImport{}", QUrl());
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVERIFY(obj->property("myInternalType").value<QObject*>() != 0);
    }
}

void tst_qqmllanguage::qmlAttachedPropertiesObjectMethod()
{
    QObject object;

    QCOMPARE(qmlAttachedPropertiesObject<MyQmlObject>(&object, false), (QObject *)nullptr);
    QVERIFY(qmlAttachedPropertiesObject<MyQmlObject>(&object, true));

    {
        QQmlComponent component(&engine, testFileUrl("qmlAttachedPropertiesObjectMethod.1.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(qmlAttachedPropertiesObject<MyQmlObject>(object.data(), false), (QObject *)nullptr);
        QVERIFY(qmlAttachedPropertiesObject<MyQmlObject>(object.data(), true) != nullptr);
    }

    {
        QQmlComponent component(&engine, testFileUrl("qmlAttachedPropertiesObjectMethod.2.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QVERIFY(qmlAttachedPropertiesObject<MyQmlObject>(object.data(), false) != nullptr);
        QVERIFY(qmlAttachedPropertiesObject<MyQmlObject>(object.data(), true) != nullptr);
    }
}

void tst_qqmllanguage::crash1()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nComponent {}", QUrl());
}

void tst_qqmllanguage::crash2()
{
    QQmlComponent component(&engine, testFileUrl("crash2.qml"));
}

// QTBUG-8676
void tst_qqmllanguage::customOnProperty()
{
    QQmlComponent component(&engine, testFileUrl("customOnProperty.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("on").toInt(), 10);
}

// QTBUG-12601
void tst_qqmllanguage::variantNotify()
{
    QQmlComponent component(&engine, testFileUrl("variantNotify.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QEXPECT_FAIL("", "var properties always trigger notify", Continue);
    QCOMPARE(object->property("notifyCount").toInt(), 1);
}

void tst_qqmllanguage::revisions()
{
    {
        QQmlComponent component(&engine, testFileUrl("revisions11.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<MyRevisionedClass> object(qobject_cast<MyRevisionedClass*>(component.create()));
        QVERIFY(object != nullptr);

        QCOMPARE(object->prop2(), 10.0);
    }
    {
        QQmlEngine myEngine;
        QQmlComponent component(&myEngine, testFileUrl("revisionssub11.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<MyRevisionedSubclass> object(qobject_cast<MyRevisionedSubclass*>(component.create()));
        QVERIFY(object != nullptr);

        QCOMPARE(object->prop1(), 10.0);
        QCOMPARE(object->prop2(), 10.0);
        QCOMPARE(object->prop3(), 10.0);
        QCOMPARE(object->prop4(), 10.0);
    }
    {
        QQmlComponent component(&engine, testFileUrl("versionedbase.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<MySubclass> object(qobject_cast<MySubclass*>(component.create()));
        QVERIFY(object != nullptr);

        QCOMPARE(object->prop1(), 10.0);
        QCOMPARE(object->prop2(), 10.0);
    }
}

void tst_qqmllanguage::revisionOverloads()
{
    {
    QQmlComponent component(&engine, testFileUrl("allowedRevisionOverloads.qml"));
    VERIFY_ERRORS(0);
    }
    {
    QQmlComponent component(&engine, testFileUrl("disallowedRevisionOverloads.qml"));
    QEXPECT_FAIL("", "QTBUG-13849", Abort);
    QVERIFY(0);
    VERIFY_ERRORS("disallowedRevisionOverloads.errors.txt");
    }
}

void tst_qqmllanguage::subclassedUncreateableRevision_data()
{
    QTest::addColumn<QString>("version");
    QTest::addColumn<QString>("prop");
    QTest::addColumn<bool>("shouldWork");

    QTest::newRow("prop1 exists in 1.0") << "1.0" << "prop1" << true;
    QTest::newRow("prop2 does not exist in 1.0") << "1.0" << "prop2" << false;
    QTest::newRow("prop3 does not exist in 1.0") << "1.0" << "prop3" << false;

    QTest::newRow("prop1 exists in 1.1") << "1.1" << "prop1" << true;
    QTest::newRow("prop2 works because it's re-declared in Derived") << "1.1" << "prop2" << true;
    QTest::newRow("prop3 only works if the Base REVISION 1 is picked up") << "1.1" << "prop3" << true;

}

void tst_qqmllanguage::subclassedUncreateableRevision()
{
    QFETCH(QString, version);
    QFETCH(QString, prop);
    QFETCH(bool, shouldWork);

    {
        QQmlEngine engine;
        QString qml = QString("import QtQuick 2.0\nimport Test %1\nMyUncreateableBaseClass {}").arg(version);
        QQmlComponent c(&engine);
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
        c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
        QScopedPointer<QObject> obj(c.create());
        QCOMPARE(obj.data(), static_cast<QObject*>(nullptr));
        QCOMPARE(c.errors().size(), 1);
        QCOMPARE(c.errors().first().description(), QString("Cannot create MyUncreateableBaseClass"));
    }

    QQmlEngine engine;
    QString qml = QString("import QtQuick 2.0\nimport Test %1\nMyCreateableDerivedClass {\n%3: true\n}").arg(version).arg(prop);
    QQmlComponent c(&engine);
    if (!shouldWork)
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
    QScopedPointer<QObject> obj(c.create());
    if (!shouldWork) {
        QCOMPARE(obj.data(), static_cast<QObject*>(nullptr));
        return;
    }

    QVERIFY(obj);
    MyUncreateableBaseClass *base = qobject_cast<MyUncreateableBaseClass*>(obj.data());
    QVERIFY(base);
    QCOMPARE(base->property(prop.toLatin1()).toBool(), true);
}

void tst_qqmllanguage::subclassedExtendedUncreateableRevision_data()
{
    QTest::addColumn<QString>("version");
    QTest::addColumn<QString>("prop");
    QTest::addColumn<bool>("shouldWork");

    QTest::newRow("prop1 exists in 1.0") << "1.0" << "prop1" << true;
    QTest::newRow("prop2 does not exist in 1.0") << "1.0" << "prop2" << false;
    QTest::newRow("prop3 does not exist in 1.0") << "1.0" << "prop3" << false;
    QTest::newRow("prop4 exists in 1.0") << "1.0" << "prop4" << true;
    QTest::newRow("prop5 exists in 1.0") << "1.0" << "prop5" << true;

    QTest::newRow("prop1 exists in 1.1") << "1.1" << "prop1" << true;
    QTest::newRow("prop2 exists in 1.1") << "1.1" << "prop2" << true;
    QTest::newRow("prop3 exists in 1.1") << "1.1" << "prop3" << true;
    QTest::newRow("prop4 exists in 1.1") << "1.1" << "prop4" << true;
    QTest::newRow("prop5 exists in 1.1") << "1.1" << "prop5" << true;
}

void tst_qqmllanguage::subclassedExtendedUncreateableRevision()
{
    QFETCH(QString, version);
    QFETCH(QString, prop);
    QFETCH(bool, shouldWork);

    {
        QQmlEngine engine;
        QString qml = QString("import QtQuick 2.0\nimport Test %1\nMyExtendedUncreateableBaseClass {}").arg(version);
        QQmlComponent c(&engine);
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
        c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
        QScopedPointer<QObject> obj(c.create());
        QCOMPARE(obj.data(), static_cast<QObject*>(nullptr));
        QCOMPARE(c.errors().size(), 1);
        QCOMPARE(c.errors().first().description(), QString("Cannot create MyExtendedUncreateableBaseClass"));
    }

    QQmlEngine engine;
    QString qml = QString("import QtQuick 2.0\nimport Test %1\nMyExtendedCreateableDerivedClass {\n%3: true\n}").arg(version).arg(prop);
    QQmlComponent c(&engine);
    if (!shouldWork)
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
    QScopedPointer<QObject> obj(c.create());
    if (!shouldWork) {
        QCOMPARE(obj.data(), static_cast<QObject*>(nullptr));
        return;
    }

    QVERIFY(obj);
    MyExtendedUncreateableBaseClass *base = qobject_cast<MyExtendedUncreateableBaseClass*>(obj.data());
    QVERIFY(base);
    QCOMPARE(base->property(prop.toLatin1()).toBool(), true);
}

void tst_qqmllanguage::uncreatableTypesAsProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("uncreatableTypeAsProperty.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
}

tst_qqmllanguage::tst_qqmllanguage()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qqmllanguage::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (dataDirectoryUrl().scheme() == QLatin1String("qrc"))
        engine.addImportPath(dataDirectory());
    else
        QVERIFY2(QDir::setCurrent(dataDirectory()), qPrintable("Could not chdir to " + dataDirectory()));


    defaultImportPathList = engine.importPathList();

    registerTypes();
    // Registered here because it uses testFileUrl
    qmlRegisterType(testFileUrl("CompositeType.qml"), "Test", 1, 0, "RegisteredCompositeType");
    qmlRegisterType(testFileUrl("CompositeType.DoesNotExist.qml"), "Test", 1, 0, "RegisteredCompositeType2");
    qmlRegisterType(testFileUrl("invalidRoot.1.qml"), "Test", 1, 0, "RegisteredCompositeType3");
    qmlRegisterType(testFileUrl("CompositeTypeWithEnum.qml"), "Test", 1, 0, "RegisteredCompositeTypeWithEnum");
    qmlRegisterType(testFileUrl("CompositeTypeWithAttachedProperty.qml"), "Test", 1, 0, "RegisteredCompositeTypeWithAttachedProperty");
    qmlRegisterType(testFileUrl("CompositeTypeWithEnumSelfReference.qml"), "Test", 1, 0, "CompositeTypeWithEnumSelfReference");

    // Registering the TestType class in other modules should have no adverse effects
    qmlRegisterType<TestType>("org.qtproject.TestPre", 1, 0, "Test");

    qmlRegisterType<TestType>("org.qtproject.Test", 0, 0, "TestTP");
    qmlRegisterType<TestType>("org.qtproject.Test", 1, 0, "Test");
    qmlRegisterType<TestType>("org.qtproject.Test", 1, 5, "Test");
    qmlRegisterType<TestType2>("org.qtproject.Test", 1, 8, "Test");
    qmlRegisterType<TestType>("org.qtproject.Test", 1, 9, "OldTest");
    qmlRegisterType<TestType2>("org.qtproject.Test", 1, 12, "Test");

    // Registering the TestType class in other modules should have no adverse effects
    qmlRegisterType<TestType>("org.qtproject.TestPost", 1, 0, "Test");

    // Create locale-specific file
    // For POSIX, this will just be data/I18nType.qml, since POSIX is 7-bit
    // For iso8859-1 locale, this will just be data/I18nType?????.qml where ????? is 5 8-bit characters
    // For utf-8 locale, this will be data/I18nType??????????.qml where ?????????? is 5 8-bit characters, UTF-8 encoded
    if (dataDirectoryUrl().scheme() != QLatin1String("qrc")) {
        QFile in(testFileUrl(QLatin1String("I18nType30.qml")).toLocalFile());
        QVERIFY2(in.open(QIODevice::ReadOnly), qPrintable(QString::fromLatin1("Cannot open '%1': %2").arg(in.fileName(), in.errorString())));
        QFile out(testFileUrl(QString::fromUtf8("I18nType\303\201\303\242\303\243\303\244\303\245.qml")).toLocalFile());
        QVERIFY2(out.open(QIODevice::WriteOnly), qPrintable(QString::fromLatin1("Cannot open '%1': %2").arg(out.fileName(), out.errorString())));
        out.write(in.readAll());
    }

    // Register a Composite Singleton.
    qmlRegisterSingletonType(testFileUrl("singleton/RegisteredCompositeSingletonType.qml"), "org.qtproject.Test", 1, 0, "RegisteredSingleton");
    qmlRegisterType(testFileUrl("Comps/OverlayDrawer.qml"), "Comps", 2, 0, "OverlayDrawer");
}

void tst_qqmllanguage::aliasPropertyChangeSignals()
{
    {
        QQmlComponent component(&engine, testFileUrl("aliasPropertyChangeSignals.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        QCOMPARE(o->property("test").toBool(), true);
    }

    // QTCREATORBUG-2769
    {
        QQmlComponent component(&engine, testFileUrl("aliasPropertyChangeSignals.2.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        QCOMPARE(o->property("test").toBool(), true);
    }
}

// Tests property initializers
void tst_qqmllanguage::propertyInit()
{
    {
        QQmlComponent component(&engine, testFileUrl("propertyInit.1.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        QCOMPARE(o->property("test").toInt(), 1);
    }

    {
        QQmlComponent component(&engine, testFileUrl("propertyInit.2.qml"));

        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        QCOMPARE(o->property("test").toInt(), 123);
    }
}

// Test that registration order doesn't break type availability
// QTBUG-16878
void tst_qqmllanguage::registrationOrder()
{
    QQmlComponent component(&engine, testFileUrl("registrationOrder.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);
    QCOMPARE(o->metaObject(), &MyVersion2Class::staticMetaObject);
}

void tst_qqmllanguage::readonly()
{
    QQmlComponent component(&engine, testFileUrl("readonly.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    QCOMPARE(o->property("test1").toInt(), 10);
    QCOMPARE(o->property("test2").toInt(), 18);
    QCOMPARE(o->property("test3").toInt(), 13);

    o->setProperty("testData", 13);

    QCOMPARE(o->property("test1").toInt(), 10);
    QCOMPARE(o->property("test2").toInt(), 22);
    QCOMPARE(o->property("test3").toInt(), 13);

    o->setProperty("testData2", 2);

    QCOMPARE(o->property("test1").toInt(), 10);
    QCOMPARE(o->property("test2").toInt(), 22);
    QCOMPARE(o->property("test3").toInt(), 2);

    o->setProperty("test1", 11);
    o->setProperty("test2", 11);
    o->setProperty("test3", 11);

    QCOMPARE(o->property("test1").toInt(), 10);
    QCOMPARE(o->property("test2").toInt(), 22);
    QCOMPARE(o->property("test3").toInt(), 2);
}

void tst_qqmllanguage::readonlyObjectProperties()
{
    QQmlComponent component(&engine, testFileUrl("readonlyObjectProperty.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQmlProperty prop(o.data(), QStringLiteral("subObject"), &engine);
    QVERIFY(!prop.isWritable());
    QVERIFY(!prop.write(QVariant::fromValue(o.data())));

    QObject *subObject = qvariant_cast<QObject*>(prop.read());
    QVERIFY(subObject);
    QCOMPARE(subObject->property("readWrite").toInt(), int(42));
    subObject->setProperty("readWrite", QVariant::fromValue(int(100)));
    QCOMPARE(subObject->property("readWrite").toInt(), int(100));
}

void tst_qqmllanguage::receivers()
{
    QQmlComponent component(&engine, testFileUrl("receivers.qml"));

    QScopedPointer<MyReceiversTestObject> o(qobject_cast<MyReceiversTestObject*>(component.create()));
    QVERIFY(o != nullptr);
    QCOMPARE(o->mySignalCount(), 1);
    QCOMPARE(o->propChangedCount(), 2);
    QCOMPARE(o->myUnconnectedSignalCount(), 0);

    QVERIFY(o->isSignalConnected(QMetaMethod::fromSignal(&MyReceiversTestObject::mySignal)));
    QVERIFY(o->isSignalConnected(QMetaMethod::fromSignal(&MyReceiversTestObject::propChanged)));
    QVERIFY(!o->isSignalConnected(QMetaMethod::fromSignal(&MyReceiversTestObject::myUnconnectedSignal)));
}

void tst_qqmllanguage::registeredCompositeType()
{
    QQmlComponent component(&engine, testFileUrl("registeredCompositeType.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);
}

// QTBUG-43582
void tst_qqmllanguage::registeredCompositeTypeWithEnum()
{
    QQmlComponent component(&engine, testFileUrl("registeredCompositeTypeWithEnum.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    QCOMPARE(o->property("enumValue0").toInt(), static_cast<int>(MyCompositeBaseType::EnumValue0));
    QCOMPARE(o->property("enumValue42").toInt(), static_cast<int>(MyCompositeBaseType::EnumValue42));
    QCOMPARE(o->property("enumValue15").toInt(), static_cast<int>(MyCompositeBaseType::ScopedCompositeEnum::EnumValue15));

    {
        QQmlComponent component(&engine);
        component.setData("import Test\nCompositeTypeWithEnumSelfReference {}", QUrl());
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        QCOMPARE(o->property("e").toInt(), 1);
        QCOMPARE(o->property("f").toInt(), 2);
    }
}

// QTBUG-43581
void tst_qqmllanguage::registeredCompositeTypeWithAttachedProperty()
{
    QQmlComponent component(&engine, testFileUrl("registeredCompositeTypeWithAttachedProperty.qml"));

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    QCOMPARE(o->property("attachedProperty").toString(), QStringLiteral("test"));
}

// QTBUG-18268
void tst_qqmllanguage::remoteLoadCrash()
{
    ThreadedTestHTTPServer server(dataDirectory());

    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Text {}", server.url("/remoteLoadCrash.qml"));
    while (component.isLoading())
        QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 50);

    QScopedPointer<QObject> o(component.create());
}

void tst_qqmllanguage::signalWithDefaultArg()
{
    QQmlComponent component(&engine, testFileUrl("signalWithDefaultArg.qml"));

    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("signalCount").toInt(), 0);
    QCOMPARE(object->property("signalArg").toInt(), 0);

    emit object->signalWithDefaultArg();
    QCOMPARE(object-> property("signalCount").toInt(), 1);
    QCOMPARE(object->property("signalArg").toInt(), 5);

    emit object->signalWithDefaultArg(15);
    QCOMPARE(object->property("signalCount").toInt(), 2);
    QCOMPARE(object->property("signalArg").toInt(), 15);


    QMetaObject::invokeMethod(object.data(), "emitNoArgSignal");
    QCOMPARE(object->property("signalCount").toInt(), 3);
    QCOMPARE(object->property("signalArg").toInt(), 5);

    QMetaObject::invokeMethod(object.data(), "emitArgSignal");
    QCOMPARE(object->property("signalCount").toInt(), 4);
    QCOMPARE(object->property("signalArg").toInt(), 22);
}

void tst_qqmllanguage::signalParameterTypes()
{
    // bound signal handlers
    {
    QQmlComponent component(&engine, testFileUrl("signalParameterTypes.1.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj != nullptr);
    QVERIFY(obj->property("success").toBool());
    }

    // dynamic signal connections
    {
    QQmlComponent component(&engine, testFileUrl("signalParameterTypes.2.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj != nullptr);
    QVERIFY(obj->property("success").toBool());
    }

    // dynamic signal connections
    {
    QQmlComponent component(&engine, testFileUrl("signalParameterTypes.3.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj != nullptr);
    QVERIFY(obj->property("success").toBool());
    }
}

void tst_qqmllanguage::functionParameterTypes()
{
    QQmlComponent component(&engine, testFileUrl("functionParameterTypes.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(!obj.isNull(), qPrintable(component.errorString()));
    const QMetaObject *metaObject = obj->metaObject();

    {
        QMetaMethod slot = metaObject->method(metaObject->indexOfSlot("returnItem()"));
        QVERIFY(slot.isValid());
        QCOMPARE(slot.returnType(), QMetaType::QObjectStar);
        QObject *returnedPtr = nullptr;
        slot.invoke(obj.data(), Qt::DirectConnection, Q_RETURN_ARG(QObject*, returnedPtr));
        QCOMPARE(returnedPtr, obj.data());
    }

    {
        QMetaMethod slot = metaObject->method(metaObject->indexOfSlot("takeString(QString)"));
        QVERIFY(slot.isValid());
        QCOMPARE(slot.parameterCount(), 1);
        QCOMPARE(slot.parameterType(0), int(QMetaType::QString));
    }
}

// QTBUG-20639
void tst_qqmllanguage::globalEnums()
{
    qRegisterMetaType<MyEnum1Class::EnumA>();
    qRegisterMetaType<MyEnum2Class::EnumB>();
    qRegisterMetaType<Qt::TextFormat>();

    QQmlComponent component(&engine, testFileUrl("globalEnums.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    MyEnum1Class *enum1Class = o->findChild<MyEnum1Class *>(QString::fromLatin1("enum1Class"));
    QVERIFY(enum1Class != nullptr);
    QVERIFY(enum1Class->getValue() == -1);

    MyEnumDerivedClass *enum2Class = o->findChild<MyEnumDerivedClass *>(QString::fromLatin1("enumDerivedClass"));
    QVERIFY(enum2Class != nullptr);
    QVERIFY(enum2Class->getValueA() == -1);
    QVERIFY(enum2Class->getValueB() == -1);
    QVERIFY(enum2Class->getValueC() == 0);
    QVERIFY(enum2Class->getValueD() == 0);
    QVERIFY(enum2Class->getValueE() == -1);
    QVERIFY(enum2Class->getValueE2() == -1);

    QVERIFY(enum2Class->property("aValue") == 0);
    QVERIFY(enum2Class->property("bValue") == 0);
    QVERIFY(enum2Class->property("cValue") == 0);
    QVERIFY(enum2Class->property("dValue") == 0);
    QVERIFY(enum2Class->property("eValue") == 0);
    QVERIFY(enum2Class->property("e2Value") == 0);

    QSignalSpy signalA(enum2Class, SIGNAL(valueAChanged(MyEnum1Class::EnumA)));
    QSignalSpy signalB(enum2Class, SIGNAL(valueBChanged(MyEnum2Class::EnumB)));

    QMetaObject::invokeMethod(o.data(), "setEnumValues");

    QVERIFY(enum1Class->getValue() == MyEnum1Class::A_13);
    QVERIFY(enum2Class->getValueA() == MyEnum1Class::A_11);
    QVERIFY(enum2Class->getValueB() == MyEnum2Class::B_37);
    QVERIFY(enum2Class->getValueC() == Qt::RichText);
    QVERIFY(enum2Class->getValueD() == Qt::ElideMiddle);
    QVERIFY(enum2Class->getValueE() == MyEnum2Class::E_14);
    QVERIFY(enum2Class->getValueE2() == MyEnum2Class::E_76);

    QVERIFY(signalA.size() == 1);
    QVERIFY(signalB.size() == 1);

    QVERIFY(enum2Class->property("aValue") == MyEnum1Class::A_11);
    QVERIFY(enum2Class->property("bValue") == 37);
    QVERIFY(enum2Class->property("cValue") == 1);
    QVERIFY(enum2Class->property("dValue") == 2);
    QVERIFY(enum2Class->property("eValue") == 14);
    QVERIFY(enum2Class->property("e2Value") == 76);
}

void tst_qqmllanguage::lowercaseEnumRuntime_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("enum from normal type") << "lowercaseEnumRuntime.1.qml";
    QTest::newRow("enum from singleton type") << "lowercaseEnumRuntime.2.qml";
}

void tst_qqmllanguage::lowercaseEnumRuntime()
{
    QFETCH(QString, file);

    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(0);
    std::unique_ptr<QObject> root { component.create() };
    QVERIFY(root);
}

void tst_qqmllanguage::lowercaseEnumCompileTime_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("assignment to int property") << "lowercaseEnumCompileTime.1.qml";
    QTest::newRow("assignment to enum property") << "lowercaseEnumCompileTime.2.qml";
}

void tst_qqmllanguage::lowercaseEnumCompileTime()
{
    QFETCH(QString, file);

    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(0);
    std::unique_ptr<QObject> root { component.create() };
    QVERIFY(root);
}

void tst_qqmllanguage::scopedEnum()
{
    QQmlComponent component(&engine, testFileUrl("scopedEnum.qml"));

    QScopedPointer<MyTypeObject> o(qobject_cast<MyTypeObject *>(component.create()));
    QVERIFY(o != nullptr);

    QCOMPARE(o->scopedEnum(), MyTypeObject::MyScopedEnum::ScopedVal1);
    QCOMPARE(o->intProperty(), (int)MyTypeObject::MyScopedEnum::ScopedVal2);
    QCOMPARE(o->property("listValue").toInt(), (int)MyTypeObject::MyScopedEnum::ScopedVal3);
    QCOMPARE(o->property("noScope").toInt(), (int)MyTypeObject::MyScopedEnum::ScopedVal1);

    QMetaObject::invokeMethod(o.data(), "assignNewValue");
    QCOMPARE(o->scopedEnum(), MyTypeObject::MyScopedEnum::ScopedVal2);
    QCOMPARE(o->property("noScope").toInt(), (int)MyTypeObject::MyScopedEnum::ScopedVal2);
}

void tst_qqmllanguage::scopedEnumsWithNameClash()
{
    auto typeId = qmlRegisterUncreatableMetaObject(
                ScopedEnumsWithNameClash::staticMetaObject,
                "ScopedEnumsWithNameClashTest", 1, 0, "ScopedEnum", "Dummy reason");
    auto registryGuard = qScopeGuard([typeId]() {
        QQmlMetaType::unregisterType(typeId);
    });

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("scopedEnumsWithNameClash.qml"));

    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "Previously registered enum will be overwritten due to name clash: ScopedEnumsWithNameClash.ScopedVal1");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "Previously registered enum will be overwritten due to name clash: ScopedEnumsWithNameClash.ScopedVal2");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "Previously registered enum will be overwritten due to name clash: ScopedEnumsWithNameClash.ScopedVal3");

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj != nullptr);
    QVERIFY(obj->property("success").toBool());
}

void tst_qqmllanguage::scopedEnumsWithResolvedNameClash()
{
    auto typeId = qmlRegisterUncreatableMetaObject(
                ScopedEnumsWithResolvedNameClash::staticMetaObject,
                "ScopedEnumsWithResolvedNameClashTest", 1, 0, "ScopedEnum", "Dummy reason");
    auto registryGuard = qScopeGuard([typeId]() {
        QQmlMetaType::unregisterType(typeId);
    });

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("scopedEnumsWithResolvedNameClash.qml"));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj != nullptr);
    QVERIFY(obj->property("success").toBool());
}

class ObjectA : public QObject
{
    Q_OBJECT

public:

    enum TestEnum {
        Default = 42,
        TestValue1,
        TestValue2
    };
    Q_ENUM(TestEnum)

    ObjectA() = default;
};

class ObjectB : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ObjectA::TestEnum testEnum READ testEnum WRITE setTestEnum NOTIFY testEnumChanged)

public:
    ObjectB() = default;
    ObjectA::TestEnum testEnum() const {return m_testEnum;}

public slots:
    void setTestEnum(ObjectA::TestEnum testEnum) {auto old = m_testEnum; m_testEnum = testEnum; if (old != m_testEnum) testEnumChanged(m_testEnum);}
signals:
    void testEnumChanged(ObjectA::TestEnum testEnum);
private:
    ObjectA::TestEnum m_testEnum = ObjectA::Default;
};

class ObjectC : public ObjectB
{
    Q_OBJECT

public:
    ObjectC() = default;
};

void tst_qqmllanguage::enumNoScopeLeak()
{
    qmlRegisterType<ObjectA>("test", 1, 0, "ObjectA");
    qmlRegisterType<ObjectB>("test", 1, 0, "ObjectB");
    qmlRegisterType<ObjectC>("test", 1, 0, "ObjectC");
    QQmlEngine engine;
    auto url = testFileUrl("enumScopeLeak.qml");
    QQmlComponent component(&engine, url);
    const auto msg = url.toString() + ":5:5: Unable to assign [undefined] to ObjectA::TestEnum";
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, msg.toUtf8().constData());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    QCOMPARE(root->property("testEnum").toInt(), ObjectA::Default);
}

void tst_qqmllanguage::qmlEnums()
{
    QQmlEngine engine;
    engine.setImportPathList(QStringList(defaultImportPathList) << testFile("lib"));

    {
        QQmlComponent component(&engine, testFileUrl("TypeWithEnum.qml"));
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o);
        QCOMPARE(o->property("enumValue").toInt(), 1);
        QCOMPARE(o->property("enumValue2").toInt(), 2);
        QCOMPARE(o->property("scopedEnumValue").toInt(), 1);

        QCOMPARE(o->property("otherEnumValue1").toInt(), 24);
        QCOMPARE(o->property("otherEnumValue2").toInt(), 25);
        QCOMPARE(o->property("otherEnumValue3").toInt(), 24);
        QCOMPARE(o->property("otherEnumValue4").toInt(), 25);
        QCOMPARE(o->property("otherEnumValue5").toInt(), 1);
        QCOMPARE(o->property("otherEnumValue6").toInt(), -42);
    }

    {
        QQmlComponent component(&engine, testFileUrl("usingTypeWithEnum.qml"));
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o);
        QCOMPARE(o->property("enumValue").toInt(), 1);
        QCOMPARE(o->property("enumValue2").toInt(), 0);
        QCOMPARE(o->property("scopedEnumValue").toInt(), 2);
        QCOMPARE(o->property("enumValueFromSingleton").toInt(), 42);
         // while this next test verifies current duplication behavior, I'm not sure it should be codified
        QCOMPARE(o->property("duplicatedEnumValueFromSingleton").toInt(), 2);
        QCOMPARE(o->property("scopedEnumValueFromSingleton1").toInt(), 43);
        QCOMPARE(o->property("scopedEnumValueFromSingleton2").toInt(), 2);
        QCOMPARE(o->property("scopedEnumValueFromSingleton3").toInt(), 2);
    }
}

void tst_qqmllanguage::literals_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("hex") << "n1" << QVariant(0xfe32);
    // Octal integer literals are deprecated
//    QTest::newRow("octal") << "n2" << QVariant(015);
    QTest::newRow("fp1") << "n3" << QVariant(-4.2E11);
    QTest::newRow("fp2") << "n4" << QVariant(.1e9);
    QTest::newRow("fp3") << "n5" << QVariant(3e-12);
    QTest::newRow("fp4") << "n6" << QVariant(3e+12);
    QTest::newRow("fp5") << "n7" << QVariant(0.1e9);
    QTest::newRow("large-int1") << "n8" << QVariant((double) 1152921504606846976);
    QTest::newRow("large-int2") << "n9" << QVariant(100000000000000000000.);

    QTest::newRow("special1") << "c1" << QVariant(QString("\b"));
    QTest::newRow("special2") << "c2" << QVariant(QString("\f"));
    QTest::newRow("special3") << "c3" << QVariant(QString("\n"));
    QTest::newRow("special4") << "c4" << QVariant(QString("\r"));
    QTest::newRow("special5") << "c5" << QVariant(QString("\t"));
    QTest::newRow("special6") << "c6" << QVariant(QString("\v"));
    QTest::newRow("special7") << "c7" << QVariant(QString("\'"));
    QTest::newRow("special8") << "c8" << QVariant(QString("\""));
    QTest::newRow("special9") << "c9" << QVariant(QString("\\"));
    // We don't handle octal escape sequences
    QTest::newRow("special10") << "c10" << QVariant(QString(1, QChar(0xa9)));
    QTest::newRow("special11") << "c11" << QVariant(QString(1, QChar(0x00A9)));
}

void tst_qqmllanguage::literals()
{
    QFETCH(QString, property);
    QFETCH(QVariant, value);

    QQmlComponent component(&engine, testFile("literals.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property(property.toLatin1()), value);
}

void tst_qqmllanguage::objectDeletionNotify_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("property QtObject") << "objectDeletionNotify.1.qml";
    QTest::newRow("property variant") << "objectDeletionNotify.2.qml";
    QTest::newRow("property var") << "objectDeletionNotify.3.qml";
    QTest::newRow("property var guard removed") << "objectDeletionNotify.4.qml";
}

void tst_qqmllanguage::objectDeletionNotify()
{
    QFETCH(QString, file);

    QQmlComponent component(&engine, testFile(file));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("success").toBool(), true);

    QMetaObject::invokeMethod(object.data(), "destroyObject");

    // Process the deletion event
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(object->property("success").toBool(), true);
}

void tst_qqmllanguage::scopedProperties()
{
    QQmlComponent component(&engine, testFileUrl("scopedProperties.qml"));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);
    QVERIFY(o->property("success").toBool());
}

void tst_qqmllanguage::deepProperty()
{
    QQmlComponent component(&engine, testFileUrl("deepProperty.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);
    QFont font = qvariant_cast<QFont>(qvariant_cast<QObject*>(o->property("someObject"))->property("font"));
    QCOMPARE(font.family(), QStringLiteral("test"));
}

void tst_qqmllanguage::groupAssignmentFailure()
{
    auto ep = std::make_unique<QQmlEngine>();
    QTest::failOnWarning("QQmlComponent: Component destroyed while completion pending");
    QQmlComponent component(ep.get(), testFileUrl("groupFailure.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o);
    ep.reset();
    // ~QQmlComponent should not crash here
}

// Tests that the implicit import has lowest precedence, in the case where
// there are conflicting types and types only found in the local import.
// Tests that just check one (or the root) type are in ::importsOrder
void tst_qqmllanguage::implicitImportsLast()
{
    if (qmlCheckTypes())
        QSKIP("This test is about maintaining the same choice when type is ambiguous.");

    if (engine.importPathList() == defaultImportPathList)
        engine.addImportPath(testFile("lib"));

    QQmlComponent component(&engine, testFileUrl("localOrderTest.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(QString(object->metaObject()->superClass()->superClass()->className())
            .startsWith(QLatin1String("QQuickMouseArea")));
    QObject* object2 = object->property("item").value<QObject*>();
    QVERIFY(object2 != nullptr);
    QCOMPARE(QString(object2->metaObject()->superClass()->className()),
             QLatin1String("QQuickRectangle"));

    engine.setImportPathList(defaultImportPathList);
}

void tst_qqmllanguage::getSingletonInstance(QQmlEngine& engine, const char* fileName, const char* propertyName, QObject** result /* out */)
{
    QVERIFY(fileName != nullptr);
    QVERIFY(propertyName != nullptr);

    if (!fileName || !propertyName)
        return;

    QQmlComponent component(&engine, testFileUrl(fileName));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    getSingletonInstance(object.data(), propertyName, result);
}

void tst_qqmllanguage::getSingletonInstance(QObject* o, const char* propertyName, QObject** result /* out */)
{
    QVERIFY(o != nullptr);
    QVERIFY(propertyName != nullptr);

    if (!o || !propertyName)
        return;

    QVariant variant = o->property(propertyName);
    QVERIFY(variant.isValid());

    QObject *singleton = nullptr;
    if (variant.typeId() == qMetaTypeId<QObject *>())
        singleton = variant.value<QObject*>();
    else if (variant.typeId() == qMetaTypeId<QJSValue>())
        singleton = variant.value<QJSValue>().toQObject();

    QVERIFY(singleton != nullptr);
    *result = singleton;
}

void verifyCompositeSingletonPropertyValues(QObject* o, const char* n1, int v1, const char* n2, int v2)
{
    QCOMPARE(o->property(n1).typeId(), (int)QMetaType::Int);
    QCOMPARE(o->property(n1), QVariant(v1));

    QCOMPARE(o->property(n2).typeId(), QMetaType::QString);
    QString numStr;
    QCOMPARE(o->property(n2), QVariant(QString(QLatin1String("Test value: ")).append(numStr.setNum(v2))));
}

// Reads values from a composite singleton type
void tst_qqmllanguage::compositeSingletonProperties()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest1.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 125, "value2", -55);
}

// Checks that the addresses of the composite singletons used in the same
// engine are the same.
void tst_qqmllanguage::compositeSingletonSameEngine()
{
    QObject* s1 = nullptr;
    getSingletonInstance(engine, "singletonTest2.qml", "singleton1", &s1);
    QVERIFY(s1 != nullptr);
    s1->setProperty("testProp2", QVariant(13));

    QObject* s2 = nullptr;
    getSingletonInstance(engine, "singletonTest3.qml", "singleton2", &s2);
    QVERIFY(s2 != nullptr);
    QCOMPARE(s2->property("testProp2"), QVariant(13));

    QCOMPARE(s1, s2);
}

// Checks that the addresses of the composite singletons used in different
// engines are different.
void tst_qqmllanguage::compositeSingletonDifferentEngine()
{
    QQmlEngine e2;

    QObject* s1 = nullptr;
    getSingletonInstance(engine, "singletonTest2.qml", "singleton1", &s1);
    QVERIFY(s1 != nullptr);
    s1->setProperty("testProp2", QVariant(13));

    QObject* s2 = nullptr;
    getSingletonInstance(e2, "singletonTest3.qml", "singleton2", &s2);
    QVERIFY(s2 != nullptr);
    QCOMPARE(s2->property("testProp2"), QVariant(25));

    QVERIFY(s1 != s2);
}

// pragma Singleton in a non-type qml file fails
void tst_qqmllanguage::compositeSingletonNonTypeError()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest4.qml"));
    VERIFY_ERRORS("singletonTest4.error.txt");
}

// Loads the singleton using a namespace qualifier
void tst_qqmllanguage::compositeSingletonQualifiedNamespace()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest5.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 125, "value2", -55);

    // lets verify that the singleton instance we are using is the same
    // when loaded through another file (without namespace!)
    QObject *s1 = nullptr;
    getSingletonInstance(o.data(), "singletonInstance", &s1);
    QVERIFY(s1 != nullptr);

    QObject* s2 = nullptr;
    getSingletonInstance(engine, "singletonTest5a.qml", "singletonInstance", &s2);
    QVERIFY(s2 != nullptr);

    QCOMPARE(s1, s2);
}

// Loads a singleton from a module
void tst_qqmllanguage::compositeSingletonModule()
{
    engine.addImportPath(testFile("singleton/module"));

    QQmlComponent component(&engine, testFileUrl("singletonTest6.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 125, "value2", -55);
    verifyCompositeSingletonPropertyValues(o.data(), "value3", 125, "value4", -55);

    // lets verify that the singleton instance we are using is the same
    // when loaded through another file
    QObject *s1 = nullptr;
    getSingletonInstance(o.data(), "singletonInstance", &s1);
    QVERIFY(s1 != nullptr);

    QObject* s2 = nullptr;
    getSingletonInstance(engine, "singletonTest6a.qml", "singletonInstance", &s2);
    QVERIFY(s2 != nullptr);

    QCOMPARE(s1, s2);
}

// Loads a singleton from a module with a higher version
void tst_qqmllanguage::compositeSingletonModuleVersioned()
{
    engine.addImportPath(testFile("singleton/module"));

    QQmlComponent component(&engine, testFileUrl("singletonTest7.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 225, "value2", 55);
    verifyCompositeSingletonPropertyValues(o.data(), "value3", 225, "value4", 55);

    // lets verify that the singleton instance we are using is the same
    // when loaded through another file
    QObject *s1 = nullptr;
    getSingletonInstance(o.data(), "singletonInstance", &s1);
    QVERIFY(s1 != nullptr);

    QObject* s2 = nullptr;
    getSingletonInstance(engine, "singletonTest7a.qml", "singletonInstance", &s2);
    QVERIFY(s2 != nullptr);

    QCOMPARE(s1, s2);
}

// Loads a singleton from a module with a qualified namespace
void tst_qqmllanguage::compositeSingletonModuleQualified()
{
    engine.addImportPath(testFile("singleton/module"));

    QQmlComponent component(&engine, testFileUrl("singletonTest8.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 225, "value2", 55);
    verifyCompositeSingletonPropertyValues(o.data(), "value3", 225, "value4", 55);

    // lets verify that the singleton instance we are using is the same
    // when loaded through another file
    QObject *s1 = nullptr;
    getSingletonInstance(o.data(), "singletonInstance", &s1);
    QVERIFY(s1 != nullptr);

    QObject* s2 = nullptr;
    getSingletonInstance(engine, "singletonTest8a.qml", "singletonInstance", &s2);
    QVERIFY(s2 != nullptr);

    QCOMPARE(s1, s2);
}

// Tries to instantiate a type with a pragma Singleton and fails
void tst_qqmllanguage::compositeSingletonInstantiateError()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest9.qml"));
    VERIFY_ERRORS("singletonTest9.error.txt");
}

// Having a composite singleton type as dynamic property type is allowed
void tst_qqmllanguage::compositeSingletonDynamicPropertyError()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest10.qml"));
    VERIFY_ERRORS(0);
}

void tst_qqmllanguage::compositeSingletonDynamicSignalAndJavaScriptPragma()
{
    {
        // Having a composite singleton type as dynamic signal parameter succeeds
        // (like C++ singleton)

        QQmlComponent component(&engine, testFileUrl("singletonTest11.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        verifyCompositeSingletonPropertyValues(o.data(), "value1", 99, "value2", -55);
    }
    {
        // Load a composite singleton type and a javascript file that has .pragma library
        // in it. This will make sure that the javascript .pragma does not get mixed with
        // the pragma Singleton changes.

        QQmlComponent component(&engine, testFileUrl("singletonTest16.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);

        // The value1 that is read from the SingletonType was changed from 125 to 99
        // above. As the type is a singleton and
        // the engine has not been destroyed, we just retrieve the old instance and
        // the value is still 99.
        verifyCompositeSingletonPropertyValues(o.data(), "value1", 99, "value2", 333);
    }
}

// Use qmlRegisterType to register a qml composite type with pragma Singleton defined in it.
// This will fail as qmlRegisterType will only instantiate CompositeTypes.
void tst_qqmllanguage::compositeSingletonQmlRegisterTypeError()
{
    qmlRegisterType(testFileUrl("singleton/registeredComposite/CompositeType.qml"),
        "CompositeSingletonTest", 1, 0, "RegisteredCompositeType");
    QQmlComponent component(&engine, testFileUrl("singletonTest12.qml"));
    VERIFY_ERRORS("singletonTest12.error.txt");
}

// Qmldir defines a type as a singleton, but the qml file does not have a pragma Singleton.
void tst_qqmllanguage::compositeSingletonQmldirNoPragmaError()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest13.qml"));
    VERIFY_ERRORS("singletonTest13.error.txt");
}

// Invalid singleton definition in the qmldir file results in an error
void tst_qqmllanguage::compositeSingletonQmlDirError()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest14.qml"));
    VERIFY_ERRORS("singletonTest14.error.txt");
}

// Load a remote composite singleton type via qmldir that defines the type as a singleton
void tst_qqmllanguage::compositeSingletonRemote()
{
    ThreadedTestHTTPServer server(dataDirectory());

    QFile f(testFile("singletonTest15.qml"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray contents = f.readAll();
    f.close();

    contents.replace(QByteArrayLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString().toUtf8());

    QQmlComponent component(&engine);
    component.setData(contents, testFileUrl("singletonTest15.qml"));

    while (component.isLoading())
        QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 50);

    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 525, "value2", 355);
}

// Reads values from a Singleton accessed through selectors.
void tst_qqmllanguage::compositeSingletonSelectors()
{
    QQmlEngine e2;
    QQmlFileSelector qmlSelector(&e2);
    qmlSelector.setExtraSelectors(QStringList() << "basicSelector");
    QQmlComponent component(&e2, testFileUrl("singletonTest1.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 625, "value2", 455);
}

// Reads values from a Singleton that was registered through the C++ API:
// qmlRegisterSingletonType.
void tst_qqmllanguage::compositeSingletonRegistered()
{
    QQmlComponent component(&engine, testFileUrl("singletonTest17.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    verifyCompositeSingletonPropertyValues(o.data(), "value1", 925, "value2", 755);
}

void tst_qqmllanguage::compositeSingletonCircular()
{
    QQmlComponent component(&engine, testFileUrl("circularSingleton.qml"));
    VERIFY_ERRORS(0);

    QQmlTestMessageHandler messageHandler;

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    // ensure we aren't hitting the recursion warning
    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));

    QCOMPARE(o->property("value").toInt(), 2);
}

void tst_qqmllanguage::compositeSingletonRequiredProperties()
{
    QFETCH(QString, warning);
    QFETCH(QString, singletonName);
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());
    {
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, qPrintable(warning));
        std::unique_ptr<QObject> singleton {engine.singletonInstance<QObject *>(
                        "SingletonWithRequiredProperties",
                        singletonName
        )};
        QVERIFY(!singleton);
    }
}

void tst_qqmllanguage::compositeSingletonRequiredProperties_data()
{
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("singletonName");

    QString warning1 = testFileUrl("SingletonWithRequiredProperties/SingletonWithRequired1.qml").toString()
            + ":5:5: Required property i was not initialized";
    QString warning2 = testFileUrl("SingletonWithRequiredProperties/SingletonWithRequired2.qml").toString()
            + ":6:9: Required property i was not initialized";

    QTest::addRow("toplevelRequired") << warning1 << "SingletonWithRequired1";
    QTest::addRow("subObjectRequired") << warning2 << "SingletonWithRequired2";
}

void tst_qqmllanguage::singletonsHaveContextAndEngine()
{
    QObject *qmlSingleton = nullptr;
    getSingletonInstance(engine, "singletonTest18.qml", "qmlSingleton", &qmlSingleton);
    QVERIFY(qmlContext(qmlSingleton));
    QCOMPARE(qmlEngine(qmlSingleton), &engine);

    QObject *jsSingleton = nullptr;
    getSingletonInstance(engine, "singletonTest18.qml", "jsSingleton", &jsSingleton);
    QVERIFY(qmlContext(jsSingleton));
    QCOMPARE(qmlEngine(jsSingleton), &engine);

    QObject *cppSingleton = nullptr;
    getSingletonInstance(engine, "singletonTest18.qml", "cppSingleton", &cppSingleton);
    QVERIFY(qmlContext(cppSingleton));
    QCOMPARE(qmlEngine(cppSingleton), &engine);
}

void tst_qqmllanguage::customParserBindingScopes()
{
    QQmlComponent component(&engine, testFileUrl("customParserBindingScopes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QPointer<QObject> child = qvariant_cast<QObject*>(o->property("child"));
    QVERIFY(!child.isNull());
    QCOMPARE(child->property("testProperty").toInt(), 42);
}

void tst_qqmllanguage::customParserEvaluateEnum()
{
    QQmlComponent component(&engine, testFileUrl("customParserEvaluateEnum.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::customParserProperties()
{
    QQmlComponent component(&engine, testFileUrl("customParserProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    SimpleObjectWithCustomParser *testObject = qobject_cast<SimpleObjectWithCustomParser*>(o.data());
    QVERIFY(testObject);
    QCOMPARE(testObject->customBindingsCount(), 0);
    QCOMPARE(testObject->intProperty(), 42);
    QCOMPARE(testObject->property("qmlString").toString(), QStringLiteral("Hello"));
    QVERIFY(!testObject->property("someObject").isNull());
}

void tst_qqmllanguage::customParserWithExtendedObject()
{
    QQmlComponent component(&engine, testFileUrl("customExtendedParserProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    SimpleObjectWithCustomParser *testObject = qobject_cast<SimpleObjectWithCustomParser*>(o.data());
    QVERIFY(testObject);
    QCOMPARE(testObject->customBindingsCount(), 0);
    QCOMPARE(testObject->intProperty(), 42);
    QCOMPARE(testObject->property("qmlString").toString(), QStringLiteral("Hello"));
    QVERIFY(!testObject->property("someObject").isNull());

    QVariant returnValue;
    QVERIFY(QMetaObject::invokeMethod(o.data(), "getExtendedProperty", Q_RETURN_ARG(QVariant, returnValue)));
    QCOMPARE(returnValue.toInt(), 1584);
}

void tst_qqmllanguage::nestedCustomParsers()
{
    QQmlComponent component(&engine, testFileUrl("nestedCustomParsers.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    SimpleObjectWithCustomParser *testObject = qobject_cast<SimpleObjectWithCustomParser*>(o.data());
    QVERIFY(testObject);
    QCOMPARE(testObject->customBindingsCount(), 1);
    SimpleObjectWithCustomParser *nestedObject = qobject_cast<SimpleObjectWithCustomParser*>(testObject->property("nested").value<QObject*>());
    QVERIFY(nestedObject);
    QCOMPARE(nestedObject->customBindingsCount(), 1);
}

void tst_qqmllanguage::preservePropertyCacheOnGroupObjects()
{
    QQmlComponent component(&engine, testFileUrl("preservePropertyCacheOnGroupObjects.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QObject *subObject = qvariant_cast<QObject*>(o->property("subObject"));
    QVERIFY(subObject);
    QCOMPARE(subObject->property("value").toInt(), 42);

    QQmlData *ddata = QQmlData::get(subObject);
    QVERIFY(ddata);
    const QQmlPropertyCache *subCache = ddata->propertyCache.data();
    QVERIFY(subCache);
    const QQmlPropertyData *pd = subCache->property(QStringLiteral("newProperty"), /*object*/nullptr, /*context*/nullptr);
    QVERIFY(pd);
    QCOMPARE(pd->propType(), QMetaType::fromType<int>());
}

void tst_qqmllanguage::propertyCacheInSync()
{
    QQmlComponent component(&engine, testFileUrl("propertyCacheInSync.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QObject *anchors = qvariant_cast<QObject*>(o->property("anchors"));
    QVERIFY(anchors);
    QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(anchors);
    QVERIFY(vmemo);
    QQmlPropertyCache::ConstPtr vmemoCache = vmemo->propertyCache();
    QVERIFY(vmemoCache);
    QQmlData *ddata = QQmlData::get(anchors);
    QVERIFY(ddata);
    QVERIFY(ddata->propertyCache);
    // Those always have to be in sync and correct.
    QCOMPARE(ddata->propertyCache, vmemoCache);
    QCOMPARE(anchors->property("margins").toInt(), 50);
}

void tst_qqmllanguage::rootObjectInCreationNotForSubObjects()
{
    QQmlComponent component(&engine, testFileUrl("rootObjectInCreationNotForSubObjects.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    // QQmlComponent should have set this back to false anyway
    QQmlData *ddata = QQmlData::get(o.data());
    QVERIFY(!ddata->rootObjectInCreation);

    QObject *subObject = qvariant_cast<QObject*>(o->property("subObject"));
    QVERIFY(!subObject);

    qmlExecuteDeferred(o.data());

    subObject = qvariant_cast<QObject*>(o->property("subObject"));
    QVERIFY(subObject);

    ddata = QQmlData::get(subObject);
    // This should never have been set in the first place as there is no
    // QQmlComponent to set it back to false.
    QVERIFY(!ddata->rootObjectInCreation);
}

// QTBUG-63036
void tst_qqmllanguage::lazyDeferredSubObject()
{
    QQmlComponent component(&engine, testFileUrl("lazyDeferredSubObject.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObject *subObject = qvariant_cast<QObject *>(object->property("subObject"));
    QVERIFY(subObject);

    QCOMPARE(object->objectName(), QStringLiteral("custom"));
    QCOMPARE(subObject->objectName(), QStringLiteral("custom"));
}

// QTBUG-63200
void tst_qqmllanguage::deferredProperties()
{
    QQmlComponent component(&engine, testFileUrl("deferredProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObject *innerObj = object->findChild<QObject *>(QStringLiteral("innerobj"));
    QVERIFY(!innerObj);

    QObject *outerObj = object->findChild<QObject *>(QStringLiteral("outerobj"));
    QVERIFY(!outerObj);

    QObject *groupProperty = object->property("groupProperty").value<QObject *>();
    QVERIFY(!groupProperty);

    QQmlListProperty<QObject> listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 0);

    QQmlData *qmlData = QQmlData::get(object.data());
    QVERIFY(qmlData);

    QCOMPARE(qmlData->deferredData.size(), 2); // MyDeferredListProperty.qml + deferredListProperty.qml
    QCOMPARE(qmlData->deferredData.first()->bindings.size(), 3); // "innerobj", "innerlist1", "innerlist2"
    QCOMPARE(qmlData->deferredData.last()->bindings.size(), 3); // "outerobj", "outerlist1", "outerlist2"

    qmlExecuteDeferred(object.data());

    QCOMPARE(qmlData->deferredData.size(), 0);

    innerObj = object->findChild<QObject *>(QStringLiteral("innerobj")); // MyDeferredListProperty.qml
    QVERIFY(innerObj);
    QCOMPARE(innerObj->property("wasCompleted"), QVariant(true));

    outerObj = object->findChild<QObject *>(QStringLiteral("outerobj")); // deferredListProperty.qml
    QVERIFY(outerObj);
    QCOMPARE(outerObj->property("wasCompleted"), QVariant(true));

    groupProperty = object->property("groupProperty").value<QObject *>();
    QCOMPARE(groupProperty, outerObj);

    listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 4);

    QCOMPARE(listProperty.at(&listProperty, 0)->objectName(), QStringLiteral("innerlist1")); // MyDeferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 0)->property("wasCompleted"), QVariant(true));
    QCOMPARE(listProperty.at(&listProperty, 1)->objectName(), QStringLiteral("innerlist2")); // MyDeferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 1)->property("wasCompleted"), QVariant(true));

    QCOMPARE(listProperty.at(&listProperty, 2)->objectName(), QStringLiteral("outerlist1")); // deferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 2)->property("wasCompleted"), QVariant(true));
    QCOMPARE(listProperty.at(&listProperty, 3)->objectName(), QStringLiteral("outerlist2")); // deferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 3)->property("wasCompleted"), QVariant(true));
}

static void beginDeferredOnce(QQmlEnginePrivate *enginePriv,
                              const QQmlProperty &property, QQmlComponentPrivate::DeferredState *deferredState)
{
    QObject *object = property.object();
    QQmlData *ddata = QQmlData::get(object);
    Q_ASSERT(!ddata->deferredData.isEmpty());

    int propertyIndex = property.index();

    for (auto dit = ddata->deferredData.rbegin(); dit != ddata->deferredData.rend(); ++dit) {
        QQmlData::DeferredData *deferData = *dit;

        auto range = deferData->bindings.equal_range(propertyIndex);
        if (range.first == deferData->bindings.end())
            continue;

        QQmlComponentPrivate::ConstructionState state;
        state.setCompletePending(true);

        state.initCreator(deferData->context->parent(), deferData->compilationUnit,
                                 QQmlRefPointer<QQmlContextData>());

        enginePriv->inProgressCreations++;

        std::deque<const QV4::CompiledData::Binding *> reversedBindings;
        std::copy(range.first, range.second, std::front_inserter(reversedBindings));
        state.creator()->beginPopulateDeferred(deferData->context);
        for (const QV4::CompiledData::Binding *binding: reversedBindings)
            state.creator()->populateDeferredBinding(property, deferData->deferredIdx, binding);
        state.creator()->finalizePopulateDeferred();
        state.appendErrors(state.creator()->errors);

        deferredState->push_back(std::move(state));

        // Cleanup any remaining deferred bindings for this property, also in inner contexts,
        // to avoid executing them later and overriding the property that was just populated.
        while (dit != ddata->deferredData.rend()) {
            (*dit)->bindings.remove(propertyIndex);
            ++dit;
        }
        break;
    }
}

static void testExecuteDeferredOnce(const QQmlProperty &property)
{
    QObject *object = property.object();
    QQmlData *data = QQmlData::get(object);
    if (data && !data->deferredData.isEmpty() && !data->wasDeleted(object)) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());

        QQmlComponentPrivate::DeferredState state;
        beginDeferredOnce(ep, property, &state);

        // Release deferred data for those compilation units that no longer have deferred bindings
        data->releaseDeferredData();

        QQmlComponentPrivate::completeDeferred(ep, &state);
    }
}

void tst_qqmllanguage::executeDeferredPropertiesOnce()
{
    QQmlComponent component(&engine, testFileUrl("deferredProperties.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObjectList innerObjsAtCreation = object->findChildren<QObject *>(QStringLiteral("innerobj"));
    QVERIFY(innerObjsAtCreation.isEmpty());

    QObjectList outerObjsAtCreation = object->findChildren<QObject *>(QStringLiteral("outerobj"));
    QVERIFY(outerObjsAtCreation.isEmpty());

    QObject *groupProperty = object->property("groupProperty").value<QObject *>();
    QVERIFY(!groupProperty);

    QQmlListProperty<QObject> listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 0);

    QQmlData *qmlData = QQmlData::get(object.data());
    QVERIFY(qmlData);

    QCOMPARE(qmlData->deferredData.size(), 2); // MyDeferredListProperty.qml + deferredListProperty.qml
    QCOMPARE(qmlData->deferredData.first()->bindings.size(), 3); // "innerobj", "innerlist1", "innerlist2"
    QCOMPARE(qmlData->deferredData.last()->bindings.size(), 3); // "outerobj", "outerlist1", "outerlist2"

    // first execution creates the outer object
    testExecuteDeferredOnce(QQmlProperty(object.data(), "groupProperty"));

    QCOMPARE(qmlData->deferredData.size(), 2); // MyDeferredListProperty.qml + deferredListProperty.qml
    QCOMPARE(qmlData->deferredData.first()->bindings.size(), 2); // "innerlist1", "innerlist2"
    QCOMPARE(qmlData->deferredData.last()->bindings.size(), 2); // "outerlist1", "outerlist2"

    QObjectList innerObjsAfterFirstExecute = object->findChildren<QObject *>(QStringLiteral("innerobj")); // MyDeferredListProperty.qml
    QVERIFY(innerObjsAfterFirstExecute.isEmpty());

    QObjectList outerObjsAfterFirstExecute = object->findChildren<QObject *>(QStringLiteral("outerobj")); // deferredListProperty.qml
    QCOMPARE(outerObjsAfterFirstExecute.size(), 1);
    QCOMPARE(outerObjsAfterFirstExecute.first()->property("wasCompleted"), QVariant(true));

    groupProperty = object->property("groupProperty").value<QObject *>();
    QCOMPARE(groupProperty, outerObjsAfterFirstExecute.first());

    listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 0);

    // re-execution does nothing (to avoid overriding the property)
    testExecuteDeferredOnce(QQmlProperty(object.data(), "groupProperty"));

    QCOMPARE(qmlData->deferredData.size(), 2); // MyDeferredListProperty.qml + deferredListProperty.qml
    QCOMPARE(qmlData->deferredData.first()->bindings.size(), 2); // "innerlist1", "innerlist2"
    QCOMPARE(qmlData->deferredData.last()->bindings.size(), 2); // "outerlist1", "outerlist2"

    QObjectList innerObjsAfterSecondExecute = object->findChildren<QObject *>(QStringLiteral("innerobj")); // MyDeferredListProperty.qml
    QVERIFY(innerObjsAfterSecondExecute.isEmpty());

    QObjectList outerObjsAfterSecondExecute = object->findChildren<QObject *>(QStringLiteral("outerobj")); // deferredListProperty.qml
    QCOMPARE(outerObjsAfterFirstExecute, outerObjsAfterSecondExecute);

    groupProperty = object->property("groupProperty").value<QObject *>();
    QCOMPARE(groupProperty, outerObjsAfterFirstExecute.first());

    listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 0);

    // execution of a list property should execute all outer list bindings
    testExecuteDeferredOnce(QQmlProperty(object.data(), "listProperty"));

    QCOMPARE(qmlData->deferredData.size(), 0);

    listProperty = object->property("listProperty").value<QQmlListProperty<QObject>>();
    QCOMPARE(listProperty.count(&listProperty), 2);

    QCOMPARE(listProperty.at(&listProperty, 0)->objectName(), QStringLiteral("outerlist1")); // deferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 0)->property("wasCompleted"), QVariant(true));
    QCOMPARE(listProperty.at(&listProperty, 1)->objectName(), QStringLiteral("outerlist2")); // deferredListProperty.qml
    QCOMPARE(listProperty.at(&listProperty, 1)->property("wasCompleted"), QVariant(true));
}

class GroupType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ getFoo WRITE setFoo BINDABLE bindableFoo)
    Q_PROPERTY(int bar READ getBar WRITE setBar BINDABLE bindableBar)
    Q_CLASSINFO("DeferredPropertyNames", "bar")

    QProperty<int> m_foo { 0 };
    QProperty<int> m_bar { 0 };

public:
    int getFoo() const { return m_foo; }
    void setFoo(int v) { m_foo = v; }
    QBindable<int> bindableFoo() const { return QBindable<int>(&m_foo); }

    int getBar() const { return m_bar; }
    void setBar(int v) { m_bar = v; }
    QBindable<int> bindableBar() const { return QBindable<int>(&m_bar); }
};

class ExtraDeferredProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GroupType *group READ getGroup)
    Q_CLASSINFO("DeferredPropertyNames", "group,MyQmlObject")

    GroupType m_group;

public:
    ExtraDeferredProperties(QObject *parent = nullptr) : QObject(parent) { }

    GroupType *getGroup() { return &m_group; }
};

void tst_qqmllanguage::deferredProperties_extra()
{
    // Note: because ExtraDeferredProperties defers only a `group` property, the
    // deferral does not actually work.
    qmlRegisterType<GroupType>("deferred.properties.extra", 1, 0, "GroupType");
    qmlRegisterType<ExtraDeferredProperties>("deferred.properties.extra", 1, 0,
                                             "ExtraDeferredProperties");
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import Test 1.0
        import deferred.properties.extra 1.0

        ExtraDeferredProperties {
            group.foo: 4
            group.bar: 4
            MyQmlObject.value: 1
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<ExtraDeferredProperties> object(
            qobject_cast<ExtraDeferredProperties *>(component.create()));
    QVERIFY(object);

    QCOMPARE(object->getGroup()->getFoo(), 4); // not deferred (as group itself is not deferred)
    QCOMPARE(object->getGroup()->getBar(), 0); // deferred, as per group's own deferred names
    // but attached property is deferred:
    QVERIFY(!qmlAttachedPropertiesObject<MyQmlObject>(object.get(), false));

    qmlExecuteDeferred(object.get());

    auto attached = qmlAttachedPropertiesObject<MyQmlObject>(object.get(), false);
    QVERIFY(attached);
    QCOMPARE(attached->property("value").toInt(), 1);
}

void tst_qqmllanguage::noChildEvents()
{
    QQmlComponent component(&engine);
    component.setData("import QtQml 2.0; import Test 1.0; MyQmlObject { property QtObject child: QtObject {} }", QUrl());
    VERIFY_ERRORS(0);
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject*>(component.create()));
    QVERIFY(object != nullptr);
    QCOMPARE(object->childAddedEventCount(), 0);
}

void tst_qqmllanguage::earlyIdObjectAccess()
{
    QQmlComponent component(&engine, testFileUrl("earlyIdObjectAccess.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QVERIFY(o->property("success").toBool());
}

void tst_qqmllanguage::deleteSingletons()
{
    QPointer<QObject> singleton;
    {
        QQmlEngine tmpEngine;
        QQmlComponent component(&tmpEngine, testFileUrl("singletonTest5.qml"));
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o != nullptr);
        QObject *s1 = nullptr;
        getSingletonInstance(o.data(), "singletonInstance", &s1);
        QVERIFY(s1 != nullptr);
        singleton = s1;
        QVERIFY(singleton.data() != nullptr);
    }
    QVERIFY(singleton.data() == nullptr);
}

void tst_qqmllanguage::arrayBuffer_data()
{
    QTest::addColumn<QString>("file");
    QTest::newRow("arraybuffer_property_get") << "arraybuffer_property_get.qml";
    QTest::newRow("arraybuffer_property_set") << "arraybuffer_property_set.qml";
    QTest::newRow("arraybuffer_signal_arg") << "arraybuffer_signal_arg.qml";
    QTest::newRow("arraybuffer_method_arg") << "arraybuffer_method_arg.qml";
    QTest::newRow("arraybuffer_method_return") << "arraybuffer_method_return.qml";
    QTest::newRow("arraybuffer_method_overload") << "arraybuffer_method_overload.qml";
}

void tst_qqmllanguage::arrayBuffer()
{
    QFETCH(QString, file);
    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("ok").toBool(), true);
}

void tst_qqmllanguage::defaultListProperty()
{
    QQmlComponent component(&engine, testFileUrl("defaultListProperty.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
}

void tst_qqmllanguage::namespacedPropertyTypes()
{
    QQmlComponent component(&engine, testFileUrl("namespacedPropertyTypes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::qmlTypeCanBeResolvedByName_data()
{
    QTest::addColumn<QUrl>("componentUrl");

    // Built-in C++ types
    QTest::newRow("C++ - Anonymous") << testFileUrl("quickTypeByName_anon.qml");
    QTest::newRow("C++ - Named") << testFileUrl("quickTypeByName_named.qml");

    // Composite types with a qmldir
    QTest::newRow("QML - Anonymous - qmldir") << testFileUrl("compositeTypeByName_anon_qmldir.qml");
    QTest::newRow("QML - Named - qmldir") << testFileUrl("compositeTypeByName_named_qmldir.qml");
}

void tst_qqmllanguage::qmlTypeCanBeResolvedByName()
{
    QFETCH(QUrl, componentUrl);

    QQmlEngine engine;
    QQmlComponent component(&engine, componentUrl);
    VERIFY_ERRORS(0);
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "[object Object]"); // a bit crude, but it will do

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

// Tests for the QML-only extensions of instanceof. Tests for the regular JS
// instanceof belong in tst_qqmlecmascript!
void tst_qqmllanguage::instanceof_data()
{
    QTest::addColumn<QUrl>("documentToTestIn");
    QTest::addColumn<QVariant>("expectedValue");

    // so the way this works is that the name of the test tag defines the test
    // to run.
    //
    // the expectedValue is either a boolean true or false for whether the two
    // operands are indeed an instanceof each other, or a string for the
    // expected error message.

    // assert that value types don't convert to QObject
    QTest::newRow("1 instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("true instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("\"foobar\" instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);

    // assert that Managed don't either
    QTest::newRow("new String(\"foobar\") instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("new Object() instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("new Date() instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);

    // test that simple QtQml comparisons work
    QTest::newRow("qtobjectInstance instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(true);
    QTest::newRow("qtobjectInstance instanceof Timer")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("timerInstance instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(true);
    QTest::newRow("timerInstance instanceof Timer")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(true);
    QTest::newRow("connectionsInstance instanceof QtObject")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(true);
    QTest::newRow("connectionsInstance instanceof Timer")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(false);
    QTest::newRow("connectionsInstance instanceof Connections")
        << testFileUrl("instanceof_qtqml.qml")
        << QVariant(true);

    // make sure they still work when imported with a qualifier
    QTest::newRow("qtobjectInstance instanceof QmlImport.QtObject")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(true);
    QTest::newRow("qtobjectInstance instanceof QmlImport.Timer")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(false);
    QTest::newRow("timerInstance instanceof QmlImport.QtObject")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(true);
    QTest::newRow("timerInstance instanceof QmlImport.Timer")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(true);
    QTest::newRow("connectionsInstance instanceof QmlImport.QtObject")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(true);
    QTest::newRow("connectionsInstance instanceof QmlImport.Timer")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(false);
    QTest::newRow("connectionsInstance instanceof QmlImport.Connections")
        << testFileUrl("instanceof_qtqml_qualified.qml")
        << QVariant(true);

    // test that Quick C++ types work ok
    QTest::newRow("itemInstance instanceof QtObject")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(true);
    QTest::newRow("itemInstance instanceof Timer")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(false);
    QTest::newRow("itemInstance instanceof Rectangle")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(false);
    QTest::newRow("rectangleInstance instanceof Item")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(true);
    QTest::newRow("rectangleInstance instanceof Rectangle")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(true);
    QTest::newRow("rectangleInstance instanceof MouseArea")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(false);
    QTest::newRow("mouseAreaInstance instanceof Item")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(true);
    QTest::newRow("mouseAreaInstance instanceof Rectangle")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(false);
    QTest::newRow("mouseAreaInstance instanceof MouseArea")
        << testFileUrl("instanceof_qtquick.qml")
        << QVariant(true);

    // test that unqualified quick composite types work ok
    QTest::newRow("rectangleInstance instanceof CustomRectangle")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(false);
    QTest::newRow("customRectangleInstance instanceof Rectangle")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(true);
    QTest::newRow("customRectangleInstance instanceof Item")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(true);
    QTest::newRow("customRectangleWithPropInstance instanceof CustomRectangleWithProp")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(true);
    QTest::newRow("customRectangleWithPropInstance instanceof CustomRectangle")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(false); // ### XXX: QTBUG-58477
    QTest::newRow("customRectangleWithPropInstance instanceof Rectangle")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(true);
    QTest::newRow("customRectangleInstance instanceof MouseArea")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(false);
    QTest::newRow("customMouseAreaInstance instanceof MouseArea")
        << testFileUrl("instanceof_qtquick_composite.qml")
        << QVariant(true);

    // test that they still work when qualified
    QTest::newRow("rectangleInstance instanceof CustomImport.CustomRectangle")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(false);
    QTest::newRow("customRectangleInstance instanceof QuickImport.Rectangle")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(true);
    QTest::newRow("customRectangleInstance instanceof QuickImport.Item")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(true);
    QTest::newRow("customRectangleWithPropInstance instanceof CustomImport.CustomRectangleWithProp")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(true);
    QTest::newRow("customRectangleWithPropInstance instanceof CustomImport.CustomRectangle")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(false); // ### XXX: QTBUG-58477
    QTest::newRow("customRectangleWithPropInstance instanceof QuickImport.Rectangle")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(true);
    QTest::newRow("customRectangleInstance instanceof QuickImport.MouseArea")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(false);
    QTest::newRow("customMouseAreaInstance instanceof QuickImport.MouseArea")
        << testFileUrl("instanceof_qtquick_composite_qualified.qml")
        << QVariant(true);
}

void tst_qqmllanguage::instanceof()
{
    QFETCH(QUrl, documentToTestIn);
    QFETCH(QVariant, expectedValue);

    QQmlEngine engine;
    QQmlComponent component(&engine, documentToTestIn);
    VERIFY_ERRORS(0);

    QScopedPointer<QObject> o(component.create());
    QVERIFY(o != nullptr);

    QQmlExpression expr(engine.contextForObject(o.data()), nullptr, QString::fromLatin1(QTest::currentDataTag()));
    QVariant ret = expr.evaluate();

    if (expectedValue.typeId() == QMetaType::Bool) {
        // no error expected
        QVERIFY2(!expr.hasError(), qPrintable(expr.error().description()));
        bool returnValue = ret.toBool();

        if (QTest::currentDataTag() == QLatin1String("customRectangleWithPropInstance instanceof CustomRectangle") ||
            QTest::currentDataTag() == QLatin1String("customRectangleWithPropInstance instanceof CustomImport.CustomRectangle"))
        QCOMPARE(returnValue, expectedValue.toBool());
    } else {
        QVERIFY(expr.hasError());
        QCOMPARE(expr.error().description(), expectedValue.toString());
    }
}

void tst_qqmllanguage::concurrentLoadQmlDir()
{
    ThreadedTestHTTPServer server(dataDirectory());
    QString serverdir = server.urlString("/lib/");
    engine.setImportPathList(QStringList(defaultImportPathList) << serverdir);

    QQmlComponent component(&engine, testFileUrl("concurrentLoad_main.qml"));
    QTRY_VERIFY(component.isReady());
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    engine.setImportPathList(defaultImportPathList);
}

// Test that deleting an object and then accessing it doesn't crash.
// QTBUG-44153
class ObjectCreator : public QObject
{
    Q_OBJECT
public slots:
    QObject *create() { return (new ObjectCreator); }
    void del() { delete this; }
};

void tst_qqmllanguage::accessDeletedObject()
{
    QQmlEngine engine;

    QScopedPointer<ObjectCreator> creator(new ObjectCreator);
    engine.rootContext()->setContextProperty("objectCreator", creator.get());
    QQmlComponent component(&engine, testFileUrl("accessDeletedObject.qml"));
    VERIFY_ERRORS(0);

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::lowercaseTypeNames()
{
    QCOMPARE(qmlRegisterType<QObject>("Test", 1, 0, "lowerCaseTypeName"), -1);
    QCOMPARE(qmlRegisterSingletonType<QObject>("Test", 1, 0, "lowerCaseTypeName", nullptr), -1);
}

void tst_qqmllanguage::thisInQmlScope()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("thisInQmlScope.qml"));
    QTRY_VERIFY(component.isReady());
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("x"), QVariant(42));
    QCOMPARE(o->property("y"), QVariant(42));
    QCOMPARE(o->property("a"), QVariant(42));
    QCOMPARE(o->property("b"), QVariant(42));
}

void tst_qqmllanguage::valueTypeGroupPropertiesInBehavior()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("groupPropertyInPropertyValueSource.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QObject *animation = qmlContext(o.data())->contextProperty("animation").value<QObject*>();
    QVERIFY(animation);

    QCOMPARE(animation->property("easing").value<QEasingCurve>().type(), QEasingCurve::InOutQuad);
}

void tst_qqmllanguage::retrieveQmlTypeId()
{
    // Register in reverse order to provoke wrong minor version matching.
    int id2 = qmlRegisterType<QObject>("Test", 2, 3, "SomeTestType");
    int id1 = qmlRegisterType<QObject>("Test", 2, 1, "SomeTestType");
    QCOMPARE(qmlTypeId("Test", 2, 1, "SomeTestType"), id1);
    QCOMPARE(qmlTypeId("Test", 2, 2, "SomeTestType"), id1);
    QCOMPARE(qmlTypeId("Test", 2, 3, "SomeTestType"), id2);

    // Error cases
    QCOMPARE(qmlTypeId("Test", 2, 0, "SomeTestType"), -1);
    QCOMPARE(qmlTypeId("Test", 2, 3, "DoesNotExist"), -1);
    QCOMPARE(qmlTypeId("DoesNotExist", 2, 3, "SomeTestType"), -1);

    // Must also work for other types (defined in testtpes.cpp)
    QVERIFY(qmlTypeId("Test", 1, 0, "MyExtendedUncreateableBaseClass") >= 0);
    QVERIFY(qmlTypeId("Test", 1, 0, "MyUncreateableBaseClass") >= 0);
    QVERIFY(qmlTypeId("Test", 1, 0, "MyTypeObjectSingleton") >= 0);

    // Must also work for declaratively registered types whose module wasn't imported  so far
    QVERIFY(qmlTypeId("testhelper", 1, 0, "PurelyDeclarativeSingleton") >= 0);
}

void tst_qqmllanguage::polymorphicFunctionLookup()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("polymorphicFunctionLookup.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QVERIFY(o->property("ok").toBool());
}

void tst_qqmllanguage::anchorsToParentInPropertyChanges()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("anchorsToParentInPropertyChagnes.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QTRY_COMPARE(o->property("edgeWidth").toInt(), 200);
}

void tst_qqmllanguage::typeWrapperToVariant()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("typeWrapperToVariant.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QObject *connections = qvariant_cast<QObject *>(o->property("connections"));
    QVERIFY(connections);
    QObject *target = qvariant_cast<QObject *>(connections->property("target"));
    QVERIFY(target);
}

void tst_qqmllanguage::extendedForeignTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("foreignExtended.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QObject *extended = o->property("extended").value<QObject *>();
    QVERIFY(extended);
    QSignalSpy extensionChangedSpy(extended, SIGNAL(extensionChanged()));
    QSignalSpy extensionChangedWithValueSpy(extended, SIGNAL(extensionChangedWithValue(int)));
    QVERIFY(extensionChangedWithValueSpy.isValid());

    QCOMPARE(o->property("extendedBase").toInt(), 43);
    QCOMPARE(o->property("extendedExtension").toInt(), 42);
    QCOMPARE(o->property("foreignExtendedExtension").toInt(), 42);

    QCOMPARE(extensionChangedSpy.size(), 0);
    extended->setProperty("extension", 44);
    QCOMPARE(extensionChangedSpy.size(), 1);
    QCOMPARE(extensionChangedWithValueSpy.size(), 1);
    QCOMPARE(o->property("extendedChangeCount").toInt(), 1);
    QCOMPARE(o->property("extendedExtension").toInt(), 44);

    QCOMPARE(o->property("foreignObjectName").toString(), QLatin1String("foreign"));
    QCOMPARE(o->property("foreignExtendedObjectName").toString(), QLatin1String("foreignExtended"));
    QCOMPARE(o->property("extendedInvokable").toInt(), 123);
    QCOMPARE(o->property("extendedSlot").toInt(), 456);

    QObject *extension = qmlExtendedObject(extended);
    QVERIFY(extension != nullptr);
    QVERIFY(qobject_cast<Extension *>(extension) != nullptr);
}

void tst_qqmllanguage::foreignTypeSingletons() {
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("foreignSingleton.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("number").toInt(), 42);
}

void tst_qqmllanguage::selfReference()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("SelfReference.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQmlComponentPrivate *componentPrivate = QQmlComponentPrivate::get(&component);
    auto compilationUnit = componentPrivate->compilationUnit;
    QVERIFY(compilationUnit);

    const QMetaObject *metaObject = o->metaObject();
    QMetaProperty selfProperty = metaObject->property(metaObject->indexOfProperty("self"));
    QCOMPARE(selfProperty.metaType().id(), compilationUnit->qmlType.typeId().id());

    QByteArray typeName = selfProperty.typeName();
    QVERIFY(typeName.endsWith('*'));
    typeName = typeName.chopped(1);
    QCOMPARE(typeName, metaObject->className());

    QMetaMethod selfFunction = metaObject->method(metaObject->indexOfMethod("returnSelf()"));
    QVERIFY(selfFunction.isValid());
    QCOMPARE(selfFunction.returnType(), compilationUnit->qmlType.typeId().id());

    QMetaMethod selfSignal;

    for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        if (method.isValid() && method.name().startsWith("blah")) {
            selfSignal = method;
            break;
        }
    }

    QVERIFY(selfSignal.isValid());
    QCOMPARE(selfSignal.parameterCount(), 1);
    QCOMPARE(selfSignal.parameterType(0), compilationUnit->qmlType.typeId().id());
}

void tst_qqmllanguage::selfReferencingSingleton()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());

    QPointer<QObject> singletonPointer;
    {
        QQmlComponent component(&engine);
        component.setData(QByteArray(R"(import QtQml 2.0
                                     import selfreferencingsingletonmodule 1.0
                                     QtObject {
                                         property SelfReferencingSingleton singletonPointer: SelfReferencingSingleton
                                     })"), QUrl());
        VERIFY_ERRORS(0);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(!o.isNull());
        singletonPointer = o->property("singletonPointer").value<QObject*>();
    }

    QVERIFY(!singletonPointer.isNull());
    QCOMPARE(singletonPointer->property("dummy").toInt(), 42);
}

void tst_qqmllanguage::listContainingDeletedObject()
{
    QQmlEngine engine;
    auto url = testFileUrl("listContainingDeleted.qml");
    const QString message = url.toString() + ":24: TypeError: Cannot read property 'enabled' of null";
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, message.toUtf8().data());
    QQmlComponent comp(&engine, url);
    QScopedPointer<QObject> root(comp.create());
    QVERIFY(root);

    auto cmp = root->property("a").value<QQmlComponent*>();
    std::unique_ptr<QObject> o { cmp->create() };

    QMetaObject::invokeMethod(root.get(), "doAssign", Q_ARG(QVariant, QVariant::fromValue(o.get())));
    o.reset();
    QMetaObject::invokeMethod(root.get(), "use");

}

void tst_qqmllanguage::overrideSingleton()
{
    auto check = [](const QString &name, const QByteArray &singletonElement) {
        const QByteArray testQml = "import Test 1.0\n"
                                   "import QtQml 2.0\n"
                                   "QtObject { objectName: " + singletonElement + ".objectName }";
        QQmlEngine engine;
        QQmlComponent component(&engine, nullptr);
        component.setData(testQml, QUrl("singleton.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> obj(component.create());
        QCOMPARE(obj->objectName(), name);
    };

    check("statically registered", "BareSingleton");

    BareSingleton singleton;
    singleton.setObjectName("dynamically registered");
    qmlRegisterSingletonInstance("Test", 1, 0, "BareSingleton", &singleton);

    check("dynamically registered", "BareSingleton");

    QTest::ignoreMessage(
                QtWarningMsg,
                "singleton.qml:3:12: TypeError: Cannot read property 'objectName' of undefined");
    check("", "UncreatableSingleton");

    qmlRegisterSingletonInstance("Test", 1, 0, "UncreatableSingleton",
                                 UncreatableSingleton::instance());
    check("uncreatable", "UncreatableSingleton");
}

class AttachedObject;
class InnerObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool revisionedProperty READ revisionedProperty WRITE setRevisionedProperty
               NOTIFY revisionedPropertyChanged REVISION 2)

public:
    InnerObject(QObject *parent = nullptr) : QObject(parent) {}

    bool revisionedProperty() const { return m_revisionedProperty; }
    void setRevisionedProperty(bool revisionedProperty)
    {
        if (revisionedProperty != m_revisionedProperty) {
            m_revisionedProperty = revisionedProperty;
            emit revisionedPropertyChanged();
        }
    }

    static AttachedObject *qmlAttachedProperties(QObject *object);

signals:
    Q_REVISION(2) void revisionedPropertyChanged();

private:
    bool m_revisionedProperty = false;
};

class AttachedObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(InnerObject *attached READ attached CONSTANT)

public:
    explicit AttachedObject(QObject *parent = nullptr) :
        QObject(parent),
        m_attached(new InnerObject(this))
    {}

    InnerObject *attached() const { return m_attached; }

private:
    InnerObject *m_attached;
};

class OuterObject : public QObject
{
    Q_OBJECT
public:
    explicit OuterObject(QObject *parent = nullptr) : QObject(parent) {}
};

AttachedObject *InnerObject::qmlAttachedProperties(QObject *object)
{
    return new AttachedObject(object);
}

QML_DECLARE_TYPE(InnerObject)
QML_DECLARE_TYPEINFO(InnerObject, QML_HAS_ATTACHED_PROPERTIES)

void tst_qqmllanguage::revisionedPropertyOfAttachedObjectProperty()
{
    qmlRegisterAnonymousType<AttachedObject>("foo", 2);
    qmlRegisterType<InnerObject>("foo", 2, 0, "InnerObject");
    qmlRegisterType<InnerObject, 2>("foo", 2, 2, "InnerObject");
    qmlRegisterType<OuterObject>("foo", 2, 2, "OuterObject");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import foo 2.2\n"
                      "OuterObject {\n"
                      "    InnerObject.attached.revisionedProperty: true\n"
                      "}", QUrl());

    QVERIFY(component.isReady());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

void tst_qqmllanguage::inlineComponent()
{
    QFETCH(QUrl, componentUrl);
    QFETCH(QColor, color);
    QFETCH(int, width);
    QFETCH(bool, checkProperties);
    QQmlEngine engine;
    QQmlComponent component(&engine, componentUrl);
    QScopedPointer<QObject> o(component.create());
    if (component.isError()) {
        qDebug() << component.errorString();
    }
    QVERIFY(!o.isNull());
    if (checkProperties) {
        auto icInstance = o->findChild<QObject *>("icInstance");
        QVERIFY(icInstance);
        QCOMPARE(icInstance->property("color").value<QColor>(),color);
        QCOMPARE(icInstance->property("width").value<qreal>(), width);
    }
}

void tst_qqmllanguage::inlineComponent_data()
{
    QTest::addColumn<QUrl>("componentUrl");
    QTest::addColumn<QColor>("color");
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("checkProperties");

    QTest::newRow("Usage from other component") << testFileUrl("inlineComponentUser1.qml") << QColorConstants::Blue << 24 << true;
    QTest::newRow("Reexport")                   << testFileUrl("inlineComponentUser2.qml") << QColorConstants::Svg::green << 24 << true;
    QTest::newRow("Usage in same component")    << testFileUrl("inlineComponentUser3.qml") << QColorConstants::Blue << 24 << true;

    QTest::newRow("Resolution happens at instantiation") << testFileUrl("inlineComponentUser4.qml") << QColorConstants::Blue << 24 << true;
    QTest::newRow("Non-toplevel IC is found") << testFileUrl("inlineComponentUser5.qml") << QColorConstants::Svg::red << 24 << true;

    QTest::newRow("Resolved in correct order") << testFileUrl("inlineComponentOrder.qml") << QColorConstants::Blue << 200 << true;

    QTest::newRow("ID resolves correctly") << testFileUrl("inlineComponentWithId.qml") << QColorConstants::Svg::red << 42 << true;
    QTest::newRow("Alias resolves correctly") << testFileUrl("inlineComponentWithAlias.qml") << QColorConstants::Svg::lime << 42 << true;

    QTest::newRow("Two inline components in same do not crash (QTBUG-86989)") << testFileUrl("twoInlineComponents.qml") << QColor() << 0 << false;
    QTest::newRow("Inline components used in same file (QTBUG-89173)") << testFileUrl("inlineComponentsSameFile.qml") << QColor() << 0 << false;
}

void tst_qqmllanguage::inlineComponentReferenceCycle_data()
{
    QTest::addColumn<QUrl>("componentUrl");

    QTest::newRow("Simple cycle") << testFileUrl("icSimpleCycle.qml");
    QTest::newRow("Via property") << testFileUrl("icCycleViaProperty.qml");
}

void tst_qqmllanguage::inlineComponentReferenceCycle()
{
    QFETCH(QUrl, componentUrl);
    QQmlEngine engine;
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlComponent: Component is not ready");
    QQmlComponent component(&engine, componentUrl);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o.isNull());
    QVERIFY(component.isError());
    QCOMPARE(component.errorString(), componentUrl.toString() + QLatin1String(":-1 Inline components form a cycle!\n"));
}

void tst_qqmllanguage::nestedInlineComponentNotAllowed()
{
    QQmlEngine engine;
    auto url = testFileUrl("nestedIC.qml");
    QQmlComponent component(&engine, url);
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlComponent: Component is not ready");
    QScopedPointer<QObject> o(component.create());
    QVERIFY(component.isError());
    QCOMPARE(component.errorString(), QLatin1String("%1:%2").arg(url.toString(), QLatin1String("5 Nested inline components are not supported\n")));
}

void tst_qqmllanguage::inlineComponentStaticTypeResolution()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("InlineComponentChild.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);
    QCOMPARE(o->property("i").toInt(), 42);
}

void tst_qqmllanguage::inlineComponentInSingleton()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("singletonICTest.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    auto untyped = o->property("singleton1");
    QVERIFY(untyped.isValid());
    auto singleton1 = untyped.value<QObject*>();
    QVERIFY(singleton1);
    QCOMPARE(singleton1->property("iProp").value<int>(), 42);
    QCOMPARE(singleton1->property("sProp").value<QString>(), QString::fromLatin1("Hello, world"));
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::nonExistingInlineComponent_data()
{
    QTest::addColumn<QUrl>("componentUrl");
    QTest::addColumn<QString>("errorMessage");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");

    QTest::newRow("Property type")  << testFileUrl("nonExistingICUser1.qml") << QString("Type InlineComponentProvider has no inline component type called NonExisting") << 4 << 5;
    QTest::newRow("Instantiation")  << testFileUrl("nonExistingICUser2.qml") << QString("Type InlineComponentProvider has no inline component type called NotExisting") << 4 << 5;
    QTest::newRow("Inheritance")    << testFileUrl("nonExistingICUser3.qml") << QString("Type InlineComponentProvider has no inline component type called NotExisting") << 3 << 1;
    QTest::newRow("From singleton") << testFileUrl("nonExistingICUser4.qml") << QString("Type MySingleton.SingletonTypeWithIC has no inline component type called NonExisting") << 5 << 5;

    QTest::newRow("Cannot access parent inline components from child")  << testFileUrl("nonExistingICUser5.qml") << QString("Type InlineComponentProviderChild has no inline component type called StyledRectangle") << 4 << 5;
}

void tst_qqmllanguage::nonExistingInlineComponent()
{
    QFETCH(QUrl, componentUrl);
    QFETCH(QString, errorMessage);
    QFETCH(int, line);
    QFETCH(int, column);
    QQmlEngine engine;
    QQmlComponent component(&engine, componentUrl);
    auto errors = component.errors();
    QCOMPARE(errors.size(), 1);
    const auto &error = errors.first();
    QCOMPARE(error.description(), errorMessage);
    QCOMPARE(error.line(), line);
    QCOMPARE(error.column(), column);
}

void tst_qqmllanguage::inlineComponentFoundBeforeOtherImports()
{
    QQmlEngine engine;
    QUrl url = testFileUrl("inlineComponentFoundBeforeOtherImports.qml");
    QQmlComponent component(&engine, url);

    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "Created");
    QScopedPointer<QObject> root {component.create()};
}

void tst_qqmllanguage::inlineComponentDuplicateNameError()
{
    QQmlEngine engine;
    QUrl url = testFileUrl("inlineComponentDuplicateName.qml");
    QQmlComponent component(&engine, url);

    QString message = QLatin1String("%1:5 Inline component names must be unique per file\n").arg(url.toString());
    QScopedPointer<QObject> root {component.create()};
    QVERIFY(root.isNull());
    QVERIFY(component.isError());
    QCOMPARE(component.errorString(), message);
}

void tst_qqmllanguage::inlineComponentWithAliasInstantiatedWithNewProperties()
{
    // this tests that metaobjects are resolved in the correct order
    // so that inline components are fully resolved before they are used
    // in their parent component

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("inlineComponentWithAliasInstantiated.qml"));
    QScopedPointer<QObject> root {component.create()};
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("result").toString(), "Bar");
}

void tst_qqmllanguage::inlineComponentWithImplicitComponent()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("inlineComponentWithImplicitComponent.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);

    QCOMPARE(root->objectName(), "green blue"_L1);
}

struct QJSValueConvertible {

    Q_GADGET

public:
    QString msg;
};

bool operator==(const QJSValueConvertible &lhs, const QJSValueConvertible &rhs) {
    return lhs.msg == rhs.msg;
}

class TestItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QVector<QPointF> positions MEMBER m_points  )
    Q_PROPERTY( QSet<QByteArray> barrays MEMBER m_barrays  )
    Q_PROPERTY( QVector<QJSValueConvertible> convertibles MEMBER m_convertibles)

public:
    TestItem() = default;
    QVector< QPointF > m_points;
    QSet<QByteArray> m_barrays;
    QVector<QJSValueConvertible> m_convertibles;
};


Q_DECLARE_METATYPE(QVector<QPointF>);
Q_DECLARE_METATYPE(QSet<QByteArray>);
Q_DECLARE_METATYPE(QJSValueConvertible);
Q_DECLARE_METATYPE(QVector<QJSValueConvertible>);

void tst_qqmllanguage::arrayToContainer()
{
    QMetaType::registerConverter< QJSValue, QJSValueConvertible >(

        [](const QJSValue& value)
        {
            return QJSValueConvertible{value.toString()};
        }
    );
    QQmlEngine engine;
    qmlRegisterType<TestItem>("qt.test", 1, 0, "TestItem");
    QVector<QPointF> points { QPointF (2.0, 3.0) };
    QSet<QByteArray> barrays { QByteArray("hello"), QByteArray("world") };
    engine.rootContext()->setContextProperty("test", QVariant::fromValue(points));
    QQmlComponent component(&engine, testFileUrl("arrayToContainer.qml"));
    VERIFY_ERRORS(0);
    QScopedPointer<TestItem> root(qobject_cast<TestItem *>(component.createWithInitialProperties( {{"vector", QVariant::fromValue(points)}, {"myset", QVariant::fromValue(barrays)} } )));
    QVERIFY(root);
    QCOMPARE(root->m_points.at(0), QPointF (2.0, 3.0) );
    QVERIFY(root->m_barrays.contains("hello"));
    QVERIFY(root->m_barrays.contains("world"));
    QCOMPARE(root->m_convertibles.size(), 2);
    QCOMPARE(root->m_convertibles.at(0).msg, QLatin1String("hello"));
    QCOMPARE(root->m_convertibles.at(1).msg, QLatin1String("world"));
}

class EnumTester : public QObject
{
    Q_OBJECT
public:
    enum Types
    {
        FIRST = 0,
        SECOND,
        THIRD
    };
    Q_ENUM(Types)
};

void tst_qqmllanguage::qualifiedScopeInCustomParser()
{
    qmlRegisterUncreatableType<EnumTester>("scoped.custom.test", 1, 0, "EnumTester",
                                           "Object only creatable in C++");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml.Models 2.12\n"
                      "import scoped.custom.test 1.0 as BACKEND\n"
                      "ListModel {\n"
                      "    ListElement { text: \"a\"; type: BACKEND.EnumTester.FIRST }\n"
                      "}\n", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

void tst_qqmllanguage::checkUncreatableNoReason()
{
    qmlRegisterTypesAndRevisions<UncreatableElementNoReason>("qt.uncreatable.noreason", 1, 0);
    QQmlEngine engine;
    QString qml = QString("import QtQuick 2.0\nimport qt.uncreatable.noreason 1.0\nUncreatableElementNoReason {}");
    QQmlComponent c(&engine);
    c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
    QCOMPARE(c.errors().size(), 1);
    QCOMPARE(c.errors().first().description(), QString("Type cannot be created in QML."));
}

void tst_qqmllanguage::checkURLtoURLObject()
{
    QQmlEngine engine;
    QString qml = QString("import QtQuick 2.0\nItem { property url source: 'file:///foo/bar/'; "
                          "Component.onCompleted: { new URL(parent.source); } }");
    QQmlComponent c(&engine);
    c.setData(qml.toUtf8(), QUrl::fromLocalFile(QDir::currentPath()));
    QCOMPARE(c.errors().size(), 0);
}

struct TestValueType
{
    Q_GADGET
    Q_PROPERTY(int foo MEMBER foo)
public:
    int foo = 12;

    friend bool operator==(const TestValueType &a, const TestValueType &b)
    {
        return a.foo == b.foo;
    }

    friend bool operator!=(const TestValueType &a, const TestValueType &b)
    {
        return a.foo != b.foo;
    }
};

struct TestExtendedValueType
{
    Q_GADGET
    Q_PROPERTY(int bar READ bar WRITE setBar)
public:
    TestValueType wrapped;

    int bar() const { return wrapped.foo; }
    void setBar(int bar) { wrapped.foo = bar; }
};

class TestObjectType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TestValueType test MEMBER test)
public:
    TestValueType test;
};

void tst_qqmllanguage::registerValueTypes()
{
    QTest::ignoreMessage(QtWarningMsg, "Invalid QML element name \"UpperCase\"; value type names should begin with a lowercase letter");
    QVERIFY(qmlRegisterType<TestValueType>("DoesNotWork", 1, 0, "UpperCase") >= 0);
    QVERIFY(qmlRegisterType<TestObjectType>("DoesWork", 1, 0, "TestObject") >= 0);

    {
        QQmlEngine engine;
        QQmlComponent c(&engine);
        c.setData("import QtQml\nimport DoesWork\nTestObject { Component.onCompleted: test.foo = 14 }", QUrl());
        QVERIFY(c.isReady());
        QScopedPointer<QObject> obj(c.create());
        QCOMPARE(obj->property("test").value<TestValueType>().foo, 14);

        QQmlComponent c2(&engine);
        c2.setData("import QtQml\nimport DoesWork\n TestObject { Component.onCompleted: test.bar = 14 }", QUrl());
        QVERIFY(c2.isReady());
        QScopedPointer<QObject> obj2(c2.create());
        QCOMPARE(obj2->property("test").value<TestValueType>().foo, 12);
    }

    QVERIFY((qmlRegisterExtendedType<TestValueType, TestExtendedValueType>("DoesWork", 1, 0, "lowerCase")) >= 0);

    {
        QQmlEngine engine;
        QQmlComponent c(&engine);
        c.setData("import QtQml\nimport DoesWork\nTestObject { Component.onCompleted: test.foo = 14 }", QUrl());
        QVERIFY(c.isReady());
        QScopedPointer<QObject> obj(c.create());
        // The foo property is hidden now.
        QCOMPARE(obj->property("test").value<TestValueType>().foo, 12);

        QQmlComponent c2(&engine);
        c2.setData("import QtQml\nimport DoesWork\n TestObject { Component.onCompleted: test.bar = 14 }", QUrl());
        QVERIFY(c2.isReady());
        QScopedPointer<QObject> obj2(c2.create());
        QCOMPARE(obj2->property("test").value<TestValueType>().foo, 14);
    }
}

void tst_qqmllanguage::accessNullPointerPropertyCache()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("NullPointerPropertyCache.qml"));
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());
}

void tst_qqmllanguage::extendedNamespace()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\n"
              "import QtQml\n"
              "ExtendedByNamespace {\n"
              "    property int mine: own\n"
              "    property int myEnum: ExtendedByNamespace.Moo\n"
              "    property int fromExtension: ExtendedByNamespace.Bar\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(obj->property("mine").toInt(), 93);
    QCOMPARE(obj->property("myEnum").toInt(), 16);
    QCOMPARE(obj->property("fromExtension").toInt(), 9);
}

void tst_qqmllanguage::extendedNamespaceByObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\n"
              "import QtQml\n"
              "ExtendedNamespaceByObject {\n"
              "    extension: 10\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(obj->property("extension").toInt(), 10);
}

void tst_qqmllanguage::extendedByAttachedType()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\n"
                  "import QtQml\n"
                  "QtObject {\n"
                  "    ExtendedNamespaceByObject.attachedName: \"name for extension\"\n"
                  "}",
                  QUrl());
        QVERIFY2(!c.isReady(), qPrintable(c.errorString()));
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\n"
                  "import QtQml\n"
                  "QtObject {\n"
                  "    Extended.attachedName: \"name for extension\"\n"
                  "}",
                  QUrl());
        QVERIFY2(!c.isReady(), qPrintable(c.errorString()));
    }
}

void tst_qqmllanguage::factorySingleton()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\n"
              "import QtQml\n"
              "QtObject {\n"
              "    property int mine: FactorySingleton.foo\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(obj->property("mine").toInt(), 314);
}

void tst_qqmllanguage::extendedSingleton()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\n"
              "import QtQml\n"
              "QtObject {\n"
              "    property int a: ExtendedSingleton.foo\n"
              "    property int b: NamespaceExtendedSingleton.foo\n"
              "    property int c: ExtendedSingleton.extension\n"
              "    property int d: NamespaceExtendedSingleton.Bar\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(obj->property("a").toInt(), 315);
    QCOMPARE(obj->property("b").toInt(), 316);
    QCOMPARE(obj->property("c").toInt(), 42);
    QCOMPARE(obj->property("d").toInt(), 9);
}

void tst_qqmllanguage::qtbug_85932()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("badSingleton/qtbug_85932.qml"));
    QVERIFY(c.isError());

    const QString error = c.errorString();
    QVERIFY(error.contains(QLatin1String("Type SingletonTest unavailable")));
    QVERIFY(error.contains(QLatin1String("%1:10 id is not unique")
                                   .arg(testFileUrl("badSingleton/SingletonTest.qml").toString())));
}

void tst_qqmllanguage::multiExtension()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\nMultiExtension {}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("a").toInt(), int('a'));
    QCOMPARE(o->property("b").toInt(), int('b'));
    QCOMPARE(o->property("p").toInt(), int('p'));
    QCOMPARE(o->property("e").toInt(), int('e'));

    // Extension properties override base object properties
    QCOMPARE(o->property("c").toInt(), 12);
    QCOMPARE(o->property("d").toInt(), 22);
    QCOMPARE(o->property("f").toInt(), 31);
    QCOMPARE(o->property("g").toInt(), 44); // NB: taken from the type, not from the extension!

    QObject *extension = qmlExtendedObject(o.get());
    QVERIFY(extension != nullptr);
    QVERIFY(qobject_cast<ExtensionB *>(extension) != nullptr);

    QCOMPARE(QQmlPrivate::qmlExtendedObject(o.get(), 0), extension);
    QObject *baseTypeExtension = QQmlPrivate::qmlExtendedObject(o.get(), 1);
    QVERIFY(baseTypeExtension);
    QVERIFY(qobject_cast<ExtensionA *>(baseTypeExtension) != nullptr);
}

void tst_qqmllanguage::multiExtensionExtra()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\nMultiExtensionThreeExtensions {}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());

        QObject *extension = qmlExtendedObject(o.get());
        QVERIFY(extension != nullptr);
        QVERIFY(qobject_cast<Extension *>(extension) != nullptr);

        QCOMPARE(QQmlPrivate::qmlExtendedObject(o.get(), 0), extension);
        QObject *baseTypeExtension = QQmlPrivate::qmlExtendedObject(o.get(), 1);
        QVERIFY(baseTypeExtension);
        QVERIFY(qobject_cast<ExtensionB *>(baseTypeExtension) != nullptr);
        QObject *baseBaseTypeExtension = QQmlPrivate::qmlExtendedObject(o.get(), 2);
        QVERIFY(baseBaseTypeExtension);
        QVERIFY(qobject_cast<ExtensionA *>(baseBaseTypeExtension) != nullptr);
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\nMultiExtensionWithExtensionInBaseBase {}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());

        QObject *extension = qmlExtendedObject(o.get());
        QVERIFY(extension != nullptr);
        QVERIFY(qobject_cast<Extension *>(extension) != nullptr);

        QCOMPARE(QQmlPrivate::qmlExtendedObject(o.get(), 0), extension);

        QObject *baseBaseTypeExtension = QQmlPrivate::qmlExtendedObject(o.get(), 1);
        QVERIFY(baseBaseTypeExtension);
        QVERIFY(qobject_cast<ExtensionB *>(baseBaseTypeExtension) != nullptr);

        QObject *baseBaseBaseTypeExtension = QQmlPrivate::qmlExtendedObject(o.get(), 2);
        QVERIFY(baseBaseBaseTypeExtension);
        QVERIFY(qobject_cast<ExtensionA *>(baseBaseBaseTypeExtension) != nullptr);
    }
}

void tst_qqmllanguage::multiExtensionIndirect()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import StaticTest\nMultiExtensionIndirect {}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("a").toInt(), int('a'));
    QCOMPARE(o->property("b").toInt(), 77); // indirect extension is not considered
    QCOMPARE(o->property("p").toInt(), int('p'));
    QCOMPARE(o->property("e").toInt(), int('e'));

    QCOMPARE(o->property("c").toInt(), int('c')); // indirect extension is not considered
    QCOMPARE(o->property("d").toInt(), 21); // indirect extension is not considered
    QCOMPARE(o->property("f").toInt(), 31);
    QCOMPARE(o->property("g").toInt(), 44); // NB: taken from the type, not from the extension!
}

void tst_qqmllanguage::multiExtensionQmlTypes()
{
    QQmlType extendedType = QQmlMetaType::qmlType(&MultiExtensionParent::staticMetaObject,
                                                  QStringLiteral("StaticTest"), QTypeRevision());
    QVERIFY(extendedType.isValid());
    QVERIFY(extendedType.extensionFunction());
    QVERIFY(extendedType.extensionMetaObject() != nullptr);

    QQmlType nonExtendedType = QQmlMetaType::qmlType(&ExtendedInParent::staticMetaObject,
                                                     QStringLiteral("StaticTest"), QTypeRevision());
    QVERIFY(nonExtendedType.isValid());
    QVERIFY(!nonExtendedType.extensionFunction());
    QCOMPARE(nonExtendedType.extensionMetaObject(), nullptr);

    QQmlType namespaceExtendedType = QQmlMetaType::qmlType(
            &ExtendedByNamespace::staticMetaObject, QStringLiteral("StaticTest"), QTypeRevision());
    QVERIFY(namespaceExtendedType.isValid());
    QVERIFY(!namespaceExtendedType.extensionFunction()); // namespaces are non-creatable
    QVERIFY(namespaceExtendedType.extensionMetaObject() != nullptr);

    QQmlType namespaceNonExtendedType =
            QQmlMetaType::qmlType(&ExtendedByNamespaceInParent::staticMetaObject,
                                  QStringLiteral("StaticTest"), QTypeRevision());
    QVERIFY(namespaceNonExtendedType.isValid());
    QVERIFY(!namespaceNonExtendedType.extensionFunction());
    QCOMPARE(namespaceNonExtendedType.extensionMetaObject(), nullptr);
}

void tst_qqmllanguage::extensionSpecial()
{
    QQmlEngine engine;

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\nExtendedInParent {}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);

        // property a exists only in the extension type
        QCOMPARE(o->property("a").toInt(), int('a'));

        // property c exists on the leaf type but since extension's properties
        // are effectively FINAL, it is not used
        QCOMPARE(o->property("c").toInt(), 11);
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\nExtendedByIndirect {}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);

        // there are (visibly) no properties in this case
        QCOMPARE(o->property("b"), QVariant());
        QCOMPARE(o->property("c"), QVariant());
        QCOMPARE(o->property("d"), QVariant());
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\nExtendedInParentByIndirect {}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);

        // there are (visibly) no properties in this case (same as the previous)
        QCOMPARE(o->property("b"), QVariant());
        QCOMPARE(o->property("c"), QVariant());
        QCOMPARE(o->property("d"), QVariant());
    }
}

void tst_qqmllanguage::extensionRevision()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest 0.5\nExtendedWithRevisionOld { extension: 40\n}", QUrl());
        QVERIFY(!c.isReady());
        QRegularExpression error(
                ".*\"ExtendedWithRevisionOld.extension\" is not available in StaticTest 0.5.*");
        QVERIFY2(error.match(c.errorString()).hasMatch(),
                 qPrintable(u"Unmatched error: "_s + c.errorString()));
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest 1.0\nExtendedWithRevisionNew { extension: 40\n}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
    }
}

void tst_qqmllanguage::extendedGroupProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(import StaticTest 1.0
        ExtendedInGroup {
            group.value: 42
            group.value2: 42
        }
    )", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    ExtendedInGroup *extendedInGroup = qobject_cast<ExtendedInGroup *>(o.data());
    QVERIFY(extendedInGroup);
    QCOMPARE(extendedInGroup->group()->value(), 42);
    QCOMPARE(extendedInGroup->group()->value2(), 42);
}

void tst_qqmllanguage::invalidInlineComponent()
{
    QQmlEngine e;
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0\n"
              "import QtQuick.Window 2.1\n"
              "Window {\n"
              "    component TestPopup: Window {\n"
              "        visibility: Window.Windowed\n"
              "    }\n"
              "    TestPopup { color: \"blue\" }\n"
              "}", QUrl());
    QVERIFY(c.isError());
    QVERIFY(c.errorString().contains("\"Window.visibility\" is not available in QtQuick 2.0."));
}

void tst_qqmllanguage::warnOnInjectedParameters()
{
    QQmlEngine e;
    QQmlComponent c(&engine);
    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/foo.qml:4:5 Parameter \"bar\" is not declared."
                         " Injection of parameters into signal handlers is deprecated."
                         " Use JavaScript functions with formal parameters instead.");
    c.setData("import QtQml\n"
              "QtObject {\n"
              "    signal foo(bar: string)\n"
              "    onFoo: print(bar)\n"
              "    Component.onCompleted: foo('baz')\n"
              "}",
              QUrl("qrc:/foo.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(QtDebugMsg, "baz");
    QScopedPointer<QObject> o(c.create());
}

#if QT_CONFIG(wheelevent)
void tst_qqmllanguage::warnOnInjectedParametersFromCppSignal()
{
    QQmlEngine e;
    QQmlComponent c(&engine);
    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/qtbug93987.qml:6:21 Parameter \"wheel\" is not declared."
                         " Injection of parameters into signal handlers is deprecated."
                         " Use JavaScript functions with formal parameters instead.");
    c.setData(R"(
                import QtQuick
                MouseArea {
                    width: 640
                    height: 480
                    onWheel: print(wheel.angleDelta)
                })",
              QUrl("qrc:/qtbug93987.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    constexpr auto wheelDelta = 120; // copied from tst_QAbstractSlider. probably a default unit
    QPoint p(100, 100);
    QWheelEvent event(p, p, QPoint(), QPoint(0, wheelDelta * 1), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QTest::ignoreMessage(QtDebugMsg, "QPoint(0, 120)");
    QCoreApplication::sendEvent(o.get(), &event);
}
#endif

void tst_qqmllanguage::qtbug_86482()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray(R"(import QtQml 2.0
                                 import StaticTest
                                 QtObject {
                                     id: root
                                     property string result
                                     property StringSignaler str: StringSignaler {
                                        onSignal: function(value) { root.result = value; }
                                     }
                                     Component.onCompleted: str.call();
                                 })"), QUrl());
    VERIFY_ERRORS(0);
    QScopedPointer<QObject> o(component.create());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QCOMPARE(o->property("result").toString(), QStringLiteral("Hello world!"));
}

void tst_qqmllanguage::qtbug_85615()
{
    qmlRegisterSingletonType("Test.Singleton", 1, 0, "SingletonString", [](QQmlEngine *, QJSEngine *) -> QJSValue {
        return QJSValue("Test");
    });
    qmlRegisterSingletonType("Test.Singleton", 1, 0, "SingletonInt", [](QQmlEngine *, QJSEngine *) -> QJSValue {
        return QJSValue(123);
    });
    qmlRegisterSingletonType("Test.Singleton", 1, 0, "SingletonDouble", [](QQmlEngine *, QJSEngine *) -> QJSValue {
        return QJSValue(1.23);
    });
    qmlRegisterSingletonType("Test.Singleton", 1, 0, "SingletonUndefined", [](QQmlEngine *, QJSEngine *) -> QJSValue {
        return QJSValue(QJSValue::UndefinedValue);
    });
    qmlRegisterSingletonType("Test.Singleton", 1, 0, "SingletonNull", [](QQmlEngine *, QJSEngine *) -> QJSValue {
        return QJSValue(QJSValue::NullValue);
    });

    QQmlEngine e;
    QQmlComponent c(&engine);
    c.setData("import QtQml 2.0\n"
              "import Test.Singleton\n"
              "QtObject {\n"
              "    property var resultString: SingletonString\n"
              "    property var resultInt: SingletonInt\n"
              "    property var resultDouble: SingletonDouble\n"
              "    property var resultUndefined: SingletonUndefined\n"
              "    property var resultNull: SingletonNull\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("resultString").toString(), "Test");
    QCOMPARE(o->property("resultInt").toInt(), 123);
    QCOMPARE(o->property("resultDouble").toDouble(), 1.23);
    QVERIFY(!o->property("resultUndefined").isValid());
    QCOMPARE(o->property("resultUndefined").metaType(), QMetaType(QMetaType::UnknownType));
    QCOMPARE(o->property("resultNull").metaType(), QMetaType(QMetaType::Nullptr));
}

void tst_qqmllanguage::bareInlineComponent()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("bareInline.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlMetaType::freeUnusedTypesAndCaches();

    bool tab1Found = false;
    const auto types = QQmlMetaType::qmlTypes();
    for (const QQmlType &type : types) {
        if (type.elementName() == QStringLiteral("Tab1")) {
            QVERIFY(type.module().isEmpty());
            tab1Found = true;

            const QQmlType leftTab = QQmlMetaType::inlineComponentType(type, "LeftTab");
            QUrl leftUrl = leftTab.sourceUrl();
            leftUrl.setFragment(QString());
            QCOMPARE(leftUrl, type.sourceUrl());

            const QQmlType rightTab = QQmlMetaType::inlineComponentType(type, "RightTab");
            QUrl rightUrl = rightTab.sourceUrl();
            rightUrl.setFragment(QString());
            QCOMPARE(rightUrl, type.sourceUrl());
        }
    }
    QVERIFY(tab1Found);
}

#if QT_CONFIG(qml_debug)
struct DummyDebugger : public QV4::Debugging::Debugger
{
    bool pauseAtNextOpportunity() const final { return false; }
    void maybeBreakAtInstruction() final { }
    void enteringFunction() final { }
    void leavingFunction(const QV4::ReturnedValue &) final { }
    void aboutToThrow() final { }
};
#else
using DummyDebugger = QV4::Debugging::Debugger; // it's already dummy
#endif

void tst_qqmllanguage::hangOnWarning()
{
    QQmlEngine engine;

    // A debugger prevents the disk cache.
    // If we load the file from disk cache we don't parse it and we don't see the warning.
    engine.handle()->setDebugger(new DummyDebugger);

    QTest::ignoreMessage(QtWarningMsg,
                         qPrintable(QStringLiteral("%1:3 : Ignored annotation")
                                            .arg(testFileUrl("hangOnWarning.qml").toString())));
    QQmlComponent component(&engine, testFileUrl("hangOnWarning.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

void tst_qqmllanguage::groupPropertyFromNonExposedBaseClass()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("derivedFromUnexposedBase.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    auto root = qobject_cast<DerivedFromUnexposedBase *>(o.get());
    QVERIFY(root);
    QVERIFY(root->group);
    QCOMPARE(root->group->value, 42);
    QCOMPARE(root->groupGadget.value, 42);

    c.loadUrl(testFileUrl("dynamicGroupPropertyRejected.qml"));
    QVERIFY(c.isError());
    QVERIFY2(c.errorString().contains("Unsupported grouped property access"), qPrintable(c.errorString()));
}

void tst_qqmllanguage::listEnumConversion()
{
    QQmlEngine e;
    QQmlComponent c(&engine);
    c.setData(R"(
import QtQml 2.0
import StaticTest 1.0
QtObject {
    property EnumList enumList: EnumList {}
    property var list: enumList.list()
    property bool resultAlpha: EnumList.Alpha === list[0]
    property bool resultBeta: EnumList.Beta === list[1]
    property bool resultGamma: EnumList.Gamma === list[2]
    property var resultEnumType: EnumList.Alpha
    property var resultEnumListType: list[0]
})",
              QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("resultAlpha").toBool(), true);
    QCOMPARE(o->property("resultBeta").toBool(), true);
    QCOMPARE(o->property("resultGamma").toBool(), true);
    QCOMPARE(o->property("resultEnumType").metaType(), QMetaType(QMetaType::Int));
    QCOMPARE(o->property("resultEnumListType").metaType(), QMetaType(QMetaType::Int));
}

void tst_qqmllanguage::deepInlineComponentScriptBinding()
{
    QQmlEngine e;
    QQmlComponent c(&engine);
    c.loadUrl(testFileUrl("deepInlineComponentScriptBinding.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::propertyObserverOnReadonly()
{
    QQmlEngine e;
    QQmlComponent c(&engine, testFileUrl("SelectionRange.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("zoomer").toDouble(), o->property("height").toDouble());
    o->setProperty("height", QVariant::fromValue<double>(54.2));
    QCOMPARE(o->property("zoomer").toDouble(), 54.2);
    QCOMPARE(o->property("height").toDouble(), 54.2);
}

void tst_qqmllanguage::valueTypeWithEnum()
{
    {
        QQmlEngine e;
        QQmlComponent c(&engine);
        c.setData("import Test\n"
                  "ObjectTypeHoldingValueType1 {\n"
                  "    vv.quality: ValueTypeWithEnum1.NormalQuality\n"
                  "}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        ObjectTypeHoldingValueType1 *holder = qobject_cast<ObjectTypeHoldingValueType1 *>(o.data());

        QCOMPARE(holder->vv().quality(), ValueTypeWithEnum1::NormalQuality);
    }

    {
        QQmlEngine e;
        QQmlComponent c(&engine);
        c.setData("import Test\n"
                  "ObjectTypeHoldingValueType2 {\n"
                  "    vv.quality: ValueTypeWithEnum2.LowQuality\n"
                  "}", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        ObjectTypeHoldingValueType2 *holder = qobject_cast<ObjectTypeHoldingValueType2 *>(o.data());

        QCOMPARE(holder->vv().quality(), ValueTypeWithEnum2::LowQuality);
    }
}

void tst_qqmllanguage::propertyAndAliasMustHaveDistinctNames_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("error");

    QTest::addRow("sameNamePropertyAlias") << "sameNamePropertyAlias.qml" << "Property duplicates alias name";
    QTest::addRow("sameNameAliasProperty") << "sameNameAliasProperty.qml" << "Alias has same name as existing property";
}

void tst_qqmllanguage::propertyAndAliasMustHaveDistinctNames()
{
    QFETCH(QString, fileName);
    QFETCH(QString, error);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(fileName));
    QVERIFY(!c.isReady());
    auto actualError = c.errorString();
    QVERIFY2(actualError.contains(error), qPrintable(actualError));
}

void tst_qqmllanguage::enumsFromRelatedTypes()
{
    QQmlEngine e;
    {
        QQmlComponent c(&engine);
        c.setData("import Test\n"
                  "ObjectTypeHoldingValueType1 {\n"
                  "    vv.quality: ObjectTypeHoldingValueType1.NormalQuality\n"
                  "}", QUrl("Test1.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        ObjectTypeHoldingValueType1 *holder = qobject_cast<ObjectTypeHoldingValueType1 *>(o.data());
        QCOMPARE(holder->q(), ValueTypeWithEnum1::NormalQuality);
    }

    {
        QQmlComponent c(&engine);
        c.setData("import Test\n"
                  "ObjectTypeHoldingValueType2 {\n"
                  "    vv.quality: ObjectTypeHoldingValueType2.LowQuality\n"
                  "}", QUrl("Test2.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));

        QTest::ignoreMessage(
                    QtWarningMsg,
                    "Test2.qml:3:5: Unable to assign [undefined] to ValueTypeWithEnum2::Quality");
        QScopedPointer<QObject> o(c.create());
        ObjectTypeHoldingValueType2 *holder = qobject_cast<ObjectTypeHoldingValueType2 *>(o.data());
        QCOMPARE(holder->q(), ValueTypeWithEnum2::HighQuality);
    }
}

void tst_qqmllanguage::variantListConversion()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("variantListConversion.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    Foo *foo = qobject_cast<Foo *>(o.data());
    QVERIFY(foo);
    const QVariantList list = foo->getList();
    QCOMPARE(list.size(), 3);
    const Large l0 = qvariant_cast<Large>(list.at(0));
    QCOMPARE(l0.a, 12ull);
    const Large l1 = qvariant_cast<Large>(list.at(1));
    QCOMPARE(l1.a, 13ull);
    const QObject *attached = qvariant_cast<QObject *>(list.at(2));
    QVERIFY(attached);
    QCOMPARE(attached->metaObject(), &QQmlComponentAttached::staticMetaObject);
}

void tst_qqmllanguage::thisInArrowFunction()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("thisInArrow.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtDebugMsg, "43");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(qvariant_cast<QObject *>(o->property("arrowResult")), o.data());
    QCOMPARE(qvariant_cast<QObject *>(o->property("funcResult")), o.data());
    QCOMPARE(qvariant_cast<QObject *>(o->property("aResult")), o.data());
    QCOMPARE(qvariant_cast<QObject *>(o->property("aaResult")), o.data());

    QCOMPARE(qvariant_cast<QObject *>(o->property("fResult")), nullptr);
    QCOMPARE(o->property("fResult").metaType(), QMetaType::fromType<QJSValue>());
    QVERIFY(qvariant_cast<QJSValue>(o->property("fResult")).isObject());

    QObject *child = qvariant_cast<QObject *>(o->property("child"));
    QVERIFY(child != nullptr);
    QCOMPARE(qvariant_cast<QObject *>(o->property("ffResult")), child);
}

void tst_qqmllanguage::jittedAsCast()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("jittedAsCast.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("running").toBool(), true);
    QTRY_COMPARE(o->property("running").toBool(), false);
    QCOMPARE(o->property("interval").toInt(), 10);
}

void tst_qqmllanguage::propertyNecromancy()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("propertyNecromancy.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(qvariant_cast<QObject *>(o->property("notified")) != nullptr);

    // It becomes null, not undefined.
    QTRY_VERIFY(o->property("notified").isNull());
    QVERIFY(o->property("notified").isValid());
}

void tst_qqmllanguage::generalizedGroupedProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("generalizedGroupedProperty.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->objectName(), QStringLiteral("foo"));
    MyAttachedObject *rootAttached = static_cast<MyAttachedObject *>(
                qmlAttachedPropertiesObject<MyQmlObject>(o.data()));
    QVERIFY(rootAttached);
    QCOMPARE(rootAttached->value(), 0);
    QCOMPARE(rootAttached->value2(), 0);

    ImmediateProperties *child = qvariant_cast<ImmediateProperties *>(o->property("child"));
    QVERIFY(child);
    QCOMPARE(child->objectName(), QStringLiteral("barrrrr"));

    MyAttachedObject *childAttached = static_cast<MyAttachedObject *>(
                qmlAttachedPropertiesObject<MyQmlObject>(child));
    QVERIFY(childAttached);
    QCOMPARE(childAttached->value(), 0);

    qmlExecuteDeferred(child);
    QCOMPARE(childAttached->value(), 4);

    QCOMPARE(o->objectName(), QStringLiteral("barrrrr ..."));
    QCOMPARE(rootAttached->value(), 10);
    QCOMPARE(rootAttached->value2(), 0);
    QCOMPARE(childAttached->value(), 4);

    o->metaObject()->invokeMethod(o.data(), "something");
    QCOMPARE(o->objectName(), QStringLiteral("rabrab ..."));

    ImmediateProperties *meanChild = qvariant_cast<ImmediateProperties *>(o->property("meanChild"));
    QVERIFY(meanChild);
    qmlExecuteDeferred(meanChild);
    QCOMPARE(child->objectName(), QStringLiteral("bar"));
    QCOMPARE(o->objectName(), QStringLiteral("bar ..."));
    QCOMPARE(childAttached->value(), 11);

    ImmediateProperties *deferred = qvariant_cast<ImmediateProperties *>(o->property("deferred"));
    QVERIFY(deferred);
    QCOMPARE(deferred->objectName(), QStringLiteral("holz"));
    qmlExecuteDeferred(deferred);
    QCOMPARE(o->objectName(), QStringLiteral("holz ..."));
    QCOMPARE(rootAttached->value(), 10);
    QCOMPARE(rootAttached->value2(), 12);

    ImmediateProperties *meanDeferred
            = qvariant_cast<ImmediateProperties *>(o->property("meanDeferred"));
    QVERIFY(meanDeferred);
    qmlExecuteDeferred(meanDeferred);
    QCOMPARE(deferred->objectName(), QStringLiteral("stein"));
    QCOMPARE(o->objectName(), QStringLiteral("stein ..."));

    {
        QQmlComponent bad(&engine, testFileUrl("badGeneralizedGroupedProperty.qml"));
        QVERIFY(bad.isError());
        QVERIFY(bad.errorString().contains(
                    QStringLiteral("Cannot assign to non-existent property \"root\"")));
    }

    {
        QQmlComponent bad(&engine, testFileUrl("badGeneralizedGroupedProperty2.qml"));
        QVERIFY(bad.isError());
        QVERIFY(bad.errorString().contains(QStringLiteral("Invalid grouped property access")));
    }
}

void tst_qqmllanguage::groupedAttachedProperty_data()
{
    QTest::addColumn<QString>("file");
    QTest::addRow("12") << QStringLiteral("validAttachedProperty.12.qml");
    QTest::addRow("13") << QStringLiteral("validAttachedProperty.13.qml");
}

void tst_qqmllanguage::groupedAttachedProperty()
{
    QFETCH(QString, file);

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(file));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    MyTypeObject *typed = qobject_cast<MyTypeObject *>(o.data());
    QVERIFY(typed != nullptr);
    MyGroupedObject *grouped = typed->grouped();
    QVERIFY(grouped != nullptr);
    MyAttachedObject *attached = qobject_cast<MyAttachedObject *>(
                qmlAttachedPropertiesObject<MyQmlObject>(grouped));
    QVERIFY(attached != nullptr);
    QCOMPARE(attached->value(), 10);
}

void tst_qqmllanguage::ambiguousContainingType()
{
    // Need to do it twice, so that we load from disk cache the second time.
    for (int i = 0; i < 2; ++i) {
        QQmlEngine engine;

        // Should not crash when loading the type
        QQmlComponent c(&engine, testFileUrl("ambiguousBinding/ambiguousContainingType.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
    }
}

void tst_qqmllanguage::objectAsBroken()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("asBroken.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QVariant selfAsBroken = o->property("selfAsBroken");
    QVERIFY(selfAsBroken.isValid());
    QCOMPARE(selfAsBroken.metaType(), QMetaType::fromType<std::nullptr_t>());

    QQmlComponent b(&engine, testFileUrl("Broken.qml"));
    QVERIFY(b.isError());
}

void tst_qqmllanguage::customValueTypes()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("customValueTypes.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(qvariant_cast<BaseValueType>(o->property("base")).content(), 27);
    QCOMPARE(qvariant_cast<DerivedValueType>(o->property("derived")).content(), 28);

    o->setObjectName(QStringLiteral("a"));

    QCOMPARE(qvariant_cast<DerivedValueType>(o->property("derived")).content(), 14);
    QCOMPARE(qvariant_cast<BaseValueType>(o->property("base")).content(), 13);
}

void tst_qqmllanguage::valueTypeList()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("valueTypeList.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    {
        QCOMPARE(o->property("c").toInt(), 17);
        QCOMPARE(qvariant_cast<QPointF>(o->property("d")), QPointF(3, 4));
        QCOMPARE(qvariant_cast<DerivedValueType>(o->property("y")).content(), 29);
        const QList<DerivedValueType> x = qvariant_cast<QList<DerivedValueType>>(o->property("x"));
        QCOMPARE(x.size(), 3);
        for (const DerivedValueType &d : x)
            QCOMPARE(d.content(), 29);

        const QList<BaseValueType> baseList
                = qvariant_cast<QList<BaseValueType>>(o->property("baseList"));
        QCOMPARE(baseList.size(), 3);
        for (const BaseValueType &b : baseList)
            QCOMPARE(b.content(), 29);

        const QRectF f = qvariant_cast<QRectF>(o->property("f"));
        QCOMPARE(f, QRectF(0, 2, 17, 1));
    }

    o->setObjectName(QStringLiteral("foo"));
    {
        QCOMPARE(qvariant_cast<QPointF>(o->property("d")), QPointF(12, 4));

        // x is an actual value type list. We don't store references to y in there, but actual copies.
        QCOMPARE(qvariant_cast<DerivedValueType>(o->property("y")).content(), 29);

        const QList<DerivedValueType> x = qvariant_cast<QList<DerivedValueType>>(o->property("x"));
        QCOMPARE(x.size(), 3);
        QCOMPARE(x[0].content(), 29);
        QCOMPARE(x[1].content(), 30);
        QCOMPARE(x[2].content(), 29);

        const QList<BaseValueType> baseList
                = qvariant_cast<QList<BaseValueType>>(o->property("baseList"));
        QCOMPARE(baseList.size(), 3);
        for (const BaseValueType &b : baseList)
            QCOMPARE(b.content(), 29);
    }
}

void tst_qqmllanguage::componentMix()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("componentMix.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QObject *things = qvariant_cast<QObject *>(o->property("things"));
    QVERIFY(things);
    QObject *delegated = qvariant_cast<QObject *>(o->property("delegated"));
    QVERIFY(delegated);
    QObject *view = qvariant_cast<QObject *>(things->property("view"));
    QVERIFY(view);
    QObject *delegate = qvariant_cast<QObject *>(view->property("delegate"));
    QVERIFY(delegate);
    QCOMPARE(delegate->metaObject(), &QQmlComponent::staticMetaObject);
    QObject *delegate2 = qvariant_cast<QObject *>(delegated->property("delegate"));
    QVERIFY(delegate2);
    QCOMPARE(delegate2->metaObject(), &QQmlComponent::staticMetaObject);
}

void tst_qqmllanguage::uncreatableAttached()
{
    qmlRegisterTypesAndRevisions<ItemAttached>("ABC", 1);
    QQmlEngine engine;
    const QUrl url = testFileUrl("uncreatableAttached.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(QtWarningMsg, "Only foo can have ItemAttached!");
    QScopedPointer o(c.create());
    QVERIFY(o.isNull());
    QVERIFY(c.errorString().contains(
                QLatin1String("Could not create attached properties object 'ItemAttached'")));
}

class MyGadget
{
    Q_GADGET
    Q_PROPERTY(int value READ value WRITE setValue RESET resetValue)

public:
    int value() const { return m_value; }
    void setValue(int value) { m_value = value; }
    void resetValue() { setValue(25); }

    bool operator==(const MyGadget &other) const { return m_value == other.m_value; }
    bool operator!=(const MyGadget &other) const { return m_value != other.m_value; }

private:
    int m_value = -1;
};

class MyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyGadget gadget MEMBER m_gadget RESET resetGadget NOTIFY gadgetChanged)
    void resetGadget() { qDebug("FAIL"); }

public:
    MyObject(QObject *parent = nullptr) : QObject(parent) { }
    MyGadget m_gadget;

signals:
    void gadgetChanged();
};

void tst_qqmllanguage::resetGadgetProperty()
{
    qmlRegisterType<MyObject>("MyObject", 1, 0, "MyObject");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                "import QtQml 2.0; import MyObject 1.0; MyObject { gadget.value: undefined }",
                QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    MyObject *m = qobject_cast<MyObject *>(o.data());
    QVERIFY(m);
    QCOMPARE(m->m_gadget.value(), 25);
}

void tst_qqmllanguage::leakingAttributesQmlAttached()
{
    // Check for leakage in the QML_ATTACHED macro
    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\n"
                  "import QtQuick\n"
                  "Item {"
                  "     OriginalQmlAttached.abc: true"
                  "}",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QObject *attached = qmlAttachedPropertiesObject<OriginalQmlAttached>(o.data());
        QVERIFY(attached);
        QCOMPARE(attached->property("abc"), QVariant(true));
    }
    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\n"
                  "import QtQuick\n"
                  "Item {"
                  "     LeakingQmlAttached.abc: true"
                  "}",
                  QUrl());
        QVERIFY(!c.isReady());
    }

    {
        QQmlComponent c(&engine);
        c.setData("import StaticTest\n"
                  "import QtQuick\n"
                  "Item {"
                  "     DerivedQmlAttached.anotherAbc: \"I am not a bool.\"\n"
                  "     OriginalQmlAttached.abc: true"
                  "}",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);

        QObject *attached = qmlAttachedPropertiesObject<OriginalQmlAttached>(o.data());
        QVERIFY(attached);
        QCOMPARE(attached->property("abc"), QVariant(true));

        attached = qmlAttachedPropertiesObject<DerivedQmlAttached>(o.data());
        QVERIFY(attached);
        QCOMPARE(attached->property("anotherAbc"), QVariant("I am not a bool."));
    }
}

void tst_qqmllanguage::leakingAttributesQmlSingleton()
{
    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
import QtQuick
Item {
    Text { text: OriginalSingleton.abc }
    Text { text: OriginalSingleton.abc }
    Text { id: check }
    Component.onCompleted: {
        OriginalSingleton.abc = "Updated string content!"
        check.text = "onCompletedExecuted!"
    }
})",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->children().size(), 4);
        QCOMPARE(o->children()[2]->property("text"), QVariant("onCompletedExecuted!"));
        QCOMPARE(o->children()[0]->property("text"), QVariant("Updated string content!"));
        QCOMPARE(o->children()[1]->property("text"), QVariant("Updated string content!"));
    }
    {
        QQmlComponent c(&engine);

        c.setData(R"(
import StaticTest
import QtQuick
Item {
    property var text: LeakingSingleton.abc
    property var text2: LeakingSingleton.abc
    Text { id: check }
    Component.onCompleted: {
        LeakingSingleton.abc = "Updated string content!"
        check.text = "onCompletedExecuted!"
    }
})",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->children().size(), 2);
        QCOMPARE(o->children().front()->property("text"), QVariant("onCompletedExecuted!"));
        // empty because not a singleton -> LeakingSingleton.abc is [undefined]
        QVERIFY(!o->property("text").isValid());
        QVERIFY(!o->property("text2").isValid());
    }
    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
import QtQuick
Item {
     Text { text: DerivedSingleton.anotherAbc }
     Text { text: DerivedSingleton.anotherAbc }
     Text { id: check }
     Component.onCompleted: {
         DerivedSingleton.anotherAbc = "Updated string content!";
         check.text = "onCompletedExecuted!"
     }
})",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->children().size(), 4);
        QCOMPARE(o->children()[2]->property("text"), QVariant("onCompletedExecuted!"));
        QCOMPARE(o->children()[0]->property("text"), QVariant("Updated string content!"));
        QCOMPARE(o->children()[1]->property("text"), QVariant("Updated string content!"));
    }
}

void tst_qqmllanguage::leakingAttributesQmlForeign()
{
    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
ForeignerForeign {
     abc: "Hello World"
})",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QVERIFY(o->property("abc").isValid());
    }
    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
LeakingForeignerForeign {
})",
                  QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QVERIFY(o->property("anotherAbc").isValid());
        QVERIFY(!o->property("abc").isValid());
    }

    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
import QtQml
QtObject {
    objectName: 'b' + ForeignNamespaceForeign.B
})", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->objectName(), "b1");
    }
    {
        QQmlComponent c(&engine);
        c.setData(R"(
import StaticTest
import QtQml
QtObject {
    objectName: 'b' + LeakingForeignNamespaceForeign.B
})", QUrl());
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->objectName(), "b2");
    }
}

void tst_qqmllanguage::attachedOwnProperties()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import QML
        QtObject {
            id: root
            property list<string> props: Object.getOwnPropertyNames(root.Component)
        }
    )", QUrl(QStringLiteral("attachedOwn.qml")));

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    const QStringList expected {"objectName", "objectNameChanged", "completed", "destruction"};
    QCOMPARE(o->property("props").value<QStringList>(), expected);
}

void tst_qqmllanguage::bindableOnly()
{
    qmlRegisterTypesAndRevisions<BindableOnly>("ABC", 1);
    QQmlEngine engine;

    QQmlComponent c(&engine);
    c.setData("import ABC\nBindableOnly {\n"
              "    property int a: score\n"
              "    data: \"sc\" + \"ore\"\n"
              "    objectName: data\n"
              "}", QUrl(u"bindableOnly.qml"_s));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    BindableOnly *bindableOnly = qobject_cast<BindableOnly *>(o.data());
    QCOMPARE(bindableOnly->scoreBindable().value(), 4);
    QCOMPARE(o->property("a").toInt(), 4);
    bindableOnly->scoreBindable().setValue(5);
    QCOMPARE(bindableOnly->scoreBindable().value(), 5);
    QCOMPARE(o->property("a").toInt(), 5);
    QCOMPARE(o->property("data").value<QByteArray>(), QByteArray("score"));
    QCOMPARE(o->objectName(), QStringLiteral("score"));
}

static void listsEqual(QObject *object, const char *method)
{
    const QByteArray v4SequencePropertyName = QByteArray("v4Sequence") + method;
    const QByteArray jsArrayPropertyName = QByteArray("jsArray") + method;

    const QList<QRectF> v4SequenceProperty
            = object->property(v4SequencePropertyName.constData()).value<QList<QRectF>>();
    const QList<QRectF> jsArrayProperty
            = object->property(jsArrayPropertyName.constData()).value<QList<QRectF>>();

    const qsizetype v4SequenceCount = v4SequenceProperty.size();
    QCOMPARE(v4SequenceCount, jsArrayProperty.size());

    for (qsizetype i = 0; i < v4SequenceCount; ++i)
        QCOMPARE(v4SequenceProperty.at(i), jsArrayProperty.at(i));
}

void tst_qqmllanguage::v4SequenceMethods()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("v4SequenceMethods.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("v4SequenceToString"), object->property("jsArrayToString"));
    QCOMPARE(object->property("v4SequenceToLocaleString"), object->property("jsArrayToLocaleString"));

    QVERIFY(object->property("entriesMatch").toBool());
    QVERIFY(object->property("keysMatch").toBool());
    QVERIFY(object->property("valuesMatch").toBool());

    listsEqual(object.data(), "Concat");
    listsEqual(object.data(), "Pop");
    listsEqual(object.data(), "Push");
    listsEqual(object.data(), "Reverse");
    listsEqual(object.data(), "Shift");
    listsEqual(object.data(), "Unshift");
    listsEqual(object.data(), "Filter");
    listsEqual(object.data(), "Sort1");
    listsEqual(object.data(), "Sort2");

    QCOMPARE(object->property("v4SequenceFind"), object->property("jsArrayFind"));
    QCOMPARE(object->property("v4SequenceFind").value<QRectF>().x(), 1.0);

    QCOMPARE(object->property("v4SequenceFindIndex"), object->property("jsArrayFindIndex"));
    QCOMPARE(object->property("v4SequenceFindIndex").toInt(), 1);

    QCOMPARE(object->property("v4SequenceIncludes"), object->property("jsArrayIncludes"));
    QVERIFY(object->property("v4SequenceIncludes").toBool());

    QCOMPARE(object->property("v4SequenceJoin"), object->property("jsArrayJoin"));
    QVERIFY(object->property("v4SequenceJoin").toString().contains(QStringLiteral("1")));

    QCOMPARE(object->property("v4SequencePopped"), object->property("jsArrayPopped"));
    QVERIFY(object->property("v4SequencePopped").value<QRectF>().x() != 1.0);

    QCOMPARE(object->property("v4SequencePushed"), object->property("jsArrayPushed"));
    QCOMPARE(object->property("v4SequencePushed").toInt(), 4);

    QCOMPARE(object->property("v4SequenceShifted"), object->property("jsArrayShifted"));
    QCOMPARE(object->property("v4SequenceShifted").value<QRectF>().x(), 1.0);

    QCOMPARE(object->property("v4SequenceUnshifted"), object->property("jsArrayUnshifted"));
    QCOMPARE(object->property("v4SequenceUnshifted").toInt(), 4);

    QCOMPARE(object->property("v4SequenceIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("v4SequenceIndexOf").toInt(), 1);

    QCOMPARE(object->property("v4SequenceLastIndexOf"), object->property("jsArrayLastIndexOf"));
    QCOMPARE(object->property("v4SequenceLastIndexOf").toInt(), 2);

    QCOMPARE(object->property("v4SequenceEvery"), object->property("jsArrayEvery"));
    QVERIFY(object->property("v4SequenceEvery").toBool());

    QCOMPARE(object->property("v4SequenceSome"), object->property("jsArrayEvery"));
    QVERIFY(object->property("v4SequenceSome").toBool());

    QCOMPARE(object->property("v4SequenceForEach"), object->property("jsArrayForEach"));
    QCOMPARE(object->property("v4SequenceForEach").toString(), QStringLiteral("-1--5--9-"));

    QCOMPARE(object->property("v4SequenceMap").toStringList(), object->property("jsArrayMap").toStringList());
    QCOMPARE(object->property("v4SequenceReduce").toString(), object->property("jsArrayReduce").toString());

    QCOMPARE(object->property("v4SequenceOwnPropertyNames").toStringList(),
             object->property("jsArrayOwnPropertyNames").toStringList());
}

void tst_qqmllanguage::v4SequenceMethodsWithParams_data()
{
    QTest::addColumn<double>("i");
    QTest::addColumn<double>("j");
    QTest::addColumn<double>("k");

    const double indices[] = {
        double(std::numeric_limits<qsizetype>::min()),
        double(std::numeric_limits<qsizetype>::min()) + 1,
        double(std::numeric_limits<uint>::min()) - 1,
        double(std::numeric_limits<uint>::min()),
        double(std::numeric_limits<uint>::min()) + 1,
        double(std::numeric_limits<int>::min()),
        -10, -3, -2, -1, 0, 1, 2, 3, 10,
        double(std::numeric_limits<int>::max()),
        double(std::numeric_limits<uint>::max()) - 1,
        double(std::numeric_limits<uint>::max()),
        double(std::numeric_limits<uint>::max()) + 1,
        double(std::numeric_limits<qsizetype>::max() - 1),
        double(std::numeric_limits<qsizetype>::max()),
    };

    // We cannot test the full cross product. So, take a random sample instead.
    const qsizetype numIndices = sizeof(indices) / sizeof(double);
    qsizetype seed = QRandomGenerator::global()->generate();
    const int numSamples = 4;
    for (int i = 0; i < numSamples; ++i) {
        seed = qHash(i, seed);
        const double vi = indices[qAbs(seed) % numIndices];
        for (int j = 0; j < numSamples; ++j) {
            seed = qHash(j, seed);
            const double vj = indices[qAbs(seed) % numIndices];
            for (int k = 0; k < numSamples; ++k) {
                seed = qHash(k, seed);
                const double vk = indices[qAbs(seed) % numIndices];
                const QString tag = QLatin1String("%1/%2/%3")
                        .arg(QString::number(vi), QString::number(vj), QString::number(vk));
                QTest::newRow(qPrintable(tag)) << vi << vj << vk;

                // output all the tags so that we can find out what combination caused a test to hang.
                qDebug().noquote() << "scheduling" << tag;
            }
        }
    }
}

void tst_qqmllanguage::v4SequenceMethodsWithParams()
{
    QFETCH(double, i);
    QFETCH(double, j);
    QFETCH(double, k);
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("v4SequenceMethodsWithParams.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {QStringLiteral("i"), i},
            {QStringLiteral("j"), j},
            {QStringLiteral("k"), k}
    }));
    QVERIFY(!object.isNull());

    listsEqual(object.data(), "CopyWithin");
    listsEqual(object.data(), "Fill");
    listsEqual(object.data(), "Slice");
    listsEqual(object.data(), "Splice");
    listsEqual(object.data(), "Spliced");

    QCOMPARE(object->property("v4SequenceIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("v4SequenceLastIndexOf"), object->property("jsArrayLastIndexOf"));
}

void tst_qqmllanguage::jsFunctionOverridesImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("jsFunctionOverridesImport.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QCOMPARE(object->objectName(), u"foo"_s);
}

void tst_qqmllanguage::bindingAliasToComponentUrl()
{
    QQmlEngine engine;
    {
        QQmlComponent component(&engine, testFileUrl("bindingAliasToComponentUrl.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);
        QCOMPARE(object->property("accessibleNormalUrl"), object->property("urlClone"));
    }
    {
        QQmlComponent component(&engine, testFileUrl("bindingAliasToComponentUrl2.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);
        QCOMPARE(object->property("accessibleNormalProgress"), QVariant(1.0));
    }
}

void tst_qqmllanguage::badGroupedProperty()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("badGroupedProperty.qml");
    QQmlComponent c(&engine, url);
    QVERIFY(c.isError());
    QCOMPARE(c.errorString(),
             QStringLiteral("%1:6 Cannot assign to non-existent property \"onComplete\"\n")
             .arg(url.toString()));
}

void tst_qqmllanguage::functionInGroupedProperty()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("functionInGroupedProperty.qml");
    QQmlComponent c(&engine, url);
    QVERIFY(c.isError());
    QCOMPARE(c.errorString(),
             QStringLiteral("%1:6 Function declaration inside grouped property\n")
                     .arg(url.toString()));
}

void tst_qqmllanguage::signalInlineComponentArg()
{
    QQmlEngine engine;
    {
        QQmlComponent component(&engine, testFileUrl("SignalInlineComponentArg.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());

        QCOMPARE(object->property("success"), u"Signal was called"_s);
    }
    {
        QQmlComponent component(&engine, testFileUrl("signalInlineComponentArg1.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());

        QCOMPARE(object->property("successFromOwnSignal"),
                 u"Own signal was called with component from another file"_s);
        QCOMPARE(object->property("successFromSignalFromFile"),
                 u"Signal was called from another file"_s);
    }
}

void tst_qqmllanguage::functionSignatureEnforcement()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, testFileUrl("signatureIgnored.qml"));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> ignored(c1.create());
    QCOMPARE(ignored->property("l").toInt(), 5);
    QCOMPARE(ignored->property("m").toInt(), 77);
    QCOMPARE(ignored->property("n").toInt(), 67);

    const QUrl url2 = testFileUrl("signatureEnforced.qml");
    QQmlComponent c2(&engine, url2);
    QVERIFY2(c2.isReady(), qPrintable(c2.errorString()));

    QTest::ignoreMessage(
            QtCriticalMsg,
            qPrintable(url2.toString() + u":36: 15 should be coerced to void because the function "
                                          "called is insufficiently annotated. The original value "
                                          "is retained. "
                                          "This will change in a future version of Qt."_s));

    QScopedPointer<QObject> enforced(c2.create());
    QCOMPARE(enforced->property("l").toInt(), 2); // strlen("no")
    QCOMPARE(enforced->property("m").toInt(), 77);
    QCOMPARE(enforced->property("n").toInt(), 99);
    QCOMPARE(enforced->property("o").toInt(), 77);
}

void tst_qqmllanguage::importPrecedence()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, testFileUrl("importPrecedenceGood.qml"));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));
    QScopedPointer<QObject> o1(c1.create());
    QVERIFY(!o1.isNull());
    QVERIFY(o1->property("theAgent").value<QObject *>() != nullptr);

    QUrl c2Url = testFileUrl("importPrecedenceBad.qml");
    QQmlComponent c2(&engine, c2Url);
    QVERIFY2(c2.isReady(), qPrintable(c2.errorString()));
    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(c2Url.toString() + u":11: ReferenceError: agent is not defined"_s));

    QScopedPointer<QObject> o2(c2.create());
    QVERIFY(!o2.isNull());
    QCOMPARE(o2->property("theAgent").value<QObject *>(), nullptr);
}

void tst_qqmllanguage::nullIsNull()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nullIsNull.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QVERIFY(o->property("someProperty").value<QObject*>() != nullptr);
    QTRY_COMPARE(o->property("someProperty").value<QObject*>(), nullptr);
}

void tst_qqmllanguage::multiRequired()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("multiRequired.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o.isNull());
    QCOMPARE(c.errorString(),
             qPrintable(url.toString() + ":5 Required property description was not initialized\n"));
}

// QTBUG-111088
void tst_qqmllanguage::isNullOrUndefined()
{
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("isNullOrUndefined_interpreter.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVariant result = o.data()->property("result");
        QVERIFY(result.isValid());
        QCOMPARE(result.toInt(), 3);
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("isNullOrUndefined_jit.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVariant result = o.data()->property("result");
        QVERIFY(result.isValid());
        QCOMPARE(result.toInt(), 150);
    }
}

void tst_qqmllanguage::objectAndGadgetMethodCallsRejectThisObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("objectAndGadgetMethodCallsRejectThisObject.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    const QList<int> lines =  { 15, 19, 21, 22, 24, 25, 28, 31 };
    for (int line : lines) {
        const QString message
                = ".*:%1: Calling C.. methods with 'this' objects different from the one "
                  "they were retrieved from is broken, due to historical reasons. The "
                  "original object is used as 'this' object. You can allow the given "
                  "'this' object to be used by setting "
                  "'pragma NativeMethodBehavior: AcceptThisObject'"_L1.arg(QString::number(line));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(message));
    }

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("badRect"), QRectF(1, 2, 3, 4));
    QCOMPARE(o->property("goodRect1"), QRectF(1, 2, 3, 4));
    QCOMPARE(o->property("goodRect2"), QRectF(1, 2, 3, 4));

    QCOMPARE(o->property("badString"), QStringLiteral("27"));
    QCOMPARE(o->property("goodString1"), QStringLiteral("27"));
    QCOMPARE(o->property("goodString2"), QStringLiteral("27"));
    QCOMPARE(o->property("goodString3"), QStringLiteral("27"));

    QVERIFY(o->property("goodString4").value<QString>().startsWith("QObject_QML_"_L1));
    QVERIFY(o->property("badString2").value<QString>().startsWith("QObject_QML_"_L1));

    QCOMPARE(o->property("badInt"), 5);
    QCOMPARE(o->property("goodInt1"), 5);
    QCOMPARE(o->property("goodInt2"), 5);
    QCOMPARE(o->property("goodInt3"), 5);
}

void tst_qqmllanguage::objectAndGadgetMethodCallsAcceptThisObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("objectAndGadgetMethodCallsAcceptThisObject.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    // Explicitly retrieve the metaobject for the Qt singleton so that the proxy data is created.
    // This way the inheritance analysis we do when figuring out what toString() means is somewhat
    // more interesting. Also, we get a deterministic result for Qt.toString().
    const QQmlType qtType = QQmlMetaType::qmlType(QStringLiteral("Qt"), QString(), QTypeRevision());
    QVERIFY(qtType.isValid());
    const QMetaObject *qtMeta = qtType.metaObject();
    QVERIFY(qtMeta);
    QCOMPARE(QString::fromUtf8(qtMeta->className()), QLatin1String("Qt"));

    QTest::ignoreMessage(
                QtWarningMsg, QRegularExpression(
                    "objectAndGadgetMethodCallsAcceptThisObject.qml:16: Error: "
                    "Cannot call method QtObject::rect on QObject_QML_"));
    QTest::ignoreMessage(
                QtWarningMsg, QRegularExpression(
                    "objectAndGadgetMethodCallsAcceptThisObject.qml:20: Error: "
                    "Cannot call method BaseValueType::report on QObject_QML_"));
    QTest::ignoreMessage(
                QtWarningMsg, QRegularExpression(
                    "objectAndGadgetMethodCallsAcceptThisObject.qml:26: Error: "
                    "Cannot call method toString on QRectF"));
    QTest::ignoreMessage(
                QtWarningMsg, QRegularExpression(
                    "objectAndGadgetMethodCallsAcceptThisObject.qml:29: Error: "
                    "Cannot call method OriginalSingleton::mm on QObject_QML_"));


    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("badRect"), QRectF());
    QCOMPARE(o->property("goodRect1"), QRectF(1, 2, 3, 4));
    QCOMPARE(o->property("goodRect2"), QRectF(1, 2, 3, 4));

    QCOMPARE(o->property("badString"), QString());
    QCOMPARE(o->property("goodString1"), QStringLiteral("27"));
    QCOMPARE(o->property("goodString2"), QStringLiteral("27"));
    QCOMPARE(o->property("goodString3"), QStringLiteral("28"));

    QVERIFY(o->property("goodString4").value<QString>().startsWith("Qt("_L1));
    QCOMPARE(o->property("badString2"), QString());

    QCOMPARE(o->property("badInt"), 0);
    QCOMPARE(o->property("goodInt1"), 5);
    QCOMPARE(o->property("goodInt2"), 5);
    QCOMPARE(o->property("goodInt3"), 5);
}

void tst_qqmllanguage::longConversion()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("longConversion.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    for (const char *prop : {
            "testProp",
            "testQProp",
            "fromLocal",
            "fromQLocal",
            "fromBoolean",
            "fromQBoolean"}) {
        const QVariant val = o->property(prop);
        QVERIFY(val.isValid());
        QCOMPARE(val.metaType(), QMetaType::fromType<bool>());
        QVERIFY(!val.toBool());
    }
}

void tst_qqmllanguage::enumPropsManyUnderylingTypes()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("enumPropsManyUnderlyingTypes.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    auto *enumObject = qobject_cast<EnumPropsManyUnderlyingTypes *>(o.get());
    QCOMPARE(enumObject->si8prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
    QCOMPARE(enumObject->ui8prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
    QCOMPARE(enumObject->si16prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
    QCOMPARE(enumObject->ui16prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
    QCOMPARE(enumObject->si64prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
    QCOMPARE(enumObject->ui64prop, EnumPropsManyUnderlyingTypes::ResolvedValue);
}

void tst_qqmllanguage::asValueType()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("asValueType.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString() + ":7:5: Unable to assign [undefined] to QRectF"_L1));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("a"), QVariant());
    QCOMPARE(o->property("b").value<QRectF>(), QRectF());
    QVERIFY(!o->property("c").toBool());

    const QRectF rect(1, 2, 3, 4);
    o->setProperty("a", QVariant(rect));
    QCOMPARE(o->property("b").value<QRectF>(), rect);
    QVERIFY(o->property("c").toBool());

    QVERIFY(!o->property("d").toBool());
    const QPointF point = o->property("e").value<QPointF>();
    QCOMPARE(point.x(), 10.0);
    QCOMPARE(point.y(), 20.0);

    const ValueTypeWithString withString = o->property("f").value<ValueTypeWithString>();
    QCOMPARE(withString.toString(), u"red");
}

void tst_qqmllanguage::typedEnums_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<double>("value");
    const QMetaObject *mo = &TypedEnums::staticMetaObject;
    for (int i = 0, end = mo->enumeratorCount(); i != end; ++i) {
        const QMetaEnum e = mo->enumerator(i);
        for (int k = 0, end = e.keyCount(); k != end; ++k) {
            QTest::addRow("%s::%s", e.name(), e.key(k))
                    << QString::fromLatin1(e.name()).toLower()
                    << double(e.value(k));
        }
    }
}
void tst_qqmllanguage::typedEnums()
{
    QFETCH(QString, property);
    QFETCH(double, value);
    QQmlEngine e;
    const QString qml = QLatin1String(R"(
        import QtQml
        import TypedEnums
        ObjectWithEnums {
            property real input: %2
            %1: input
            g.%1: input
            property real output1: %1
            property real output2: g.%1
        }
    )").arg(property).arg(value, 0, 'f');
    QQmlComponent c(&engine);
    c.setData(qml.toUtf8(), QUrl("enums.qml"_L1));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    // TODO: This silently fails for quint32, qint64 and quint64 because QMetaEnum cannot encode
    //       such values either. For the 64bit values we'll also need a better type than double
    //       inside QML.
    QEXPECT_FAIL("E32U::E32UD", "Not supported", Abort);
    QEXPECT_FAIL("E32U::E32UE", "Not supported", Abort);
    QEXPECT_FAIL("E64U::E64UE", "Not supported", Abort);

    QCOMPARE(o->property("output1").toDouble(), value);
    QCOMPARE(o->property("output2").toDouble(), value);
}

void tst_qqmllanguage::objectMethodClone()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("objectMethodClone.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QTRY_COMPARE(o->property("doneClicks").toInt(), 2);
}

void tst_qqmllanguage::unregisteredValueTypeConversion()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("unregisteredValueTypeConversion.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    UnregisteredValueTypeHandler *handler = qobject_cast<UnregisteredValueTypeHandler *>(o.data());
    Q_ASSERT(handler);
    QCOMPARE(handler->consumed, 2);
    QCOMPARE(handler->gadgeted, 1);
}

void tst_qqmllanguage::retainThis()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("retainThis.qml");
    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    const QString warning = u"Calling C++ methods with 'this' objects different "
                            "from the one they were retrieved from is broken, due to "
                            "historical reasons. The original object is used as 'this' "
                            "object. You can allow the given 'this' object to be used "
                            "by setting 'pragma NativeMethodBehavior: AcceptThisObject'"_s;

    // Both cases objA because we retain the thisObject.
    for (int i = 0; i < 2; ++i) {
        QTest::ignoreMessage(QtWarningMsg, qPrintable(url.toString() + u":12: "_s + warning));
        QTest::ignoreMessage(QtDebugMsg, "objA says hello");
        QTest::ignoreMessage(QtWarningMsg, qPrintable(url.toString() + u":13: "_s + warning));
        QTest::ignoreMessage(QtDebugMsg, "objA says 5 + 6 = 11");
    }

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url.toString() + u":22: "_s + warning));
    QTest::ignoreMessage(QtDebugMsg, "objA says hello");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(url.toString() + u":22: "_s + warning));
    QTest::ignoreMessage(QtDebugMsg, "objB says hello");

    QTest::ignoreMessage(QtDebugMsg, "objC says 7 + 7 = 14");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(url.toString() + u":32: "_s + warning));
    QTest::ignoreMessage(QtDebugMsg, "objB says 7 + 7 = 14");

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::variantObjectList()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("variantObjectList.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    BirthdayParty *party = o->property("q").value<BirthdayParty *>();
    QCOMPARE(party->guestCount(), 3);
    QCOMPARE(party->guest(0)->objectName(), "Leo Hodges");
    QCOMPARE(party->guest(1)->objectName(), "Jack Smith");
    QCOMPARE(party->guest(2)->objectName(), "Anne Brown");
}

void tst_qqmllanguage::jitExceptions()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("jitExceptions.qml");
    QQmlComponent c(&e, testFileUrl("jitExceptions.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString() + u":5: ReferenceError: control is not defined"_s));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::attachedInCtor()
{
    QQmlEngine e;
    QQmlComponent c(&e);
    c.setData(R"(
        import Test
        AttachedInCtor {}
    )", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    AttachedInCtor *a = qobject_cast<AttachedInCtor *>(o.data());
    QVERIFY(a->attached);
    QCOMPARE(a->attached, qmlAttachedPropertiesObject<AttachedInCtor>(a, false));
}

void tst_qqmllanguage::byteArrayConversion()
{
    QQmlEngine e;
    QQmlComponent c(&e);
    c.setData(R"(
        import Test
        import QtQml
        ByteArrayReceiver {
            Component.onCompleted: {
                byteArrayTest([1, 2, 3]);
                byteArrayTest(Array.from('456'));
            }
        }
    )", QUrl());

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    ByteArrayReceiver *receiver = qobject_cast<ByteArrayReceiver *>(o.data());
    QVERIFY(receiver);
    QCOMPARE(receiver->byteArrays.length(), 2);
    QCOMPARE(receiver->byteArrays[0], QByteArray("\1\2\3"));
    QCOMPARE(receiver->byteArrays[1], QByteArray("\4\5\6"));
}
void tst_qqmllanguage::propertySignalNames_data()
{
    QTest::addColumn<QString>("propertyName");
    QTest::addColumn<QString>("propertyChangedSignal");
    QTest::addColumn<QString>("propertyChangedHandler");
    QTest::addRow("helloWorld") << u"helloWorld"_s << u"helloWorldChanged"_s
                                << u"onHelloWorldChanged"_s;
    QTest::addRow("$helloWorld") << u"$helloWorld"_s << u"$helloWorldChanged"_s
                                 << u"on$HelloWorldChanged"_s;
    QTest::addRow("_helloWorld") << u"_helloWorld"_s << u"_helloWorldChanged"_s
                                 << u"on_HelloWorldChanged"_s;
    QTest::addRow("_") << u"_"_s << u"_Changed"_s << u"on_Changed"_s;
    QTest::addRow("$") << u"$"_s << u"$Changed"_s << u"on$Changed"_s;
    QTest::addRow("ä") << u"ä"_s << u"äChanged"_s << u"onÄChanged"_s;
    QTest::addRow("___123a") << u"___123a"_s << u"___123aChanged"_s << u"on___123AChanged"_s;
}
void tst_qqmllanguage::propertySignalNames()
{
    QFETCH(QString, propertyName);
    QFETCH(QString, propertyChangedSignal);
    QFETCH(QString, propertyChangedHandler);
    QQmlEngine e;
    QQmlComponent c(&e);
    c.setData(uR"(
import QtQuick
Item {
    property int %1: 456
    property bool success: false
    function f() { %1 = 123; }
    function g() { %2(); }
    %3: success = true
})"_s.arg(propertyName, propertyChangedSignal, propertyChangedHandler)
                      .toUtf8(),
              QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o != nullptr);
    const QMetaObject *metaObject = o->metaObject();
    auto signalIndex =
            metaObject->indexOfSignal(propertyChangedSignal.append("()").toStdString().c_str());
    QVERIFY(signalIndex > -1);
    auto signal = metaObject->method(signalIndex);
    QVERIFY(signal.isValid());
    QSignalSpy changeSignal(o.data(), signal);
    QMetaObject::invokeMethod(o.data(), "f");
    QCOMPARE(o->property(propertyName.toStdString().c_str()), 123);
    QVERIFY(changeSignal.size() == 1);
    QCOMPARE(o->property("success"), true);
    QMetaObject::invokeMethod(o.data(), "g");
    QVERIFY(changeSignal.size() == 2);
}
void tst_qqmllanguage::signalNames_data()
{
    QTest::addColumn<QString>("signalName");
    QTest::addColumn<QString>("handlerName");
    QTest::addRow("helloWorld") << u"helloWorld"_s << u"onHelloWorld"_s;
    QTest::addRow("$helloWorld") << u"$helloWorld"_s << u"on$HelloWorld"_s;
    QTest::addRow("_helloWorld") << u"_helloWorld"_s << u"on_HelloWorld"_s;
    QTest::addRow("_") << u"_"_s << u"on_"_s;
    QTest::addRow("aUmlaut") << u"ä"_s << u"onÄ"_s;
    QTest::addRow("___123a") << u"___123a"_s << u"on___123A"_s;
}
void tst_qqmllanguage::signalNames()
{
    QFETCH(QString, signalName);
    QFETCH(QString, handlerName);
    QQmlEngine e;
    QQmlComponent c(&e);
    c.setData(uR"(
import QtQuick
Item {
    signal %1()
    property bool success: false
    function f() { %1(); }
    %2: success = true
})"_s.arg(signalName, handlerName)
                      .toUtf8(),
              QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o != nullptr);
    const QMetaObject *metaObject = o->metaObject();
    auto signalIndex = metaObject->indexOfSignal(signalName.append("()").toStdString().c_str());
    QVERIFY(signalIndex > -1);
    auto signal = metaObject->method(signalIndex);
    QVERIFY(signal.isValid());
    QSignalSpy changeSignal(o.data(), signal);
    signal.invoke(o.data());
    QVERIFY(changeSignal.size() == 1);
    QCOMPARE(o->property("success"), true);
    QMetaObject::invokeMethod(o.data(), "f");
    QVERIFY(changeSignal.size() == 2);
}

void tst_qqmllanguage::callMethodOfAttachedDerived()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import QtQml
        import Test

        QtObject {
            Component.onCompleted: Counter.increase()
            property int v: Counter.value
        }
    )", QUrl());

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("v").toInt(), 99);
}

void tst_qqmllanguage::multiVersionSingletons()
{
    qmlRegisterTypesAndRevisions<BareSingleton>("MultiVersionSingletons", 11);
    qmlRegisterTypesAndRevisions<UncreatableSingleton>("MultiVersionSingletons", 11);
    QQmlEngine engine;

    for (const char *name : { "BareSingleton", "UncreatableSingleton"}) {
        const int id1 = qmlTypeId("MultiVersionSingletons", 1, 0, name);
        const int id2 = qmlTypeId("MultiVersionSingletons", 11, 0, name);
        QVERIFY(id1 != id2);
        const QJSValue value1 = engine.singletonInstance<QJSValue>(id1);
        const QJSValue value2 = engine.singletonInstance<QJSValue>(id2);
        QVERIFY(value1.strictlyEquals(value2));
    }
}

void tst_qqmllanguage::typeAnnotationCycle()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("TypeAnnotationCycle1.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("b").value<QObject*>(), o.data());
}

void tst_qqmllanguage::corpseInQmlList()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("corpseInQmlList.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QScopedPointer<QObject> a(new QObject);
    QMetaObject::invokeMethod(o.data(), "setB", Q_ARG(QObject *, a.data()));

    QJSValue b = o->property("b").value<QJSValue>();
    QQmlListProperty<QObject> list
            = qjsvalue_cast<QQmlListProperty<QObject>>(b.property(QStringLiteral("b")));

    QCOMPARE(list.count(&list), 1);
    QCOMPARE(list.at(&list, 0), a.data());

    a.reset();

    b = o->property("b").value<QJSValue>();
    list = qjsvalue_cast<QQmlListProperty<QObject>>(b.property(QStringLiteral("b")));

    QCOMPARE(list.count(&list), 1);
    QCOMPARE(list.at(&list, 0), nullptr);

    // The list itself is still alive:

    list.append(&list, o.data());
    QCOMPARE(list.count(&list), 2);
    QCOMPARE(list.at(&list, 0), nullptr);
    QCOMPARE(list.at(&list, 1), o.data());

    list.replace(&list, 0, o.data());
    QCOMPARE(list.count(&list), 2);
    QCOMPARE(list.at(&list, 0), o.data());
    QCOMPARE(list.at(&list, 1), o.data());

    list.removeLast(&list);
    QCOMPARE(list.count(&list), 1);
    QCOMPARE(list.at(&list, 0), o.data());

    list.clear(&list);
    QCOMPARE(list.count(&list), 0);
}

void tst_qqmllanguage::objectInQmlListAndGc()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("objectInList.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    // Process the deletion event
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QQmlListProperty<QObject> children = o->property("child").value<QQmlListProperty<QObject>>();
    QCOMPARE(children.count(&children), 1);
    QObject *child = children.at(&children, 0);
    QVERIFY(child);
    QCOMPARE(child->objectName(), QLatin1String("child"));
}

void tst_qqmllanguage::asCastToInlineComponent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("asCastToInlineComponent.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), QLatin1String("value: 20"));
}

void tst_qqmllanguage::deepAliasOnICOrReadonly()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("deepAliasOnICUser.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("borderColor").toString(), QLatin1String("black"));
    QCOMPARE(o->property("borderObjectName").toString(), QLatin1String("theLeaf"));

    const QVariant var = o->property("borderVarvar");
    QCOMPARE(var.metaType(), QMetaType::fromType<QString>());
    QCOMPARE(var.toString(), QLatin1String("mauve"));

    QQmlComponent c2(&engine, testFileUrl("deepAliasOnReadonly.qml"));
    QVERIFY(c2.isError());
    QVERIFY(c2.errorString().contains(
            QLatin1String(
                    "Invalid property assignment: \"readonlyRectX\" is a read-only property")));
}

void tst_qqmllanguage::optionalChainCallOnNullProperty()
{
    QTest::failOnWarning(QRegularExpression(".*Cannot call method 'destroy' of null.*"));

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("optionalChainCallOnNullProperty.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_qqmllanguage::ambiguousComponents()
{
    auto e1 = std::make_unique<QQmlEngine>();
    e1->addImportPath(dataDirectory());
    bool isInstanceOf = false;

    {
        QQmlComponent c(e1.get());
        c.loadUrl(testFileUrl("ambiguousComponents.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));

        QScopedPointer<QObject> o(c.create());
        QTest::ignoreMessage(QtDebugMsg, "do");
        QMetaObject::invokeMethod(o.data(), "dodo");

        QMetaObject::invokeMethod(o.data(), "testInstanceOf", Q_RETURN_ARG(bool, isInstanceOf));
        QVERIFY(isInstanceOf);
    }

    QQmlEngine e2;
    e2.addImportPath(dataDirectory());
    QQmlComponent c2(&e2);
    c2.loadUrl(testFileUrl("ambiguousComponents.qml"));
    QVERIFY2(c2.isReady(), qPrintable(c2.errorString()));

    QScopedPointer<QObject> o2(c2.create());
    QTest::ignoreMessage(QtDebugMsg, "do");
    QMetaObject::invokeMethod(o2.data(), "dodo");

    isInstanceOf = false;
    QMetaObject::invokeMethod(o2.data(), "testInstanceOf", Q_RETURN_ARG(bool, isInstanceOf));
    QVERIFY(isInstanceOf);

    e1.reset();

    // We can still invoke the function. This means its CU belongs to e2.
    QTest::ignoreMessage(QtDebugMsg, "do");
    QMetaObject::invokeMethod(o2.data(), "dodo");

    isInstanceOf = false;
    QMetaObject::invokeMethod(o2.data(), "testInstanceOf", Q_RETURN_ARG(bool, isInstanceOf));
    QVERIFY(isInstanceOf);
}

void tst_qqmllanguage::writeNumberToEnumAlias()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("aliasWriter.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("strokeStyle").toInt(), 1);
}

void tst_qqmllanguage::badInlineComponentAnnotation()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("badICAnnotation.qml");
    QQmlComponent c(&engine, testFileUrl("badICAnnotation.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
                QtCriticalMsg,
                qPrintable(url.toString() + ":20: 5 should be coerced to void because the function "
                                            "called is insufficiently annotated. The original "
                                            "value is retained. This will change in a future "
                                            "version of Qt."));
    QTest::ignoreMessage(
                QtCriticalMsg,
                QRegularExpression(":22: IC\\([^\\)]+\\) should be coerced to void because the "
                                   "function called is insufficiently annotated. The original "
                                   "value is retained. This will change in a future version of "
                                   "Qt\\."));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("a").toInt(), 5);

    QObject *ic = o->property("ic").value<QObject *>();
    QVERIFY(ic);

    QCOMPARE(o->property("b").value<QObject *>(), ic);
    QCOMPARE(o->property("c").value<QObject *>(), ic);
    QCOMPARE(o->property("d").value<QObject *>(), nullptr);
}

void tst_qqmllanguage::manuallyCallSignalHandler()
{
    // TODO: This test verifies the absence of regression legacy behavior. See QTBUG-120573
    //       Once we can get rid of the legacy behavior, delete this test!

    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("manuallyCallSignalHandler.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    for (int i = 0; i < 10; ++i) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
            "Property 'onDestruction' of object QQmlComponentAttached\\(0x[0-9a-f]+\\) is a signal "
            "handler\\. You should not call it directly\\. Make it a proper function and call that "
            "or emit the signal\\."));
        QTest::ignoreMessage(QtDebugMsg, "evil!");
        QScopedPointer<QObject> o(c.create());
        QTest::ignoreMessage(QtDebugMsg, "evil!");
    }
}

void tst_qqmllanguage::overrideDefaultProperty()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("overrideDefaultProperty.qml");

    // Should not crash here!

    QQmlComponent c(&e, url);
    QVERIFY(c.isError());
    QCOMPARE(c.errorString(),
             url.toString() + QLatin1String(":5 Cannot assign object to list property \"data\"\n"));
}

void tst_qqmllanguage::enumScopes()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("enumScopes.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("singletonUnscoped"), false);
    QCOMPARE(o->property("singletonScoped"), true);
    QCOMPARE(o->property("nonSingletonUnscoped"), false);
    QCOMPARE(o->property("nonSingletonScoped"), true);

    QCOMPARE(o->property("singletonScopedValue").toInt(), int(EnumProviderSingleton::Expected::Value));
    QCOMPARE(o->property("singletonUnscopedValue").toInt(), int(EnumProviderSingleton::Expected::Value));
}

void tst_qqmllanguage::typedObjectList()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("typedObjectList.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QJSValue b = o->property("b").value<QJSValue>();
    auto list = qjsvalue_cast<QQmlListProperty<QQmlComponent>>(b.property(QStringLiteral("b")));

    QCOMPARE(list.count(&list), 1);
    QVERIFY(list.at(&list, 0) != nullptr);
}

void tst_qqmllanguage::nestedVectors()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("nestedVectors.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    NestedVectors *n = qobject_cast<NestedVectors *>(o.data());
    QVERIFY(n);

    const std::vector<std::vector<int>> expected1 { { 1, 2, 3 }, { 4, 5 } };
    const QVariant list1 = n->property("list1");
    QCOMPARE(list1.metaType(), QMetaType::fromType<std::vector<std::vector<int>>>());
    QCOMPARE(list1.value<std::vector<std::vector<int>>>(), expected1);

    const std::vector<std::vector<int>> expected2 { { 2, 3, 4 }, { 5, 6 } };
    QCOMPARE(n->getList(), expected2);
}

void tst_qqmllanguage::overrideInnerBinding()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("BindingOverrider.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("width").toReal(), 20.0);
    QCOMPARE(o->property("innerWidth").toReal(), 20.0);
}

QTEST_MAIN(tst_qqmllanguage)

#include "tst_qqmllanguage.moc"
