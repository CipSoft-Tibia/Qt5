#include <QtProtobuf/qprotobufserializer.h>
#include "anymessages.qpb.h"

namespace qtproto::tests {
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarAnyMessage(qRegisterProtobufType<AnyMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarRepeatedAnyMessage(qRegisterProtobufType<RepeatedAnyMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarTwoAnyMessage(qRegisterProtobufType<TwoAnyMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarExample(qRegisterProtobufType<Example>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarSimpleMessage(qRegisterProtobufType<SimpleMessage>);
static bool RegisterAnymessagesProtobufTypes = [](){ qRegisterProtobufTypes(); return true; }();
} // namespace qtproto::tests

