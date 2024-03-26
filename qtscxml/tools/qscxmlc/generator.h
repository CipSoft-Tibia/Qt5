// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GENERATOR_H
#define GENERATOR_H

#include "moc.h"

// -- QtScxml
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qiodevice.h>
// -- QtScxml

QT_BEGIN_NAMESPACE

class Generator
{
    QIODevice &out; // -- QtScxml
    ClassDef *cdef;
    QList<uint> meta_data;

public:
    Generator(ClassDef *classDef, const QList<QByteArray> &metaTypes,
              const QHash<QByteArray, QByteArray> &knownQObjectClasses,
              const QHash<QByteArray, QByteArray> &knownGadgets,
              QIODevice &outfile, // -- QtScxml
              bool requireCompleteTypes = false);
    void generateCode();

// -- QtScxml
    void generateAccessorDefs();
    void generateSignalDefs();
// -- QtScxml

private:
    bool registerableMetaType(const QByteArray &propertyType);
    void registerClassInfoStrings();
    void generateClassInfos();
    void registerFunctionStrings(const QList<FunctionDef> &list);
    void registerByteArrayVector(const QList<QByteArray> &list);
    void generateFunctions(const QList<FunctionDef> &list, const char *functype, int type,
                           int &paramsIndex, int &initialMetatypeOffset);
    void generateFunctionRevisions(const QList<FunctionDef> &list, const char *functype);
    void generateFunctionParameters(const QList<FunctionDef> &list, const char *functype);
    void generateTypeInfo(const QByteArray &typeName, bool allowEmptyName = false);
    void registerEnumStrings();
    void generateEnums(int index);
    void registerPropertyStrings();
    void generateProperties();
    void generateMetacall();
    void generateStaticMetacall();
    void generateSignal(FunctionDef *def, int index);
    void generateQPropertyApi();
#if 0 // -- QtScxml
    void generatePluginMetaData();
#endif // -- QtScxml
    QMultiMap<QByteArray, int> automaticPropertyMetaTypesHelper();
    QMap<int, QMultiMap<QByteArray, int>>
    methodsWithAutomaticTypesHelper(const QList<FunctionDef> &methodList);

    void strreg(const QByteArray &); // registers a string
    int stridx(const QByteArray &); // returns a string's id
    QList<QByteArray> strings;
    QByteArray purestSuperClass;
    QList<QByteArray> metaTypes;
    QHash<QByteArray, QByteArray> knownQObjectClasses;
    QHash<QByteArray, QByteArray> knownGadgets;
    bool requireCompleteTypes;
};

QT_END_NAMESPACE

#endif // GENERATOR_H
