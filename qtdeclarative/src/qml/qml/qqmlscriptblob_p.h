// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSCRIPTBLOB_P_H
#define QQMLSCRIPTBLOB_P_H

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

#include <private/qqmltypeloader_p.h>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class Q_AUTOTEST_EXPORT QQmlScriptBlob : public QQmlTypeLoader::Blob
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptBlob(const QUrl &, QQmlTypeLoader *);

public:
    ~QQmlScriptBlob() override;

    struct ScriptReference
    {
        QV4::CompiledData::Location location;
        QString qualifier;
        QString nameSpace;
        QQmlRefPointer<QQmlScriptBlob> script;
    };

    QQmlRefPointer<QQmlScriptData> scriptData() const;

protected:
    void dataReceived(const SourceCodeData &) override;
    void initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *unit) override;
    void done() override;

    QString stringAt(int index) const override;

private:
    void scriptImported(const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location, const QString &qualifier, const QString &nameSpace) override;
    void initializeFromCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit);
    void initializeFromNative(const QV4::Value &value);

    QList<ScriptReference> m_scripts;
    QQmlRefPointer<QQmlScriptData> m_scriptData;
    const bool m_isModule;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTBLOB_P_H
