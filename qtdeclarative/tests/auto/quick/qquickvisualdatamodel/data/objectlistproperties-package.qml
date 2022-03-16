import QtQuick 2.0
import QtQml.Models 2.12
import tst_qquickvisualdatamodel 1.0

ListView {
    width: 100
    height: 100
    model: visualModel.parts.package

    DelegateModel {
        id: visualModel
        objectName: "visualModel"

        property list<DataObject> objects: [
            DataObject { name: "Item 1"; color: "red" },
            DataObject { name: "Item 2"; color: "green" },
            DataObject { name: "Item 3"; color: "blue"},
            DataObject { name: "Item 4"; color: "yellow" }
        ]

        groups: [
            DelegateModelGroup { id: visibleItems; objectName: "visibleItems"; name: "visible"; includeByDefault: true },
            DelegateModelGroup { id: selectedItems; objectName: "selectedItems"; name: "selected" }
        ]

        model: objects

        delegate: Package {
            id: delegate

            property variant test1: index
            property variant test2: model.index
            property variant test3: name
            property variant test4: model.name

            function setTest3(arg) { name = arg }
            function setTest4(arg) { model.name = arg }

            Item {
                objectName: "delegate"

                width: 100
                height: 2
                Package.name: "package"
            }
        }
    }
}

