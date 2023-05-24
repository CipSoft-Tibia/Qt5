// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GRPCTEMPLATES_H
#define GRPCTEMPLATES_H

namespace QtGrpc {

class GrpcTemplates
{
public:
    // gRPC
    static const char *ChildClassDeclarationTemplate();

    static const char *ClientQmlDeclarationTemplate();

    static const char *GrpcClientFileSuffix();
    static const char *GrpcServiceFileSuffix();

    static const char *ClientConstructorDefinitionTemplate();

    static const char *ClientMethodDeclarationSyncTemplate();
    static const char *ClientMethodDeclarationAsyncTemplate();
    static const char *ClientMethodDeclarationAsync2Template();
    static const char *ClientMethodDeclarationQmlTemplate();

    static const char *ServerMethodDeclarationTemplate();

    static const char *ClientMethodDefinitionSyncTemplate();
    static const char *ClientMethodDefinitionAsyncTemplate();
    static const char *ClientMethodDefinitionAsync2Template();
    static const char *ClientMethodDefinitionQmlTemplate();

    static const char *ClientMethodServerStreamDeclarationTemplate();

    static const char *ClientMethodServerStreamDefinitionTemplate();
};

} // namespace QtGrpc

#endif // GRPCTEMPLATES_H
