// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandclipboard_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatasource_p.h"
#include "qwaylanddatadevice_p.h"
#if QT_CONFIG(wayland_client_primary_selection)
#include "qwaylandprimaryselectionv1_p.h"
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandClipboard::QWaylandClipboard(QWaylandDisplay *display)
    : mDisplay(display)
{
    m_clientClipboard[QClipboard::Clipboard] = nullptr;
    m_clientClipboard[QClipboard::Selection] = nullptr;
}

QWaylandClipboard::~QWaylandClipboard()
{
    if (m_clientClipboard[QClipboard::Clipboard] != m_clientClipboard[QClipboard::Selection])
        delete m_clientClipboard[QClipboard::Clipboard];
    delete m_clientClipboard[QClipboard::Selection];
}

QMimeData *QWaylandClipboard::mimeData(QClipboard::Mode mode)
{
    auto *seat = mDisplay->currentInputDevice();
    if (!seat)
        return &m_emptyData;

    switch (mode) {
    case QClipboard::Clipboard:
        if (auto *dataDevice = seat->dataDevice()) {
            if (dataDevice->selectionSource())
                return m_clientClipboard[QClipboard::Clipboard];
            if (auto *offer = dataDevice->selectionOffer())
                return offer->mimeData();
        }
        return &m_emptyData;
    case QClipboard::Selection:
#if QT_CONFIG(wayland_client_primary_selection)
        if (auto *selectionDevice = seat->primarySelectionDevice()) {
            if (selectionDevice->selectionSource())
                return m_clientClipboard[QClipboard::Selection];
            if (auto *offer = selectionDevice->selectionOffer())
                return offer->mimeData();
        }
#endif
        return &m_emptyData;
    default:
        return &m_emptyData;
    }
}

void QWaylandClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    auto *seat = mDisplay->currentInputDevice();
    if (!seat) {
        qCWarning(lcQpaWayland) << "Can't set clipboard contents with no wl_seats available";
        return;
    }

    static const QString plain = QStringLiteral("text/plain");
    static const QString utf8 = QStringLiteral("text/plain;charset=utf-8");

    if (data && data->hasFormat(plain) && !data->hasFormat(utf8))
        data->setData(utf8, data->data(plain));

    if (m_clientClipboard[mode]) {
        if (m_clientClipboard[QClipboard::Clipboard] != m_clientClipboard[QClipboard::Selection])
            delete m_clientClipboard[mode];
        m_clientClipboard[mode] = nullptr;
    }

    m_clientClipboard[mode] = data;

    switch (mode) {
    case QClipboard::Clipboard:
        if (auto *dataDevice = seat->dataDevice()) {
            dataDevice->setSelectionSource(data ? new QWaylandDataSource(mDisplay->dndSelectionHandler(),
                                                                         m_clientClipboard[QClipboard::Clipboard]) : nullptr);
            emitChanged(mode);
        }
        break;
    case QClipboard::Selection:
#if QT_CONFIG(wayland_client_primary_selection)
        if (auto *selectionDevice = seat->primarySelectionDevice()) {
            selectionDevice->setSelectionSource(data ? new QWaylandPrimarySelectionSourceV1(mDisplay->primarySelectionManager(),
                                                                                            m_clientClipboard[QClipboard::Selection]) : nullptr);
            emitChanged(mode);
        }
#endif
        break;
    default:
        break;
    }
}

bool QWaylandClipboard::supportsMode(QClipboard::Mode mode) const
{
#if QT_CONFIG(wayland_client_primary_selection)
    if (mode == QClipboard::Selection) {
        auto *seat = mDisplay->currentInputDevice();
        return seat && seat->primarySelectionDevice();
    }
#endif
    return mode == QClipboard::Clipboard;
}

bool QWaylandClipboard::ownsMode(QClipboard::Mode mode) const
{
    QWaylandInputDevice *seat = mDisplay->currentInputDevice();
    if (!seat)
        return false;

    switch (mode) {
    case QClipboard::Clipboard:
        return seat->dataDevice() && seat->dataDevice()->selectionSource() != nullptr;
#if QT_CONFIG(wayland_client_primary_selection)
    case QClipboard::Selection:
        return seat->primarySelectionDevice() && seat->primarySelectionDevice()->selectionSource() != nullptr;
#endif
    default:
        return false;
    }
}

}

QT_END_NAMESPACE
