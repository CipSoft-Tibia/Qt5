// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layout_propertysheet.h"

// sdk
#include <QtDesigner/qextensionmanager.h>
#include <QtDesigner/abstractformeditor.h>
// shared

#include <qlayout_widget_p.h>

#include <QtDesigner/private/ui4_p.h>
#include <QtDesigner/private/formbuilderextra_p.h>

#include <QtWidgets/qformlayout.h>

#include <QtCore/qhash.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qbytearray.h>
#include <QtCore/QRegularExpression> // Remove once there is an editor for lists

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define USE_LAYOUT_SIZE_CONSTRAINT

static const char leftMargin[] = "leftMargin";
static const char topMargin[] = "topMargin";
static const char rightMargin[] = "rightMargin";
static const char bottomMargin[] = "bottomMargin";
static const char horizontalSpacing[] = "horizontalSpacing";
static const char verticalSpacing[] = "verticalSpacing";
static const char spacing[] = "spacing";
static const char sizeConstraint[] = "sizeConstraint";
static const char boxStretchPropertyC[] = "stretch";
static const char gridRowStretchPropertyC[] = "rowStretch";
static const char gridColumnStretchPropertyC[] = "columnStretch";
static const char gridRowMinimumHeightPropertyC[] = "rowMinimumHeight";
static const char gridColumnMinimumWidthPropertyC[] = "columnMinimumWidth";

namespace {
    enum LayoutPropertyType {
        LayoutPropertyNone,
        LayoutPropertyLeftMargin,
        LayoutPropertyTopMargin,
        LayoutPropertyRightMargin,
        LayoutPropertyBottomMargin,
        LayoutPropertySpacing,
        LayoutPropertyHorizontalSpacing,
        LayoutPropertyVerticalSpacing,
        LayoutPropertySizeConstraint,
        LayoutPropertyBoxStretch,
        LayoutPropertyGridRowStretch,
        LayoutPropertyGridColumnStretch,
        LayoutPropertyGridRowMinimumHeight,
        LayoutPropertyGridColumnMinimumWidth
    };
}

// Check for a  comma-separated list of integers. Used for
// per-cell stretch properties and grid per row/column properties.
// As it works now, they are passed as QByteArray strings. The
// property sheet refuses all invalid values. This could be
// replaced by lists once the property editor can handle them.

static bool isIntegerList(const QString &s)
{
    // Check for empty string or comma-separated list of integers
    static const QRegularExpression re(u"^[0-9]+(,[0-9]+)+$"_s);
    Q_ASSERT(re.isValid());
    return s.isEmpty() || re.match(s).hasMatch();
}

// Quick lookup by name
static LayoutPropertyType  layoutPropertyType(const QString &name)
{
    static const QHash<QString, LayoutPropertyType> namePropertyMap = {
        {QLatin1StringView(leftMargin), LayoutPropertyLeftMargin},
        {QLatin1StringView(topMargin), LayoutPropertyTopMargin},
        {QLatin1StringView(rightMargin), LayoutPropertyRightMargin},
        {QLatin1StringView(bottomMargin), LayoutPropertyBottomMargin},
        {QLatin1StringView(horizontalSpacing), LayoutPropertyHorizontalSpacing},
        {QLatin1StringView(verticalSpacing), LayoutPropertyVerticalSpacing},
        {QLatin1StringView(spacing), LayoutPropertySpacing},
        {QLatin1StringView(sizeConstraint), LayoutPropertySizeConstraint},
        {QLatin1StringView(boxStretchPropertyC), LayoutPropertyBoxStretch},
        {QLatin1StringView(gridRowStretchPropertyC), LayoutPropertyGridRowStretch},
        {QLatin1StringView(gridColumnStretchPropertyC), LayoutPropertyGridColumnStretch},
        {QLatin1StringView(gridRowMinimumHeightPropertyC), LayoutPropertyGridRowMinimumHeight},
        {QLatin1StringView(gridColumnMinimumWidthPropertyC), LayoutPropertyGridColumnMinimumWidth}
    };
    return namePropertyMap.value(name, LayoutPropertyNone);
}

// return the layout margin if it is  margin
static int getLayoutMargin(const QLayout *l, LayoutPropertyType type)
{
    int left, top, right, bottom;
    l->getContentsMargins(&left, &top, &right, &bottom);
    switch (type) {
    case LayoutPropertyLeftMargin:
        return left;
    case LayoutPropertyTopMargin:
        return top;
    case LayoutPropertyRightMargin:
        return right;
    case LayoutPropertyBottomMargin:
        return bottom;
    default:
        Q_ASSERT(0);
        break;
    }
    return 0;
}

// return the layout margin if it is  margin
static void setLayoutMargin(QLayout *l, LayoutPropertyType type, int margin)
{
    int left, top, right, bottom;
    l->getContentsMargins(&left, &top, &right, &bottom);
    switch (type) {
    case LayoutPropertyLeftMargin:
        left = margin;
        break;
    case LayoutPropertyTopMargin:
        top = margin;
        break;
    case LayoutPropertyRightMargin:
        right = margin;
        break;
    case LayoutPropertyBottomMargin:
        bottom = margin;
        break;
    default:
        Q_ASSERT(0);
        break;
    }
    l->setContentsMargins(left, top, right, bottom);
}

namespace qdesigner_internal {

// ---------- LayoutPropertySheet: This sheet is never visible in
// the property editor. Rather, the sheet pulled for QLayoutWidget
// forwards all properties to it. Some properties (grid spacings) must be handled
// manually, as they are QDOC_PROPERTY only and not visible to introspection. Ditto
// for the 4 margins.

LayoutPropertySheet::LayoutPropertySheet(QLayout *l, QObject *parent)
    : QDesignerPropertySheet(l, parent), m_layout(l)
{
    const QString layoutGroup = u"Layout"_s;
    int pindex = createFakeProperty(QLatin1StringView(leftMargin), 0);
    setPropertyGroup(pindex, layoutGroup);

    pindex = createFakeProperty(QLatin1StringView(topMargin), 0);
    setPropertyGroup(pindex, layoutGroup);

    pindex = createFakeProperty(QLatin1StringView(rightMargin), 0);
    setPropertyGroup(pindex, layoutGroup);

    pindex = createFakeProperty(QLatin1StringView(bottomMargin), 0);
    setPropertyGroup(pindex, layoutGroup);

    const int visibleMask = LayoutProperties::visibleProperties(m_layout);
    if (visibleMask & LayoutProperties::HorizSpacingProperty) {
        pindex = createFakeProperty(QLatin1StringView(horizontalSpacing), 0);
        setPropertyGroup(pindex, layoutGroup);

        pindex = createFakeProperty(QLatin1StringView(verticalSpacing), 0);
        setPropertyGroup(pindex, layoutGroup);

        setAttribute(indexOf(QLatin1StringView(spacing)), true);
    }

    // Stretch
    if (visibleMask & LayoutProperties::BoxStretchProperty) {
        pindex = createFakeProperty(QLatin1StringView(boxStretchPropertyC), QByteArray());
        setPropertyGroup(pindex, layoutGroup);
        setAttribute(pindex, true);
    } else {
        // Add the grid per-row/column stretch and size limits
        if (visibleMask & LayoutProperties::GridColumnStretchProperty) {
            const QByteArray empty;
            pindex = createFakeProperty(QLatin1StringView(gridRowStretchPropertyC), empty);
            setPropertyGroup(pindex, layoutGroup);
            setAttribute(pindex, true);
            pindex = createFakeProperty(QLatin1StringView(gridColumnStretchPropertyC), empty);
            setPropertyGroup(pindex, layoutGroup);
            setAttribute(pindex, true);
            pindex = createFakeProperty(QLatin1StringView(gridRowMinimumHeightPropertyC), empty);
            setPropertyGroup(pindex, layoutGroup);
            setAttribute(pindex, true);
            pindex = createFakeProperty(QLatin1StringView(gridColumnMinimumWidthPropertyC), empty);
            setPropertyGroup(pindex, layoutGroup);
            setAttribute(pindex, true);
        }
    }
#ifdef USE_LAYOUT_SIZE_CONSTRAINT
    // SizeConstraint cannot possibly be handled as a real property
    // as it affects the layout parent widget and thus
    // conflicts with Designer's special layout widget.
    // It will take effect on the preview only.
    pindex = createFakeProperty(QLatin1StringView(sizeConstraint));
    setPropertyGroup(pindex, layoutGroup);
#endif
}

LayoutPropertySheet::~LayoutPropertySheet() = default;

void LayoutPropertySheet::setProperty(int index, const QVariant &value)
{
    const LayoutPropertyType type = layoutPropertyType(propertyName(index));
    if (QLayoutWidget *lw = qobject_cast<QLayoutWidget *>(m_layout->parent())) {
        switch (type) {
        case LayoutPropertyLeftMargin:
            lw->setLayoutLeftMargin(value.toInt());
            return;
        case LayoutPropertyTopMargin:
            lw->setLayoutTopMargin(value.toInt());
            return;
        case LayoutPropertyRightMargin:
            lw->setLayoutRightMargin(value.toInt());
            return;
        case LayoutPropertyBottomMargin:
            lw->setLayoutBottomMargin(value.toInt());
            return;
        default:
            break;
        }
    }
    switch (type) {
    case LayoutPropertyLeftMargin:
    case LayoutPropertyTopMargin:
    case LayoutPropertyRightMargin:
    case LayoutPropertyBottomMargin:
        setLayoutMargin(m_layout, type, value.toInt());
        return;
    case LayoutPropertyHorizontalSpacing:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            grid->setHorizontalSpacing(value.toInt());
            return;
        }
        if (QFormLayout *form = qobject_cast<QFormLayout *>(m_layout)) {
            form->setHorizontalSpacing(value.toInt());
            return;
        }
        break;
    case LayoutPropertyVerticalSpacing:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            grid->setVerticalSpacing(value.toInt());
            return;
        }
        if (QFormLayout *form = qobject_cast<QFormLayout *>(m_layout)) {
            form->setVerticalSpacing(value.toInt());
            return;
        }
        break;
    case LayoutPropertyBoxStretch:
        // TODO: Remove the regexp check once a proper editor for integer
        // lists is in place?
        if (QBoxLayout *box = qobject_cast<QBoxLayout *>(m_layout)) {
            const QString stretch = value.toString();
            if (isIntegerList(stretch))
                QFormBuilderExtra::setBoxLayoutStretch(value.toString(), box);
        }
        break;
    case LayoutPropertyGridRowStretch:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            const QString stretch = value.toString();
            if (isIntegerList(stretch))
                QFormBuilderExtra::setGridLayoutRowStretch(stretch, grid);
        }
        break;
    case LayoutPropertyGridColumnStretch:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            const QString stretch = value.toString();
            if (isIntegerList(stretch))
                QFormBuilderExtra::setGridLayoutColumnStretch(value.toString(), grid);
        }
        break;
    case LayoutPropertyGridRowMinimumHeight:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            const QString minSize = value.toString();
            if (isIntegerList(minSize))
                QFormBuilderExtra::setGridLayoutRowMinimumHeight(minSize, grid);
        }
        break;
    case LayoutPropertyGridColumnMinimumWidth:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout)) {
            const QString minSize = value.toString();
            if (isIntegerList(minSize))
                QFormBuilderExtra::setGridLayoutColumnMinimumWidth(minSize, grid);
        }
        break;
    default:
        break;
    }
    QDesignerPropertySheet::setProperty(index, value);
}

QVariant LayoutPropertySheet::property(int index) const
{
    const LayoutPropertyType type = layoutPropertyType(propertyName(index));
    if (const QLayoutWidget *lw = qobject_cast<QLayoutWidget *>(m_layout->parent())) {
        switch (type) {
        case LayoutPropertyLeftMargin:
            return lw->layoutLeftMargin();
        case LayoutPropertyTopMargin:
            return lw->layoutTopMargin();
        case LayoutPropertyRightMargin:
            return lw->layoutRightMargin();
        case LayoutPropertyBottomMargin:
             return lw->layoutBottomMargin();
        default:
            break;
        }
    }
    switch (type) {
    case LayoutPropertyLeftMargin:
    case LayoutPropertyTopMargin:
    case LayoutPropertyRightMargin:
    case LayoutPropertyBottomMargin:
        return getLayoutMargin(m_layout, type);
    case LayoutPropertyHorizontalSpacing:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return grid->horizontalSpacing();
        if (const QFormLayout *form = qobject_cast<QFormLayout *>(m_layout))
            return form->horizontalSpacing();
        break;
    case LayoutPropertyVerticalSpacing:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return grid->verticalSpacing();
        if (const QFormLayout *form = qobject_cast<QFormLayout *>(m_layout))
            return form->verticalSpacing();
        break;
    case LayoutPropertyBoxStretch:
        if (const QBoxLayout *box = qobject_cast<QBoxLayout *>(m_layout))
            return QVariant(QByteArray(QFormBuilderExtra::boxLayoutStretch(box).toUtf8()));
        break;
    case LayoutPropertyGridRowStretch:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return QVariant(QByteArray(QFormBuilderExtra::gridLayoutRowStretch(grid).toUtf8()));
        break;
    case LayoutPropertyGridColumnStretch:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return QVariant(QByteArray(QFormBuilderExtra::gridLayoutColumnStretch(grid).toUtf8()));
        break;
    case LayoutPropertyGridRowMinimumHeight:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return QVariant(QByteArray(QFormBuilderExtra::gridLayoutRowMinimumHeight(grid).toUtf8()));
        break;
    case LayoutPropertyGridColumnMinimumWidth:
        if (const QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            return QVariant(QByteArray(QFormBuilderExtra::gridLayoutColumnMinimumWidth(grid).toUtf8()));
        break;
    default:
        break;
    }
    return QDesignerPropertySheet::property(index);
}

bool LayoutPropertySheet::reset(int index)
{
    int left, top, right, bottom;
    m_layout->getContentsMargins(&left, &top, &right, &bottom);
    const LayoutPropertyType type = layoutPropertyType(propertyName(index));
    bool rc = true;
    switch (type) {
    case LayoutPropertyLeftMargin:
        m_layout->setContentsMargins(-1, top, right, bottom);
        break;
    case LayoutPropertyTopMargin:
        m_layout->setContentsMargins(left, -1, right, bottom);
        break;
    case LayoutPropertyRightMargin:
        m_layout->setContentsMargins(left, top, -1, bottom);
        break;
    case LayoutPropertyBottomMargin:
        m_layout->setContentsMargins(left, top, right, -1);
        break;
    case LayoutPropertyBoxStretch:
        if (QBoxLayout *box = qobject_cast<QBoxLayout *>(m_layout))
            QFormBuilderExtra::clearBoxLayoutStretch(box);
        break;
    case LayoutPropertyGridRowStretch:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            QFormBuilderExtra::clearGridLayoutRowStretch(grid);
        break;
    case LayoutPropertyGridColumnStretch:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            QFormBuilderExtra::clearGridLayoutColumnStretch(grid);
        break;
    case LayoutPropertyGridRowMinimumHeight:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            QFormBuilderExtra::clearGridLayoutRowMinimumHeight(grid);
        break;
    case LayoutPropertyGridColumnMinimumWidth:
        if (QGridLayout *grid = qobject_cast<QGridLayout *>(m_layout))
            QFormBuilderExtra::clearGridLayoutColumnMinimumWidth(grid);
        break;
    default:
        rc = QDesignerPropertySheet::reset(index);
        break;
    }
    return rc;
}

void LayoutPropertySheet::setChanged(int index, bool changed)
{
    const LayoutPropertyType type = layoutPropertyType(propertyName(index));
    switch (type) {
    case LayoutPropertySpacing:
        if (LayoutProperties::visibleProperties(m_layout) & LayoutProperties::HorizSpacingProperty) {
            setChanged(indexOf(QLatin1StringView(horizontalSpacing)), changed);
            setChanged(indexOf(QLatin1StringView(verticalSpacing)), changed);
        }
        break;
    default:
        break;
    }
    QDesignerPropertySheet::setChanged(index, changed);
}

void LayoutPropertySheet::stretchAttributesToDom(QDesignerFormEditorInterface *core, QLayout *lt, DomLayout *domLayout)
{
    // Check if the respective stretch properties of the layout are changed.
    // If so, set them to the DOM
    const int visibleMask = LayoutProperties::visibleProperties(lt);
    if (!(visibleMask & (LayoutProperties::BoxStretchProperty|LayoutProperties::GridColumnStretchProperty|LayoutProperties::GridRowStretchProperty)))
        return;
    const QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), lt);
    Q_ASSERT(sheet);

    // Stretch
    if (visibleMask & LayoutProperties::BoxStretchProperty) {
        const int index = sheet->indexOf(QLatin1StringView(boxStretchPropertyC));
        Q_ASSERT(index != -1);
        if (sheet->isChanged(index))
            domLayout->setAttributeStretch(sheet->property(index).toString());
    }
    if (visibleMask & LayoutProperties::GridColumnStretchProperty) {
        const int index = sheet->indexOf(QLatin1StringView(gridColumnStretchPropertyC));
        Q_ASSERT(index != -1);
        if (sheet->isChanged(index))
            domLayout->setAttributeColumnStretch(sheet->property(index).toString());
    }
    if (visibleMask & LayoutProperties::GridRowStretchProperty) {
        const int index = sheet->indexOf(QLatin1StringView(gridRowStretchPropertyC));
        Q_ASSERT(index != -1);
        if (sheet->isChanged(index))
            domLayout->setAttributeRowStretch(sheet->property(index).toString());
    }
    if (visibleMask & LayoutProperties::GridRowMinimumHeightProperty) {
        const int index = sheet->indexOf(QLatin1StringView(gridRowMinimumHeightPropertyC));
        Q_ASSERT(index != -1);
        if (sheet->isChanged(index))
            domLayout->setAttributeRowMinimumHeight(sheet->property(index).toString());
    }
    if (visibleMask & LayoutProperties::GridColumnMinimumWidthProperty) {
        const int index = sheet->indexOf(QLatin1StringView(gridColumnMinimumWidthPropertyC));
        Q_ASSERT(index != -1);
        if (sheet->isChanged(index))
            domLayout->setAttributeColumnMinimumWidth(sheet->property(index).toString());
    }
}

void LayoutPropertySheet::markChangedStretchProperties(QDesignerFormEditorInterface *core, QLayout *lt, const DomLayout *domLayout)
{
    // While the actual values are applied by the form builder, we still need
    // to mark them as 'changed'.
    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), lt);
    Q_ASSERT(sheet);
    if (!domLayout->attributeStretch().isEmpty())
        sheet->setChanged(sheet->indexOf(QLatin1StringView(boxStretchPropertyC)), true);
    if (!domLayout->attributeRowStretch().isEmpty())
        sheet->setChanged(sheet->indexOf(QLatin1StringView(gridRowStretchPropertyC)), true);
    if (!domLayout->attributeColumnStretch().isEmpty())
        sheet->setChanged(sheet->indexOf(QLatin1StringView(gridColumnStretchPropertyC)), true);
   if (!domLayout->attributeColumnMinimumWidth().isEmpty())
        sheet->setChanged(sheet->indexOf(QLatin1StringView(gridColumnMinimumWidthPropertyC)), true);
   if (!domLayout->attributeRowMinimumHeight().isEmpty())
        sheet->setChanged(sheet->indexOf(QLatin1StringView(gridRowMinimumHeightPropertyC)), true);
}

}

QT_END_NAMESPACE
