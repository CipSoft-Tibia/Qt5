/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GENERATOR_H
#define GENERATOR_H

#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qmap.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include "config.h"
#include "node.h"
#include "text.h"

QT_BEGIN_NAMESPACE

typedef QMultiMap<QString, Node*> NodeMultiMap;
typedef QMap<Node*, NodeMultiMap> ParentMaps;

class Config;
class CodeMarker;
class Location;
class QDocDatabase;

class Generator
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Generator)

public:
    enum QDocPass { Neither, Prepare, Generate };
    enum ListType { Generic, Obsolete };

    Generator();
    virtual ~Generator();

    virtual bool canHandleFormat(const QString &format) { return format == this->format(); }
    virtual QString format() = 0;
    virtual void generateDocs();
    virtual void initializeGenerator(const Config &config);
    virtual void initializeFormat(const Config &config);
    virtual void terminateGenerator();

    QString fullDocumentLocation(const Node *node, bool useSubdir = false);
    const Config* config() { return config_; }

    static Generator *currentGenerator() { return currentGenerator_; }
    static Generator *generatorForFormat(const QString& format);
    static void initialize(const Config& config);
    static const QString& outputDir() { return outDir_; }
    static const QString& outputSubdir() { return outSubdir_; }
    static void terminate();
    static const QStringList& outputFileNames() { return outFileNames_; }
    static void writeOutFileNames();
    static void augmentImageDirs(QSet<QString>& moreImageDirs);
    static void startDebugging(const QString& message);
    static void stopDebugging(const QString& message);
    static bool debugging();
    static bool noLinkErrors() { return noLinkErrors_; }
    static bool autolinkErrors() { return autolinkErrors_; }
    static void setQDocPass(QDocPass t) { qdocPass_ = t; }
    static bool preparing() { return (qdocPass_ == Prepare); }
    static bool generating() { return (qdocPass_ == Generate); }
    static bool singleExec() { return qdocSingleExec_; }
    static bool writeQaPages() { return qdocWriteQaPages_; }
    static void setSingleExec() { qdocSingleExec_ = true; }
    static void setWriteQaPages() { qdocWriteQaPages_ = true; }
    static QString defaultModuleName() { return project_; }
    static void resetUseOutputSubdirs() { useOutputSubdirs_ = false; }
    static bool useOutputSubdirs() { return useOutputSubdirs_; }
    static void setQmlTypeContext(QmlTypeNode* t) { qmlTypeContext_ = t; }
    static QmlTypeNode* qmlTypeContext() { return qmlTypeContext_; }
    static QString cleanRef(const QString& ref);
    static QString plainCode(const QString& markedCode);

protected:
    void beginSubPage(const Node* node, const QString& fileName);
    void endSubPage();
    virtual QString fileBase(const Node* node) const;
    virtual QString fileExtension() const = 0;
    virtual void generateQAPage() { }
    virtual void generateAlsoList(const Node *node, CodeMarker *marker);
    virtual int generateAtom(const Atom *atom, const Node *relative, CodeMarker *marker);
    virtual void generateBody(const Node *node, CodeMarker *marker);
    virtual void generateCppReferencePage(Node* node, CodeMarker* marker);
    virtual void generateQmlTypePage(QmlTypeNode* , CodeMarker* ) { }
    virtual void generateQmlBasicTypePage(QmlBasicTypeNode* , CodeMarker* ) { }
    virtual void generateDocumentNode(DocumentNode* dn, CodeMarker* marker);
    virtual void generateCollectionNode(CollectionNode* cn, CodeMarker* marker);
    virtual void generateInheritedBy(const ClassNode *classe, CodeMarker *marker);
    virtual void generateInherits(const ClassNode *classe, CodeMarker *marker);
    virtual void generateDocumentation(Node* node);
    virtual void generateMaintainerList(const Aggregate* node, CodeMarker* marker);
    virtual void generateQmlInheritedBy(const QmlTypeNode* qcn, CodeMarker* marker);
    virtual void generateQmlInherits(QmlTypeNode* qcn, CodeMarker* marker);
    virtual bool generateQmlText(const Text& text,
                                 const Node *relative,
                                 CodeMarker *marker,
                                 const QString& qmlName);
    virtual bool generateText(const Text& text, const Node *relative, CodeMarker *marker);
    virtual QString imageFileName(const Node *relative, const QString& fileBase);
    virtual int skipAtoms(const Atom *atom, Atom::AtomType type) const;
    virtual QString typeString(const Node *node);

    static bool matchAhead(const Atom *atom, Atom::AtomType expectedAtomType);
    static QString outputPrefix(const Node* node);
    static QString outputSuffix(const Node* node);
    static void singularPlural(Text& text, const NodeList& nodes);
    static void supplementAlsoList(const Node *node, QList<Text> &alsoList);
    static QString trimmedTrailing(const QString &string,
                                   const QString &prefix,
                                   const QString &suffix);
    void initializeTextOutput();
    QString fileName(const Node* node, const QString &extension = QString()) const;
    QMap<QString, QString> &formattingLeftMap();
    QMap<QString, QString> &formattingRightMap();
    const Atom* generateAtomList(const Atom *atom,
                                 const Node *relative,
                                 CodeMarker *marker,
                                 bool generate,
                                 int& numGeneratedAtoms);
    void generateExampleFiles(const DocumentNode *dn, CodeMarker *marker);
    void generateFileList(const DocumentNode* dn,
                          CodeMarker* marker,
                          Node::DocSubtype subtype,
                          const QString& regExp = QString());
    void generateSince(const Node *node, CodeMarker *marker);
    void generateStatus(const Node *node, CodeMarker *marker);
    void generatePrivateSignalNote(const Node* node, CodeMarker* marker);
    void generateInvokableNote(const Node* node, CodeMarker* marker);
    void generateThreadSafeness(const Node *node, CodeMarker *marker);
    QString getMetadataElement(const Aggregate* inner, const QString& t);
    QStringList getMetadataElements(const Aggregate* inner, const QString& t);
    void generateOverloadedSignal(const Node *node, CodeMarker *marker);
    QString indent(int level, const QString& markedCode);
    QTextStream& out();
    QString outFileName();
    bool parseArg(const QString& src,
                  const QString& tag,
                  int* pos,
                  int n,
                  QStringRef* contents,
                  QStringRef* par1 = 0,
                  bool debug = false);
    void setImageFileExtensions(const QStringList& extensions);
    void unknownAtom(const Atom *atom);
    int appendSortedQmlNames(Text& text, const Node* base, const NodeList& subs);

    QMap<QString, QStringList> editionGroupMap;
    QMap<QString, QStringList> editionModuleMap;
    QString naturalLanguage;
#ifndef QT_NO_TEXTCODEC
    QTextCodec* outputCodec;
    QString outputEncoding;
#endif
    QString tagFile_;
    QStack<QTextStream*> outStreamStack;

    void appendFullName(Text& text,
                        const Node *apparentNode,
                        const Node *relative,
                        const Node *actualNode = 0);
    void appendFullName(Text& text,
                        const Node *apparentNode,
                        const QString& fullName,
                        const Node *actualNode);
    void appendFullNames(Text& text, const NodeList& nodes, const Node* relative);
    int appendSortedNames(Text& text, const ClassNode *classe, const QList<RelatedClass> &classes);
    void appendSignature(Text& text, const Node* node);
    void signatureList(const NodeList& nodes, const Node* relative, CodeMarker* marker);

private:
    static Generator* currentGenerator_;
    static QStringList exampleDirs;
    static QStringList exampleImgExts;
    static QMap<QString, QMap<QString, QString> > fmtLeftMaps;
    static QMap<QString, QMap<QString, QString> > fmtRightMaps;
    static QList<Generator *> generators;
    static QStringList imageDirs;
    static QStringList imageFiles;
    static QMap<QString, QStringList> imgFileExts;
    static QString project_;
    static QString outDir_;
    static QString outSubdir_;
    static QStringList outFileNames_;
    static QSet<QString> outputFormats;
    static QHash<QString, QString> outputPrefixes;
    static QHash<QString, QString> outputSuffixes;
    static QStringList scriptDirs;
    static QStringList scriptFiles;
    static QStringList styleDirs;
    static QStringList styleFiles;
    static bool noLinkErrors_;
    static bool autolinkErrors_;
    static bool redirectDocumentationToDevNull_;
    static QDocPass qdocPass_;
    static bool qdocSingleExec_;
    static bool qdocWriteQaPages_;
    static bool useOutputSubdirs_;
    static QmlTypeNode* qmlTypeContext_;

    void generateReimplementedFrom(const FunctionNode *func, CodeMarker *marker);
    static bool compareNodes(Node *a, Node *b) { return (a->name() < b->name()); }
    static void copyTemplateFiles(const Config &config,
                                  const QString &configVar,
                                  const QString &subDir);

 protected:
    const Config* config_;
    QDocDatabase* qdb_;
    bool inLink_;
    bool inContents_;
    bool inSectionHeading_;
    bool inTableHeader_;
    bool threeColumnEnumValueTable_;
    bool showInternal_;
    bool singleExec_;
    bool quoting_;
    int numTableRows_;
    QString link_;
    QString sectionNumber_;
};

QT_END_NAMESPACE

#endif
