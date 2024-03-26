// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLANGUAGESERVERBASE_P_P_H
#define QLANGUAGESERVERBASE_P_P_H

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

#include <QtLanguageServer/private/qlanguageserverbase_p.h>
#include <QtLanguageServer/private/qlanguageserverjsonrpctransport_p.h>

QT_BEGIN_NAMESPACE
namespace QLspSpecification {

class ProtocolBasePrivate
{
public:
    QLanguageServerJsonRpcTransport transport;
    QJsonRpc::TypedRpc typedRpc;
    ProtocolBase::ResponseErrorHandler errorHandler;
    ProtocolBase::GenericRequestHandler undispachedRequestHandler;
    ProtocolBase::GenericNotificationHandler undispachedNotificationHandler;
};

} // namespace QLspSpecification
QT_END_NAMESPACE
#endif // QLANGUAGESERVERBASE_P_P_H
