CONFIG += testcase
TARGET = tst_moc

#exists(/usr/include/boost/spirit.hpp) {
#    message("including boost headers in test")
#    DEFINES += PARSE_BOOST
#    # need to add explicitly so that it ends up in moc's search path
#    INCLUDEPATH += /usr/include
#}

INCLUDEPATH += testproject/include testproject

cross_compile: DEFINES += MOC_CROSS_COMPILED

HEADERS += using-namespaces.h no-keywords.h task87883.h c-comments.h backslash-newlines.h oldstyle-casts.h \
           slots-with-void-template.h qinvokable.h namespaced-flags.h trigraphs.h \
           escapes-in-string-literals.h cstyle-enums.h qprivateslots.h gadgetwithnoenums.h \
           dir-in-include-path.h single_function_keyword.h task192552.h \
           task234909.h task240368.h pure-virtual-signals.h cxx11-enums.h \
           cxx11-final-classes.h \
           cxx11-explicit-override-control.h \
           cxx11-trailing-return.h \
           forward-declared-param.h \
           parse-defines.h \
           function-with-attributes.h \
           plugin_metadata.h \
           single-quote-digit-separator-n3781.h \
           related-metaobjects-in-namespaces.h \
           qtbug-35657-gadget.h \
           non-gadget-parent-class.h grand-parent-gadget-class.h \
           related-metaobjects-in-gadget.h \
           related-metaobjects-name-conflict.h \
           namespace.h cxx17-namespaces.h \
           cxx-attributes.h \
           enum_inc.h enum_with_include.h

# No platform specifics in the JSON files, so that we can compare them
JSON_HEADERS = $$HEADERS
JSON_HEADERS -= cxx-attributes.h
JSON_HEADERS -= enum_inc.h

if(*-g++*|*-icc*|*-clang*|*-llvm):!win32-*: HEADERS += os9-newlines.h win-newlines.h
if(*-g++*|*-clang*): HEADERS += dollars.h
SOURCES += tst_moc.cpp

QT = core testlib
qtHaveModule(dbus):       QT += dbus
qtHaveModule(concurrent): QT += concurrent
qtHaveModule(network):    QT += network
qtHaveModule(sql):        QT += sql

# tst_Moc::specifyMetaTagsFromCmdline()
# Ensure that plugin_metadata.h are moc-ed with some extra -M arguments:
QMAKE_MOC_OPTIONS += -Muri=com.company.app -Muri=com.company.app.private

# Define macro on the command lines used in  parse-defines.h
QMAKE_MOC_OPTIONS += "-DDEFINE_CMDLINE_EMPTY="  "\"-DDEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)\""

QMAKE_MOC_OPTIONS += --output-json

debug_and_release {
    CONFIG(debug, debug|release) {
        MOC_CPP_DIR = $$MOC_DIR/debug
    } else {
        MOC_CPP_DIR = $$MOC_DIR/release
    }
} else {
    MOC_CPP_DIR = $$MOC_DIR
}

moc_json_header.input = JSON_HEADERS
moc_json_header.output = $$MOC_CPP_DIR/$${QMAKE_H_MOD_MOC}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_CPP)}.json
moc_json_header.CONFIG = no_link moc_verify
moc_json_header.depends = $$MOC_CPP_DIR/$${QMAKE_H_MOD_MOC}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_CPP)}
moc_json_header.commands = $$escape_expand(\\n)  # force creation of rule
moc_json_header.variable_out = MOC_JSON_HEADERS

BASELINE_IN = allmocs_baseline_in.json
copy_baseline.commands = $${QMAKE_COPY} $$shell_path(${QMAKE_FILE_NAME}) ${QMAKE_FILE_OUT}
copy_baseline.input = BASELINE_IN
copy_baseline.output = $$OUT_PWD/allmocs_baseline.json
copy_baseline.CONFIG = no_link

qtPrepareTool(MOC_COLLECT_JSON, moc)
jsoncollector.CONFIG += combine
jsoncollector.commands = $$MOC_COLLECT_JSON --collect-json -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
jsoncollector.input = MOC_JSON_HEADERS
jsoncollector.output = $$OUT_PWD/allmocs.json
jsoncollector.variable_out = GENERATED_FILES

allmocs_contents = \
    "<!DOCTYPE RCC><RCC version=\"1.0\">"\
    "<qresource prefix=\"/\">"\
    "<file>allmocs.json</file>"\
    "<file>allmocs_baseline.json</file>"\
    "</qresource>"\
    "</RCC>"

allmocs_file = $$OUT_PWD/allmocs.qrc

!write_file($$allmocs_file, allmocs_contents): error()
RESOURCES += $$allmocs_file

QMAKE_EXTRA_COMPILERS += moc_json_header copy_baseline jsoncollector
