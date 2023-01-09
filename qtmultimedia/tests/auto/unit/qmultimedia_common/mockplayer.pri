INCLUDEPATH *= $$PWD \
    ../../../src/multimedia \
    ../../../src/multimedia/audio \
    ../../../src/multimedia/video \

HEADERS *= \
    ../qmultimedia_common/mockmediaplayerservice.h \
    ../qmultimedia_common/mockmediaplayercontrol.h \
    ../qmultimedia_common/mockmediastreamscontrol.h \
    ../qmultimedia_common/mockmedianetworkaccesscontrol.h \
    ../qmultimedia_common/mockvideoprobecontrol.h \
    ../qmultimedia_common/mockaudiorolecontrol.h \
    ../qmultimedia_common/mockcustomaudiorolecontrol.h

include(mockvideo.pri)
