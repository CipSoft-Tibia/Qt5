// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolor.h"
#include "qcolor_p.h"
#include "qdrawhelper_p.h"
#include "qfloat16.h"
#include "qnamespace.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qdebug.h"
#include "private/qtools_p.h"

#include <algorithm>
#include <optional>

#include <stdio.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

// QColor fits into QVariant's internal storage on 64bit systems.
// It could also fit on 32bit systems, but we cannot make it happen in Qt6, due to BC.
#if QT_VERSION >= QT_VERSION_CHECK(7,0,0) || QT_POINTER_SIZE > 4
static_assert(sizeof(QColor) <= QVariant::Private::MaxInternalSize);
#endif

/*!
    \internal
    If s[0..n] is a valid hex number, returns its integer value,
    otherwise returns -1.
 */
static inline int hex2int(const char *s, int n)
{
    if (n < 0)
        return -1;
    int result = 0;
    for (; n > 0; --n) {
        result = result * 16;
        const int h = QtMiscUtils::fromHex(*s++);
        if (h < 0)
            return -1;
        result += h;
    }
    return result;
}

static std::optional<QRgba64> get_hex_rgb(const char *name, size_t len)
{
    if (name[0] != '#')
        return std::nullopt;
    name++;
    --len;
    int a, r, g, b;
    a = 65535;
    if (len == 12) {
        r = hex2int(name + 0, 4);
        g = hex2int(name + 4, 4);
        b = hex2int(name + 8, 4);
    } else if (len == 9) {
        r = hex2int(name + 0, 3);
        g = hex2int(name + 3, 3);
        b = hex2int(name + 6, 3);
        if (r == -1 || g == -1 || b == -1)
            return std::nullopt;
        r = (r << 4) | (r >> 8);
        g = (g << 4) | (g >> 8);
        b = (b << 4) | (b >> 8);
    } else if (len == 8) {
        a = hex2int(name + 0, 2) * 0x101;
        r = hex2int(name + 2, 2) * 0x101;
        g = hex2int(name + 4, 2) * 0x101;
        b = hex2int(name + 6, 2) * 0x101;
    } else if (len == 6) {
        r = hex2int(name + 0, 2) * 0x101;
        g = hex2int(name + 2, 2) * 0x101;
        b = hex2int(name + 4, 2) * 0x101;
    } else if (len == 3) {
        r = hex2int(name + 0, 1) * 0x1111;
        g = hex2int(name + 1, 1) * 0x1111;
        b = hex2int(name + 2, 1) * 0x1111;
    } else {
        r = g = b = -1;
    }
    if (uint(r) > 65535 || uint(g) > 65535 || uint(b) > 65535 || uint(a) > 65535)
        return std::nullopt;
    return qRgba64(r, g ,b, a);
}

std::optional<QRgb> qt_get_hex_rgb(const char *name)
{
    if (std::optional<QRgba64> rgba64 = get_hex_rgb(name, qstrlen(name)))
        return rgba64->toArgb32();
    return std::nullopt;
}

static std::optional<QRgba64> get_hex_rgb(const QChar *str, size_t len)
{
    if (len > 13)
        return std::nullopt;
    char tmp[16];
    for (size_t i = 0; i < len; ++i)
        tmp[i] = str[i].toLatin1();
    tmp[len] = 0;
    return get_hex_rgb(tmp, len);
}

static std::optional<QRgba64> get_hex_rgb(QAnyStringView name)
{
    return name.visit([] (auto name) {
        return get_hex_rgb(name.data(), name.size());
    });
}

#ifndef QT_NO_COLORNAMES

/*
  CSS color names = SVG 1.0 color names + transparent (rgba(0,0,0,0))
*/

#ifdef rgb
#  undef rgb
#endif
#define rgb(r,g,b) (0xff000000 | (r << 16) |  (g << 8) | b)

// keep this is in sync with QColorConstants
static constexpr struct RGBData {
    const char name[21];
    uint  value;
} rgbTbl[] = {
    { "aliceblue", rgb(240, 248, 255) },
    { "antiquewhite", rgb(250, 235, 215) },
    { "aqua", rgb( 0, 255, 255) },
    { "aquamarine", rgb(127, 255, 212) },
    { "azure", rgb(240, 255, 255) },
    { "beige", rgb(245, 245, 220) },
    { "bisque", rgb(255, 228, 196) },
    { "black", rgb( 0, 0, 0) },
    { "blanchedalmond", rgb(255, 235, 205) },
    { "blue", rgb( 0, 0, 255) },
    { "blueviolet", rgb(138, 43, 226) },
    { "brown", rgb(165, 42, 42) },
    { "burlywood", rgb(222, 184, 135) },
    { "cadetblue", rgb( 95, 158, 160) },
    { "chartreuse", rgb(127, 255, 0) },
    { "chocolate", rgb(210, 105, 30) },
    { "coral", rgb(255, 127, 80) },
    { "cornflowerblue", rgb(100, 149, 237) },
    { "cornsilk", rgb(255, 248, 220) },
    { "crimson", rgb(220, 20, 60) },
    { "cyan", rgb( 0, 255, 255) },
    { "darkblue", rgb( 0, 0, 139) },
    { "darkcyan", rgb( 0, 139, 139) },
    { "darkgoldenrod", rgb(184, 134, 11) },
    { "darkgray", rgb(169, 169, 169) },
    { "darkgreen", rgb( 0, 100, 0) },
    { "darkgrey", rgb(169, 169, 169) },
    { "darkkhaki", rgb(189, 183, 107) },
    { "darkmagenta", rgb(139, 0, 139) },
    { "darkolivegreen", rgb( 85, 107, 47) },
    { "darkorange", rgb(255, 140, 0) },
    { "darkorchid", rgb(153, 50, 204) },
    { "darkred", rgb(139, 0, 0) },
    { "darksalmon", rgb(233, 150, 122) },
    { "darkseagreen", rgb(143, 188, 143) },
    { "darkslateblue", rgb( 72, 61, 139) },
    { "darkslategray", rgb( 47, 79, 79) },
    { "darkslategrey", rgb( 47, 79, 79) },
    { "darkturquoise", rgb( 0, 206, 209) },
    { "darkviolet", rgb(148, 0, 211) },
    { "deeppink", rgb(255, 20, 147) },
    { "deepskyblue", rgb( 0, 191, 255) },
    { "dimgray", rgb(105, 105, 105) },
    { "dimgrey", rgb(105, 105, 105) },
    { "dodgerblue", rgb( 30, 144, 255) },
    { "firebrick", rgb(178, 34, 34) },
    { "floralwhite", rgb(255, 250, 240) },
    { "forestgreen", rgb( 34, 139, 34) },
    { "fuchsia", rgb(255, 0, 255) },
    { "gainsboro", rgb(220, 220, 220) },
    { "ghostwhite", rgb(248, 248, 255) },
    { "gold", rgb(255, 215, 0) },
    { "goldenrod", rgb(218, 165, 32) },
    { "gray", rgb(128, 128, 128) },
    { "green", rgb( 0, 128, 0) },
    { "greenyellow", rgb(173, 255, 47) },
    { "grey", rgb(128, 128, 128) },
    { "honeydew", rgb(240, 255, 240) },
    { "hotpink", rgb(255, 105, 180) },
    { "indianred", rgb(205, 92, 92) },
    { "indigo", rgb( 75, 0, 130) },
    { "ivory", rgb(255, 255, 240) },
    { "khaki", rgb(240, 230, 140) },
    { "lavender", rgb(230, 230, 250) },
    { "lavenderblush", rgb(255, 240, 245) },
    { "lawngreen", rgb(124, 252, 0) },
    { "lemonchiffon", rgb(255, 250, 205) },
    { "lightblue", rgb(173, 216, 230) },
    { "lightcoral", rgb(240, 128, 128) },
    { "lightcyan", rgb(224, 255, 255) },
    { "lightgoldenrodyellow", rgb(250, 250, 210) },
    { "lightgray", rgb(211, 211, 211) },
    { "lightgreen", rgb(144, 238, 144) },
    { "lightgrey", rgb(211, 211, 211) },
    { "lightpink", rgb(255, 182, 193) },
    { "lightsalmon", rgb(255, 160, 122) },
    { "lightseagreen", rgb( 32, 178, 170) },
    { "lightskyblue", rgb(135, 206, 250) },
    { "lightslategray", rgb(119, 136, 153) },
    { "lightslategrey", rgb(119, 136, 153) },
    { "lightsteelblue", rgb(176, 196, 222) },
    { "lightyellow", rgb(255, 255, 224) },
    { "lime", rgb( 0, 255, 0) },
    { "limegreen", rgb( 50, 205, 50) },
    { "linen", rgb(250, 240, 230) },
    { "magenta", rgb(255, 0, 255) },
    { "maroon", rgb(128, 0, 0) },
    { "mediumaquamarine", rgb(102, 205, 170) },
    { "mediumblue", rgb( 0, 0, 205) },
    { "mediumorchid", rgb(186, 85, 211) },
    { "mediumpurple", rgb(147, 112, 219) },
    { "mediumseagreen", rgb( 60, 179, 113) },
    { "mediumslateblue", rgb(123, 104, 238) },
    { "mediumspringgreen", rgb( 0, 250, 154) },
    { "mediumturquoise", rgb( 72, 209, 204) },
    { "mediumvioletred", rgb(199, 21, 133) },
    { "midnightblue", rgb( 25, 25, 112) },
    { "mintcream", rgb(245, 255, 250) },
    { "mistyrose", rgb(255, 228, 225) },
    { "moccasin", rgb(255, 228, 181) },
    { "navajowhite", rgb(255, 222, 173) },
    { "navy", rgb( 0, 0, 128) },
    { "oldlace", rgb(253, 245, 230) },
    { "olive", rgb(128, 128, 0) },
    { "olivedrab", rgb(107, 142, 35) },
    { "orange", rgb(255, 165, 0) },
    { "orangered", rgb(255, 69, 0) },
    { "orchid", rgb(218, 112, 214) },
    { "palegoldenrod", rgb(238, 232, 170) },
    { "palegreen", rgb(152, 251, 152) },
    { "paleturquoise", rgb(175, 238, 238) },
    { "palevioletred", rgb(219, 112, 147) },
    { "papayawhip", rgb(255, 239, 213) },
    { "peachpuff", rgb(255, 218, 185) },
    { "peru", rgb(205, 133, 63) },
    { "pink", rgb(255, 192, 203) },
    { "plum", rgb(221, 160, 221) },
    { "powderblue", rgb(176, 224, 230) },
    { "purple", rgb(128, 0, 128) },
    { "red", rgb(255, 0, 0) },
    { "rosybrown", rgb(188, 143, 143) },
    { "royalblue", rgb( 65, 105, 225) },
    { "saddlebrown", rgb(139, 69, 19) },
    { "salmon", rgb(250, 128, 114) },
    { "sandybrown", rgb(244, 164, 96) },
    { "seagreen", rgb( 46, 139, 87) },
    { "seashell", rgb(255, 245, 238) },
    { "sienna", rgb(160, 82, 45) },
    { "silver", rgb(192, 192, 192) },
    { "skyblue", rgb(135, 206, 235) },
    { "slateblue", rgb(106, 90, 205) },
    { "slategray", rgb(112, 128, 144) },
    { "slategrey", rgb(112, 128, 144) },
    { "snow", rgb(255, 250, 250) },
    { "springgreen", rgb( 0, 255, 127) },
    { "steelblue", rgb( 70, 130, 180) },
    { "tan", rgb(210, 180, 140) },
    { "teal", rgb( 0, 128, 128) },
    { "thistle", rgb(216, 191, 216) },
    { "tomato", rgb(255, 99, 71) },
    { "transparent", 0 },
    { "turquoise", rgb( 64, 224, 208) },
    { "violet", rgb(238, 130, 238) },
    { "wheat", rgb(245, 222, 179) },
    { "white", rgb(255, 255, 255) },
    { "whitesmoke", rgb(245, 245, 245) },
    { "yellow", rgb(255, 255, 0) },
    { "yellowgreen", rgb(154, 205, 50) }
};

static const int rgbTblSize = sizeof(rgbTbl) / sizeof(RGBData);

static_assert([] {
    for (auto e : rgbTbl) {
        for (auto it = e.name; *it ; ++it) {
            if (uchar(*it) > 127)
                return false;
        }
    }
    return true;
    }(), "the lookup code expects color names to be US-ASCII-only");

#undef rgb

inline bool operator<(const char *name, const RGBData &data)
{ return qstrcmp(name, data.name) < 0; }
inline bool operator<(const RGBData &data, const char *name)
{ return qstrcmp(data.name, name) < 0; }

static std::optional<QRgb> get_named_rgb_no_space(const char *name_no_space)
{
    const RGBData *r = std::lower_bound(rgbTbl, rgbTbl + rgbTblSize, name_no_space);
    if ((r != rgbTbl + rgbTblSize) && !(name_no_space < *r))
        return r->value;
    return std::nullopt;
}

namespace {
// named colors are US-ASCII (enforced by static_assert above):
static char to_char(char ch) noexcept { return ch; }
static char to_char(QChar ch) noexcept { return ch.toLatin1(); }
}

static std::optional<QRgb> get_named_rgb(QAnyStringView name)
{
    if (name.size() > 255)
        return std::nullopt;
    char name_no_space[256];
    int pos = 0;
    name.visit([&pos, &name_no_space] (auto name) {
        for (auto c : name) {
            if (c != u'\t' && c != u' ')
                name_no_space[pos++] = QtMiscUtils::toAsciiLower(to_char(c));
        }
    });
    name_no_space[pos] = 0;

    return get_named_rgb_no_space(name_no_space);
}

#endif // QT_NO_COLORNAMES

static QStringList get_colornames()
{
    QStringList lst;
#ifndef QT_NO_COLORNAMES
    lst.reserve(rgbTblSize);
    for (int i = 0; i < rgbTblSize; i++)
        lst << QLatin1StringView(rgbTbl[i].name);
#endif
    return lst;
}

/*!
    \class QColor
    \brief The QColor class provides colors based on RGB, HSV or CMYK values.

    \ingroup painting
    \ingroup appearance
    \inmodule QtGui


    A color is normally specified in terms of RGB (red, green, and
    blue) components, but it is also possible to specify it in terms
    of HSV (hue, saturation, and value) and CMYK (cyan, magenta,
    yellow and black) components. In addition a color can be specified
    using a color name. The color name can be any of the SVG 1.0 color
    names.

    \table
    \header
    \li RGB \li HSV \li CMYK
    \row
    \li \inlineimage qcolor-rgb.png
    \li \inlineimage qcolor-hsv.png
    \li \inlineimage qcolor-cmyk.png
    \endtable

    The QColor constructor creates the color based on RGB values.  To
    create a QColor based on either HSV or CMYK values, use the
    toHsv() and toCmyk() functions respectively. These functions
    return a copy of the color using the desired format. In addition
    the static fromRgb(), fromHsv() and fromCmyk() functions create
    colors from the specified values. Alternatively, a color can be
    converted to any of the three formats using the convertTo()
    function (returning a copy of the color in the desired format), or
    any of the setRgb(), setHsv() and setCmyk() functions altering \e
    this color's format. The spec() function tells how the color was
    specified.

    A color can be set by passing an RGB string (such as "#112233"),
    or an ARGB string (such as "#ff112233") or a color name (such as "blue"),
    to the fromString() function.
    The color names are taken from the SVG 1.0 color names. The name()
    function returns the name of the color in the format
    "#RRGGBB". Colors can also be set using setRgb(), setHsv() and
    setCmyk(). To get a lighter or darker color use the lighter() and
    darker() functions respectively.

    The isValid() function indicates whether a QColor is legal at
    all. For example, a RGB color with RGB values out of range is
    illegal. For performance reasons, QColor mostly disregards illegal
    colors, and for that reason, the result of using an invalid color
    is undefined.

    The color components can be retrieved individually, e.g with
    red(), hue() and cyan(). The values of the color components can
    also be retrieved in one go using the getRgb(), getHsv() and
    getCmyk() functions. Using the RGB color model, the color
    components can in addition be accessed with rgb().

    There are several related non-members: QRgb is a typdef for an
    unsigned int representing the RGB value triplet (r, g, b). Note
    that it also can hold a value for the alpha-channel (for more
    information, see the \l {QColor#Alpha-Blended
    Drawing}{Alpha-Blended Drawing} section). The qRed(), qBlue() and
    qGreen() functions return the respective component of the given
    QRgb value, while the qRgb() and qRgba() functions create and
    return the QRgb triplet based on the given component
    values. Finally, the qAlpha() function returns the alpha component
    of the provided QRgb, and the qGray() function calculates and
    return a gray value based on the given value.

    QColor is platform and device independent. The QColormap class
    maps the color to the hardware.

    For more information about painting in general, see the \l{Paint
    System} documentation.

    \tableofcontents

    \section1 Integer vs. Floating Point Precision

    QColor supports floating point precision and provides floating
    point versions of all the color components functions,
    e.g. getRgbF(), hueF() and fromCmykF(). Note that since the
    components are stored using 16-bit integers, there might be minor
    deviations between the values set using, for example, setRgbF()
    and the values returned by the getRgbF() function due to rounding.

    While the integer based functions take values in the range 0-255
    (except hue() which must have values within the range 0-359),
    the floating point functions accept values in the range 0.0 - 1.0.

    \section1 Alpha-Blended Drawing

    QColor also support alpha-blended outlining and filling. The
    alpha channel of a color specifies the transparency effect, 0
    represents a fully transparent color, while 255 represents a fully
    opaque color. For example:

    \snippet code/src_gui_painting_qcolor.cpp 0

    The code above produces the following output:

    \image alphafill.png

    The alpha channel of a color can be retrieved and set using the
    alpha() and setAlpha() functions if its value is an integer, and
    alphaF() and setAlphaF() if its value is float. By
    default, the alpha-channel is set to 255 (opaque). To retrieve and
    set \e all the RGB color components (including the alpha-channel)
    in one go, use the rgba() and setRgba() functions.

    \section1 Predefined Colors

    There are 20 predefined QColor objects in the \c{QColorConstants}
    namespace, including black, white, primary and secondary colors,
    darker versions of these colors, and three shades of gray.
    Furthermore, the \c{QColorConstants::Svg} namespace defines QColor
    objects for the standard \l{https://www.w3.org/TR/SVG11/types.html#ColorKeywords}{SVG color keyword names}.

    \image qt-colors.png Qt Colors

    The \c{QColorConstants::Color0}, \c{QColorConstants::Color1} and
    \c{QColorConstants::Transparent} colors are used for special
    purposes.

    \c{QColorConstants::Color0} (zero pixel value) and
    \c{QColorConstants::Color1} (non-zero pixel value) are special
    colors for drawing in QBitmaps. Painting with
    \c{QColorConstants::Color0} sets the bitmap bits to 0 (transparent;
    i.e., background), and painting with c{QColorConstants::Color1}
    sets the bits to 1 (opaque; i.e., foreground).

    \c{QColorConstants::Transparent} is used to indicate a transparent
    pixel. When painting with this value, a pixel value will be used
    that is appropriate for the underlying pixel format in use.

    For historical reasons, the 20 predefined colors are also available
    in the Qt::GlobalColor enumeration.

    Finally, QColor recognizes a variety of color names (as strings);
    the static colorNames() function returns a QStringList color names
    that QColor knows about.

    \section1 The Extended RGB Color Model

    The extended RGB color model, also known as the scRGB color space,
    is the same the RGB color model except it allows values under 0.0,
    and over 1.0. This makes it possible to represent colors that would
    otherwise be outside the range of the RGB colorspace but still use
    the same values for colors inside the RGB colorspace.

    \section1 The HSV Color Model

    The RGB model is hardware-oriented. Its representation is close to
    what most monitors show. In contrast, HSV represents color in a way
    more suited to the human perception of color. For example, the
    relationships "stronger than", "darker than", and "the opposite of"
    are easily expressed in HSV but are much harder to express in RGB.

    HSV, like RGB, has three components:

    \list
    \li H, for hue, is in the range 0 to 359 if the color is chromatic (not
    gray), or meaningless if it is gray. It represents degrees on the
    color wheel familiar to most people. Red is 0 (degrees), green is
    120, and blue is 240.

    \inlineimage qcolor-hue.png

    \li S, for saturation, is in the range 0 to 255, and the bigger it is,
    the stronger the color is. Grayish colors have saturation near 0; very
    strong colors have saturation near 255.

    \inlineimage qcolor-saturation.png

    \li V, for value, is in the range 0 to 255 and represents lightness or
    brightness of the color. 0 is black; 255 is as far from black as
    possible.

    \inlineimage qcolor-value.png
    \endlist

    Here are some examples: pure red is H=0, S=255, V=255; a dark red,
    moving slightly towards the magenta, could be H=350 (equivalent to
    -10), S=255, V=180; a grayish light red could have H about 0 (say
    350-359 or 0-10), S about 50-100, and S=255.

    Qt returns a hue value of -1 for achromatic colors. If you pass a
    hue value that is too large, Qt forces it into range. Hue 360 or 720 is
    treated as 0; hue 540 is treated as 180.

    In addition to the standard HSV model, Qt provides an
    alpha-channel to feature \l {QColor#Alpha-Blended
    Drawing}{alpha-blended drawing}.

    \section1 The HSL Color Model

    HSL is similar to HSV, however instead of the Value parameter, HSL
    specifies a Lightness parameter which maps somewhat differently to the
    brightness of the color.

    Similarly, the HSL saturation value is not in general the same as the HSV
    saturation value for the same color. hslSaturation() provides the color's
    HSL saturation value, while saturation() and hsvSaturation() provides the
    HSV saturation value.

    The hue value is defined to be the same in HSL and HSV.

    \section1 The CMYK Color Model

    While the RGB and HSV color models are used for display on
    computer monitors, the CMYK model is used in the four-color
    printing process of printing presses and some hard-copy
    devices.

    CMYK has four components, all in the range 0-255: cyan (C),
    magenta (M), yellow (Y) and black (K).  Cyan, magenta and yellow
    are called subtractive colors; the CMYK color model creates color
    by starting with a white surface and then subtracting color by
    applying the appropriate components. While combining cyan, magenta
    and yellow gives the color black, subtracting one or more will
    yield any other color. When combined in various percentages, these
    three colors can create the entire spectrum of colors.

    Mixing 100 percent of cyan, magenta and yellow \e does produce
    black, but the result is unsatisfactory since it wastes ink,
    increases drying time, and gives a muddy colour when printing. For
    that reason, black is added in professional printing to provide a
    solid black tone; hence the term 'four color process'.

    In addition to the standard CMYK model, Qt provides an
    alpha-channel to feature \l {QColor#Alpha-Blended
    Drawing}{alpha-blended drawing}.

    \sa QPalette, QBrush, QColorConstants
*/

#define QCOLOR_INT_RANGE_CHECK(fn, var) \
    do { \
        if (var < 0 || var > 255) { \
            qWarning(#fn": invalid value %d", var); \
            var = qMax(0, qMin(var, 255)); \
        } \
    } while (0)

#define QCOLOR_REAL_RANGE_CHECK(fn, var) \
    do { \
        if (var < 0.0f || var > 1.0f) { \
            qWarning(#fn": invalid value %g", var); \
            var = qMax(0.0f, qMin(var, 1.0f));      \
        } \
    } while (0)

/*****************************************************************************
  QColor member functions
 *****************************************************************************/

/*!
    \enum QColor::Spec

    The type of color specified, either RGB, extended RGB, HSV, CMYK or HSL.

    \value Rgb
    \value Hsv
    \value Cmyk
    \value Hsl
    \value ExtendedRgb
    \value Invalid

    \sa spec(), convertTo()
*/

/*!
    \enum QColor::NameFormat

    How to format the output of the name() function

    \value HexRgb #RRGGBB A "#" character followed by three two-digit hexadecimal numbers (i.e. \c{#RRGGBB}).
    \value HexArgb #AARRGGBB A "#" character followed by four two-digit hexadecimal numbers (i.e. \c{#AARRGGBB}).

    \sa name()
*/

/*!
    \fn Spec QColor::spec() const

    Returns how the color was specified.

    \sa Spec, convertTo()
*/


/*!
    \fn QColor::QColor()

    Constructs an invalid color with the RGB value (0, 0, 0). An
    invalid color is a color that is not properly set up for the
    underlying window system.

    The alpha value of an invalid color is unspecified.

    \sa isValid()
*/

/*!
    \overload

    Constructs a new color with a color value of \a color.

    \sa isValid(), {QColor#Predefined Colors}{Predefined Colors}
 */
QColor::QColor(Qt::GlobalColor color) noexcept
{
#define QRGB(r, g, b) \
    QRgb(((0xffu << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)))
#define QRGBA(r, g, b, a) \
    QRgb(((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff))

    static const QRgb global_colors[] = {
        QRGB(255, 255, 255), // Qt::color0
        QRGB(  0,   0,   0), // Qt::color1
        QRGB(  0,   0,   0), // black
        QRGB(255, 255, 255), // white
        /*
         * From the "The Palette Manager: How and Why" by Ron Gery,
         * March 23, 1992, archived on MSDN:
         *
         *     The Windows system palette is broken up into two
         *     sections, one with fixed colors and one with colors
         *     that can be changed by applications. The system palette
         *     predefines 20 entries; these colors are known as the
         *     static or reserved colors and consist of the 16 colors
         *     found in the Windows version 3.0 VGA driver and 4
         *     additional colors chosen for their visual appeal.  The
         *     DEFAULT_PALETTE stock object is, as the name implies,
         *     the default palette selected into a device context (DC)
         *     and consists of these static colors. Applications can
         *     set the remaining 236 colors using the Palette Manager.
         *
         * The 20 reserved entries have indices in [0,9] and
         * [246,255]. We reuse 17 of them.
         */
        QRGB(128, 128, 128), // index 248   medium gray
        QRGB(160, 160, 164), // index 247   light gray
        QRGB(192, 192, 192), // index 7     light gray
        QRGB(255,   0,   0), // index 249   red
        QRGB(  0, 255,   0), // index 250   green
        QRGB(  0,   0, 255), // index 252   blue
        QRGB(  0, 255, 255), // index 254   cyan
        QRGB(255,   0, 255), // index 253   magenta
        QRGB(255, 255,   0), // index 251   yellow
        QRGB(128,   0,   0), // index 1     dark red
        QRGB(  0, 128,   0), // index 2     dark green
        QRGB(  0,   0, 128), // index 4     dark blue
        QRGB(  0, 128, 128), // index 6     dark cyan
        QRGB(128,   0, 128), // index 5     dark magenta
        QRGB(128, 128,   0), // index 3     dark yellow
        QRGBA(0, 0, 0, 0)    //             transparent
    };
#undef QRGB
#undef QRGBA

    setRgb(qRed(global_colors[color]),
           qGreen(global_colors[color]),
           qBlue(global_colors[color]),
           qAlpha(global_colors[color]));
}

/*!
    \fn QColor::QColor(int r, int g, int b, int a = 255)

    Constructs a color with the RGB value \a r, \a g, \a b, and the
    alpha-channel (transparency) value of \a a.

    The color is left invalid if any of the arguments are invalid.

    \sa setRgba(), isValid()
*/

/*!
    Constructs a color with the value \a color. The alpha component is
    ignored and set to solid.

    \sa fromRgb(), isValid()
*/

QColor::QColor(QRgb color) noexcept
{
    cspec = Rgb;
    ct.argb.alpha = 0xffff;
    ct.argb.red   = qRed(color)   * 0x101;
    ct.argb.green = qGreen(color) * 0x101;
    ct.argb.blue  = qBlue(color)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \since 5.6

    Constructs a color with the value \a rgba64.

    \sa fromRgba64()
*/

QColor::QColor(QRgba64 rgba64) noexcept
{
    setRgba64(rgba64);
}

/*!
    \internal

    Constructs a color with the given \a spec.

    This function is primarily present to avoid that QColor::Invalid
    becomes a valid color by accident.
*/

QColor::QColor(Spec spec) noexcept
{
    switch (spec) {
    case Invalid:
        invalidate();
        break;
    case Rgb:
        setRgb(0, 0, 0);
        break;
    case Hsv:
        setHsv(0, 0, 0);
        break;
    case Cmyk:
        setCmyk(0, 0, 0, 0);
        break;
    case Hsl:
        setHsl(0, 0, 0, 0);
        break;
    case ExtendedRgb:
        cspec = spec;
        setRgbF(0, 0, 0, 0);
        break;
    }
}

// ### Qt 7: remove those after deprecating them for the last Qt 6 LTS release
/*!
    \fn QColor::QColor(const QString &name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    The color is left invalid if the \a name cannot be parsed.

    \sa setNamedColor(), name(), isValid()
*/

/*!
    \fn QColor::QColor(const char *name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    \overload
    \sa setNamedColor(), name(), isValid()
*/

/*!
    \fn QColor::QColor(QLatin1StringView name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    \overload
    \since 5.8
    \sa setNamedColor(), name(), isValid()
*/

/*!
    \fn bool QColor::isValid() const

    Returns \c true if the color is valid; otherwise returns \c false.
*/

/*!
    \since 5.2

    Returns the name of the color in the specified \a format.

    \sa fromString(), NameFormat
*/

QString QColor::name(NameFormat format) const
{
    switch (format) {
    case HexRgb:
        return u'#' + QStringView{QString::number(rgba() | 0x1000000, 16)}.right(6);
    case HexArgb:
        // it's called rgba() but it does return AARRGGBB
        return u'#' + QStringView{QString::number(rgba() | Q_INT64_C(0x100000000), 16)}.right(8);
    }
    return QString();
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use fromString() instead.

    Sets the RGB value of this QColor to \a name, which may be in one
    of these formats:

    \list
    \li #RGB (each of R, G, and B is a single hex digit)
    \li #RRGGBB
    \li #AARRGGBB (Since 5.2)
    \li #RRRGGGBBB
    \li #RRRRGGGGBBBB
    \li A name from the list of colors defined in the list of
       \l{https://www.w3.org/TR/SVG11/types.html#ColorKeywords}{SVG color keyword names}
       provided by the World Wide Web Consortium; for example, "steelblue" or "gainsboro".
       These color names work on all platforms. Note that these color names are \e not the
       same as defined by the Qt::GlobalColor enums, e.g. "green" and Qt::green does not
       refer to the same color.
    \li \c transparent - representing the absence of a color.
    \endlist

    The color is invalid if \a name cannot be parsed.

    \sa QColor(), name(), isValid()
*/

void QColor::setNamedColor(const QString &name)
{
    *this = fromString(qToAnyStringViewIgnoringNull(name));
}

/*!
    \overload
    \since 5.10
    \deprecated [6.6] Use fromString() instead.
*/

void QColor::setNamedColor(QStringView name)
{
    *this = fromString(name);
}

/*!
    \overload
    \since 5.8
    \deprecated [6.6] Use fromString() instead.
*/

void QColor::setNamedColor(QLatin1StringView name)
{
    *this = fromString(name);
}

/*!
   \since 4.7

   \deprecated [6.6] Use isValidColorName() instead.

   Returns \c true if the \a name is a valid color name and can
   be used to construct a valid QColor object, otherwise returns
   false.

   It uses the same algorithm used in setNamedColor().

   \sa setNamedColor()
*/
bool QColor::isValidColor(const QString &name)
{
    return isValidColorName(qToAnyStringViewIgnoringNull(name));
}

/*!
    \overload
    \since 5.10
    \deprecated [6.6] Use isValidColorName() instead.
*/
bool QColor::isValidColor(QStringView name) noexcept
{
    return isValidColorName(name);
}

/*!
    \overload
    \since 5.8
    \deprecated [6.6] Use isValidColorName() instead.
*/
bool QColor::isValidColor(QLatin1StringView name) noexcept
{
    return isValidColorName(name);
}
#endif // QT_DEPRECATED_SINCE(6, 6)

/*!
   \since 6.4

   Returns \c true if the \a name is a valid color name and can
   be used to construct a valid QColor object, otherwise returns
   false.

   It uses the same algorithm used in fromString().

   \sa fromString()
*/
bool QColor::isValidColorName(QAnyStringView name) noexcept
{
    return fromString(name).isValid();
}

/*!
    \since 6.4

    Returns an RGB QColor parsed from \a name, which may be in one
    of these formats:

    \list
    \li #RGB (each of R, G, and B is a single hex digit)
    \li #RRGGBB
    \li #AARRGGBB (Since 5.2)
    \li #RRRGGGBBB
    \li #RRRRGGGGBBBB
    \li A name from the list of colors defined in the list of
       \l{https://www.w3.org/TR/SVG11/types.html#ColorKeywords}{SVG color keyword names}
       provided by the World Wide Web Consortium; for example, "steelblue" or "gainsboro".
       These color names work on all platforms. Note that these color names are \e not the
       same as defined by the Qt::GlobalColor enums, e.g. "green" and Qt::green does not
       refer to the same color.
    \li \c transparent - representing the absence of a color.
    \endlist

    Returns an invalid color if \a name cannot be parsed.

    \sa isValidColorName()
*/
QColor QColor::fromString(QAnyStringView name) noexcept
{
    if (!name.size())
        return {};

    if (name.front() == u'#') {
        if (std::optional<QRgba64> r = get_hex_rgb(name))
            return QColor::fromRgba64(*r);
#ifndef QT_NO_COLORNAMES
    } else if (std::optional<QRgb> r = get_named_rgb(name)) {
        return QColor::fromRgba(*r);
#endif
    }

    return {};
}

/*!
    Returns a QStringList containing the color names Qt knows about.

    \sa {QColor#Predefined Colors}{Predefined Colors}
*/
QStringList QColor::colorNames()
{
    return get_colornames();
}

/*!
    Sets the contents pointed to by \a h, \a s, \a v, and \a a, to the hue,
    saturation, value, and alpha-channel (transparency) components of the
    color's HSV value.

    These components can be retrieved individually using the hueF(),
    saturationF(), valueF() and alphaF() functions.

    \sa setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::getHsvF(float *h, float *s, float *v, float *a) const
{
        if (!h || !s || !v)
        return;

    if (cspec != Invalid && cspec != Hsv) {
        toHsv().getHsvF(h, s, v, a);
        return;
    }

    *h = ct.ahsv.hue == USHRT_MAX ? -1.0f : ct.ahsv.hue / 36000.0f;
    *s = ct.ahsv.saturation / float(USHRT_MAX);
    *v = ct.ahsv.value / float(USHRT_MAX);

    if (a)
        *a = ct.ahsv.alpha / float(USHRT_MAX);
}

/*!
    Sets the contents pointed to by \a h, \a s, \a v, and \a a, to the hue,
    saturation, value, and alpha-channel (transparency) components of the
    color's HSV value.

    These components can be retrieved individually using the hue(),
    saturation(), value() and alpha() functions.

    \sa setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::getHsv(int *h, int *s, int *v, int *a) const
{
    if (!h || !s || !v)
        return;

    if (cspec != Invalid && cspec != Hsv) {
        toHsv().getHsv(h, s, v, a);
        return;
    }

    *h = ct.ahsv.hue == USHRT_MAX ? -1 : ct.ahsv.hue / 100;
    *s = qt_div_257(ct.ahsv.saturation);
    *v = qt_div_257(ct.ahsv.value);

    if (a)
        *a = qt_div_257(ct.ahsv.alpha);
}

/*!
    Sets a HSV color value; \a h is the hue, \a s is the saturation, \a v is
    the value and \a a is the alpha component of the HSV color.

    All the values must be in the range 0.0-1.0.

    \sa getHsvF(), setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::setHsvF(float h, float s, float v, float a)
{
    if (((h < 0.0f || h > 1.0f) && h != -1.0f)
        || (s < 0.0f || s > 1.0f)
        || (v < 0.0f || v > 1.0f)
        || (a < 0.0f || a > 1.0f)) {
        qWarning("QColor::setHsvF: HSV parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsv;
    ct.ahsv.alpha      = qRound(a * USHRT_MAX);
    ct.ahsv.hue        = h == -1.0f ? USHRT_MAX : qRound(h * 36000.0f);
    ct.ahsv.saturation = qRound(s * USHRT_MAX);
    ct.ahsv.value      = qRound(v * USHRT_MAX);
    ct.ahsv.pad        = 0;
}

/*!
    Sets a HSV color value; \a h is the hue, \a s is the saturation, \a v is
    the value and \a a is the alpha component of the HSV color.

    The saturation, value and alpha-channel values must be in the range 0-255,
    and the hue value must be greater than -1.

    \sa getHsv(), setHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::setHsv(int h, int s, int v, int a)
{
    if (h < -1 || (uint)s > 255 || (uint)v > 255 || (uint)a > 255) {
        qWarning("QColor::setHsv: HSV parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsv;
    ct.ahsv.alpha      = a * 0x101;
    ct.ahsv.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    ct.ahsv.saturation = s * 0x101;
    ct.ahsv.value      = v * 0x101;
    ct.ahsv.pad        = 0;
}

/*!
    \since 4.6

    Sets the contents pointed to by \a h, \a s, \a l, and \a a, to the hue,
    saturation, lightness, and alpha-channel (transparency) components of the
    color's HSL value.

    These components can be retrieved individually using the hslHueF(),
    hslSaturationF(), lightnessF() and alphaF() functions.

    \sa getHsl(), setHslF(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
void QColor::getHslF(float *h, float *s, float *l, float *a) const
{
        if (!h || !s || !l)
        return;

    if (cspec != Invalid && cspec != Hsl) {
        toHsl().getHslF(h, s, l, a);
        return;
    }

    *h = ct.ahsl.hue == USHRT_MAX ? -1.0f : ct.ahsl.hue / 36000.0f;
    *s = ct.ahsl.saturation / float(USHRT_MAX);
    *l = ct.ahsl.lightness / float(USHRT_MAX);

    if (a)
        *a = ct.ahsl.alpha / float(USHRT_MAX);
}

/*!
    \since 4.6

    Sets the contents pointed to by \a h, \a s, \a l, and \a a, to the hue,
    saturation, lightness, and alpha-channel (transparency) components of the
    color's HSL value.

    These components can be retrieved individually using the hslHue(),
    hslSaturation(), lightness() and alpha() functions.

    \sa getHslF(), setHsl(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
void QColor::getHsl(int *h, int *s, int *l, int *a) const
{
    if (!h || !s || !l)
        return;

    if (cspec != Invalid && cspec != Hsl) {
        toHsl().getHsl(h, s, l, a);
        return;
    }

    *h = ct.ahsl.hue == USHRT_MAX ? -1 : ct.ahsl.hue / 100;
    *s = qt_div_257(ct.ahsl.saturation);
    *l = qt_div_257(ct.ahsl.lightness);

    if (a)
        *a = qt_div_257(ct.ahsl.alpha);
}

/*!
    \since 4.6

    Sets a HSL color lightness; \a h is the hue, \a s is the saturation, \a l is
    the lightness and \a a is the alpha component of the HSL color.

    All the values must be in the range 0.0-1.0.

    \sa getHslF(), setHsl()
*/
void QColor::setHslF(float h, float s, float l, float a)
{
    if (((h < 0.0f || h > 1.0f) && h != -1.0f)
        || (s < 0.0f || s > 1.0f)
        || (l < 0.0f || l > 1.0f)
        || (a < 0.0f || a > 1.0f)) {
        qWarning("QColor::setHslF: HSL parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsl;
    ct.ahsl.alpha      = qRound(a * USHRT_MAX);
    ct.ahsl.hue        = h == -1.0f ? USHRT_MAX : qRound(h * 36000.0f);
    ct.ahsl.saturation = qRound(s * USHRT_MAX);
    ct.ahsl.lightness  = qRound(l * USHRT_MAX);
    ct.ahsl.pad        = 0;
}

/*!
    \since 4.6

    Sets a HSL color value; \a h is the hue, \a s is the saturation, \a l is
    the lightness and \a a is the alpha component of the HSL color.

    The saturation, value and alpha-channel values must be in the range 0-255,
    and the hue value must be greater than -1.

    \sa getHsl(), setHslF()
*/
void QColor::setHsl(int h, int s, int l, int a)
{
    if (h < -1 || (uint)s > 255 || (uint)l > 255 || (uint)a > 255) {
        qWarning("QColor::setHsl: HSL parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsl;
    ct.ahsl.alpha      = a * 0x101;
    ct.ahsl.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    ct.ahsl.saturation = s * 0x101;
    ct.ahsl.lightness  = l * 0x101;
    ct.ahsl.pad        = 0;
}

static inline qfloat16 &castF16(quint16 &v)
{
    // this works because qfloat16 internally is a quint16
    return *reinterpret_cast<qfloat16 *>(&v);
}

static inline const qfloat16 &castF16(const quint16 &v)
{
    return *reinterpret_cast<const qfloat16 *>(&v);
}

/*!
    Sets the contents pointed to by \a r, \a g, \a b, and \a a, to the red,
    green, blue, and alpha-channel (transparency) components of the color's
    RGB value.

    These components can be retrieved individually using the redF(), greenF(),
    blueF() and alphaF() functions.

    \sa rgb(), setRgb()
*/
void QColor::getRgbF(float *r, float *g, float *b, float *a) const
{
    if (!r || !g || !b)
        return;

    if (cspec != Invalid && cspec != Rgb && cspec != ExtendedRgb) {
        toRgb().getRgbF(r, g, b, a);
        return;
    }

    if (cspec == Rgb || cspec == Invalid) {
        *r = ct.argb.red   / float(USHRT_MAX);
        *g = ct.argb.green / float(USHRT_MAX);
        *b = ct.argb.blue  / float(USHRT_MAX);
        if (a)
            *a = ct.argb.alpha / float(USHRT_MAX);
    }  else {
        *r = castF16(ct.argbExtended.redF16);
        *g = castF16(ct.argbExtended.greenF16);
        *b = castF16(ct.argbExtended.blueF16);
        if (a)
            *a = castF16(ct.argbExtended.alphaF16);
    }
}

/*!
    Sets the contents pointed to by \a r, \a g, \a b, and \a a, to the red,
    green, blue, and alpha-channel (transparency) components of the color's
    RGB value.

    These components can be retrieved individually using the red(), green(),
    blue() and alpha() functions.

    \sa rgb(), setRgb()
*/
void QColor::getRgb(int *r, int *g, int *b, int *a) const
{
    if (!r || !g || !b)
        return;

    if (cspec != Invalid && cspec != Rgb) {
        toRgb().getRgb(r, g, b, a);
        return;
    }

    *r = qt_div_257(ct.argb.red);
    *g = qt_div_257(ct.argb.green);
    *b = qt_div_257(ct.argb.blue);

    if (a)
        *a = qt_div_257(ct.argb.alpha);
}

/*!
    \fn void QColor::setRgbF(float r, float g, float b, float a)

    Sets the color channels of this color to \a r (red), \a g (green),
    \a b (blue) and \a a (alpha, transparency).

    The alpha value must be in the range 0.0-1.0.
    If any of the other values are outside the range of 0.0-1.0 the
    color model will be set as \c ExtendedRgb.

    \sa rgb(), getRgbF(), setRgb()
*/
void QColor::setRgbF(float r, float g, float b, float a)
{
    if (a < 0.0f || a > 1.0f) {
        qWarning("QColor::setRgbF: Alpha parameter is out of range");
        invalidate();
        return;
    }
    if (r < 0.0f || r > 1.0f ||
        g < 0.0f || g > 1.0f ||
        b < 0.0f || b > 1.0f || cspec == ExtendedRgb) {
        cspec = ExtendedRgb;
        castF16(ct.argbExtended.redF16)   = qfloat16(r);
        castF16(ct.argbExtended.greenF16) = qfloat16(g);
        castF16(ct.argbExtended.blueF16)  = qfloat16(b);
        castF16(ct.argbExtended.alphaF16) = qfloat16(a);
        ct.argbExtended.pad   = 0;
        return;
    }
    cspec = Rgb;
    ct.argb.red   = qRound(r * USHRT_MAX);
    ct.argb.green = qRound(g * USHRT_MAX);
    ct.argb.blue  = qRound(b * USHRT_MAX);
    ct.argb.alpha = qRound(a * USHRT_MAX);
    ct.argb.pad   = 0;
}

/*!
    Sets the RGB value to \a r, \a g, \a b and the alpha value to \a a.

    All the values must be in the range 0-255.

    \sa rgb(), getRgb(), setRgbF()
*/
void QColor::setRgb(int r, int g, int b, int a)
{
    if (!isRgbaValid(r, g, b, a)) {
        qWarning("QColor::setRgb: RGB parameters out of range");
        invalidate();
        return;
    }

    cspec = Rgb;
    ct.argb.alpha = a * 0x101;
    ct.argb.red   = r * 0x101;
    ct.argb.green = g * 0x101;
    ct.argb.blue  = b * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \fn QRgb QColor::rgba() const

    Returns the RGB value of the color, including its alpha.

    For an invalid color, the alpha value of the returned color is unspecified.

    \sa setRgba(), rgb(), rgba64()
*/

QRgb QColor::rgba() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgba();
    return qRgba(qt_div_257(ct.argb.red), qt_div_257(ct.argb.green), qt_div_257(ct.argb.blue), qt_div_257(ct.argb.alpha));
}

/*!
    Sets the RGB value to \a rgba, including its alpha.

    \sa rgba(), rgb(), setRgba64()
*/
void QColor::setRgba(QRgb rgba) noexcept
{
    cspec = Rgb;
    ct.argb.alpha = qAlpha(rgba) * 0x101;
    ct.argb.red   = qRed(rgba)   * 0x101;
    ct.argb.green = qGreen(rgba) * 0x101;
    ct.argb.blue  = qBlue(rgba)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \since 5.6

    Returns the RGB64 value of the color, including its alpha.

    For an invalid color, the alpha value of the returned color is unspecified.

    \sa setRgba64(), rgba(), rgb()
*/

QRgba64 QColor::rgba64() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgba64();
    return qRgba64(ct.argb.red, ct.argb.green, ct.argb.blue, ct.argb.alpha);
}

/*!
    \since 5.6

    Sets the RGB64 value to \a rgba, including its alpha.

    \sa setRgba(), rgba64()
*/
void QColor::setRgba64(QRgba64 rgba) noexcept
{
    cspec = Rgb;
    ct.argb.alpha = rgba.alpha();
    ct.argb.red   = rgba.red();
    ct.argb.green = rgba.green();
    ct.argb.blue  = rgba.blue();
    ct.argb.pad   = 0;
}

/*!
    \fn QRgb QColor::rgb() const

    Returns the RGB value of the color. The alpha value is opaque.

    \sa getRgb(), rgba()
*/
QRgb QColor::rgb() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgb();
    return qRgb(qt_div_257(ct.argb.red), qt_div_257(ct.argb.green), qt_div_257(ct.argb.blue));
}

/*!
    \overload

    Sets the RGB value to \a rgb. The alpha value is set to opaque.
*/
void QColor::setRgb(QRgb rgb) noexcept
{
    cspec = Rgb;
    ct.argb.alpha = 0xffff;
    ct.argb.red   = qRed(rgb)   * 0x101;
    ct.argb.green = qGreen(rgb) * 0x101;
    ct.argb.blue  = qBlue(rgb)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    Returns the alpha color component of this color.

    \sa setAlpha(), alphaF(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/
int QColor::alpha() const noexcept
{
    if (cspec == ExtendedRgb)
        return qRound(float(castF16(ct.argbExtended.alphaF16)) * 255);
    return qt_div_257(ct.argb.alpha);
}


/*!
    Sets the alpha of this color to \a alpha. Integer alpha is specified in the
    range 0-255.

    \sa alpha(), alphaF(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/

void QColor::setAlpha(int alpha)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setAlpha", alpha);
    if (cspec == ExtendedRgb) {
        constexpr float f = 1.0f / 255;
        castF16(ct.argbExtended.alphaF16) = qfloat16(alpha * f);
        return;
    }
    ct.argb.alpha = alpha * 0x101;
}

/*!
    Returns the alpha color component of this color.

    \sa setAlphaF(), alpha(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/
float QColor::alphaF() const noexcept
{
    if (cspec == ExtendedRgb)
        return castF16(ct.argbExtended.alphaF16);
    return ct.argb.alpha / float(USHRT_MAX);
}

/*!
    Sets the alpha of this color to \a alpha. float alpha is specified in the
    range 0.0-1.0.

    \sa alphaF(), alpha(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}

*/
void QColor::setAlphaF(float alpha)
{
    QCOLOR_REAL_RANGE_CHECK("QColor::setAlphaF", alpha);
    if (cspec == ExtendedRgb) {
        castF16(ct.argbExtended.alphaF16) = qfloat16(alpha);
        return;
    }
    float tmp = alpha * USHRT_MAX;
    ct.argb.alpha = qRound(tmp);
}


/*!
    Returns the red color component of this color.

    \sa setRed(), redF(), getRgb()
*/
int QColor::red() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().red();
    return qt_div_257(ct.argb.red);
}

/*!
    Sets the red color component of this color to \a red. Integer components
    are specified in the range 0-255.

    \sa red(), redF(), setRgb()
*/
void QColor::setRed(int red)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setRed", red);
    if (cspec != Rgb)
        setRgb(red, green(), blue(), alpha());
    else
        ct.argb.red = red * 0x101;
}

/*!
    Returns the green color component of this color.

    \sa setGreen(), greenF(), getRgb()
*/
int QColor::green() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().green();
    return qt_div_257(ct.argb.green);
}

/*!
    Sets the green color component of this color to \a green. Integer
    components are specified in the range 0-255.

    \sa green(), greenF(), setRgb()
*/
void QColor::setGreen(int green)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setGreen", green);
    if (cspec != Rgb)
        setRgb(red(), green, blue(), alpha());
    else
        ct.argb.green = green * 0x101;
}


/*!
    Returns the blue color component of this color.

    \sa setBlue(), blueF(), getRgb()
*/
int QColor::blue() const noexcept
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().blue();
    return qt_div_257(ct.argb.blue);
}


/*!
    Sets the blue color component of this color to \a blue. Integer components
    are specified in the range 0-255.

    \sa blue(), blueF(), setRgb()
*/
void QColor::setBlue(int blue)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setBlue", blue);
    if (cspec != Rgb)
        setRgb(red(), green(), blue, alpha());
    else
        ct.argb.blue = blue * 0x101;
}

/*!
    Returns the red color component of this color.

    \sa setRedF(), red(), getRgbF()
*/
float QColor::redF() const noexcept
{
    if (cspec == Rgb || cspec == Invalid)
        return ct.argb.red / float(USHRT_MAX);
    if (cspec == ExtendedRgb)
        return castF16(ct.argbExtended.redF16);

    return toRgb().redF();
}


/*!
    Sets the red color component of this color to \a red. If \a red lies outside
    the 0.0-1.0 range, the color model will be changed to \c ExtendedRgb.

    \sa redF(), red(), setRgbF()
*/
void QColor::setRedF(float red)
{
    if (cspec == Rgb && red >= 0.0f && red <= 1.0f)
        ct.argb.red = qRound(red * USHRT_MAX);
    else if (cspec == ExtendedRgb)
        castF16(ct.argbExtended.redF16) = qfloat16(red);
    else
        setRgbF(red, greenF(), blueF(), alphaF());
}

/*!
    Returns the green color component of this color.

    \sa setGreenF(), green(), getRgbF()
*/
float QColor::greenF() const noexcept
{
    if (cspec == Rgb || cspec == Invalid)
        return ct.argb.green / float(USHRT_MAX);
    if (cspec == ExtendedRgb)
        return castF16(ct.argbExtended.greenF16);

    return toRgb().greenF();
}


/*!
    Sets the green color component of this color to \a green. If \a green lies outside
    the 0.0-1.0 range, the color model will be changed to \c ExtendedRgb.

    \sa greenF(), green(), setRgbF()
*/
void QColor::setGreenF(float green)
{
    if (cspec == Rgb && green >= 0.0f && green <= 1.0f)
        ct.argb.green = qRound(green * USHRT_MAX);
    else if (cspec == ExtendedRgb)
        castF16(ct.argbExtended.greenF16) = qfloat16(green);
    else
        setRgbF(redF(), green, blueF(), alphaF());
}

/*!
    Returns the blue color component of this color.

     \sa setBlueF(), blue(), getRgbF()
*/
float QColor::blueF() const noexcept
{
    if (cspec == Rgb || cspec == Invalid)
        return ct.argb.blue / float(USHRT_MAX);
    if (cspec == ExtendedRgb)
        return castF16(ct.argbExtended.blueF16);

    return toRgb().blueF();
}

/*!
    Sets the blue color component of this color to \a blue. If \a blue lies outside
    the 0.0-1.0 range, the color model will be changed to \c ExtendedRgb.
    \sa blueF(), blue(), setRgbF()
*/
void QColor::setBlueF(float blue)
{
    if (cspec == Rgb && blue >= 0.0f && blue <= 1.0f)
        ct.argb.blue = qRound(blue * USHRT_MAX);
    else if (cspec == ExtendedRgb)
        castF16(ct.argbExtended.blueF16) = qfloat16(blue);
    else
        setRgbF(redF(), greenF(), blue, alphaF());
}

/*!
    Returns the HSV hue color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvHue(), hslHue(), hueF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/

int QColor::hue() const noexcept
{
    return hsvHue();
}

/*!
    Returns the HSV hue color component of this color.

    \sa hueF(), hslHue(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::hsvHue() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().hue();
    return ct.ahsv.hue == USHRT_MAX ? -1 : ct.ahsv.hue / 100;
}

/*!
    Returns the HSV saturation color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvSaturation(), hslSaturation(), saturationF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/

int QColor::saturation() const noexcept
{
    return hsvSaturation();
}

/*!
    Returns the HSV saturation color component of this color.

    \sa saturationF(), hslSaturation(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::hsvSaturation() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().saturation();
    return qt_div_257(ct.ahsv.saturation);
}

/*!
    Returns the value color component of this color.

    \sa valueF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::value() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().value();
    return qt_div_257(ct.ahsv.value);
}

/*!
    Returns the HSV hue color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvHueF(), hslHueF(), hue(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
float QColor::hueF() const noexcept
{
    return hsvHueF();
}

/*!
    Returns the hue color component of this color.

    \sa hue(), hslHueF(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/
float QColor::hsvHueF() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().hueF();
    return ct.ahsv.hue == USHRT_MAX ? -1.0f : ct.ahsv.hue / 36000.0f;
}

/*!
    Returns the HSV saturation color component of this color.

     The color is implicitly converted to HSV.

    \sa hsvSaturationF(), hslSaturationF(), saturation(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/
float QColor::saturationF() const noexcept
{
    return hsvSaturationF();
}

/*!
    Returns the HSV saturation color component of this color.

    \sa saturation(), hslSaturationF(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
float QColor::hsvSaturationF() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().saturationF();
    return ct.ahsv.saturation / float(USHRT_MAX);
}

/*!
    Returns the value color component of this color.

    \sa value(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
float QColor::valueF() const noexcept
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().valueF();
    return ct.ahsv.value / float(USHRT_MAX);
}

/*!
    \since 4.6

    Returns the HSL hue color component of this color.

    \sa hslHueF(), hsvHue(), getHsl(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
int QColor::hslHue() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslHue();
    return ct.ahsl.hue == USHRT_MAX ? -1 : ct.ahsl.hue / 100;
}

/*!
    \since 4.6

    Returns the HSL saturation color component of this color.

    \sa hslSaturationF(), hsvSaturation(), getHsl(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
int QColor::hslSaturation() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslSaturation();
    return qt_div_257(ct.ahsl.saturation);
}

/*!
    \since 4.6

    Returns the lightness color component of this color.

    \sa lightnessF(), getHsl()
*/
int QColor::lightness() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().lightness();
    return qt_div_257(ct.ahsl.lightness);
}

/*!
    \since 4.6

    Returns the HSL hue color component of this color.

    \sa hslHue(), hsvHueF(), getHslF()
*/
float QColor::hslHueF() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslHueF();
    return ct.ahsl.hue == USHRT_MAX ? -1.0f : ct.ahsl.hue / 36000.0f;
}

/*!
    \since 4.6

    Returns the HSL saturation color component of this color.

    \sa hslSaturation(), hsvSaturationF(), getHslF(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
float QColor::hslSaturationF() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslSaturationF();
    return ct.ahsl.saturation / float(USHRT_MAX);
}

/*!
    \since 4.6

    Returns the lightness color component of this color.

    \sa value(), getHslF()
*/
float QColor::lightnessF() const noexcept
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().lightnessF();
    return ct.ahsl.lightness / float(USHRT_MAX);
}

/*!
    Returns the cyan color component of this color.

    \sa cyanF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::cyan() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().cyan();
    return qt_div_257(ct.acmyk.cyan);
}

/*!
    Returns the magenta color component of this color.

    \sa magentaF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::magenta() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().magenta();
    return qt_div_257(ct.acmyk.magenta);
}

/*!
    Returns the yellow color component of this color.

    \sa yellowF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::yellow() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().yellow();
    return qt_div_257(ct.acmyk.yellow);
}

/*!
    Returns the black color component of this color.

    \sa blackF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}

*/
int QColor::black() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().black();
    return qt_div_257(ct.acmyk.black);
}

/*!
    Returns the cyan color component of this color.

    \sa cyan(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
float QColor::cyanF() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().cyanF();
    return ct.acmyk.cyan / float(USHRT_MAX);
}

/*!
    Returns the magenta color component of this color.

    \sa magenta(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
float QColor::magentaF() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().magentaF();
    return ct.acmyk.magenta / float(USHRT_MAX);
}

/*!
    Returns the yellow color component of this color.

     \sa yellow(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
float QColor::yellowF() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().yellowF();
    return ct.acmyk.yellow / float(USHRT_MAX);
}

/*!
    Returns the black color component of this color.

    \sa black(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
float QColor::blackF() const noexcept
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().blackF();
    return ct.acmyk.black / float(USHRT_MAX);
}

/*!
    Create and returns an extended RGB QColor based on this color.
    \since 5.14

    \sa toRgb, convertTo()
*/
QColor QColor::toExtendedRgb() const noexcept
{
    if (!isValid() || cspec == ExtendedRgb)
        return *this;
    if (cspec != Rgb)
        return toRgb().toExtendedRgb();

    constexpr float f = 1.0f / USHRT_MAX;
    QColor color;
    color.cspec = ExtendedRgb;
    castF16(color.ct.argbExtended.alphaF16) = qfloat16(ct.argb.alpha * f);
    castF16(color.ct.argbExtended.redF16)   = qfloat16(ct.argb.red   * f);
    castF16(color.ct.argbExtended.greenF16) = qfloat16(ct.argb.green * f);
    castF16(color.ct.argbExtended.blueF16)  = qfloat16(ct.argb.blue  * f);
    color.ct.argbExtended.pad = 0;
    return color;
}

/*!
    Create and returns an RGB QColor based on this color.

    \sa fromRgb(), convertTo(), isValid()
*/
QColor QColor::toRgb() const noexcept
{
    if (!isValid() || cspec == Rgb)
        return *this;

    QColor color;
    color.cspec = Rgb;
    if (cspec != ExtendedRgb)
        color.ct.argb.alpha = ct.argb.alpha;
    color.ct.argb.pad = 0;

    switch (cspec) {
    case Hsv:
        {
            if (ct.ahsv.saturation == 0 || ct.ahsv.hue == USHRT_MAX) {
                // achromatic case
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = ct.ahsv.value;
                break;
            }

            // chromatic case
            const float h = ct.ahsv.hue == 36000 ? 0.0f : ct.ahsv.hue / 6000.0f;
            const float s = ct.ahsv.saturation / float(USHRT_MAX);
            const float v = ct.ahsv.value / float(USHRT_MAX);
            const int i = int(h);
            const float f = h - i;
            const float p = v * (1.0f - s);

            if (i & 1) {
                const float q = v * (1.0f - (s * f));

                switch (i) {
                case 1:
                    color.ct.argb.red   = qRound(q * USHRT_MAX);
                    color.ct.argb.green = qRound(v * USHRT_MAX);
                    color.ct.argb.blue  = qRound(p * USHRT_MAX);
                    break;
                case 3:
                    color.ct.argb.red   = qRound(p * USHRT_MAX);
                    color.ct.argb.green = qRound(q * USHRT_MAX);
                    color.ct.argb.blue  = qRound(v * USHRT_MAX);
                    break;
                case 5:
                    color.ct.argb.red   = qRound(v * USHRT_MAX);
                    color.ct.argb.green = qRound(p * USHRT_MAX);
                    color.ct.argb.blue  = qRound(q * USHRT_MAX);
                    break;
                }
            } else {
                const float t = v * (1.0f - (s * (1.0f - f)));

                switch (i) {
                case 0:
                    color.ct.argb.red   = qRound(v * USHRT_MAX);
                    color.ct.argb.green = qRound(t * USHRT_MAX);
                    color.ct.argb.blue  = qRound(p * USHRT_MAX);
                    break;
                case 2:
                    color.ct.argb.red   = qRound(p * USHRT_MAX);
                    color.ct.argb.green = qRound(v * USHRT_MAX);
                    color.ct.argb.blue  = qRound(t * USHRT_MAX);
                    break;
                case 4:
                    color.ct.argb.red   = qRound(t * USHRT_MAX);
                    color.ct.argb.green = qRound(p * USHRT_MAX);
                    color.ct.argb.blue  = qRound(v * USHRT_MAX);
                    break;
                }
            }
            break;
        }
    case Hsl:
        {
            if (ct.ahsl.saturation == 0 || ct.ahsl.hue == USHRT_MAX) {
                // achromatic case
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = ct.ahsl.lightness;
            } else if (ct.ahsl.lightness == 0) {
                // lightness 0
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = 0;
            } else {
                // chromatic case
                const float h = ct.ahsl.hue == 36000 ? 0.0f : ct.ahsl.hue / 36000.0f;
                const float s = ct.ahsl.saturation / float(USHRT_MAX);
                const float l = ct.ahsl.lightness / float(USHRT_MAX);

                float temp2;
                if (l < 0.5f)
                    temp2 = l * (1.0f + s);
                else
                    temp2 = l + s - (l * s);

                const float temp1 = (2.0f * l) - temp2;
                float temp3[3] = { h + (1.0f / 3.0f),
                                   h,
                                   h - (1.0f / 3.0f) };

                for (int i = 0; i != 3; ++i) {
                    if (temp3[i] < 0.0f)
                        temp3[i] += 1.0f;
                    else if (temp3[i] > 1.0f)
                        temp3[i] -= 1.0f;

                    const float sixtemp3 = temp3[i] * 6.0f;
                    if (sixtemp3 < 1.0f)
                        color.ct.array[i+1] = qRound((temp1 + (temp2 - temp1) * sixtemp3) * USHRT_MAX);
                    else if ((temp3[i] * 2.0f) < 1.0f)
                        color.ct.array[i+1] = qRound(temp2 * USHRT_MAX);
                    else if ((temp3[i] * 3.0f) < 2.0f)
                        color.ct.array[i+1] = qRound((temp1 + (temp2 -temp1) * (2.0f /3.0f - temp3[i]) * 6.0f) * USHRT_MAX);
                    else
                        color.ct.array[i+1] = qRound(temp1 * USHRT_MAX);
                }
                color.ct.argb.red = color.ct.argb.red == 1 ? 0 : color.ct.argb.red;
                color.ct.argb.green = color.ct.argb.green == 1 ? 0 : color.ct.argb.green;
                color.ct.argb.blue = color.ct.argb.blue == 1 ? 0 : color.ct.argb.blue;
            }
            break;
        }
    case Cmyk:
        {
            const float c = ct.acmyk.cyan / float(USHRT_MAX);
            const float m = ct.acmyk.magenta / float(USHRT_MAX);
            const float y = ct.acmyk.yellow / float(USHRT_MAX);
            const float k = ct.acmyk.black / float(USHRT_MAX);

            color.ct.argb.red   = qRound((1.0f - (c * (1.0f - k) + k)) * USHRT_MAX);
            color.ct.argb.green = qRound((1.0f - (m * (1.0f - k) + k)) * USHRT_MAX);
            color.ct.argb.blue  = qRound((1.0f - (y * (1.0f - k) + k)) * USHRT_MAX);
            break;
        }
    case ExtendedRgb:
        color.ct.argb.alpha = qRound(USHRT_MAX * float(castF16(ct.argbExtended.alphaF16)));
        color.ct.argb.red   = qRound(USHRT_MAX * qBound(0.0f, float(castF16(ct.argbExtended.redF16)),   1.0f));
        color.ct.argb.green = qRound(USHRT_MAX * qBound(0.0f, float(castF16(ct.argbExtended.greenF16)), 1.0f));
        color.ct.argb.blue  = qRound(USHRT_MAX * qBound(0.0f, float(castF16(ct.argbExtended.blueF16)),  1.0f));
        break;
    default:
        break;
    }

    return color;
}


#define Q_MAX_3(a, b, c) ( ( a > b && a > c) ? a : (b > c ? b : c) )
#define Q_MIN_3(a, b, c) ( ( a < b && a < c) ? a : (b < c ? b : c) )


/*!
    Creates and returns an HSV QColor based on this color.

    \sa fromHsv(), convertTo(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::toHsv() const noexcept
{
    if (!isValid() || cspec == Hsv)
        return *this;

    if (cspec != Rgb)
        return toRgb().toHsv();

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha = ct.argb.alpha;
    color.ct.ahsv.pad = 0;

    const float r = ct.argb.red   / float(USHRT_MAX);
    const float g = ct.argb.green / float(USHRT_MAX);
    const float b = ct.argb.blue  / float(USHRT_MAX);
    const float max = Q_MAX_3(r, g, b);
    const float min = Q_MIN_3(r, g, b);
    const float delta = max - min;
    color.ct.ahsv.value = qRound(max * USHRT_MAX);
    if (qFuzzyIsNull(delta)) {
        // achromatic case, hue is undefined
        color.ct.ahsv.hue = USHRT_MAX;
        color.ct.ahsv.saturation = 0;
    } else {
        // chromatic case
        float hue = 0;
        color.ct.ahsv.saturation = qRound((delta / max) * USHRT_MAX);
        if (qFuzzyCompare(r, max)) {
            hue = ((g - b) /delta);
        } else if (qFuzzyCompare(g, max)) {
            hue = (2.0f + (b - r) / delta);
        } else if (qFuzzyCompare(b, max)) {
            hue = (4.0f + (r - g) / delta);
        } else {
            Q_ASSERT_X(false, "QColor::toHsv", "internal error");
        }
        hue *= 60.0f;
        if (hue < 0.0f)
            hue += 360.0f;
        color.ct.ahsv.hue = qRound(hue * 100.0f);
    }

    return color;
}

/*!
    Creates and returns an HSL QColor based on this color.

    \sa fromHsl(), convertTo(), isValid(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
QColor QColor::toHsl() const noexcept
{
    if (!isValid() || cspec == Hsl)
        return *this;

    if (cspec != Rgb)
        return toRgb().toHsl();

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha = ct.argb.alpha;
    color.ct.ahsl.pad = 0;

    const float r = ct.argb.red   / float(USHRT_MAX);
    const float g = ct.argb.green / float(USHRT_MAX);
    const float b = ct.argb.blue  / float(USHRT_MAX);
    const float max = Q_MAX_3(r, g, b);
    const float min = Q_MIN_3(r, g, b);
    const float delta = max - min;
    const float delta2 = max + min;
    const float lightness = 0.5f * delta2;
    color.ct.ahsl.lightness = qRound(lightness * USHRT_MAX);
    if (qFuzzyIsNull(delta)) {
        // achromatic case, hue is undefined
        color.ct.ahsl.hue = USHRT_MAX;
        color.ct.ahsl.saturation = 0;
    } else {
        // chromatic case
        float hue = 0;
        if (lightness < 0.5f)
            color.ct.ahsl.saturation = qRound((delta / delta2) * USHRT_MAX);
        else
            color.ct.ahsl.saturation = qRound((delta / (2.0f - delta2)) * USHRT_MAX);
        if (qFuzzyCompare(r, max)) {
            hue = ((g - b) /delta);
        } else if (qFuzzyCompare(g, max)) {
            hue = (2.0f + (b - r) / delta);
        } else if (qFuzzyCompare(b, max)) {
            hue = (4.0f + (r - g) / delta);
        } else {
            Q_ASSERT_X(false, "QColor::toHsv", "internal error");
        }
        hue *= 60.0f;
        if (hue < 0.0f)
            hue += 360.0f;
        color.ct.ahsl.hue = qRound(hue * 100.0f);
    }

    return color;
}

/*!
    Creates and returns a CMYK QColor based on this color.

    \sa fromCmyk(), convertTo(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::toCmyk() const noexcept
{
    if (!isValid() || cspec == Cmyk)
        return *this;
    if (cspec != Rgb)
        return toRgb().toCmyk();

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha = ct.argb.alpha;

    if (!ct.argb.red && !ct.argb.green && !ct.argb.blue) {
        // Avoid div-by-0 below
        color.ct.acmyk.cyan    = 0;
        color.ct.acmyk.magenta = 0;
        color.ct.acmyk.yellow  = 0;
        color.ct.acmyk.black   = USHRT_MAX;
    } else {
        // rgb -> cmy
        const float r = ct.argb.red   / float(USHRT_MAX);
        const float g = ct.argb.green / float(USHRT_MAX);
        const float b = ct.argb.blue  / float(USHRT_MAX);
        float c = 1.0f - r;
        float m = 1.0f - g;
        float y = 1.0f - b;

        // cmy -> cmyk
        const float k = qMin(c, qMin(m, y));
        c = (c - k) / (1.0f - k);
        m = (m - k) / (1.0f - k);
        y = (y - k) / (1.0f - k);

        color.ct.acmyk.cyan    = qRound(c * USHRT_MAX);
        color.ct.acmyk.magenta = qRound(m * USHRT_MAX);
        color.ct.acmyk.yellow  = qRound(y * USHRT_MAX);
        color.ct.acmyk.black   = qRound(k * USHRT_MAX);
    }

    return color;
}

QColor QColor::convertTo(QColor::Spec colorSpec) const noexcept
{
    if (colorSpec == cspec)
        return *this;
    switch (colorSpec) {
    case Rgb:
        return toRgb();
    case ExtendedRgb:
        return toExtendedRgb();
    case Hsv:
        return toHsv();
    case Cmyk:
        return toCmyk();
    case Hsl:
        return toHsl();
    case Invalid:
        break;
    }
    return QColor(); // must be invalid
}


/*!
    Static convenience function that returns a QColor constructed from the
    given QRgb value \a rgb.

    The alpha component of \a rgb is ignored (i.e. it is automatically set to
    255), use the fromRgba() function to include the alpha-channel specified by
    the given QRgb value.

    \sa fromRgba(), fromRgbF(), toRgb(), isValid()
*/

QColor QColor::fromRgb(QRgb rgb) noexcept
{
    return fromRgb(qRed(rgb), qGreen(rgb), qBlue(rgb));
}


/*!
    Static convenience function that returns a QColor constructed from the
    given QRgb value \a rgba.

    Unlike the fromRgb() function, the alpha-channel specified by the given
    QRgb value is included.

    \sa fromRgb(), fromRgba64(), isValid()
*/

QColor QColor::fromRgba(QRgb rgba) noexcept
{
    return fromRgb(qRed(rgba), qGreen(rgba), qBlue(rgba), qAlpha(rgba));
}

/*!
    Static convenience function that returns a QColor constructed from the RGB
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa toRgb(), fromRgba64(), fromRgbF(), isValid()
*/
QColor QColor::fromRgb(int r, int g, int b, int a)
{
    if (!isRgbaValid(r, g, b, a)) {
        qWarning("QColor::fromRgb: RGB parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Rgb;
    color.ct.argb.alpha = a * 0x101;
    color.ct.argb.red   = r * 0x101;
    color.ct.argb.green = g * 0x101;
    color.ct.argb.blue  = b * 0x101;
    color.ct.argb.pad   = 0;
    return color;
}

/*!
    Static convenience function that returns a QColor constructed from the RGB
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    The alpha value must be in the range 0.0-1.0.
    If any of the other values are outside the range of 0.0-1.0 the
    color model will be set as \c ExtendedRgb.

    \sa fromRgb(), fromRgba64(), toRgb(), isValid()
*/
QColor QColor::fromRgbF(float r, float g, float b, float a)
{
    if (a < 0.0f || a > 1.0f) {
        qWarning("QColor::fromRgbF: Alpha parameter out of range");
        return QColor();
    }

    if (r < 0.0f || r > 1.0f
            || g < 0.0f || g > 1.0f
            || b < 0.0f || b > 1.0f) {
        QColor color;
        color.cspec = ExtendedRgb;
        castF16(color.ct.argbExtended.alphaF16) = qfloat16(a);
        castF16(color.ct.argbExtended.redF16)   = qfloat16(r);
        castF16(color.ct.argbExtended.greenF16) = qfloat16(g);
        castF16(color.ct.argbExtended.blueF16)  = qfloat16(b);
        color.ct.argbExtended.pad   = 0;
        return color;
    }

    QColor color;
    color.cspec = Rgb;
    color.ct.argb.alpha = qRound(a * USHRT_MAX);
    color.ct.argb.red   = qRound(r * USHRT_MAX);
    color.ct.argb.green = qRound(g * USHRT_MAX);
    color.ct.argb.blue  = qRound(b * USHRT_MAX);
    color.ct.argb.pad   = 0;
    return color;
}


/*!
    \since 5.6

    Static convenience function that returns a QColor constructed from the RGBA64
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    \sa fromRgb(), fromRgbF(), toRgb(), isValid()
*/
QColor QColor::fromRgba64(ushort r, ushort g, ushort b, ushort a) noexcept
{
    QColor color;
    color.setRgba64(qRgba64(r, g, b, a));
    return color;
}

/*!
    \since 5.6

    Static convenience function that returns a QColor constructed from the
    given QRgba64 value \a rgba64.

    \sa fromRgb(), fromRgbF(), toRgb(), isValid()
*/
QColor QColor::fromRgba64(QRgba64 rgba64) noexcept
{
    QColor color;
    color.setRgba64(rgba64);
    return color;
}

/*!
    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a v (value), and \a a
    (alpha-channel, i.e. transparency).

    The value of \a s, \a v, and \a a must all be in the range 0-255; the value
    of \a h must be in the range 0-359.

    \sa toHsv(), fromHsvF(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::fromHsv(int h, int s, int v, int a)
{
    if (((h < 0 || h >= 360) && h != -1)
        || s < 0 || s > 255
        || v < 0 || v > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromHsv: HSV parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha      = a * 0x101;
    color.ct.ahsv.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    color.ct.ahsv.saturation = s * 0x101;
    color.ct.ahsv.value      = v * 0x101;
    color.ct.ahsv.pad        = 0;
    return color;
}

/*!
    \overload

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a v (value), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toHsv(), fromHsv(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::fromHsvF(float h, float s, float v, float a)
{
    if (((h < 0.0f || h > 1.0f) && h != -1.0f)
        || (s < 0.0f || s > 1.0f)
        || (v < 0.0f || v > 1.0f)
        || (a < 0.0f || a > 1.0f)) {
        qWarning("QColor::fromHsvF: HSV parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha      = qRound(a * USHRT_MAX);
    color.ct.ahsv.hue        = h == -1.0f ? USHRT_MAX : qRound(h * 36000.0f);
    color.ct.ahsv.saturation = qRound(s * USHRT_MAX);
    color.ct.ahsv.value      = qRound(v * USHRT_MAX);
    color.ct.ahsv.pad        = 0;
    return color;
}

/*!
    \since 4.6

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a l (lightness), and \a a
    (alpha-channel, i.e. transparency).

    The value of \a s, \a l, and \a a must all be in the range 0-255; the value
    of \a h must be in the range 0-359.

    \sa toHsl(), fromHslF(), isValid(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
QColor QColor::fromHsl(int h, int s, int l, int a)
{
    if (((h < 0 || h >= 360) && h != -1)
        || s < 0 || s > 255
        || l < 0 || l > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromHsl: HSL parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha      = a * 0x101;
    color.ct.ahsl.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    color.ct.ahsl.saturation = s * 0x101;
    color.ct.ahsl.lightness  = l * 0x101;
    color.ct.ahsl.pad        = 0;
    return color;
}

/*!
    \overload
    \since 4.6

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a l (lightness), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toHsl(), fromHsl(), isValid(), {QColor#The HSL Color Model}{The HSL Color Model}
*/
QColor QColor::fromHslF(float h, float s, float l, float a)
{
    if (((h < 0.0f || h > 1.0f) && h != -1.0f)
        || (s < 0.0f || s > 1.0f)
        || (l < 0.0f || l > 1.0f)
        || (a < 0.0f || a > 1.0f)) {
        qWarning("QColor::fromHslF: HSL parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha      = qRound(a * USHRT_MAX);
    color.ct.ahsl.hue        = (h == -1.0f) ? USHRT_MAX : qRound(h * 36000.0f);
    if (color.ct.ahsl.hue == 36000)
        color.ct.ahsl.hue = 0;
    color.ct.ahsl.saturation = qRound(s * USHRT_MAX);
    color.ct.ahsl.lightness  = qRound(l * USHRT_MAX);
    color.ct.ahsl.pad        = 0;
    return color;
}

/*!
    Sets the contents pointed to by \a c, \a m, \a y, \a k, and \a a, to the
    cyan, magenta, yellow, black, and alpha-channel (transparency) components
    of the color's CMYK value.

    These components can be retrieved individually using the cyan(), magenta(),
    yellow(), black() and alpha() functions.

    \sa setCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::getCmyk(int *c, int *m, int *y, int *k, int *a) const
{
    if (!c || !m || !y || !k)
        return;

    if (cspec != Invalid && cspec != Cmyk) {
        toCmyk().getCmyk(c, m, y, k, a);
        return;
    }

    *c = qt_div_257(ct.acmyk.cyan);
    *m = qt_div_257(ct.acmyk.magenta);
    *y = qt_div_257(ct.acmyk.yellow);
    *k = qt_div_257(ct.acmyk.black);

    if (a)
        *a = qt_div_257(ct.acmyk.alpha);
}

/*!
    Sets the contents pointed to by \a c, \a m, \a y, \a k, and \a a, to the
    cyan, magenta, yellow, black, and alpha-channel (transparency) components
    of the color's CMYK value.

    These components can be retrieved individually using the cyanF(),
    magentaF(), yellowF(), blackF() and alphaF() functions.

    \sa setCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::getCmykF(float *c, float *m, float *y, float *k, float *a) const
{
    if (!c || !m || !y || !k)
        return;

    if (cspec != Invalid && cspec != Cmyk) {
        toCmyk().getCmykF(c, m, y, k, a);
        return;
    }

    *c = ct.acmyk.cyan    / float(USHRT_MAX);
    *m = ct.acmyk.magenta / float(USHRT_MAX);
    *y = ct.acmyk.yellow  / float(USHRT_MAX);
    *k = ct.acmyk.black   / float(USHRT_MAX);

    if (a)
        *a = ct.acmyk.alpha / float(USHRT_MAX);
}

/*!
    Sets the color to CMYK values, \a c (cyan), \a m (magenta), \a y (yellow),
    \a k (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa getCmyk(), setCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::setCmyk(int c, int m, int y, int k, int a)
{
    if (c < 0 || c > 255
        || m < 0 || m > 255
        || y < 0 || y > 255
        || k < 0 || k > 255
        || a < 0 || a > 255) {
        qWarning("QColor::setCmyk: CMYK parameters out of range");
        invalidate();
        return;
    }

    cspec = Cmyk;
    ct.acmyk.alpha   = a * 0x101;
    ct.acmyk.cyan    = c * 0x101;
    ct.acmyk.magenta = m * 0x101;
    ct.acmyk.yellow  = y * 0x101;
    ct.acmyk.black   = k * 0x101;
}

/*!
    \overload

    Sets the color to CMYK values, \a c (cyan), \a m (magenta), \a y (yellow),
    \a k (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa getCmykF(), setCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::setCmykF(float c, float m, float y, float k, float a)
{
    if (c < 0.0f || c > 1.0f
        || m < 0.0f || m > 1.0f
        || y < 0.0f || y > 1.0f
        || k < 0.0f || k > 1.0f
        || a < 0.0f || a > 1.0f) {
        qWarning("QColor::setCmykF: CMYK parameters out of range");
        invalidate();
        return;
    }

    cspec = Cmyk;
    ct.acmyk.alpha   = qRound(a * USHRT_MAX);
    ct.acmyk.cyan    = qRound(c * USHRT_MAX);
    ct.acmyk.magenta = qRound(m * USHRT_MAX);
    ct.acmyk.yellow  = qRound(y * USHRT_MAX);
    ct.acmyk.black   = qRound(k * USHRT_MAX);
}

/*!
    Static convenience function that returns a QColor constructed from the
    given CMYK color values: \a c (cyan), \a m (magenta), \a y (yellow), \a k
    (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa toCmyk(), fromCmykF(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::fromCmyk(int c, int m, int y, int k, int a)
{
    if (c < 0 || c > 255
        || m < 0 || m > 255
        || y < 0 || y > 255
        || k < 0 || k > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromCmyk: CMYK parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha   = a * 0x101;
    color.ct.acmyk.cyan    = c * 0x101;
    color.ct.acmyk.magenta = m * 0x101;
    color.ct.acmyk.yellow  = y * 0x101;
    color.ct.acmyk.black   = k * 0x101;
    return color;
}

/*!
    \overload

    Static convenience function that returns a QColor constructed from the
    given CMYK color values: \a c (cyan), \a m (magenta), \a y (yellow), \a k
    (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toCmyk(), fromCmyk(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::fromCmykF(float c, float m, float y, float k, float a)
{
    if (c < 0.0f || c > 1.0f
        || m < 0.0f || m > 1.0f
        || y < 0.0f || y > 1.0f
        || k < 0.0f || k > 1.0f
        || a < 0.0f || a > 1.0f) {
        qWarning("QColor::fromCmykF: CMYK parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha   = qRound(a * USHRT_MAX);
    color.ct.acmyk.cyan    = qRound(c * USHRT_MAX);
    color.ct.acmyk.magenta = qRound(m * USHRT_MAX);
    color.ct.acmyk.yellow  = qRound(y * USHRT_MAX);
    color.ct.acmyk.black   = qRound(k * USHRT_MAX);
    return color;
}

/*!
    \fn QColor QColor::lighter(int factor) const
    \since 4.3

    Returns a lighter (or darker) color, but does not change this object.

    If the \a factor is greater than 100, this functions returns a lighter
    color. Setting \a factor to 150 returns a color that is 50% brighter. If
    the \a factor is less than 100, the return color is darker, but we
    recommend using the darker() function for this purpose. If the \a factor
    is 0 or negative, the return value is unspecified.

    The function converts the current color to HSV, multiplies the value
    (V) component by \a factor and converts the color back to it's original
    color spec.

    \sa darker(), isValid()
*/
QColor QColor::lighter(int factor) const noexcept
{
    if (factor <= 0)                                // invalid lightness factor
        return *this;
    else if (factor < 100)                        // makes color darker
        return darker(10000 / factor);

    QColor hsv = toHsv();
    int s = hsv.ct.ahsv.saturation;
    uint v = hsv.ct.ahsv.value;

    v = (factor*v)/100;
    if (v > USHRT_MAX) {
        // overflow... adjust saturation
        s -= v - USHRT_MAX;
        if (s < 0)
            s = 0;
        v = USHRT_MAX;
    }

    hsv.ct.ahsv.saturation = s;
    hsv.ct.ahsv.value = v;

    // convert back to same color spec as original color
    return hsv.convertTo(cspec);
}

/*!
    \fn QColor QColor::darker(int factor) const
    \since 4.3

    Returns a darker (or lighter) color, but does not change this object.

    If the \a factor is greater than 100, this functions returns a darker
    color. Setting \a factor to 300 returns a color that has one-third the
    brightness. If the \a factor is less than 100, the return color is lighter,
    but we recommend using the lighter() function for this purpose. If the
    \a factor is 0 or negative, the return value is unspecified.

    The function converts the current color to HSV, divides the value (V)
    component by \a factor and converts the color back to it's original
    color spec.

    \sa lighter(), isValid()
*/
QColor QColor::darker(int factor) const noexcept
{
    if (factor <= 0)                                // invalid darkness factor
        return *this;
    else if (factor < 100)                        // makes color lighter
        return lighter(10000 / factor);

    QColor hsv = toHsv();
    hsv.ct.ahsv.value = (hsv.ct.ahsv.value * 100) / factor;

    // convert back to same color spec as original color
    return hsv.convertTo(cspec);
}

/*! \overload
    Assigns a copy of \a color and returns a reference to this color.
 */
QColor &QColor::operator=(Qt::GlobalColor color) noexcept
{
    return operator=(QColor(color));
}

/*!
    Returns \c true if this color has the same color specification and component values as \a color;
    otherwise returns \c false.

    ExtendedRgb and Rgb specifications are considered matching in this context.

    \sa spec()
*/
bool QColor::operator==(const QColor &color) const noexcept
{
    if ((cspec == ExtendedRgb || color.cspec == ExtendedRgb) &&
               (cspec == color.cspec || cspec == Rgb || color.cspec == Rgb))  {
        return qFuzzyCompare(alphaF(), color.alphaF())
            && qFuzzyCompare(redF(), color.redF())
            && qFuzzyCompare(greenF(), color.greenF())
            && qFuzzyCompare(blueF(), color.blueF());
    } else {
        return (cspec == color.cspec
                && ct.argb.alpha == color.ct.argb.alpha
                && (((cspec == QColor::Hsv || cspec == QColor::Hsl)
                     && ((ct.ahsv.hue % 36000) == (color.ct.ahsv.hue % 36000)))
                    || (ct.argb.red == color.ct.argb.red))
                && ct.argb.green == color.ct.argb.green
                && ct.argb.blue  == color.ct.argb.blue
                && ct.argb.pad   == color.ct.argb.pad);
    }
}

/*!
    Returns \c true if this color has different color specification or component values from
    \a color; otherwise returns \c false.

    ExtendedRgb and Rgb specifications are considered matching in this context.

    \sa spec()
*/
bool QColor::operator!=(const QColor &color) const noexcept
{ return !operator==(color); }


/*!
    Returns the color as a QVariant
*/
QColor::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*! \internal

    Marks the color as invalid and sets all components to zero (alpha is set
    to fully opaque for compatibility with Qt 3).
*/
void QColor::invalidate() noexcept
{
    cspec = Invalid;
    ct.argb.alpha = USHRT_MAX;
    ct.argb.red = 0;
    ct.argb.green = 0;
    ct.argb.blue = 0;
    ct.argb.pad = 0;
}

/*****************************************************************************
  QColor stream functions
 *****************************************************************************/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QColor &c)
{
    QDebugStateSaver saver(dbg);
    if (!c.isValid())
        dbg.nospace() << "QColor(Invalid)";
    else if (c.spec() == QColor::Rgb)
        dbg.nospace() << "QColor(ARGB " << c.alphaF() << ", " << c.redF() << ", " << c.greenF() << ", " << c.blueF() << ')';
    else if (c.spec() == QColor::ExtendedRgb)
        dbg.nospace() << "QColor(Ext. ARGB " << c.alphaF() << ", " << c.redF() << ", " << c.greenF() << ", " << c.blueF() << ')';
    else if (c.spec() == QColor::Hsv)
        dbg.nospace() << "QColor(AHSV " << c.alphaF() << ", " << c.hueF() << ", " << c.saturationF() << ", " << c.valueF() << ')';
    else if (c.spec() == QColor::Cmyk)
        dbg.nospace() << "QColor(ACMYK " << c.alphaF() << ", " << c.cyanF() << ", " << c.magentaF() << ", " << c.yellowF() << ", "
                      << c.blackF()<< ')';
    else if (c.spec() == QColor::Hsl)
        dbg.nospace() << "QColor(AHSL " << c.alphaF() << ", " << c.hslHueF() << ", " << c.hslSaturationF() << ", " << c.lightnessF() << ')';

    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QColor &color)
    \relates QColor

    Writes the \a color to the \a stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &stream, const QColor &color)
{
    if (stream.version() < 7) {
        if (!color.isValid())
            return stream << quint32(0x49000000);
        quint32 p = (quint32)color.rgb();
        if (stream.version() == 1) // Swap red and blue
            p = ((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
        return stream << p;
    }

    qint8   s = color.cspec;
    quint16 a = color.ct.argb.alpha;
    quint16 r = color.ct.argb.red;
    quint16 g = color.ct.argb.green;
    quint16 b = color.ct.argb.blue;
    quint16 p = color.ct.argb.pad;

    stream << s;
    stream << a;
    stream << r;
    stream << g;
    stream << b;
    stream << p;

    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QColor &color)
    \relates QColor

    Reads the \a color from the \a stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &stream, QColor &color)
{
    if (stream.version() < 7) {
        quint32 p;
        stream >> p;
        if (p == 0x49000000) {
            color.invalidate();
            return stream;
        }
        if (stream.version() == 1) // Swap red and blue
            p = ((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
        color.setRgb(p);
        return stream;
    }

    qint8 s;
    quint16 a, r, g, b, p;
    stream >> s;
    stream >> a;
    stream >> r;
    stream >> g;
    stream >> b;
    stream >> p;

    color.cspec = QColor::Spec(s);
    color.ct.argb.alpha = a;
    color.ct.argb.red   = r;
    color.ct.argb.green = g;
    color.ct.argb.blue  = b;
    color.ct.argb.pad   = p;

    return stream;
}
#endif // QT_NO_DATASTREAM

// A table of precalculated results of 0x00ff00ff/alpha use by qUnpremultiply:
const uint qt_inv_premul_factor[256] = {
    0, 16711935, 8355967, 5570645, 4177983, 3342387, 2785322, 2387419,
    2088991, 1856881, 1671193, 1519266, 1392661, 1285533, 1193709, 1114129,
    1044495, 983055, 928440, 879575, 835596, 795806, 759633, 726605,
    696330, 668477, 642766, 618960, 596854, 576273, 557064, 539094,
    522247, 506422, 491527, 477483, 464220, 451673, 439787, 428511,
    417798, 407608, 397903, 388649, 379816, 371376, 363302, 355573,
    348165, 341059, 334238, 327685, 321383, 315319, 309480, 303853,
    298427, 293191, 288136, 283253, 278532, 273966, 269547, 265268,
    261123, 257106, 253211, 249431, 245763, 242201, 238741, 235379,
    232110, 228930, 225836, 222825, 219893, 217038, 214255, 211543,
    208899, 206320, 203804, 201348, 198951, 196611, 194324, 192091,
    189908, 187774, 185688, 183647, 181651, 179698, 177786, 175915,
    174082, 172287, 170529, 168807, 167119, 165464, 163842, 162251,
    160691, 159161, 157659, 156186, 154740, 153320, 151926, 150557,
    149213, 147893, 146595, 145321, 144068, 142837, 141626, 140436,
    139266, 138115, 136983, 135869, 134773, 133695, 132634, 131590,
    130561, 129549, 128553, 127572, 126605, 125653, 124715, 123792,
    122881, 121984, 121100, 120229, 119370, 118524, 117689, 116866,
    116055, 115254, 114465, 113686, 112918, 112160, 111412, 110675,
    109946, 109228, 108519, 107818, 107127, 106445, 105771, 105106,
    104449, 103800, 103160, 102527, 101902, 101284, 100674, 100071,
    99475, 98887, 98305, 97730, 97162, 96600, 96045, 95496,
    94954, 94417, 93887, 93362, 92844, 92331, 91823, 91322,
    90825, 90334, 89849, 89368, 88893, 88422, 87957, 87497,
    87041, 86590, 86143, 85702, 85264, 84832, 84403, 83979,
    83559, 83143, 82732, 82324, 81921, 81521, 81125, 80733,
    80345, 79961, 79580, 79203, 78829, 78459, 78093, 77729,
    77370, 77013, 76660, 76310, 75963, 75619, 75278, 74941,
    74606, 74275, 73946, 73620, 73297, 72977, 72660, 72346,
    72034, 71725, 71418, 71114, 70813, 70514, 70218, 69924,
    69633, 69344, 69057, 68773, 68491, 68211, 67934, 67659,
    67386, 67116, 66847, 66581, 66317, 66055, 65795, 65537
};

/*****************************************************************************
  QColor global functions (documentation only)
 *****************************************************************************/

/*!
    \fn int qRed(QRgb rgb)
    \relates QColor

    Returns the red component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::red()
*/

/*!
    \fn int qGreen(QRgb rgb)
    \relates QColor

    Returns the green component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::green()
*/

/*!
    \fn int qBlue(QRgb rgb)
    \relates QColor

    Returns the blue component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::blue()
*/

/*!
    \fn int qAlpha(QRgb rgba)
    \relates QColor

    Returns the alpha component of the ARGB quadruplet \a rgba.

    \sa qRgb(), QColor::alpha()
*/

/*!
    \fn QRgb qRgb(int r, int g, int b)
    \relates QColor

    Returns the ARGB quadruplet (255, \a{r}, \a{g}, \a{b}).

    \sa qRgba(), qRed(), qGreen(), qBlue(), qAlpha()
*/

/*!
    \fn QRgb qRgba(int r, int g, int b, int a)
    \relates QColor

    Returns the ARGB quadruplet (\a{a}, \a{r}, \a{g}, \a{b}).

    \sa qRgb(), qRed(), qGreen(), qBlue(), qAlpha()
*/

/*!
    \fn int qGray(int r, int g, int b)
    \relates QColor

    Returns a gray value (0 to 255) from the (\a r, \a g, \a b)
    triplet.

    The gray value is calculated using the formula (\a r * 11 + \a g * 16 +
    \a b * 5)/32.
*/

/*!
    \fn int qGray(QRgb rgb)
    \overload
    \relates QColor

    Returns a gray value (0 to 255) from the given ARGB quadruplet \a rgb.

    The gray value is calculated using the formula (R * 11 + G * 16 + B * 5)/32;
    the alpha-channel is ignored.
*/

/*!
    \fn QRgb qPremultiply(QRgb rgb)
    \since 5.3
    \relates QColor

    Converts an unpremultiplied ARGB quadruplet \a rgb into a premultiplied ARGB quadruplet.

    \sa qUnpremultiply()
*/

/*!
    \fn QRgb qUnpremultiply(QRgb rgb)
    \since 5.3
    \relates QColor

    Converts a premultiplied ARGB quadruplet \a rgb into an unpremultiplied ARGB quadruplet.

    \sa qPremultiply()
*/

/*!
    \fn QColor QColor::convertTo(Spec colorSpec) const

    Creates a copy of \e this color in the format specified by \a colorSpec.

    \sa spec(), toCmyk(), toHsv(), toRgb(), isValid()
*/

/*!
    \typedef QRgb
    \relates QColor

    An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.

    The type also holds a value for the alpha-channel. The default alpha
    channel is \c ff, i.e opaque. For more information, see the
    \l{QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing} section.

    Here are some examples of how QRgb values can be created:

    \snippet code/src_gui_painting_qcolor.cpp QRgb

    \sa qRgb(), qRgba(), QColor::rgb(), QColor::rgba()
*/

/*!
    \namespace QColorConstants
    \inmodule QtGui
    \since 5.14

    \brief The QColorConstants namespace contains QColor predefined constants.

    These constants are usable everywhere a QColor object is expected:

    \code
    painter.setBrush(QColorConstants::Svg::lightblue);
    \endcode

    Their usage is much cheaper than e.g. passing a string to QColor's constructor,
    as they don't require any parsing of the string, and always result in a valid
    QColor object:

    \badcode
    object.setColor(QColor("lightblue")); // expensive
    \endcode

    \section1 Qt Colors

    The following colors are defined in the \c{QColorConstants} namespace:

    \include qt-colors.qdocinc

    \section1 SVG Colors

    The following table lists the available
    \l {http://www.w3.org/TR/SVG/types.html#ColorKeywords}{SVG colors}.
    They are available in the \c{QColorConstants::Svg} inner namespace.

    \include svg-colors.qdocinc

    \sa QColor, Qt::GlobalColor
*/

QT_END_NAMESPACE
