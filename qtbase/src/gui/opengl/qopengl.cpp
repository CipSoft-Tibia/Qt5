// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopengl.h"
#include "qopengl_p.h"

#include "qopenglcontext.h"
#include "qopenglfunctions.h"
#include "qoperatingsystemversion.h"
#include "qoffscreensurface.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if defined(QT_OPENGL_3)
typedef const GLubyte * (QOPENGLF_APIENTRYP qt_glGetStringi)(GLenum, GLuint);
#endif

#ifndef GL_CONTEXT_LOST
#define GL_CONTEXT_LOST                   0x0507
#endif

QOpenGLExtensionMatcher::QOpenGLExtensionMatcher()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLExtensionMatcher::QOpenGLExtensionMatcher: No context");
        return;
    }
    QOpenGLFunctions *funcs = ctx->functions();
    const char *extensionStr = nullptr;

    if (ctx->isOpenGLES() || ctx->format().majorVersion() < 3)
        extensionStr = reinterpret_cast<const char *>(funcs->glGetString(GL_EXTENSIONS));

    if (extensionStr) {
        QByteArray ba(extensionStr);
        QList<QByteArray> extensions = ba.split(' ');
        m_extensions = QSet<QByteArray>(extensions.constBegin(), extensions.constEnd());
    } else {
#ifdef QT_OPENGL_3
        // clear error state
        while (true) { // Clear error state.
            GLenum error = funcs->glGetError();
            if (error == GL_NO_ERROR)
                break;
            if (error == GL_CONTEXT_LOST)
                return;
        };
        qt_glGetStringi glGetStringi = (qt_glGetStringi)ctx->getProcAddress("glGetStringi");

        if (!glGetStringi)
            return;

        GLint numExtensions = 0;
        funcs->glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (int i = 0; i < numExtensions; ++i) {
            const char *str = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
            m_extensions.insert(str);
        }
#endif // QT_OPENGL_3
    }
}

/* Helpers to read out the list of features matching a device from
 * a Chromium driver bug list. Note that not all keys are supported and
 * some may behave differently: gl_vendor is a substring match instead of regex.
 {
  "entries": [
 {
      "id": 20,
      "description": "Disable EXT_draw_buffers on GeForce GT 650M on Linux due to driver bugs",
      "os": {
        "type": "linux"
      },
      // Optional: "exceptions" list
      "vendor_id": "0x10de",
      "device_id": ["0x0fd5"],
      "multi_gpu_category": "any",
      "features": [
        "disable_ext_draw_buffers"
      ]
    },
   ....
   }
*/

QDebug operator<<(QDebug d, const QOpenGLConfig::Gpu &g)
{
    QDebugStateSaver s(d);
    d.nospace();
    d << "Gpu(";
    if (g.isValid()) {
        d << "vendor=" << Qt::hex << Qt::showbase <<g.vendorId << ", device=" << g.deviceId
          << "version=" << g.driverVersion;
    } else {
        d << 0;
    }
    d << ')';
    return d;
}

typedef QJsonArray::ConstIterator JsonArrayConstIt;

static inline bool contains(const QJsonArray &haystack, unsigned needle)
{
    for (JsonArrayConstIt it = haystack.constBegin(), cend = haystack.constEnd(); it != cend; ++it) {
        if (needle == it->toString().toUInt(nullptr, /* base */ 0))
            return true;
    }
    return false;
}

static inline bool contains(const QJsonArray &haystack, const QString &needle)
{
    for (JsonArrayConstIt it = haystack.constBegin(), cend = haystack.constEnd(); it != cend; ++it) {
        if (needle == it->toString())
            return true;
    }
    return false;
}

namespace {
enum Operator { NotEqual, LessThan, LessEqualThan, Equals, GreaterThan, GreaterEqualThan };
static const char operators[][3] = {"!=", "<", "<=", "=", ">", ">="};

// VersionTerm describing a version term consisting of number and operator
// found in os.version and driver_version.
struct VersionTerm {
    VersionTerm() : op(NotEqual) {}
    static VersionTerm fromJson(const QJsonValue &v);
    bool isNull() const { return number.isNull(); }
    bool matches(const QVersionNumber &other) const;

    QVersionNumber number;
    Operator op;
};

bool VersionTerm::matches(const QVersionNumber &other) const
{
    if (isNull() || other.isNull()) {
        qWarning("called with invalid parameters");
        return false;
    }
    switch (op) {
    case NotEqual:
        return other != number;
    case LessThan:
        return other < number;
    case LessEqualThan:
        return other <= number;
    case Equals:
        return other == number;
    case GreaterThan:
        return other > number;
    case GreaterEqualThan:
        return other >= number;
    }
    return false;
}

VersionTerm VersionTerm::fromJson(const QJsonValue &v)
{
    VersionTerm result;
    if (!v.isObject())
        return result;
    const QJsonObject o = v.toObject();
    result.number = QVersionNumber::fromString(o.value("value"_L1).toString());
    const QString opS = o.value("op"_L1).toString();
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); ++i) {
        if (opS == QLatin1StringView(operators[i])) {
            result.op = static_cast<Operator>(i);
            break;
        }
    }
    return result;
}

// OS term consisting of name and optional version found in
// under "os" in main array and in "exceptions" lists.
struct OsTypeTerm
{
    static OsTypeTerm fromJson(const QJsonValue &v);
    static QString hostOs();
    static QVersionNumber hostKernelVersion() { return QVersionNumber::fromString(QSysInfo::kernelVersion()); }
    static QString hostOsRelease() {
#ifdef Q_OS_WIN
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11)
            return u"11"_s;
        return u"10"_s;
#else
        return {};
#endif
    }

    bool isNull() const { return type.isEmpty(); }
    bool matches(const QString &osName, const QVersionNumber &kernelVersion, const QString &osRelease) const
    {
        if (isNull() || osName.isEmpty() || kernelVersion.isNull()) {
            qWarning("called with invalid parameters");
            return false;
        }
        if (type != osName)
            return false;
        if (!versionTerm.isNull() && !versionTerm.matches(kernelVersion))
            return false;
        // release is a list of Windows versions where the rule should match
        if (!release.isEmpty() && !contains(release, osRelease))
            return false;
        return true;
    }

    QString type;
    VersionTerm versionTerm;
    QJsonArray release;
};

OsTypeTerm OsTypeTerm::fromJson(const QJsonValue &v)
{
    OsTypeTerm result;
    if (!v.isObject())
        return result;
    const QJsonObject o = v.toObject();
    result.type = o.value("type"_L1).toString();
    result.versionTerm = VersionTerm::fromJson(o.value("version"_L1));
    result.release = o.value("release"_L1).toArray();
    return result;
}

QString OsTypeTerm::hostOs()
{
    // Determine Host OS.
#if defined(Q_OS_WIN)
    return  QStringLiteral("win");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#elif defined(Q_OS_MACOS)
    return  QStringLiteral("macosx");
#elif defined(Q_OS_ANDROID)
    return  QStringLiteral("android");
#else
    return QString();
#endif
}
} // anonymous namespace

static QString msgSyntaxWarning(const QJsonObject &object, const QString &what)
{
    QString result;
    QTextStream(&result) << "Id " << object.value("id"_L1).toInt()
        << " (\"" << object.value("description"_L1).toString()
        << "\"): " << what;
    return result;
}

// Check whether an entry matches. Called recursively for
// "exceptions" list.

static bool matches(const QJsonObject &object,
                    const QString &osName,
                    const QVersionNumber &kernelVersion,
                    const QString &osRelease,
                    const QOpenGLConfig::Gpu &gpu)
{
    const OsTypeTerm os = OsTypeTerm::fromJson(object.value("os"_L1));
    if (!os.isNull() && !os.matches(osName, kernelVersion, osRelease))
        return false;

    const QJsonValue exceptionsV = object.value("exceptions"_L1);
    if (exceptionsV.isArray()) {
        const QJsonArray exceptionsA = exceptionsV.toArray();
        for (JsonArrayConstIt it = exceptionsA.constBegin(), cend = exceptionsA.constEnd(); it != cend; ++it) {
            if (matches(it->toObject(), osName, kernelVersion, osRelease, gpu))
                return false;
        }
    }

    const QJsonValue vendorV = object.value("vendor_id"_L1);
    if (vendorV.isString()) {
        if (gpu.vendorId != vendorV.toString().toUInt(nullptr, /* base */ 0))
            return false;
    } else {
        if (object.contains("gl_vendor"_L1)) {
            const QByteArray glVendorV = object.value("gl_vendor"_L1).toString().toUtf8();
            if (!gpu.glVendor.contains(glVendorV))
                return false;
        }
    }

    if (gpu.deviceId) {
        const QJsonValue deviceIdV = object.value("device_id"_L1);
        switch (deviceIdV.type()) {
        case QJsonValue::Array:
            if (!contains(deviceIdV.toArray(), gpu.deviceId))
                return false;
            break;
        case QJsonValue::Undefined:
        case QJsonValue::Null:
            break;
        default:
            qWarning().noquote()
                << msgSyntaxWarning(object, "Device ID must be of type array."_L1);
        }
    }
    if (!gpu.driverVersion.isNull()) {
        const QJsonValue driverVersionV = object.value("driver_version"_L1);
        switch (driverVersionV.type()) {
        case QJsonValue::Object:
            if (!VersionTerm::fromJson(driverVersionV).matches(gpu.driverVersion))
                return false;
            break;
        case QJsonValue::Undefined:
        case QJsonValue::Null:
            break;
        default:
            qWarning().noquote()
                << msgSyntaxWarning(object, "Driver version must be of type object."_L1);
        }
    }

    if (!gpu.driverDescription.isEmpty()) {
        const QJsonValue driverDescriptionV = object.value("driver_description"_L1);
        if (driverDescriptionV.isString()) {
            if (!gpu.driverDescription.contains(driverDescriptionV.toString().toUtf8()))
                return false;
        }
    }

    return true;
}

static bool readGpuFeatures(const QOpenGLConfig::Gpu &gpu,
                            const QString &osName,
                            const QVersionNumber &kernelVersion,
                            const QString &osRelease,
                            const QJsonDocument &doc,
                            QSet<QString> *result,
                            QString *errorMessage)
{
    result->clear();
    errorMessage->clear();
    const QJsonValue entriesV = doc.object().value("entries"_L1);
    if (!entriesV.isArray()) {
        *errorMessage = "No entries read."_L1;
        return false;
    }

    const QJsonArray entriesA = entriesV.toArray();
    for (JsonArrayConstIt eit = entriesA.constBegin(), ecend = entriesA.constEnd(); eit != ecend; ++eit) {
        if (eit->isObject()) {
            const QJsonObject object = eit->toObject();
            if (matches(object, osName, kernelVersion, osRelease, gpu)) {
                const QJsonValue featuresListV = object.value("features"_L1);
                if (featuresListV.isArray()) {
                    const QJsonArray featuresListA = featuresListV.toArray();
                    for (JsonArrayConstIt fit = featuresListA.constBegin(), fcend = featuresListA.constEnd(); fit != fcend; ++fit)
                        result->insert(fit->toString());
                }
            }
        }
    }
    return true;
}

static bool readGpuFeatures(const QOpenGLConfig::Gpu &gpu,
                            const QString &osName,
                            const QVersionNumber &kernelVersion,
                            const QString &osRelease,
                            const QByteArray &jsonAsciiData,
                            QSet<QString> *result, QString *errorMessage)
{
    result->clear();
    errorMessage->clear();
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(jsonAsciiData, &error);
    if (document.isNull()) {
        const qsizetype lineNumber =
                QByteArrayView(jsonAsciiData).left(error.offset).count('\n') + 1;
        QTextStream str(errorMessage);
        str << "Failed to parse data: \"" << error.errorString()
            << "\" at line " << lineNumber << " (offset: "
            << error.offset << ").";
        return false;
    }
    return readGpuFeatures(gpu, osName, kernelVersion, osRelease, document, result, errorMessage);
}

static bool readGpuFeatures(const QOpenGLConfig::Gpu &gpu,
                            const QString &osName,
                            const QVersionNumber &kernelVersion,
                            const QString &osRelease,
                            const QString &fileName,
                            QSet<QString> *result, QString *errorMessage)
{
    result->clear();
    errorMessage->clear();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QTextStream str(errorMessage);
        str << "Cannot open \"" << QDir::toNativeSeparators(fileName) << "\": "
            << file.errorString();
        return false;
    }
    const bool success = readGpuFeatures(gpu, osName, kernelVersion, osRelease, file.readAll(), result, errorMessage);
    if (!success) {
        errorMessage->prepend("Error reading \""_L1
                              + QDir::toNativeSeparators(fileName)
                              + "\": "_L1);
    }
    return success;
}

QSet<QString> QOpenGLConfig::gpuFeatures(const QOpenGLConfig::Gpu &gpu,
                                         const QString &osName,
                                         const QVersionNumber &kernelVersion,
                                         const QString &osRelease,
                                         const QJsonDocument &doc)
{
    QSet<QString> result;
    QString errorMessage;
    if (!readGpuFeatures(gpu, osName, kernelVersion, osRelease, doc, &result, &errorMessage))
        qWarning().noquote() << errorMessage;
    return result;
}

QSet<QString> QOpenGLConfig::gpuFeatures(const QOpenGLConfig::Gpu &gpu,
                                         const QString &osName,
                                         const QVersionNumber &kernelVersion,
                                         const QString &osRelease,
                                         const QString &fileName)
{
    QSet<QString> result;
    QString errorMessage;
    if (!readGpuFeatures(gpu, osName, kernelVersion, osRelease, fileName, &result, &errorMessage))
        qWarning().noquote() << errorMessage;
    return result;
}

QSet<QString> QOpenGLConfig::gpuFeatures(const Gpu &gpu, const QJsonDocument &doc)
{
    return gpuFeatures(gpu, OsTypeTerm::hostOs(), OsTypeTerm::hostKernelVersion(), OsTypeTerm::hostOsRelease(), doc);
}

QSet<QString> QOpenGLConfig::gpuFeatures(const Gpu &gpu, const QString &fileName)
{
    return gpuFeatures(gpu, OsTypeTerm::hostOs(), OsTypeTerm::hostKernelVersion(), OsTypeTerm::hostOsRelease(), fileName);
}

QOpenGLConfig::Gpu QOpenGLConfig::Gpu::fromContext()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QScopedPointer<QOpenGLContext> tmpContext;
    QScopedPointer<QOffscreenSurface> tmpSurface;
    if (!ctx) {
        tmpContext.reset(new QOpenGLContext);
        if (!tmpContext->create()) {
            qWarning("QOpenGLConfig::Gpu::fromContext: Failed to create temporary context");
            return QOpenGLConfig::Gpu();
        }
        tmpSurface.reset(new QOffscreenSurface);
        tmpSurface->setFormat(tmpContext->format());
        tmpSurface->create();
        tmpContext->makeCurrent(tmpSurface.data());
    }

    QOpenGLConfig::Gpu gpu;
    ctx = QOpenGLContext::currentContext();
    const GLubyte *p = ctx->functions()->glGetString(GL_VENDOR);
    if (p)
        gpu.glVendor = QByteArray(reinterpret_cast<const char *>(p));

    return gpu;
}

QT_END_NAMESPACE
