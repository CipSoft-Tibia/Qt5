#ifndef GETOPTIONALLOOKUP_H
#define GETOPTIONALLOOKUP_H

#include <QObject>
#include <QQmlEngine>

class GOL_Object : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int i READ i WRITE setI)
    Q_PROPERTY(QString s READ s WRITE setS)
    Q_PROPERTY(GOL_Object *childA READ childA WRITE setChildA)
    Q_PROPERTY(Enum e READ e)

public:
    GOL_Object(QObject *parent = nullptr) : QObject(parent) { }

    int i() const { return m_i; }
    void setI(int i) { m_i = i; }

    QString s() const { return m_s; }
    void setS(QString s) { m_s = s; }

    GOL_Object *childA() const { return m_childA; }
    void setChildA(GOL_Object *a) { m_childA = a; }

    enum Enum { V1, V2 };
    Q_ENUM(Enum)
    Enum e() const { return Enum::V2; }

private:
    int m_i = 5;
    QString m_s = "6";
    GOL_Object *m_childA = nullptr;
};

#endif // GETOPTIONALLOOKUP_H
