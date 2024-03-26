// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwldatadevicemanager_p.h"

#include <QtWaylandCompositor/QWaylandCompositor>

#include <QtWaylandCompositor/private/qwaylandcompositor_p.h>
#include <QtWaylandCompositor/private/qwaylandseat_p.h>
#include "qwldatadevice_p.h"
#include "qwldatasource_p.h"
#include "qwldataoffer_p.h"
#include "qwaylandmimehelper_p.h"

#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>
#include <fcntl.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QFile>

QT_BEGIN_NAMESPACE

namespace QtWayland {

DataDeviceManager::DataDeviceManager(QWaylandCompositor *compositor)
    : wl_data_device_manager(compositor->display(), 1)
    , m_compositor(compositor)
{
}

void DataDeviceManager::setCurrentSelectionSource(DataSource *source)
{
    if (m_current_selection_source && source
            && m_current_selection_source->time() > source->time()) {
        qDebug() << "Trying to set older selection";
        return;
    }

    m_compositorOwnsSelection = false;

    finishReadFromClient();

    m_current_selection_source = source;
    if (source)
        source->setManager(this);

    // When retained selection is enabled, the compositor will query all the data from the client.
    // This makes it possible to
    //    1. supply the selection after the offering client is gone
    //    2. make it possible for the compositor to participate in copy-paste
    // The downside is decreased performance, therefore this mode has to be enabled
    // explicitly in the compositors.
    if (source && m_compositor->retainedSelectionEnabled()) {
        m_retainedData.clear();
        m_retainedReadIndex = 0;
        retain();
    }
}

void DataDeviceManager::sourceDestroyed(DataSource *source)
{
    if (m_current_selection_source == source)
        finishReadFromClient();
}

void DataDeviceManager::retain()
{
    QList<QString> offers = m_current_selection_source->mimeTypes();
    finishReadFromClient();
    if (m_retainedReadIndex >= offers.size()) {
        QWaylandCompositorPrivate::get(m_compositor)->feedRetainedSelectionData(&m_retainedData);
        return;
    }
    QString mimeType = offers.at(m_retainedReadIndex);
    m_retainedReadBuf.clear();
    int fd[2];
    if (pipe(fd) == -1) {
        qWarning("Clipboard: Failed to create pipe");
        return;
    }
    fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL, 0) | O_NONBLOCK);
    m_current_selection_source->send(mimeType, fd[1]);
    m_retainedReadNotifier = new QSocketNotifier(fd[0], QSocketNotifier::Read, this);
    connect(m_retainedReadNotifier, &QSocketNotifier::activated, this, &DataDeviceManager::readFromClient);
}

void DataDeviceManager::finishReadFromClient(bool exhausted)
{
    Q_UNUSED(exhausted);
    if (m_retainedReadNotifier) {
        if (exhausted) {
            int fd = m_retainedReadNotifier->socket();
            delete m_retainedReadNotifier;
            close(fd);
        } else {
            // Do not close the handle or destroy the read notifier here
            // or else clients may SIGPIPE.
            m_obsoleteRetainedReadNotifiers.append(m_retainedReadNotifier);
        }
        m_retainedReadNotifier = nullptr;
    }
}

void DataDeviceManager::readFromClient(int fd)
{
    static char buf[4096];
    int obsCount = m_obsoleteRetainedReadNotifiers.size();
    for (int i = 0; i < obsCount; ++i) {
        QSocketNotifier *sn = m_obsoleteRetainedReadNotifiers.at(i);
        if (sn->socket() == fd) {
            // Read and drop the data, stopping to read and closing the handle
            // is not yet safe because that could kill the client with SIGPIPE
            // when it still tries to write.
            int n;
            do {
                n = QT_READ(fd, buf, sizeof buf);
            } while (n > 0);
            if (n != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                m_obsoleteRetainedReadNotifiers.removeAt(i);
                delete sn;
                close(fd);
            }
            return;
        }
    }
    int n = QT_READ(fd, buf, sizeof buf);
    if (n <= 0) {
        if (n != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            finishReadFromClient(true);
            QList<QString> offers = m_current_selection_source->mimeTypes();
            QString mimeType = offers.at(m_retainedReadIndex);
            m_retainedData.setData(mimeType, m_retainedReadBuf);
            ++m_retainedReadIndex;
            retain();
        }
    } else {
        m_retainedReadBuf.append(buf, n);
    }
}

DataSource *DataDeviceManager::currentSelectionSource()
{
    return m_current_selection_source;
}

struct wl_display *DataDeviceManager::display() const
{
    return m_compositor->display();
}

void DataDeviceManager::overrideSelection(const QMimeData &mimeData)
{
    const QStringList formats = mimeData.formats();
    if (formats.isEmpty())
        return;

    m_retainedData.clear();
    for (const QString &format : formats)
        m_retainedData.setData(format, mimeData.data(format));

    QWaylandCompositorPrivate::get(m_compositor)->feedRetainedSelectionData(&m_retainedData);

    m_compositorOwnsSelection = true;

    QWaylandSeat *dev = m_compositor->defaultSeat();
    QWaylandSurface *focusSurface = dev->keyboardFocus();
    if (focusSurface)
        offerFromCompositorToClient(
                    QWaylandSeatPrivate::get(dev)->dataDevice()->resourceMap().value(focusSurface->waylandClient())->handle);
}

bool DataDeviceManager::offerFromCompositorToClient(wl_resource *clientDataDeviceResource)
{
    if (!m_compositorOwnsSelection)
        return false;

    wl_client *client = wl_resource_get_client(clientDataDeviceResource);
    //qDebug("compositor offers %d types to %p", m_retainedData.formats().count(), client);

    struct wl_resource *selectionOffer =
             wl_resource_create(client, &wl_data_offer_interface, -1, 0);
    wl_resource_set_implementation(selectionOffer, &compositor_offer_interface, this, nullptr);
    wl_data_device_send_data_offer(clientDataDeviceResource, selectionOffer);
    const auto formats = m_retainedData.formats();
    for (const QString &format : formats) {
        QByteArray ba = format.toLatin1();
        wl_data_offer_send_offer(selectionOffer, ba.constData());
    }
    wl_data_device_send_selection(clientDataDeviceResource, selectionOffer);

    return true;
}

void DataDeviceManager::offerRetainedSelection(wl_resource *clientDataDeviceResource)
{
    if (m_retainedData.formats().isEmpty())
        return;

    m_compositorOwnsSelection = true;
    offerFromCompositorToClient(clientDataDeviceResource);
}

void DataDeviceManager::data_device_manager_create_data_source(Resource *resource, uint32_t id)
{
    new DataSource(resource->client(), id, m_compositor->currentTimeMsecs());
}

void DataDeviceManager::data_device_manager_get_data_device(Resource *resource, uint32_t id, struct ::wl_resource *seat)
{
    QWaylandSeat *input_device = QWaylandSeat::fromSeatResource(seat);
    QWaylandSeatPrivate::get(input_device)->clientRequestedDataDevice(this, resource->client(), id);
}

void DataDeviceManager::comp_accept(wl_client *, wl_resource *, uint32_t, const char *)
{
}

void DataDeviceManager::comp_receive(wl_client *client, wl_resource *resource, const char *mime_type, int32_t fd)
{
    Q_UNUSED(client);
    DataDeviceManager *self = static_cast<DataDeviceManager *>(wl_resource_get_user_data(resource));
    //qDebug("client %p wants data for type %s from compositor", client, mime_type);
    QByteArray content = QWaylandMimeHelper::getByteArray(&self->m_retainedData, QString::fromLatin1(mime_type));
    if (!content.isEmpty()) {
        QFile f;
        if (f.open(fd, QIODevice::WriteOnly))
            f.write(content);
    }
    close(fd);
}

void DataDeviceManager::comp_destroy(wl_client *, wl_resource *)
{
}

QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")
QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")

const struct wl_data_offer_interface DataDeviceManager::compositor_offer_interface = {
    DataDeviceManager::comp_accept,
    DataDeviceManager::comp_receive,
    DataDeviceManager::comp_destroy
};

} //namespace

QT_END_NAMESPACE

#include "moc_qwldatadevicemanager_p.cpp"
