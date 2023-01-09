import QtQuick 2.0
import QtQml.Models 2.12

Rectangle {
    width: 240
    height: 320
    color: "#ffffff"
    resources: [
        Component {
            id: myDelegate
            Package {
                Item {
                    id: wrapper
                    objectName: "wrapper"
                    height: ListView.previousSection != ListView.section ? 40 : 20;
                    width: 240
                    Package.name: "package"
                    Rectangle {
                        y: wrapper.ListView.previousSection != wrapper.ListView.section ? 20 : 0
                        height: 20
                        width: parent.width
                        color: wrapper.ListView.isCurrentItem ? "lightsteelblue" : "white"
                        Text {
                            text: index
                        }
                        Text {
                            x: 30
                            id: textName
                            objectName: "textName"
                            text: name
                        }
                        Text {
                            x: 100
                            id: textNumber
                            objectName: "textNumber"
                            text: number
                        }
                        Text {
                            objectName: "nextSection"
                            x: 150
                            text: wrapper.ListView.nextSection
                        }
                        Text {
                            x: 200
                            text: wrapper.y
                        }
                    }
                    Rectangle {
                        color: "#99bb99"
                        height: wrapper.ListView.previousSection != wrapper.ListView.section ? 20 : 0
                        width: parent.width
                        visible: wrapper.ListView.previousSection != wrapper.ListView.section ? true : false
                        Text { text: wrapper.ListView.section }
                    }
                }
            }
        },
        DelegateModel {
            id: visualModel
            model: testModel
            delegate: myDelegate
        }

    ]
    ListView {
        id: list
        objectName: "list"
        width: 240
        height: 320
        model: visualModel.parts.package
        section.property: "number"
        cacheBuffer: 60
    }
}
