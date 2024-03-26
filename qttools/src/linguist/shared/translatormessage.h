// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TRANSLATORMESSAGE_H
#define TRANSLATORMESSAGE_H

#include <QString>
#include <QStringList>
#include <QHash>


QT_BEGIN_NAMESPACE

enum TranslatorSaveMode { SaveEverything, SaveStripped };

class TranslatorMessage
{
public:
    enum Type { Unfinished, Finished, Vanished, Obsolete };
    typedef QHash<QString, QString> ExtraData;
    class Reference
    {
        QString m_fileName;
        int m_lineNumber;
    public:
        Reference(const QString &n, int l) : m_fileName(n), m_lineNumber(l) {}
        bool operator==(const Reference &other) const
            { return fileName() == other.fileName() && lineNumber() == other.lineNumber(); }
        QString fileName() const { return m_fileName; }
        int lineNumber() const { return m_lineNumber; }
    };
    typedef QList<Reference> References;

    TranslatorMessage();
    TranslatorMessage(const QString &context, const QString &sourceText,
        const QString &comment, const QString &userData,
        const QString &fileName, int lineNumber,
        const QStringList &translations = QStringList(),
        Type type = Unfinished, bool plural = false);

    uint hash() const;

    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }

    QString context() const { return m_context; }
    void setContext(const QString &context) { m_context = context; }

    QString sourceText() const { return m_sourcetext; }
    void setSourceText(const QString &sourcetext) { m_sourcetext = sourcetext; }
    QString oldSourceText() const { return m_oldsourcetext; }
    void setOldSourceText(const QString &oldsourcetext) { m_oldsourcetext = oldsourcetext; }

    QString comment() const { return m_comment; }
    void setComment(const QString &comment) { m_comment = comment; }
    QString oldComment() const { return m_oldcomment; }
    void setOldComment(const QString &oldcomment) { m_oldcomment = oldcomment; }

    QStringList translations() const { return m_translations; }
    void setTranslations(const QStringList &translations) { m_translations = translations; }
    QString translation() const { return m_translations.value(0); }
    void setTranslation(const QString &translation) { m_translations = QStringList(translation); }
    void appendTranslation(const QString &translation) { m_translations.append(translation); }
    bool isTranslated() const
    {
        for (const QString &trans : m_translations)
            if (!trans.isEmpty())
                return true;
        return false;
    }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    int lineNumber() const { return m_lineNumber; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }
    int tsLineNumber() const { return m_tsLineNumber; }
    void setTsLineNumber(int lineNumber) { m_tsLineNumber = lineNumber; }
    void clearReferences();
    void setReferences(const References &refs);
    void addReference(const QString &fileName, int lineNumber);
    void addReference(const Reference &ref) { addReference(ref.fileName(), ref.lineNumber()); }
    void addReferenceUniq(const QString &fileName, int lineNumber);
    References extraReferences() const { return m_extraRefs; }
    References allReferences() const;
    QString userData() const { return m_userData; }
    void setUserData(const QString &userData) { m_userData = userData; }
    QString extraComment() const { return m_extraComment; }
    void setExtraComment(const QString &extraComment) { m_extraComment = extraComment; }
    QString translatorComment() const { return m_translatorComment; }
    void setTranslatorComment(const QString &translatorComment) { m_translatorComment = translatorComment; }
    QString warning() const {return m_warning;}
    void setWarning(const QString &warning) {m_warning = warning;}


    bool isNull() const { return m_sourcetext.isNull() && m_lineNumber == -1 && m_translations.isEmpty(); }

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }
    bool isPlural() const { return m_plural; }
    void setPlural(bool isplural) { m_plural = isplural; }

    // note: use '<fileformat>:' as prefix for file format specific members,
    // e.g. "po-msgid_plural"
    QString extra(const QString &ba) const;
    void setExtra(const QString &ba, const QString &var);
    bool hasExtra(const QString &ba) const;
    const ExtraData &extras() const { return m_extra; }
    void setExtras(const ExtraData &extras) { m_extra = extras; }
    void unsetExtra(const QString &key);

    bool warningOnly() const { return m_warningOnly; }
    void setWarningOnly(bool isWarningOnly) { m_warningOnly = isWarningOnly; }

    void dump() const;

private:
    QString     m_id;
    QString     m_context;
    QString     m_sourcetext;
    QString     m_oldsourcetext;
    QString     m_comment;
    QString     m_oldcomment;
    QString     m_userData;
    ExtraData   m_extra; // PO flags, PO plurals
    QString     m_extraComment;
    QString     m_translatorComment;
    QString     m_warning;
    QStringList m_translations;
    QString     m_fileName;
    int         m_lineNumber;
    int         m_tsLineNumber = -1;
    References  m_extraRefs;
    bool        m_warningOnly = false;

    Type m_type;
    bool m_plural;
};

Q_DECLARE_TYPEINFO(TranslatorMessage, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // TRANSLATORMESSAGE_H
