import QtQml
import "." as MyStuff

MyStuff.Simple {
    property bool something: contains(Qt.point(12, 34))
    property int other: Qt.AlignBottom
}
