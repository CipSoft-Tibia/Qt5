// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [MySensor]
class MyReadingPrivate;

class MyReading : public QSensorReading
  {
    Q_OBJECT
    Q_PROPERTY(qreal myprop READ myprop)
    DECLARE_READING(MyReading)
public:
    qreal myprop() const;
    void setMyprop(qreal myprop);
  };

class MySensor : public QSensor
{
    Q_OBJECT
public:
    explicit MySensor(QObject *parent = 0);
    MyReading *reading() const;
    static char const * const sensorType;
  };
//! [MySensor]
