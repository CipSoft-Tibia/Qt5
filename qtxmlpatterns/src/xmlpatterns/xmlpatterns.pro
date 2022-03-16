TARGET     = QtXmlPatterns
CONFIG += exceptions
QT = core-private network

DEFINES += QT_NO_USING_NAMESPACE QT_ENABLE_QEXPLICITLYSHAREDDATAPOINTER_STATICCAST
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x61000000

QMAKE_DOCS = $$PWD/doc/qtxmlpatterns.qdocconf

include($$PWD/common.pri)
include($$PWD/acceltree/acceltree.pri)
include($$PWD/api/api.pri)
include($$PWD/data/data.pri)
include($$PWD/environment/environment.pri)
include($$PWD/expr/expr.pri)
include($$PWD/functions/functions.pri)
include($$PWD/iterators/iterators.pri)
include($$PWD/janitors/janitors.pri)
include($$PWD/parser/parser.pri)
include($$PWD/projection/projection.pri)

qtConfig(xml-schema) {
    include($$PWD/schema/schema.pri)
}
include($$PWD/type/type.pri)
include($$PWD/utils/utils.pri)
include($$PWD/qobjectmodel/qobjectmodel.pri, "", true)

load(qt_module)
