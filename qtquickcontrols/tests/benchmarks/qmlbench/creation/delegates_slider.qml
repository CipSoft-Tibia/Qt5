import QtQuick 2.0
import QmlBench 1.0
import QtQuick.Controls 1.4

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: Slider {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        value: index / root.staticCount
    }
}
