// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmloutput.h"

QT_BEGIN_NAMESPACE

XmlOutput::XmlOutput(QTextStream &file, ConverstionType type)
    : xmlFile(file), indent("\t"), currentLevel(0), currentState(Bare), format(NewLine),
      conversion(type)
{
    tagStack.clear();
}

XmlOutput::~XmlOutput()
{
    closeAll();
}

// Settings ------------------------------------------------------------------
void XmlOutput::setIndentString(const QString &indentString)
{
    indent = indentString;
}

QString XmlOutput::indentString()
{
    return indent;
}

void XmlOutput::setIndentLevel(int level)
{
    currentLevel = level;
}

int XmlOutput::indentLevel()
{
    return currentLevel;
}

void XmlOutput::setState(XMLState state)
{
    currentState = state;
}

void XmlOutput::setFormat(XMLFormat newFormat)
{
    format = newFormat;
}

XmlOutput::XMLState XmlOutput::state()
{
    return currentState;
}

void XmlOutput::updateIndent()
{
    currentIndent.clear();
    currentIndent.reserve(currentLevel);
    for (int i = 0; i < currentLevel; ++i)
        currentIndent.append(indent);
}

void XmlOutput::increaseIndent()
{
    ++currentLevel;
    updateIndent();
}

void XmlOutput::decreaseIndent()
{
    if (currentLevel)
        --currentLevel;
    updateIndent();
    if (!currentLevel)
        currentState = Bare;
}

QString XmlOutput::doConversion(const QString &text)
{
    if (!text.size())
        return QString();
    else if (conversion == NoConversion)
        return text;

    QString output;
    if (conversion == XMLConversion) {

        // this is a way to escape characters that shouldn't be converted
        for (int i=0; i<text.size(); ++i) {
            const QChar c = text.at(i);
            if (c == QLatin1Char('&')) {
                if ( (i + 7) < text.size() &&
                    text.at(i + 1) == QLatin1Char('#') &&
                    text.at(i + 2) == QLatin1Char('x') &&
                    text.at(i + 7) == QLatin1Char(';') ) {
                    output += text.at(i);
                } else {
                    output += QLatin1String("&amp;");
                }
            } else if (c == QLatin1Char('<')) {
                output += QLatin1String("&lt;");
            } else if (c == QLatin1Char('>')) {
                output += QLatin1String("&gt;");
            } else {
                if (c.unicode() < 0x20) {
                    output += QString("&#x%1;").arg(c.unicode(), 2, 16, QLatin1Char('0'));
                } else {
                    output += c;
                }
            }
        }
    } else {
        output = text;
    }

    if (conversion == XMLConversion) {
        output.replace('\"', QLatin1String("&quot;"));
        output.replace('\'', QLatin1String("&apos;"));
    } else if (conversion == EscapeConversion) {
        output.replace('\"', QLatin1String("\\\""));
        output.replace('\'', QLatin1String("\\\'"));
    }
    return output;
}

// Stream functions ----------------------------------------------------------
XmlOutput& XmlOutput::operator<<(const QString& o)
{
    return operator<<(data(o));
}

XmlOutput& XmlOutput::operator<<(const xml_output& o)
{
    switch(o.xo_type) {
    case tNothing:
        break;
    case tRaw:
        addRaw(o.xo_text);
        break;
    case tDeclaration:
        addDeclaration(o.xo_text, o.xo_value);
        break;
    case tTag:
        newTagOpen(o.xo_text);
        break;
    case tTagValue:
        addRaw(QString("\n%1<%2>").arg(currentIndent).arg(o.xo_text));
        addRaw(doConversion(o.xo_value));
        addRaw(QString("</%1>").arg(o.xo_text));
        break;
    case tValueTag:
        addRaw(doConversion(o.xo_text));
        setFormat(NoNewLine);
        closeTag();
        setFormat(NewLine);
        break;
    case tImport:
        addRaw(QString("\n%1<Import %2=\"%3\" />").arg(currentIndent).arg(o.xo_text).arg(o.xo_value));
        break;
    case tCloseTag:
        if (o.xo_value.size())
            closeAll();
        else if (o.xo_text.size())
            closeTo(o.xo_text);
        else
            closeTag();
        break;
    case tAttribute:
        addAttribute(o.xo_text, o.xo_value);
        break;
    case tAttributeTag:
        addAttributeTag(o.xo_text, o.xo_value);
        break;
    case tData:
        {
            // Special case to be able to close tag in normal
            // way ("</tag>", not "/>") without using addRaw()..
            if (!o.xo_text.size()) {
                closeOpen();
                break;
            }
            QString output = doConversion(o.xo_text);
            output.replace('\n', "\n" + currentIndent);
            addRaw(QString("\n%1%2").arg(currentIndent).arg(output));
        }
        break;
    case tComment:
        {
            QString output("<!--%1-->");
            addRaw(output.arg(o.xo_text));
        }
        break;
    case tCDATA:
        {
            QString output("<![CDATA[\n%1\n]]>");
            addRaw(output.arg(o.xo_text));
        }
        break;
    }
    return *this;
}


// Output functions ----------------------------------------------------------
void XmlOutput::newTag(const QString &tag)
{
    Q_ASSERT_X(tag.size(), "XmlOutput", "Cannot open an empty tag");
    newTagOpen(tag);
    closeOpen();
}

void XmlOutput::newTagOpen(const QString &tag)
{
    Q_ASSERT_X(tag.size(), "XmlOutput", "Cannot open an empty tag");
    closeOpen();

    if (format == NewLine)
        xmlFile << Qt::endl << currentIndent;
    xmlFile << '<' << doConversion(tag);
    currentState = Attribute;
    tagStack.append(tag);
    increaseIndent(); // ---> indent
}

void XmlOutput::closeOpen()
{
    switch(currentState) {
        case Bare:
        case Tag:
            return;
        case Attribute:
            break;
    }
    xmlFile << '>';
    currentState = Tag;
}

void XmlOutput::closeTag()
{
    switch(currentState) {
        case Bare:
            if (tagStack.size())
                //warn_msg(WarnLogic, "<Root>: Cannot close tag in Bare state, %d tags on stack", tagStack.count());
                qDebug("<Root>: Cannot close tag in Bare state, %d tags on stack", int(tagStack.size()));
            else
                //warn_msg(WarnLogic, "<Root>: Cannot close tag, no tags on stack");
                qDebug("<Root>: Cannot close tag, no tags on stack");
            return;
        case Tag:
            decreaseIndent(); // <--- Pre-decrease indent
            if (format == NewLine)
                xmlFile << Qt::endl << currentIndent;
            xmlFile << "</" << doConversion(tagStack.last()) << '>';
            tagStack.pop_back();
            break;
        case Attribute:
            xmlFile << " />";
            tagStack.pop_back();
            currentState = Tag;
            decreaseIndent(); // <--- Post-decrease indent
            break;
    }
}

void XmlOutput::closeTo(const QString &tag)
{
    bool cont = true;
    if (!tagStack.contains(tag) && !tag.isNull()) {
        //warn_msg(WarnLogic, "<%s>: Cannot close to tag <%s>, not on stack", tagStack.last().latin1(), tag.latin1());
        qDebug("<%s>: Cannot close to tag <%s>, not on stack", tagStack.last().toLatin1().constData(), tag.toLatin1().constData());
        return;
    }
    int left = tagStack.size();
    while (left-- && cont) {
        cont = tagStack.last().compare(tag) != 0;
        closeTag();
    }
}

void XmlOutput::closeAll()
{
    if (!tagStack.size())
        return;
    closeTo(QString());
}

void XmlOutput::addDeclaration(const QString &version, const QString &encoding)
{
    switch(currentState) {
        case Bare:
            break;
        case Tag:
        case Attribute:
            //warn_msg(WarnLogic, "<%s>: Cannot add declaration when not in bare state", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add declaration when not in bare state", tagStack.last().toLatin1().constData());
            return;
    }
    QString outData = QString("<?xml version=\"%1\" encoding=\"%2\"?>")
                              .arg(doConversion(version))
                              .arg(doConversion(encoding));
    addRaw(outData);
}

void XmlOutput::addRaw(const QString &rawText)
{
    closeOpen();
    xmlFile << rawText;
}

void XmlOutput::addAttribute(const QString &attribute, const QString &value)
{
     switch(currentState) {
        case Bare:
        case Tag:
            //warn_msg(WarnLogic, "<%s>: Cannot add attribute since tags not open", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add attribute (%s) since tag's not open",
                   (tagStack.size() ? tagStack.last().toLatin1().constData() : "Root"),
                   attribute.toLatin1().constData());
            return;
        case Attribute:
            break;
    }
    if (format == NewLine)
        xmlFile << Qt::endl;
    xmlFile << currentIndent << doConversion(attribute) << "=\"" << doConversion(value) << "\"";
}

void XmlOutput::addAttributeTag(const QString &attribute, const QString &value)
{
     switch(currentState) {
        case Bare:
        case Tag:
            //warn_msg(WarnLogic, "<%s>: Cannot add attribute since tags not open", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add attribute (%s) since tag's not open",
                   (tagStack.size() ? tagStack.last().toLatin1().constData() : "Root"),
                   attribute.toLatin1().constData());
            return;
        case Attribute:
            break;
    }
    xmlFile << " " << doConversion(attribute) << "=\"" << doConversion(value) << "\"";
}

QT_END_NAMESPACE
