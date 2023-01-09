import QtQuick 2.0

Item {
    property bool success: false
    required property QtObject componentCache

    function reportError(s) { console.warn(s) }

    Component.onCompleted: {
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMETransientEmptyComponent.1.qml')) return reportError('Test component not loaded')
        if (componentCache.isTypeLoaded('VMETransientEmptyComponent.qml')) return reportError('Transient component already loaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already loaded')

        var comp = Qt.createComponent('VMETransientEmptyComponent.qml')
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMETransientEmptyComponent.1.qml')) return reportError('Test component not loaded 2')
        if (!componentCache.isTypeLoaded('VMETransientEmptyComponent.qml')) return reportError('Transient component not loaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already loaded')

        var obj = comp.createObject()
        if (!obj) return reportError('Invalid object')
        if (obj.x == undefined) return reportError('Invalid object 2')
        if (!obj.p) return reportError('Invalid object 3')
        if (obj.p.x == undefined) return reportError('Invalid object 4')
        if (!componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not loaded')

        obj.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMETransientEmptyComponent.1.qml')) return reportError('Test component not loaded 3')
        if (!componentCache.isTypeLoaded('VMETransientEmptyComponent.qml')) return reportError('Transient component already unloaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not unloaded')

        comp.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMETransientEmptyComponent.1.qml')) return reportError('Test component not loaded 4')
        if (componentCache.isTypeLoaded('VMETransientEmptyComponent.qml')) return reportError('Transient component not unloaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not unloaded 2')

        success = true
    }
}
