/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpf2.h"

#include <math.h>
#include <private/qfontengine_p.h>
#include <QFile>
#include <qendian.h>

QT_BEGIN_NAMESPACE

int QPF::debugVerbosity = 0;

// ### copied from qfontdatabase.cpp

// see the Unicode subset bitfields in the MSDN docs
static int requiredUnicodeBits[QFontDatabase::WritingSystemsCount][2] = {
        // Any,
    { 127, 127 },
        // Latin,
    { 0, 127 },
        // Greek,
    { 7, 127 },
        // Cyrillic,
    { 9, 127 },
        // Armenian,
    { 10, 127 },
        // Hebrew,
    { 11, 127 },
        // Arabic,
    { 13, 127 },
        // Syriac,
    { 71, 127 },
    //Thaana,
    { 72, 127 },
    //Devanagari,
    { 15, 127 },
    //Bengali,
    { 16, 127 },
    //Gurmukhi,
    { 17, 127 },
    //Gujarati,
    { 18, 127 },
    //Oriya,
    { 19, 127 },
    //Tamil,
    { 20, 127 },
    //Telugu,
    { 21, 127 },
    //Kannada,
    { 22, 127 },
    //Malayalam,
    { 23, 127 },
    //Sinhala,
    { 73, 127 },
    //Thai,
    { 24, 127 },
    //Lao,
    { 25, 127 },
    //Tibetan,
    { 70, 127 },
    //Myanmar,
    { 74, 127 },
        // Georgian,
    { 26, 127 },
        // Khmer,
    { 80, 127 },
        // SimplifiedChinese,
    { 126, 127 },
        // TraditionalChinese,
    { 126, 127 },
        // Japanese,
    { 126, 127 },
        // Korean,
    { 56, 127 },
        // Vietnamese,
    { 0, 127 }, // same as latin1
        // Other,
    { 126, 127 }
};

#define SimplifiedChineseCsbBit 18
#define TraditionalChineseCsbBit 20
#define JapaneseCsbBit 17
#define KoreanCsbBit 21

static const QFontEngineQPF2::TagType tagTypes[QFontEngineQPF2::NumTags] = {
    QFontEngineQPF2::StringType, // FontName
    QFontEngineQPF2::StringType, // FileName
    QFontEngineQPF2::UInt32Type, // FileIndex
    QFontEngineQPF2::UInt32Type, // FontRevision
    QFontEngineQPF2::StringType, // FreeText
    QFontEngineQPF2::FixedType,  // Ascent
    QFontEngineQPF2::FixedType,  // Descent
    QFontEngineQPF2::FixedType,  // Leading
    QFontEngineQPF2::FixedType,  // XHeight
    QFontEngineQPF2::FixedType,  // AverageCharWidth
    QFontEngineQPF2::FixedType,  // MaxCharWidth
    QFontEngineQPF2::FixedType,  // LineThickness
    QFontEngineQPF2::FixedType,  // MinLeftBearing
    QFontEngineQPF2::FixedType,  // MinRightBearing
    QFontEngineQPF2::FixedType,  // UnderlinePosition
    QFontEngineQPF2::UInt8Type,  // GlyphFormat
    QFontEngineQPF2::UInt8Type,  // PixelSize
    QFontEngineQPF2::UInt8Type,  // Weight
    QFontEngineQPF2::UInt8Type,  // Style
    QFontEngineQPF2::StringType, // EndOfHeader
    QFontEngineQPF2::BitFieldType// WritingSystems
};

static QList<QFontDatabase::WritingSystem> determineWritingSystemsFromTrueTypeBits(quint32 unicodeRange[4], quint32 codePageRange[2])
{
    QList<QFontDatabase::WritingSystem> writingSystems;
    bool hasScript = false;

    int i;
    for(i = 0; i < QFontDatabase::WritingSystemsCount; i++) {
        int bit = requiredUnicodeBits[i][0];
        int index = bit/32;
        int flag =  1 << (bit&31);
        if (bit != 126 && unicodeRange[index] & flag) {
            bit = requiredUnicodeBits[i][1];
            index = bit/32;

            flag =  1 << (bit&31);
            if (bit == 127 || unicodeRange[index] & flag) {
                writingSystems.append(QFontDatabase::WritingSystem(i));
                hasScript = true;
                // qDebug("font %s: index=%d, flag=%8x supports script %d", familyName.latin1(), index, flag, i);
            }
        }
    }
    if(codePageRange[0] & (1 << SimplifiedChineseCsbBit)) {
        writingSystems.append(QFontDatabase::SimplifiedChinese);
        hasScript = true;
        //qDebug("font %s supports Simplified Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << TraditionalChineseCsbBit)) {
        writingSystems.append(QFontDatabase::TraditionalChinese);
        hasScript = true;
        //qDebug("font %s supports Traditional Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << JapaneseCsbBit)) {
        writingSystems.append(QFontDatabase::Japanese);
        hasScript = true;
        //qDebug("font %s supports Japanese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << KoreanCsbBit)) {
        writingSystems.append(QFontDatabase::Korean);
        hasScript = true;
        //qDebug("font %s supports Korean", familyName.latin1());
    }
    if (!hasScript)
        writingSystems.append(QFontDatabase::Symbol);

    return writingSystems;
}

static QByteArray getWritingSystems(QFontEngine *fontEngine)
{
    QByteArray os2Table = fontEngine->getSfntTable(MAKE_TAG('O', 'S', '/', '2'));
    if (os2Table.isEmpty())
        return QByteArray();

    const uchar *data = reinterpret_cast<const uchar *>(os2Table.constData());

    quint32 unicodeRange[4] = {
        qFromBigEndian<quint32>(data + 42),
        qFromBigEndian<quint32>(data + 46),
        qFromBigEndian<quint32>(data + 50),
        qFromBigEndian<quint32>(data + 54)
    };
    quint32 codePageRange[2] = { qFromBigEndian<quint32>(data + 78), qFromBigEndian<quint32>(data + 82) };
    QList<QFontDatabase::WritingSystem> systems = determineWritingSystemsFromTrueTypeBits(unicodeRange, codePageRange);

    QByteArray bitField((QFontDatabase::WritingSystemsCount + 7) / 8, 0);

    for (int i = 0; i < systems.count(); ++i) {
        int bitPos = systems.at(i);
        bitField[bitPos / 8] = bitField.at(bitPos / 8) | (1 << (bitPos % 8));
    }

    return bitField;
}

static QString stringify(const QByteArray &bits)
{
    QString result;
    for (int i = 0; i < bits.count(); ++i) {
        uchar currentByte = bits.at(i);
        for (int j = 0; j < 8; ++j) {
            if (currentByte & 1)
                result += '1';
            else
                result += '0';
            currentByte >>= 1;
        }
    }
    return result;
}

static void dumpWritingSystems(const QByteArray &bits)
{
    QStringList writingSystems;

    QString bitString = stringify(bits);
    for (int i = 0; i < qMin(int(QFontDatabase::WritingSystemsCount), bitString.length()); ++i) {
        if (bitString.at(i) == QLatin1Char('1'))
            writingSystems << QFontDatabase::writingSystemName(QFontDatabase::WritingSystem(i));
    }

    qDebug() << "Supported writing systems:" << writingSystems;
}

static const char *headerTagNames[QFontEngineQPF2::NumTags] = {
    "FontName",
    "FileName",
    "FileIndex",
    "FontRevision",
    "FreeText",
    "Ascent",
    "Descent",
    "Leading",
    "XHeight",
    "AverageCharWidth",
    "MaxCharWidth",
    "LineThickness",
    "MinLeftBearing",
    "MinRightBearing",
    "UnderlinePosition",
    "GlyphFormat",
    "PixelSize",
    "Weight",
    "Style",
    "EndOfHeader",
    "WritingSystems"
};

QString QPF::fileNameForFont(const QFont &f)
{
    QString fileName = f.family().toLower() + "_" + QString::number(f.pixelSize())
                       + "_" + QString::number(f.weight())
                       + (f.italic() ? "_italic" : "")
                       + ".qpf2";
    fileName.replace(QLatin1Char(' '), QLatin1Char('_'));
    return fileName;
}

QByteArray QPF::generate(const QFont &font, int options, const QList<CharacterRange> &ranges, QString *originalFontFile)
{
    QTextEngine engine("Test", font);
    engine.itemize();
    engine.shape(0);
    QFontEngine *fontEngine = engine.fontEngine(engine.layoutData->items[0]);
    if (fontEngine->type() == QFontEngine::Multi)
        fontEngine = static_cast<QFontEngineMulti *>(fontEngine)->engine(0);

    if (originalFontFile)
        *originalFontFile = QFile::decodeName(fontEngine->faceId().filename);

    return generate(fontEngine, options, ranges);
}

QByteArray QPF::generate(QFontEngine *fontEngine, int options, const QList<CharacterRange> &ranges)
{
    QPF font;

    font.options = options;
    font.addHeader(fontEngine);
    if (options & IncludeCMap)
        font.addCMap(fontEngine);
    font.addGlyphs(fontEngine, ranges);

    return font.qpf;
}

void QPF::addHeader(QFontEngine *fontEngine)
{
    QFontEngineQPF2::Header *header = reinterpret_cast<QFontEngineQPF2::Header *>(addBytes(sizeof(QFontEngineQPF2::Header)));

    header->magic[0] = 'Q';
    header->magic[1] = 'P';
    header->magic[2] = 'F';
    header->magic[3] = '2';
    if (options & RenderGlyphs)
        header->lock = 0xffffffff;
    else
        header->lock = 0;
    header->majorVersion = QFontEngineQPF2::CurrentMajorVersion;
    header->minorVersion = QFontEngineQPF2::CurrentMinorVersion;
    header->dataSize = 0;
    int oldSize = qpf.size();

    addTaggedString(QFontEngineQPF2::Tag_FontName, fontEngine->fontDef.family.toUtf8());

    QFontEngine::FaceId face = fontEngine->faceId();
    addTaggedString(QFontEngineQPF2::Tag_FileName, face.filename);
    addTaggedUInt32(QFontEngineQPF2::Tag_FileIndex, face.index);

    {
        const QByteArray head = fontEngine->getSfntTable(MAKE_TAG('h', 'e', 'a', 'd'));
        const quint32 revision = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(head.constData()) + 4);
        addTaggedUInt32(QFontEngineQPF2::Tag_FontRevision, revision);
    }

    addTaggedQFixed(QFontEngineQPF2::Tag_Ascent, fontEngine->ascent());
    addTaggedQFixed(QFontEngineQPF2::Tag_Descent, fontEngine->descent());
    addTaggedQFixed(QFontEngineQPF2::Tag_Leading, fontEngine->leading());
    addTaggedQFixed(QFontEngineQPF2::Tag_XHeight, fontEngine->xHeight());
    addTaggedQFixed(QFontEngineQPF2::Tag_AverageCharWidth, fontEngine->averageCharWidth());
    addTaggedQFixed(QFontEngineQPF2::Tag_MaxCharWidth, QFixed::fromReal(fontEngine->maxCharWidth()));
    addTaggedQFixed(QFontEngineQPF2::Tag_LineThickness, fontEngine->lineThickness());
    addTaggedQFixed(QFontEngineQPF2::Tag_MinLeftBearing, QFixed::fromReal(fontEngine->minLeftBearing()));
    addTaggedQFixed(QFontEngineQPF2::Tag_MinRightBearing, QFixed::fromReal(fontEngine->minRightBearing()));
    addTaggedQFixed(QFontEngineQPF2::Tag_UnderlinePosition, fontEngine->underlinePosition());
    addTaggedUInt8(QFontEngineQPF2::Tag_PixelSize, fontEngine->fontDef.pixelSize);
    addTaggedUInt8(QFontEngineQPF2::Tag_Weight, fontEngine->fontDef.weight);
    addTaggedUInt8(QFontEngineQPF2::Tag_Style, fontEngine->fontDef.style);

    QByteArray writingSystemBitField = getWritingSystems(fontEngine);
    if (!writingSystemBitField.isEmpty())
        addTaggedString(QFontEngineQPF2::Tag_WritingSystems, writingSystemBitField);

    addTaggedUInt8(QFontEngineQPF2::Tag_GlyphFormat, QFontEngineQPF2::AlphamapGlyphs);

    addTaggedString(QFontEngineQPF2::Tag_EndOfHeader, QByteArray());
    align4();
    header = reinterpret_cast<QFontEngineQPF2::Header *>(qpf.data());
    header->dataSize = qToBigEndian<quint16>(qpf.size() - oldSize);
}

static uchar *appendBytes(QByteArray &array, int size)
{
    int oldSize = array.size();
    array.resize(array.size() + size);
    return reinterpret_cast<uchar *>(array.data() + oldSize);
}

#define APPEND(type, value) \
    qToBigEndian<type>(value, appendBytes(cmap, sizeof(type)))

struct CMapSegment
{
    int start; // codepoints
    int end;
    int startGlyphIndex;
};

static QByteArray generateTrueTypeCMap(QFontEngine *fe)
{
    QByteArray cmap;
    const int glyphCount = fe->glyphCount();
    if (!glyphCount)
        return cmap;

    // cmap header
    APPEND(quint16, 0); // table version number
    APPEND(quint16, 1); // number of tables

    // encoding record
    APPEND(quint16, 3); // platform-id
    APPEND(quint16, 10); // encoding-id (ucs-4)
    const int cmapOffset = cmap.size() + sizeof(quint32);
    APPEND(quint32, cmapOffset); // offset to sub-table

    APPEND(quint16, 4); // subtable format
    const int cmapTableLengthOffset = cmap.size();
    APPEND(quint16, 0); // length in bytes, will fill in later
    APPEND(quint16, 0); // language field

    QList<CMapSegment> segments;
    CMapSegment currentSegment;
    currentSegment.start = 0xffff;
    currentSegment.end = 0;
    currentSegment.startGlyphIndex = 0;
    quint32 previousGlyphIndex = 0xfffffffe;
    bool inSegment = false;

    QGlyphLayoutArray<1> layout;
    for (uint uc = 0; uc < 0x10000; ++uc) {
        QChar ch(uc);
        int nglyphs = 1;

        bool validGlyph = fe->stringToCMap(&ch, 1, &layout, &nglyphs, /*flags*/ 0)
                          && nglyphs == 1 && layout.glyphs[0];

        // leaving a segment?
        if (inSegment && (!validGlyph || layout.glyphs[0] != previousGlyphIndex + 1)) {
            Q_ASSERT(currentSegment.start != 0xffff);
            // store the current segment
            currentSegment.end = uc - 1;
            segments.append(currentSegment);
            currentSegment.start = 0xffff;
            inSegment = false;
        }
        // entering a new segment?
        if (validGlyph && (!inSegment || layout.glyphs[0] != previousGlyphIndex + 1)) {
            currentSegment.start = uc;
            currentSegment.startGlyphIndex = layout.glyphs[0];
            inSegment = true;
        }

        if (validGlyph)
            previousGlyphIndex = layout.glyphs[0];
        else
            previousGlyphIndex = 0xfffffffe;
    }

    currentSegment.start = 0xffff;
    currentSegment.end = 0xffff;
    currentSegment.startGlyphIndex = 0;
    segments.append(currentSegment);

    if (QPF::debugVerbosity > 3)
        qDebug() << "segments:" << segments.count();

    Q_ASSERT(!inSegment);

    const quint16 entrySelector = int(log2(segments.count()));
    const quint16 searchRange = 2 * (1 << entrySelector);
    const quint16 rangeShift = segments.count() * 2 - searchRange;

    if (QPF::debugVerbosity > 3)
        qDebug() << "entrySelector" << entrySelector << "searchRange" << searchRange
                 << "rangeShift" << rangeShift;

    APPEND(quint16, segments.count() * 2); // segCountX2
    APPEND(quint16, searchRange);
    APPEND(quint16, entrySelector);
    APPEND(quint16, rangeShift);

    // end character codes
    for (int i = 0; i < segments.count(); ++i)
        APPEND(quint16, segments.at(i).end);

    APPEND(quint16, 0); // pad

    // start character codes
    for (int i = 0; i < segments.count(); ++i)
        APPEND(quint16, segments.at(i).start);

    // id deltas
    for (int i = 0; i < segments.count(); ++i)
        APPEND(quint16, segments.at(i).startGlyphIndex - segments.at(i).start);

    // id range offsets
    for (int i = 0; i < segments.count(); ++i)
        APPEND(quint16, 0);

    uchar *lengthPtr = reinterpret_cast<uchar *>(cmap.data()) + cmapTableLengthOffset;
    qToBigEndian<quint16>(cmap.size() - cmapOffset, lengthPtr);

    return cmap;
}

void QPF::addCMap(QFontEngine *fontEngine)
{
    QByteArray cmapTable = fontEngine->getSfntTable(MAKE_TAG('c', 'm', 'a', 'p'));
    if (cmapTable.isEmpty())
        cmapTable = generateTrueTypeCMap(fontEngine);
    addBlock(QFontEngineQPF2::CMapBlock, cmapTable);
}

void QPF::addGlyphs(QFontEngine *fe, const QList<CharacterRange> &ranges)
{
    const quint16 glyphCount = fe->glyphCount();

    QByteArray gmap;
    gmap.resize(glyphCount * sizeof(quint32));
    gmap.fill(char(0xff));
    //qDebug() << "glyphCount" << glyphCount;

    QByteArray glyphs;
    if (options & RenderGlyphs) {
        // this is only a rough estimation
        glyphs.reserve(glyphCount
                * (sizeof(QFontEngineQPF2::Glyph)
                    + qRound(fe->maxCharWidth() * (fe->ascent() + fe->descent()).toReal())));

        QGlyphLayoutArray<1> layout;

        for (CharacterRange range : ranges) {
            if (debugVerbosity > 2)
                qDebug() << "rendering range from" << range.start << "to" << range.end;
            for (uint uc = range.start; uc < range.end; ++uc) {
                QChar ch(uc);
                int nglyphs = 1;
                if (!fe->stringToCMap(&ch, 1, &layout, &nglyphs, /*flags*/ 0))
                    continue;

                if (nglyphs != 1)
                    continue;

                const quint32 glyphIndex = layout.glyphs[0];

                if (!glyphIndex)
                    continue;

                Q_ASSERT(glyphIndex < glyphCount);

                glyph_metrics_t metrics = fe->boundingBox(glyphIndex);
                const bool valid = metrics.width.value() != 0 && metrics.height.value() != 0;

                QImage img = valid
                    ? fe->alphaMapForGlyph(glyphIndex).convertToFormat(QImage::Format_Indexed8)
                    : QPixmap(0,0).toImage();

                const quint32 oldSize = glyphs.size();
                glyphs.resize(glyphs.size() + sizeof(QFontEngineQPF2::Glyph) + img.byteCount());
                uchar *data = reinterpret_cast<uchar *>(glyphs.data() + oldSize);

                uchar *gmapPtr = reinterpret_cast<uchar *>(gmap.data() + glyphIndex * sizeof(quint32));
                qToBigEndian(oldSize, gmapPtr);

                QFontEngineQPF2::Glyph *glyph = reinterpret_cast<QFontEngineQPF2::Glyph *>(data);
                glyph->width = img.width();
                glyph->height = img.height();
                glyph->bytesPerLine = img.bytesPerLine();
                glyph->x = qRound(metrics.x);
                glyph->y = qRound(metrics.y);
                glyph->advance = qRound(metrics.xoff);
                data += sizeof(QFontEngineQPF2::Glyph);

                if ((debugVerbosity && (uc >= 'A') && (uc <= 'z')) || debugVerbosity > 1) {
                    qDebug() << "adding glyph with index" << glyphIndex << " uc =" << char(uc) << ":\n"
                        << "    glyph->x =" << glyph->x << "rounded from" << metrics.x << "\n"
                        << "    glyph->y =" << glyph->y << "rounded from" << metrics.y << "\n"
                        << "    width =" << glyph->width << "height =" << glyph->height
                        << "    advance =" << glyph->advance << "rounded from" << metrics.xoff
                        ;
                }

                memcpy(data, img.bits(), img.byteCount());
            }
        }
    }

    addBlock(QFontEngineQPF2::GMapBlock, gmap);
    addBlock(QFontEngineQPF2::GlyphBlock, glyphs);
}

void QPF::addBlock(QFontEngineQPF2::BlockTag tag, const QByteArray &blockData)
{
    addUInt16(tag);
    addUInt16(0); // padding
    const int padSize = ((blockData.size() + 3) / 4) * 4 - blockData.size();
    addUInt32(blockData.size() + padSize);
    addByteArray(blockData);
    for (int i = 0; i < padSize; ++i)
        addUInt8(0);
}

#define ADD_TAGGED_DATA(tag, qtype, type, value) \
    addUInt16(tag); \
    addUInt16(sizeof(qtype)); \
    add##type(value)

void QPF::addTaggedString(QFontEngineQPF2::HeaderTag tag, const QByteArray &string)
{
    addUInt16(tag);
    addUInt16(string.length());
    addByteArray(string);
}

void QPF::addTaggedQFixed(QFontEngineQPF2::HeaderTag tag, QFixed value)
{
    ADD_TAGGED_DATA(tag, quint32, UInt32, value.value());
}

void QPF::addTaggedUInt8(QFontEngineQPF2::HeaderTag tag, quint8 value)
{
    ADD_TAGGED_DATA(tag, quint8, UInt8, value);
}

void QPF::addTaggedInt8(QFontEngineQPF2::HeaderTag tag, qint8 value)
{
    ADD_TAGGED_DATA(tag, qint8, Int8, value);
}

void QPF::addTaggedUInt16(QFontEngineQPF2::HeaderTag tag, quint16 value)
{
    ADD_TAGGED_DATA(tag, quint16, UInt16, value);
}

void QPF::addTaggedUInt32(QFontEngineQPF2::HeaderTag tag, quint32 value)
{
    ADD_TAGGED_DATA(tag, quint32, UInt32, value);
}

void QPF::dump(const QByteArray &qpf)
{
    QPF font;
    font.qpf = qpf;

    const uchar *data = reinterpret_cast<const uchar *>(qpf.constData());
    const uchar *endPtr = reinterpret_cast<const uchar *>(qpf.constData() + qpf.size());
    data = font.dumpHeader(data);

    const quint32 *gmap = 0;
    quint32 glyphCount = 0;

    while (data < endPtr) {
        const QFontEngineQPF2::Block *block = reinterpret_cast<const QFontEngineQPF2::Block *>(data);
        quint32 tag = qFromBigEndian(block->tag);
        quint32 blockSize = qFromBigEndian(block->dataSize);
        qDebug() << "Block: Tag =" << qFromBigEndian(block->tag) << "; Size =" << blockSize << "; Offset =" << hex << data - reinterpret_cast<const uchar *>(qpf.constData());
        data += sizeof(QFontEngineQPF2::Block);

        if (debugVerbosity) {
            if (tag == QFontEngineQPF2::GMapBlock) {
                gmap = reinterpret_cast<const quint32 *>(data);
                glyphCount = blockSize / 4;
                font.dumpGMapBlock(gmap, glyphCount);
            } else if (tag == QFontEngineQPF2::GlyphBlock
                       && gmap && debugVerbosity > 1) {
                font.dumpGlyphBlock(gmap, glyphCount, data, data + blockSize);
            }
        }

        data += blockSize;
    }
}

const uchar *QPF::dumpHeader(const uchar *data)
{
    const QFontEngineQPF2::Header *header = reinterpret_cast<const QFontEngineQPF2::Header *>(data);
    qDebug() << "Header:";
    qDebug() << "magic ="
             << header->magic[0]
             << header->magic[1]
             << header->magic[2]
             << header->magic[3];
    qDebug() << "lock =" << qFromBigEndian(header->lock);
    qDebug() << "majorVersion =" << header->majorVersion;
    qDebug() << "minorVersion =" << header->minorVersion;
    qDebug() << "dataSize =" << qFromBigEndian(header->dataSize);

    data += sizeof(QFontEngineQPF2::Header);

    const uchar *endPtr = data + qFromBigEndian(header->dataSize);

    while (data && data < endPtr) {
        data = dumpHeaderTag(data);
    }

    return endPtr;
}

const uchar *QPF::dumpHeaderTag(const uchar *data)
{
    const QFontEngineQPF2::Tag *tagPtr = reinterpret_cast<const QFontEngineQPF2::Tag *>(data);
    quint16 tag = qFromBigEndian(tagPtr->tag);
    quint16 size = qFromBigEndian(tagPtr->size);

    qDebug() << "Tag =" << tag << headerTagNames[tag];
    qDebug() << "Size =" << size;

    if (tag == QFontEngineQPF2::Tag_EndOfHeader)
        return 0;

    data += sizeof(QFontEngineQPF2::Tag);

    Q_ASSERT(tag < QFontEngineQPF2::NumTags);

    switch (tagTypes[tag]) {
        case QFontEngineQPF2::StringType:
            qDebug() << "Payload =" << QString::fromUtf8(QByteArray(reinterpret_cast<const char *>(data), size));
            break;
        case QFontEngineQPF2::FixedType:
            Q_ASSERT(size == sizeof(quint32));
            qDebug() << "Payload =" << QFixed::fromFixed(qFromBigEndian<quint32>(data)).toReal();
            break;
        case QFontEngineQPF2::UInt8Type:
            Q_ASSERT(size == sizeof(quint8));
            qDebug() << "Payload =" << *data;
            break;
        case QFontEngineQPF2::UInt32Type:
            Q_ASSERT(size == sizeof(quint32));
            qDebug() << "Payload =" << qFromBigEndian<quint32>(data);
            break;
        case QFontEngineQPF2::BitFieldType: {
            QByteArray bits(reinterpret_cast<const char *>(data), size);
            qDebug() << "Payload =" << stringify(bits);
            if (QPF::debugVerbosity > 2 && tag == QFontEngineQPF2::Tag_WritingSystems)
                dumpWritingSystems(bits);
            } break;
    }

    data += size;
    return data;
}

void QPF::dumpGMapBlock(const quint32 *gmap, int glyphCount)
{
    qDebug() << "glyphCount =" << glyphCount;
    int renderedGlyphs = 0;
    for (int i = 0; i < glyphCount; ++i) {
        if (gmap[i] != 0xffffffff) {
            const quint32 glyphPos = qFromBigEndian(gmap[i]);
            qDebug("gmap[%d] = 0x%x / %u", i, glyphPos, glyphPos);
            ++renderedGlyphs;
        }
    }
    qDebug() << "Glyphs rendered:" << renderedGlyphs << "; Glyphs missing from the font:" << glyphCount - renderedGlyphs;
}

void QPF::dumpGlyphBlock(const quint32 *gmap, int glyphCount, const uchar *data, const uchar *endPtr)
{
    // glyphPos -> glyphIndex
    QMap<quint32, quint32> reverseGlyphMap;
    for (int i = 0; i < glyphCount; ++i) {
        if (gmap[i] == 0xffffffff)
            continue;
        const quint32 glyphPos = qFromBigEndian(gmap[i]);
        reverseGlyphMap[glyphPos] = i;
    }

    const uchar *glyphBlockBegin = data;
    while (data < endPtr) {
        const QFontEngineQPF2::Glyph *g = reinterpret_cast<const QFontEngineQPF2::Glyph *>(data);

        const quint64 glyphOffset = data - glyphBlockBegin;
        const quint32 glyphIndex = reverseGlyphMap.value(glyphOffset, 0xffffffff);

        if (glyphIndex == 0xffffffff)
            qDebug() << "############: Glyph present in glyph block is not listed in glyph map!";
        qDebug("glyph at offset 0x%x glyphIndex = %u", quint32(glyphOffset), glyphIndex);
        qDebug() << "    width =" << g->width << "height =" << g->height << "x =" << g->x << "y =" << g->y;
        qDebug() << "    advance =" << g->advance << "bytesPerLine =" << g->bytesPerLine;

        data += sizeof(*g);
        if (glyphIndex == 0xffffffff || debugVerbosity > 4) {
            dumpGlyph(data, g);
        }

        data += g->height * g->bytesPerLine;
    }
}

void QPF::dumpGlyph(const uchar *data, const QFontEngineQPF2::Glyph *glyph)
{
    fprintf(stderr, "---- glyph data:\n");
    const char *alphas = " .o#";
    for (int y = 0; y < glyph->height; ++y) {
        for (int x = 0; x < glyph->width; ++x) {
            const uchar value = data[y * glyph->bytesPerLine + x];
            fprintf(stderr, "%c", alphas[value >> 6]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "----\n");
}

QT_END_NAMESPACE
