// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "translatormessage.h"

#include <qplatformdefs.h>

#include <QDataStream>
#include <QDebug>

#include <stdlib.h>


QT_BEGIN_NAMESPACE

TranslatorMessage::TranslatorMessage()
  : m_lineNumber(-1), m_type(Unfinished), m_plural(false)
{
}

TranslatorMessage::TranslatorMessage(const QString &context,
    const QString &sourceText, const QString &comment,
    const QString &userData,
    const QString &fileName, int lineNumber, const QStringList &translations,
    Type type, bool plural)
  : m_context(context), m_sourcetext(sourceText), m_comment(comment),
    m_userData(userData),
    m_translations(translations), m_fileName(fileName), m_lineNumber(lineNumber),
    m_type(type), m_plural(plural)
{
}

void TranslatorMessage::addReference(const QString &fileName, int lineNumber)
{
    if (m_fileName.isEmpty()) {
        m_fileName = fileName;
        m_lineNumber = lineNumber;
    } else {
        m_extraRefs.append(Reference(fileName, lineNumber));
    }
}

void TranslatorMessage::addReferenceUniq(const QString &fileName, int lineNumber)
{
    if (m_fileName.isEmpty()) {
        m_fileName = fileName;
        m_lineNumber = lineNumber;
    } else {
        if (fileName == m_fileName && lineNumber == m_lineNumber)
            return;
        if (!m_extraRefs.isEmpty()) { // Rather common case, so special-case it
            for (const Reference &ref : std::as_const(m_extraRefs)) {
                if (fileName == ref.fileName() && lineNumber == ref.lineNumber())
                    return;
            }
        }
        m_extraRefs.append(Reference(fileName, lineNumber));
    }
}

void TranslatorMessage::clearReferences()
{
    m_fileName.clear();
    m_lineNumber = -1;
    m_extraRefs.clear();
}

void TranslatorMessage::setReferences(const TranslatorMessage::References &refs0)
{
    if (!refs0.isEmpty()) {
        References refs = refs0;
        const Reference &ref = refs.takeFirst();
        m_fileName = ref.fileName();
        m_lineNumber = ref.lineNumber();
        m_extraRefs = refs;
    } else {
        clearReferences();
    }
}

TranslatorMessage::References TranslatorMessage::allReferences() const
{
    References refs;
    if (!m_fileName.isEmpty()) {
        refs.append(Reference(m_fileName, m_lineNumber));
        refs += m_extraRefs;
    }
    return refs;
}


bool TranslatorMessage::hasExtra(const QString &key) const
{
    return m_extra.contains(key);
}

QString TranslatorMessage::extra(const QString &key) const
{
    return m_extra[key];
}

void TranslatorMessage::setExtra(const QString &key, const QString &value)
{
    m_extra[key] = value;
}

void TranslatorMessage::unsetExtra(const QString &key)
{
    m_extra.remove(key);
}

void TranslatorMessage::dump() const
{
    qDebug()
        << "\nId                : " << m_id
        << "\nContext           : " << m_context
        << "\nSource            : " << m_sourcetext
        << "\nComment           : " << m_comment
        << "\nUserData          : " << m_userData
        << "\nExtraComment      : " << m_extraComment
        << "\nTranslatorComment : " << m_translatorComment
        << "\nTranslations      : " << m_translations
        << "\nFileName          : " << m_fileName
        << "\nLineNumber        : " << m_lineNumber
        << "\nType              : " << m_type
        << "\nPlural            : " << m_plural
        << "\nExtra             : " << m_extra;
}


QT_END_NAMESPACE
