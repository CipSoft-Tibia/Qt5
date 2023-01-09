import QtQuick 2.0

Item {
    property bool success: false
    required property QtObject componentCache

    function reportError(s) { console.warn(s) }

    Component.onCompleted: {
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMEComponent.2.qml')) return reportError('Test component not loaded')
        if (componentCache.isTypeLoaded('VMEComponent.qml')) return reportError('VME component already loaded')

        var comp = Qt.createComponent('VMEComponent.qml')
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMEComponent.2.qml')) return reportError('Test component not loaded 2')
        if (!componentCache.isTypeLoaded('VMEComponent.qml')) return reportError('VME component not loaded')

        var obj = comp.createObject()
        if (!obj) return reportError('Invalid object')
        if (obj.foo != 'bar') return reportError('Invalid object 2')

        comp.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMEComponent.2.qml')) return reportError('Test component not loaded 3')
        if (!componentCache.isTypeLoaded('VMEComponent.qml')) return reportError('VME component already unloaded')
        if (!obj) return reportError('Invalid object 3')
        if (obj.foo != 'bar') return reportError('Invalid object 4')

        obj.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testVMEComponent.2.qml')) return reportError('Test component not loaded 4')
        if (componentCache.isTypeLoaded('VMEComponent.qml')) return reportError('VME component not unloaded')

        success = true
    }
}
