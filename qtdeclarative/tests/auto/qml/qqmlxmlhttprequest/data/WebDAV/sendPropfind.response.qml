// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    property string url
    property bool xmlTest: false
    property bool typeTest: false

    function checkXML(document)
    {
        if (document.xmlVersion != "1.0")
            return;

        if (document.xmlEncoding != "utf-8")
            return;

        if (document.documentElement == null)
            return;

        var multistatus = document.documentElement;
        if (multistatus.nodeName != "multistatus")
            return;

        if (multistatus.namespaceUri != "DAV:")
            return;

        var multistatusChildTags = [ "response", "responsedescription" ];
        for (var node = 0; node < multistatus.childNodes.length; ++node) {
            if (multistatus.childNodes[node].nodeName != multistatusChildTags[node])
                return;
        }

        var response = multistatus.childNodes[0];
        var responseChildTags =  [ "href", "propstat", "propstat" ];
        for (var node = 0; node < response.childNodes.length; ++node) {
            var nodeName = response.childNodes[node].nodeName;
            if (nodeName != responseChildTags[node])
                return;

            var nodeValue = response.childNodes[node].childNodes[0].nodeValue;
            if ((nodeName == "href") && (nodeValue != "http://www.example.com/file"))
                return;
        }

        if (multistatus.childNodes[1].childNodes[0].nodeValue != "There has been an access violation error.")
            return;

        var propstat1 = response.childNodes[1];
        var propstat1ChildTags = ["prop", "status"];
        for (var node = 0; node < propstat1.childNodes.length; ++node) {
            var nodeName = propstat1.childNodes[node].nodeName;
            if (nodeName != propstat1ChildTags[node])
                return;

            var nodeValue = propstat1.childNodes[node].childNodes[0].nodeValue;
            if ((nodeName == "status") && (nodeValue != "HTTP/1.1 200 OK"))
                return;
        }

        var prop1 = propstat1.childNodes[0];
        var prop1ChildTags = [ "bigbox", "author" ];
        for (var node = 0; node < prop1.childNodes.length; ++node) {
            var nodeName = prop1.childNodes[node].nodeName;
            if (nodeName != prop1ChildTags[node])
                return;

            if (nodeName == "bigbox") {
                if (prop1.childNodes[node].childNodes.length != 1)
                    return;

                var boxType = prop1.childNodes[node].childNodes[0];
                if (boxType.nodeName != "BoxType")
                    return;
                if (boxType.childNodes[0].nodeValue != "Box type A")
                    return;
            }
        }

        var propstat2 = response.childNodes[2];
        var propstat2ChildTags = ["prop", "status", "responsedescription" ];
        for (var node = 0; node < propstat2.childNodes.length; ++node) {
            var nodeName = propstat2.childNodes[node].nodeName;
            if (nodeName != propstat2ChildTags[node])
                return;

            var nodeValue = propstat2.childNodes[node].childNodes[0].nodeValue;
            if ((nodeName == "status") && (nodeValue != "HTTP/1.1 403 Forbidden"))
                return;
            if ((nodeName == "responsedescription") && (nodeValue != "The user does not have access to the DingALing property."))
                return;
        }

        var prop2 = propstat2.childNodes[0];
        var prop2ChildTags = [ "DingALing", "Random" ];
        for (var node = 0; node < prop2.childNodes.length; ++node) {
            var nodeName = prop2.childNodes[node].nodeName;
            if (nodeName != prop2ChildTags[node])
                return;
        }

        xmlTest = true;
    }

    Component.onCompleted: {

        var request = new XMLHttpRequest();
        request.open("PROPFIND", url);
        request.responseType = "document";

        request.onreadystatechange = function() {
            if (request.readyState == XMLHttpRequest.DONE) {
                checkXML(request.response);
                typeTest = (request.responseType == "document");
            }
        }

        var requestBody = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
                          "<D:propfind xmlns:D=\"DAV:\">\n" +
                              "<D:prop xmlns:R=\"http://www.foo.bar/boxschema/\">\n" +
                                  "<R:bigbox/>\n" +
                                  "<R:author/>\n" +
                                  "<R:DingALing/>\n" +
                                  "<R:Random/>\n" +
                              "</D:prop>\n" +
                          "</D:propfind>\n"
        request.send(requestBody);
    }
}

