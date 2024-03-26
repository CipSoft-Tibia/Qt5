// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformfontdatabase.h"
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qfontdatabase_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <qpa/qplatformscreen.h>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#include <QtCore/QMetaEnum>
#include <QtCore/qendian.h>

#include <algorithm>
#include <iterator>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpaFonts, "qt.qpa.fonts")

void qt_registerFont(const QString &familyname, const QString &stylename,
                     const QString &foundryname, int weight,
                     QFont::Style style, int stretch, bool antialiased,
                     bool scalable, int pixelSize, bool fixedPitch,
                     const QSupportedWritingSystems &writingSystems, void *hanlde);

void qt_registerFontFamily(const QString &familyName);
void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias);
bool qt_isFontFamilyPopulated(const QString &familyName);

/*!
    Registers a font with the given set of attributes describing the font's
    foundry, family name, style and stretch information, pixel size, and
    supported writing systems. Additional information about whether the font
    can be scaled and antialiased can also be provided.

    The foundry name and font family are described by \a foundryName and
    \a familyName. The font weight (light, normal, bold, etc.), style (normal,
    oblique, italic) and stretch information (condensed, expanded, unstretched,
    etc.) are specified by \a weight, \a style and \a stretch.

    Some fonts can be antialiased and scaled; \a scalable and \a antialiased
    can be set to true for fonts with these attributes. The intended pixel
    size of non-scalable fonts is specified by \a pixelSize; this value will be
    ignored for scalable fonts.

    The writing systems supported by the font are specified by the
    \a writingSystems argument.

    \sa registerFontFamily()
*/
void QPlatformFontDatabase::registerFont(const QString &familyname, const QString &stylename,
                                         const QString &foundryname, QFont::Weight weight,
                                         QFont::Style style, QFont::Stretch stretch, bool antialiased,
                                         bool scalable, int pixelSize, bool fixedPitch,
                                         const QSupportedWritingSystems &writingSystems, void *usrPtr)
{
    if (scalable)
        pixelSize = 0;

    qt_registerFont(familyname, stylename, foundryname, weight, style,
                    stretch, antialiased, scalable, pixelSize,
                    fixedPitch, writingSystems, usrPtr);
}

/*!
    Registers a font family with the font database. The font will be
    lazily populated by a callback to populateFamily() when the font
    database determines that the family needs population.

    \sa populateFamily(), registerFont()
*/
void QPlatformFontDatabase::registerFontFamily(const QString &familyName)
{
    qt_registerFontFamily(familyName);
}

class QWritingSystemsPrivate
{
public:
    QWritingSystemsPrivate()
        : ref(1)
        , list(QFontDatabase::WritingSystemsCount, false)
    {
    }

    QWritingSystemsPrivate(const QWritingSystemsPrivate *other)
        : ref(1)
        , list(other->list)
    {
    }

    QAtomicInt ref;
    QList<bool> list;
};

/*!
    Constructs a new object to handle supported writing systems.
*/
QSupportedWritingSystems::QSupportedWritingSystems()
{
    d = new QWritingSystemsPrivate;
}

/*!
    Constructs a copy of the \a other writing systems object.
*/
QSupportedWritingSystems::QSupportedWritingSystems(const QSupportedWritingSystems &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Constructs a copy of the \a other writing systems object.
*/
QSupportedWritingSystems &QSupportedWritingSystems::operator=(const QSupportedWritingSystems &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

bool operator==(const QSupportedWritingSystems &lhs, const QSupportedWritingSystems &rhs)
{
    return !(lhs != rhs);
}

bool operator!=(const QSupportedWritingSystems &lhs, const QSupportedWritingSystems &rhs)
{
    if (lhs.d == rhs.d)
        return false;

    Q_ASSERT(lhs.d->list.size() == rhs.d->list.size());
    Q_ASSERT(lhs.d->list.size() == QFontDatabase::WritingSystemsCount);
    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        if (lhs.d->list.at(i) != rhs.d->list.at(i))
            return true;
    }

    return false;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSupportedWritingSystems &sws)
{
    const QMetaObject *mo = &QFontDatabase::staticMetaObject;
    QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("WritingSystem"));

    QDebugStateSaver saver(debug);
    debug.nospace() << "QSupportedWritingSystems(";
    int i = sws.d->list.indexOf(true);
    while (i > 0) {
        debug << me.valueToKey(i);
        i = sws.d->list.indexOf(true, i + 1);
        if (i > 0)
            debug << ", ";
    }
    debug << ")";
    return debug;
}
#endif

/*!
    Destroys the supported writing systems object.
*/
QSupportedWritingSystems::~QSupportedWritingSystems()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \internal
*/
void QSupportedWritingSystems::detach()
{
    if (d->ref.loadRelaxed() != 1) {
        QWritingSystemsPrivate *newd = new QWritingSystemsPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Sets or clears support for the specified \a writingSystem based on the
    value given by \a support.
*/
void QSupportedWritingSystems::setSupported(QFontDatabase::WritingSystem writingSystem, bool support)
{
    detach();
    d->list[writingSystem] = support;
}

/*!
    Returns \c true if the writing system specified by \a writingSystem is
    supported; otherwise returns \c false.
*/
bool QSupportedWritingSystems::supported(QFontDatabase::WritingSystem writingSystem) const
{
    return d->list.at(writingSystem);
}

/*!
    \class QSupportedWritingSystems
    \brief The QSupportedWritingSystems class is used when registering fonts with the internal Qt
    fontdatabase.
    \ingroup painting
    \inmodule QtGui

    Its to provide an easy to use interface for indicating what writing systems a specific font
    supports.

*/

/*!
    \internal
 */
QPlatformFontDatabase::~QPlatformFontDatabase()
{
}

/*!
  This function is called once at startup by Qt's internal font database.
  Reimplement this function in a subclass for a convenient place to initialize
  the internal font database.

  You may lazily populate the database by calling registerFontFamily() instead
  of registerFont(), in which case you'll get a callback to populateFamily()
  when the required family needs population. You then call registerFont() to
  finish population of the family.

  The default implementation does nothing.
*/
void QPlatformFontDatabase::populateFontDatabase()
{
}

/*!
    This function is called whenever a lazily populated family, populated
    through registerFontFamily(), needs full population.

    You are expected to fully populate the family by calling registerFont()
    for each font that matches the family name.
*/
void QPlatformFontDatabase::populateFamily(const QString &familyName)
{
    Q_UNUSED(familyName);
}

/*!
    This function is called whenever the font database is invalidated.

    Reimplement this function to clear any internal data structures that
    will need to be rebuilt at the next call to populateFontDatabase().
*/
void QPlatformFontDatabase::invalidate()
{
}

/*!
    Returns a multi font engine in the specified \a script to encapsulate \a fontEngine with the
    option to fall back to the fonts given by \a fallbacks if \a fontEngine does not support
    a certain character.
*/
QFontEngineMulti *QPlatformFontDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
    return new QFontEngineMulti(fontEngine, script);
}

/*!
    Returns the font engine that can be used to render the font described by
    the font definition, \a fontDef, in the specified \a script.

    This function is called by QFontDatabase both for system fonts provided
    by the platform font database, as well as for application fonts added by
    the application developer.

    The handle is the QPlatformFontDatabase specific handle passed when
    registering the font family via QPlatformFontDatabase::registerFont.

    The function is called for both fonts added via a filename as well
    as fonts added from QByteArray data. Subclasses will need to handle
    both cases via its platform specific handle.
*/
QFontEngine *QPlatformFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    Q_UNUSED(fontDef);
    Q_UNUSED(handle);
    qWarning("This plugin does not support loading system fonts.");
    return nullptr;
}

/*!
    Returns the font engine that will be used to back a QRawFont,
    based on the given \fontData, \a pixelSize, and \a hintingPreference.

    This function is called by QRawFont, and does not play a part in
    the normal operations of QFontDatabase.
*/
QFontEngine *QPlatformFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize,
                                               QFont::HintingPreference hintingPreference)
{
    Q_UNUSED(fontData);
    Q_UNUSED(pixelSize);
    Q_UNUSED(hintingPreference);
    qWarning("This plugin does not support font engines created directly from font data");
    return nullptr;
}

/*!
    Adds an application font described by the font contained supplied \a fontData
    or using the font contained in the file referenced by \a fileName. Returns
    a list of family names, or an empty list if the font could not be added.

    If \a applicationFont is non-null, its \c properties list should be filled
    with information from the loaded fonts. This is exposed through FontLoader in
    Qt Quick where it is needed for disambiguating fonts in the same family. When
    the function exits, the \a applicationFont should contain an entry of properties
    per font in the file, or it should be empty if no font was loaded.

    \note The default implementation of this function does not add an application
    font. Subclasses should reimplement this function to perform the necessary
    loading and registration of fonts.
*/
QStringList QPlatformFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    Q_UNUSED(fontData);
    Q_UNUSED(fileName);
    Q_UNUSED(applicationFont);

    if (applicationFont != nullptr)
        applicationFont->properties.clear();

    qWarning("This plugin does not support application fonts");

    return QStringList();
}

/*!
    Releases the specified font \a handle.
*/
void QPlatformFontDatabase::releaseHandle(void *handle)
{
    QByteArray *fileDataPtr = static_cast<QByteArray *>(handle);
    delete fileDataPtr;
}

/*!
    Returns the directory containing the fonts used by the database.
*/
QString QPlatformFontDatabase::fontDir() const
{
    QString fontpath = QString::fromLocal8Bit(qgetenv("QT_QPA_FONTDIR"));
    if (fontpath.isEmpty())
        fontpath = QLibraryInfo::path(QLibraryInfo::LibrariesPath) + "/fonts"_L1;

    return fontpath;
}

/*!
    Returns true if the font family is private. For any given family name,
    the result is platform dependent.
*/
bool QPlatformFontDatabase::isPrivateFontFamily(const QString &family) const
{
    Q_UNUSED(family);
    return false;
}

/*!
    Returns the default system font.

    \sa QGuiApplication::font()
    \since 5.0
*/

QFont QPlatformFontDatabase::defaultFont() const
{
    return QFont("Helvetica"_L1);
}


QString qt_resolveFontFamilyAlias(const QString &alias);

/*!
    Resolve alias to actual font family names.

    \since 5.0
 */
QString QPlatformFontDatabase::resolveFontFamilyAlias(const QString &family) const
{
    return qt_resolveFontFamilyAlias(family);
}

/*!
    Return true if all fonts are considered scalable when using this font database.
    Defaults to false.

    \since 5.0
 */

bool QPlatformFontDatabase::fontsAlwaysScalable() const
{
    return false;
}

/*!
    Return list of standard font sizes when using this font database.

    \since 5.0
 */

 QList<int> QPlatformFontDatabase::standardSizes() const
{
    QList<int> ret;
    static const quint8 standard[] =
        { 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
    static const int num_standards = int(sizeof standard / sizeof *standard);
    ret.reserve(num_standards);
    std::copy(standard, standard + num_standards, std::back_inserter(ret));
    return ret;
}

// see the Unicode subset bitfields in the MSDN docs
static const quint8 requiredUnicodeBits[QFontDatabase::WritingSystemsCount][2] = {
    { 127, 127 }, // Any
    { 0, 127 },   // Latin
    { 7, 127 },   // Greek
    { 9, 127 },   // Cyrillic
    { 10, 127 },  // Armenian
    { 11, 127 },  // Hebrew
    { 13, 127 },  // Arabic
    { 71, 127 },  // Syriac
    { 72, 127 },  // Thaana
    { 15, 127 },  // Devanagari
    { 16, 127 },  // Bengali
    { 17, 127 },  // Gurmukhi
    { 18, 127 },  // Gujarati
    { 19, 127 },  // Oriya
    { 20, 127 },  // Tamil
    { 21, 127 },  // Telugu
    { 22, 127 },  // Kannada
    { 23, 127 },  // Malayalam
    { 73, 127 },  // Sinhala
    { 24, 127 },  // Thai
    { 25, 127 },  // Lao
    { 70, 127 },  // Tibetan
    { 74, 127 },  // Myanmar
    { 26, 127 },  // Georgian
    { 80, 127 },  // Khmer
    { 126, 127 }, // SimplifiedChinese
    { 126, 127 }, // TraditionalChinese
    { 126, 127 }, // Japanese
    { 56, 127 },  // Korean
    { 0, 127 },   // Vietnamese (same as latin1)
    { 126, 127 }, // Other
    { 78, 127 },  // Ogham
    { 79, 127 },  // Runic
    { 14, 127 },  // Nko
};

enum CsbBits {
    Latin1CsbBit = 0,
    CentralEuropeCsbBit = 1,
    TurkishCsbBit = 4,
    BalticCsbBit = 7,
    CyrillicCsbBit = 2,
    GreekCsbBit = 3,
    HebrewCsbBit = 5,
    ArabicCsbBit = 6,
    VietnameseCsbBit = 8,
    SimplifiedChineseCsbBit = 18,
    TraditionalChineseCsbBit = 20,
    ThaiCsbBit = 16,
    JapaneseCsbBit = 17,
    KoreanCsbBit = 19,
    KoreanJohabCsbBit = 21,
    SymbolCsbBit = 31
};

/*!
    Helper function that determines the writing system support based on the contents of the OS/2 table
    in the font.

    \since 6.0
*/
QSupportedWritingSystems QPlatformFontDatabase::writingSystemsFromOS2Table(const char *os2Table, size_t length)
{
    if (length >= 86)  {
        quint32 unicodeRange[4] = {
            qFromBigEndian<quint32>(os2Table + 42),
            qFromBigEndian<quint32>(os2Table + 46),
            qFromBigEndian<quint32>(os2Table + 50),
            qFromBigEndian<quint32>(os2Table + 54)
        };
        quint32 codePageRange[2] = {
            qFromBigEndian<quint32>(os2Table + 78),
            qFromBigEndian<quint32>(os2Table + 82)
        };

        return writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);
    }

    return QSupportedWritingSystems();
}

/*!
    Helper function that determines the writing systems support by a given
    \a unicodeRange and \a codePageRange.

    \since 5.1
*/
QSupportedWritingSystems QPlatformFontDatabase::writingSystemsFromTrueTypeBits(quint32 unicodeRange[4], quint32 codePageRange[2])
{
    QSupportedWritingSystems writingSystems;

    bool hasScript = false;
    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        int bit = requiredUnicodeBits[i][0];
        int index = bit/32;
        int flag = 1 << (bit&31);
        if (bit != 126 && (unicodeRange[index] & flag)) {
            bit = requiredUnicodeBits[i][1];
            index = bit/32;

            flag = 1 << (bit&31);
            if (bit == 127 || (unicodeRange[index] & flag)) {
                writingSystems.setSupported(QFontDatabase::WritingSystem(i));
                hasScript = true;
                // qDebug("font %s: index=%d, flag=%8x supports script %d", familyName.latin1(), index, flag, i);
            }
        }
    }
    if (codePageRange[0] & ((1 << Latin1CsbBit) | (1 << CentralEuropeCsbBit) | (1 << TurkishCsbBit) | (1 << BalticCsbBit))) {
        writingSystems.setSupported(QFontDatabase::Latin);
        hasScript = true;
        //qDebug("font %s supports Latin", familyName.latin1());
    }
    if (codePageRange[0] & (1 << CyrillicCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Cyrillic);
        hasScript = true;
        //qDebug("font %s supports Cyrillic", familyName.latin1());
    }
    if (codePageRange[0] & (1 << GreekCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Greek);
        hasScript = true;
        //qDebug("font %s supports Greek", familyName.latin1());
    }
    if (codePageRange[0] & (1 << HebrewCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Hebrew);
        hasScript = true;
        //qDebug("font %s supports Hebrew", familyName.latin1());
    }
    if (codePageRange[0] & (1 << ArabicCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Arabic);
        hasScript = true;
        //qDebug("font %s supports Arabic", familyName.latin1());
    }
    if (codePageRange[0] & (1 << ThaiCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Thai);
        hasScript = true;
        //qDebug("font %s supports Thai", familyName.latin1());
    }
    if (codePageRange[0] & (1 << VietnameseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Vietnamese);
        hasScript = true;
        //qDebug("font %s supports Vietnamese", familyName.latin1());
    }
    if (codePageRange[0] & (1 << SimplifiedChineseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::SimplifiedChinese);
        hasScript = true;
        //qDebug("font %s supports Simplified Chinese", familyName.latin1());
    }
    if (codePageRange[0] & (1 << TraditionalChineseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::TraditionalChinese);
        hasScript = true;
        //qDebug("font %s supports Traditional Chinese", familyName.latin1());
    }
    if (codePageRange[0] & (1 << JapaneseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Japanese);
        hasScript = true;
        //qDebug("font %s supports Japanese", familyName.latin1());
    }
    if (codePageRange[0] & ((1 << KoreanCsbBit) | (1 << KoreanJohabCsbBit))) {
        writingSystems.setSupported(QFontDatabase::Korean);
        hasScript = true;
        //qDebug("font %s supports Korean", familyName.latin1());
    }
    if (codePageRange[0] & (1U << SymbolCsbBit)) {
        writingSystems = QSupportedWritingSystems();
        hasScript = false;
    }

    if (!hasScript)
        writingSystems.setSupported(QFontDatabase::Symbol);

    return writingSystems;
}

/*!
    Helper function that register the \a alias for the \a familyName.

    \since 5.2
*/

void QPlatformFontDatabase::registerAliasToFontFamily(const QString &familyName, const QString &alias)
{
    qt_registerAliasToFontFamily(familyName, alias);
}

/*!
    Requests that the platform font database should be repopulated.

    This will result in invalidating the entire font database.

    The next time the font database is accessed it will be repopulated
    via a call to QPlatformFontDatabase::populate().

    Application fonts will not be removed, and will be automatically
    populated when the font database is repopulated.

    \since 6.4
*/
void QPlatformFontDatabase::repopulateFontDatabase()
{
    QFontDatabasePrivate::instance()->invalidate();
}

/*!
    Helper function that returns true if the font family has already been registered and populated.

    \since 5.14
*/
bool QPlatformFontDatabase::isFamilyPopulated(const QString &familyName)
{
    return qt_isFontFamilyPopulated(familyName);
}

/*!
    \class QPlatformFontDatabase
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \ingroup painting

    \brief The QPlatformFontDatabase class makes it possible to customize how fonts
    are discovered and how they are rendered

    QPlatformFontDatabase is the superclass which is intended to let platform implementations use
    native font handling.

    Qt has its internal font database which it uses to discover available fonts on the
    user's system. To be able to populate this database subclass this class, and
    reimplement populateFontDatabase().

    Use the function registerFont() to populate the internal font database.

    Sometimes a specified font does not have the required glyphs; in such a case, the
    fallbackForFamily() function is called automatically to find alternative font
    families that can supply alternatives to the missing glyphs.

    \sa QSupportedWritingSystems
*/
QT_END_NAMESPACE
