// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "moc.h"

#include <QDir>
#include <QMetaObject>
#include <QMetaProperty>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

QByteArray setterName(const QByteArray &propertyName)
{
    QByteArray setter(propertyName);
    if (isupper(setter.at(0))) {
        setter = "Set" + setter;
    } else {
        setter[0] = QtMiscUtils::toAsciiUpper(setter[0]);
        setter = "set" + setter;
    }
    return setter;
}

void formatCppEnum(QTextStream &str, const QMetaEnum &metaEnum)
{
    str << "    enum " << metaEnum.name() << " {" << Qt::endl;
    for (int k = 0, last = metaEnum.keyCount() - 1; k <= last; ++k) {
        QByteArray key(metaEnum.key(k));
        str << "        " << key.leftJustified(24) << "= " << metaEnum.value(k);
        if (k < last)
            str << ',';
        str << Qt::endl;
    }
    str << "    };" << Qt::endl;
}

void formatCppEnums(QTextStream &str, const QMetaObject *mo,
                 const char *qEnumDecl = nullptr /* Q_ENUM, Q_ENUM_NS */)
{
    const int offset = mo->enumeratorOffset();
    const int count = mo->enumeratorCount();
    for (int e = offset; e < count; ++e) {
        const auto me = mo->enumerator(e);
        formatCppEnum(str, me);
        if (qEnumDecl)
            str << "    " << qEnumDecl << '(' << me.name() << ")\n";
        str << '\n';
    }
    if (offset < count)
        str << '\n';
}

static void formatCppMethods(QTextStream &str, const QMetaObject *mo,
                             QMetaMethod::MethodType filter)
{
    for (int m = mo->methodOffset(), count = mo->methodCount(); m < count; ++m) {
        const auto &mt = mo->method(m);
        if (mt.methodType() == filter)
            str << "    " << mt.typeName() << ' ' << mt.methodSignature() << ";\n";
    }
}

static void formatCppProperty(QTextStream &str, const QMetaProperty &p)
{
    str << "    Q_PROPERTY(" << p.typeName() << ' ' << p.name()
        << " READ " << p.name();
    if (p.isWritable())
        str << " WRITE " << setterName(p.name());
    if (p.hasNotifySignal())
        str << " NOTIFY " << p.notifySignal().name();
    if (p.isUser())
         str << " USER true";
    if (!p.isDesignable())
        str << " DESIGNABLE false";
    if (!p.isStored())
        str << " STORED false";
    if (p.isFinal())
        str << " FINAL";
    str << ")\n";
}

static void formatCppQuotedString(QTextStream &str, const char *s)
{
    str << '"';
    for ( ; *s ; ++s) {
        const char c = *s;
        if (c == '\\' || c == '\"')
            str << '\\';
        str << c;
    }
    str << '"';
}

// Generate C++ code from an ActiveQt QMetaObject to be parsed by moc
static QString mocHeader(const QMetaObject *mo, const QStringList &name,
                         const QString &baseClass)
{
    QString result;
    QTextStream str(&result);

    str << "#pragma once\n\n";
    if (!baseClass.isEmpty())
        str << "#include <" << baseClass << ">\n";
    str  << "#include <qt_windows.h>\n\n";

    for (int n = 0, count = name.size() - 1; n < count; ++n)
        str << "namespace " << name.at(n) << " {\n";

    str << "\nclass " << name.constLast();
    if (!baseClass.isEmpty())
        str << " : public " << baseClass;
    str<< "\n{\n    Q_OBJECT\n";

    for (int i = mo->classInfoOffset(), count = mo->classInfoCount(); i < count; ++i) {
        const auto &info = mo->classInfo(i);
        str << "    Q_CLASSINFO(";
        formatCppQuotedString(str, info.name());
        str << ", ";
        formatCppQuotedString(str, info.value());
        str << ")\n";
    }

    for (int p = mo->propertyOffset(), count = mo-> propertyCount(); p < count; ++p)
        formatCppProperty(str, mo->property(p));

    str << "public:\n";

    formatCppEnums(str, mo, "Q_ENUM");

    formatCppMethods(str, mo, QMetaMethod::Constructor);
    str << "\nQ_SIGNALS:\n";
    formatCppMethods(str, mo, QMetaMethod::Signal);
    str << "\npublic Q_SLOTS:\n";
    formatCppMethods(str, mo, QMetaMethod::Slot);
    str << "};\n";

    for (int n = name.size() - 1; n >= 0 ; --n)
        str << "} // namespace " << name.at(n) << '\n';

    return result;
}

static QString processOutput(QByteArray output)
{
    for (int c = output.size() - 1; c >= 0; --c) {
        if (output.at(c) == '\r')
            output.remove(c, 1);
    }
    return QString::fromUtf8(output);
}

static QString runProcess(const QString  &binary, const QStringList &args,
                          QString *errorString)
{
    QProcess process;
    process.start(binary, args);
    if (!process.waitForStarted()) {
        *errorString = QLatin1String("Cannot start ") + binary + QLatin1String(": ") + process.errorString();
        return QString();
    }
    if (!process.waitForFinished()) {
        *errorString = binary + QLatin1String(" timed out: ") + process.errorString();
        return QString();
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorString = binary + QLatin1String(" crashed: ") + process.errorString();
        return QString();
    }
    if (process.exitCode() != 0) {
        *errorString = binary + QLatin1String(" failed: ") + processOutput(process.readAllStandardError());
        return QString();
    }
    return processOutput(process.readAllStandardOutput());
}

static int lineStart(int pos, const QString *s)
{
    const int lineStart = s->lastIndexOf(QLatin1Char('\n'), pos);
    return lineStart >= 0 ? lineStart + 1 : 0;
}

static int nextLineFeed(int pos, const QString *s)
{
    const int nextLineStart = s->indexOf(QLatin1Char('\n'), pos);
    return nextLineStart >= 0 ? nextLineStart : s->size();
}

static void removeLines(const QString &start, const QString &end,
                        QString *s, bool keepEnd = false)
{
    int startPos = s->indexOf(start);
    if (startPos < 0)
        return;
    int endPos = s->indexOf(end, startPos + start.size());
    if (endPos < 0)
        return;

    startPos = lineStart(startPos, s);
    endPos = keepEnd
        ? lineStart(endPos, s)
        : nextLineFeed(endPos + end.size(), s);
    s->remove(startPos, endPos - startPos);
}

static QString cleanCode(QString code, const QString &className, const QString &headerFileName)
{
    // remove include of temp file
    code.remove(QLatin1String("#include \"") + headerFileName + QLatin1String("\"\n"));

    const char *removeFunctions[] = {"metaObject", "qt_metacall", "qt_static_metacall"};

    const QString funcStart = className + QLatin1String("::");
    const QString nextFuncStart = QLatin1String("\n}");
    for (auto function :  removeFunctions)
        removeLines(funcStart + QLatin1String(function) + QLatin1Char('('), nextFuncStart, &code);

    // qt_static_metacall is not implemented, cannot access private function of QAxObject
    code.replace(QLatin1String("    qt_static_metacall,"), QLatin1String("    nullptr,"));

    // Remove internal signals
    removeLines(QLatin1String("// SIGNAL 0"), QLatin1String("QT_WARNING_POP"), &code, true);

    // Fix enum uint(Namespace::Class::Value) -> uint(Namespace::Value) (dumpcpp convention)
    const QString enumPrefix = QLatin1String("uint(");
    QString parentName = className;
    const int lastSep = parentName.lastIndexOf(QLatin1String("::"));
    if (lastSep >= 0)
        parentName.truncate(lastSep);
    else
        parentName.clear();
    code.replace(enumPrefix + className + QLatin1String("::"),
                 enumPrefix + parentName + QLatin1String("::"));
    return code;
}

QString mocCode(const QMetaObject *mo, const QString &qualifiedClassName,
                QString *errorString)
{
    QStringList name = qualifiedClassName.split(QLatin1String("::"));
    if (name.isEmpty())
        name.append(QLatin1String(mo->className()));

    const QString baseClass = QLatin1String(mo->superClass()->className());

    const QString tempPattern = QDir::tempPath() + QLatin1Char('/')
        + name.constLast().toLower() + QLatin1String("_XXXXXX.h");
    QTemporaryFile header(tempPattern);
    if (!header.open()) {
        *errorString = QLatin1String("Cannot open temporary file: ") + header.errorString();
        return QString();
    }
    const QString headerCode = mocHeader(mo, name, baseClass);
    header.write(headerCode.toUtf8());
    const QString headerFileName = header.fileName();
    header.close();

    const QString binary = QLatin1String("moc.exe");

    QString result = runProcess(binary, {header.fileName()}, errorString);
    if (result.isEmpty()) {
        errorString->append(QLatin1String("\n\nOffending code:\n"));
        errorString->append(headerCode);
        return result;
    }

    return cleanCode(result, name.join(QLatin1String("::")), headerFileName);
}

QT_END_NAMESPACE
