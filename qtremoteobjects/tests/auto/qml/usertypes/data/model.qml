import QtQuick 2.0
import QtRemoteObjects 5.12
import usertypes 1.0

TypeWithModelReplica {
    node: Node {
        registryUrl: "local:testModel"
    }
}
