// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstract3dinputhandler_p.h"
#include "qquickgraphsitem_p.h"

#include "q3dscene_p.h"
#include "q3dtheme_p.h"
#include "qabstract3daxis_p.h"
#include "qabstract3dseries.h"
#include "qabstract3dseries_p.h"
#include "qcategory3daxis.h"
#include "qcategory3daxis_p.h"
#include "qcustom3ditem.h"
#include "qcustom3ditem_p.h"
#include "qcustom3dlabel.h"
#include "qcustom3dvolume.h"
#include "qtouch3dinputhandler.h"
#include "qvalue3daxis.h"
#include "qvalue3daxis_p.h"
#include "thememanager_p.h"
#include "utils_p.h"

#include <QtGui/QGuiApplication>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dloader_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>

#if defined(Q_OS_IOS)
#include <QtCore/QTimer>
#endif

#if defined(Q_OS_MACOS)
#include <qpa/qplatformnativeinterface.h>
#endif

QT_BEGIN_NAMESPACE

constexpr float doublePi = static_cast<float>(M_PI) * 2.0f;
constexpr float polarRoundness = 64.0f;

QQuickGraphsItem::QQuickGraphsItem(QQuickItem *parent)
    : QQuick3DViewport(parent)
    , m_locale(QLocale::c())
{
    if (!m_scene)
        m_scene = new Q3DScene;
    m_scene->setParent(this);

    m_qml = this;
    m_themeManager = new ThemeManager(this);

    // Set initial theme
    Q3DTheme *defaultTheme = new Q3DTheme(Q3DTheme::Theme::Qt);
    defaultTheme->d_func()->setDefaultTheme(true);
    setTheme(defaultTheme);

    m_scene->d_func()->setViewport(boundingRect().toRect());

    connect(m_scene, &Q3DScene::needRender, this, &QQuickGraphsItem::emitNeedRender);
    connect(m_scene,
            &Q3DScene::graphPositionQueryChanged,
            this,
            &QQuickGraphsItem::handleQueryPositionChanged);

    m_nodeMutex = QSharedPointer<QMutex>::create();

    QQuick3DSceneEnvironment *scene = environment();
    scene->setBackgroundMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes::Color);
    scene->setClearColor(Qt::transparent);

    auto sceneManager = QQuick3DObjectPrivate::get(rootNode())->sceneManager;
    connect(sceneManager.data(),
            &QQuick3DSceneManager::windowChanged,
            this,
            &QQuickGraphsItem::handleWindowChanged);
    // Set contents to false in case we are in qml designer to make component look
    // nice
    m_runningInDesigner = QGuiApplication::applicationDisplayName() == QLatin1String("Qml2Puppet");
    setFlag(ItemHasContents /*, !m_runningInDesigner*/); // Is this relevant anymore?

    // Set 4x MSAA by default
    setRenderingMode(QAbstract3DGraph::RenderingMode::Indirect);
    setMsaaSamples(4);

    // Accept touchevents
    setAcceptTouchEvents(true);
}

QQuickGraphsItem::~QQuickGraphsItem()
{
    disconnect(this, 0, this, 0);
    checkWindowList(0);

    m_repeaterX->model().clear();
    m_repeaterY->model().clear();
    m_repeaterZ->model().clear();
    m_repeaterX->deleteLater();
    m_repeaterY->deleteLater();
    m_repeaterZ->deleteLater();

    delete m_gridGeometryModel;
    delete m_sliceGridGeometryModel;

    // Make sure not deleting locked mutex
    QMutexLocker locker(&m_mutex);
    locker.unlock();

    m_nodeMutex.clear();
}

void QQuickGraphsItem::handleAxisTitleChanged(const QString &title)
{
    Q_UNUSED(title);
    handleAxisTitleChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisTitleChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXTitleChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYTitleChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZTitleChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";

    markSeriesItemLabelsDirty();
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisLabelsChanged()
{
    handleAxisLabelsChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisLabelsChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXLabelsChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYLabelsChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZLabelsChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";

    markSeriesItemLabelsDirty();
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisRangeChanged(float min, float max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);
    handleAxisRangeChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisRangeChangedBySender(QObject *sender)
{
    if (sender == m_axisX) {
        m_isSeriesVisualsDirty = true;
        m_changeTracker.axisXRangeChanged = true;
    } else if (sender == m_axisY) {
        m_isSeriesVisualsDirty = true;
        m_changeTracker.axisYRangeChanged = true;
    } else if (sender == m_axisZ) {
        m_isSeriesVisualsDirty = true;
        m_changeTracker.axisZRangeChanged = true;
    } else {
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    }
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisSegmentCountChanged(int count)
{
    Q_UNUSED(count);
    handleAxisSegmentCountChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisSegmentCountChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXSegmentCountChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYSegmentCountChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZSegmentCountChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisSubSegmentCountChanged(int count)
{
    Q_UNUSED(count);
    handleAxisSubSegmentCountChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisSubSegmentCountChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXSubSegmentCountChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYSubSegmentCountChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZSubSegmentCountChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisAutoAdjustRangeChanged(bool autoAdjust)
{
    QObject *sender = QObject::sender();
    if (sender != m_axisX && sender != m_axisY && sender != m_axisZ)
        return;

    QAbstract3DAxis *axis = static_cast<QAbstract3DAxis *>(sender);
    handleAxisAutoAdjustRangeChangedInOrientation(axis->orientation(), autoAdjust);
}

void QQuickGraphsItem::handleAxisLabelFormatChanged(const QString &format)
{
    Q_UNUSED(format);
    handleAxisLabelFormatChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisReversedChanged(bool enable)
{
    Q_UNUSED(enable);
    handleAxisReversedChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisFormatterDirty()
{
    handleAxisFormatterDirtyBySender(sender());
}

void QQuickGraphsItem::handleAxisLabelAutoRotationChanged(float angle)
{
    Q_UNUSED(angle);
    handleAxisLabelAutoRotationChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisTitleVisibilityChanged(bool visible)
{
    Q_UNUSED(visible);
    handleAxisTitleVisibilityChangedBySender(sender());
}

void QQuickGraphsItem::handleAxisTitleFixedChanged(bool fixed)
{
    Q_UNUSED(fixed);
    handleAxisTitleFixedChangedBySender(sender());
}

void QQuickGraphsItem::handleInputViewChanged(QAbstract3DInputHandler::InputView view)
{
    // When in automatic slicing mode, input view change to primary disables slice mode
    if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionSlice)
        && view == QAbstract3DInputHandler::InputView::OnPrimary) {
        setSlicingActive(false);
    }

    emitNeedRender();
}

void QQuickGraphsItem::handleInputPositionChanged(const QPoint &position)
{
    Q_UNUSED(position);
    emitNeedRender();
}

void QQuickGraphsItem::handleSeriesVisibilityChanged(bool visible)
{
    Q_UNUSED(visible);

    handleSeriesVisibilityChangedBySender(sender());
}

void QQuickGraphsItem::handleRequestShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    setShadowQuality(quality);
}

void QQuickGraphsItem::handleQueryPositionChanged(const QPoint &position)
{
    graphPositionAt(position);
}

void QQuickGraphsItem::handleAxisLabelFormatChangedBySender(QObject *sender)
{
    // Label format changing needs to dirty the data so that labels are reset.
    if (sender == m_axisX) {
        m_isDataDirty = true;
        m_changeTracker.axisXLabelFormatChanged = true;
    } else if (sender == m_axisY) {
        m_isDataDirty = true;
        m_changeTracker.axisYLabelFormatChanged = true;
    } else if (sender == m_axisZ) {
        m_isDataDirty = true;
        m_changeTracker.axisZLabelFormatChanged = true;
    } else {
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    }
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisReversedChangedBySender(QObject *sender)
{
    // Reversing change needs to dirty the data so item positions are recalculated
    if (sender == m_axisX) {
        m_isDataDirty = true;
        m_changeTracker.axisXReversedChanged = true;
    } else if (sender == m_axisY) {
        m_isDataDirty = true;
        m_changeTracker.axisYReversedChanged = true;
    } else if (sender == m_axisZ) {
        m_isDataDirty = true;
        m_changeTracker.axisZReversedChanged = true;
    } else {
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    }
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisFormatterDirtyBySender(QObject *sender)
{
    // Sender is QValue3DAxisPrivate
    QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(sender);
    if (valueAxis == m_axisX) {
        m_isDataDirty = true;
        m_changeTracker.axisXFormatterChanged = true;
    } else if (valueAxis == m_axisY) {
        m_isDataDirty = true;
        m_changeTracker.axisYFormatterChanged = true;
    } else if (valueAxis == m_axisZ) {
        m_isDataDirty = true;
        m_changeTracker.axisZFormatterChanged = true;
    } else {
        qWarning() << __FUNCTION__ << "invoked for invalid axis";
    }
    emitNeedRender();
}

void QQuickGraphsItem::handleAxisLabelAutoRotationChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXLabelAutoRotationChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYLabelAutoRotationChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZLabelAutoRotationChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";

    emitNeedRender();
}

void QQuickGraphsItem::handleAxisTitleVisibilityChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXTitleVisibilityChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYTitleVisibilityChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZTitleVisibilityChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";

    emitNeedRender();
}

void QQuickGraphsItem::handleAxisTitleFixedChangedBySender(QObject *sender)
{
    if (sender == m_axisX)
        m_changeTracker.axisXTitleFixedChanged = true;
    else if (sender == m_axisY)
        m_changeTracker.axisYTitleFixedChanged = true;
    else if (sender == m_axisZ)
        m_changeTracker.axisZTitleFixedChanged = true;
    else
        qWarning() << __FUNCTION__ << "invoked for invalid axis";

    emitNeedRender();
}

void QQuickGraphsItem::handleSeriesVisibilityChangedBySender(QObject *sender)
{
    QAbstract3DSeries *series = static_cast<QAbstract3DSeries *>(sender);
    series->d_func()->m_changeTracker.visibilityChanged = true;

    m_isDataDirty = true;
    m_isSeriesVisualsDirty = true;

    adjustAxisRanges();

    emitNeedRender();
}

void QQuickGraphsItem::markDataDirty()
{
    m_isDataDirty = true;

    markSeriesItemLabelsDirty();
    emitNeedRender();
}

void QQuickGraphsItem::markSeriesVisualsDirty()
{
    m_isSeriesVisualsDirty = true;
    emitNeedRender();
}

void QQuickGraphsItem::markSeriesItemLabelsDirty()
{
    for (int i = 0; i < m_seriesList.size(); i++)
        m_seriesList.at(i)->d_func()->markItemLabelDirty();
}

QAbstract3DAxis *QQuickGraphsItem::createDefaultAxis(QAbstract3DAxis::AxisOrientation orientation)
{
    Q_UNUSED(orientation);

    // The default default axis is a value axis. If the graph type has a different
    // default axis for some orientation, this function needs to be overridden.
    QAbstract3DAxis *defaultAxis = createDefaultValueAxis();
    return defaultAxis;
}

QValue3DAxis *QQuickGraphsItem::createDefaultValueAxis()
{
    // Default value axis has single segment, empty label format, and auto scaling
    QValue3DAxis *defaultAxis = new QValue3DAxis;
    defaultAxis->d_func()->setDefaultAxis(true);

    return defaultAxis;
}

QCategory3DAxis *QQuickGraphsItem::createDefaultCategoryAxis()
{
    // Default category axis has no labels
    QCategory3DAxis *defaultAxis = new QCategory3DAxis;
    defaultAxis->d_func()->setDefaultAxis(true);
    return defaultAxis;
}

void QQuickGraphsItem::setAxisHelper(QAbstract3DAxis::AxisOrientation orientation,
                                     QAbstract3DAxis *axis,
                                     QAbstract3DAxis **axisPtr)
{
    // Setting null axis indicates using default axis
    if (!axis)
        axis = createDefaultAxis(orientation);

    // If old axis is default axis, delete it
    QAbstract3DAxis *oldAxis = *axisPtr;
    if (oldAxis) {
        if (oldAxis->d_func()->isDefaultAxis()) {
            m_axes.removeAll(oldAxis);
            delete oldAxis;
            oldAxis = 0;
        } else {
            // Disconnect the old axis from use
            QObject::disconnect(oldAxis, 0, this, 0);
            oldAxis->d_func()->setOrientation(QAbstract3DAxis::AxisOrientation::None);
        }
    }

    // Assume ownership
    addAxis(axis);

    // Connect the new axis
    *axisPtr = axis;

    axis->d_func()->setOrientation(orientation);

    QObject::connect(axis,
                     &QAbstract3DAxis::titleChanged,
                     this,
                     &QQuickGraphsItem::handleAxisTitleChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::labelsChanged,
                     this,
                     &QQuickGraphsItem::handleAxisLabelsChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::rangeChanged,
                     this,
                     &QQuickGraphsItem::handleAxisRangeChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::autoAdjustRangeChanged,
                     this,
                     &QQuickGraphsItem::handleAxisAutoAdjustRangeChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::labelAutoRotationChanged,
                     this,
                     &QQuickGraphsItem::handleAxisLabelAutoRotationChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::titleVisibilityChanged,
                     this,
                     &QQuickGraphsItem::handleAxisTitleVisibilityChanged);
    QObject::connect(axis,
                     &QAbstract3DAxis::titleFixedChanged,
                     this,
                     &QQuickGraphsItem::handleAxisTitleFixedChanged);

    if (orientation == QAbstract3DAxis::AxisOrientation::X)
        m_changeTracker.axisXTypeChanged = true;
    else if (orientation == QAbstract3DAxis::AxisOrientation::Y)
        m_changeTracker.axisYTypeChanged = true;
    else if (orientation == QAbstract3DAxis::AxisOrientation::Z)
        m_changeTracker.axisZTypeChanged = true;

    handleAxisTitleChangedBySender(axis);
    handleAxisLabelsChangedBySender(axis);
    handleAxisRangeChangedBySender(axis);
    handleAxisAutoAdjustRangeChangedInOrientation(axis->orientation(), axis->isAutoAdjustRange());
    handleAxisLabelAutoRotationChangedBySender(axis);
    handleAxisTitleVisibilityChangedBySender(axis);
    handleAxisTitleFixedChangedBySender(axis);

    if (axis->type() == QAbstract3DAxis::AxisType::Value) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
        QObject::connect(valueAxis,
                         &QValue3DAxis::segmentCountChanged,
                         this,
                         &QQuickGraphsItem::handleAxisSegmentCountChanged);
        QObject::connect(valueAxis,
                         &QValue3DAxis::subSegmentCountChanged,
                         this,
                         &QQuickGraphsItem::handleAxisSubSegmentCountChanged);
        QObject::connect(valueAxis,
                         &QValue3DAxis::labelFormatChanged,
                         this,
                         &QQuickGraphsItem::handleAxisLabelFormatChanged);
        QObject::connect(valueAxis,
                         &QValue3DAxis::reversedChanged,
                         this,
                         &QQuickGraphsItem::handleAxisReversedChanged);
        // TODO: Handle this somehow (add API to QValue3DAxis?)
        //        QObject::connect(valueAxis->d_func(), &QValue3DAxisPrivate::formatterDirty,
        //                         this, &Abstract3DController::handleAxisFormatterDirty);

        handleAxisSegmentCountChangedBySender(valueAxis);
        handleAxisSubSegmentCountChangedBySender(valueAxis);
        handleAxisLabelFormatChangedBySender(valueAxis);
        handleAxisReversedChangedBySender(valueAxis);
        // TODO: Handle this somehow (add API to QValue3DAxis?)
        //        handleAxisFormatterDirtyBySender(valueAxis->d_func());

        valueAxis->formatter()->setLocale(m_locale);
    }
}

void QQuickGraphsItem::startRecordingRemovesAndInserts()
{
    // Default implementation does nothing
}

int QQuickGraphsItem::horizontalFlipFactor() const
{
    return m_horizontalFlipFactor;
}

void QQuickGraphsItem::setHorizontalFlipFactor(int newHorizontalFlipFactor)
{
    m_gridUpdate = true;
    m_horizontalFlipFactor = newHorizontalFlipFactor;
}

void QQuickGraphsItem::emitNeedRender()
{
    if (!m_renderPending) {
        emit needRender();
        m_renderPending = true;
    }
}

void QQuickGraphsItem::handleThemeColorStyleChanged(Q3DTheme::ColorStyle style)
{
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.colorStyleOverride) {
            series->setColorStyle(style);
            series->d_func()->m_themeTracker.colorStyleOverride = false;
        }
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeBaseColorsChanged(const QList<QColor> &colors)
{
    int colorIdx = 0;
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.baseColorOverride) {
            series->setBaseColor(colors.at(colorIdx));
            series->d_func()->m_themeTracker.baseColorOverride = false;
        }
        if (++colorIdx >= colors.size())
            colorIdx = 0;
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeBaseGradientsChanged(const QList<QLinearGradient> &gradients)
{
    int gradientIdx = 0;
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.baseGradientOverride) {
            series->setBaseGradient(gradients.at(gradientIdx));
            series->d_func()->m_themeTracker.baseGradientOverride = false;
        }
        if (++gradientIdx >= gradients.size())
            gradientIdx = 0;
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeSingleHighlightColorChanged(const QColor &color)
{
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.singleHighlightColorOverride) {
            series->setSingleHighlightColor(color);
            series->d_func()->m_themeTracker.singleHighlightColorOverride = false;
        }
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeSingleHighlightGradientChanged(const QLinearGradient &gradient)
{
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.singleHighlightGradientOverride) {
            series->setSingleHighlightGradient(gradient);
            series->d_func()->m_themeTracker.singleHighlightGradientOverride = false;
        }
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeMultiHighlightColorChanged(const QColor &color)
{
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.multiHighlightColorOverride) {
            series->setMultiHighlightColor(color);
            series->d_func()->m_themeTracker.multiHighlightColorOverride = false;
        }
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeMultiHighlightGradientChanged(const QLinearGradient &gradient)
{
    // Set value for series that have not explicitly set this value
    for (QAbstract3DSeries *series : m_seriesList) {
        if (!series->d_func()->m_themeTracker.multiHighlightGradientOverride) {
            series->setMultiHighlightGradient(gradient);
            series->d_func()->m_themeTracker.multiHighlightGradientOverride = false;
        }
    }
    markSeriesVisualsDirty();
}

void QQuickGraphsItem::handleThemeTypeChanged(Q3DTheme::Theme theme)
{
    Q_UNUSED(theme);

    // Changing theme type is logically equivalent of changing the entire theme
    // object, so reset all attached series to the new theme.
    bool force = m_qml->isReady();
    Q3DTheme *activeTheme = m_themeManager->activeTheme();
    for (int i = 0; i < m_seriesList.size(); i++)
        m_seriesList.at(i)->d_func()->resetToTheme(*activeTheme, i, force);

    markSeriesVisualsDirty();

    emit themeTypeChanged();
}

void QQuickGraphsItem::addSeriesInternal(QAbstract3DSeries *series)
{
    insertSeries(m_seriesList.size(), series);
}

void QQuickGraphsItem::insertSeries(int index, QAbstract3DSeries *series)
{
    if (series) {
        if (m_seriesList.contains(series)) {
            int oldIndex = m_seriesList.indexOf(series);
            if (index != oldIndex) {
                m_seriesList.removeOne(series);
                if (oldIndex < index)
                    index--;
                m_seriesList.insert(index, series);
            }
        } else {
            int oldSize = m_seriesList.size();
            m_seriesList.insert(index, series);
            series->d_func()->setGraph(this);
            QObject::connect(series,
                             &QAbstract3DSeries::visibilityChanged,
                             this,
                             &QQuickGraphsItem::handleSeriesVisibilityChanged);
            series->d_func()->resetToTheme(*m_themeManager->activeTheme(), oldSize, false);
        }
        if (series->isVisible())
            handleSeriesVisibilityChangedBySender(series);
    }
}

void QQuickGraphsItem::removeSeriesInternal(QAbstract3DSeries *series)
{
    if (series && series->d_func()->m_graph == this) {
        m_seriesList.removeAll(series);
        QObject::disconnect(series,
                            &QAbstract3DSeries::visibilityChanged,
                            this,
                            &QQuickGraphsItem::handleSeriesVisibilityChanged);
        series->d_func()->setGraph(0);
        m_isDataDirty = true;
        m_isSeriesVisualsDirty = true;
        emitNeedRender();
    }
}

QList<QAbstract3DSeries *> QQuickGraphsItem::seriesList()
{
    return m_seriesList;
}

void QQuickGraphsItem::setAxisX(QAbstract3DAxis *axis)
{
    // Setting null axis will always create new default axis
    if (!axis || axis != m_axisX) {
        setAxisHelper(QAbstract3DAxis::AxisOrientation::X, axis, &m_axisX);
        emit axisXChanged(m_axisX);
    }
}

QAbstract3DAxis *QQuickGraphsItem::axisX() const
{
    return m_axisX;
}

void QQuickGraphsItem::setAxisY(QAbstract3DAxis *axis)
{
    // Setting null axis will always create new default axis
    if (!axis || axis != m_axisY) {
        setAxisHelper(QAbstract3DAxis::AxisOrientation::Y, axis, &m_axisY);
        emit axisYChanged(m_axisY);
    }
}

QAbstract3DAxis *QQuickGraphsItem::axisY() const
{
    return m_axisY;
}

void QQuickGraphsItem::setAxisZ(QAbstract3DAxis *axis)
{
    // Setting null axis will always create new default axis
    if (!axis || axis != m_axisZ) {
        setAxisHelper(QAbstract3DAxis::AxisOrientation::Z, axis, &m_axisZ);
        emit axisZChanged(m_axisZ);
    }
}

QAbstract3DAxis *QQuickGraphsItem::axisZ() const
{
    return m_axisZ;
}

void QQuickGraphsItem::addAxis(QAbstract3DAxis *axis)
{
    Q_ASSERT(axis);
    QQuickGraphsItem *owner = qobject_cast<QQuickGraphsItem *>(axis->parent());
    if (owner != this) {
        Q_ASSERT_X(!owner, "addAxis", "Axis already attached to a graph.");
        axis->setParent(this);
    }
    if (!m_axes.contains(axis))
        m_axes.append(axis);
}

void QQuickGraphsItem::releaseAxis(QAbstract3DAxis *axis)
{
    if (axis && m_axes.contains(axis)) {
        // Clear the default status from released default axes
        if (axis->d_func()->isDefaultAxis())
            axis->d_func()->setDefaultAxis(false);

        // If the axis is in use, replace it with a temporary one
        switch (axis->orientation()) {
        case QAbstract3DAxis::AxisOrientation::X:
            setAxisX(0);
            break;
        case QAbstract3DAxis::AxisOrientation::Y:
            setAxisY(0);
            break;
        case QAbstract3DAxis::AxisOrientation::Z:
            setAxisZ(0);
            break;
        default:
            break;
        }

        m_axes.removeAll(axis);
        axis->setParent(0);
    }
}

QList<QAbstract3DAxis *> QQuickGraphsItem::axes() const
{
    return m_axes;
}

void QQuickGraphsItem::setRenderingMode(QAbstract3DGraph::RenderingMode mode)
{
    if (mode == m_renderMode)
        return;

    QAbstract3DGraph::RenderingMode previousMode = m_renderMode;

    m_renderMode = mode;

    m_initialisedSize = QSize(0, 0);
    setFlag(ItemHasContents /*, !m_runningInDesigner*/);

    // TODO - Need to check if the mode is set properly
    switch (mode) {
    case QAbstract3DGraph::RenderingMode::DirectToBackground:
        update();
        setRenderMode(QQuick3DViewport::Underlay);
        if (previousMode == QAbstract3DGraph::RenderingMode::Indirect) {
            checkWindowList(window());
            setAntialiasing(m_windowSamples > 0);
            if (m_windowSamples != m_samples)
                emit msaaSamplesChanged(m_windowSamples);
        }
        break;
    case QAbstract3DGraph::RenderingMode::Indirect:
        update();
        setRenderMode(QQuick3DViewport::Offscreen);
        break;
    }

    updateWindowParameters();

    emit renderingModeChanged(mode);
}

QAbstract3DGraph::RenderingMode QQuickGraphsItem::renderingMode() const
{
    return m_renderMode;
}

void QQuickGraphsItem::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
    setFlag(ItemHasContents);
    update();
}

bool QQuickGraphsItem::handleMousePressedEvent(QMouseEvent *event)
{
    if (Qt::LeftButton == event->button()) {
        if (scene()->isSlicingActive()) {
            m_sliceActivatedChanged = true;
            return false;
        }
        checkSliceEnabled();
    }
    return true;
}

bool QQuickGraphsItem::handleTouchEvent(QTouchEvent *event)
{
    if (!event->isUpdateEvent()) {
        if (scene()->isSlicingActive()) {
            m_sliceActivatedChanged = true;
            return false;
        }
        checkSliceEnabled();
    }
    return true;
}

void QQuickGraphsItem::checkSliceEnabled()
{
    if (selectionMode().testFlag(QAbstract3DGraph::SelectionSlice)
        && (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn)
            != selectionMode().testFlag(QAbstract3DGraph::SelectionRow))) {
        m_sliceEnabled = true;
    } else {
        m_sliceEnabled = false;
    }
}

void QQuickGraphsItem::handleThemeTypeChange() {}

void QQuickGraphsItem::handleFpsChanged()
{
    int fps = renderStats()->fps();
    if (m_currentFps != fps) {
        m_currentFps = fps;
        emit currentFpsChanged(fps);
    }
}

void QQuickGraphsItem::handleParentWidthChange()
{
    if (m_sliceView->isVisible())
        setWidth(parentItem()->width() * .2f);
    else
        setWidth(parentItem()->width());

    if (m_sliceView && isSliceOrthoProjection()) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(
            m_sliceView->camera());
        const float magnificationScaleFactor = .16f; // this controls the size of the slice view
        const float magnification = scale * magnificationScaleFactor;
        camera->setHorizontalMagnification(magnification);
        camera->setVerticalMagnification(magnification);
    }
}

void QQuickGraphsItem::handleParentHeightChange()
{
    if (m_sliceView->isVisible())
        setHeight(parentItem()->height() * .2f);
    else
        setHeight(parentItem()->height());

    if (m_sliceView && isSliceOrthoProjection()) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(
            m_sliceView->camera());
        const float magnificationScaleFactor = .16f; // this controls the size of the slice view
        const float magnification = scale * magnificationScaleFactor;
        camera->setHorizontalMagnification(magnification);
        camera->setVerticalMagnification(magnification);
    }
}

void QQuickGraphsItem::componentComplete()
{
    QQuick3DViewport::componentComplete();

    auto url = QUrl(QStringLiteral("defaultMeshes/backgroundMesh"));
    m_background = new QQuick3DModel();
    m_backgroundScale = new QQuick3DNode();
    m_backgroundRotation = new QQuick3DNode();
    m_graphNode = new QQuick3DNode();

    m_backgroundScale->setParent(rootNode());
    m_backgroundScale->setParentItem(rootNode());

    m_backgroundRotation->setParent(m_backgroundScale);
    m_backgroundRotation->setParentItem(m_backgroundScale);

    m_background->setObjectName("Background");
    m_background->setParent(m_backgroundRotation);
    m_background->setParentItem(m_backgroundRotation);

    m_background->setSource(url);

    m_backgroundBB = new QQuick3DModel();
    m_backgroundBB->setObjectName("BackgroundBB");
    m_backgroundBB->setParent(m_background);
    m_backgroundBB->setParentItem(m_background);
    m_backgroundBB->setSource(QUrl(QStringLiteral("defaultMeshes/barMeshFull")));
    m_backgroundBB->setPickable(true);

    m_graphNode->setParent(rootNode());
    m_graphNode->setParentItem(rootNode());

    setUpCamera();
    setUpLight();

    // Create repeaters for each axis X, Y, Z
    m_repeaterX = createRepeater();
    m_repeaterY = createRepeater();
    m_repeaterZ = createRepeater();

    m_delegateModelX.reset(new QQmlComponent(qmlEngine(this), (QStringLiteral(":/axis/AxisLabel"))));
    m_delegateModelY.reset(new QQmlComponent(qmlEngine(this), (QStringLiteral(":/axis/AxisLabel"))));
    m_delegateModelZ.reset(new QQmlComponent(qmlEngine(this), (QStringLiteral(":/axis/AxisLabel"))));

    m_repeaterX->setDelegate(m_delegateModelX.get());
    m_repeaterY->setDelegate(m_delegateModelY.get());
    m_repeaterZ->setDelegate(m_delegateModelZ.get());

    // title labels for axes
    m_titleLabelX = createTitleLabel();
    m_titleLabelX->setVisible(axisX()->isTitleVisible());
    m_titleLabelX->setProperty("labelText", axisX()->title());

    m_titleLabelY = createTitleLabel();
    m_titleLabelY->setVisible(axisY()->isTitleVisible());
    m_titleLabelY->setProperty("labelText", axisY()->title());

    m_titleLabelZ = createTitleLabel();
    m_titleLabelZ->setVisible(axisZ()->isTitleVisible());
    m_titleLabelZ->setProperty("labelText", axisZ()->title());

    // Grid with geometry
    m_gridGeometryModel = new QQuick3DModel(m_graphNode);
    m_gridGeometryModel->setCastsShadows(false);
    m_gridGeometryModel->setReceivesShadows(false);
    auto gridGeometry = new QQuick3DGeometry(m_gridGeometryModel);
    gridGeometry->setStride(sizeof(QVector3D));
    gridGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                               0,
                               QQuick3DGeometry::Attribute::F32Type);
    m_gridGeometryModel->setGeometry(gridGeometry);
    QQmlListReference gridMaterialRef(m_gridGeometryModel, "materials");
    auto gridMaterial = new QQuick3DPrincipledMaterial(m_gridGeometryModel);
    gridMaterial->setLighting(QQuick3DPrincipledMaterial::Lighting::NoLighting);
    gridMaterial->setCullMode(QQuick3DMaterial::CullMode::BackFaceCulling);
    gridMaterialRef.append(gridMaterial);

    createItemLabel();

    auto axis = axisX();
    m_repeaterX->setModel(axis->labels().size());
    handleAxisLabelsChangedBySender(axisX());

    axis = axisY();
    m_repeaterY->setModel(2 * axis->labels().size());
    handleAxisLabelsChangedBySender(axisY());

    axis = axisZ();
    m_repeaterZ->setModel(axis->labels().size());
    handleAxisLabelsChangedBySender(axisZ());

    if (!m_pendingCustomItemList.isEmpty()) {
        for (const auto &item : std::as_const(m_pendingCustomItemList))
            addCustomItem(item);
    }

    // Create initial default input handler, if one hasn't already been created by QML
    if (!activeInputHandler()) {
        QAbstract3DInputHandler *inputHandler;
        inputHandler = new QTouch3DInputHandler(this);
        inputHandler->d_func()->m_isDefaultHandler = true;
        setActiveInputHandler(inputHandler);
    }
}

QQuick3DDirectionalLight *QQuickGraphsItem::light() const
{
    return m_light;
}

bool QQuickGraphsItem::isSlicingActive() const
{
    return m_scene->isSlicingActive();
}

void QQuickGraphsItem::setSlicingActive(bool isSlicing)
{
    m_scene->setSlicingActive(isSlicing);
}

bool QQuickGraphsItem::isCustomLabelItem(QCustom3DItem *item) const
{
    return item->d_func()->m_isLabelItem;
}

bool QQuickGraphsItem::isCustomVolumeItem(QCustom3DItem *item) const
{
    return item->d_func()->m_isVolumeItem;
}

QImage QQuickGraphsItem::customTextureImage(QCustom3DItem *item)
{
    return item->d_func()->textureImage();
}

Q3DScene *QQuickGraphsItem::scene()
{
    return m_scene;
}

void QQuickGraphsItem::addTheme(Q3DTheme *theme)
{
    m_themeManager->addTheme(theme);
}

void QQuickGraphsItem::releaseTheme(Q3DTheme *theme)
{
    Q3DTheme *oldTheme = m_themeManager->activeTheme();

    m_themeManager->releaseTheme(theme);

    if (oldTheme != m_themeManager->activeTheme())
        emit activeThemeChanged(m_themeManager->activeTheme());
}

QList<Q3DTheme *> QQuickGraphsItem::themes() const
{
    return m_themeManager->themes();
}

void QQuickGraphsItem::setTheme(Q3DTheme *theme)
{
    if (theme != m_themeManager->activeTheme()) {
        m_themeManager->setActiveTheme(theme);
        m_changeTracker.themeChanged = true;
        // Default theme can be created by theme manager, so ensure we have correct theme
        Q3DTheme *newActiveTheme = m_themeManager->activeTheme();
        // Reset all attached series to the new theme
        for (int i = 0; i < m_seriesList.size(); i++)
            m_seriesList.at(i)->d_func()->resetToTheme(*newActiveTheme, i, isComponentComplete());
        markSeriesVisualsDirty();
        emit activeThemeChanged(newActiveTheme);
    }
}

Q3DTheme *QQuickGraphsItem::theme() const
{
    return m_themeManager->activeTheme();
}

bool QQuickGraphsItem::hasSeries(QAbstract3DSeries *series)
{
    return m_seriesList.contains(series);
}

void QQuickGraphsItem::setSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    if (mode != m_selectionMode) {
        m_selectionMode = mode;
        m_changeTracker.selectionModeChanged = true;
        emit selectionModeChanged(mode);
        emitNeedRender();
    }
}

QAbstract3DGraph::SelectionFlags QQuickGraphsItem::selectionMode() const
{
    return m_selectionMode;
}

void QQuickGraphsItem::doSetShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    if (quality != m_shadowQuality) {
        m_shadowQuality = quality;
        m_changeTracker.shadowQualityChanged = true;
        emit shadowQualityChanged(m_shadowQuality);
        emitNeedRender();
    }
}

void QQuickGraphsItem::setShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    if (!m_useOrthoProjection)
        doSetShadowQuality(quality);
}

QAbstract3DGraph::ShadowQuality QQuickGraphsItem::shadowQuality() const
{
    return m_shadowQuality;
}

int QQuickGraphsItem::addCustomItem(QCustom3DItem *item)
{
    if (isComponentComplete()) {
        if (isCustomLabelItem(item)) {
            QQuick3DNode *label = createTitleLabel();
            QCustom3DLabel *key = static_cast<QCustom3DLabel *>(item);
            m_customLabelList.insert(key, label);
        } else if (isCustomVolumeItem(item)) {
            QQuick3DModel *model = new QQuick3DModel();
            model->setParent(graphNode());
            model->setParentItem(graphNode());
            m_customItemList.insert(item, model);
        } else {
            QQuick3DModel *model = new QQuick3DModel();
            model->setParent(graphNode());
            model->setParentItem(graphNode());
            QQmlListReference materialsRef(model, "materials");
            QQuick3DPrincipledMaterial *material = new QQuick3DPrincipledMaterial();
            material->setParent(model);
            material->setParentItem(model);
            materialsRef.append(material);
            if (!selectionMode().testFlag(QAbstract3DGraph::SelectionNone))
                model->setPickable(true);
            m_customItemList.insert(item, model);
        }
    } else {
        m_pendingCustomItemList.append(item);
    }

    if (!item)
        return -1;

    int index = m_customItems.indexOf(item);

    if (index != -1)
        return index;

    item->setParent(this);
    connect(item, &QCustom3DItem::needUpdate, this, &QQuickGraphsItem::updateCustomItem);
    m_customItems.append(item);
    item->d_func()->resetDirtyBits();
    m_isCustomDataDirty = true;
    emitNeedRender();
    return m_customItems.size() - 1;
}

void QQuickGraphsItem::deleteCustomItems()
{
    for (QCustom3DItem *item : m_customItems)
        delete item;
    m_customItems.clear();
    m_isCustomDataDirty = true;
    emitNeedRender();
}

void QQuickGraphsItem::deleteCustomItem(QCustom3DItem *item)
{
    if (!item)
        return;

    m_customItems.removeOne(item);
    delete item;
    item = 0;
    m_isCustomDataDirty = true;
    emitNeedRender();
}

void QQuickGraphsItem::deleteCustomItem(const QVector3D &position)
{
    // Get the item for the position
    for (QCustom3DItem *item : m_customItems) {
        if (item->position() == position)
            deleteCustomItem(item);
    }
}

QList<QCustom3DItem *> QQuickGraphsItem::customItems() const
{
    return m_customItems;
}

void QQuickGraphsItem::updateCustomItem()
{
    m_isCustomItemDirty = true;
    m_isCustomDataDirty = true;
    emitNeedRender();
}

void QQuickGraphsItem::removeCustomItems()
{
    m_customItemList.clear();
    m_customLabelList.clear();
    deleteCustomItems();
}

void QQuickGraphsItem::removeCustomItem(QCustom3DItem *item)
{
    if (isCustomLabelItem(item)) {
        m_customLabelList.remove(static_cast<QCustom3DLabel *>(item));
    } else if (isCustomVolumeItem(item)) {
        m_customItemList.remove(item);
        auto volume = static_cast<QCustom3DVolume *>(item);
        if (m_customVolumes.contains(volume)) {
            m_customVolumes[volume].model->deleteLater();
            m_customVolumes.remove(volume);
        }
    } else {
        m_customItemList.remove(item);
    }
    deleteCustomItem(item);
}

void QQuickGraphsItem::removeCustomItemAt(const QVector3D &position)
{
    auto labelIterator = m_customLabelList.constBegin();
    while (labelIterator != m_customLabelList.constEnd()) {
        QCustom3DLabel *label = labelIterator.key();
        if (label->position() == position) {
            labelIterator.value()->setVisible(false);
            labelIterator = m_customLabelList.erase(labelIterator);
        } else {
            ++labelIterator;
        }
    }

    auto itemIterator = m_customItemList.constBegin();
    while (itemIterator != m_customItemList.constEnd()) {
        QCustom3DItem *item = itemIterator.key();
        if (item->position() == position) {
            itemIterator.value()->setVisible(false);
            itemIterator = m_customItemList.erase(itemIterator);
            if (isCustomVolumeItem(item)) {
                auto volume = static_cast<QCustom3DVolume *>(item);
                if (m_customVolumes.contains(volume)) {
                    m_customVolumes[volume].model->deleteLater();
                    m_customVolumes.remove(volume);
                }
            }
        } else {
            ++itemIterator;
        }
    }
    deleteCustomItem(position);
}

void QQuickGraphsItem::releaseCustomItem(QCustom3DItem *item)
{
    if (isCustomLabelItem(item)) {
        m_customLabelList.remove(static_cast<QCustom3DLabel *>(item));
    } else if (isCustomVolumeItem(item)) {
        m_customItemList.remove(item);
        auto volume = static_cast<QCustom3DVolume *>(item);
        if (m_customVolumes.contains(volume)) {
            m_customVolumes[volume].model->deleteLater();
            m_customVolumes.remove(volume);
        }
    } else {
        m_customItemList.remove(item);
    }

    if (item && m_customItems.contains(item)) {
        disconnect(item, &QCustom3DItem::needUpdate, this, &QQuickGraphsItem::updateCustomItem);
        m_customItems.removeOne(item);
        item->setParent(0);
        m_isCustomDataDirty = true;
        emitNeedRender();
    }
}

int QQuickGraphsItem::selectedLabelIndex() const
{
    int index = m_selectedLabelIndex;
    QAbstract3DAxis *axis = selectedAxis();
    if (axis && axis->labels().size() <= index)
        index = -1;
    return index;
}

QAbstract3DAxis *QQuickGraphsItem::selectedAxis() const
{
    QAbstract3DAxis *axis = 0;
    QAbstract3DGraph::ElementType type = m_clickedType;
    switch (type) {
    case QAbstract3DGraph::ElementType::AxisXLabel:
        axis = axisX();
        break;
    case QAbstract3DGraph::ElementType::AxisYLabel:
        axis = axisY();
        break;
    case QAbstract3DGraph::ElementType::AxisZLabel:
        axis = axisZ();
        break;
    default:
        axis = 0;
        break;
    }

    return axis;
}

int QQuickGraphsItem::selectedCustomItemIndex() const
{
    int index = m_selectedCustomItemIndex;
    if (m_customItems.size() <= index)
        index = -1;
    return index;
}

QCustom3DItem *QQuickGraphsItem::selectedCustomItem() const
{
    QCustom3DItem *item = 0;
    int index = selectedCustomItemIndex();
    if (index >= 0)
        item = m_customItems[index];
    return item;
}

QQmlListProperty<QCustom3DItem> QQuickGraphsItem::customItemList()
{
    return QQmlListProperty<QCustom3DItem>(this,
                                           this,
                                           &QQuickGraphsItem::appendCustomItemFunc,
                                           &QQuickGraphsItem::countCustomItemFunc,
                                           &QQuickGraphsItem::atCustomItemFunc,
                                           &QQuickGraphsItem::clearCustomItemFunc);
}

void QQuickGraphsItem::appendCustomItemFunc(QQmlListProperty<QCustom3DItem> *list,
                                            QCustom3DItem *item)
{
    QQuickGraphsItem *decl = reinterpret_cast<QQuickGraphsItem *>(list->data);
    decl->addCustomItem(item);
}

qsizetype QQuickGraphsItem::countCustomItemFunc(QQmlListProperty<QCustom3DItem> *list)
{
    Q_UNUSED(list);
    return reinterpret_cast<QQuickGraphsItem *>(list->data)->m_customItems.size();
}

QCustom3DItem *QQuickGraphsItem::atCustomItemFunc(QQmlListProperty<QCustom3DItem> *list,
                                                  qsizetype index)
{
    Q_UNUSED(list);
    Q_UNUSED(index);
    return reinterpret_cast<QQuickGraphsItem *>(list->data)->m_customItems.at(index);
}

void QQuickGraphsItem::clearCustomItemFunc(QQmlListProperty<QCustom3DItem> *list)
{
    QQuickGraphsItem *decl = reinterpret_cast<QQuickGraphsItem *>(list->data);
    decl->removeCustomItems();
}

void QQuickGraphsItem::synchData()
{
    if (!isVisible())
        return;

    m_renderPending = false;

    if (m_changeTracker.selectionModeChanged) {
        updateSelectionMode(selectionMode());
        m_changeTracker.selectionModeChanged = false;
    }

    bool recalculateScale = false;
    if (m_changeTracker.aspectRatioChanged) {
        recalculateScale = true;
        m_changeTracker.aspectRatioChanged = false;
    }

    if (m_changeTracker.horizontalAspectRatioChanged) {
        recalculateScale = true;
        m_changeTracker.horizontalAspectRatioChanged = false;
    }

    if (m_changeTracker.marginChanged) {
        recalculateScale = true;
        m_changeTracker.marginChanged = false;
    }

    if (m_changeTracker.polarChanged) {
        recalculateScale = true;
        m_changeTracker.polarChanged = false;
    }

    if (recalculateScale)
        calculateSceneScalingFactors();

    bool axisDirty = recalculateScale;
    if (m_changeTracker.axisXFormatterChanged) {
        m_changeTracker.axisXFormatterChanged = false;
        if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX());
            valueAxisX->recalculate();
            repeaterX()->setModel(valueAxisX->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_changeTracker.axisYFormatterChanged) {
        m_changeTracker.axisYFormatterChanged = false;
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY());
            valueAxisY->recalculate();
            repeaterY()->setModel(2 * valueAxisY->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_changeTracker.axisZFormatterChanged) {
        m_changeTracker.axisZFormatterChanged = false;
        if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ());
            valueAxisZ->recalculate();
            repeaterZ()->setModel(valueAxisZ->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_changeTracker.axisXSegmentCountChanged) {
        if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX());
            valueAxisX->recalculate();
        }
        m_changeTracker.axisXSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisYSegmentCountChanged) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY());
            valueAxisY->recalculate();
        }
        m_changeTracker.axisYSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisZSegmentCountChanged) {
        if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ());
            valueAxisZ->recalculate();
        }
        m_changeTracker.axisZSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisXSubSegmentCountChanged) {
        if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX());
            valueAxisX->recalculate();
        }
        m_changeTracker.axisXSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisYSubSegmentCountChanged) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY());
            valueAxisY->recalculate();
        }
        m_changeTracker.axisYSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisZSubSegmentCountChanged) {
        if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ());
            valueAxisZ->recalculate();
        }
        m_changeTracker.axisZSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_changeTracker.axisXLabelsChanged) {
        if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
            auto valueAxisX = static_cast<QValue3DAxis *>(axisX());
            valueAxisX->recalculate();
            repeaterX()->setModel(valueAxisX->formatter()->labelPositions().size());
        } else if (axisX()->type() == QAbstract3DAxis::AxisType::Category) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisX());
            repeaterX()->setModel(categoryAxis->labels().size());
        }

        m_changeTracker.axisXLabelsChanged = false;
        handleLabelCountChanged(m_repeaterX);
        axisDirty = true;
    }

    if (m_changeTracker.axisYLabelsChanged) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            auto valueAxisY = static_cast<QValue3DAxis *>(axisY());
            valueAxisY->recalculate();
            repeaterY()->setModel(2 * valueAxisY->formatter()->labelPositions().size());
        } else if (axisY()->type() == QAbstract3DAxis::AxisType::Category) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisY());
            repeaterY()->setModel(2 * categoryAxis->labels().size());
        }

        m_changeTracker.axisYLabelsChanged = false;
        handleLabelCountChanged(m_repeaterY);
        axisDirty = true;
    }

    if (m_changeTracker.axisZLabelsChanged) {
        if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
            auto valueAxisZ = static_cast<QValue3DAxis *>(axisZ());
            valueAxisZ->recalculate();
            repeaterZ()->setModel(valueAxisZ->formatter()->labelPositions().size());
        } else if (axisZ()->type() == QAbstract3DAxis::AxisType::Category) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisZ());
            repeaterZ()->setModel(categoryAxis->labels().size());
        }

        m_changeTracker.axisZLabelsChanged = false;
        handleLabelCountChanged(m_repeaterZ);
        axisDirty = true;
    }

    updateTitleLabels();

    if (m_changeTracker.shadowQualityChanged) {
        updateShadowQuality(shadowQuality());
        m_changeTracker.shadowQualityChanged = false;
    }

    if (m_changeTracker.axisXRangeChanged) {
        axisDirty = true;
        calculateSceneScalingFactors();
        m_changeTracker.axisXRangeChanged = false;
    }

    if (m_changeTracker.axisYRangeChanged) {
        axisDirty = true;
        QAbstract3DAxis *axis = axisY();
        updateAxisRange(axis->min(), axis->max());
        calculateSceneScalingFactors();
        m_changeTracker.axisYRangeChanged = false;
    }

    if (m_changeTracker.axisZRangeChanged) {
        axisDirty = true;
        calculateSceneScalingFactors();
        m_changeTracker.axisZRangeChanged = false;
    }

    if (m_changeTracker.axisYReversedChanged) {
        m_changeTracker.axisYReversedChanged = false;
        if (m_axisY->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(m_axisY);
            updateAxisReversed(valueAxisY->reversed());
        }
    }

    if (m_changeTracker.axisXLabelAutoRotationChanged) {
        axisDirty = true;
        m_changeTracker.axisXLabelAutoRotationChanged = false;
    }

    if (m_changeTracker.axisYLabelAutoRotationChanged) {
        axisDirty = true;
        m_changeTracker.axisYLabelAutoRotationChanged = false;
    }

    if (m_changeTracker.axisZLabelAutoRotationChanged) {
        axisDirty = true;
        m_changeTracker.axisZLabelAutoRotationChanged = false;
    }

    if (m_changeTracker.axisXTitleFixedChanged) {
        axisDirty = true;
        m_changeTracker.axisXTitleFixedChanged = false;
    }

    if (m_changeTracker.axisYTitleFixedChanged) {
        axisDirty = true;
        m_changeTracker.axisYTitleFixedChanged = false;
    }

    if (m_changeTracker.axisZTitleFixedChanged) {
        axisDirty = true;
        m_changeTracker.axisZTitleFixedChanged = false;
    }

    updateCamera();

    QVector3D forward = camera()->forward();
    auto targetRotation = cameraTarget()->eulerRotation();
    if (m_yFlipped != (targetRotation.x() > 0)) {
        m_yFlipped = (targetRotation.x() > 0);
        axisDirty = true;
    }
    if (m_xFlipped != (forward.x() > 0)) {
        m_xFlipped = (forward.x() > 0);
        axisDirty = true;
    }
    if (m_zFlipped != ((forward.z() > .1f))) {
        m_zFlipped = ((forward.z() > .1f));
        axisDirty = true;
    }

    if (axisDirty) {
        updateGrid();
        updateLabels();
        updateCustomData();
        if (m_sliceView && isSliceEnabled()) {
            updateSliceGrid();
            updateSliceLabels();
        }
        m_gridUpdated = true;
    }

    if (m_changeTracker.radialLabelOffsetChanged) {
        updateRadialLabelOffset();
        m_changeTracker.radialLabelOffsetChanged = false;
    }

    QMatrix4x4 modelMatrix;
    m_backgroundScale->setScale(m_scaleWithBackground + m_backgroundScaleMargin);

    QVector3D rotVec;
    if (!m_yFlipped) {
        rotVec = QVector3D(0, 270, 0);
        if (m_xFlipped && m_zFlipped)
            rotVec.setY(90);
        else if (!m_xFlipped && m_zFlipped)
            rotVec.setY(0);
        else if (m_xFlipped && !m_zFlipped)
            rotVec.setY(180);
    } else {
        rotVec = QVector3D(0, 180, 180);
        if (m_xFlipped && m_zFlipped)
            rotVec.setY(0);
        else if (!m_xFlipped && m_zFlipped)
            rotVec.setY(270);
        else if (m_xFlipped && !m_zFlipped)
            rotVec.setY(90);
    }

    auto rotation = Utils::calculateRotation(rotVec);
    if (m_yFlipped) {
        m_backgroundRotation->setRotation(rotation);
    } else {
        modelMatrix.rotate(rotation);
        m_backgroundRotation->setRotation(rotation);
    }

    if (graphPositionQueryPending())
        graphPositionAt(scene()->graphPositionQuery());

    bool forceUpdateCustomVolumes = false;
    if (m_changeTracker.projectionChanged) {
        forceUpdateCustomVolumes = true;
        bool useOrtho = isOrthoProjection();
        if (useOrtho)
            setCamera(m_oCamera);
        else
            setCamera(m_pCamera);
        m_changeTracker.projectionChanged = false;
    }

    if (m_changeTracker.themeChanged) {
        environment()->setClearColor(theme()->windowColor());
        m_changeTracker.themeChanged = false;
    }

    if (theme()->d_func()->m_dirtyBits.lightStrengthDirty) {
        light()->setBrightness(theme()->lightStrength() * .2f);
        if (qFuzzyIsNull(light()->brightness()))
            light()->setBrightness(.0000001f);
        updateLightStrength();
        theme()->d_func()->m_dirtyBits.lightStrengthDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.ambientLightStrengthDirty) {
        float ambientStrength = theme()->ambientLightStrength();
        QColor ambientColor = QColor::fromRgbF(ambientStrength, ambientStrength, ambientStrength);
        light()->setAmbientColor(ambientColor);
        if (qFuzzyIsNull(light()->brightness()))
            light()->setBrightness(.0000001f);
        theme()->d_func()->m_dirtyBits.ambientLightStrengthDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.lightColorDirty) {
        light()->setColor(theme()->lightColor());
        theme()->d_func()->m_dirtyBits.lightColorDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.shadowStrengthDirty) {
        light()->setShadowFactor(theme()->shadowStrength());
        theme()->d_func()->m_dirtyBits.shadowStrengthDirty = false;
    }

    // label Adjustments
    if (theme()->d_func()->m_dirtyBits.labelBackgroundColorDirty) {
        QColor labelBackgroundColor = theme()->labelBackgroundColor();
        changeLabelBackgroundColor(m_repeaterX, labelBackgroundColor);
        changeLabelBackgroundColor(m_repeaterY, labelBackgroundColor);
        changeLabelBackgroundColor(m_repeaterZ, labelBackgroundColor);
        m_titleLabelX->setProperty("backgroundColor", labelBackgroundColor);
        m_titleLabelY->setProperty("backgroundColor", labelBackgroundColor);
        m_titleLabelZ->setProperty("backgroundColor", labelBackgroundColor);
        m_itemLabel->setProperty("backgroundColor", labelBackgroundColor);

        if (m_sliceView) {
            changeLabelBackgroundColor(m_sliceHorizontalLabelRepeater, labelBackgroundColor);
            changeLabelBackgroundColor(m_sliceVerticalLabelRepeater, labelBackgroundColor);
            m_sliceItemLabel->setProperty("backgroundColor", labelBackgroundColor);
            m_sliceHorizontalTitleLabel->setProperty("backgroundColor", labelBackgroundColor);
            m_sliceVerticalTitleLabel->setProperty("backgroundColor", labelBackgroundColor);
        }
        theme()->d_func()->m_dirtyBits.labelBackgroundColorDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.labelBackgroundEnabledDirty) {
        bool enabled = theme()->isLabelBackgroundEnabled();
        changeLabelBackgroundEnabled(m_repeaterX, enabled);
        changeLabelBackgroundEnabled(m_repeaterY, enabled);
        changeLabelBackgroundEnabled(m_repeaterZ, enabled);
        m_titleLabelX->setProperty("backgroundEnabled", enabled);
        m_titleLabelY->setProperty("backgroundEnabled", enabled);
        m_titleLabelZ->setProperty("backgroundEnabled", enabled);
        m_itemLabel->setProperty("backgroundEnabled", enabled);

        if (m_sliceView) {
            changeLabelBackgroundEnabled(m_sliceHorizontalLabelRepeater, enabled);
            changeLabelBackgroundEnabled(m_sliceVerticalLabelRepeater, enabled);
            m_sliceItemLabel->setProperty("backgroundEnabled", enabled);
            m_sliceHorizontalTitleLabel->setProperty("backgroundEnabled", enabled);
            m_sliceVerticalTitleLabel->setProperty("backgroundEnabled", enabled);
        }
        theme()->d_func()->m_dirtyBits.labelBackgroundEnabledDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.labelBorderEnabledDirty) {
        bool enabled = theme()->isLabelBorderEnabled();
        changeLabelBorderEnabled(m_repeaterX, enabled);
        changeLabelBorderEnabled(m_repeaterY, enabled);
        changeLabelBorderEnabled(m_repeaterZ, enabled);
        m_titleLabelX->setProperty("borderEnabled", enabled);
        m_titleLabelY->setProperty("borderEnabled", enabled);
        m_titleLabelZ->setProperty("borderEnabled", enabled);
        m_itemLabel->setProperty("borderEnabled", enabled);

        if (m_sliceView) {
            changeLabelBorderEnabled(m_sliceHorizontalLabelRepeater, enabled);
            changeLabelBorderEnabled(m_sliceVerticalLabelRepeater, enabled);
            m_sliceItemLabel->setProperty("borderEnabled", enabled);
            m_sliceHorizontalTitleLabel->setProperty("borderEnabled", enabled);
            m_sliceVerticalTitleLabel->setProperty("borderEnabled", enabled);
        }
        theme()->d_func()->m_dirtyBits.labelBorderEnabledDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.labelTextColorDirty) {
        QColor labelTextColor = theme()->labelTextColor();
        changeLabelTextColor(m_repeaterX, labelTextColor);
        changeLabelTextColor(m_repeaterY, labelTextColor);
        changeLabelTextColor(m_repeaterZ, labelTextColor);
        m_titleLabelX->setProperty("labelTextColor", labelTextColor);
        m_titleLabelY->setProperty("labelTextColor", labelTextColor);
        m_titleLabelZ->setProperty("labelTextColor", labelTextColor);
        m_itemLabel->setProperty("labelTextColor", labelTextColor);

        if (m_sliceView && isSliceEnabled()) {
            changeLabelTextColor(m_sliceHorizontalLabelRepeater, labelTextColor);
            changeLabelTextColor(m_sliceVerticalLabelRepeater, labelTextColor);
            m_sliceItemLabel->setProperty("labelTextColor", labelTextColor);
            m_sliceHorizontalTitleLabel->setProperty("labelTextColor", labelTextColor);
            m_sliceVerticalTitleLabel->setProperty("labelTextColor", labelTextColor);
        }
        theme()->d_func()->m_dirtyBits.labelTextColorDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.fontDirty) {
        auto font = theme()->font();
        changeLabelFont(m_repeaterX, font);
        changeLabelFont(m_repeaterY, font);
        changeLabelFont(m_repeaterZ, font);
        m_titleLabelX->setProperty("labelFont", font);
        m_titleLabelY->setProperty("labelFont", font);
        m_titleLabelZ->setProperty("labelFont", font);
        m_itemLabel->setProperty("labelFont", font);
        updateLabels();

        if (m_sliceView && isSliceEnabled()) {
            changeLabelFont(m_sliceHorizontalLabelRepeater, font);
            changeLabelFont(m_sliceVerticalLabelRepeater, font);
            m_sliceItemLabel->setProperty("labelFont", font);
            m_sliceHorizontalTitleLabel->setProperty("labelFont", font);
            m_sliceVerticalTitleLabel->setProperty("labelFont", font);
            updateSliceLabels();
        }
        theme()->d_func()->m_dirtyBits.fontDirty = false;
        m_isSeriesVisualsDirty = true;
    }

    if (theme()->d_func()->m_dirtyBits.labelsEnabledDirty) {
        bool enabled = theme()->isLabelsEnabled();
        changeLabelsEnabled(m_repeaterX, enabled);
        changeLabelsEnabled(m_repeaterY, enabled);
        changeLabelsEnabled(m_repeaterZ, enabled);
        m_titleLabelX->setProperty("visible", enabled && axisX()->isTitleVisible());
        m_titleLabelY->setProperty("visible", enabled && axisY()->isTitleVisible());
        m_titleLabelZ->setProperty("visible", enabled && axisZ()->isTitleVisible());
        m_itemLabel->setProperty("visible", enabled);

        if (m_sliceView) {
            changeLabelsEnabled(m_sliceHorizontalLabelRepeater, enabled);
            changeLabelsEnabled(m_sliceVerticalLabelRepeater, enabled);
            m_sliceItemLabel->setProperty("visible", enabled);
            m_sliceHorizontalTitleLabel->setProperty("visible", enabled);
            m_sliceVerticalTitleLabel->setProperty("visible", enabled);
        }
        theme()->d_func()->m_dirtyBits.labelsEnabledDirty = false;
    }

    // Grid and background adjustments
    if (theme()->d_func()->m_dirtyBits.backgroundColorDirty) {
        QQmlListReference materialsRef(m_background, "materials");
        QQuick3DPrincipledMaterial *bgMat;
        if (!materialsRef.size()) {
            bgMat = new QQuick3DPrincipledMaterial();
            bgMat->setParent(m_background);
            bgMat->setMetalness(0.f);
            bgMat->setRoughness(.3f);
            bgMat->setEmissiveFactor(QVector3D(.001f, .001f, .001f));
            materialsRef.append(bgMat);
        } else {
            bgMat = static_cast<QQuick3DPrincipledMaterial *>(materialsRef.at(0));
        }
        bgMat->setBaseColor(theme()->backgroundColor());
        theme()->d_func()->m_dirtyBits.backgroundColorDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.backgroundEnabledDirty) {
        m_background->setLocalOpacity(theme()->isBackgroundEnabled());
        theme()->d_func()->m_dirtyBits.backgroundEnabledDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.gridEnabledDirty) {
        bool enabled = theme()->isGridEnabled();

        m_gridGeometryModel->setVisible(enabled);
        if (m_sliceView && isSliceEnabled())
            m_sliceGridGeometryModel->setVisible(enabled);

        theme()->d_func()->m_dirtyBits.gridEnabledDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.gridLineColorDirty) {
        QColor gridLineColor = theme()->gridLineColor();

        QQmlListReference materialRef(m_gridGeometryModel, "materials");
        Q_ASSERT(materialRef.size());
        auto *material = static_cast<QQuick3DPrincipledMaterial *>(materialRef.at(0));
        material->setBaseColor(gridLineColor);
        theme()->d_func()->m_dirtyBits.gridLineColorDirty = false;
    }

    if (theme()->d_func()->m_dirtyBits.singleHighlightColorDirty) {
        updateSingleHighlightColor();
        theme()->d_func()->m_dirtyBits.singleHighlightColorDirty = false;
    }

    // Other adjustments
    if (theme()->d_func()->m_dirtyBits.windowColorDirty) {
        window()->setColor(theme()->windowColor());
        environment()->setClearColor(theme()->windowColor());
        theme()->d_func()->m_dirtyBits.windowColorDirty = false;
    }
    if (theme()->windowColor() != window()->color()) {
        window()->setColor(theme()->windowColor());
    }

    if (isCustomDataDirty()) {
        forceUpdateCustomVolumes = true;
        updateCustomData();
        setCustomDataDirty(false);
    }

    if (m_changedSeriesList.size()) {
        forceUpdateCustomVolumes = true;
        updateGraph();
        m_changedSeriesList.clear();
    }

    if (m_isSeriesVisualsDirty) {
        forceUpdateCustomVolumes = true;
        updateGrid();
        updateLabels();
        if (m_sliceView && isSliceEnabled()) {
            updateSliceGrid();
            updateSliceLabels();
        }
        updateGraph();
        m_isSeriesVisualsDirty = false;
    }

    if (m_gridUpdate)
        updateGrid();

    if (m_isDataDirty) {
        forceUpdateCustomVolumes = true;
        updateGraph();
        m_isDataDirty = false;
    }

    if (m_sliceActivatedChanged)
        updateSliceGraph();

    if (isCustomItemDirty() || forceUpdateCustomVolumes)
        updateCustomVolumes();

    if (m_measureFps)
        QQuickItem::update();
}

void QQuickGraphsItem::updateGrid()
{
    int gridLineCountX = 0;
    int subGridLineCountX = 0;
    gridLineCountHelper(axisX(), gridLineCountX, subGridLineCountX);

    int gridLineCountY = 0;
    int subGridLineCountY = 0;
    gridLineCountHelper(axisY(), gridLineCountY, subGridLineCountY);

    int gridLineCountZ = 0;
    int subGridLineCountZ = 0;
    gridLineCountHelper(axisZ(), gridLineCountZ, subGridLineCountZ);

    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    QVector3D scaleX(backgroundScale.x() * lineLengthScaleFactor(),
                     lineWidthScaleFactor(),
                     lineWidthScaleFactor());
    QVector3D scaleY(lineWidthScaleFactor(),
                     backgroundScale.y() * lineLengthScaleFactor(),
                     lineWidthScaleFactor());
    QVector3D scaleZ(backgroundScale.z() * lineLengthScaleFactor(),
                     lineWidthScaleFactor(),
                     lineWidthScaleFactor());

    const bool xFlipped = isXFlipped();
    const bool yFlipped = isYFlipped();
    const bool zFlipped = isZFlipped();

    const float lineOffset = 0.01f;
    const float backOffsetAdjustment = 0.005f;

    QQuaternion lineRotation(.0f, .0f, .0f, .0f);
    QVector3D rotation(90.0f, 0.0f, 0.0f);

    QByteArray vertices;
    int verticalXCount = gridLineCountX + subGridLineCountX;
    int horizontalZCount = gridLineCountZ + subGridLineCountZ;
    int horizontalYCount = gridLineCountY + subGridLineCountY;
    int calculatedSize = 0;

    bool usePolar = isPolar() && (m_graphType != QAbstract3DSeries::SeriesType::Bar);

    if (!usePolar) {
        int factor = m_hasVerticalSegmentLine ? 2 : 1;
        calculatedSize = (factor * verticalXCount + factor * horizontalZCount + 2 * horizontalYCount)
                         * 2 * sizeof(QVector3D);
    } else {
        int radialMainGridSize = static_cast<QValue3DAxis *>(axisZ())->gridSize() * polarRoundness;
        int radialSubGridSize = static_cast<QValue3DAxis *>(axisZ())->subGridSize()
                                * polarRoundness;

        int angularMainGridsize = static_cast<QValue3DAxis *>(axisX())->gridSize();
        int angularSubGridsize = static_cast<QValue3DAxis *>(axisX())->subGridSize();

        int yLines = 2 * horizontalYCount;

        calculatedSize = (radialSubGridSize + radialMainGridSize + angularMainGridsize
                          + angularSubGridsize + yLines - 1)
                         * 2 * sizeof(QVector3D);
    }
    vertices.resize(calculatedSize);
    QVector3D *data = reinterpret_cast<QVector3D *>(vertices.data());

    // Floor horizontal line
    float linePosX = 0.0f;
    float linePosY = backgroundScale.y();
    float linePosZ = 0.0f;
    float scale = m_scaleWithBackground.z();

    float x0 = backgroundScale.x();
    float x1 = -backgroundScale.x();

    float tempLineOffset = -lineOffset;
    if (!yFlipped) {
        linePosY *= -1.0f;
        rotation.setZ(180.0f);
        tempLineOffset *= -1.0f;
    }
    lineRotation = Utils::calculateRotation(rotation);
    linePosY *= m_horizontalFlipFactor;
    tempLineOffset *= m_horizontalFlipFactor;
    if (!usePolar) {
        for (int i = 0; i < subGridLineCountZ; i++) {
            if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ())->subGridPositionAt(i) * -scale
                               * 2.0f
                           + scale;
            } else if (axisZ()->type() == QAbstract3DAxis::AxisType::Category) {
                linePosZ = calculateCategoryGridLinePosition(axisZ(), i);
                linePosY = calculateCategoryGridLinePosition(axisY(), i);
            }

            *data++ = QVector3D(x0, linePosY + tempLineOffset, linePosZ);
            *data++ = QVector3D(x1, linePosY + tempLineOffset, linePosZ);
        }

        for (int i = 0; i < gridLineCountZ; i++) {
            if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ())->gridPositionAt(i) * -scale * 2.0f
                           + scale;
            } else if (axisZ()->type() == QAbstract3DAxis::AxisType::Category) {
                linePosZ = calculateCategoryGridLinePosition(axisZ(), i);
                linePosY = calculateCategoryGridLinePosition(axisY(), i);
            }

            *data++ = QVector3D(x0, linePosY + tempLineOffset, linePosZ);
            *data++ = QVector3D(x1, linePosY + tempLineOffset, linePosZ);
        }
    } else {
        auto valueAxisZ = static_cast<QValue3DAxis *>(axisZ());

        for (int k = 0; k < subGridLineCountZ; k++) {
            float degrees = 0.0f;
            const float r = (m_polarRadius) *valueAxisZ->subGridPositionAt(k);
            QVector3D lastPoint(r * qCos(degrees), linePosY + tempLineOffset, r * qSin(degrees));
            for (int i = 1; i <= polarRoundness; i++) {
                degrees = doublePi * i / polarRoundness;
                const float xPos = qCos(degrees);
                const float zPos = qSin(degrees);

                const QVector3D pos(r * xPos, linePosY + tempLineOffset, r * zPos);
                *data++ = lastPoint;
                *data++ = pos;
                lastPoint = pos;
            }
        }

        for (int k = 0; k < gridLineCountZ; k++) {
            float degrees = 0.0f;
            const float r = (m_polarRadius) *valueAxisZ->gridPositionAt(k);
            QVector3D lastPoint(r * qCos(degrees), linePosY + tempLineOffset, r * qSin(degrees));

            for (int i = 1; i <= polarRoundness; i++) {
                degrees = doublePi * i / polarRoundness;
                const float xPos = qCos(degrees);
                const float zPos = qSin(degrees);

                const QVector3D pos(r * xPos, linePosY + tempLineOffset, r * zPos);
                *data++ = lastPoint;
                *data++ = pos;
                lastPoint = pos;
            }
        }
    }

    // Side vertical line
    linePosX = -backgroundScale.x();
    linePosY = 0.0f;
    rotation = QVector3D(0.0f, 90.0f, 0.0f);

    float y0 = -backgroundScale.y();
    float y1 = backgroundScale.y();

    x0 = -backgroundScale.x();
    x1 = -backgroundScale.x();

    tempLineOffset = lineOffset;

    if (xFlipped) {
        linePosX *= -1.0f;
        rotation.setY(-90.0f);
        tempLineOffset *= -1.0f;
        x0 *= -1.0f;
        x1 *= -1.0f;
    }
    lineRotation = Utils::calculateRotation(rotation);
    if (m_hasVerticalSegmentLine) {
        for (int i = 0; i < subGridLineCountZ; i++) {
            if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ())->subGridPositionAt(i) * scale * 2.0f
                           - scale;
            }

            *data++ = QVector3D(x0 + tempLineOffset, y0, linePosZ);
            *data++ = QVector3D(x1 + tempLineOffset, y1, linePosZ);
        }

        for (int i = 0; i < gridLineCountZ; i++) {
            if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ())->gridPositionAt(i) * scale * 2.0f
                           - scale;
            }

            *data++ = QVector3D(x0 + tempLineOffset, y0, linePosZ);
            *data++ = QVector3D(x1 + tempLineOffset, y1, linePosZ);
        }
    }

    // Side horizontal line
    scale = m_scaleWithBackground.y();
    rotation = QVector3D(180.0f, -90.0f, 0.0f);

    float z0 = backgroundScale.z();
    float z1 = -backgroundScale.z();

    x0 = -backgroundScale.x();
    x1 = -backgroundScale.x();

    tempLineOffset = lineOffset;

    if (xFlipped) {
        rotation.setY(90.0f);
        tempLineOffset *= -1.0f;
        x0 *= -1.0f;
        x1 *= -1.0f;
    }
    lineRotation = Utils::calculateRotation(rotation);
    for (int i = 0; i < gridLineCountY; i++) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            linePosY = static_cast<QValue3DAxis *>(axisY())->gridPositionAt(i) * scale * 2.0f
                       - scale;
        } else if (axisY()->type() == QAbstract3DAxis::AxisType::Category) {
            linePosY = calculateCategoryGridLinePosition(axisY(), i);
        }

        *data++ = QVector3D(x0 + tempLineOffset, linePosY, z0);
        *data++ = QVector3D(x1 + tempLineOffset, linePosY, z1);
    }

    for (int i = 0; i < subGridLineCountY; i++) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            linePosY = static_cast<QValue3DAxis *>(axisY())->subGridPositionAt(i) * scale * 2.0f
                       - scale;
        } else if (axisY()->type() == QAbstract3DAxis::AxisType::Category) {
            linePosY = calculateCategoryGridLinePosition(axisY(), i);
        }

        *data++ = QVector3D(x0 + tempLineOffset, linePosY, z0);
        *data++ = QVector3D(x1 + tempLineOffset, linePosY, z1);
    }

    // Floor vertical line
    linePosY = -backgroundScale.y();
    rotation = QVector3D(-90.0f, 90.0f, 0.0f);

    tempLineOffset = lineOffset;
    z0 = backgroundScale.z();
    z1 = -backgroundScale.z();

    if (yFlipped) {
        linePosY *= -1.0f;
        rotation.setZ(180.0f);
        tempLineOffset *= -1.0f;
    }
    scale = m_scaleWithBackground.x();

    linePosY *= m_horizontalFlipFactor;
    tempLineOffset *= m_horizontalFlipFactor;

    if (!usePolar) {
        for (int i = 0; i < subGridLineCountX; i++) {
            if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosX = static_cast<QValue3DAxis *>(axisX())->subGridPositionAt(i) * scale * 2.0f
                           - scale;
            } else if (axisX()->type() == QAbstract3DAxis::AxisType::Category) {
                linePosX = calculateCategoryGridLinePosition(axisX(), i);
                linePosY = calculateCategoryGridLinePosition(axisY(), i);
            }

            *data++ = QVector3D(linePosX, linePosY + tempLineOffset, z0);
            *data++ = QVector3D(linePosX, linePosY + tempLineOffset, z1);
        }

        for (int i = 0; i < gridLineCountX; i++) {
            if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosX = static_cast<QValue3DAxis *>(axisX())->gridPositionAt(i) * scale * 2.0f
                           - scale;
            } else if (axisX()->type() == QAbstract3DAxis::AxisType::Category) {
                linePosX = calculateCategoryGridLinePosition(axisX(), i);
                linePosY = calculateCategoryGridLinePosition(axisY(), i);
            }

            *data++ = QVector3D(linePosX, linePosY + tempLineOffset, backgroundScale.z());
            *data++ = QVector3D(linePosX, linePosY + tempLineOffset, -backgroundScale.z());
        }
    } else {
        auto valueAxisX = static_cast<QValue3DAxis *>(axisX());
        const QVector3D center(0.0f, linePosY + tempLineOffset, 0.0f);
        const float halfRatio = ((m_polarRadius) + (m_labelMargin * 0.5f));

        for (int i = 0; i < subGridLineCountX; i++) {
            float angle = valueAxisX->subGridPositionAt(i) * 360.0f - rotationOffset;
            float posX = halfRatio * qCos(qDegreesToRadians(angle));
            float posZ = halfRatio * qSin(qDegreesToRadians(angle));
            *data++ = center;
            *data++ = QVector3D(posX, linePosY + tempLineOffset, posZ);
        }

        for (int i = 0; i < gridLineCountX - 1; i++) {
            float angle = valueAxisX->gridPositionAt(i) * 360.0f - rotationOffset;
            float posX = halfRatio * qCos(qDegreesToRadians(angle));
            float posZ = halfRatio * qSin(qDegreesToRadians(angle));
            *data++ = center;
            *data++ = QVector3D(posX, linePosY + tempLineOffset, posZ);
        }
    }

    // Back horizontal line
    linePosX = 0.0f;
    rotation = QVector3D(0.0f, 0.0f, 0.0f);

    x0 = -backgroundScale.x();
    x1 = backgroundScale.x();

    z0 = -backgroundScale.z();
    z1 = -backgroundScale.z();

    tempLineOffset = lineOffset;
    float tempBackOffsetAdjustment = backOffsetAdjustment;

    if (zFlipped) {
        rotation.setX(180.0f);
        z0 *= -1.0f;
        z1 *= -1.0f;
        tempLineOffset *= -1.0f;
        tempBackOffsetAdjustment *= -1.0f;
    }
    lineRotation = Utils::calculateRotation(rotation);
    scale = m_scaleWithBackground.y();
    for (int i = 0; i < subGridLineCountY; i++) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            linePosY = static_cast<QValue3DAxis *>(axisY())->subGridPositionAt(i) * scale * 2.0f
                       - scale;
        } else if (axisY()->type() == QAbstract3DAxis::AxisType::Category) {
            linePosY = calculateCategoryGridLinePosition(axisY(), i);
        }
        *data++ = QVector3D(x0, linePosY, z0 + tempLineOffset + tempBackOffsetAdjustment);
        *data++ = QVector3D(x1, linePosY, z1 + tempLineOffset + tempBackOffsetAdjustment);
    }

    for (int i = 0; i < gridLineCountY; i++) {
        if (axisY()->type() == QAbstract3DAxis::AxisType::Value) {
            linePosY = static_cast<QValue3DAxis *>(axisY())->gridPositionAt(i) * scale * 2.0f
                       - scale;
        } else if (axisY()->type() == QAbstract3DAxis::AxisType::Category) {
            linePosY = calculateCategoryGridLinePosition(axisY(), i);
        }
        *data++ = QVector3D(x0, linePosY, z0 + tempLineOffset + tempBackOffsetAdjustment);
        *data++ = QVector3D(x1, linePosY, z1 + tempLineOffset + tempBackOffsetAdjustment);
    }

    // Back vertical line
    scale = m_scaleWithBackground.x();
    rotation = QVector3D(0.0f, 0.0f, 0.0f);

    y0 = -backgroundScale.y();
    y1 = backgroundScale.y();

    z0 = -backgroundScale.z();
    z1 = -backgroundScale.z();

    tempLineOffset = lineOffset;
    tempBackOffsetAdjustment = backOffsetAdjustment;

    if (zFlipped) {
        rotation.setY(180.0f);
        z0 *= -1.0f;
        z1 *= -1.0f;
        tempLineOffset *= -1.0f;
        tempBackOffsetAdjustment *= -1.0f;
    }
    lineRotation = Utils::calculateRotation(rotation);
    if (m_hasVerticalSegmentLine) {
        for (int i = 0; i < gridLineCountX; i++) {
            if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosX = static_cast<QValue3DAxis *>(axisX())->gridPositionAt(i) * scale * 2.0f
                           - scale;
            }
            *data++ = QVector3D(linePosX, y0, z0 + tempLineOffset + tempBackOffsetAdjustment);
            *data++ = QVector3D(linePosX, y1, z1 + tempLineOffset + tempBackOffsetAdjustment);
        }

        for (int i = 0; i < subGridLineCountX; i++) {
            if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
                linePosX = static_cast<QValue3DAxis *>(axisX())->subGridPositionAt(i) * scale * 2.0f
                           - scale;
            }
            *data++ = QVector3D(linePosX, y0, z0 + tempLineOffset + tempBackOffsetAdjustment);
            *data++ = QVector3D(linePosX, y1, z1 + tempLineOffset + tempBackOffsetAdjustment);
        }
    }
    QQuick3DGeometry *gridGeometry = m_gridGeometryModel->geometry();
    gridGeometry->setVertexData(vertices);
    gridGeometry->update();
    m_gridUpdate = false;
}

float QQuickGraphsItem::fontScaleFactor(float pointSize)
{
    return 0.00007f + pointSize / (500000.0f * pointSize);
}

float QQuickGraphsItem::labelAdjustment(float width)
{
    float a = -2.43761e-13f;
    float b = 4.23579e-10f;
    float c = 0.00414881f;

    float factor = a * qPow(width, 3) + b * qPow(width, 2) + c;
#if defined(Q_OS_WIN)
    factor *= .8f;
#endif
    float ret = width * .5f * factor;
    return ret;
}

void QQuickGraphsItem::gridLineCountHelper(QAbstract3DAxis *axis, int &lineCount, int &sublineCount)
{
    if (axis->type() == QAbstract3DAxis::AxisType::Value) {
        auto valueAxis = static_cast<QValue3DAxis *>(axis);
        lineCount = valueAxis->gridSize();
        sublineCount = valueAxis->subGridSize();
    } else if (axis->type() == QAbstract3DAxis::AxisType::Category) {
        lineCount = axis->labels().size();
        sublineCount = 0;
    }
}

QVector3D QQuickGraphsItem::graphPosToAbsolute(const QVector3D &position)
{
    QVector3D pos = position;
    const int maxX = axisX()->max();
    const int minX = axisX()->min();
    const int maxY = axisY()->max();
    const int minY = axisY()->min();
    const int maxZ = axisZ()->max();
    const int minZ = axisZ()->min();
    const QVector3D adjustment = m_scaleWithBackground * QVector3D(1.0f, 1.0f, -1.0f);

    float xNormalizer = maxX - minX;
    float xPos = (pos.x() - minX) / xNormalizer;
    float yNormalizer = maxY - minY;
    float yPos = (pos.y() - minY) / yNormalizer;
    float zNormalizer = maxZ - minZ;
    float zPos = (pos.z() - minZ) / zNormalizer;
    pos = QVector3D(xPos, yPos, zPos);
    if (isPolar()) {
        float angle = xPos * M_PI * 2.0f;
        float radius = zPos;
        xPos = radius * qSin(angle) * 1.0f;
        zPos = -(radius * qCos(angle)) * 1.0f;
        yPos = yPos * adjustment.y() * 2.0f - adjustment.y();
        pos = QVector3D(xPos, yPos, zPos);
    } else {
        pos = pos * adjustment * 2.0f - adjustment;
    }
    return pos;
}

void QQuickGraphsItem::updateLabels()
{
    auto labels = axisX()->labels();
    int labelCount = labels.size();
    float labelAutoAngle = axisX()->labelAutoRotation();
    float labelAngleFraction = labelAutoAngle / 90.0f;
    float fractionCamX = m_xRotation * labelAngleFraction;
    float fractionCamY = m_yRotation * labelAngleFraction;

    QVector3D labelRotation = QVector3D(0.0f, 0.0f, 0.0f);

    float xPos = 0.0f;
    float yPos = 0.0f;
    float zPos = 0.0f;

    const bool xFlipped = isXFlipped();
    const bool yFlipped = isYFlipped();
    const bool zFlipped = isZFlipped();

    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;

    if (labelAutoAngle == 0.0f) {
        labelRotation = QVector3D(-90.0f, 90.0f, 0.0f);
        if (xFlipped)
            labelRotation.setY(-90.0f);
        if (yFlipped) {
            if (xFlipped)
                labelRotation.setY(-90.0f);
            else
                labelRotation.setY(90.0f);
            labelRotation.setX(90.0f);
        }
    } else {
        if (xFlipped)
            labelRotation.setY(-90.0f);
        else
            labelRotation.setY(90.0f);
        if (yFlipped) {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(90.0f
                                       - (2.0f * labelAutoAngle - fractionCamX)
                                             * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(90.0f
                                       - (2.0f * labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(
                        90.0f + fractionCamX * -(labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(
                        90.0f - fractionCamX * (-labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                }
            }
        } else {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(-90.0f
                                       + (2.0f * labelAutoAngle - fractionCamX)
                                             * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(-90.0f
                                       + (2.0f * labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(
                        -90.0f - fractionCamX * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(
                        -90.0f + fractionCamX * -(labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                }
            }
        }
    }
    if (isPolar())
        labelRotation.setY(0.0f);
    QQuaternion totalRotation = Utils::calculateRotation(labelRotation);

    float scale = backgroundScale.x() - m_backgroundScaleMargin.x();

    float pointSize = theme()->font().pointSizeF();

    float textPadding = pointSize * .5f;

    float labelsMaxWidth = float(findLabelsMaxWidth(axisX()->labels())) + textPadding;
    QFontMetrics fm(theme()->font());
    float labelHeight = fm.height() + textPadding;

    float scaleFactor = fontScaleFactor(pointSize) * pointSize;
    float fontRatio = labelsMaxWidth / labelHeight;
    m_fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    float adjustment = labelAdjustment(labelsMaxWidth);
    zPos = backgroundScale.z() + adjustment + m_labelMargin;

    adjustment *= qAbs(qSin(qDegreesToRadians(labelRotation.z())));
    yPos = backgroundScale.y() + adjustment;

    float yOffset = -0.1f;
    if (!yFlipped) {
        yPos *= -1.0f;
        yOffset *= -1.0f;
    }

    if (zFlipped)
        zPos *= -1.0f;

    auto labelTrans = QVector3D(0.0f, yPos, zPos);
    float angularLabelZPos = 0.0f;

    const float angularAdjustment{1.1f};
    if (axisX()->type() == QAbstract3DAxis::AxisType::Value) {
        auto valueAxisX = static_cast<QValue3DAxis *>(axisX());
        for (int i = 0; i < repeaterX()->count(); i++) {
            if (labelCount <= i)
                break;
            auto obj = static_cast<QQuick3DNode *>(repeaterX()->objectAt(i));
            if (isPolar()) {
                if (i == repeaterX()->count() - 1) {
                    obj->setVisible(false);
                    break;
                }
                float rad = qDegreesToRadians(valueAxisX->labelPositionAt(i) * 360.0f);
                labelTrans.setX((-qSin(rad) * -scale + qSin(rad) * m_labelMargin * m_polarRadius)
                                * angularAdjustment);
                labelTrans.setY(yPos + yOffset);
                labelTrans.setZ((qCos(rad) * -scale - qCos(rad) * m_labelMargin * m_polarRadius)
                                * angularAdjustment);
                if (i == 0) {
                    angularLabelZPos = labelTrans.z();
                    rad = qDegreesToRadians(valueAxisX->labelPositionAt(i) * 360.0f);
                    labelTrans.setX(
                        (-qSin(rad) * -scale + qSin(rad) * m_labelMargin * m_polarRadius));
                    labelTrans.setY(yPos + yOffset);
                    labelTrans.setZ(
                        (qCos(rad) * -scale - qCos(rad) * m_labelMargin * m_polarRadius));
                }
            } else {
                labelTrans.setX(valueAxisX->labelPositionAt(i) * scale * 2.0f - scale);
            }
            obj->setObjectName(QStringLiteral("ElementAxisXLabel"));
            obj->setScale(m_fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    } else if (axisX()->type() == QAbstract3DAxis::AxisType::Category) {
        for (int i = 0; i < repeaterX()->count(); i++) {
            if (labelCount <= i)
                break;
            labelTrans = calculateCategoryLabelPosition(axisX(), labelTrans, i);
            auto obj = static_cast<QQuick3DNode *>(repeaterX()->objectAt(i));
            obj->setObjectName(QStringLiteral("ElementAxisXLabel"));
            obj->setScale(m_fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    }

    float x = labelTrans.x();
    labelTrans.setX(0.0f);
    updateXTitle(labelRotation, labelTrans, totalRotation, labelsMaxWidth, m_fontScaled);
    if (isPolar()) {
        m_titleLabelX->setZ(angularLabelZPos - m_labelMargin * 2.0f);
        m_titleLabelX->setRotation(totalRotation);
    }
    labelTrans.setX(x);

    labels = axisY()->labels();
    labelCount = labels.size();
    labelAutoAngle = axisY()->labelAutoRotation();
    labelAngleFraction = labelAutoAngle / 90.0f;
    fractionCamX = m_xRotation * labelAngleFraction;
    fractionCamY = m_yRotation * labelAngleFraction;

    QVector3D sideLabelRotation(0.0f, -90.0f, 0.0f);
    QVector3D backLabelRotation(0.0f, 0.0f, 0.0f);

    if (labelAutoAngle == 0.0f) {
        if (!xFlipped)
            sideLabelRotation.setY(90.0f);
        if (zFlipped)
            backLabelRotation.setY(180.f);
    } else {
        // Orient side labels somewhat towards the camera
        if (xFlipped) {
            if (zFlipped)
                backLabelRotation.setY(180.0f + (2.0f * labelAutoAngle) - fractionCamX);
            else
                backLabelRotation.setY(-fractionCamX);
            sideLabelRotation.setY(-90.0f + labelAutoAngle - fractionCamX);
        } else {
            if (zFlipped)
                backLabelRotation.setY(180.0f - (2.0f * labelAutoAngle) - fractionCamX);
            else
                backLabelRotation.setY(-fractionCamX);
            sideLabelRotation.setY(90.0f - labelAutoAngle - fractionCamX);
        }
    }

    backLabelRotation.setX(-fractionCamY);
    sideLabelRotation.setX(-fractionCamY);

    totalRotation = Utils::calculateRotation(sideLabelRotation);
    scale = backgroundScale.y() - m_backgroundScaleMargin.y();
    labelsMaxWidth = float(findLabelsMaxWidth(axisY()->labels())) + textPadding;
    fontRatio = labelsMaxWidth / labelHeight;
    m_fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);

    xPos = backgroundScale.x();
    if (!xFlipped)
        xPos *= -1.0f;
    labelTrans.setX(xPos);

    adjustment = labelAdjustment(labelsMaxWidth);
    zPos = backgroundScale.z() + adjustment + m_labelMargin;
    if (zFlipped)
        zPos *= -1.0f;
    labelTrans.setZ(zPos);

    for (int i = 0; i < repeaterY()->count() / 2; i++) {
        if (labelCount <= i)
            break;
        auto obj = static_cast<QQuick3DNode *>(repeaterY()->objectAt(i));
        labelTrans.setY(static_cast<QValue3DAxis *>(axisY())->labelPositionAt(i) * scale * 2.0f
                        - scale);
        obj->setObjectName(QStringLiteral("ElementAxisYLabel"));
        obj->setScale(m_fontScaled);
        obj->setPosition(labelTrans);
        obj->setRotation(totalRotation);
        obj->setProperty("labelText", labels[i]);
        obj->setProperty("labelWidth", labelsMaxWidth);
        obj->setProperty("labelHeight", labelHeight);
    }

    auto sideLabelTrans = labelTrans;
    auto totalSideLabelRotation = totalRotation;

    labels = axisZ()->labels();
    labelCount = labels.size();
    labelAutoAngle = axisZ()->labelAutoRotation();
    labelAngleFraction = labelAutoAngle / 90.0f;
    fractionCamX = m_xRotation * labelAngleFraction;
    fractionCamY = m_yRotation * labelAngleFraction;

    if (labelAutoAngle == 0.0f) {
        labelRotation = QVector3D(90.0f, 0.0f, 0.0f);
        if (zFlipped)
            labelRotation.setY(180.0f);
        if (yFlipped) {
            if (zFlipped)
                labelRotation.setY(180.0f);
            else
                labelRotation.setY(0.0f);
            labelRotation.setX(90.0f);
        } else {
            labelRotation.setX(-90.0f);
        }
    } else {
        if (zFlipped)
            labelRotation.setY(180.0f);
        else
            labelRotation.setY(0.0f);
        if (yFlipped) {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(90.0f
                                       - (labelAutoAngle - fractionCamX)
                                             * (-labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(90.0f
                                       + (labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(90.0f
                                       + (labelAutoAngle - fractionCamX)
                                             * -(labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(90.0f
                                       - (labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                }
            }
        } else {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(-90.0f
                                       + (labelAutoAngle - fractionCamX)
                                             * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(-90.0f
                                       - (labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(-90.0f
                                       - (labelAutoAngle - fractionCamX)
                                             * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(-90.0f
                                       + (labelAutoAngle + fractionCamX)
                                             * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                }
            }
        }
    }

    totalRotation = Utils::calculateRotation(labelRotation);

    scale = backgroundScale.z() - m_backgroundScaleMargin.z();
    labelsMaxWidth = float(findLabelsMaxWidth(axisZ()->labels())) + textPadding;
    fontRatio = labelsMaxWidth / labelHeight;
    m_fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    adjustment = labelAdjustment(labelsMaxWidth);
    xPos = backgroundScale.x() + adjustment + m_labelMargin;
    if (xFlipped)
        xPos *= -1.0f;

    adjustment *= qAbs(qSin(qDegreesToRadians(labelRotation.z())));
    yPos = backgroundScale.y() + adjustment;
    if (!yFlipped)
        yPos *= -1.0f;

    labelTrans = QVector3D(xPos, yPos, 0.0f);
    if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
        auto valueAxisZ = static_cast<QValue3DAxis *>(axisZ());
        float offsetAdjustment = 0.05f;
        float offset = radialLabelOffset() + offsetAdjustment;
        for (int i = 0; i < repeaterZ()->count(); i++) {
            if (labelCount <= i)
                break;

            auto obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
            if (isPolar()) {
                // RADIAL LABELS
                float polarX = backgroundScale.x() * offset + m_labelMargin * 2.0f;
                if (xFlipped)
                    polarX *= -1;
                labelTrans.setX(polarX);
                labelTrans.setY(yPos + yOffset);

                labelTrans.setZ(-valueAxisZ->labelPositionAt(i) * m_polarRadius);
            } else {
                labelTrans.setZ(valueAxisZ->labelPositionAt(i) * scale * -2.0f + scale);
            }
            obj->setObjectName(QStringLiteral("ElementAxisZLabel"));
            obj->setScale(m_fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    } else if (axisZ()->type() == QAbstract3DAxis::AxisType::Category) {
        for (int i = 0; i < repeaterZ()->count(); i++) {
            if (labelCount <= i)
                break;
            labelTrans = calculateCategoryLabelPosition(axisZ(), labelTrans, i);
            auto obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
            obj->setObjectName(QStringLiteral("ElementAxisZLabel"));
            obj->setScale(m_fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    }

    float z = labelTrans.z();
    labelTrans.setZ(0.0f);
    updateZTitle(labelRotation, labelTrans, totalRotation, labelsMaxWidth, m_fontScaled);
    labelTrans.setZ(z);

    labels = axisY()->labels();
    labelCount = labels.size();
    totalRotation = Utils::calculateRotation(backLabelRotation);
    scale = backgroundScale.y() - m_backgroundScaleMargin.y();
    labelsMaxWidth = float(findLabelsMaxWidth(axisY()->labels())) + textPadding;
    fontRatio = labelsMaxWidth / labelHeight;
    m_fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    adjustment = labelAdjustment(labelsMaxWidth);

    xPos = backgroundScale.x() + adjustment + m_labelMargin;
    if (xFlipped)
        xPos *= -1.0f;
    labelTrans.setX(xPos);

    zPos = -backgroundScale.z();
    if (zFlipped)
        zPos *= -1.0f;
    labelTrans.setZ(zPos);

    for (int i = 0; i < repeaterY()->count() / 2; i++) {
        if (labelCount <= i)
            break;
        auto obj = static_cast<QQuick3DNode *>(
            repeaterY()->objectAt(i + (repeaterY()->count() / 2)));
        labelTrans.setY(static_cast<QValue3DAxis *>(axisY())->labelPositionAt(i) * scale * 2.0f
                        - scale);
        obj->setObjectName(QStringLiteral("ElementAxisYLabel"));
        obj->setScale(m_fontScaled);
        obj->setPosition(labelTrans);
        obj->setRotation(totalRotation);
        obj->setProperty("labelText", labels[i]);
        obj->setProperty("labelWidth", labelsMaxWidth);
        obj->setProperty("labelHeight", labelHeight);
    }

    QVector3D backLabelTrans = labelTrans;
    QQuaternion totalBackLabelRotation = totalRotation;
    updateYTitle(sideLabelRotation,
                 backLabelRotation,
                 sideLabelTrans,
                 backLabelTrans,
                 totalSideLabelRotation,
                 totalBackLabelRotation,
                 labelsMaxWidth,
                 m_fontScaled);
}

void QQuickGraphsItem::updateRadialLabelOffset()
{
    if (!isPolar())
        return;

    QVector3D backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float offset = radialLabelOffset();
    float scale = backgroundScale.x() + (m_backgroundScaleMargin.x());
    float polarX = scale * offset + m_labelMargin * 2.0f;
    if (isXFlipped())
        polarX *= -1;
    if (axisZ()->type() == QAbstract3DAxis::AxisType::Value) {
        for (int i = 0; i < repeaterZ()->count(); i++) {
            QQuick3DNode *obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
            QVector3D pos = obj->position();
            pos.setX(polarX);
            obj->setPosition(pos);
        }
    }

    polarX += m_labelMargin * 2.5f;
    QVector3D pos = m_titleLabelZ->position();
    pos.setX(polarX);
    m_titleLabelZ->setPosition(pos);
}

void QQuickGraphsItem::positionAndScaleLine(QQuick3DNode *lineNode,
                                            QVector3D scale,
                                            QVector3D position)
{
    lineNode->setScale(scale);
    lineNode->setPosition(position);
}

void QQuickGraphsItem::graphPositionAt(const QPoint &point)
{
    bool isHit = false;
    auto result = pick(point.x(), point.y());
    if (result.objectHit()) {
        isHit = true;
        setQueriedGraphPosition(QVector3D(result.scenePosition().x(),
                                          result.scenePosition().y(),
                                          result.scenePosition().z()));
    }

    if (!isHit)
        setQueriedGraphPosition(QVector3D(0, 0, 0));

    emit queriedGraphPositionChanged(queriedGraphPosition());
    setGraphPositionQueryPending(false);
    scene()->setGraphPositionQuery(scene()->invalidSelectionPoint());
}

void QQuickGraphsItem::updateShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    if (quality != QAbstract3DGraph::ShadowQuality::None) {
        light()->setCastsShadow(true);
        light()->setShadowFactor(25.f);

        QQuick3DAbstractLight::QSSGShadowMapQuality shadowMapQuality;
        switch (quality) {
        case QAbstract3DGraph::ShadowQuality::Low:
        case QAbstract3DGraph::ShadowQuality::SoftLow:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityMedium;
            break;
        case QAbstract3DGraph::ShadowQuality::Medium:
        case QAbstract3DGraph::ShadowQuality::SoftMedium:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityHigh;
            break;
        case QAbstract3DGraph::ShadowQuality::High:
        case QAbstract3DGraph::ShadowQuality::SoftHigh:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityVeryHigh;
            break;
        default:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityHigh;
            break;
        }
        light()->setShadowMapQuality(shadowMapQuality);
        if (quality >= QAbstract3DGraph::ShadowQuality::SoftLow)
            light()->setShadowFilter(10.f);
        else
            light()->setShadowFilter(2.f);
    } else {
        light()->setCastsShadow(false);
        light()->setShadowFactor(0.f);
    }
}

void QQuickGraphsItem::updateItemLabel(const QVector3D &position)
{
    if (m_labelPosition != position)
        m_labelPosition = position;
    QVector3D pos2d = mapFrom3DScene(m_labelPosition);
    int pointSize = theme()->font().pointSize();
    float scale = m_labelScale.x() * ((-10.0f * pointSize) + 650.0f) / pos2d.z();
    if (m_sliceView && m_sliceView->isVisible())
        m_itemLabel->setScale(scale * .2f);
    else
        m_itemLabel->setScale(scale);
    pos2d.setX(pos2d.x() - (m_itemLabel->width() / 2.f));
    pos2d.setY(pos2d.y() - (m_itemLabel->height() / 2.f)
               - (m_itemLabel->height() * m_itemLabel->scale()));
    m_itemLabel->setPosition(pos2d.toPointF());
}

void QQuickGraphsItem::updateSliceItemLabel(QString label, const QVector3D &position)
{
    Q_UNUSED(position);

    QFontMetrics fm(theme()->font());
    float textPadding = theme()->font().pointSizeF() * .7f;
    float labelHeight = fm.height() + textPadding;
    float labelWidth = fm.horizontalAdvance(label) + textPadding;

    float pointSize = theme()->font().pointSizeF();
    float scaleFactor = fontScaleFactor(pointSize) * pointSize;
    float fontRatio = labelWidth / labelHeight;

    QVector3D fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    m_sliceItemLabel->setScale(fontScaled);
}

void QQuickGraphsItem::createVolumeMaterial(QCustom3DVolume *volume, Volume &volumeItem)
{
    if (volumeItem.texture)
        volumeItem.texture->deleteLater();
    volumeItem.texture = new QQuick3DTexture();
    auto texture = volumeItem.texture;

    texture->setParent(this);
    texture->setMinFilter(QQuick3DTexture::Filter::Nearest);
    texture->setMagFilter(QQuick3DTexture::Filter::Nearest);
    texture->setHorizontalTiling(QQuick3DTexture::TilingMode::ClampToEdge);
    texture->setVerticalTiling(QQuick3DTexture::TilingMode::ClampToEdge);

    if (volumeItem.textureData)
        volumeItem.textureData->deleteLater();
    volumeItem.textureData = new QQuick3DTextureData();
    auto textureData = volumeItem.textureData;

    int color8Bit = (volume->textureFormat() == QImage::Format_Indexed8) ? 1 : 0;

    textureData->setParent(texture);
    textureData->setParentItem(texture);
    textureData->setSize(QSize(volume->textureWidth(), volume->textureHeight()));
    textureData->setDepth(volume->textureDepth());
    if (color8Bit)
        textureData->setFormat(QQuick3DTextureData::R8);
    else
        textureData->setFormat(QQuick3DTextureData::RGBA8);
    textureData->setTextureData(
        QByteArray::fromRawData(reinterpret_cast<const char *>(volume->textureData()->constData()),
                                volume->textureData()->size()));
    texture->setTextureData(textureData);

    QObject::connect(volume, &QCustom3DVolume::textureDataChanged, this, [this, volume] {
        m_customVolumes[volume].updateTextureData = true;
    });

    if (color8Bit) {
        if (volumeItem.colorTexture)
            volumeItem.colorTexture->deleteLater();
        volumeItem.colorTexture = new QQuick3DTexture();
        auto colorTexture = volumeItem.colorTexture;

        colorTexture->setParent(this);
        colorTexture->setMinFilter(QQuick3DTexture::Filter::Nearest);
        colorTexture->setMagFilter(QQuick3DTexture::Filter::Nearest);
        colorTexture->setHorizontalTiling(QQuick3DTexture::TilingMode::ClampToEdge);
        colorTexture->setVerticalTiling(QQuick3DTexture::TilingMode::ClampToEdge);

        QByteArray colorTableBytes;
        const QList<QRgb> &colorTable = volume->colorTable();
        for (int i = 0; i < colorTable.size(); i++) {
            QRgb shifted = qRgba(qBlue(colorTable[i]),
                                 qGreen(colorTable[i]),
                                 qRed(colorTable[i]),
                                 qAlpha(colorTable[i]));
            colorTableBytes.append(
                QByteArray::fromRawData(reinterpret_cast<const char *>(&shifted), sizeof(shifted)));
        }

        if (volumeItem.colorTextureData)
            volumeItem.colorTextureData->deleteLater();
        volumeItem.colorTextureData = new QQuick3DTextureData();
        auto colorTextureData = volumeItem.colorTextureData;

        colorTextureData->setParent(colorTexture);
        colorTextureData->setParentItem(colorTexture);
        colorTextureData->setSize(QSize(volume->colorTable().size(), 1));
        colorTextureData->setFormat(QQuick3DTextureData::RGBA8);
        colorTextureData->setTextureData(colorTableBytes);
        colorTexture->setTextureData(colorTextureData);

        QObject::connect(volume, &QCustom3DVolume::colorTableChanged, this, [this, volume] {
            m_customVolumes[volume].updateColorTextureData = true;
        });
    }

    auto model = volumeItem.model;
    QQmlListReference materialsRef(model, "materials");

    QQuick3DCustomMaterial *material = nullptr;

    if (volume->drawSlices() && m_validVolumeSlice)
        material = createQmlCustomMaterial(QStringLiteral(":/materials/VolumeSliceMaterial"));
    else if (volume->useHighDefShader())
        material = createQmlCustomMaterial(QStringLiteral(":/materials/VolumeMaterial"));
    else
        material = createQmlCustomMaterial(QStringLiteral(":/materials/VolumeLowDefMaterial"));

    auto textureSamplerVariant = material->property("textureSampler");
    auto textureSampler = textureSamplerVariant.value<QQuick3DShaderUtilsTextureInput *>();
    textureSampler->setTexture(volumeItem.texture);

    if (color8Bit) {
        auto colorSamplerVariant = material->property("colorSampler");
        auto colorSampler = colorSamplerVariant.value<QQuick3DShaderUtilsTextureInput *>();
        colorSampler->setTexture(volumeItem.colorTexture);
    }

    material->setProperty("textureDimensions",
                          QVector3D(1.0f / float(volume->textureWidth()),
                                    1.0f / float(volume->textureHeight()),
                                    1.0f / float(volume->textureDepth())));

    materialsRef.append(material);

    volumeItem.useHighDefShader = volume->useHighDefShader();
    volumeItem.drawSlices = volume->drawSlices() && m_validVolumeSlice;
}

QQuick3DModel *QQuickGraphsItem::createSliceFrame(Volume &volumeItem)
{
    QQuick3DModel *model = new QQuick3DModel();
    model->setParent(volumeItem.model);
    model->setParentItem(volumeItem.model);
    model->setSource(QUrl(QStringLiteral("defaultMeshes/barMeshFull")));
    model->setScale(QVector3D(1, 1, 0.01f));
    model->setDepthBias(-100.0f);

    QQmlListReference materialsRef(model, "materials");
    QQuick3DCustomMaterial *material = createQmlCustomMaterial(
        QStringLiteral(":/materials/VolumeFrameMaterial"));
    material->setParent(model);
    material->setParentItem(model);
    material->setCullMode(QQuick3DMaterial::NoCulling);
    materialsRef.append(material);

    return model;
}

void QQuickGraphsItem::updateSliceFrameMaterials(QCustom3DVolume *volume, Volume &volumeItem)
{
    QQmlListReference materialsRefX(volumeItem.sliceFrameX, "materials");
    QQmlListReference materialsRefY(volumeItem.sliceFrameY, "materials");
    QQmlListReference materialsRefZ(volumeItem.sliceFrameZ, "materials");

    QVector2D frameWidth;
    QVector3D frameScaling;

    frameScaling = QVector3D(volume->scaling().z()
                                 + (volume->scaling().z() * volume->sliceFrameGaps().z())
                                 + (volume->scaling().z() * volume->sliceFrameWidths().z()),
                             volume->scaling().y()
                                 + (volume->scaling().y() * volume->sliceFrameGaps().y())
                                 + (volume->scaling().y() * volume->sliceFrameWidths().y()),
                             volume->scaling().x() * volume->sliceFrameThicknesses().x());

    frameWidth = QVector2D(volume->scaling().z() * volume->sliceFrameWidths().z(),
                           volume->scaling().y() * volume->sliceFrameWidths().y());

    frameWidth.setX(1.0f - (frameWidth.x() / frameScaling.x()));
    frameWidth.setY(1.0f - (frameWidth.y() / frameScaling.y()));

    auto material = materialsRefX.at(0);
    material->setProperty("color", volume->sliceFrameColor());
    material->setProperty("sliceFrameWidth", frameWidth);

    frameScaling = QVector3D(volume->scaling().x()
                                 + (volume->scaling().x() * volume->sliceFrameGaps().x())
                                 + (volume->scaling().x() * volume->sliceFrameWidths().x()),
                             volume->scaling().z()
                                 + (volume->scaling().z() * volume->sliceFrameGaps().z())
                                 + (volume->scaling().z() * volume->sliceFrameWidths().z()),
                             volume->scaling().y() * volume->sliceFrameThicknesses().y());
    frameWidth = QVector2D(volume->scaling().x() * volume->sliceFrameWidths().x(),
                           volume->scaling().z() * volume->sliceFrameWidths().z());

    frameWidth.setX(1.0f - (frameWidth.x() / frameScaling.x()));
    frameWidth.setY(1.0f - (frameWidth.y() / frameScaling.y()));

    material = materialsRefY.at(0);
    material->setProperty("color", volume->sliceFrameColor());
    material->setProperty("sliceFrameWidth", frameWidth);

    frameScaling = QVector3D(volume->scaling().x()
                                 + (volume->scaling().x() * volume->sliceFrameGaps().x())
                                 + (volume->scaling().x() * volume->sliceFrameWidths().x()),
                             volume->scaling().y()
                                 + (volume->scaling().y() * volume->sliceFrameGaps().y())
                                 + (volume->scaling().y() * volume->sliceFrameWidths().y()),
                             volume->scaling().z() * volume->sliceFrameThicknesses().z());
    frameWidth = QVector2D(volume->scaling().x() * volume->sliceFrameWidths().x(),
                           volume->scaling().y() * volume->sliceFrameWidths().y());

    frameWidth.setX(1.0f - (frameWidth.x() / frameScaling.x()));
    frameWidth.setY(1.0f - (frameWidth.y() / frameScaling.y()));

    material = materialsRefZ.at(0);
    material->setProperty("color", volume->sliceFrameColor());
    material->setProperty("sliceFrameWidth", frameWidth);
}

void QQuickGraphsItem::updateCustomVolumes()
{
    auto itemIterator = m_customItemList.constBegin();
    while (itemIterator != m_customItemList.constEnd()) {
        QCustom3DItem *item = itemIterator.key();
        QQuick3DModel *model = itemIterator.value();

        if (auto volume = qobject_cast<QCustom3DVolume *>(item)) {
            auto &&volumeItem = m_customVolumes[volume];

            QQmlListReference materialsRef(model, "materials");
            if (volumeItem.useHighDefShader != volume->useHighDefShader()) {
                materialsRef.clear();
                createVolumeMaterial(volume, volumeItem);
            }

            m_validVolumeSlice = volume->sliceIndexX() >= 0
                    || volume->sliceIndexY() >= 0
                    || volume->sliceIndexZ() >= 0;

            if (volumeItem.drawSlices != (volume->drawSlices() && m_validVolumeSlice)) {
                materialsRef.clear();
                createVolumeMaterial(volume, volumeItem);
            }

            QVector3D sliceIndices(
                (float(volume->sliceIndexX()) + 0.5f) / float(volume->textureWidth()) * 2.0 - 1.0,
                (float(volume->sliceIndexY()) + 0.5f) / float(volume->textureHeight()) * 2.0 - 1.0,
                (float(volume->sliceIndexZ()) + 0.5f) / float(volume->textureDepth()) * 2.0 - 1.0);

            if (volumeItem.drawSliceFrames != volume->drawSliceFrames()) {
                if (volume->drawSliceFrames()) {
                    volumeItem.sliceFrameX->setVisible(true);
                    volumeItem.sliceFrameY->setVisible(true);
                    volumeItem.sliceFrameZ->setVisible(true);

                    volumeItem.sliceFrameX->setRotation(QQuaternion::fromEulerAngles(0, 90, 0));
                    volumeItem.sliceFrameY->setRotation(QQuaternion::fromEulerAngles(90, 0, 0));

                    updateSliceFrameMaterials(volume, volumeItem);
                } else {
                    volumeItem.sliceFrameX->setVisible(false);
                    volumeItem.sliceFrameY->setVisible(false);
                    volumeItem.sliceFrameZ->setVisible(false);
                }
                volumeItem.drawSliceFrames = volume->drawSliceFrames();
            }

            auto material = materialsRef.at(0);
            QVector3D minBounds(-1, 1, 1);
            QVector3D maxBounds(1, -1, -1);
            QVector3D translation(0, 0, 0);
            QVector3D scaling(1, 1, 1);

            model->setVisible(volume->isVisible());
            if (!volume->isScalingAbsolute() && !volume->isPositionAbsolute()) {

                QVector3D pos = volume->position();
                QVector3D scale = volume->scaling() / 2;

                QVector3D minGraphBounds(pos.x() - scale.x(),
                                    pos.y() - scale.y(),
                                    pos.z() + scale.z());
                QVector3D maxGraphBounds(pos.x() + scale.x(),
                                    pos.y() + scale.y(),
                                    pos.z() - scale.z());

                QVector3D minCorner = graphPosToAbsolute(minGraphBounds);
                QVector3D maxCorner = graphPosToAbsolute(maxGraphBounds);

                scale = QVector3D(qAbs(maxCorner.x() - minCorner.x()),
                                  qAbs(maxCorner.y() - minCorner.y()),
                                  qAbs(maxCorner.z() - minCorner.z())) / 2.0f;

                const QVector3D mScale = scaleWithBackground();
                const QVector3D itemRange = maxCorner - minCorner;
                if (minCorner.x() < -mScale.x())
                    minBounds.setX(-1.0f + (2.0f * qAbs(minCorner.x() + mScale.x()) / itemRange.x()));
                if (minCorner.y() < -mScale.y())
                    minBounds.setY(-(-1.0f + (2.0f * qAbs(minCorner.y() + mScale.y()) / itemRange.y())));
                if (minCorner.z() < -mScale.z())
                    minBounds.setZ(-(-1.0f + (2.0f * qAbs(minCorner.z() + mScale.z()) / itemRange.z())));

                if (maxCorner.x() > mScale.x())
                    maxBounds.setX(1.0f - (2.0f * qAbs(maxCorner.x() - mScale.x()) / itemRange.x()));
                if (maxCorner.y() > mScale.y())
                    maxBounds.setY(-(1.0f - (2.0f * qAbs(maxCorner.y() - mScale.y()) / itemRange.y())));
                if (maxCorner.z() > mScale.z())
                    maxBounds.setZ(-(1.0f - (2.0f * qAbs(maxCorner.z() - mScale.z()) / itemRange.z())));

                QVector3D minBoundsNorm = minBounds;
                QVector3D maxBoundsNorm = maxBounds;

                minBoundsNorm.setY(-minBoundsNorm.y());
                minBoundsNorm.setZ(-minBoundsNorm.z());
                minBoundsNorm = 0.5f * (minBoundsNorm + QVector3D(1,1,1));

                maxBoundsNorm.setY(-maxBoundsNorm.y());
                maxBoundsNorm.setZ(-maxBoundsNorm.z());
                maxBoundsNorm = 0.5f * (maxBoundsNorm + QVector3D(1,1,1));

                QVector3D adjScaling = scale * (maxBoundsNorm - minBoundsNorm);
                model->setScale(adjScaling);

                QVector3D adjPos = volume->position();
                QVector3D dataExtents = (maxGraphBounds - minGraphBounds) / 2.0f;

                adjPos = adjPos + (dataExtents * minBoundsNorm)
                        - (dataExtents * (QVector3D(1,1,1) - maxBoundsNorm));
                adjPos = graphPosToAbsolute(adjPos);
                model->setPosition(adjPos);
            } else {
                model->setScale(volume->scaling());
            }
            model->setRotation(volume->rotation());

            material->setProperty("minBounds", minBounds);
            material->setProperty("maxBounds", maxBounds);

            if (volume->drawSlices())
                material->setProperty("volumeSliceIndices", sliceIndices);

            if (volume->drawSliceFrames()) {
                float sliceFrameX = sliceIndices.x();
                float sliceFrameY = sliceIndices.y();
                float sliceFrameZ = sliceIndices.z();
                if (volume->sliceIndexX() >= 0 && scaling.x() > 0)
                    sliceFrameX = (sliceFrameX + translation.x()) / scaling.x();
                if (volume->sliceIndexY() >= 0 && scaling.y() > 0)
                    sliceFrameY = (sliceFrameY - translation.y()) / scaling.y();
                if (volume->sliceIndexZ() >= 0 && scaling.z() > 0)
                    sliceFrameZ = (sliceFrameZ + translation.z()) / scaling.z();

                if (sliceFrameX < -1 || sliceFrameX > 1)
                    volumeItem.sliceFrameX->setVisible(false);
                else
                    volumeItem.sliceFrameX->setVisible(true);

                if (sliceFrameY < -1 || sliceFrameY > 1)
                    volumeItem.sliceFrameY->setVisible(false);
                else
                    volumeItem.sliceFrameY->setVisible(true);

                if (sliceFrameZ < -1 || sliceFrameZ > 1)
                    volumeItem.sliceFrameZ->setVisible(false);
                else
                    volumeItem.sliceFrameZ->setVisible(true);

                volumeItem.sliceFrameX->setX(sliceFrameX);
                volumeItem.sliceFrameY->setY(-sliceFrameY);
                volumeItem.sliceFrameZ->setZ(-sliceFrameZ);
            }

            material->setProperty("alphaMultiplier", volume->alphaMultiplier());
            material->setProperty("preserveOpacity", volume->preserveOpacity());
            material->setProperty("useOrtho", isOrthoProjection());

            int sampleCount = volume->textureWidth() + volume->textureHeight()
                              + volume->textureDepth();
            material->setProperty("sampleCount", sampleCount);

            int color8Bit = (volume->textureFormat() == QImage::Format_Indexed8) ? 1 : 0;
            material->setProperty("color8Bit", color8Bit);

            if (volumeItem.updateTextureData) {
                auto textureData = volumeItem.textureData;
                textureData->setSize(QSize(volume->textureWidth(), volume->textureHeight()));
                textureData->setDepth(volume->textureDepth());

                if (color8Bit)
                    textureData->setFormat(QQuick3DTextureData::R8);
                else
                    textureData->setFormat(QQuick3DTextureData::RGBA8);

                textureData->setTextureData(
                    QByteArray::fromRawData(reinterpret_cast<const char *>(
                                                volume->textureData()->constData()),
                                            volume->textureData()->size()));

                material->setProperty("textureDimensions",
                                      QVector3D(1.0f / float(volume->textureWidth()),
                                                1.0f / float(volume->textureHeight()),
                                                1.0f / float(volume->textureDepth())));

                volumeItem.updateTextureData = false;
            }

            if (volumeItem.updateColorTextureData) {
                auto colorTextureData = volumeItem.colorTextureData;
                QByteArray colorTableBytes;
                const QList<QRgb> &colorTable = volume->colorTable();
                for (int i = 0; i < colorTable.size(); i++) {
                    QRgb shifted = qRgba(qBlue(colorTable[i]),
                                         qGreen(colorTable[i]),
                                         qRed(colorTable[i]),
                                         qAlpha(colorTable[i]));
                    colorTableBytes.append(
                        QByteArray::fromRawData(reinterpret_cast<const char *>(&shifted),
                                                sizeof(shifted)));
                }
                colorTextureData->setTextureData(colorTableBytes);
            }
        }
        ++itemIterator;
    }
}

void QQuickGraphsItem::updateAxisRange(float min, float max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);
}

void QQuickGraphsItem::updateAxisReversed(bool enable)
{
    Q_UNUSED(enable);
}

int QQuickGraphsItem::findLabelsMaxWidth(const QStringList &labels)
{
    int labelWidth = 0;
    QFontMetrics labelFM(theme()->font());

    for (const auto &label : std::as_const(labels)) {
        auto width = labelFM.horizontalAdvance(label);
        if (labelWidth < width)
            labelWidth = width;
    }
    return labelWidth;
}

QVector3D QQuickGraphsItem::calculateCategoryLabelPosition(QAbstract3DAxis *axis,
                                                           QVector3D labelPosition,
                                                           int index)
{
    Q_UNUSED(axis);
    Q_UNUSED(index);
    return labelPosition;
}

float QQuickGraphsItem::calculateCategoryGridLinePosition(QAbstract3DAxis *axis, int index)
{
    Q_UNUSED(axis);
    Q_UNUSED(index);
    return 0.0f;
}

float QQuickGraphsItem::calculatePolarBackgroundMargin()
{
    // Check each extents of each angular label
    // Calculate angular position
    auto valueAxisX = static_cast<QValue3DAxis *>(axisX());
    auto labelPositions = const_cast<QList<float> &>(valueAxisX->formatter()->labelPositions());
    float actualLabelHeight = m_fontScaled.y() * 2.0f; // All labels are same height
    float maxNeededMargin = 0.0f;

    // Axis title needs to be accounted for
    if (valueAxisX->isTitleVisible())
        maxNeededMargin = 2.0f * actualLabelHeight + 3.0f * labelMargin();

    for (int label = 0; label < labelPositions.size(); label++) {
        QSizeF labelSize{m_fontScaled.x(), m_fontScaled.z()};
        float actualLabelWidth = actualLabelHeight / labelSize.height() * labelSize.width();
        float labelPosition = labelPositions.at(label);
        qreal angle = labelPosition * M_PI * 2.0;
        float x = qAbs((m_polarRadius + labelMargin()) * float(qSin(angle))) + actualLabelWidth
                  - m_polarRadius + labelMargin();
        float z = qAbs(-(m_polarRadius + labelMargin()) * float(qCos(angle))) + actualLabelHeight
                  - m_polarRadius + labelMargin();
        float neededMargin = qMax(x, z);
        maxNeededMargin = qMax(maxNeededMargin, neededMargin);
    }

    maxNeededMargin *= 0.2f;
    return maxNeededMargin;
}

void QQuickGraphsItem::updateXTitle(const QVector3D &labelRotation,
                                    const QVector3D &labelTrans,
                                    const QQuaternion &totalRotation,
                                    float labelsMaxWidth,
                                    const QVector3D &scale)
{
    float pointSize = theme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(theme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(axisX()->title()) + textPadding;

    float titleOffset;

    bool radial = false;
    if (radial)
        titleOffset = -2.0f * (m_labelMargin + scale.y());
    else
        titleOffset = 2.0f * m_labelMargin + (labelsMaxWidth * scale.y());

    float zRotation = 0.0f;
    float yRotation = 0.0f;
    float xRotation = -90.0f + labelRotation.z();
    float offsetRotation = labelRotation.z();
    float extraRotation = -90.0f;
    if (m_yFlipped) {
        zRotation = 180.0f;
        if (m_zFlipped) {
            titleOffset = -titleOffset;
            if (m_xFlipped) {
                offsetRotation = -offsetRotation;
                extraRotation = -extraRotation;
            } else {
                xRotation = -90.0f - labelRotation.z();
            }
        } else {
            yRotation = 180.0f;
            if (m_xFlipped) {
                offsetRotation = -offsetRotation;
                xRotation = -90.0f - labelRotation.z();
            } else {
                extraRotation = -extraRotation;
            }
        }
    } else {
        if (m_zFlipped) {
            titleOffset = -titleOffset;
            if (m_xFlipped) {
                offsetRotation = -offsetRotation;
            } else {
                xRotation = -90.0f - labelRotation.z();
                extraRotation = -extraRotation;
            }
            yRotation = 180.0f;
            if (m_yFlipped) {
                extraRotation = -extraRotation;
                if (m_xFlipped)
                    xRotation = 90.0f + labelRotation.z();
                else
                    xRotation = 90.0f - labelRotation.z();
            }
        } else {
            if (m_xFlipped) {
                offsetRotation = -offsetRotation;
                xRotation = -90.0f - labelRotation.z();
                extraRotation = -extraRotation;
            }
            if (m_yFlipped) {
                xRotation = 90.0f + labelRotation.z();
                extraRotation = -extraRotation;
                if (m_xFlipped)
                    xRotation = 90.0f - labelRotation.z();
            }
        }
    }

    if (offsetRotation == 180.0f || offsetRotation == -180.0f)
        offsetRotation = 0.0f;

    QQuaternion offsetRotator = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, offsetRotation);
    QVector3D titleOffsetVector = offsetRotator.rotatedVector(QVector3D(0.0f, 0.0f, titleOffset));

    QQuaternion titleRotation;
    if (axisX()->isTitleFixed()) {
        titleRotation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, zRotation)
                        * QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, yRotation)
                        * QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, xRotation);
    } else {
        titleRotation = totalRotation
                        * QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, extraRotation);
    }

    QVector3D titleScale = scale;
    titleScale.setX(titleScale.y() * width / height);
    m_titleLabelX->setScale(titleScale);
    m_titleLabelX->setPosition(labelTrans + titleOffsetVector);
    m_titleLabelX->setRotation(titleRotation);
    m_titleLabelX->setProperty("labelWidth", width);
    m_titleLabelX->setProperty("labelHeight", height);
}

void QQuickGraphsItem::updateYTitle(const QVector3D &sideLabelRotation,
                                    const QVector3D &backLabelRotation,
                                    const QVector3D &sideLabelTrans,
                                    const QVector3D &backLabelTrans,
                                    const QQuaternion &totalSideRotation,
                                    const QQuaternion &totalBackRotation,
                                    float labelsMaxWidth,
                                    const QVector3D &scale)
{
    float pointSize = theme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(theme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(axisY()->title()) + textPadding;

    float titleOffset = m_labelMargin + (labelsMaxWidth * scale.x());

    QQuaternion zRightAngleRotation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, 90.0f);
    float yRotation;
    QVector3D titleTrans;
    QQuaternion totalRotation;
    if (m_xFlipped != m_zFlipped) {
        yRotation = backLabelRotation.y();
        titleTrans = backLabelTrans;
        totalRotation = totalBackRotation;
    } else {
        yRotation = sideLabelRotation.y();
        titleTrans = sideLabelTrans;
        totalRotation = totalSideRotation;
    }
    titleTrans.setY(.0f);

    QQuaternion offsetRotator = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, yRotation);
    QVector3D titleOffsetVector = offsetRotator.rotatedVector(QVector3D(-titleOffset, 0.0f, 0.0f));

    QQuaternion titleRotation;
    if (axisY()->isTitleFixed()) {
        titleRotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, yRotation)
                        * zRightAngleRotation;
    } else {
        titleRotation = totalRotation * zRightAngleRotation;
    }

    QVector3D titleScale = scale;
    titleScale.setX(titleScale.y() * width / height);
    m_titleLabelY->setScale(titleScale);
    m_titleLabelY->setPosition(titleTrans + titleOffsetVector);
    m_titleLabelY->setRotation(titleRotation);
    m_titleLabelY->setProperty("labelWidth", width);
    m_titleLabelY->setProperty("labelHeight", height);
}

void QQuickGraphsItem::updateZTitle(const QVector3D &labelRotation,
                                    const QVector3D &labelTrans,
                                    const QQuaternion &totalRotation,
                                    float labelsMaxWidth,
                                    const QVector3D &scale)
{
    float pointSize = theme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(theme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(axisZ()->title()) + textPadding;

    float titleOffset = m_labelMargin + (labelsMaxWidth * scale.x());

    float zRotation = labelRotation.z();
    float yRotation = -90.0f;
    float xRotation = -90.0f;
    float extraRotation = 90.0f;

    if (m_yFlipped) {
        xRotation = -xRotation;
        if (m_zFlipped) {
            if (m_xFlipped) {
                titleOffset = -titleOffset;
                zRotation = -zRotation;
                extraRotation = -extraRotation;
            } else {
                zRotation = -zRotation;
                yRotation = -yRotation;
            }
        } else {
            if (m_xFlipped) {
                titleOffset = -titleOffset;
            } else {
                extraRotation = -extraRotation;
                yRotation = -yRotation;
            }
        }
    } else {
        if (m_zFlipped) {
            zRotation = -zRotation;
            if (m_xFlipped) {
                titleOffset = -titleOffset;
            } else {
                extraRotation = -extraRotation;
                yRotation = -yRotation;
            }
        } else {
            if (m_xFlipped) {
                titleOffset = -titleOffset;
                extraRotation = -extraRotation;
            } else {
                yRotation = -yRotation;
            }
        }
        if (m_yFlipped) {
            xRotation = -xRotation;
            extraRotation = -extraRotation;
        }
    }

    float offsetRotation = zRotation;
    if (offsetRotation == 180.0f || offsetRotation == -180.0f)
        offsetRotation = 0.0f;

    QQuaternion offsetRotator = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, offsetRotation);
    QVector3D titleOffsetVector = offsetRotator.rotatedVector(QVector3D(titleOffset, 0.0f, 0.0f));

    QQuaternion titleRotation;
    if (axisZ()->isTitleFixed()) {
        titleRotation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, zRotation)
                        * QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, yRotation)
                        * QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, xRotation);
    } else {
        titleRotation = totalRotation
                        * QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, extraRotation);
    }

    QVector3D titleScale = scale;
    titleScale.setX(titleScale.y() * width / height);
    m_titleLabelZ->setScale(titleScale);
    m_titleLabelZ->setPosition(labelTrans + titleOffsetVector);
    m_titleLabelZ->setRotation(titleRotation);
    m_titleLabelZ->setProperty("labelWidth", width);
    m_titleLabelZ->setProperty("labelHeight", height);
}

void QQuickGraphsItem::updateCamera()
{
    QVector3D lookingPosition = m_requestedTarget;

    const float scale = qMin(width(), height() * 1.6f);
    const float magnificationScaleFactor = 1.0f / 640.0f;
    const float magnification = scale * magnificationScaleFactor;

    auto useOrtho = isOrthoProjection();
    if (useOrtho) {
        if (m_sliceView && m_sliceView->isVisible()) {
            m_oCamera->setVerticalMagnification(m_zoomLevel * .4f);
            m_oCamera->setHorizontalMagnification(m_zoomLevel * .4f);
        } else {
            m_oCamera->setVerticalMagnification(m_zoomLevel * magnification);
            m_oCamera->setHorizontalMagnification(m_zoomLevel * magnification);
        }
    }
    cameraTarget()->setPosition(lookingPosition);
    auto rotation = QVector3D(-m_yRotation, -m_xRotation, 0);
    cameraTarget()->setEulerRotation(rotation);
    float zoom = 720.f / m_zoomLevel;
    m_pCamera->setZ(zoom);
    updateCustomLabelsRotation();
    updateItemLabel(m_labelPosition);
}

void QQuickGraphsItem::handleLabelCountChanged(QQuick3DRepeater *repeater)
{
    changeLabelBackgroundColor(repeater, theme()->labelBackgroundColor());
    changeLabelBackgroundEnabled(repeater, theme()->isLabelBackgroundEnabled());
    changeLabelBorderEnabled(repeater, theme()->isLabelBorderEnabled());
    changeLabelTextColor(repeater, theme()->labelTextColor());
    changeLabelFont(repeater, theme()->font());

    if (m_sliceView) {
        changeLabelBackgroundColor(m_sliceHorizontalLabelRepeater, theme()->labelBackgroundColor());
        changeLabelBackgroundColor(m_sliceVerticalLabelRepeater, theme()->labelBackgroundColor());
        changeLabelBackgroundEnabled(m_sliceHorizontalLabelRepeater,
                                     theme()->isLabelBackgroundEnabled());
        changeLabelBackgroundEnabled(m_sliceVerticalLabelRepeater,
                                     theme()->isLabelBackgroundEnabled());
        changeLabelBorderEnabled(m_sliceHorizontalLabelRepeater, theme()->isLabelBorderEnabled());
        changeLabelBorderEnabled(m_sliceVerticalLabelRepeater, theme()->isLabelBorderEnabled());
        changeLabelTextColor(m_sliceHorizontalLabelRepeater, theme()->labelTextColor());
        changeLabelTextColor(m_sliceVerticalLabelRepeater, theme()->labelTextColor());
        changeLabelFont(m_sliceHorizontalLabelRepeater, theme()->font());
        changeLabelFont(m_sliceVerticalLabelRepeater, theme()->font());
    }
}

void QQuickGraphsItem::updateCustomData()
{
    int maxX = axisX()->max();
    int minX = axisX()->min();
    int maxY = axisY()->max();
    int minY = axisY()->min();
    int maxZ = axisZ()->max();
    int minZ = axisZ()->min();

    auto labelIterator = m_customLabelList.constBegin();
    while (labelIterator != m_customLabelList.constEnd()) {
        QCustom3DLabel *label = labelIterator.key();
        QQuick3DNode *customLabel = labelIterator.value();

        QVector3D pos = label->position();
        if (!label->isPositionAbsolute()) {
            if (label->position().x() < minX || label->position().x() > maxX
                || label->position().y() < minY || label->position().y() > maxY
                || label->position().z() < minZ || label->position().z() > maxZ) {
                customLabel->setVisible(false);
                ++labelIterator;
                continue;
            }
            pos = graphPosToAbsolute(pos);
        }

        QFontMetrics fm(label->font());
        int width = fm.horizontalAdvance(label->text());
        int height = fm.height();
        customLabel->setProperty("labelWidth", width);
        customLabel->setProperty("labelHeight", height);
        customLabel->setPosition(pos);
        QQuaternion rotation = label->rotation();
        if (label->isFacingCamera())
            rotation = Utils::calculateRotation(QVector3D(-m_yRotation, -m_xRotation, 0));
        customLabel->setRotation(rotation);
        float pointSize = theme()->font().pointSizeF();
        float scaleFactor = fontScaleFactor(pointSize) * pointSize;
        float fontRatio = float(height) / float(width);
        QVector3D fontScaled = QVector3D(scaleFactor / fontRatio, scaleFactor, 0.0f);
        customLabel->setScale(fontScaled);
        customLabel->setProperty("labelText", label->text());
        customLabel->setProperty("labelTextColor", label->textColor());
        customLabel->setProperty("labelFont", label->font());
        customLabel->setProperty("backgroundEnabled", label->isBackgroundEnabled());
        customLabel->setProperty("backgroundColor", label->backgroundColor());
        customLabel->setProperty("borderEnabled", label->isBorderEnabled());
        customLabel->setVisible(label->isVisible());

        ++labelIterator;
    }

    auto itemIterator = m_customItemList.constBegin();
    while (itemIterator != m_customItemList.constEnd()) {
        QCustom3DItem *item = itemIterator.key();
        QQuick3DModel *model = itemIterator.value();

        QVector3D pos = item->position();
        QVector<QAbstract3DAxis *> axes{axisX(), axisY(), axisZ()};
        QVector<float> bScales{scaleWithBackground().x(),
                    scaleWithBackground().y(),
                    scaleWithBackground().z()};
        if (!item->isPositionAbsolute()) {
            if (item->position().x() < minX || item->position().x() > maxX
                || item->position().y() < minY || item->position().y() > maxY
                || item->position().z() < minZ || item->position().z() > maxZ) {
                model->setVisible(false);
                ++itemIterator;
                continue;
            }
            pos = graphPosToAbsolute(pos);
        }
        model->setPosition(pos);

        if (!item->isScalingAbsolute()) {
            QVector<float> iScales{item->scaling().x(), item->scaling().y(), item->scaling().z()};
            for (int i = 0; i < axes.count(); i++) {
                if (auto vAxis = static_cast<QValue3DAxis *>(axes.at(i))) {
                    float axisRange = vAxis->max() - vAxis->min();
                    float realRange = bScales.at(i);
                    float ratio = realRange / axisRange;
                    iScales[i] *= ratio;
                }
            }
            model->setScale(QVector3D(iScales.at(0), iScales.at(1), iScales.at(2)));
        } else {
            model->setScale(item->scaling());
        }

        if (auto volume = qobject_cast<QCustom3DVolume *>(item)) {
            if (!m_customVolumes.contains(volume)) {
                auto &&volumeItem = m_customVolumes[volume];

                volumeItem.model = model;
                model->setSource(QUrl(volume->meshFile()));

                volumeItem.useHighDefShader = volume->useHighDefShader();

                m_validVolumeSlice = volume->sliceIndexX() >= 0
                        || volume->sliceIndexY() >= 0
                        || volume->sliceIndexZ() >= 0;

                volumeItem.drawSlices = volume->drawSlices() && m_validVolumeSlice;

                createVolumeMaterial(volume, volumeItem);

                volumeItem.sliceFrameX = createSliceFrame(volumeItem);
                volumeItem.sliceFrameY = createSliceFrame(volumeItem);
                volumeItem.sliceFrameZ = createSliceFrame(volumeItem);

                if (volume->drawSliceFrames()) {
                    volumeItem.sliceFrameX->setVisible(true);
                    volumeItem.sliceFrameY->setVisible(true);
                    volumeItem.sliceFrameZ->setVisible(true);

                    QVector3D sliceIndices((float(volume->sliceIndexX()) + 0.5f)
                                                   / float(volume->textureWidth()) * 2.0
                                               - 1.0,
                                           (float(volume->sliceIndexY()) + 0.5f)
                                                   / float(volume->textureHeight()) * 2.0
                                               - 1.0,
                                           (float(volume->sliceIndexZ()) + 0.5f)
                                                   / float(volume->textureDepth()) * 2.0
                                               - 1.0);

                    volumeItem.sliceFrameX->setX(sliceIndices.x());
                    volumeItem.sliceFrameY->setY(-sliceIndices.y());
                    volumeItem.sliceFrameZ->setZ(-sliceIndices.z());

                    volumeItem.sliceFrameX->setRotation(QQuaternion::fromEulerAngles(0, 90, 0));
                    volumeItem.sliceFrameY->setRotation(QQuaternion::fromEulerAngles(90, 0, 0));

                    updateSliceFrameMaterials(volume, volumeItem);
                } else {
                    volumeItem.sliceFrameX->setVisible(false);
                    volumeItem.sliceFrameY->setVisible(false);
                    volumeItem.sliceFrameZ->setVisible(false);
                }
                volumeItem.drawSliceFrames = volume->drawSliceFrames();
                m_customItemList.insert(item, model);
            }
        } else {
            model->setSource(QUrl::fromLocalFile(item->meshFile()));
            QQmlListReference materialsRef(model, "materials");
            QQuick3DPrincipledMaterial *material = static_cast<QQuick3DPrincipledMaterial *>(
                materialsRef.at(0));
            QQuick3DTexture *texture = material->baseColorMap();
            if (!texture) {
                texture = new QQuick3DTexture();
                texture->setParent(model);
                texture->setParentItem(model);
                material->setBaseColorMap(texture);
            }
            if (!item->textureFile().isEmpty()) {
                texture->setSource(QUrl::fromLocalFile(item->textureFile()));
            } else {
                QImage textureImage = customTextureImage(item);
                textureImage.convertTo(QImage::Format_RGBA32FPx4);
                QQuick3DTextureData *textureData = texture->textureData();
                if (!textureData) {
                    textureData = new QQuick3DTextureData();
                    textureData->setParent(texture);
                    textureData->setParentItem(texture);
                    textureData->setFormat(QQuick3DTextureData::RGBA32F);
                    texture->setTextureData(textureData);
                }
                textureData->setSize(textureImage.size());
                textureData->setTextureData(
                    QByteArray(reinterpret_cast<const char *>(textureImage.bits()),
                               textureImage.sizeInBytes()));
            }
            model->setRotation(item->rotation());
            model->setVisible(item->isVisible());
        }
        ++itemIterator;
    }
}

void QQuickGraphsItem::updateCustomLabelsRotation()
{
    auto labelIterator = m_customLabelList.constBegin();
    while (labelIterator != m_customLabelList.constEnd()) {
        QCustom3DLabel *label = labelIterator.key();
        QQuick3DNode *customLabel = labelIterator.value();
        QQuaternion rotation = label->rotation();
        if (label->isFacingCamera())
            rotation = Utils::calculateRotation(QVector3D(-m_yRotation, -m_xRotation, 0));
        customLabel->setRotation(rotation);
        ++labelIterator;
    }
}

int QQuickGraphsItem::msaaSamples() const
{
    if (m_renderMode == QAbstract3DGraph::RenderingMode::Indirect)
        return m_samples;
    else
        return m_windowSamples;
}

void QQuickGraphsItem::setMsaaSamples(int samples)
{
    if (m_renderMode != QAbstract3DGraph::RenderingMode::Indirect) {
        qWarning("Multisampling cannot be adjusted in this render mode");
    } else if (m_samples != samples) {
        m_samples = samples;
        setAntialiasing(m_samples > 0);
        auto sceneEnv = environment();
        sceneEnv->setAntialiasingMode(
            m_samples > 0 ? QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues::MSAA
                          : QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues::NoAA);
        switch (m_samples) {
        case 0:
            // no-op
            break;
        case 2:
            sceneEnv->setAntialiasingQuality(
                QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues::Medium);
            break;
        case 4:
            sceneEnv->setAntialiasingQuality(
                QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues::High);
            break;
        case 8:
            sceneEnv->setAntialiasingQuality(
                QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues::VeryHigh);
            break;
        default:
            qWarning("Invalid multisampling sample number, using 4x instead");
            sceneEnv->setAntialiasingQuality(
                QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues::High);
            m_samples = 4;
            break;
        }
        emit msaaSamplesChanged(m_samples);
        update();
    }
}

void QQuickGraphsItem::handleWindowChanged(/*QQuickWindow *window*/)
{
    auto window = QQuick3DObjectPrivate::get(rootNode())->sceneManager->window();
    checkWindowList(window);
    if (!window)
        return;

#if defined(Q_OS_MACOS)
    bool previousVisibility = window->isVisible();
    // Enable touch events for Mac touchpads
    window->setVisible(true);
    typedef void *(*EnableTouch)(QWindow *, bool);
    EnableTouch enableTouch = (EnableTouch) QGuiApplication::platformNativeInterface()
                                  ->nativeResourceFunctionForIntegration("registertouchwindow");
    if (enableTouch)
        enableTouch(window, true);
    window->setVisible(previousVisibility);
#endif

    connect(window, &QObject::destroyed, this, &QQuickGraphsItem::windowDestroyed);

    int oldWindowSamples = m_windowSamples;
    m_windowSamples = window->format().samples();
    if (m_windowSamples < 0)
        m_windowSamples = 0;

    connect(window, &QQuickWindow::beforeSynchronizing, this, &QQuickGraphsItem::synchData);

    if (m_renderMode == QAbstract3DGraph::RenderingMode::DirectToBackground) {
        setAntialiasing(m_windowSamples > 0);
        if (m_windowSamples != oldWindowSamples)
            emit msaaSamplesChanged(m_windowSamples);
    }

    connect(this, &QQuickGraphsItem::needRender, window, &QQuickWindow::update);
    // Force camera update before rendering the first frame
    // to workaround a Quick3D device pixel ratio bug
    connect(window, &QQuickWindow::beforeRendering, this, [this, window]() {
        m_oCamera->setClipNear(0.001f);
        disconnect(window, &QQuickWindow::beforeRendering, this, nullptr);
    });
    updateWindowParameters();

#if defined(Q_OS_IOS)
    // Scenegraph render cycle in iOS sometimes misses update after
    // beforeSynchronizing signal. This ensures we don't end up displaying the
    // graph without any data, in case update is skipped after synchData.
    QTimer::singleShot(0, window, SLOT(update()));
#endif
}

void QQuickGraphsItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    m_cachedGeometry = newGeometry;
    updateWindowParameters();
}

void QQuickGraphsItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuick3DViewport::itemChange(change, value);
    updateWindowParameters();
}

void QQuickGraphsItem::updateWindowParameters()
{
    const QMutexLocker locker(&m_mutex);
    // Update the device pixel ratio, window size and bounding box
    QQuickWindow *win = window();
    if (win) {
        if (win->devicePixelRatio() != scene()->devicePixelRatio()) {
            scene()->setDevicePixelRatio(win->devicePixelRatio());
            win->update();
        }

        bool directRender = m_renderMode == QAbstract3DGraph::RenderingMode::DirectToBackground;
        QSize windowSize;

        if (directRender)
            windowSize = win->size();
        else
            windowSize = m_cachedGeometry.size().toSize();

        if (windowSize != scene()->d_func()->windowSize()) {
            scene()->d_func()->setWindowSize(windowSize);
            win->update();
        }

        if (directRender) {
            // Origin mapping is needed when rendering directly to background
            QPointF point = QQuickItem::mapToScene(QPointF(0.0, 0.0));
            scene()->d_func()->setViewport(QRect(point.x() + 0.5f,
                                                 point.y() + 0.5f,
                                                 m_cachedGeometry.width() + 0.5f,
                                                 m_cachedGeometry.height() + 0.5f));
        } else {
            // No translation needed when rendering to FBO
            scene()->d_func()->setViewport(
                QRect(0.0, 0.0, m_cachedGeometry.width() + 0.5f, m_cachedGeometry.height() + 0.5f));
        }
    }

    if (m_sliceView && m_sliceView->isVisible() && isSliceOrthoProjection()) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(
            m_sliceView->camera());
        const float magnificationScaleFactor = .16f; // this controls the size of the slice view
        const float magnification = scale * magnificationScaleFactor;
        camera->setHorizontalMagnification(magnification);
        camera->setVerticalMagnification(magnification);
    }
}

void QQuickGraphsItem::handleSelectionModeChange(QAbstract3DGraph::SelectionFlags mode)
{
    emit selectionModeChanged(mode);
}

void QQuickGraphsItem::handleShadowQualityChange(QAbstract3DGraph::ShadowQuality quality)
{
    emit shadowQualityChanged(quality);
}

void QQuickGraphsItem::handleSelectedElementChange(QAbstract3DGraph::ElementType type)
{
    m_clickedType = type;
    emit selectedElementChanged(type);
}

void QQuickGraphsItem::handleOptimizationHintChange(QAbstract3DGraph::OptimizationHint hint)
{
    Q_UNUSED(hint)
}

QAbstract3DInputHandler *QQuickGraphsItem::inputHandler() const
{
    return m_activeInputHandler;
}

void QQuickGraphsItem::setInputHandler(QAbstract3DInputHandler *inputHandler)
{
    setActiveInputHandler(inputHandler);
}

void QQuickGraphsItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_activeInputHandler)
        m_activeInputHandler->mouseDoubleClickEvent(event);
}

void QQuickGraphsItem::touchEvent(QTouchEvent *event)
{
    if (m_activeInputHandler)
        m_activeInputHandler->touchEvent(event);
    handleTouchEvent(event);
    window()->update();
}

void QQuickGraphsItem::mousePressEvent(QMouseEvent *event)
{
    QPoint mousePos = event->pos();
    handleMousePressedEvent(event);
    if (m_activeInputHandler)
        m_activeInputHandler->mousePressEvent(event, mousePos);
}

void QQuickGraphsItem::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint mousePos = event->pos();
    if (m_activeInputHandler)
        m_activeInputHandler->mouseReleaseEvent(event, mousePos);
}

void QQuickGraphsItem::mouseMoveEvent(QMouseEvent *event)
{
    QPoint mousePos = event->pos();
    if (m_activeInputHandler)
        m_activeInputHandler->mouseMoveEvent(event, mousePos);
}

#if QT_CONFIG(wheelevent)
void QQuickGraphsItem::wheelEvent(QWheelEvent *event)
{
    if (m_activeInputHandler)
        m_activeInputHandler->wheelEvent(event);
}
#endif

void QQuickGraphsItem::checkWindowList(QQuickWindow *window)
{
    QQuickWindow *oldWindow = m_graphWindowList.value(this);
    m_graphWindowList[this] = window;

    if (oldWindow != window && oldWindow) {
        QObject::disconnect(oldWindow,
                            &QObject::destroyed,
                            this,
                            &QQuickGraphsItem::windowDestroyed);
        QObject::disconnect(oldWindow,
                            &QQuickWindow::beforeSynchronizing,
                            this,
                            &QQuickGraphsItem::synchData);
        QObject::disconnect(this, &QQuickGraphsItem::needRender, oldWindow, &QQuickWindow::update);
    }

    QList<QQuickWindow *> windowList;

    const auto keys = m_graphWindowList.keys();
    for (const auto &graph : keys) {
        if (graph->m_renderMode == QAbstract3DGraph::RenderingMode::DirectToBackground)
            windowList.append(m_graphWindowList.value(graph));
    }

    if (!window) {
        m_graphWindowList.remove(this);
        return;
    }
}

void QQuickGraphsItem::setMeasureFps(bool enable)
{
    if (m_measureFps != enable) {
        m_measureFps = enable;
        if (enable) {
            QObject::connect(renderStats(),
                             &QQuick3DRenderStats::fpsChanged,
                             this,
                             &QQuickGraphsItem::handleFpsChanged);
            emitNeedRender();
        } else {
            QObject::disconnect(renderStats(), 0, this, 0);
        }
    }
}

bool QQuickGraphsItem::measureFps() const
{
    return m_measureFps;
}

int QQuickGraphsItem::currentFps() const
{
    return m_currentFps;
}

void QQuickGraphsItem::createInitialInputHandler()
{
    QAbstract3DInputHandler *inputHandler;
    inputHandler = new QTouch3DInputHandler(this);
    inputHandler->d_func()->m_isDefaultHandler = true;
    setActiveInputHandler(inputHandler);
}

void QQuickGraphsItem::setOrthoProjection(bool enable)
{
    if (enable != m_useOrthoProjection) {
        m_useOrthoProjection = enable;
        m_changeTracker.projectionChanged = true;
        emit orthoProjectionChanged(m_useOrthoProjection);
        // If changed to ortho, disable shadows
        if (m_useOrthoProjection)
            doSetShadowQuality(QAbstract3DGraph::ShadowQuality::None);
        emitNeedRender();
    }
}

bool QQuickGraphsItem::isOrthoProjection() const
{
    return m_useOrthoProjection;
}

QAbstract3DGraph::ElementType QQuickGraphsItem::selectedElement() const
{
    return m_clickedType;
}

void QQuickGraphsItem::setAspectRatio(qreal ratio)
{
    if (m_aspectRatio != ratio) {
        m_aspectRatio = ratio;
        m_changeTracker.aspectRatioChanged = true;
        emit aspectRatioChanged(m_aspectRatio);
        m_isDataDirty = true;
        emitNeedRender();
    }
}

qreal QQuickGraphsItem::aspectRatio() const
{
    return m_aspectRatio;
}

void QQuickGraphsItem::setOptimizationHint(QAbstract3DGraph::OptimizationHint hint)
{
    if (hint != m_optimizationHint) {
        m_optimizationHint = hint;
        m_changeTracker.optimizationHintChanged = true;
        m_isDataDirty = true;
        handleOptimizationHintChange(m_optimizationHint);
        emit optimizationHintChanged(hint);
        emitNeedRender();
    }
}

QAbstract3DGraph::OptimizationHint QQuickGraphsItem::optimizationHint() const
{
    return m_optimizationHint;
}

void QQuickGraphsItem::setPolar(bool enable)
{
    if (enable != m_isPolar) {
        if (m_graphType == QAbstract3DSeries::SeriesType::Bar)
            qWarning("Polar type with bars is not supported.");
        m_isPolar = enable;
        m_changeTracker.polarChanged = true;
        setVerticalSegmentLine(!m_isPolar);
        m_isDataDirty = true;
        emit polarChanged(m_isPolar);
        emitNeedRender();
    }
}

bool QQuickGraphsItem::isPolar() const
{
    return m_isPolar;
}

void QQuickGraphsItem::setRadialLabelOffset(float offset)
{
    if (m_radialLabelOffset != offset) {
        m_radialLabelOffset = offset;
        m_changeTracker.radialLabelOffsetChanged = true;
        emit radialLabelOffsetChanged(m_radialLabelOffset);
        emitNeedRender();
    }
}

float QQuickGraphsItem::radialLabelOffset() const
{
    return m_radialLabelOffset;
}

void QQuickGraphsItem::setHorizontalAspectRatio(qreal ratio)
{
    if (m_horizontalAspectRatio != ratio) {
        m_horizontalAspectRatio = ratio;
        m_changeTracker.horizontalAspectRatioChanged = true;
        emit horizontalAspectRatioChanged(m_horizontalAspectRatio);
        m_isDataDirty = true;
        emitNeedRender();
    }
}

qreal QQuickGraphsItem::horizontalAspectRatio() const
{
    return m_horizontalAspectRatio;
}

void QQuickGraphsItem::setLocale(const QLocale &locale)
{
    if (m_locale != locale) {
        m_locale = locale;

        // Value axis formatters need to be updated
        QValue3DAxis *axis = qobject_cast<QValue3DAxis *>(m_axisX);
        if (axis)
            axis->formatter()->setLocale(m_locale);
        axis = qobject_cast<QValue3DAxis *>(m_axisY);
        if (axis)
            axis->formatter()->setLocale(m_locale);
        axis = qobject_cast<QValue3DAxis *>(m_axisZ);
        if (axis)
            axis->formatter()->setLocale(m_locale);
        emit localeChanged(m_locale);
    }
}

QLocale QQuickGraphsItem::locale() const
{
    return m_locale;
}

QVector3D QQuickGraphsItem::queriedGraphPosition() const
{
    return m_queriedGraphPosition;
}

void QQuickGraphsItem::setMargin(qreal margin)
{
    if (m_margin != margin) {
        m_margin = margin;
        m_changeTracker.marginChanged = true;
        emit marginChanged(margin);
        emitNeedRender();
    }
}

qreal QQuickGraphsItem::margin() const
{
    return m_margin;
}

QQuick3DNode *QQuickGraphsItem::rootNode() const
{
    return QQuick3DViewport::scene();
}

void QQuickGraphsItem::changeLabelBackgroundColor(QQuick3DRepeater *repeater, const QColor &color)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("backgroundColor", color);
    }
}

void QQuickGraphsItem::changeLabelBackgroundEnabled(QQuick3DRepeater *repeater, const bool &enabled)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("backgroundEnabled", enabled);
    }
}

void QQuickGraphsItem::changeLabelBorderEnabled(QQuick3DRepeater *repeater, const bool &enabled)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("borderEnabled", enabled);
    }
}

void QQuickGraphsItem::changeLabelTextColor(QQuick3DRepeater *repeater, const QColor &color)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("labelTextColor", color);
    }
}

void QQuickGraphsItem::changeLabelFont(QQuick3DRepeater *repeater, const QFont &font)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("labelFont", font);
    }
}

void QQuickGraphsItem::changeLabelsEnabled(QQuick3DRepeater *repeater, const bool &enabled)
{
    int count = repeater->count();
    for (int i = 0; i < count; i++) {
        auto label = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        label->setProperty("visible", enabled);
    }
}

void QQuickGraphsItem::changeGridLineColor(QQuick3DRepeater *repeater, const QColor &color)
{
    for (int i = 0; i < repeater->count(); i++) {
        auto lineNode = static_cast<QQuick3DNode *>(repeater->objectAt(i));
        lineNode->setProperty("lineColor", color);
    }
}

void QQuickGraphsItem::updateTitleLabels()
{
    if (m_changeTracker.axisXTitleVisibilityChanged) {
        m_titleLabelX->setVisible(axisX()->isTitleVisible());
        m_changeTracker.axisXTitleVisibilityChanged = false;
    }

    if (m_changeTracker.axisYTitleVisibilityChanged) {
        m_titleLabelY->setVisible(axisY()->isTitleVisible());
        m_changeTracker.axisYTitleVisibilityChanged = false;
    }

    if (m_changeTracker.axisZTitleVisibilityChanged) {
        m_titleLabelZ->setVisible(axisZ()->isTitleVisible());
        m_changeTracker.axisZTitleVisibilityChanged = false;
    }

    if (m_changeTracker.axisXTitleChanged) {
        m_titleLabelX->setProperty("labelText", axisX()->title());
        m_changeTracker.axisXTitleChanged = false;
    }

    if (m_changeTracker.axisYTitleChanged) {
        m_titleLabelY->setProperty("labelText", axisY()->title());
        m_changeTracker.axisYTitleChanged = false;
    }

    if (m_changeTracker.axisZTitleChanged) {
        m_titleLabelZ->setProperty("labelText", axisZ()->title());
        m_changeTracker.axisZTitleChanged = false;
    }
}

void QQuickGraphsItem::updateSelectionMode(QAbstract3DGraph::SelectionFlags newMode)
{
    Q_UNUSED(newMode);

    if (m_sliceView && m_sliceView->isVisible())
        updateSliceGraph();
}

bool QQuickGraphsItem::doPicking(const QPointF &point)
{
    if (m_activeInputHandler->d_func()->m_inputState
        == QAbstract3DInputHandlerPrivate::InputState::Selecting) {
        QList<QQuick3DPickResult> results = pickAll(point.x(), point.y());
        if (!m_customItemList.isEmpty()) {
            // Try to pick custom item only
            for (const auto &result : results) {
                QCustom3DItem *customItem = m_customItemList.key(result.objectHit(), nullptr);

                if (customItem) {
                    int selectedIndex = m_customItems.indexOf(customItem);
                    m_selectedCustomItemIndex = selectedIndex;
                    handleSelectedElementChange(QAbstract3DGraph::ElementType::CustomItem);
                    // Don't allow picking in subclasses if custom item is picked
                    return false;
                }
            }
        }

        for (const auto &result : results) {
            if (!result.objectHit())
                continue;
            QString objName = result.objectHit()->objectName();
            if (objName.contains(QStringLiteral("ElementAxisXLabel"))) {
                for (int i = 0; i < repeaterX()->count(); i++) {
                    auto obj = static_cast<QQuick3DNode *>(repeaterX()->objectAt(i));
                    if (result.objectHit() == obj)
                        m_selectedLabelIndex = i;
                }
                handleSelectedElementChange(QAbstract3DGraph::ElementType::AxisXLabel);
                break;
            } else if (objName.contains(QStringLiteral("ElementAxisYLabel"))) {
                handleSelectedElementChange(QAbstract3DGraph::ElementType::AxisYLabel);
                break;
            } else if (objName.contains(QStringLiteral("ElementAxisZLabel"))) {
                for (int i = 0; i < repeaterX()->count(); i++) {
                    auto obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
                    if (result.objectHit() == obj)
                        m_selectedLabelIndex = i;
                }
                handleSelectedElementChange(QAbstract3DGraph::ElementType::AxisZLabel);
                break;
            } else {
                continue;
            }
        }
        return true;
    }

    return false;
}

void QQuickGraphsItem::minimizeMainGraph()
{
    QQuickItem *anchor = QQuickItemPrivate::get(this)->anchors()->fill();
    if (anchor)
        QQuickItemPrivate::get(this)->anchors()->resetFill();

    const float minimizedSize = .2f;
    setWidth(parentItem()->width() * minimizedSize);
    setHeight(parentItem()->height() * minimizedSize);
}

void QQuickGraphsItem::updateSliceGraph()
{
    if (!m_sliceView || !m_sliceActivatedChanged)
        return;

    if (m_sliceView->isVisible()) {
        setWidth(parentItem()->width());
        setHeight(parentItem()->height());

        m_sliceView->setVisible(false);
        setSlicingActive(false);
    } else {
        minimizeMainGraph();
        m_sliceView->setVisible(true);
        updateSliceGrid();
        updateSliceLabels();
        setSlicingActive(true);
    }

    m_sliceActivatedChanged = false;
}

void QQuickGraphsItem::addInputHandler(QAbstract3DInputHandler *inputHandler)
{
    Q_ASSERT(inputHandler);
    QQuickGraphsItem *owner = qobject_cast<QQuickGraphsItem *>(inputHandler->parent());
    if (owner != this) {
        Q_ASSERT_X(!owner,
                   "addInputHandler",
                   "Input handler already attached to another component.");
        inputHandler->setParent(this);
    }

    if (!m_inputHandlers.contains(inputHandler))
        m_inputHandlers.append(inputHandler);
}

void QQuickGraphsItem::releaseInputHandler(QAbstract3DInputHandler *inputHandler)
{
    if (inputHandler && m_inputHandlers.contains(inputHandler)) {
        // Clear the default status from released default input handler
        if (inputHandler->d_func()->m_isDefaultHandler)
            inputHandler->d_func()->m_isDefaultHandler = false;

        // If the input handler is in use, remove it
        if (m_activeInputHandler == inputHandler)
            setActiveInputHandler(nullptr);

        m_inputHandlers.removeAll(inputHandler);
        inputHandler->setParent(nullptr);
    }
}

void QQuickGraphsItem::setActiveInputHandler(QAbstract3DInputHandler *inputHandler)
{
    if (inputHandler == m_activeInputHandler)
        return;

    // If existing input handler is the default input handler, delete it
    if (m_activeInputHandler) {
        if (m_activeInputHandler->d_func()->m_isDefaultHandler) {
            m_inputHandlers.removeAll(m_activeInputHandler);
            delete m_activeInputHandler;
        } else {
            // Disconnect the old input handler
            m_activeInputHandler->setScene(nullptr);
            QObject::disconnect(m_activeInputHandler, nullptr, this, nullptr);
            QObject::disconnect(m_activeInputHandler,
                                &QAbstract3DInputHandler::positionChanged,
                                this,
                                &QQuickGraphsItem::doPicking);
        }
    }

    // Assume ownership and connect to this graphs scene
    if (inputHandler)
        addInputHandler(inputHandler);

    m_activeInputHandler = inputHandler;

    if (m_activeInputHandler) {
        m_activeInputHandler->setItem(this);
        m_activeInputHandler->setScene(scene());

        // Connect the input handler
        QObject::connect(m_activeInputHandler,
                         &QAbstract3DInputHandler::inputViewChanged,
                         this,
                         &QQuickGraphsItem::handleInputViewChanged);
        QObject::connect(m_activeInputHandler,
                         &QAbstract3DInputHandler::positionChanged,
                         this,
                         &QQuickGraphsItem::handleInputPositionChanged);
        QObject::connect(m_activeInputHandler,
                         &QAbstract3DInputHandler::positionChanged,
                         this,
                         &QQuickGraphsItem::doPicking);
    }

    // Notify change of input handler
    emit inputHandlerChanged(m_activeInputHandler);
}

void QQuickGraphsItem::windowDestroyed(QObject *obj)
{
    // Remove destroyed window from window lists
    QQuickWindow *win = static_cast<QQuickWindow *>(obj);
    QQuickWindow *oldWindow = m_graphWindowList.value(this);

    if (win == oldWindow)
        m_graphWindowList.remove(this);
}

QQmlComponent *QQuickGraphsItem::createRepeaterDelegateComponent(const QString &fileName)
{
    QQmlComponent component(qmlEngine(this), fileName);
    return qobject_cast<QQmlComponent *>(component.create());
}

QQuick3DRepeater *QQuickGraphsItem::createRepeater(QQuick3DNode *parent)
{
    auto engine = qmlEngine(this);
    QQmlComponent repeaterComponent(engine);
    repeaterComponent.setData("import QtQuick3D; Repeater3D{}", QUrl());
    auto repeater = qobject_cast<QQuick3DRepeater *>(repeaterComponent.create());
    repeater->setParent(parent ? parent : graphNode());
    repeater->setParentItem(parent ? parent : graphNode());
    return repeater;
}

QQuick3DNode *QQuickGraphsItem::createTitleLabel(QQuick3DNode *parent)
{
    auto engine = qmlEngine(this);
    QQmlComponent comp(engine, QStringLiteral(":/axis/TitleLabel"));
    auto titleLabel = qobject_cast<QQuick3DNode *>(comp.create());
    titleLabel->setParent(parent ? parent : graphNode());
    titleLabel->setParentItem(parent ? parent : graphNode());
    titleLabel->setVisible(false);
    titleLabel->setScale(m_labelScale);
    return titleLabel;
}

void QQuickGraphsItem::createItemLabel()
{
    auto engine = qmlEngine(this);
    QQmlComponent comp(engine, QStringLiteral(":/axis/ItemLabel"));
    m_itemLabel = qobject_cast<QQuickItem *>(comp.create());
    m_itemLabel->setParent(this);
    m_itemLabel->setParentItem(this);
    m_itemLabel->setVisible(false);
}

QQuick3DCustomMaterial *QQuickGraphsItem::createQmlCustomMaterial(const QString &fileName)
{
    QQmlComponent component(qmlEngine(this), fileName);
    QQuick3DCustomMaterial *material = qobject_cast<QQuick3DCustomMaterial *>(component.create());
    return material;
}

QQuick3DPrincipledMaterial *QQuickGraphsItem::createPrincipledMaterial()
{
    QQmlComponent component(qmlEngine(this));
    component.setData("import QtQuick3D; PrincipledMaterial{}", QUrl());
    return qobject_cast<QQuick3DPrincipledMaterial *>(component.create());
}

QAbstract3DGraph::CameraPreset QQuickGraphsItem::cameraPreset() const
{
    return m_activePreset;
}

void QQuickGraphsItem::setCameraPreset(QAbstract3DGraph::CameraPreset preset)
{
    switch (preset) {
    case QAbstract3DGraph::CameraPreset::FrontLow: {
        m_xRotation = 0.0f;
        m_yRotation = 0.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::Front: {
        m_xRotation = 0.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::FrontHigh: {
        m_xRotation = 0.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::LeftLow: {
        m_xRotation = 90.0f;
        m_yRotation = 0.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::Left: {
        m_xRotation = 90.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::LeftHigh: {
        m_xRotation = 90.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::RightLow: {
        m_xRotation = -90.0f;
        m_yRotation = 0.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::Right: {
        m_xRotation = -90.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::RightHigh: {
        m_xRotation = -90.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::BehindLow: {
        m_xRotation = 180.0f;
        m_yRotation = 0.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::Behind: {
        m_xRotation = 180.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::BehindHigh: {
        m_xRotation = 180.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::IsometricLeft: {
        m_xRotation = 45.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::IsometricLeftHigh: {
        m_xRotation = 45.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::IsometricRight: {
        m_xRotation = -45.0f;
        m_yRotation = 22.5f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::IsometricRightHigh: {
        m_xRotation = -45.0f;
        m_yRotation = 45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::DirectlyAbove: {
        m_xRotation = 0.0f;
        m_yRotation = 90.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::DirectlyAboveCW45: {
        m_xRotation = -45.0f;
        m_yRotation = 90.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::DirectlyAboveCCW45: {
        m_xRotation = 45.0f;
        m_yRotation = 90.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::FrontBelow: {
        m_xRotation = 0.0f;
        m_yRotation = -45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::LeftBelow: {
        m_xRotation = 90.0f;
        m_yRotation = -45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::RightBelow: {
        m_xRotation = -90.0f;
        m_yRotation = -45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::BehindBelow: {
        m_xRotation = 180.0f;
        m_yRotation = -45.0f;
        break;
    }
    case QAbstract3DGraph::CameraPreset::DirectlyBelow: {
        m_xRotation = 0.0f;
        m_yRotation = -90.0f;
        break;
    }
    default:
        preset = QAbstract3DGraph::CameraPreset::NoPreset;
        break;
    }

    // All presets target the center of the graph
    setCameraTargetPosition(QVector3D());

    if (m_activePreset != preset) {
        m_activePreset = preset;
        emit cameraPresetChanged(preset);
    }
    if (camera()) {
        updateCamera();
        connect(this, &QQuickGraphsItem::cameraXRotationChanged, m_scene, &Q3DScene::needRender);
        connect(this, &QQuickGraphsItem::cameraYRotationChanged, m_scene, &Q3DScene::needRender);
        connect(this, &QQuickGraphsItem::cameraZoomLevelChanged, m_scene, &Q3DScene::needRender);
    }
}

void QQuickGraphsItem::setCameraXRotation(float rotation)
{
    if (m_wrapXRotation)
        rotation = Utils::wrapValue(rotation, m_minXRotation, m_maxXRotation);
    else
        rotation = qBound(m_minXRotation, rotation, m_maxXRotation);
    if (rotation != m_xRotation) {
        m_xRotation = rotation;
        emit cameraXRotationChanged(m_xRotation);
    }
}

void QQuickGraphsItem::setCameraYRotation(float rotation)
{
    if (m_wrapYRotation)
        rotation = Utils::wrapValue(rotation, m_minYRotation, m_maxYRotation);
    else
        rotation = qBound(m_minYRotation, rotation, m_maxYRotation);
    if (rotation != m_yRotation) {
        m_yRotation = rotation;
        emit cameraYRotationChanged(m_yRotation);
    }
}

void QQuickGraphsItem::setMinCameraXRotation(float rotation)
{
    if (m_minXRotation == rotation)
        return;

    m_minXRotation = rotation;
    emit minCameraXRotationChanged(rotation);
}

void QQuickGraphsItem::setMaxCameraXRotation(float rotation)
{
    if (m_maxXRotation == rotation)
        return;

    m_maxXRotation = rotation;
    emit maxCameraXRotationChanged(rotation);
}

void QQuickGraphsItem::setMinCameraYRotation(float rotation)
{
    if (m_minYRotation == rotation)
        return;

    m_minYRotation = rotation;
    emit minCameraYRotationChanged(rotation);
}

void QQuickGraphsItem::setMaxCameraYRotation(float rotation)
{
    if (m_maxYRotation == rotation)
        return;

    m_maxYRotation = rotation;
    emit maxCameraYRotationChanged(rotation);
}

void QQuickGraphsItem::setCameraZoomLevel(float level)
{
    if (m_zoomLevel == level)
        return;

    m_zoomLevel = level;
    emit cameraZoomLevelChanged(level);
}

void QQuickGraphsItem::setMinCameraZoomLevel(float level)
{
    if (m_minZoomLevel == level)
        return;

    m_minZoomLevel = level;
    emit minCameraZoomLevelChanged(level);
}

void QQuickGraphsItem::setMaxCameraZoomLevel(float level)
{
    if (m_maxZoomLevel == level)
        return;

    m_maxZoomLevel = level;
    emit maxCameraZoomLevelChanged(level);
}

void QQuickGraphsItem::setCameraTargetPosition(const QVector3D &target)
{
    if (m_requestedTarget == target)
        return;

    m_requestedTarget = target;
    emit cameraTargetPositionChanged(target);
}

void QQuickGraphsItem::setCameraPosition(float horizontal, float vertical, float zoom)
{
    setCameraZoomLevel(zoom);
    setCameraXRotation(horizontal);
    setCameraYRotation(vertical);
}

bool QQuickGraphsItem::event(QEvent *event)
{
    return QQuickItem::event(event);
}

void QQuickGraphsItem::createSliceView()
{
    if (m_sliceView)
        return;

    connect(parentItem(),
            &QQuickItem::widthChanged,
            this,
            &QQuickGraphsItem::handleParentWidthChange);
    connect(parentItem(),
            &QQuickItem::heightChanged,
            this,
            &QQuickGraphsItem::handleParentHeightChange);

    m_sliceView = new QQuick3DViewport();
    m_sliceView->setParent(parent());
    m_sliceView->setParentItem(parentItem());
    m_sliceView->setVisible(false);

    m_sliceView->bindableHeight().setBinding([&] { return parentItem()->height(); });
    m_sliceView->bindableWidth().setBinding([&] { return parentItem()->width(); });

    auto scene = m_sliceView->scene();

    createSliceCamera();

    // auto gridDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_labelDelegate.reset(new QQmlComponent(qmlEngine(this), QStringLiteral(":/axis/AxisLabel")));

    m_sliceGridGeometryModel = new QQuick3DModel(scene);

    auto sliceGridGeometry = new QQuick3DGeometry(m_sliceGridGeometryModel);
    sliceGridGeometry->setStride(sizeof(QVector3D));
    sliceGridGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
    sliceGridGeometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                                    0,
                                    QQuick3DGeometry::Attribute::F32Type);
    m_sliceGridGeometryModel->setGeometry(sliceGridGeometry);

    QQmlListReference gridMaterialRef(m_sliceGridGeometryModel, "materials");
    auto gridMaterial = new QQuick3DPrincipledMaterial(m_sliceGridGeometryModel);
    gridMaterial->setLighting(QQuick3DPrincipledMaterial::Lighting::NoLighting);
    gridMaterial->setCullMode(QQuick3DMaterial::CullMode::BackFaceCulling);
    gridMaterial->setBaseColor(Qt::red);
    gridMaterialRef.append(gridMaterial);

    m_sliceHorizontalLabelRepeater = createRepeater(scene);
    m_sliceHorizontalLabelRepeater->setDelegate(m_labelDelegate.get());

    m_sliceVerticalLabelRepeater = createRepeater(scene);
    m_sliceVerticalLabelRepeater->setDelegate(m_labelDelegate.get());

    m_sliceHorizontalTitleLabel = createTitleLabel(scene);
    m_sliceHorizontalTitleLabel->setVisible(true);

    m_sliceVerticalTitleLabel = createTitleLabel(scene);
    m_sliceVerticalTitleLabel->setVisible(true);

    m_sliceItemLabel = createTitleLabel(scene);
    m_sliceItemLabel->setVisible(false);
}

void QQuickGraphsItem::createSliceCamera()
{
    if (isSliceOrthoProjection()) {
        auto camera = new QQuick3DOrthographicCamera(sliceView()->scene());
        camera->setPosition(QVector3D(.0f, .0f, 20.0f));
        const float scale = qMin(sliceView()->width(), sliceView()->height());
        const float magnificationScaleFactor = 2 * window()->devicePixelRatio()
                                               * .08f; // this controls the size of the slice view
        const float magnification = scale * magnificationScaleFactor;
        camera->setHorizontalMagnification(magnification);
        camera->setVerticalMagnification(magnification);
        sliceView()->setCamera(camera);

        auto light = new QQuick3DDirectionalLight(sliceView()->scene());
        light->setParent(camera);
        light->setParentItem(camera);
    } else {
        auto camera = new QQuick3DPerspectiveCamera(sliceView()->scene());
        camera->setFieldOfViewOrientation(
            QQuick3DPerspectiveCamera::FieldOfViewOrientation::Vertical);
        camera->setClipNear(0.1f);
        camera->setClipFar(100.f);
        camera->setFieldOfView(35.f);
        camera->setPosition(QVector3D(.0f, .0f, 10.f));
        sliceView()->setCamera(camera);

        auto light = new QQuick3DDirectionalLight(sliceView()->scene());
        light->setParent(camera);
        light->setParentItem(camera);
        light->setAmbientColor(QColor::fromRgbF(1.f, 1.f, 1.f));
    }
}

void QQuickGraphsItem::updateSliceGrid()
{
    QAbstract3DAxis *horizontalAxis = nullptr;
    QAbstract3DAxis *verticalAxis = axisY();
    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float scale;
    float translate;

    float horizontalScale = 0.0f;

    if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow)) {
        horizontalAxis = axisX();
        horizontalScale = backgroundScale.x();
        scale = m_scaleWithBackground.x();
        translate = m_scaleWithBackground.x();
    } else if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn)) {
        horizontalAxis = axisZ();
        horizontalScale = backgroundScale.z();
        scale = m_scaleWithBackground.z();
        translate = m_scaleWithBackground.z();
    }

    if (horizontalAxis == nullptr) {
        qWarning("Invalid axis type");
        return;
    }
    int lineCount = 0;
    if (m_hasVerticalSegmentLine || isPolar()) {
        if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Value) {
            QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(horizontalAxis);
            lineCount += valueAxis->gridSize() + valueAxis->subGridSize();
        } else if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Category) {
            lineCount += horizontalAxis->labels().size();
        }
    }

    if (verticalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(verticalAxis);
        lineCount += valueAxis->gridSize() + valueAxis->subGridSize();
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        lineCount += verticalAxis->labels().size();
    }

    QByteArray vertices;
    vertices.resize(lineCount * 2 * sizeof(QVector3D));
    auto data = reinterpret_cast<QVector3D *>(vertices.data());
    float linePosX = .0f;
    float linePosY = .0f;
    const float linePosZ = -1.f; // Draw grid lines behind slice (especially for surface)

    float x0, x1;
    float y0, y1;
    y0 = -backgroundScale.y();
    y1 = backgroundScale.y();
    if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        auto axis = static_cast<QValue3DAxis *>(horizontalAxis);
        for (int i = 0; i < axis->subGridSize(); i++) {
            linePosX = axis->subGridPositionAt(i) * scale * 2.0f - translate;
            *data++ = QVector3D(linePosX, y0, linePosZ);
            *data++ = QVector3D(linePosX, y1, linePosZ);
        }
        for (int i = 0; i < axis->gridSize(); i++) {
            linePosX = axis->gridPositionAt(i) * scale * 2.0f - translate;
            *data++ = QVector3D(linePosX, y0, linePosZ);
            *data++ = QVector3D(linePosX, y1, linePosZ);
        }
    }

    scale = m_scaleWithBackground.y();
    translate = m_scaleWithBackground.y();

    x0 = horizontalScale * 1.1f;
    x1 = -horizontalScale * 1.1f;
    if (verticalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        auto axis = static_cast<QValue3DAxis *>(verticalAxis);
        for (int i = 0; i < axis->gridSize(); i++) {
            linePosY = axis->gridPositionAt(i) * scale * 2.0f - translate;
            *data++ = QVector3D(x0, linePosY, linePosZ);
            *data++ = QVector3D(x1, linePosY, linePosZ);
        }
        for (int i = 0; i < axis->subGridSize(); i++) {
            linePosY = axis->subGridPositionAt(i) * scale * 2.0f - translate;
            *data++ = QVector3D(x0, linePosY, linePosZ);
            *data++ = QVector3D(x1, linePosY, linePosZ);
        }
    } else if (verticalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        for (int i = 0; i < verticalAxis->labels().size(); i++) {
            linePosY = calculateCategoryGridLinePosition(verticalAxis, i);
            *data++ = QVector3D(x0, linePosY, linePosZ);
            *data++ = QVector3D(x1, linePosY, linePosZ);
        }
    }

    auto geometry = m_sliceGridGeometryModel->geometry();
    geometry->setVertexData(vertices);
    geometry->update();

    QQmlListReference materialRef(m_sliceGridGeometryModel, "materials");
    auto material = static_cast<QQuick3DPrincipledMaterial *>(materialRef.at(0));
    material->setBaseColor(theme()->gridLineColor());
}

void QQuickGraphsItem::updateSliceLabels()
{
    QAbstract3DAxis *horizontalAxis = nullptr;
    QAbstract3DAxis *verticalAxis = axisY();
    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float scale;
    float translate;

    if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow)) {
        horizontalAxis = axisX();
        scale = backgroundScale.x() - m_backgroundScaleMargin.x();
        translate = backgroundScale.x() - m_backgroundScaleMargin.x();
    } else if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn)) {
        horizontalAxis = axisZ();
        scale = backgroundScale.z() - m_backgroundScaleMargin.z();
        translate = backgroundScale.z() - m_backgroundScaleMargin.z();
    }

    if (horizontalAxis == nullptr) {
        qWarning("Invalid selection mode");
        return;
    }

    if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(horizontalAxis);
        m_sliceHorizontalLabelRepeater->model().clear();
        m_sliceHorizontalLabelRepeater->setModel(valueAxis->labels().size());
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        m_sliceHorizontalLabelRepeater->model().clear();
        m_sliceHorizontalLabelRepeater->setModel(horizontalAxis->labels().size());
    }

    if (verticalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(verticalAxis);
        m_sliceVerticalLabelRepeater->model().clear();
        m_sliceVerticalLabelRepeater->setModel(valueAxis->labels().size());
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        m_sliceVerticalLabelRepeater->model().clear();
        m_sliceVerticalLabelRepeater->setModel(verticalAxis->labels().size());
    }

    float textPadding = 12.0f;

    float labelsMaxWidth = float(findLabelsMaxWidth(horizontalAxis->labels())) + textPadding;
    QFontMetrics fm(theme()->font());
    float labelHeight = fm.height() + textPadding;

    float pointSize = theme()->font().pointSizeF();
    float scaleFactor = fontScaleFactor(pointSize) * pointSize;
    float fontRatio = labelsMaxWidth / labelHeight;
    QVector3D fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);

    float adjustment = labelsMaxWidth * scaleFactor;
    float yPos = backgroundScale.y() + adjustment;

    QVector3D labelTrans = QVector3D(0.0f, -yPos, 0.0f);
    QStringList labels = horizontalAxis->labels();
    QFont font = theme()->font();
    bool borderEnabled = theme()->isLabelBorderEnabled();
    QColor labelTextColor = theme()->labelTextColor();
    bool backgroundEnabled = theme()->isLabelBackgroundEnabled();
    QColor backgroundColor = theme()->labelBackgroundColor();

    if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        for (int i = 0; i < m_sliceHorizontalLabelRepeater->count(); i++) {
            auto obj = static_cast<QQuick3DNode *>(m_sliceHorizontalLabelRepeater->objectAt(i));
            // It is important to use the position of vertical grids so that they can be in the same
            // position when col/row ranges are updated.
            float linePosX = static_cast<QValue3DAxis *>(horizontalAxis)->gridPositionAt(i) * scale
                                 * 2.0f
                             - translate;
            labelTrans.setX(linePosX);
            labelTrans.setY(-yPos - adjustment);
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
            obj->setProperty("labelFont", font);
            obj->setProperty("borderEnabled", borderEnabled);
            obj->setProperty("labelTextColor", labelTextColor);
            obj->setProperty("backgroundEnabled", backgroundEnabled);
            obj->setProperty("backgroundColor", backgroundColor);
            obj->setEulerRotation(QVector3D(.0f, .0f, -45.0f));
        }
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        for (int i = 0; i < m_sliceHorizontalLabelRepeater->count(); i++) {
            labelTrans = calculateCategoryLabelPosition(horizontalAxis, labelTrans, i);
            labelTrans.setY(-yPos);
            if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
                labelTrans.setX(labelTrans.z());
            labelTrans.setZ(1.0f); // Bring the labels on top of bars and grid
            auto obj = static_cast<QQuick3DNode *>(m_sliceHorizontalLabelRepeater->objectAt(i));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
            obj->setProperty("labelFont", font);
            obj->setProperty("borderEnabled", borderEnabled);
            obj->setProperty("labelTextColor", labelTextColor);
            obj->setProperty("backgroundEnabled", backgroundEnabled);
            obj->setProperty("backgroundColor", backgroundColor);
            obj->setEulerRotation(QVector3D(0.0f, 0.0f, -60.0f));
        }
    }

    scale = backgroundScale.y() - m_backgroundScaleMargin.y();
    translate = backgroundScale.y() - m_backgroundScaleMargin.y();
    labels = verticalAxis->labels();
    labelsMaxWidth = float(findLabelsMaxWidth(labels)) + textPadding;
    // Since labelsMaxWidth changes for each axis, these needs to be recalculated for scaling.
    fontRatio = labelsMaxWidth / labelHeight;
    fontScaled.setX(scaleFactor * fontRatio);
    adjustment = labelsMaxWidth * scaleFactor;
    float xPos = 0.0f;
    if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
        xPos = backgroundScale.x() + (adjustment * 1.5f);
    else if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
        xPos = backgroundScale.z() + (adjustment * 1.5f);
    labelTrans = QVector3D(xPos, 0.0f, 0.0f);

    if (verticalAxis->type() == QAbstract3DAxis::AxisType::Value) {
        auto valueAxis = static_cast<QValue3DAxis *>(verticalAxis);
        for (int i = 0; i < m_sliceVerticalLabelRepeater->count(); i++) {
            auto obj = static_cast<QQuick3DNode *>(m_sliceVerticalLabelRepeater->objectAt(i));
            labelTrans.setY(valueAxis->labelPositionAt(i) * scale * 2.0f - translate);
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
            obj->setProperty("labelFont", font);
            obj->setProperty("borderEnabled", borderEnabled);
            obj->setProperty("labelTextColor", labelTextColor);
            obj->setProperty("backgroundEnabled", backgroundEnabled);
            obj->setProperty("backgroundColor", backgroundColor);
        }
    } else if (verticalAxis->type() == QAbstract3DAxis::AxisType::Category) {
        for (int i = 0; i < m_sliceVerticalLabelRepeater->count(); i++) {
            labelTrans = calculateCategoryLabelPosition(verticalAxis, labelTrans, i);
            auto obj = static_cast<QQuick3DNode *>(m_sliceVerticalLabelRepeater->objectAt(i));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
            obj->setProperty("labelFont", font);
            obj->setProperty("borderEnabled", borderEnabled);
            obj->setProperty("labelTextColor", labelTextColor);
            obj->setProperty("backgroundEnabled", backgroundEnabled);
            obj->setProperty("backgroundColor", backgroundColor);
        }
    }

    labelHeight = fm.height() + textPadding;
    float labelWidth = fm.horizontalAdvance(verticalAxis->title()) + textPadding;
    QVector3D vTitleScale = fontScaled;
    vTitleScale.setX(fontScaled.y() * labelWidth / labelHeight);
    adjustment = labelHeight * scaleFactor;
    if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
        xPos = backgroundScale.x() + adjustment;
    else if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
        xPos = backgroundScale.z() + adjustment;
    labelTrans = QVector3D(-(xPos + adjustment), 0.0f, 0.0f);

    if (!verticalAxis->title().isEmpty()) {
        m_sliceVerticalTitleLabel->setScale(vTitleScale);
        m_sliceVerticalTitleLabel->setPosition(labelTrans);
        m_sliceVerticalTitleLabel->setProperty("labelWidth", labelWidth);
        m_sliceVerticalTitleLabel->setProperty("labelHeight", labelHeight);
        m_sliceVerticalTitleLabel->setProperty("labelText", verticalAxis->title());
        m_sliceVerticalTitleLabel->setProperty("labelFont", font);
        m_sliceVerticalTitleLabel->setProperty("borderEnabled", borderEnabled);
        m_sliceVerticalTitleLabel->setProperty("labelTextColor", labelTextColor);
        m_sliceVerticalTitleLabel->setProperty("backgroundEnabled", backgroundEnabled);
        m_sliceVerticalTitleLabel->setProperty("backgroundColor", backgroundColor);
        m_sliceVerticalTitleLabel->setEulerRotation(QVector3D(.0f, .0f, 90.0f));
    } else {
        m_sliceVerticalTitleLabel->setVisible(false);
    }

    labelHeight = fm.height() + textPadding;
    labelWidth = fm.horizontalAdvance(horizontalAxis->title()) + textPadding;
    QVector3D hTitleScale = fontScaled;
    hTitleScale.setX(fontScaled.y() * labelWidth / labelHeight);
    adjustment = labelHeight * scaleFactor;
    yPos = backgroundScale.y() * 1.5f + (adjustment * 6.f);
    labelTrans = QVector3D(0.0f, -yPos, 0.0f);

    if (!horizontalAxis->title().isEmpty()) {
        m_sliceHorizontalTitleLabel->setScale(hTitleScale);
        m_sliceHorizontalTitleLabel->setPosition(labelTrans);
        m_sliceHorizontalTitleLabel->setProperty("labelWidth", labelWidth);
        m_sliceHorizontalTitleLabel->setProperty("labelHeight", labelHeight);
        m_sliceHorizontalTitleLabel->setProperty("labelText", horizontalAxis->title());
        m_sliceHorizontalTitleLabel->setProperty("labelFont", font);
        m_sliceHorizontalTitleLabel->setProperty("borderEnabled", borderEnabled);
        m_sliceHorizontalTitleLabel->setProperty("labelTextColor", labelTextColor);
        m_sliceHorizontalTitleLabel->setProperty("backgroundEnabled", backgroundEnabled);
        m_sliceHorizontalTitleLabel->setProperty("backgroundColor", backgroundColor);
    } else {
        m_sliceHorizontalTitleLabel->setVisible(false);
    }

    m_sliceItemLabel->setProperty("labelFont", font);
    m_sliceItemLabel->setProperty("borderEnabled", borderEnabled);
    m_sliceItemLabel->setProperty("labelTextColor", labelTextColor);
    m_sliceItemLabel->setProperty("backgroundEnabled", backgroundEnabled);
    m_sliceItemLabel->setProperty("backgroundColor", backgroundColor);
}

void QQuickGraphsItem::setUpCamera()
{
    m_pCamera = new QQuick3DPerspectiveCamera(rootNode());
    m_pCamera->setClipNear(0.001f);
    m_pCamera->setFieldOfView(45.0f);
    m_pCamera->setPosition(QVector3D(.0f, .0f, 5.f));

    auto cameraTarget = new QQuick3DNode(rootNode());
    cameraTarget->setParentItem(rootNode());

    setCameraTarget(cameraTarget);
    cameraTarget->setPosition(QVector3D(0, 0, 0));
    QQuick3DObjectPrivate::get(cameraTarget)
        ->refSceneManager(*QQuick3DObjectPrivate::get(rootNode())->sceneManager);

    m_pCamera->lookAt(cameraTarget);
    m_pCamera->setParent(cameraTarget);
    m_pCamera->setParentItem(cameraTarget);

    m_oCamera = new QQuick3DOrthographicCamera(rootNode());
    // Set clip near 0.0001f so that it can be set correct value to workaround
    // a Quick3D device pixel ratio bug
    m_oCamera->setClipNear(0.0001f);
    m_oCamera->setPosition(QVector3D(0.f, 0.f, 5.f));
    m_oCamera->setParent(cameraTarget);
    m_oCamera->setParentItem(cameraTarget);
    m_oCamera->lookAt(cameraTarget);

    auto useOrtho = isOrthoProjection();
    if (useOrtho)
        setCamera(m_oCamera);
    else
        setCamera(m_pCamera);
}

void QQuickGraphsItem::setUpLight()
{
    auto light = new QQuick3DDirectionalLight(rootNode());
    QQuick3DObjectPrivate::get(light)->refSceneManager(
        *QQuick3DObjectPrivate::get(rootNode())->sceneManager);
    light->setParent(camera());
    light->setParentItem(camera());
    m_light = light;
}

QT_END_NAMESPACE

void QQuickGraphsItem::setWrapCameraXRotation(bool wrap)
{
    if (m_wrapXRotation == wrap)
        return;
    m_wrapXRotation = wrap;
    emit wrapCameraXRotationChanged(wrap);
}

void QQuickGraphsItem::setWrapCameraYRotation(bool wrap)
{
    if (m_wrapYRotation == wrap)
        return;
    m_wrapYRotation = wrap;
    emit wrapCameraYRotationChanged(wrap);
}
