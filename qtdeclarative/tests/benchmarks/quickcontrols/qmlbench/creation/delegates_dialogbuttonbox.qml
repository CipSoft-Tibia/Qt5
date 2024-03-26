import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: DialogButtonBox {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }
}
