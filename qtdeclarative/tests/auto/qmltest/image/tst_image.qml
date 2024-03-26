// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import QtTest 1.1

Item {
    id: top

    property string srcImage: "logo.png"
    property bool canconnect
    property bool checkfinished: false

    Component.onCompleted: {
        var check = new XMLHttpRequest;
        check.open("GET", "http://127.0.0.1:14445/logo.png");
        check.onreadystatechange = function() {
            if (check.readyState == XMLHttpRequest.DONE) {
                if (check.status == 404) {
                    top.canconnect = false;
                } else {
                    top.canconnect = true;
                }
                top.checkfinished = true;
            }
        }
        check.send();
    }

    Image {
        id: noSource
        source: ""
    }

    Image {
        id: clearSource
        source: srcImage
    }

    Image {
        id: resized
        source: srcImage
        width: 300
        height: 300
    }

    Image {
        id: smooth
        source: srcImage
        smooth: true
        width: 300
        height: 300
    }

    Image {
        id: tileModes1
        source: srcImage
        width: 100
        height: 300
        fillMode: Image.Tile
    }

    Image {
        id: tileModes2
        source: srcImage
        width: 300
        height: 150
        fillMode: Image.TileVertically
    }
    Image {
        id: tileModes3
        source: srcImage
        width: 300
        height: 150
        fillMode: Image.TileHorizontally
    }

    TestCase {
        name: "Image"

        function test_noSource() {
            compare(noSource.source, "")
            compare(noSource.width, 0)
            compare(noSource.height, 0)
            compare(noSource.fillMode, Image.Stretch)
        }

        function test_imageSource_data() {
            return [
                {
                    tag: "local",
                    source: "logo.png",
                    remote: false,
                    error: ""
                },
                {
                    tag: "local not found",
                    source: "no-such-file.png",
                    remote: false,
                    error: "SUBinline:1:21: QML Image: Cannot open: SUBno-such-file.png"
                },
                {
                    tag: "remote",
                    source: "http://127.0.0.1:14445/logo.png",
                    remote: true,
                    error: ""
                }
            ]
        }

        function test_imageSource(row) {
            var expectError = (row.error.length != 0)
            if (expectError) {
                var parentUrl = Qt.resolvedUrl(".")
                ignoreWarning(row.error.replace(/SUB/g, parentUrl))
            }

            var img = Qt.createQmlObject('import QtQuick 2.0; Image { source: "'+row.source+'" }', top)

            if (row.remote) {
                skip("Remote solution not yet complete")
                tryCompare(img, "status", Image.Loading)
                tryCompare(top, "checkfinished", true, 10000)
                if (top.canconnect == false)
                    skip("Cannot access remote")
            }

            if (!expectError) {
                tryCompare(img, "status", Image.Ready, 10000)
                compare(img.width, 59)
                compare(img.height, 71)
                compare(img.fillMode, Image.Stretch)
            } else {
                tryCompare(img, "status", Image.Error)
            }

            img.destroy()
        }

        function test_clearSource() {
            compare(clearSource.source, srcImage)
            compare(clearSource.width, 59)
            compare(clearSource.height, 71)

            srcImage = ""
            compare(clearSource.source, "")
            compare(clearSource.width, 0)
            compare(clearSource.height, 0)
        }

        function test_resized() {
            compare(resized.width, 300)
            compare(resized.height, 300)
            compare(resized.fillMode, Image.Stretch)
        }

        function test_smooth() {
            compare(smooth.smooth, true)
            compare(smooth.width, 300)
            compare(smooth.height, 300)
            compare(smooth.fillMode, Image.Stretch)
        }

        function test_tileModes() {
            compare(tileModes1.width, 100)
            compare(tileModes1.height, 300)
            compare(tileModes1.fillMode, Image.Tile)

            compare(tileModes2.width, 300)
            compare(tileModes2.height, 150)
            compare(tileModes2.fillMode, Image.TileVertically)

            compare(tileModes3.width, 300)
            compare(tileModes3.height, 150)
            compare(tileModes3.fillMode, Image.TileHorizontally)
        }
    }
}
