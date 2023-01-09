%{Cpp:LicenseTemplate}\

#ifndef %{PRIVATEGUARD}
#define %{PRIVATEGUARD}

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

@if '%{Base}' === 'QNode'
#include <Qt3DCore/private/qnode_p.h>
@elsif '%{Base}' === 'QComponent'
#include <Qt3DCore/private/qcomponent_p.h>
@elsif '%{Base}' === 'QEntity'
#include <Qt3DCore/private/qentity_p.h>
@endif

QT_BEGIN_NAMESPACE
%{JS: Cpp.openNamespaces('%{Class}')}
class %{CN}Private : public Qt3DCore::%{Base}Private
{
public:
    %{CN}Private();

    Q_DECLARE_PUBLIC(%{CN})

    // TODO Add member variables
};
@if '%{Base}' === 'QNode' || '%{Base}' === 'QComponent'

struct %{CN}Data
{
    // TODO: Add members that should be sent to the backend
};
@endif
%{JS: Cpp.closeNamespaces('%{Class}')}

QT_END_NAMESPACE

#endif // %{PRIVATEGUARD}\
