// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtTest
import QtLocation
import "utils.js" as Utils

TestCase {
    id: testCase

    name: "ImageModel"

    Plugin {
        id: testPlugin
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        parameters: [
            PluginParameter {
                name: "initializePlaceData"
                value: true
            }
        ]
    }

    ImageModel {
        id: testModel
    }

    Place {
        id: testPlace
        name: "Test Place"
    }

    Place {
        id: parkViewHotel
        placeId: "4dcc74ce-fdeb-443e-827c-367438017cf1"
        plugin: testPlugin
    }

    Place {
        id: seaViewHotel
        placeId: "8f72057a-54b2-4e95-a7bb-97b4d2b5721e"
        plugin: testPlugin
    }

    function test_setAndGet_data() {
        return [
            { tag: "place", property: "place", signal: "placeChanged", value: testPlace },
            { tag: "batchSize", property: "batchSize", signal: "batchSizeChanged", value: 10, reset: 1 },
        ];
    }

    function test_setAndGet(data) {
        Utils.testObjectProperties(testCase, testModel, data);
    }

    function test_consecutive_fetch_data() {
        return [
            { tag: "batchSize 1", batchSize: 1 },
            { tag: "batchSize 2", batchSize: 2 },
            { tag: "batchSize 5", batchSize: 5 },
            { tag: "batchSize 10", batchSize: 10 },
        ];
    }

    function test_consecutive_fetch(data) {
        var expectedImages = [
                    {
                        "url": "http://somewhere.com/image1.png",
                        "imageId": "0001",
                        "mimeType": "image/png"
                    },
                    {
                        "url": "http://somewhere.com/image2.png",
                        "imageId": "0002",
                        "mimeType": "image/png"
                    },
                    {
                        "url": "http://somewhere.com/image3.png",
                        "imageId": "0003",
                        "mimeType": "image/png"
                    },
                    {
                        "url": "",
                        "imageId": "",
                        "mimeType": ""
                    },
                    {
                        "url": "http://somewhere.com/image5.png",
                        "imageId": "0005",
                        "mimeType": "image/png"
                    }
                ]

        var model = createModel();
        Utils.testConsecutiveFetch(testCase, model, parkViewHotel, expectedImages, data);
        model.destroy();
    }

    function test_reset() {
        var model = createModel();
        Utils.testReset(testCase, model, parkViewHotel);
        model.destroy();
    }

    function test_fetch_data() {
        return [
                    {
                        tag: "fetch all images in a single batch",
                        model: createModel(),
                        batchSize: 10,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    },
                    {
                        tag: "fetch from a place with no images",
                        model: createModel(),
                        batchSize: 1,
                        place: seaViewHotel,
                        expectedTotalCount: 0,
                        expectedCount: 0
                    },
                    {
                        tag: "fetch with batch size one less than the total",
                        model: createModel(),
                        batchSize: 4,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 4
                    },
                    {
                        tag: "fetch with batch size equal to the total",
                        model: createModel(),
                        batchSize: 5,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    },
                    {
                        tag: "fetch with batch size larger than the total",
                        model: createModel(),
                        batchSize: 6,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    }
                ]
    }

    function test_fetch(data) {
        Utils.testFetch(testCase, data);
        data.model.destroy();
    }

    function createModel() {
        return Qt.createQmlObject('import QtLocation 5.3; ImageModel {}',
                                  testCase, "imageModel");
    }
}
