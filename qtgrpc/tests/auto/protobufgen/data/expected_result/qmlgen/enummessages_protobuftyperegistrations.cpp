#include <QtProtobuf/qprotobufserializer.h>
#include "enummessages.qpb.h"

namespace qtprotobufnamespace::tests {
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarSimpleEnumMessage(qRegisterProtobufType<SimpleEnumMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarRepeatedEnumMessage(qRegisterProtobufType<RepeatedEnumMessage>);
namespace MixedEnumUsageMessage_QtProtobufNested {
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarNestedEnumHolder(qRegisterProtobufType<NestedEnumHolder>);
namespace NestedEnumHolderLevel1_QtProtobufNested {
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarNestedEnumHolderLevel2(qRegisterProtobufType<NestedEnumHolderLevel2>);
} // namespace NestedEnumHolderLevel1_QtProtobufNested
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarNestedEnumHolderLevel1(qRegisterProtobufType<NestedEnumHolderLevel1>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarNestedEnumMessage(qRegisterProtobufType<NestedEnumMessage>);
} // namespace MixedEnumUsageMessage_QtProtobufNested
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarMixedEnumUsageMessage(qRegisterProtobufType<MixedEnumUsageMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarSimpleFileEnumMessage(qRegisterProtobufType<SimpleFileEnumMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarStepChildEnumMessage(qRegisterProtobufType<StepChildEnumMessage>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarA(qRegisterProtobufType<A>);
static QtProtobuf::ProtoTypeRegistrar ProtoTypeRegistrarB(qRegisterProtobufType<B>);
static bool RegisterEnummessagesProtobufTypes = [](){ qRegisterProtobufTypes(); return true; }();
} // namespace qtprotobufnamespace::tests

