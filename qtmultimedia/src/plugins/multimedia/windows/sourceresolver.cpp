// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfstream_p.h"
#include "sourceresolver_p.h"
#include <mferror.h>
#include <nserror.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtMultimedia/qmediaplayer.h>

QT_BEGIN_NAMESPACE

/*
    SourceResolver is separated from MFPlayerSession to handle the work of resolving a media source
    asynchronously. You call SourceResolver::load to request resolving a media source asynchronously,
    and it will emit mediaSourceReady() when resolving is done. You can call SourceResolver::cancel to
    stop the previous load operation if there is any.
*/

SourceResolver::SourceResolver()
    : m_cRef(1)
    , m_cancelCookie(0)
    , m_sourceResolver(0)
    , m_mediaSource(0)
    , m_stream(0)
{
}

SourceResolver::~SourceResolver()
{
    shutdown();
    if (m_mediaSource) {
        m_mediaSource->Release();
        m_mediaSource = NULL;
    }

    if (m_cancelCookie)
        m_cancelCookie->Release();
    if (m_sourceResolver)
        m_sourceResolver->Release();
}

STDMETHODIMP SourceResolver::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else if (riid == IID_IMFAsyncCallback) {
        *ppvObject = static_cast<IMFAsyncCallback*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SourceResolver::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) SourceResolver::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        this->deleteLater();
    return cRef;
}

HRESULT STDMETHODCALLTYPE SourceResolver::Invoke(IMFAsyncResult *pAsyncResult)
{
    QMutexLocker locker(&m_mutex);

    if (!m_sourceResolver)
        return S_OK;

    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
    IUnknown* pSource = NULL;
    State *state = static_cast<State*>(pAsyncResult->GetStateNoAddRef());

    HRESULT hr = S_OK;
    if (state->fromStream())
        hr = m_sourceResolver->EndCreateObjectFromByteStream(pAsyncResult, &ObjectType, &pSource);
    else
        hr = m_sourceResolver->EndCreateObjectFromURL(pAsyncResult, &ObjectType, &pSource);

    if (state->sourceResolver() != m_sourceResolver) {
        //This is a cancelled one
        return S_OK;
    }

    if (m_cancelCookie) {
        m_cancelCookie->Release();
        m_cancelCookie = NULL;
    }

    if (FAILED(hr)) {
        emit error(hr);
        return S_OK;
    }

    if (m_mediaSource) {
        m_mediaSource->Release();
        m_mediaSource = NULL;
    }

    hr = pSource->QueryInterface(IID_PPV_ARGS(&m_mediaSource));
    pSource->Release();
    if (FAILED(hr)) {
        emit error(hr);
        return S_OK;
    }

    emit mediaSourceReady();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE SourceResolver::GetParameters(DWORD*, DWORD*)
{
    return E_NOTIMPL;
}

void SourceResolver::load(const QUrl &url, QIODevice* stream)
{
    QMutexLocker locker(&m_mutex);
    HRESULT hr = S_OK;
    if (!m_sourceResolver)
        hr = MFCreateSourceResolver(&m_sourceResolver);

    if (m_stream) {
        m_stream->Release();
        m_stream = NULL;
    }

    if (FAILED(hr)) {
        qWarning() << "Failed to create Source Resolver!";
        emit error(hr);
    } else if (stream) {
        QString urlString = url.toString();
        m_stream = new MFStream(stream, false);
        hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                    m_stream, urlString.isEmpty() ? 0 : reinterpret_cast<LPCWSTR>(urlString.utf16()),
                    MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE
                    , NULL, &m_cancelCookie, this, new State(m_sourceResolver, true));
        if (FAILED(hr)) {
            qWarning() << "Unsupported stream!";
            emit error(hr);
        }
    } else {
#ifdef DEBUG_MEDIAFOUNDATION
        qDebug() << "loading :" << url;
        qDebug() << "url path =" << url.path().mid(1);
#endif
#ifdef TEST_STREAMING
        //Testing stream function
        if (url.scheme() == QLatin1String("file")) {
            stream = new QFile(url.path().mid(1));
            if (stream->open(QIODevice::ReadOnly)) {
                m_stream = new MFStream(stream, true);
                hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                            m_stream, reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                            MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                            NULL, &m_cancelCookie, this, new State(m_sourceResolver, true));
                if (FAILED(hr)) {
                    qWarning() << "Unsupported stream!";
                    emit error(hr);
                }
            } else {
                delete stream;
                emit error(QMediaPlayer::FormatError);
            }
        } else
#endif
        if (url.scheme() == QLatin1String("qrc")) {
            // If the canonical URL refers to a Qt resource, open with QFile and use
            // the stream playback capability to play.
            stream = new QFile(QLatin1Char(':') + url.path());
            if (stream->open(QIODevice::ReadOnly)) {
                m_stream = new MFStream(stream, true);
                hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                            m_stream, reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                            MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                            NULL, &m_cancelCookie, this, new State(m_sourceResolver, true));
                if (FAILED(hr)) {
                    qWarning() << "Unsupported stream!";
                    emit error(hr);
                }
            } else {
                delete stream;
                emit error(QMediaPlayer::FormatError);
            }
        } else {
            hr = m_sourceResolver->BeginCreateObjectFromURL(
                        reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                        MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                        NULL, &m_cancelCookie, this, new State(m_sourceResolver, false));
            if (FAILED(hr)) {
                qWarning() << "Unsupported url scheme!";
                emit error(hr);
            }
        }
    }
}

void SourceResolver::cancel()
{
    QMutexLocker locker(&m_mutex);
    if (m_cancelCookie) {
        m_sourceResolver->CancelObjectCreation(m_cancelCookie);
        m_cancelCookie->Release();
        m_cancelCookie = NULL;
        m_sourceResolver->Release();
        m_sourceResolver = NULL;
    }
}

void SourceResolver::shutdown()
{
    if (m_mediaSource) {
        m_mediaSource->Shutdown();
        m_mediaSource->Release();
        m_mediaSource = NULL;
    }

    if (m_stream) {
        m_stream->Release();
        m_stream = NULL;
    }
}

IMFMediaSource* SourceResolver::mediaSource() const
{
    return m_mediaSource;
}

/////////////////////////////////////////////////////////////////////////////////
SourceResolver::State::State(IMFSourceResolver *sourceResolver, bool fromStream)
    : m_cRef(0)
    , m_sourceResolver(sourceResolver)
    , m_fromStream(fromStream)
{
    sourceResolver->AddRef();
}

SourceResolver::State::~State()
{
    m_sourceResolver->Release();
}

STDMETHODIMP SourceResolver::State::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SourceResolver::State::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) SourceResolver::State::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    // For thread safety, return a temporary variable.
    return cRef;
}

IMFSourceResolver* SourceResolver::State::sourceResolver() const
{
    return m_sourceResolver;
}

bool SourceResolver::State::fromStream() const
{
    return m_fromStream;
}

QT_END_NAMESPACE

#include "moc_sourceresolver_p.cpp"
