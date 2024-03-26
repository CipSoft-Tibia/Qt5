import QtQuick 2.0

Rectangle {
    property string sectionProperty: "number"
    property int sectionPositioning: ViewSection.InlineLabels
    width: 240
    height: 320
    color: "#ffffff"
    resources: [
        Component {
            id: myDelegate
            Item {
                id: wrapper
                objectName: "wrapper"
                property string section: ListView.section
                property string nextSection: ListView.nextSection
                property string prevSection: ListView.previousSection
                height: 20;
                width: 240

                ListView.onRemove: sequentialAnimation.start()

                SequentialAnimation {
                    id: sequentialAnimation

                    PropertyAction { target: wrapper; property: "ListView.delayRemove"; value: true }
                    NumberAnimation { target: wrapper; property: "height"; to: 0; duration: 100; easing.type: Easing.InOutQuad }
                    PropertyAction { target: wrapper; property: "ListView.delayRemove"; value: false }
                }

                Rectangle {
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
            }
        }
    ]
    ListView {
        id: list
        objectName: "list"
        width: 240
        height: 320
        cacheBuffer: 60
        model: testModel
        delegate: myDelegate
        section.property: sectionProperty
        section.delegate: Rectangle {
            objectName: "sect_" + section
            color: "#99bb99"
            height: 20
            width: list.width
            Text { text: section + ",   " + parent.y + ",    " + parent.objectName }
        }
        section.labelPositioning: sectionPositioning
    }
}
