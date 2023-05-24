#include <QtProtobuf/qprotobufserializer.h>
#include "fieldindexrange.qpb.h"

namespace qtprotobufnamespace::tests {
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarFieldIndexTest1Message(qRegisterProtobufType<FieldIndexTest1Message>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarFieldIndexTest2Message(qRegisterProtobufType<FieldIndexTest2Message>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarFieldIndexTest3Message(qRegisterProtobufType<FieldIndexTest3Message>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarFieldIndexTest4Message(qRegisterProtobufType<FieldIndexTest4Message>);
static bool RegisterFieldindexrangeProtobufTypes = [](){ qRegisterProtobufTypes(); return true; }();
} // namespace qtprotobufnamespace::tests

