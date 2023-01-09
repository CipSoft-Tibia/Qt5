import QtQuick 2.0

Item {
    property bool success: false
    required property QtObject componentCache

    function reportError(s) { console.warn(s) }

    Component.onCompleted: {
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyPropertyEmptyComponent.2.qml')) return reportError('Test component not loaded')
        if (componentCache.isTypeLoaded('EmptyPropertyEmptyComponent.qml')) return reportError('Property component already loaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already loaded')

        var comp = Qt.createComponent('EmptyPropertyEmptyComponent.qml')
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyPropertyEmptyComponent.2.qml')) return reportError('Test component not loaded 2')
        if (!componentCache.isTypeLoaded('EmptyPropertyEmptyComponent.qml')) return reportError('Property component not loaded')
        if (!componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not loaded')

        var obj = comp.createObject()
        if (!obj) return reportError('Invalid object')
        if (obj.x == undefined) return reportError('Invalid object 2')
        if (obj.p == undefined) return reportError('Invalid object 3')
        if (obj.p.x == undefined) return reportError('Invalid object 4')

        comp.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyPropertyEmptyComponent.2.qml')) return reportError('Test component not loaded 3')
        if (!componentCache.isTypeLoaded('EmptyPropertyEmptyComponent.qml')) return reportError('Property component already unloaded')
        if (!componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already unloaded')
        if (!obj) return reportError('Invalid object 5')
        if (obj.x == undefined) return reportError('Invalid object 6')
        if (obj.p == undefined) return reportError('Invalid object 7')
        if (obj.p.x == undefined) return reportError('Invalid object 8')

        obj.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyPropertyEmptyComponent.2.qml')) return reportError('Test component not loaded 4')
        if (componentCache.isTypeLoaded('EmptyPropertyEmptyComponent.qml')) return reportError('Property component not unloaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not unloaded')

        success = true
    }
}
