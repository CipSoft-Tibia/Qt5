import QtQuick 2.0

QtObject {
    property bool xmlTest: false
    property bool dataOK: false
    property int status: 0

    function checkText(text, whitespacetext)
    {
        if (text == null)
            return;

        if (text.nodeName != "#text")
            return;

        if (text.nodeValue != "Hello world!")
            return;

        if (text.nodeType != 3)
            return;

        if (text.parentNode.nodeName != "item")
            return;

        if (text.childNodes.length != 0)
            return;

        if (text.firstChild != null)
            return;

        if (text.lastChild != null)
            return;

        if (text.previousSibling != null)
            return;

        if (text.nextSibling != null)
            return;

        if (text.attributes != null)
            return;

        if (text.wholeText != "Hello world!")
            return;

        if (text.data != "Hello world!")
            return;

        if (text.length != 12)
            return;

        if (text.isElementContentWhitespace != false)
            return;

        if (whitespacetext.nodeName != "#text")
            return;

        if (whitespacetext.nodeValue != "   ")
            return;

        if (whitespacetext.nodeType != 3)
            return;

        if (whitespacetext.parentNode.nodeName != "item")
            return;

        if (whitespacetext.childNodes.length != 0)
            return;

        if (whitespacetext.firstChild != null)
            return;

        if (whitespacetext.lastChild != null)
            return;

        if (whitespacetext.previousSibling != null)
            return;

        if (whitespacetext.nextSibling != null)
            return;

        if (whitespacetext.attributes != null)
            return;

        if (whitespacetext.wholeText != "   ")
            return;

        if (whitespacetext.data != "   ")
            return;

        if (whitespacetext.length != 3)
            return;

        if (whitespacetext.isElementContentWhitespace != true)
            return;

        xmlTest = true;
    }

    function checkXML(document)
    {
        checkText(document.documentElement.childNodes[0].childNodes[0],
                  document.documentElement.childNodes[1].childNodes[0]);

    }

    Component.onCompleted: {
        var x = new XMLHttpRequest;

        x.open("GET", "text.xml");

        // Test to the end
        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE) {
                dataOK = true;
                status = x.status;

                if (x.responseXML != null)
                    checkXML(x.responseXML);
            }
        }

        x.send()
    }
}




