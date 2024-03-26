// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


/*!
    \class QSslCertificate
    \brief The QSslCertificate class provides a convenient API for an X509 certificate.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslCertificate stores an X509 certificate, and is commonly used
    to verify the identity and store information about the local host,
    a remotely connected peer, or a trusted third party Certificate
    Authority.

    There are many ways to construct a QSslCertificate. The most
    common way is to call QSslSocket::peerCertificate(), which returns
    a QSslCertificate object, or QSslSocket::peerCertificateChain(),
    which returns a list of them. You can also load certificates from
    a DER (binary) or PEM (Base64) encoded bundle, typically stored as
    one or more local files, or in a Qt Resource.

    You can call isNull() to check if your certificate is null. By default,
    QSslCertificate constructs a null certificate. A null certificate is
    invalid, but an invalid certificate is not necessarily null. If you want
    to reset all contents in a certificate, call clear().

    After loading a certificate, you can find information about the
    certificate, its subject, and its issuer, by calling one of the
    many accessor functions, including version(), serialNumber(),
    issuerInfo() and subjectInfo(). You can call effectiveDate() and
    expiryDate() to check when the certificate starts being
    effective and when it expires.
    The publicKey() function returns the certificate
    subject's public key as a QSslKey. You can call issuerInfo() or
    subjectInfo() to get detailed information about the certificate
    issuer and its subject.

    Internally, QSslCertificate is stored as an X509 structure. You
    can access this handle by calling handle(), but the results are
    likely to not be portable.

    \sa QSslSocket, QSslKey, QSslCipher, QSslError
*/

/*!
    \enum QSslCertificate::SubjectInfo

    Describes keys that you can pass to QSslCertificate::issuerInfo() or
    QSslCertificate::subjectInfo() to get information about the certificate
    issuer or subject.

    \value Organization "O" The name of the organization.

    \value CommonName "CN" The common name; most often this is used to store
    the host name.

    \value LocalityName "L" The locality.

    \value OrganizationalUnitName "OU" The organizational unit name.

    \value CountryName "C" The country.

    \value StateOrProvinceName "ST" The state or province.

    \value DistinguishedNameQualifier The distinguished name qualifier

    \value SerialNumber The certificate's serial number

    \value EmailAddress The email address associated with the certificate
*/

/*!
    \enum QSslCertificate::PatternSyntax
    \since 5.15

    The syntax used to interpret the meaning of the pattern.

    \value RegularExpression A rich Perl-like pattern matching syntax.

    \value Wildcard This provides a simple pattern matching syntax
    similar to that used by shells (command interpreters) for "file
    globbing". See \l {QRegularExpression::fromWildcard()}.

    \value FixedString The pattern is a fixed string. This is
    equivalent to using the RegularExpression pattern on a string in
    which all metacharacters are escaped using escape(). This is the
    default.
*/

#include <QtNetwork/qtnetworkglobal.h>

#if QT_CONFIG(regularexpression)
#include "qregularexpression.h"
#endif

#include "qsslcertificateextension_p.h"
#include "qsslcertificate_p.h"
#include "qsslcertificate.h"
#include "qssl_p.h"

#ifndef QT_NO_SSL
#include "qsslsocket_p.h"
#include "qsslkey_p.h"
#endif

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN(QSslCertificate)

QSslCertificatePrivate::QSslCertificatePrivate()
{
#ifndef QT_NO_SSL
    QSslSocketPrivate::ensureInitialized();
#endif

    const QTlsBackend *tlsBackend = QTlsBackend::activeOrAnyBackend();
    if (tlsBackend)
        backend.reset(tlsBackend->createCertificate());
    else
        qCWarning(lcSsl, "No TLS backend is available");
}

QSslCertificatePrivate::~QSslCertificatePrivate() = default;

/*!
    Constructs a QSslCertificate by reading \a format encoded data
    from \a device and using the first certificate found. You can
    later call isNull() to see if \a device contained a certificate,
    and if this certificate was loaded successfully.
*/
QSslCertificate::QSslCertificate(QIODevice *device, QSsl::EncodingFormat format)
    : d(new QSslCertificatePrivate)
{
    if (device) {
        const auto data = device->readAll();
        if (data.isEmpty())
            return;

        const auto *tlsBackend = QTlsBackend::activeOrAnyBackend();
        if (!tlsBackend)
            return;

        auto *X509Reader = format == QSsl::Pem ? tlsBackend->X509PemReader() : tlsBackend->X509DerReader();
        if (!X509Reader) {
            qCWarning(lcSsl, "Current TLS plugin does not support reading from PEM/DER");
            return;
        }

        QList<QSslCertificate> certs = X509Reader(data, 1);
        if (!certs.isEmpty())
            d = certs.first().d;
    }
}

/*!
    Constructs a QSslCertificate by parsing the \a format encoded
    \a data and using the first available certificate found. You can
    later call isNull() to see if \a data contained a certificate,
    and if this certificate was loaded successfully.
*/
QSslCertificate::QSslCertificate(const QByteArray &data, QSsl::EncodingFormat format)
    : d(new QSslCertificatePrivate)
{
    if (data.isEmpty())
        return;

    const auto *tlsBackend = QTlsBackend::activeOrAnyBackend();
    if (!tlsBackend)
        return;

    auto *X509Reader = format == QSsl::Pem ? tlsBackend->X509PemReader() : tlsBackend->X509DerReader();
    if (!X509Reader) {
        qCWarning(lcSsl, "Current TLS plugin does not support reading from PEM/DER");
        return;
    }

    QList<QSslCertificate> certs = X509Reader(data, 1);
    if (!certs.isEmpty())
        d = certs.first().d;
}

/*!
    Constructs an identical copy of \a other.
*/
QSslCertificate::QSslCertificate(const QSslCertificate &other) : d(other.d)
{
}

/*!
    Destroys the QSslCertificate.
*/
QSslCertificate::~QSslCertificate()
{
}

/*!
    Copies the contents of \a other into this certificate, making the two
    certificates identical.
*/
QSslCertificate &QSslCertificate::operator=(const QSslCertificate &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QSslCertificate::swap(QSslCertificate &other)
    \since 5.0

    Swaps this certificate instance with \a other. This function is
    very fast and never fails.
*/

/*!
    \fn bool QSslCertificate::operator==(const QSslCertificate &other) const

    Returns \c true if this certificate is the same as \a other; otherwise
    returns \c false.
*/

bool QSslCertificate::operator==(const QSslCertificate &other) const
{
    if (d == other.d)
        return true;

    if (isNull() && other.isNull())
        return true;

    if (d->backend.get() && other.d->backend.get())
        return d->backend->isEqual(*other.d->backend.get());

    return false;
}

/*!
    \fn bool QSslCertificate::operator!=(const QSslCertificate &other) const

    Returns \c true if this certificate is not the same as \a other; otherwise
    returns \c false.
*/

/*!
    \fn bool QSslCertificate::isNull() const

    Returns \c true if this is a null certificate (i.e., a certificate
    with no contents); otherwise returns \c false.

    By default, QSslCertificate constructs a null certificate.

    \sa clear()
*/
bool QSslCertificate::isNull() const
{
    if (const auto *backend = d->backend.get())
        return backend->isNull();

    return true;
}

/*!
    Returns \c true if this certificate is blacklisted; otherwise
    returns \c false.

    \sa isNull()
*/
bool QSslCertificate::isBlacklisted() const
{
    return QSslCertificatePrivate::isBlacklisted(*this);
}

/*!
    \fn bool QSslCertificate::isSelfSigned() const
    \since 5.4

    Returns \c true if this certificate is self signed; otherwise
    returns \c false.

    A certificate is considered self-signed its issuer and subject
    are identical.
*/
bool QSslCertificate::isSelfSigned() const
{
    if (const auto *backend = d->backend.get())
        return backend->isSelfSigned();

    return false;
}

/*!
    Clears the contents of this certificate, making it a null
    certificate.

    \sa isNull()
*/
void QSslCertificate::clear()
{
    if (isNull())
        return;
    d = new QSslCertificatePrivate;
}

/*!
    \fn QByteArray QSslCertificate::version() const
    Returns the certificate's version string.
*/
QByteArray QSslCertificate::version() const
{
    if (const auto *backend = d->backend.get())
        return backend->version();

    return {};
}

/*!
    \fn QByteArray QSslCertificate::serialNumber() const

    Returns the certificate's serial number string in hexadecimal format.
*/
QByteArray QSslCertificate::serialNumber() const
{
    if (const auto *backend = d->backend.get())
        return backend->serialNumber();

    return {};
}

/*!
    Returns a cryptographic digest of this certificate. By default,
    an MD5 digest will be generated, but you can also specify a
    custom \a algorithm.
*/
QByteArray QSslCertificate::digest(QCryptographicHash::Algorithm algorithm) const
{
    return QCryptographicHash::hash(toDer(), algorithm);
}

/*!
  \fn QString QSslCertificate::issuerInfo(SubjectInfo subject) const

  Returns the issuer information for the \a subject from the
  certificate, or an empty list if there is no information for
  \a subject in the certificate. There can be more than one entry
  of each type.

  \sa subjectInfo()
*/
QStringList QSslCertificate::issuerInfo(SubjectInfo info) const
{
    if (const auto *backend = d->backend.get())
        return backend->issuerInfo(info);

    return {};
}

/*!
  \fn QStringList QSslCertificate::issuerInfo(const QByteArray &attribute) const

  Returns the issuer information for \a attribute from the certificate,
  or an empty list if there is no information for \a attribute in the
  certificate. There can be more than one entry for an attribute.

  \sa subjectInfo()
*/
QStringList QSslCertificate::issuerInfo(const QByteArray &attribute) const
{
    if (const auto *backend = d->backend.get())
        return backend->issuerInfo(attribute);

    return {};
}

/*!
  \fn QString QSslCertificate::subjectInfo(SubjectInfo subject) const

  Returns the information for the \a subject, or an empty list if
  there is no information for \a subject in the certificate. There
  can be more than one entry of each type.

    \sa issuerInfo()
*/
QStringList QSslCertificate::subjectInfo(SubjectInfo info) const
{
    if (const auto *backend = d->backend.get())
        return backend->subjectInfo(info);

    return {};
}

/*!
    \fn QStringList QSslCertificate::subjectInfo(const QByteArray &attribute) const

    Returns the subject information for \a attribute, or an empty list if
    there is no information for \a attribute in the certificate. There
    can be more than one entry for an attribute.

    \sa issuerInfo()
*/
QStringList QSslCertificate::subjectInfo(const QByteArray &attribute) const
{
    if (const auto *backend = d->backend.get())
        return backend->subjectInfo(attribute);

    return {};
}

/*!
    \fn QList<QByteArray> QSslCertificate::subjectInfoAttributes() const

    \since 5.0
    Returns a list of the attributes that have values in the subject
    information of this certificate. The information associated
    with a given attribute can be accessed using the subjectInfo()
    method. Note that this list may include the OIDs for any
    elements that are not known by the SSL backend.

    \sa subjectInfo()
*/
QList<QByteArray> QSslCertificate::subjectInfoAttributes() const
{
    if (const auto *backend = d->backend.get())
        return backend->subjectInfoAttributes();

    return {};
}

/*!
    \fn QList<QByteArray> QSslCertificate::issuerInfoAttributes() const

    \since 5.0
    Returns a list of the attributes that have values in the issuer
    information of this certificate. The information associated
    with a given attribute can be accessed using the issuerInfo()
    method. Note that this list may include the OIDs for any
    elements that are not known by the SSL backend.

    \sa subjectInfo()
*/
QList<QByteArray> QSslCertificate::issuerInfoAttributes() const
{
    if (const auto *backend = d->backend.get())
        return backend->issuerInfoAttributes();

    return {};
}

/*!
  \fn QMultiMap<QSsl::AlternativeNameEntryType, QString> QSslCertificate::subjectAlternativeNames() const

  Returns the list of alternative subject names for this
  certificate. The alternative names typically contain host
  names, optionally with wildcards, that are valid for this
  certificate.

  These names are tested against the connected peer's host name, if
  either the subject information for \l CommonName doesn't define a
  valid host name, or the subject info name doesn't match the peer's
  host name.

  \sa subjectInfo()
*/
QMultiMap<QSsl::AlternativeNameEntryType, QString> QSslCertificate::subjectAlternativeNames() const
{
    if (const auto *backend = d->backend.get())
        return backend->subjectAlternativeNames();

    return {};
}

/*!
  \fn QDateTime QSslCertificate::effectiveDate() const

  Returns the date-time that the certificate becomes valid, or an
  empty QDateTime if this is a null certificate.

  \sa expiryDate()
*/
QDateTime QSslCertificate::effectiveDate() const
{
    if (const auto *backend = d->backend.get())
        return backend->effectiveDate();

    return {};
}

/*!
  \fn QDateTime QSslCertificate::expiryDate() const

  Returns the date-time that the certificate expires, or an empty
  QDateTime if this is a null certificate.

    \sa effectiveDate()
*/
QDateTime QSslCertificate::expiryDate() const
{
    if (const auto *backend = d->backend.get())
        return backend->expiryDate();

    return {};
}

/*!
    \fn Qt::HANDLE QSslCertificate::handle() const
    Returns a pointer to the native certificate handle, if there is
    one, else \nullptr.

    You can use this handle, together with the native API, to access
    extended information about the certificate.

    \warning Use of this function has a high probability of being
    non-portable, and its return value may vary from platform to
    platform or change from minor release to minor release.
*/
Qt::HANDLE QSslCertificate::handle() const
{
    if (const auto *backend = d->backend.get())
        return backend->handle();

    return {};
}

#ifndef QT_NO_SSL
/*!
    \fn QSslKey QSslCertificate::publicKey() const
    Returns the certificate subject's public key.
*/
QSslKey QSslCertificate::publicKey() const
{
    QSslKey key;
    if (const auto *backend = d->backend.get())
        QTlsBackend::resetBackend(key, backend->publicKey());

    return key;
}
#endif // QT_NO_SSL


/*!
    \fn QList<QSslCertificateExtension> QSslCertificate::extensions() const

    Returns a list containing the X509 extensions of this certificate.
    \since 5.0
 */
QList<QSslCertificateExtension> QSslCertificate::extensions() const
{
    return d->extensions();
}

/*!
    \fn QByteArray QSslCertificate::toPem() const

    Returns this certificate converted to a PEM (Base64) encoded
    representation.
*/
QByteArray QSslCertificate::toPem() const
{
    if (const auto *backend = d->backend.get())
        return backend->toPem();

    return {};
}

/*!
    \fn QByteArray QSslCertificate::toDer() const

    Returns this certificate converted to a DER (binary) encoded
    representation.
*/
QByteArray QSslCertificate::toDer() const
{
    if (const auto *backend = d->backend.get())
        return backend->toDer();

    return {};
}

/*!
    \fn QString QSslCertificate::toText() const

    Returns this certificate converted to a human-readable text
    representation.

    \since 5.0
*/
QString QSslCertificate::toText() const
{
    if (const auto *backend = d->backend.get())
        return backend->toText();

    return {};
}

/*!
    \since 5.15

    Searches all files in the \a path for certificates encoded in the
    specified \a format and returns them in a list. \a path must be a file
    or a pattern matching one or more files, as specified by \a syntax.

    Example:

    \snippet code/src_network_ssl_qsslcertificate.cpp 1

    \sa fromData()
*/
QList<QSslCertificate> QSslCertificate::fromPath(const QString &path,
                                                 QSsl::EncodingFormat format,
                                                 PatternSyntax syntax)
{
    // $, (,), *, +, ., ?, [, ,], ^, {, | and }.

    // make sure to use the same path separators on Windows and Unix like systems.
    QString sourcePath = QDir::fromNativeSeparators(path);

    // Find the path without the filename
    QString pathPrefix = sourcePath.left(sourcePath.lastIndexOf(u'/'));

    // Check if the path contains any special chars
    int pos = -1;

#if QT_CONFIG(regularexpression)
    if (syntax == PatternSyntax::Wildcard)
        pos = pathPrefix.indexOf(QRegularExpression("[*?[]"_L1));
    else if (syntax == PatternSyntax::RegularExpression)
        pos = sourcePath.indexOf(QRegularExpression("[\\$\\(\\)\\*\\+\\.\\?\\[\\]\\^\\{\\}\\|]"_L1));
#else
    if (syntax == PatternSyntax::Wildcard || syntax == PatternSyntax::RegExp)
        qWarning("Regular expression support is disabled in this build. Only fixed string can be searched");
        return QList<QSslCertificate>();
#endif

    if (pos != -1) {
        // there was a special char in the path so cut of the part containing that char.
        pathPrefix = pathPrefix.left(pos);
        const qsizetype lastIndexOfSlash = pathPrefix.lastIndexOf(u'/');
        if (lastIndexOfSlash != -1)
            pathPrefix = pathPrefix.left(lastIndexOfSlash);
        else
            pathPrefix.clear();
    } else {
        // Check if the path is a file.
        if (QFileInfo(sourcePath).isFile()) {
            QFile file(sourcePath);
            QIODevice::OpenMode openMode = QIODevice::ReadOnly;
            if (format == QSsl::Pem)
                openMode |= QIODevice::Text;
            if (file.open(openMode))
                return QSslCertificate::fromData(file.readAll(), format);
            return QList<QSslCertificate>();
        }
    }

    // Special case - if the prefix ends up being nothing, use "." instead.
    int startIndex = 0;
    if (pathPrefix.isEmpty()) {
        pathPrefix = "."_L1;
        startIndex = 2;
    }

    // The path can be a file or directory.
    QList<QSslCertificate> certs;

#if QT_CONFIG(regularexpression)
    if (syntax == PatternSyntax::Wildcard)
        sourcePath = QRegularExpression::wildcardToRegularExpression(sourcePath, QRegularExpression::UnanchoredWildcardConversion);

    QRegularExpression pattern(QRegularExpression::anchoredPattern(sourcePath));
#endif

    QDirIterator it(pathPrefix, QDir::Files, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = startIndex == 0 ? it.next() : it.next().mid(startIndex);

#if QT_CONFIG(regularexpression)
        if (!pattern.match(filePath).hasMatch())
            continue;
#else
        if (sourcePath != filePath)
            continue;
#endif

        QFile file(filePath);
        QIODevice::OpenMode openMode = QIODevice::ReadOnly;
        if (format == QSsl::Pem)
            openMode |= QIODevice::Text;
        if (file.open(openMode))
            certs += QSslCertificate::fromData(file.readAll(), format);
    }
    return certs;
}

/*!
    Searches for and parses all certificates in \a device that are
    encoded in the specified \a format and returns them in a list of
    certificates.

    \sa fromData()
*/
QList<QSslCertificate> QSslCertificate::fromDevice(QIODevice *device, QSsl::EncodingFormat format)
{
    if (!device) {
        qCWarning(lcSsl, "QSslCertificate::fromDevice: cannot read from a null device");
        return QList<QSslCertificate>();
    }
    return fromData(device->readAll(), format);
}

/*!
    Searches for and parses all certificates in \a data that are
    encoded in the specified \a format and returns them in a list of
    certificates.

    \sa fromDevice()
*/
QList<QSslCertificate> QSslCertificate::fromData(const QByteArray &data, QSsl::EncodingFormat format)
{
    const auto *tlsBackend = QTlsBackend::activeOrAnyBackend();
    if (!tlsBackend) {
        qCWarning(lcSsl, "No TLS backend is available");
        return {};
    }

    auto reader = format == QSsl::Pem ? tlsBackend->X509PemReader() : tlsBackend->X509DerReader();
    if (!reader) {
        qCWarning(lcSsl, "The available TLS backend does not support reading PEM/DER");
        return {};
    }

    return reader(data, -1);
}

#ifndef QT_NO_SSL
/*!
    Verifies a certificate chain. The chain to be verified is passed in the
    \a certificateChain parameter. The first certificate in the list should
    be the leaf certificate of the chain to be verified. If \a hostName is
    specified then the certificate is also checked to see if it is valid for
    the specified host name.

    Note that the root (CA) certificate should not be included in the list to be verified,
    this will be looked up automatically using the CA list specified in the
    default QSslConfiguration, and, in addition, if possible, CA certificates loaded on
    demand on Unix and Windows.

    \since 5.0
 */
QList<QSslError> QSslCertificate::verify(const QList<QSslCertificate> &certificateChain, const QString &hostName)
{
    const auto *tlsBackend = QTlsBackend::activeOrAnyBackend();
    if (!tlsBackend) {
        qCWarning(lcSsl, "No TLS backend is available");
        return {};
    }
    auto verifyPtr = tlsBackend->X509Verifier();
    if (!verifyPtr) {
        qCWarning(lcSsl, "Available TLS backend does not support manual certificate verification");
        return {};
    }
    return verifyPtr(certificateChain, hostName);
}

/*!
  \since 5.4

  Imports a PKCS#12 (pfx) file from the specified \a device. A PKCS#12
  file is a bundle that can contain a number of certificates and keys.
  This method reads a single \a key, its \a certificate and any
  associated \a caCertificates from the bundle. If a \a passPhrase is
  specified then this will be used to decrypt the bundle. Returns
  \c true if the PKCS#12 file was successfully loaded.

  \note The \a device must be open and ready to be read from.
 */
bool QSslCertificate::importPkcs12(QIODevice *device,
                                   QSslKey *key, QSslCertificate *certificate,
                                   QList<QSslCertificate> *caCertificates,
                                   const QByteArray &passPhrase)
{
    if (!device || !key || !certificate)
        return false;

    const auto *tlsBackend = QTlsBackend::activeOrAnyBackend();
    if (!tlsBackend) {
        qCWarning(lcSsl, "No TLS backend is available");
        return false;
    }

    if (auto reader = tlsBackend->X509Pkcs12Reader())
        return reader(device, key, certificate, caCertificates, passPhrase);

    qCWarning(lcSsl, "Available TLS backend does not support PKCS12");

    return false;
}
#endif // QT_NO_SSL

QList<QSslCertificateExtension> QSslCertificatePrivate::extensions() const
{
    QList<QSslCertificateExtension> result;

    if (backend.get()) {
        auto nExt = backend->numberOfExtensions();
        for (decltype (nExt) i = 0; i < nExt; ++i) {
            QSslCertificateExtension ext;
            ext.d->oid = backend->oidForExtension(i);
            ext.d->name = backend->nameForExtension(i);
            ext.d->value = backend->valueForExtension(i);
            ext.d->critical = backend->isExtensionCritical(i);
            ext.d->supported = backend->isExtensionSupported(i);
            result << ext;
        }
    }

    return result;
}

// These certificates are known to be fraudulent and were created during the comodo
// compromise. See http://www.comodo.com/Comodo-Fraud-Incident-2011-03-23.html
static const char *const certificate_blacklist[] = {
    "04:7e:cb:e9:fc:a5:5f:7b:d0:9e:ae:36:e1:0c:ae:1e", "mail.google.com", // Comodo
    "f5:c8:6a:f3:61:62:f1:3a:64:f5:4f:6d:c9:58:7c:06", "www.google.com", // Comodo
    "d7:55:8f:da:f5:f1:10:5b:b2:13:28:2b:70:77:29:a3", "login.yahoo.com", // Comodo
    "39:2a:43:4f:0e:07:df:1f:8a:a3:05:de:34:e0:c2:29", "login.yahoo.com", // Comodo
    "3e:75:ce:d4:6b:69:30:21:21:88:30:ae:86:a8:2a:71", "login.yahoo.com", // Comodo
    "e9:02:8b:95:78:e4:15:dc:1a:71:0a:2b:88:15:44:47", "login.skype.com", // Comodo
    "92:39:d5:34:8f:40:d1:69:5a:74:54:70:e1:f2:3f:43", "addons.mozilla.org", // Comodo
    "b0:b7:13:3e:d0:96:f9:b5:6f:ae:91:c8:74:bd:3a:c0", "login.live.com", // Comodo
    "d8:f3:5f:4e:b7:87:2b:2d:ab:06:92:e3:15:38:2f:b0", "global trustee", // Comodo

    "05:e2:e6:a4:cd:09:ea:54:d6:65:b0:75:fe:22:a2:56", "*.google.com", // leaf certificate issued by DigiNotar
    "0c:76:da:9c:91:0c:4e:2c:9e:fe:15:d0:58:93:3c:4c", "DigiNotar Root CA", // DigiNotar root
    "f1:4a:13:f4:87:2b:56:dc:39:df:84:ca:7a:a1:06:49", "DigiNotar Services CA", // DigiNotar intermediate signed by DigiNotar Root
    "36:16:71:55:43:42:1b:9d:e6:cb:a3:64:41:df:24:38", "DigiNotar Services 1024 CA", // DigiNotar intermediate signed by DigiNotar Root
    "0a:82:bd:1e:14:4e:88:14:d7:5b:1a:55:27:be:bf:3e", "DigiNotar Root CA G2", // other DigiNotar Root CA
    "a4:b6:ce:e3:2e:d3:35:46:26:3c:b3:55:3a:a8:92:21", "CertiID Enterprise Certificate Authority", // DigiNotar intermediate signed by "DigiNotar Root CA G2"
    "5b:d5:60:9c:64:17:68:cf:21:0e:35:fd:fb:05:ad:41", "DigiNotar Qualified CA", // DigiNotar intermediate signed by DigiNotar Root

    "46:9c:2c:b0",                                     "DigiNotar Services 1024 CA", // DigiNotar intermediate cross-signed by Entrust
    "07:27:10:0d",                                     "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "07:27:0f:f9",                                     "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "07:27:10:03",                                     "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "01:31:69:b0",                                     "DigiNotar PKIoverheid CA Overheid en Bedrijven", // DigiNotar intermediate cross-signed by the Dutch government
    "01:31:34:bf",                                     "DigiNotar PKIoverheid CA Organisatie - G2", // DigiNotar intermediate cross-signed by the Dutch government
    "d6:d0:29:77:f1:49:fd:1a:83:f2:b9:ea:94:8c:5c:b4", "DigiNotar Extended Validation CA", // DigiNotar intermediate signed by DigiNotar EV Root
    "1e:7d:7a:53:3d:45:30:41:96:40:0f:71:48:1f:45:04", "DigiNotar Public CA 2025", // DigiNotar intermediate
//    "(has not been seen in the wild so far)", "DigiNotar Public CA - G2", // DigiNotar intermediate
//    "(has not been seen in the wild so far)", "Koninklijke Notariele Beroepsorganisatie CA", // compromised during DigiNotar breach
//    "(has not been seen in the wild so far)", "Stichting TTP Infos CA," // compromised during DigiNotar breach
    "46:9c:2c:af",                                     "DigiNotar Root CA", // DigiNotar intermediate cross-signed by Entrust
    "46:9c:3c:c9",                                     "DigiNotar Root CA", // DigiNotar intermediate cross-signed by Entrust

    "07:27:14:a9",                                     "Digisign Server ID (Enrich)", // (Malaysian) Digicert Sdn. Bhd. cross-signed by Verizon CyberTrust
    "4c:0e:63:6a",                                     "Digisign Server ID - (Enrich)", // (Malaysian) Digicert Sdn. Bhd. cross-signed by Entrust
    "72:03:21:05:c5:0c:08:57:3d:8e:a5:30:4e:fe:e8:b0", "UTN-USERFirst-Hardware", // comodogate test certificate
    "41",                                              "MD5 Collisions Inc. (http://www.phreedom.org/md5)", // http://www.phreedom.org/research/rogue-ca/

    "08:27",                                           "*.EGO.GOV.TR", // Turktrust mis-issued intermediate certificate
    "08:64",                                           "e-islem.kktcmerkezbankasi.org", // Turktrust mis-issued intermediate certificate

    "03:1d:a7",                                        "AC DG Tr\xC3\xA9sor SSL", // intermediate certificate linking back to ANSSI French National Security Agency
    "27:83",                                           "NIC Certifying Authority", // intermediate certificate from NIC India (2007)
    "27:92",                                           "NIC CA 2011", // intermediate certificate from NIC India (2011)
    "27:b1",                                           "NIC CA 2014", // intermediate certificate from NIC India (2014)
    nullptr
};

bool QSslCertificatePrivate::isBlacklisted(const QSslCertificate &certificate)
{
    for (int a = 0; certificate_blacklist[a] != nullptr; a++) {
        QString blacklistedCommonName = QString::fromUtf8(certificate_blacklist[(a+1)]);
        if (certificate.serialNumber() == certificate_blacklist[a++] &&
            (certificate.subjectInfo(QSslCertificate::CommonName).contains(blacklistedCommonName) ||
             certificate.issuerInfo(QSslCertificate::CommonName).contains(blacklistedCommonName)))
            return true;
    }
    return false;
}

QByteArray QSslCertificatePrivate::subjectInfoToString(QSslCertificate::SubjectInfo info)
{
    QByteArray str;
    switch (info) {
    case QSslCertificate::Organization: str = QByteArray("O"); break;
    case QSslCertificate::CommonName: str = QByteArray("CN"); break;
    case QSslCertificate::LocalityName: str = QByteArray("L"); break;
    case QSslCertificate::OrganizationalUnitName: str = QByteArray("OU"); break;
    case QSslCertificate::CountryName: str = QByteArray("C"); break;
    case QSslCertificate::StateOrProvinceName: str = QByteArray("ST"); break;
    case QSslCertificate::DistinguishedNameQualifier: str = QByteArray("dnQualifier"); break;
    case QSslCertificate::SerialNumber: str = QByteArray("serialNumber"); break;
    case QSslCertificate::EmailAddress: str = QByteArray("emailAddress"); break;
    }
    return str;
}

/*!
    \since 5.12

    Returns a name that describes the issuer. It returns the QSslCertificate::CommonName
    if available, otherwise falls back to the first QSslCertificate::Organization or the
    first QSslCertificate::OrganizationalUnitName.

    \sa issuerInfo()
*/
QString QSslCertificate::issuerDisplayName() const
{
    QStringList names;
    names = issuerInfo(QSslCertificate::CommonName);
    if (!names.isEmpty())
        return names.first();
    names = issuerInfo(QSslCertificate::Organization);
    if (!names.isEmpty())
        return names.first();
    names = issuerInfo(QSslCertificate::OrganizationalUnitName);
    if (!names.isEmpty())
        return names.first();

    return QString();
}

/*!
    \since 5.12

    Returns a name that describes the subject. It returns the QSslCertificate::CommonName
    if available, otherwise falls back to the first QSslCertificate::Organization or the
    first QSslCertificate::OrganizationalUnitName.

    \sa subjectInfo()
*/
QString QSslCertificate::subjectDisplayName() const
{
    QStringList names;
    names = subjectInfo(QSslCertificate::CommonName);
    if (!names.isEmpty())
        return names.first();
    names = subjectInfo(QSslCertificate::Organization);
    if (!names.isEmpty())
        return names.first();
    names = subjectInfo(QSslCertificate::OrganizationalUnitName);
    if (!names.isEmpty())
        return names.first();

    return QString();
}

/*!
    Returns the hash value for the \a key, using \a seed to seed the calculation.
    \since 5.4
    \relates QHash
*/
size_t qHash(const QSslCertificate &key, size_t seed) noexcept
{
    if (const auto *backend = key.d->backend.get())
        return backend->hash(seed);

    return seed;

}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSslCertificate &certificate)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QSslCertificate("
          << "Version=" << certificate.version()
          << ", SerialNumber=" << certificate.serialNumber()
          << ", Digest=" << certificate.digest().toBase64()
          << ", Issuer=" << certificate.issuerDisplayName()
          << ", Subject=" << certificate.subjectDisplayName()
          << ", AlternativeSubjectNames=" << certificate.subjectAlternativeNames()
#if QT_CONFIG(datestring)
          << ", EffectiveDate=" << certificate.effectiveDate()
          << ", ExpiryDate=" << certificate.expiryDate()
#endif
          << ')';
    return debug;
}
QDebug operator<<(QDebug debug, QSslCertificate::SubjectInfo info)
{
    switch (info) {
    case QSslCertificate::Organization: debug << "Organization"; break;
    case QSslCertificate::CommonName: debug << "CommonName"; break;
    case QSslCertificate::CountryName: debug << "CountryName"; break;
    case QSslCertificate::LocalityName: debug << "LocalityName"; break;
    case QSslCertificate::OrganizationalUnitName: debug << "OrganizationalUnitName"; break;
    case QSslCertificate::StateOrProvinceName: debug << "StateOrProvinceName"; break;
    case QSslCertificate::DistinguishedNameQualifier: debug << "DistinguishedNameQualifier"; break;
    case QSslCertificate::SerialNumber: debug << "SerialNumber"; break;
    case QSslCertificate::EmailAddress: debug << "EmailAddress"; break;
    }
    return debug;
}
#endif

QT_END_NAMESPACE
