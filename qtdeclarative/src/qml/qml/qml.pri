SOURCES += \
    $$PWD/qqmlopenmetaobject.cpp \
    $$PWD/qqmlvmemetaobject.cpp \
    $$PWD/qqmlengine.cpp \
    $$PWD/qqmlexpression.cpp \
    $$PWD/qqmlproperty.cpp \
    $$PWD/qqmlcomponent.cpp \
    $$PWD/qqmlincubator.cpp \
    $$PWD/qqmlcontext.cpp \
    $$PWD/qqmlcustomparser.cpp \
    $$PWD/qqmlpropertyvaluesource.cpp \
    $$PWD/qqmlpropertyvalueinterceptor.cpp \
    $$PWD/qqmlproxymetaobject.cpp \
    $$PWD/qqmlvme.cpp \
    $$PWD/qqmlboundsignal.cpp \
    $$PWD/qqmlmetatype.cpp \
    $$PWD/qqmlstringconverters.cpp \
    $$PWD/qqmlparserstatus.cpp \
    $$PWD/qqmltypeloader.cpp \
    $$PWD/qqmlinfo.cpp \
    $$PWD/qqmlerror.cpp \
    $$PWD/qqmlvaluetype.cpp \
    $$PWD/qqmlcleanup.cpp \
    $$PWD/qqmlpropertycache.cpp \
    $$PWD/qqmlnotifier.cpp \
    $$PWD/qqmltypenotavailable.cpp \
    $$PWD/qqmltypenamecache.cpp \
    $$PWD/qqmlscriptstring.cpp \
    $$PWD/qqmlnetworkaccessmanagerfactory.cpp \
    $$PWD/qqmlextensionplugin.cpp \
    $$PWD/qqmlimport.cpp \
    $$PWD/qqmllist.cpp \
    $$PWD/qqmljavascriptexpression.cpp \
    $$PWD/qqmlabstractbinding.cpp \
    $$PWD/qqmlvaluetypeproxybinding.cpp \
    $$PWD/qqmlglobal.cpp \
    $$PWD/qqmlfile.cpp \
    $$PWD/qqmlplatform.cpp \
    $$PWD/qqmlbinding.cpp \
    $$PWD/qqmlabstracturlinterceptor.cpp \
    $$PWD/qqmlapplicationengine.cpp \
    $$PWD/qqmllistwrapper.cpp \
    $$PWD/qqmlvaluetypewrapper.cpp \
    $$PWD/qqmltypewrapper.cpp \
    $$PWD/qqmlfileselector.cpp \
    $$PWD/qqmlobjectcreator.cpp \
    $$PWD/qqmldirparser.cpp \
    $$PWD/qqmldelayedcallqueue.cpp \
    $$PWD/qqmlloggingcategory.cpp

HEADERS += \
    $$PWD/qqmlglobal_p.h \
    $$PWD/qqmlopenmetaobject_p.h \
    $$PWD/qqmlvmemetaobject_p.h \
    $$PWD/qqml.h \
    $$PWD/qqmlproperty.h \
    $$PWD/qqmlcomponent.h \
    $$PWD/qqmlcomponent_p.h \
    $$PWD/qqmlincubator.h \
    $$PWD/qqmlincubator_p.h \
    $$PWD/qqmlcustomparser_p.h \
    $$PWD/qqmlpropertyvaluesource.h \
    $$PWD/qqmlpropertyvalueinterceptor_p.h \
    $$PWD/qqmlboundsignal_p.h \
    $$PWD/qqmlboundsignalexpressionpointer_p.h \
    $$PWD/qqmlparserstatus.h \
    $$PWD/qqmlproxymetaobject_p.h \
    $$PWD/qqmlvme_p.h \
    $$PWD/qqmlengine_p.h \
    $$PWD/qqmlexpression_p.h \
    $$PWD/qqmlprivate.h \
    $$PWD/qqmlmetatype_p.h \
    $$PWD/qqmlengine.h \
    $$PWD/qqmlcontext.h \
    $$PWD/qqmlexpression.h \
    $$PWD/qqmlstringconverters_p.h \
    $$PWD/qqmlinfo.h \
    $$PWD/qqmlproperty_p.h \
    $$PWD/qqmlcontext_p.h \
    $$PWD/qqmltypeloader_p.h \
    $$PWD/qqmllist.h \
    $$PWD/qqmllist_p.h \
    $$PWD/qqmldata_p.h \
    $$PWD/qqmlerror.h \
    $$PWD/qqmlvaluetype_p.h \
    $$PWD/qqmlcleanup_p.h \
    $$PWD/qqmlpropertycache_p.h \
    $$PWD/qqmlpropertyindex_p.h \
    $$PWD/qqmlnotifier_p.h \
    $$PWD/qqmltypenotavailable_p.h \
    $$PWD/qqmltypenamecache_p.h \
    $$PWD/qqmlscriptstring.h \
    $$PWD/qqmlguard_p.h \
    $$PWD/qqmlnetworkaccessmanagerfactory.h \
    $$PWD/qqmlextensioninterface.h \
    $$PWD/qqmlimport_p.h \
    $$PWD/qqmlextensionplugin.h \
    $$PWD/qqmlscriptstring_p.h \
    $$PWD/qqmlcomponentattached_p.h \
    $$PWD/qqmljavascriptexpression_p.h \
    $$PWD/qqmlabstractbinding_p.h \
    $$PWD/qqmlvaluetypeproxybinding_p.h \
    $$PWD/qqmlfile.h \
    $$PWD/qqmlplatform_p.h \
    $$PWD/qqmlbinding_p.h \
    $$PWD/qqmlextensionplugin_p.h \
    $$PWD/qqmlabstracturlinterceptor.h \
    $$PWD/qqmlapplicationengine_p.h \
    $$PWD/qqmlapplicationengine.h \
    $$PWD/qqmllistwrapper_p.h \
    $$PWD/qqmltypewrapper_p.h \
    $$PWD/qqmlfileselector_p.h \
    $$PWD/qqmlfileselector.h \
    $$PWD/qqmlobjectcreator_p.h \
    $$PWD/qqmldirparser_p.h \
    $$PWD/qqmldelayedcallqueue_p.h \
    $$PWD/qqmlloggingcategory_p.h

qtConfig(qml-xml-http-request) {
    HEADERS += \
        $$PWD/qqmlxmlhttprequest_p.h

    SOURCES += \
        $$PWD/qqmlxmlhttprequest.cpp

}

qtConfig(qml-locale) {
    HEADERS += \
        $$PWD/qqmllocale_p.h

    SOURCES += \
        $$PWD/qqmllocale.cpp
}

include(ftw/ftw.pri)
include(v8/v8.pri)
