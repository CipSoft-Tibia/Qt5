import QtQuick 2.0

Item {
    property bool success: false
    required property QtObject componentCache

    function reportError(s) { console.warn(s) }

    Component.onCompleted: {
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyExtendEmptyComponent.2.qml')) return reportError('Test component not loaded')
        if (componentCache.isTypeLoaded('EmptyExtendEmptyComponent.qml')) return reportError('Extend component already loaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already loaded')

        var comp = Qt.createComponent('EmptyExtendEmptyComponent.qml')
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyExtendEmptyComponent.2.qml')) return reportError('Test component not loaded 2')
        if (!componentCache.isTypeLoaded('EmptyExtendEmptyComponent.qml')) return reportError('Extend component not loaded')
        if (!componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not loaded')

        var obj = comp.createObject()
        if (!obj) return reportError('Invalid object')
        if (obj.x == undefined) return reportError('Invalid object 2')

        comp.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyExtendEmptyComponent.2.qml')) return reportError('Test component not loaded 3')
        if (!componentCache.isTypeLoaded('EmptyExtendEmptyComponent.qml')) return reportError('Extend component already unloaded')
        if (!componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component already unloaded')
        if (!obj) return reportError('Invalid object 3')
        if (obj.x == undefined) return reportError('Invalid object 4')

        obj.destroy()
        componentCache.trim()
        if (!componentCache.isTypeLoaded('testEmptyExtendEmptyComponent.2.qml')) return reportError('Test component not loaded 4')
        if (componentCache.isTypeLoaded('EmptyExtendEmptyComponent.qml')) return reportError('Extend component not unloaded')
        if (componentCache.isTypeLoaded('EmptyComponent.qml')) return reportError('Empty component not unloaded')

        success = true
    }
}
