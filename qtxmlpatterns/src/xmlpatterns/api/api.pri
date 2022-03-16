HEADERS += $$PWD/qabstractxmlforwarditerator_p.h \
    $$PWD/qabstractmessagehandler.h \
    $$PWD/qabstracturiresolver.h \
    $$PWD/qabstractxmlnodemodel.h \
    $$PWD/qabstractxmlnodemodel_p.h \
    $$PWD/qabstractxmlpullprovider_p.h \
    $$PWD/qabstractxmlreceiver.h \
    $$PWD/qabstractxmlreceiver_p.h \
    $$PWD/qdeviceresourceloader_p.h \
    $$PWD/qiodevicedelegate_p.h \
    $$PWD/qnetworkaccessdelegator_p.h \
    $$PWD/qpullbridge_p.h \
    $$PWD/qresourcedelegator_p.h \
    $$PWD/qsimplexmlnodemodel.h \
    $$PWD/qsourcelocation.h \
    $$PWD/qtxmlpatternsglobal.h \
    $$PWD/quriloader_p.h \
    $$PWD/qvariableloader_p.h \
    $$PWD/qxmlformatter.h \
    $$PWD/qxmlname.h \
    $$PWD/qxmlnamepool.h \
    $$PWD/qxmlquery.h \
    $$PWD/qxmlquery_p.h \
    $$PWD/qxmlresultitems.h \
    $$PWD/qxmlresultitems_p.h \
    $$PWD/qxmlserializer.h \
    $$PWD/qxmlserializer_p.h \
    $$PWD/qcoloringmessagehandler_p.h \
    $$PWD/qcoloroutput_p.h \
    $$PWD/qxmlpatternistcli_p.h
SOURCES += $$PWD/qvariableloader.cpp \
    $$PWD/qabstractmessagehandler.cpp \
    $$PWD/qabstracturiresolver.cpp \
    $$PWD/qabstractxmlnodemodel.cpp \
    $$PWD/qabstractxmlpullprovider.cpp \
    $$PWD/qabstractxmlreceiver.cpp \
    $$PWD/qiodevicedelegate.cpp \
    $$PWD/qnetworkaccessdelegator.cpp \
    $$PWD/qpullbridge.cpp \
    $$PWD/qresourcedelegator.cpp \
    $$PWD/qsimplexmlnodemodel.cpp \
    $$PWD/qsourcelocation.cpp \
    $$PWD/quriloader.cpp \
    $$PWD/qxmlformatter.cpp \
    $$PWD/qxmlname.cpp \
    $$PWD/qxmlnamepool.cpp \
    $$PWD/qxmlquery.cpp \
    $$PWD/qxmlresultitems.cpp \
    $$PWD/qxmlserializer.cpp \
    $$PWD/qcoloringmessagehandler.cpp \
    $$PWD/qcoloroutput.cpp

qtConfig(xml-schema) {
    HEADERS += $$PWD/qxmlschema.h \
        $$PWD/qxmlschema_p.h \
        $$PWD/qxmlschemavalidator.h \
        $$PWD/qxmlschemavalidator_p.h
    SOURCES += $$PWD/qxmlschema.cpp \
        $$PWD/qxmlschema_p.cpp \
        $$PWD/qxmlschemavalidator.cpp
}
