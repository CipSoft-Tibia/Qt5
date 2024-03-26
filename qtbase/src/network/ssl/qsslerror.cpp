// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


/*!
    \class QSslError
    \brief The QSslError class provides an SSL error.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslError provides a simple API for managing errors during QSslSocket's
    SSL handshake.

    \sa QSslSocket, QSslCertificate, QSslCipher
*/

/*!
    \enum QSslError::SslError

    Describes all recognized errors that can occur during an SSL handshake.

    \value NoError
    \value UnableToGetIssuerCertificate
    \value UnableToDecryptCertificateSignature
    \value UnableToDecodeIssuerPublicKey
    \value CertificateSignatureFailed
    \value CertificateNotYetValid
    \value CertificateExpired
    \value InvalidNotBeforeField
    \value InvalidNotAfterField
    \value SelfSignedCertificate
    \value SelfSignedCertificateInChain
    \value UnableToGetLocalIssuerCertificate
    \value UnableToVerifyFirstCertificate
    \value CertificateRevoked
    \value InvalidCaCertificate
    \value PathLengthExceeded
    \value InvalidPurpose
    \value CertificateUntrusted
    \value CertificateRejected
    \value SubjectIssuerMismatch
    \value AuthorityIssuerSerialNumberMismatch
    \value NoPeerCertificate
    \value HostNameMismatch
    \value UnspecifiedError
    \value NoSslSupport
    \value CertificateBlacklisted
    \value CertificateStatusUnknown
    \value OcspNoResponseFound
    \value OcspMalformedRequest
    \value OcspMalformedResponse
    \value OcspInternalError
    \value OcspTryLater
    \value OcspSigRequred
    \value OcspUnauthorized
    \value OcspResponseCannotBeTrusted
    \value OcspResponseCertIdUnknown
    \value OcspResponseExpired
    \value OcspStatusUnknown


    \sa QSslError::errorString()
*/

#include "qsslerror.h"
#include "qsslsocket.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SSL
QT_IMPL_METATYPE_EXTERN_TAGGED(QList<QSslError>, QList_QSslError)
#endif


#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
// Avoid an ABI break due to the QScopedPointer->std::unique_ptr change
static_assert(sizeof(QScopedPointer<QSslErrorPrivate>) == sizeof(std::unique_ptr<QSslErrorPrivate>));
#endif

class QSslErrorPrivate
{
public:
    QSslError::SslError error;
    QSslCertificate certificate;
};

// RVCT compiler in debug build does not like about default values in const-
// So as an workaround we define all constructor overloads here explicitly
/*!
    Constructs a QSslError object with no error and default certificate.

*/

QSslError::QSslError()
    : d(new QSslErrorPrivate)
{
    d->error = QSslError::NoError;
    d->certificate = QSslCertificate();
}

/*!
    Constructs a QSslError object. The argument specifies the \a
    error that occurred.

*/
QSslError::QSslError(SslError error)
    : d(new QSslErrorPrivate)
{
    d->error = error;
    d->certificate = QSslCertificate();
}

/*!
    Constructs a QSslError object. The two arguments specify the \a
    error that occurred, and which \a certificate the error relates to.

    \sa QSslCertificate
*/
QSslError::QSslError(SslError error, const QSslCertificate &certificate)
    : d(new QSslErrorPrivate)
{
    d->error = error;
    d->certificate = certificate;
}

/*!
    Constructs an identical copy of \a other.
*/
QSslError::QSslError(const QSslError &other)
    : d(new QSslErrorPrivate)
{
    *d.get() = *other.d.get();
}

/*!
    Destroys the QSslError object.
*/
QSslError::~QSslError()
{
}

/*!
    \since 4.4

    Assigns the contents of \a other to this error.
*/
QSslError &QSslError::operator=(const QSslError &other)
{
    *d.get() = *other.d.get();
    return *this;
}

/*!
    \fn void QSslError::swap(QSslError &other)
    \since 5.0

    Swaps this error instance with \a other. This function is very
    fast and never fails.
*/

/*!
    \since 4.4

    Returns \c true if this error is equal to \a other; otherwise returns \c false.
*/
bool QSslError::operator==(const QSslError &other) const
{
    return d->error == other.d->error
        && d->certificate == other.d->certificate;
}

/*!
    \fn bool QSslError::operator!=(const QSslError &other) const
    \since 4.4

    Returns \c true if this error is not equal to \a other; otherwise returns
    false.
*/

/*!
    Returns the type of the error.

    \sa errorString(), certificate()
*/
QSslError::SslError QSslError::error() const
{
    return d->error;
}

/*!
    Returns a short localized human-readable description of the error.

    \sa error(), certificate()
*/
QString QSslError::errorString() const
{
    QString errStr;
    switch (d->error) {
    case NoError:
        errStr = QSslSocket::tr("No error");
        break;
    case UnableToGetIssuerCertificate:
        errStr = QSslSocket::tr("The issuer certificate could not be found");
        break;
    case UnableToDecryptCertificateSignature:
        errStr = QSslSocket::tr("The certificate signature could not be decrypted");
        break;
    case UnableToDecodeIssuerPublicKey:
        errStr = QSslSocket::tr("The public key in the certificate could not be read");
        break;
    case CertificateSignatureFailed:
        errStr = QSslSocket::tr("The signature of the certificate is invalid");
        break;
    case CertificateNotYetValid:
        errStr = QSslSocket::tr("The certificate is not yet valid");
        break;
    case CertificateExpired:
        errStr = QSslSocket::tr("The certificate has expired");
        break;
    case InvalidNotBeforeField:
        errStr = QSslSocket::tr("The certificate's notBefore field contains an invalid time");
        break;
    case InvalidNotAfterField:
        errStr = QSslSocket::tr("The certificate's notAfter field contains an invalid time");
        break;
    case SelfSignedCertificate:
        errStr = QSslSocket::tr("The certificate is self-signed, and untrusted");
        break;
    case SelfSignedCertificateInChain:
        errStr = QSslSocket::tr("The root certificate of the certificate chain is self-signed, and untrusted");
        break;
    case UnableToGetLocalIssuerCertificate:
        errStr = QSslSocket::tr("The issuer certificate of a locally looked up certificate could not be found");
        break;
    case UnableToVerifyFirstCertificate:
        errStr = QSslSocket::tr("No certificates could be verified");
        break;
    case InvalidCaCertificate:
        errStr = QSslSocket::tr("One of the CA certificates is invalid");
        break;
    case PathLengthExceeded:
        errStr = QSslSocket::tr("The basicConstraints path length parameter has been exceeded");
        break;
    case InvalidPurpose:
        errStr = QSslSocket::tr("The supplied certificate is unsuitable for this purpose");
        break;
    case CertificateUntrusted:
        errStr = QSslSocket::tr("The root CA certificate is not trusted for this purpose");
        break;
    case CertificateRejected:
        errStr = QSslSocket::tr("The root CA certificate is marked to reject the specified purpose");
        break;
    case SubjectIssuerMismatch: // hostname mismatch
        errStr = QSslSocket::tr("The current candidate issuer certificate was rejected because its"
                                " subject name did not match the issuer name of the current certificate");
        break;
    case AuthorityIssuerSerialNumberMismatch:
        errStr = QSslSocket::tr("The current candidate issuer certificate was rejected because"
                             " its issuer name and serial number was present and did not match the"
                             " authority key identifier of the current certificate");
        break;
    case NoPeerCertificate:
        errStr = QSslSocket::tr("The peer did not present any certificate");
        break;
    case HostNameMismatch:
        errStr = QSslSocket::tr("The host name did not match any of the valid hosts"
                             " for this certificate");
        break;
    case NoSslSupport:
        break;
    case CertificateBlacklisted:
        errStr = QSslSocket::tr("The peer certificate is blacklisted");
        break;
    case OcspNoResponseFound:
        errStr = QSslSocket::tr("No OCSP status response found");
        break;
    case OcspMalformedRequest:
        errStr = QSslSocket::tr("The OCSP status request had invalid syntax");
        break;
    case OcspMalformedResponse:
        errStr = QSslSocket::tr("OCSP response contains an unexpected number of SingleResponse structures");
        break;
    case OcspInternalError:
        errStr = QSslSocket::tr("OCSP responder reached an inconsistent internal state");
        break;
    case OcspTryLater:
        errStr = QSslSocket::tr("OCSP responder was unable to return a status for the requested certificate");
        break;
    case OcspSigRequred:
        errStr = QSslSocket::tr("The server requires the client to sign the OCSP request in order to construct a response");
        break;
    case OcspUnauthorized:
        errStr = QSslSocket::tr("The client is not authorized to request OCSP status from this server");
        break;
    case OcspResponseCannotBeTrusted:
        errStr = QSslSocket::tr("OCSP responder's identity cannot be verified");
        break;
    case OcspResponseCertIdUnknown:
        errStr = QSslSocket::tr("The identity of a certificate in an OCSP response cannot be established");
        break;
    case OcspResponseExpired:
        errStr = QSslSocket::tr("The certificate status response has expired");
        break;
    case OcspStatusUnknown:
        errStr = QSslSocket::tr("The certificate's status is unknown");
        break;
    default:
        errStr = QSslSocket::tr("Unknown error");
        break;
    }

    return errStr;
}

/*!
    Returns the certificate associated with this error, or a null certificate
    if the error does not relate to any certificate.

    \sa error(), errorString()
*/
QSslCertificate QSslError::certificate() const
{
    return d->certificate;
}

/*!
    Returns the hash value for the \a key, using \a seed to seed the calculation.
    \since 5.4
    \relates QHash
*/
size_t qHash(const QSslError &key, size_t seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.error());
    seed = hash(seed, key.certificate());
    return seed;
}

#ifndef QT_NO_DEBUG_STREAM
//class QDebug;
QDebug operator<<(QDebug debug, const QSslError &error)
{
    debug << error.errorString();
    return debug;
}
QDebug operator<<(QDebug debug, const QSslError::SslError &error)
{
    debug << QSslError(error).errorString();
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qsslerror.cpp"
