// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qstringlist.h>
#include "registry_p.h"

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN32
/*
  Returns the path part of a registry key.
  e.g.
      For a key
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"
      it returns
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\"
*/
static QString keyPath(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return QString();
    return rKey.left(idx + 1);
}

/*
  Returns the name part of a registry key.
  e.g.
      For a key
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"
      it returns
          "ProductDir"
*/
static QString keyName(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return rKey;

    QString res(rKey.mid(idx + 1));
    if (res == QLatin1String("Default") || res == QLatin1String("."))
        res = QString();
    return res;
}
#endif

QString qt_readRegistryKey(HKEY parentHandle, const QString &rSubkey, unsigned long options)
{
    QString result;

#ifdef Q_OS_WIN32
    QString rSubkeyName = keyName(rSubkey);
    QString rSubkeyPath = keyPath(rSubkey);

    HKEY handle = nullptr;
    LONG res = RegOpenKeyEx(parentHandle, (wchar_t*)rSubkeyPath.utf16(), 0,
                            KEY_READ | options, &handle);

    if (res != ERROR_SUCCESS)
        return QString();

    // get the size and type of the value
    DWORD dataType;
    DWORD dataSize;
    res = RegQueryValueEx(handle, (wchar_t*)rSubkeyName.utf16(), nullptr, &dataType, nullptr, &dataSize);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    // get the value
    QByteArray data(dataSize, 0);
    res = RegQueryValueEx(handle, (wchar_t*)rSubkeyName.utf16(), nullptr, nullptr,
                          reinterpret_cast<unsigned char*>(data.data()), &dataSize);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    switch (dataType) {
        case REG_EXPAND_SZ:
        case REG_SZ: {
            result = QString::fromWCharArray(((const wchar_t *)data.constData()));
            break;
        }

        case REG_MULTI_SZ: {
            QStringList l;
            int i = 0;
            for (;;) {
                QString s = QString::fromWCharArray((const wchar_t *)data.constData() + i);
                i += s.length() + 1;

                if (s.isEmpty())
                    break;
                l.append(s);
            }
            result = l.join(QLatin1String(", "));
            break;
        }

        case REG_NONE:
        case REG_BINARY: {
            result = QString::fromWCharArray((const wchar_t *)data.constData(), data.size() / 2);
            break;
        }

        case REG_DWORD_BIG_ENDIAN:
        case REG_DWORD: {
            Q_ASSERT(data.size() == sizeof(int));
            int i;
            memcpy((char*)&i, data.constData(), sizeof(int));
            result = QString::number(i);
            break;
        }

        default:
            qWarning("QSettings: unknown data %u type in windows registry", quint32(dataType));
            break;
    }

    RegCloseKey(handle);
#else
    Q_UNUSED(parentHandle);
    Q_UNUSED(rSubkey);
    Q_UNUSED(options);
#endif

    return result;
}

QT_END_NAMESPACE

