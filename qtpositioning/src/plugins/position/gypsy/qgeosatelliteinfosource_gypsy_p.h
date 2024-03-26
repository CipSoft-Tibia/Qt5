// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOSATELLITEINFOSOURCE_GYPSY_H
#define QGEOSATELLITEINFOSOURCE_GYPSY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgeosatelliteinfosource.h"
#include "qgeosatelliteinfo.h"
#include <gypsy/gypsy-satellite.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gconf/gconf-client.h>
#include <QTimer>

// #define Q_LOCATION_GYPSY_DEBUG

QT_BEGIN_NAMESPACE

// An engine that encapsulates all symbols we want
// to be able to mock (for unit/autotest purposes).
class SatelliteGypsyEngine
{
public:
    SatelliteGypsyEngine(QGeoSatelliteInfoSource *parent = 0);
    virtual ~SatelliteGypsyEngine();
    // Glib symbols
    virtual gulong eng_g_signal_connect(gpointer instance,
                                    const gchar *detailed_signal,
                                    GCallback c_handler,
                                    gpointer data);
    virtual guint eng_g_signal_handlers_disconnect_by_func(gpointer instance,
                                                        gpointer func,
                                                        gpointer data);
    virtual void eng_g_free(gpointer mem);
    // Gypsy symbols
    virtual GypsyControl *eng_gypsy_control_get_default (void);
    virtual char *eng_gypsy_control_create (GypsyControl *control, const char *device_name, GError **error);
    virtual GypsyDevice *eng_gypsy_device_new (const char *object_path);
    virtual GypsySatellite *eng_gypsy_satellite_new (const char *object_path);
    virtual gboolean eng_gypsy_device_start (GypsyDevice *device, GError **error);
    virtual gboolean eng_gypsy_device_stop (GypsyDevice *device, GError **error);
    virtual GypsyDeviceFixStatus eng_gypsy_device_get_fix_status (GypsyDevice *device, GError **error);
    virtual GPtrArray *eng_gypsy_satellite_get_satellites (GypsySatellite *satellite, GError **error);
    virtual void eng_gypsy_satellite_free_satellite_array (GPtrArray *satellites);
    // GConf symbols (mockability due to X11 requirement)
    virtual GConfClient *eng_gconf_client_get_default(void);
    virtual gchar *eng_gconf_client_get_string(GConfClient *client, const gchar *key, GError** err);
protected:
    QGeoSatelliteInfoSource *m_owner;
};

class QGeoSatelliteInfoSourceGypsy : public QGeoSatelliteInfoSource
  {
      Q_OBJECT

public:
      explicit QGeoSatelliteInfoSourceGypsy(QObject *parent = 0);
      ~QGeoSatelliteInfoSourceGypsy();
      int init(const QVariantMap parameters);

      int minimumUpdateInterval() const override;
      Error error() const override;

public slots:
      virtual void startUpdates() override;
      void stopUpdates() override;
      void requestUpdate(int timeout = 5000) override;
      void satellitesChanged(GypsySatellite *satellite, GPtrArray *satellites);

private slots:
      void requestUpdateTimeout();

private:
      void setError(QGeoSatelliteInfoSource::Error error);
      QString extractDeviceNameFromParameters(const QVariantMap &parameters) const;

protected:
      // Creates an engine which encapsulates all used symbols
      // that we want to be also able to mock.
      virtual void createEngine();
      SatelliteGypsyEngine *m_engine;

private:
      Q_DISABLE_COPY(QGeoSatelliteInfoSourceGypsy)
      GypsySatellite *m_satellite;
      GypsyDevice *m_device;
      QTimer m_requestTimer;
      bool m_updatesOngoing;
      bool m_requestOngoing;
      QGeoSatelliteInfoSource::Error m_error = QGeoSatelliteInfoSource::NoError;
      GypsyControl *m_control = nullptr;
  };

QT_END_NAMESPACE

#endif // QGEOSATELLITEINFOSOURCE_GYPSY_H
