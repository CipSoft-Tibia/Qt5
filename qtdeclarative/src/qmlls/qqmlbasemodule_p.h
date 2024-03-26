// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLBASEMODULE_P_H
#define QQMLBASEMODULE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qlanguageserver_p.h"
#include "qqmlcodemodel_p.h"
#include "qqmllsutils_p.h"

#include <QObject>
#include <unordered_map>

template<typename ParametersT, typename ResponseT>
struct BaseRequest
{
    // allow using Parameters and Response type aliases in the
    // implementations of the different requests.
    using Parameters = ParametersT;
    using Response = ResponseT;

    // The version of the code on which the typedefinition request was made.
    // Request is received: mark it with the current version of the textDocument.
    // Then, wait for the codemodel to finish creating a snapshot version that is newer or equal to
    // the textDocument version at request-received-time.
    int m_minVersion;
    Parameters m_parameters;
    Response m_response;

    bool fillFrom(QmlLsp::OpenDocument doc, const Parameters &params, Response &&response);
};

template<typename RequestType>
struct QQmlBaseModule : public QLanguageServerModule
{
    using RequestParameters = typename RequestType::Parameters;
    using RequestResponse = typename RequestType::Response;
    using RequestPointer = std::unique_ptr<RequestType>;
    using RequestPointerArgument = RequestPointer &&;
    using BaseT = QQmlBaseModule<RequestType>;

    QQmlBaseModule(QmlLsp::QQmlCodeModel *codeModel);
    ~QQmlBaseModule();

    void requestHandler(const RequestParameters &parameters, RequestResponse &&response);
    decltype(auto) getRequestHandler();
    // processes a request in a different thread.
    virtual void process(RequestPointerArgument toBeProcessed) = 0;
    std::optional<QList<QQmlLSUtilsItemLocation>> itemsForRequest(const RequestPointer &request);

public Q_SLOTS:
    void updatedSnapshot(const QByteArray &uri);

protected:
    QMutex m_pending_mutex;
    std::unordered_multimap<QString, RequestPointer> m_pending;
    QmlLsp::QQmlCodeModel *m_codeModel;
};

template<typename Parameters, typename Response>
bool BaseRequest<Parameters, Response>::fillFrom(QmlLsp::OpenDocument doc, const Parameters &params,
                                                 Response &&response)
{
    Q_UNUSED(doc);
    m_parameters = params;
    m_response = std::move(response);

    if (!doc.textDocument) {
        qDebug() << "Cannot find document in qmlls's codemodel, did you open it before accessing "
                    "it?";
        return false;
    }

    {
        QMutexLocker l(doc.textDocument->mutex());
        m_minVersion = doc.textDocument->version().value_or(0);
    }
    return true;
}

template<typename RequestType>
QQmlBaseModule<RequestType>::QQmlBaseModule(QmlLsp::QQmlCodeModel *codeModel)
    : m_codeModel(codeModel)
{
    QObject::connect(m_codeModel, &QmlLsp::QQmlCodeModel::updatedSnapshot, this,
                     &QQmlBaseModule<RequestType>::updatedSnapshot);
}

template<typename RequestType>
QQmlBaseModule<RequestType>::~QQmlBaseModule()
{
    QMutexLocker l(&m_pending_mutex);
    m_pending.clear(); // empty the m_pending while the mutex is hold
}

template<typename RequestType>
decltype(auto) QQmlBaseModule<RequestType>::getRequestHandler()
{
    auto handler = [this](const QByteArray &, const RequestParameters &parameters,
                          RequestResponse &&response) {
        requestHandler(parameters, std::move(response));
    };
    return handler;
}

template<typename RequestType>
void QQmlBaseModule<RequestType>::requestHandler(const RequestParameters &parameters,
                                                 RequestResponse &&response)
{
    auto req = std::make_unique<RequestType>();
    QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(parameters.textDocument.uri));

    if (!req->fillFrom(doc, parameters, std::move(response))) {
        req->m_response.sendErrorResponse(0, "Received invalid request", parameters);
        return;
    }
    const int minVersion = req->m_minVersion;
    {
        QMutexLocker l(&m_pending_mutex);
        m_pending.insert({ QString::fromUtf8(req->m_parameters.textDocument.uri), std::move(req) });
    }

    if (doc.snapshot.docVersion && *doc.snapshot.docVersion >= minVersion)
        updatedSnapshot(QQmlLSUtils::lspUriToQmlUrl(parameters.textDocument.uri));
}

template<typename RequestType>
void QQmlBaseModule<RequestType>::updatedSnapshot(const QByteArray &url)
{
    QmlLsp::OpenDocumentSnapshot doc = m_codeModel->snapshotByUrl(url);
    std::vector<RequestPointer> toCompl;
    {
        QMutexLocker l(&m_pending_mutex);
        for (auto [it, end] = m_pending.equal_range(QString::fromUtf8(url)); it != end;) {
            if (auto &[key, value] = *it;
                doc.docVersion && value->m_minVersion <= *doc.docVersion) {
                toCompl.push_back(std::move(value));
                it = m_pending.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto it = toCompl.rbegin(), end = toCompl.rend(); it != end; ++it) {
        process(std::move(*it));
    }
}

template<typename RequestType>
std::optional<QList<QQmlLSUtilsItemLocation>>
QQmlBaseModule<RequestType>::itemsForRequest(const RequestPointer &request)
{

    QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));

    QQmlJS::Dom::DomItem file = doc.snapshot.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (auto envPtr = file.environment().ownerAs<QQmlJS::Dom::DomEnvironment>())
        envPtr->clearReferenceCache();
    if (!file) {
        qWarning() << u"Could not find file in Dom Environment from Codemodel :"_s
                   << doc.snapshot.doc.toString();
        return {};
    }

    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, request->m_parameters.position.line,
                                                         request->m_parameters.position.character);

    if (itemsFound.isEmpty()) {
        qWarning() << u"Could not find any items at given text location."_s;
        return {};
    }
    return itemsFound;
}

#endif // QQMLBASEMODULE_P_H
