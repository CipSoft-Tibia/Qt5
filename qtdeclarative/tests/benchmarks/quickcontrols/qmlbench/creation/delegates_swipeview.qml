import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: SwipeView {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        width: 100
        height: 100
        Item {
            focus: SwipeView.isCurrentItem
        }
        Item {
            focus: SwipeView.isCurrentItem
        }
        Item {
            focus: SwipeView.isCurrentItem
        }
    }
}
