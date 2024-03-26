import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: ScrollBar {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        height: 100
        size: index / root.staticCount
        pressed: index % 2
        active: true
    }
}
