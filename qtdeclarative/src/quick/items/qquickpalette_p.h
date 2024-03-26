// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQUICKPALETTE_H
#define QQUICKPALETTE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QQuickPalette. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qquickcolorgroup_p.h>

#include <array>

QT_BEGIN_NAMESPACE

class QQuickAbstractPaletteProvider;

class Q_QUICK_PRIVATE_EXPORT QQuickPalette : public QQuickColorGroup
{
    Q_OBJECT

    Q_PROPERTY(QQuickColorGroup *active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QQuickColorGroup *inactive READ inactive WRITE setInactive NOTIFY inactiveChanged FINAL)
    Q_PROPERTY(QQuickColorGroup *disabled READ disabled WRITE setDisabled NOTIFY disabledChanged FINAL)

    QML_NAMED_ELEMENT(Palette)
    QML_ADDED_IN_VERSION(6, 0)

public: // Types
    using PalettePtr = QPointer<QQuickPalette>;

public:
    Q_DISABLE_COPY_MOVE(QQuickPalette)
    explicit QQuickPalette(QObject *parent = nullptr);

    QQuickColorGroup *active() const;
    QQuickColorGroup *inactive() const;
    QQuickColorGroup *disabled() const;

    QPalette::ColorGroup currentColorGroup() const override;
    void setCurrentGroup(QPalette::ColorGroup currentGroup);

    void fromQPalette(QPalette palette);
    QPalette toQPalette() const;

    const QQuickAbstractPaletteProvider *paletteProvider() const;
    void setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider);

    void reset();

    void inheritPalette(const QPalette &palette);

public Q_SLOTS:
    void setActive(QQuickColorGroup *active);
    void setInactive(QQuickColorGroup *inactive);
    void setDisabled(QQuickColorGroup *disabled);

Q_SIGNALS:
    void activeChanged();
    void inactiveChanged();
    void disabledChanged();

private:
    void setColorGroup(QPalette::ColorGroup groupTag,
                       const QQuickColorGroup::GroupPtr &group,
                       void (QQuickPalette::*notifier)());
    QQuickColorGroup::GroupPtr colorGroup(QPalette::ColorGroup groupTag) const;
    QQuickColorGroup::GroupPtr findColorGroup(QPalette::ColorGroup groupTag) const;

    void registerColorGroup(QQuickColorGroup *group, QPalette::ColorGroup groupTag);

    bool isValidColorGroup(QPalette::ColorGroup groupTag,
                           const QQuickColorGroup::GroupPtr &colorGroup) const;

    static constexpr QPalette::ColorGroup defaultCurrentGroup() { return QPalette::Active; }

private:
    std::array<QQuickColorGroup::GroupPtr, QPalette::NColorGroups> m_colorGroups = {};
    QPalette::ColorGroup m_currentGroup;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPalette)

#endif // QQUICKPALETTE_H
