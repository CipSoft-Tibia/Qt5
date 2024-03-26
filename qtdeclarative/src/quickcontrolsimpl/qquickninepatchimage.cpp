// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickninepatchimage_p.h"

#include <QtCore/qfileinfo.h>
#include <QtQuick/qsggeometry.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/private/qsgnode_p.h>
#include <QtQuick/private/qquickimage_p_p.h>

QT_BEGIN_NAMESPACE

struct QQuickNinePatchData
{
    QList<qreal> coordsForSize(qreal count) const;

    inline bool isNull() const { return data.isEmpty(); }
    inline int count() const { return data.size(); }
    inline qreal at(int index) const { return data.at(index); }
    inline qreal size() const { return data.last(); }

    void fill(const QList<qreal> &coords, qreal count);
    void clear();

private:
    bool inverted = false;
    QList<qreal> data;
};

QList<qreal> QQuickNinePatchData::coordsForSize(qreal size) const
{
    // n = number of stretchable sections
    // We have to compensate when adding 0 and/or
    // the source image width to the divs vector.
    const int l = data.size();
    const int n = (inverted ? l - 1 : l) / 2;
    const qreal stretch = (size - data.last()) / n;

    QList<qreal> coords;
    coords.reserve(l);
    coords.append(0);

    bool stretched = !inverted;
    for (int i = 1; i < l; ++i) {
        qreal advance = data[i] - data[i - 1];
        if (stretched)
            advance += stretch;
        coords.append(coords.last() + advance);

        stretched = !stretched;
    }

    return coords;
}

/*
    Adds the 0 index coordinate if appropriate, and the one at "size".
*/
void QQuickNinePatchData::fill(const QList<qreal> &coords, qreal size)
{
    data.clear();
    inverted = coords.isEmpty() || coords.first() != 0;

    // Reserve an extra item in case we need to add the image width/height
    if (inverted) {
        data.reserve(coords.size() + 2);
        data.append(0);
    } else {
        data.reserve(coords.size() + 1);
    }

    data += coords;
    data.append(size);
}

void QQuickNinePatchData::clear()
{
    data.clear();
}

class QQuickNinePatchNode : public QSGGeometryNode
{
public:
    QQuickNinePatchNode();
    ~QQuickNinePatchNode();

    void initialize(QSGTexture *texture, const QSizeF &targetSize, const QSize &sourceSize,
                    const QQuickNinePatchData &xDivs, const QQuickNinePatchData &yDivs, qreal dpr);

private:
    QSGGeometry m_geometry;
    QSGTextureMaterial m_material;
};

QQuickNinePatchNode::QQuickNinePatchNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
    setGeometry(&m_geometry);
    setMaterial(&m_material);
}

QQuickNinePatchNode::~QQuickNinePatchNode()
{
    delete m_material.texture();
}

void QQuickNinePatchNode::initialize(QSGTexture *texture, const QSizeF &targetSize, const QSize &sourceSize,
                                     const QQuickNinePatchData &xDivs, const QQuickNinePatchData &yDivs, qreal dpr)
{
    delete m_material.texture();
    m_material.setTexture(texture);

    const int xlen = xDivs.count();
    const int ylen = yDivs.count();

    if (xlen > 0 && ylen > 0) {
        const int quads = (xlen - 1) * (ylen - 1);
        static const int verticesPerQuad = 6;
        m_geometry.allocate(xlen * ylen, verticesPerQuad * quads);

        QSGGeometry::TexturedPoint2D *vertices = m_geometry.vertexDataAsTexturedPoint2D();
        QList<qreal> xCoords = xDivs.coordsForSize(targetSize.width());
        QList<qreal> yCoords = yDivs.coordsForSize(targetSize.height());

        for (int y = 0; y < ylen; ++y) {
            for (int x = 0; x < xlen; ++x, ++vertices)
                vertices->set(xCoords[x] / dpr, yCoords[y] / dpr,
                              xDivs.at(x) / sourceSize.width(),
                              yDivs.at(y) / sourceSize.height());
        }

        quint16 *indices = m_geometry.indexDataAsUShort();
        int n = quads;
        for (int q = 0; n--; ++q) {
            if ((q + 1) % xlen == 0) // next row
                ++q;
            // Bottom-left half quad triangle
            indices[0] = q;
            indices[1] = q + xlen;
            indices[2] = q + xlen + 1;

            // Top-right half quad triangle
            indices[3] = q;
            indices[4] = q + xlen + 1;
            indices[5] = q + 1;

            indices += verticesPerQuad;
        }
    }

    markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}

class QQuickNinePatchImagePrivate : public QQuickImagePrivate
{
    Q_DECLARE_PUBLIC(QQuickNinePatchImage)

public:
    void updatePatches();
    void updatePaddings(const QSizeF &size, const QList<qreal> &horizontal, const QList<qreal> &vertical);
    void updateInsets(const QList<qreal> &horizontal, const QList<qreal> &vertical);

    bool resetNode = false;
    qreal topPadding = 0;
    qreal leftPadding = 0;
    qreal rightPadding = 0;
    qreal bottomPadding = 0;
    qreal topInset = 0;
    qreal leftInset = 0;
    qreal rightInset = 0;
    qreal bottomInset = 0;

    QImage ninePatch;
    QQuickNinePatchData xDivs;
    QQuickNinePatchData yDivs;
};

/*
    Examines each pixel in a horizontal or vertical (if offset is equal to the image's width)
    line, storing the start and end index ("coordinate") of each 9-patch line.

    For instance, in the 7x3 (9x5 actual size) 9-patch image below, which has no horizontal
    stretchable area, it would return {}:

     +-----+
     |     |
     +-----+

    If indices 3 to 5 were marked, it would return {2, 5}:

       xxx
     +-----+
     |     |
     +-----+

    If indices 3 and 5 were marked, it would store {0, 2, 3, 4, 5, 7}:

       x x
     +-----+
     |     |
     +-----+
*/
static QList<qreal> readCoords(const QRgb *data, int from, int count, int offset, QRgb color)
{
    int p1 = -1;
    QList<qreal> coords;
    for (int i = 0; i < count; ++i) {
        int p2 = from + i * offset;
        if (data[p2] == color) {
            // colored pixel
            if (p1 == -1) {
                // This is the start of a 9-patch line.
                p1 = i;
            }
        } else {
            // empty pixel
            if (p1 != -1) {
                // This is the end of a 9-patch line; add the start and end indices as coordinates...
                coords << p1 << i;
                // ... and reset p1 so that we can search for the next one.
                p1 = -1;
            }
        }
    }
    return coords;
}

/*
    Called whenever a 9-patch image is set as the image's source.

    Reads the 9-patch lines from the source image and sets the
    inset and padding properties accordingly.
*/
void QQuickNinePatchImagePrivate::updatePatches()
{
    if (ninePatch.isNull())
        return;

    int w = ninePatch.width();
    int h = ninePatch.height();
    const QRgb *data = reinterpret_cast<const QRgb *>(ninePatch.constBits());

    const QRgb black = qRgb(0,0,0);
    const QRgb red = qRgb(255,0,0);

    xDivs.fill(readCoords(data, 1, w - 1, 1, black), w - 2); // top left -> top right
    yDivs.fill(readCoords(data, w, h - 1, w, black), h - 2); // top left -> bottom left

    QList<qreal> hInsets = readCoords(data, (h - 1) * w + 1, w - 1, 1, red); // bottom left -> bottom right
    QList<qreal> vInsets = readCoords(data, 2 * w - 1, h - 1, w, red); // top right -> bottom right
    updateInsets(hInsets, vInsets);

    const QSizeF sz(w - leftInset - rightInset, h - topInset - bottomInset);
    QList<qreal> hPaddings = readCoords(data, (h - 1) * w + leftInset + 1, sz.width() - 2, 1, black); // bottom left -> bottom right
    QList<qreal> vPaddings = readCoords(data, (2 + topInset) * w - 1, sz.height() - 2, w, black); // top right -> bottom right
    updatePaddings(sz, hPaddings, vPaddings);
}

void QQuickNinePatchImagePrivate::updatePaddings(const QSizeF &size, const QList<qreal> &horizontal, const QList<qreal> &vertical)
{
    Q_Q(QQuickNinePatchImage);
    qreal oldTopPadding = topPadding;
    qreal oldLeftPadding = leftPadding;
    qreal oldRightPadding = rightPadding;
    qreal oldBottomPadding = bottomPadding;

    if (horizontal.size() >= 2) {
        leftPadding = horizontal.first();
        rightPadding = size.width() - horizontal.last() - 2;
    } else {
        leftPadding = 0;
        rightPadding = 0;
    }

    if (vertical.size() >= 2) {
        topPadding = vertical.first();
        bottomPadding = size.height() - vertical.last() - 2;
    } else {
        topPadding = 0;
        bottomPadding = 0;
    }

    if (!qFuzzyCompare(oldTopPadding, topPadding))
        emit q->topPaddingChanged();
    if (!qFuzzyCompare(oldBottomPadding, bottomPadding))
        emit q->bottomPaddingChanged();
    if (!qFuzzyCompare(oldLeftPadding, leftPadding))
        emit q->leftPaddingChanged();
    if (!qFuzzyCompare(oldRightPadding, rightPadding))
        emit q->rightPaddingChanged();
}

void QQuickNinePatchImagePrivate::updateInsets(const QList<qreal> &horizontal, const QList<qreal> &vertical)
{
    Q_Q(QQuickNinePatchImage);
    qreal oldTopInset = topInset;
    qreal oldLeftInset = leftInset;
    qreal oldRightInset = rightInset;
    qreal oldBottomInset = bottomInset;

    if (horizontal.size() >= 2 && horizontal.first() == 0)
        leftInset = horizontal.at(1);
    else
        leftInset = 0;

    if (horizontal.size() == 2 && horizontal.first() > 0)
        rightInset = horizontal.last() - horizontal.first();
    else if (horizontal.size() == 4)
        rightInset = horizontal.last() - horizontal.at(2);
    else
        rightInset = 0;

    if (vertical.size() >= 2 && vertical.first() == 0)
        topInset = vertical.at(1);
    else
        topInset = 0;

    if (vertical.size() == 2 && vertical.first() > 0)
        bottomInset = vertical.last() - vertical.first();
    else if (vertical.size() == 4)
        bottomInset = vertical.last() - vertical.at(2);
    else
        bottomInset = 0;

    if (!qFuzzyCompare(oldTopInset, topInset))
        emit q->topInsetChanged();
    if (!qFuzzyCompare(oldBottomInset, bottomInset))
        emit q->bottomInsetChanged();
    if (!qFuzzyCompare(oldLeftInset, leftInset))
        emit q->leftInsetChanged();
    if (!qFuzzyCompare(oldRightInset, rightInset))
        emit q->rightInsetChanged();
}

QQuickNinePatchImage::QQuickNinePatchImage(QQuickItem *parent)
    : QQuickImage(*(new QQuickNinePatchImagePrivate), parent)
{
    Q_D(QQuickNinePatchImage);
    d->smooth = qEnvironmentVariableIntValue("QT_QUICK_CONTROLS_IMAGINE_SMOOTH");
}

qreal QQuickNinePatchImage::topPadding() const
{
    Q_D(const QQuickNinePatchImage);
    return d->topPadding / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::leftPadding() const
{
    Q_D(const QQuickNinePatchImage);
    return d->leftPadding / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::rightPadding() const
{
    Q_D(const QQuickNinePatchImage);
    return d->rightPadding / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::bottomPadding() const
{
    Q_D(const QQuickNinePatchImage);
    return d->bottomPadding / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::topInset() const
{
    Q_D(const QQuickNinePatchImage);
    return d->topInset / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::leftInset() const
{
    Q_D(const QQuickNinePatchImage);
    return d->leftInset / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::rightInset() const
{
    Q_D(const QQuickNinePatchImage);
    return d->rightInset / d->devicePixelRatio;
}

qreal QQuickNinePatchImage::bottomInset() const
{
    Q_D(const QQuickNinePatchImage);
    return d->bottomInset / d->devicePixelRatio;
}

void QQuickNinePatchImage::pixmapChange()
{
    Q_D(QQuickNinePatchImage);
    if (QFileInfo(d->url.fileName()).completeSuffix().toLower() == QLatin1String("9.png")) {
        // Keep resetNode if it is already set, we do not want to miss an
        // ImageNode->NinePatchNode change.  Without this there's a chance one gets
        // an incorrect cast on oldNode every once in a while with source changes.
        if (!d->resetNode)
            d->resetNode = d->ninePatch.isNull();

        d->ninePatch = d->pix.image();
        if (d->ninePatch.depth() != 32)
            d->ninePatch = d->ninePatch.convertToFormat(QImage::Format_ARGB32);

        int w = d->ninePatch.width();
        int h = d->ninePatch.height();
        d->pix.setImage(QImage(d->ninePatch.constBits() + 4 * (w + 1), w - 2, h - 2, d->ninePatch.bytesPerLine(), d->ninePatch.format()));

        d->updatePatches();
    } else {
        /*
            Only change resetNode when it's false; i.e. when no reset is pending.
            updatePaintNode() will take care of setting it to false if it's true.

            Consider the following changes in source:

                normal.png => press.9.png => normal.png => focus.png

            If the last two events happen quickly, pixmapChange() can be called
            twice with no call to updatePaintNode() inbetween. On the first call,
            resetNode will be true (because ninePatch is not null since it is still
            in the process of going from a 9-patch image to a regular image),
            and on the second call, resetNode would be false if we didn't have this check.
            This results in the oldNode never being deleted, and QQuickImage
            tries to static_cast a QQuickNinePatchImage to a QSGInternalImageNode.
        */
        if (!d->resetNode)
            d->resetNode = !d->ninePatch.isNull();
        d->ninePatch = QImage();
    }
    QQuickImage::pixmapChange();
}

QSGNode *QQuickNinePatchImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_D(QQuickNinePatchImage);
    Q_UNUSED(data);

    if (d->resetNode) {
        delete oldNode;
        oldNode = nullptr;
        d->resetNode = false;
    }

    if (d->ninePatch.isNull())
        return QQuickImage::updatePaintNode(oldNode, data);

    QSizeF sz = size();
    QImage image = d->pix.image();
    if (!sz.isValid() || image.isNull()) {
        if (d->provider)
            d->provider->updateTexture(nullptr);
        delete oldNode;
        return nullptr;
    }

    QQuickNinePatchNode *patchNode = static_cast<QQuickNinePatchNode *>(oldNode);
    if (!patchNode)
        patchNode = new QQuickNinePatchNode;

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(patchNode, QString::fromLatin1("QQuickNinePatchImage: '%1'").arg(d->url.toString()));
#endif

    // The image may wrap non-owned data (due to pixmapChange). Ensure we never
    // pass such an image to the scenegraph, because with a separate render
    // thread the data may become invalid (in a subsequent pixmapChange on the
    // gui thread) by the time the renderer gets to do something with the QImage
    // passed in here.
    image.detach();

    QSGTexture *texture = window()->createTextureFromImage(image);
    patchNode->initialize(texture, sz * d->devicePixelRatio, image.size(), d->xDivs, d->yDivs, d->devicePixelRatio);
    auto patchNodeMaterial = static_cast<QSGTextureMaterial *>(patchNode->material());
    patchNodeMaterial->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);
    return patchNode;
}

QT_END_NAMESPACE

#include "moc_qquickninepatchimage_p.cpp"
