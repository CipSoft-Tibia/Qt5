// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Tatyana Borisova <tanusshhka@mail.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTPROTOCCOMMON_TEMPLATES_H
#define QTPROTOCCOMMON_TEMPLATES_H

#include <unordered_map>
#include <set>
#include <string>
#include <google/protobuf/descriptor.h>

namespace qtprotoccommon {

class CommonTemplates
{
public:
    static const std::vector<std::string> &ListOfQmlExceptions();
    static const std::set<std::string_view> &ListOfCppExceptions();
    static const char *ProtoSuffix();
    static const char *DefaultProtobufIncludesTemplate();
    static const char *DefaultSystemIncludesTemplate();
    static const char *DefaultQtIncludesTemplate();
    static const char *QmlProtobufIncludesTemplate();
    static const char *PreambleTemplate();
    static const char *FooterTemplate();
    static const char *DisclaimerTemplate();
    static const char *InternalIncludeTemplate();
    static const char *ExternalIncludeTemplate();
    static const char *MetaTypeRegistrationDeclaration();
    static const char *EnumRegistrationDeclaration();
    static const char *EnumRegistrationDeclarationNoExport();
    static const char *MetaTypeRegistrationMessageDefinition();
    static const char *MetaTypeRegistrationGlobalEnumDefinition();
    static const char *MetaTypeRegistrationGlobalEnumTemplate();
    static const char *UsingMessageTemplate();
    static const char *UsingMapTemplate();
    static const char *UsingNestedMessageTemplate();
    static const char *UsingRepeatedEnumTemplate();
    static const char *UsingEnumTemplate();
    static const char *NamespaceTemplate();
    static const char *NamespaceClosingTemplate();
    static const char *EnumGadgetDeclarationTemplate();
    static const char *QNamespaceDeclarationTemplate();
    static const char *QNamespaceDeclarationNoExportTemplate();
    static const char *ClassMessageForwardDeclarationTemplate();
    static const char *EnumForwardDeclarationTemplate();
    static const char *EnumClassForwardDeclarationTemplate();
    static const char *ClassMessageBeginDeclarationTemplate();
    static const char *ClassMessageBeginDeclarationTemplateEmptyMacros();
    static const char *ClassMessageQmlBeginDeclarationTemplate();
    static const char *ClassMessageDataBeginDeclarationTemplate();
    static const char *ConstructorMessageDataDefinitionTemplate();
    static const char *CopyConstructorMessageDataDefinitionTemplate();

    static const char *UseNamespace();

    static const char *PropertyTemplate();
    static const char *PropertyRepeatedTemplate();
    static const char *PropertyRepeatedMessageTemplate();
    static const char *PropertyMessageTemplate();
    static const char *PropertyQmlMessageTemplate();
    static const char *PropertyOneofTemplate();
    static const char *PropertyOneofMessageTemplate();
    static const char *PropertyHasOneofTemplate();

    static const char *ConstructorMessageDeclarationTemplate();
    static const char *DestructorMessageDeclarationTemplate();

    static const char *ConstructorMessageDefinitionTemplate();

    static const char *MemberSharedDataPointerTemplate();
    static const char *MemberTemplate();
    static const char *MemberRepeatedTemplate();
    static const char *MemberMessageTemplate();
    static const char *MemberOneofTemplate();
    static const char *MemberOptionalTemplate();
    static const char *PublicBlockTemplate();
    static const char *PrivateBlockTemplate();
    static const char *EnumDefinitionTemplate();
    static const char *EnumClassDefinitionTemplate();
    static const char *EnumFieldTemplate();
    static const char *CopyConstructorDeclarationTemplate();
    static const char *MoveConstructorDeclarationTemplate();
    static const char *CopyConstructorDefinitionTemplate();
    static const char *MoveConstructorDefinitionTemplate();
    static const char *AssignmentOperatorDeclarationTemplate();
    static const char *AssignmentOperatorDefinitionTemplate();
    static const char *MoveAssignmentOperatorDeclarationTemplate();
    static const char *MoveAssignmentOperatorDefinitionTemplate();
    static const char *EqualOperatorDeclarationTemplate();
    static const char *EqualOperatorDefinitionTemplate();
    static const char *EqualOperatorMemberTemplate();
    static const char *EqualOperatorMemberMessageTemplate();
    static const char *EqualOperatorMemberRepeatedTemplate();
    static const char *EqualOperatorMemberOneofTemplate();
    static const char *NotEqualOperatorDeclarationTemplate();
    static const char *NotEqualOperatorDefinitionTemplate();
    static const char *PrivateGetterMessageDeclarationTemplate();
    static const char *PrivateGetterMessageDefinitionTemplate();
    static const char *ClearMessageDeclarationTemplate();
    static const char *ClearQmlMessageDeclarationTemplate();
    static const char *ClearMessageDefinitionTemplate();
    static const char *GetterMessageDeclarationTemplate();
    static const char *GetterMessageDefinitionTemplate();
    static const char *PrivateGetterOneofDeclarationTemplate();
    static const char *PrivateGetterOneofDefinitionTemplate();
    static const char *PrivateGetterOptionalDefinitionTemplate();
    static const char *PrivateGetterOneofMessageDeclarationTemplate();
    static const char *PrivateGetterOneofMessageDefinitionTemplate();
    static const char *GetterOneofFieldNumberDeclarationTemplate();
    static const char *GetterOneofFieldNumberDefinitionTemplate();
    static const char *GetterOneofDeclarationTemplate();
    static const char *GetterOneofDefinitionTemplate();
    static const char *GetterOptionalDefinitionTemplate();
    static const char *GetterOneofMessageDeclarationTemplate();
    static const char *GetterOneofMessageDefinitionTemplate();
    static const char *GetterDeclarationTemplate();
    static const char *GetterDefinitionTemplate();
    static const char *GetterComplexDeclarationTemplate();
    static const char *GetterComplexDefinitionTemplate();
    static const char *PrivateSetterMessageDeclarationTemplate();
    static const char *PrivateSetterMessageDefinitionTemplate();
    static const char *SetterMessageDeclarationTemplate();
    static const char *SetterMessageDefinitionTemplate();
    static const char *SetterComplexDeclarationTemplate();
    static const char *SetterComplexDefinitionTemplate();
    static const char *SetterDeclarationTemplate();
    static const char *SetterDefinitionTemplate();
    static const char *SetterFloatingPointDefinitionTemplate();
    static const char *SetterOneofDeclarationTemplate();
    static const char *SetterOneofDefinitionTemplate();
    static const char *SetterOptionalDefinitionTemplate();
    static const char *PrivateSetterOneofDeclarationTemplate();
    static const char *PrivateSetterOneofDefinitionTemplate();
    static const char *PrivateSetterOptionalDefinitionTemplate();
    static const char *PrivateSetterOneofMessageDeclarationTemplate();
    static const char *PrivateSetterOneofMessageDefinitionTemplate();
    static const char *ClearOneofDeclarationTemplate();
    static const char *ClearQmlOneofDeclarationTemplate();
    static const char *ClearOneofDefinitionTemplate();
    static const char *ClearOptionalDefinitionTemplate();
    static const char *JsonNameOffsetsUintDataTemplate();
    static const char *FieldNumbersUintDataTemplate();
    static const char *QtPropertyIndicesUintDataTemplate();
    static const char *FieldFlagsUintDataTemplate();
    static const char *PropertyOrderingDataOpeningTemplate();
    static const char *PropertyOrderingDataClosingTemplate();
    static const char *PropertyOrderingDefinitionTemplate();
    static const char *SimpleBlockEnclosureTemplate();
    static const char *SemicolonBlockEnclosureTemplate();
    static const char *InitializerMemberTemplate();
    static const char *CopyInitializerMemberTemplate();
    static const char *CopyInitializerMemberMessageTemplate();
    static const char *CopyInitializerMemberOneofTemplate();
    static const char *EmptyBracesTemplate();
    static const char *DeclareMetaTypeTemplate();
    static const char *MetaTypeRegistrationLocalEnumTemplate();
    static const char *MetaTypeRegistrationMapTemplate();
    static const char *QEnumNSTemplate();
    static const char *RegisterEnumSerializersTemplate();
    static const char *RegistrarTemplate();
    static const char *ProtobufTypeRegistrarTemplate();
    static const char *RegistrarEnumTemplate();
    static const char *QmlRegisterGlobalEnumTypeTemplate();
    static const char *QmlRegisterMessageTypeTemplate();

    static const char *RepeatedSuffix();
    static const char *ProtoFileSuffix();
    static const char *EnumClassSuffix();

    static const std::unordered_map<::google::protobuf::FieldDescriptor::Type, std::string> &
    TypeReflection();

    static const char *QtProtobufNamespace();
    static const char *QtProtobufNestedNamespace();

    static const char *DataClassName();

    static const char *FieldEnumTemplate();
    static const char *FieldNumberTemplate();
    static const char *QtProtobufFieldEnum();

    static const char *ExportMacroTemplate();

    static const char *QmlNamedElement();
    static const char *QObjectConstructorMessageDeclarationTemplate();
    static const char *MocIncludeTemplate();
};

} // namespace qtprotoccommon
#endif // QTPROTOCCOMMON_TEMPLATES_H
