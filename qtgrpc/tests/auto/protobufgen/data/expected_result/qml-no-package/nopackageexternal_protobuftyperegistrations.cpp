#include <QtProtobuf/qprotobufserializer.h>
#include "nopackageexternal.qpb.h"

static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarSimpleIntMessageExt(qRegisterProtobufType<SimpleIntMessageExt>);
static bool RegisterNopackageexternalProtobufTypes = [](){ qRegisterProtobufTypes(); return true; }();

