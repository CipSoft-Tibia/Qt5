// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstract3dinputhandler_p.h"
#include "qquickgraphsitem_p.h"

#include "abstract3dcontroller_p.h"
#include "declarativetheme_p.h"
#include "declarativescene_p.h"
#include "q3dscene_p.h"
#include "qcustom3dlabel.h"
#include "qcustom3ditem.h"
#include "qcustom3ditem_p.h"
#include "qtouch3dinputhandler.h"
#include "qvalue3daxis.h"
#include "qcategory3daxis.h"
#include "utils_p.h"
#include "qcustom3dvolume.h"

#include <QtGui/QGuiApplication>

#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>
#include <QtQuick3D/private/qquick3dloader_p.h>
#include <QtQuick/private/qquickitem_p.h>

#if defined(Q_OS_IOS)
#include <QtCore/QTimer>
#endif

#if defined(Q_OS_MACOS)
#include <qpa/qplatformnativeinterface.h>
#endif

QT_BEGIN_NAMESPACE

QQuickGraphsItem::QQuickGraphsItem(QQuickItem *parent) :
    QQuick3DViewport(parent),
    m_controller(nullptr)
{
    m_nodeMutex = QSharedPointer<QMutex>::create();

    QQuick3DSceneEnvironment *scene = environment();
    scene->setBackgroundMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes::Color);
    scene->setClearColor(Qt::transparent);

    auto sceneManager = QQuick3DObjectPrivate::get(rootNode())->sceneManager;
    connect(sceneManager.data(), &QQuick3DSceneManager::windowChanged, this, &QQuickGraphsItem::handleWindowChanged);
    // Set contents to false in case we are in qml designer to make component look nice
    m_runningInDesigner = QGuiApplication::applicationDisplayName() == QLatin1String("Qml2Puppet");
    setFlag(ItemHasContents/*, !m_runningInDesigner*/); // Is this relevant anymore?

    // Set 4x MSAA by default
    setRenderingMode(QAbstract3DGraph::RenderIndirect);
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

    m_segmentLineRepeaterX->model().clear();
    m_segmentLineRepeaterY->model().clear();
    m_segmentLineRepeaterZ->model().clear();
    m_segmentLineRepeaterX->deleteLater();
    m_segmentLineRepeaterY->deleteLater();
    m_segmentLineRepeaterZ->deleteLater();

    m_subsegmentLineRepeaterX->model().clear();
    m_subsegmentLineRepeaterY->model().clear();
    m_subsegmentLineRepeaterZ->model().clear();
    m_subsegmentLineRepeaterX->deleteLater();
    m_subsegmentLineRepeaterY->deleteLater();
    m_subsegmentLineRepeaterZ->deleteLater();

    if (m_sliceVerticalGridRepeater) {
        m_sliceVerticalGridRepeater->model().clear();
        m_sliceHorizontalGridRepeater->model().clear();
        m_sliceHorizontalLabelRepeater->model().clear();
        m_sliceVerticalLabelRepeater->model().clear();
        m_sliceVerticalGridRepeater->deleteLater();
        m_sliceHorizontalGridRepeater->deleteLater();
        m_sliceHorizontalLabelRepeater->deleteLater();
        m_sliceVerticalLabelRepeater->deleteLater();
    }

    // Make sure not deleting locked mutex
    QMutexLocker locker(&m_mutex);
    locker.unlock();

    m_nodeMutex.clear();
}

void QQuickGraphsItem::setRenderingMode(QAbstract3DGraph::RenderingMode mode)
{
    if (mode == m_renderMode)
        return;

    QAbstract3DGraph::RenderingMode previousMode = m_renderMode;

    m_renderMode = mode;

    m_initialisedSize = QSize(0, 0);
    setFlag(ItemHasContents/*, !m_runningInDesigner*/);

    // TODO - Need to check if the mode is set properly
    switch (mode) {
    case QAbstract3DGraph::RenderDirectToBackground:
        update();
        setRenderMode(QQuick3DViewport::Underlay);
        if (previousMode == QAbstract3DGraph::RenderIndirect) {
            checkWindowList(window());
            setAntialiasing(m_windowSamples > 0);
            if (m_windowSamples != m_samples)
                emit msaaSamplesChanged(m_windowSamples);
        }
        break;
    case QAbstract3DGraph::RenderIndirect:
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
    auto selectionMode = m_controller->selectionMode();
    if (selectionMode.testFlag(QAbstract3DGraph::SelectionSlice)
        && (selectionMode.testFlag(QAbstract3DGraph::SelectionColumn)
            != selectionMode.testFlag(QAbstract3DGraph::SelectionRow))) {
        m_sliceEnabled = true;
    } else {
        m_sliceEnabled = false;
    }
}

void QQuickGraphsItem::handleThemeTypeChange()
{
}

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

    if (m_sliceView) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(m_sliceView->camera());
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

    if (m_sliceView) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(m_sliceView->camera());
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

    auto delegateModelX = createRepeaterDelegateComponent(QStringLiteral(":/axis/AxisLabel"));
    auto delegateModelY = createRepeaterDelegateComponent(QStringLiteral(":/axis/AxisLabel"));
    auto delegateModelZ = createRepeaterDelegateComponent(QStringLiteral(":/axis/AxisLabel"));

    m_repeaterX->setDelegate(delegateModelX);
    m_repeaterY->setDelegate(delegateModelY);
    m_repeaterZ->setDelegate(delegateModelZ);

    // title labels for axes
    m_titleLabelX = createTitleLabel();
    m_titleLabelX->setVisible(m_controller->axisX()->isTitleVisible());
    m_titleLabelX->setProperty("labelText", m_controller->axisX()->title());

    m_titleLabelY = createTitleLabel();
    m_titleLabelY->setVisible(m_controller->axisY()->isTitleVisible());
    m_titleLabelY->setProperty("labelText", m_controller->axisY()->title());

    m_titleLabelZ = createTitleLabel();
    m_titleLabelZ->setVisible(m_controller->axisZ()->isTitleVisible());
    m_titleLabelZ->setProperty("labelText", m_controller->axisZ()->title());

    // Testing gridline

    // X lines
    m_segmentLineRepeaterX = createRepeater();

    auto segmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_segmentLineRepeaterX->setDelegate(segmentLineDelegate);

    m_subsegmentLineRepeaterX = createRepeater();

    auto subsegmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_subsegmentLineRepeaterX->setDelegate(subsegmentLineDelegate);

    // Y lines
    m_segmentLineRepeaterY = createRepeater();

    segmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_segmentLineRepeaterY->setDelegate(segmentLineDelegate);

    m_subsegmentLineRepeaterY = createRepeater();

    subsegmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_subsegmentLineRepeaterY->setDelegate(subsegmentLineDelegate);

    // Z lines
    m_segmentLineRepeaterZ = createRepeater();

    segmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_segmentLineRepeaterZ->setDelegate(segmentLineDelegate);

    m_subsegmentLineRepeaterZ = createRepeater();

    subsegmentLineDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    m_subsegmentLineRepeaterZ->setDelegate(subsegmentLineDelegate);

    createItemLabel();

    auto axis = m_controller->axisX();
    int segmentCount = 0;
    int subSegmentCount = 0;
    int gridLineCount = 0;
    int subGridLineCount = 0;
    if (axis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
        segmentCount = valueAxis->segmentCount();
        subSegmentCount = valueAxis->subSegmentCount();
        gridLineCount = 2 * (segmentCount + 1);
        subGridLineCount = 2 * (segmentCount * (subSegmentCount - 1));
    } else if (axis->type() & QAbstract3DAxis::AxisTypeCategory) {
        gridLineCount = axis->labels().size();
    }
    m_segmentLineRepeaterX->setModel(gridLineCount);
    m_subsegmentLineRepeaterX->setModel(subGridLineCount);
    m_repeaterX->setModel(axis->labels().size());
    m_controller->handleAxisLabelsChangedBySender(m_controller->axisX());

    axis = m_controller->axisY();
    if (axis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
        segmentCount = valueAxis->segmentCount();
        subSegmentCount = valueAxis->subSegmentCount();
        gridLineCount = 2 * (segmentCount + 1);
        subGridLineCount = 2 * (segmentCount * (subSegmentCount - 1));
    } else if (axis->type() & QAbstract3DAxis::AxisTypeCategory) {
        gridLineCount = axis->labels().size();
    }
    m_segmentLineRepeaterY->setModel(gridLineCount);
    m_subsegmentLineRepeaterY->setModel(subGridLineCount);
    m_repeaterY->setModel(2 * axis->labels().size());
    m_controller->handleAxisLabelsChangedBySender(m_controller->axisY());

    axis = m_controller->axisZ();
    if (axis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
        segmentCount = valueAxis->segmentCount();
        subSegmentCount = valueAxis->subSegmentCount();
        gridLineCount = 2 * (segmentCount + 1);
        subGridLineCount = 2 * (segmentCount * (subSegmentCount - 1));
    } else if (axis->type() & QAbstract3DAxis::AxisTypeCategory) {
        gridLineCount = axis->labels().size();
    }
    m_segmentLineRepeaterZ->setModel(gridLineCount);
    m_subsegmentLineRepeaterZ->setModel(subGridLineCount);
    m_repeaterZ->setModel(axis->labels().size());
    m_controller->handleAxisLabelsChangedBySender(m_controller->axisZ());

    if (!m_pendingCustomItemList.isEmpty()) {
        foreach (auto item, m_pendingCustomItemList)
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

// TODO: Check if it would make sense to remove these from Abstract3DController - QTBUG-113812
// begin..
void QQuickGraphsItem::addTheme(Q3DTheme *theme)
{
    m_controller->addTheme(theme);
}

void QQuickGraphsItem::releaseTheme(Q3DTheme *theme)
{
    m_controller->releaseTheme(theme);
}

QList<Q3DTheme *> QQuickGraphsItem::themes() const
{
    return m_controller->themes();
}

void QQuickGraphsItem::setTheme(Q3DTheme *theme)
{
    m_controller->setActiveTheme(theme, isComponentComplete());
}

Q3DTheme *QQuickGraphsItem::theme() const
{
    return m_controller->activeTheme();
}

void QQuickGraphsItem::clearSelection()
{
    m_controller->clearSelection();
}

bool QQuickGraphsItem::hasSeries(QAbstract3DSeries *series)
{
    return m_controller->hasSeries(series);
}

void QQuickGraphsItem::setSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    int intmode = int(mode);
    m_controller->setSelectionMode(QAbstract3DGraph::SelectionFlags(intmode));
}

QAbstract3DGraph::SelectionFlags QQuickGraphsItem::selectionMode() const
{
    return m_controller->selectionMode();
}

void QQuickGraphsItem::setShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    m_controller->setShadowQuality(quality);
}

QAbstract3DGraph::ShadowQuality QQuickGraphsItem::shadowQuality() const
{
    return m_controller->shadowQuality();
}

int QQuickGraphsItem::addCustomItem(QCustom3DItem *item)
{
    if (isComponentComplete()) {
        if (m_controller->isCustomLabelItem(item)) {
            QQuick3DNode *label = createTitleLabel();
            QCustom3DLabel *key = static_cast<QCustom3DLabel *>(item);
            m_customLabelList.insert(key, label);
        } else if (m_controller->isCustomVolumeItem(item)) {
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
    return m_controller->addCustomItem(item);
}

void QQuickGraphsItem::removeCustomItems()
{
    m_customItemList.clear();
    m_customLabelList.clear();
    m_controller->deleteCustomItems();
}

void QQuickGraphsItem::removeCustomItem(QCustom3DItem *item)
{
    if (m_controller->isCustomLabelItem(item)) {
        m_customLabelList.remove(static_cast<QCustom3DLabel *>(item));
    } else if (m_controller->isCustomVolumeItem(item)) {
        m_customItemList.remove(item);
        auto volume = static_cast<QCustom3DVolume *>(item);
        if (m_customVolumes.contains(volume)) {
            m_customVolumes[volume].model->deleteLater();
            m_customVolumes.remove(volume);
        }
    } else {
        m_customItemList.remove(item);
    }
    m_controller->deleteCustomItem(item);
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
            if (m_controller->isCustomVolumeItem(item)) {
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
    m_controller->deleteCustomItem(position);
}

void QQuickGraphsItem::releaseCustomItem(QCustom3DItem *item)
{
    if (m_controller->isCustomLabelItem(item)) {
        m_customLabelList.remove(static_cast<QCustom3DLabel *>(item));
    } else if (m_controller->isCustomVolumeItem(item)) {
        m_customItemList.remove(item);
        auto volume = static_cast<QCustom3DVolume *>(item);
        if (m_customVolumes.contains(volume)) {
            m_customVolumes[volume].model->deleteLater();
            m_customVolumes.remove(volume);
        }
    } else {
        m_customItemList.remove(item);
    }
    m_controller->releaseCustomItem(item);
}

int QQuickGraphsItem::selectedLabelIndex() const
{
    return m_controller->selectedLabelIndex();
}

QAbstract3DAxis *QQuickGraphsItem::selectedAxis() const
{
    return m_controller->selectedAxis();
}

int QQuickGraphsItem::selectedCustomItemIndex() const
{
    return m_controller->selectedCustomItemIndex();
}

QCustom3DItem *QQuickGraphsItem::selectedCustomItem() const
{
    return m_controller->selectedCustomItem();
}
// ...end
// TODO: Check if it would make sense to remove these from Abstract3DController - QTBUG-113812

QQmlListProperty<QCustom3DItem> QQuickGraphsItem::customItemList()
{
    return QQmlListProperty<QCustom3DItem>(this, this,
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
    return reinterpret_cast<QQuickGraphsItem *>(list->data)->m_controller->m_customItems.size();
}

QCustom3DItem *QQuickGraphsItem::atCustomItemFunc(QQmlListProperty<QCustom3DItem> *list,
                                                  qsizetype index)
{
    Q_UNUSED(list);
    Q_UNUSED(index);
    return reinterpret_cast<QQuickGraphsItem *>(list->data)->m_controller->m_customItems.at(index);
}

void QQuickGraphsItem::clearCustomItemFunc(QQmlListProperty<QCustom3DItem> *list)
{
    QQuickGraphsItem *decl = reinterpret_cast<QQuickGraphsItem *>(list->data);
    decl->removeCustomItems();
}

void QQuickGraphsItem::setSharedController(Abstract3DController *controller)
{
    Q_ASSERT(controller);
    m_controller = controller;
    m_controller->m_qml = this;

    // Reset default theme, as the default C++ theme is Q3DTheme, not DeclarativeTheme3D.
    DeclarativeTheme3D *defaultTheme = new DeclarativeTheme3D;
    defaultTheme->d_func()->setDefaultTheme(true);
    defaultTheme->setType(Q3DTheme::ThemeQt);
    m_controller->setActiveTheme(defaultTheme);

    QObject::connect(m_controller.data(), &Abstract3DController::shadowQualityChanged, this,
                     &QQuickGraphsItem::handleShadowQualityChange);
    QObject::connect(m_controller.data(), &Abstract3DController::activeThemeChanged, this,
                     &QQuickGraphsItem::themeChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::themeTypeChanged, this,
                     &QQuickGraphsItem::handleThemeTypeChange);
    QObject::connect(m_controller.data(), &Abstract3DController::selectionModeChanged, this,
                     &QQuickGraphsItem::handleSelectionModeChange);
    QObject::connect(m_controller.data(), &Abstract3DController::elementSelected, this,
                     &QQuickGraphsItem::handleSelectedElementChange);

    QObject::connect(m_controller.data(), &Abstract3DController::axisXChanged, this,
                     &QQuickGraphsItem::handleAxisXChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::axisYChanged, this,
                     &QQuickGraphsItem::handleAxisYChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::axisZChanged, this,
                     &QQuickGraphsItem::handleAxisZChanged);

    QObject::connect(m_controller.data(), &Abstract3DController::orthoProjectionChanged, this,
                     &QQuickGraphsItem::orthoProjectionChanged);

    QObject::connect(m_controller.data(), &Abstract3DController::aspectRatioChanged, this,
                     &QQuickGraphsItem::aspectRatioChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::optimizationHintsChanged, this,
                     &QQuickGraphsItem::handleOptimizationHintChange);
    QObject::connect(m_controller.data(), &Abstract3DController::polarChanged, this,
                     &QQuickGraphsItem::polarChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::radialLabelOffsetChanged, this,
                     &QQuickGraphsItem::radialLabelOffsetChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::horizontalAspectRatioChanged, this,
                     &QQuickGraphsItem::horizontalAspectRatioChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::reflectionChanged, this,
                     &QQuickGraphsItem::reflectionChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::reflectivityChanged, this,
                     &QQuickGraphsItem::reflectivityChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::localeChanged, this,
                     &QQuickGraphsItem::localeChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::queriedGraphPositionChanged, this,
                     &QQuickGraphsItem::queriedGraphPositionChanged);
    QObject::connect(m_controller.data(), &Abstract3DController::marginChanged, this,
                     &QQuickGraphsItem::marginChanged);
}

void QQuickGraphsItem::synchData()
{
    if (!isVisible())
        return;
    m_controller->m_renderPending = false;

    if (m_controller->m_changeTracker.selectionModeChanged) {
        updateSelectionMode(m_controller->selectionMode());
        m_controller->m_changeTracker.selectionModeChanged = false;
    }

    bool recalculateScale = false;
    if (m_controller->m_changeTracker.aspectRatioChanged) {
        recalculateScale = true;
        m_controller->m_changeTracker.aspectRatioChanged = false;
    }

    if (m_controller->m_changeTracker.horizontalAspectRatioChanged) {
        recalculateScale = true;
        m_controller->m_changeTracker.horizontalAspectRatioChanged = false;
    }

    if (m_controller->m_changeTracker.marginChanged) {
        recalculateScale = true;
        m_controller->m_changeTracker.marginChanged = false;
    }

    if (recalculateScale)
        calculateSceneScalingFactors();

    bool axisDirty = recalculateScale;
    if (m_controller->m_changeTracker.axisXFormatterChanged) {
        m_controller->m_changeTracker.axisXFormatterChanged = false;
        QAbstract3DAxis *axisX = m_controller->axisX();
        if (axisX->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX);
            valueAxisX->recalculate();
            repeaterX()->setModel(valueAxisX->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisYFormatterChanged) {
        m_controller->m_changeTracker.axisYFormatterChanged = false;
        QAbstract3DAxis *axisY = m_controller->axisY();
        if (axisY->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY);
            valueAxisY->recalculate();
            repeaterY()->setModel(2 * valueAxisY->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisZFormatterChanged) {
        m_controller->m_changeTracker.axisZFormatterChanged = false;
        QAbstract3DAxis *axisZ = m_controller->axisZ();
        if (axisZ->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ);
            valueAxisZ->recalculate();
            repeaterZ()->setModel(valueAxisZ->formatter()->labelPositions().size());
        }
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisXSegmentCountChanged) {
        QAbstract3DAxis *axisX = m_controller->axisX();
        if (axisX->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX);
            valueAxisX->recalculate();
        }
        handleSegmentLineCountChanged(axisX, m_segmentLineRepeaterX);
        m_controller->m_changeTracker.axisXSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisYSegmentCountChanged) {
        QAbstract3DAxis *axisY = m_controller->axisY();
        if (axisY->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY);
            valueAxisY->recalculate();
        }
        handleSegmentLineCountChanged(axisY, m_segmentLineRepeaterY);
        m_controller->m_changeTracker.axisYSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisZSegmentCountChanged) {
        QAbstract3DAxis *axisZ = m_controller->axisZ();
        if (axisZ->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ);
            valueAxisZ->recalculate();
        }
        handleSegmentLineCountChanged(axisZ, m_segmentLineRepeaterZ);
        m_controller->m_changeTracker.axisZSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisXSubSegmentCountChanged) {
        QAbstract3DAxis *axisX = m_controller->axisX();
        if (axisX->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(axisX);
            valueAxisX->recalculate();
        }
        handleSubSegmentLineCountChanged(axisX, m_subsegmentLineRepeaterX);
        m_controller->m_changeTracker.axisXSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisYSubSegmentCountChanged) {
        QAbstract3DAxis *axisY = m_controller->axisY();
        if (axisY->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(axisY);
            valueAxisY->recalculate();
        }
        handleSubSegmentLineCountChanged(axisY, m_subsegmentLineRepeaterY);
        m_controller->m_changeTracker.axisYSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisZSubSegmentCountChanged) {
        QAbstract3DAxis *axisZ = m_controller->axisZ();
        if (axisZ->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(axisZ);
            valueAxisZ->recalculate();
        }
        handleSubSegmentLineCountChanged(axisZ, m_subsegmentLineRepeaterZ);
        m_controller->m_changeTracker.axisZSubSegmentCountChanged = false;
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisXLabelsChanged) {
        QAbstract3DAxis *axisX = m_controller->axisX();
        if (axisX->type() & QAbstract3DAxis::AxisTypeValue) {
            auto valueAxisX = static_cast<QValue3DAxis *>(axisX);
            valueAxisX->recalculate();
            repeaterX()->setModel(valueAxisX->formatter()->labelPositions().size());
        } else if (axisX->type() & QAbstract3DAxis::AxisTypeCategory) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisX);
            repeaterX()->setModel(categoryAxis->labels().size());
        }

        handleSegmentLineCountChanged(axisX, segmentLineRepeaterX());
        handleSubSegmentLineCountChanged(axisX, subsegmentLineRepeaterX());
        m_controller->m_changeTracker.axisXLabelsChanged = false;
        handleLabelCountChanged(m_repeaterX);
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisYLabelsChanged) {
        QAbstract3DAxis *axisY = m_controller->axisY();
        if (axisY->type() & QAbstract3DAxis::AxisTypeValue) {
            auto valueAxisY = static_cast<QValue3DAxis *>(axisY);
            valueAxisY->recalculate();
            repeaterY()->setModel(2 * valueAxisY->formatter()->labelPositions().size());
        } else if (axisY->type() & QAbstract3DAxis::AxisTypeCategory) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisY);
            repeaterY()->setModel(2 * categoryAxis->labels().size());
        }

        handleSegmentLineCountChanged(axisY, segmentLineRepeaterY());
        handleSubSegmentLineCountChanged(axisY, subsegmentLineRepeaterY());
        m_controller->m_changeTracker.axisYLabelsChanged = false;
        handleLabelCountChanged(m_repeaterY);
        axisDirty = true;
    }

    if (m_controller->m_changeTracker.axisZLabelsChanged) {
        QAbstract3DAxis *axisZ = m_controller->axisZ();
        if (axisZ->type() & QAbstract3DAxis::AxisTypeValue) {
            auto valueAxisZ = static_cast<QValue3DAxis *>(axisZ);
            valueAxisZ->recalculate();
            repeaterZ()->setModel(valueAxisZ->formatter()->labelPositions().size());
        } else if (axisZ->type() & QAbstract3DAxis::AxisTypeCategory) {
            auto categoryAxis = static_cast<QCategory3DAxis *>(axisZ);
            repeaterZ()->setModel(categoryAxis->labels().size());
        }

        handleSegmentLineCountChanged(axisZ, segmentLineRepeaterZ());
        handleSubSegmentLineCountChanged(axisZ, subsegmentLineRepeaterZ());
        m_controller->m_changeTracker.axisZLabelsChanged = false;
        handleLabelCountChanged(m_repeaterZ);
        axisDirty = true;
    }

    updateTitleLabels();

    if (m_controller->m_changeTracker.shadowQualityChanged) {
        updateShadowQuality(shadowQuality());
        m_controller->m_changeTracker.shadowQualityChanged = false;
    }

    if (m_controller->m_changeTracker.axisXRangeChanged) {
        axisDirty = true;
        calculateSceneScalingFactors();
        m_controller->m_changeTracker.axisXRangeChanged = false;
    }

    if (m_controller->m_changeTracker.axisYRangeChanged) {
        axisDirty = true;
        QAbstract3DAxis *axis = m_controller->axisY();
        updateAxisRange(axis->min(), axis->max());
        calculateSceneScalingFactors();
        m_controller->m_changeTracker.axisYRangeChanged = false;
    }

    if (m_controller->m_changeTracker.axisZRangeChanged) {
        axisDirty = true;
        calculateSceneScalingFactors();
        m_controller->m_changeTracker.axisZRangeChanged = false;
    }

    if (m_controller->m_changeTracker.axisYReversedChanged) {
        m_controller->m_changeTracker.axisYReversedChanged = false;
        if (m_controller->m_axisY->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(m_controller->m_axisY);
            updateAxisReversed(valueAxisY->reversed());
        }
    }

    if (m_controller->m_changeTracker.polarChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.polarChanged = false;
    }

    if (m_controller->m_changeTracker.axisXLabelAutoRotationChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisXLabelAutoRotationChanged = false;
    }

    if (m_controller->m_changeTracker.axisYLabelAutoRotationChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisYLabelAutoRotationChanged = false;
    }

    if (m_controller->m_changeTracker.axisZLabelAutoRotationChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisZLabelAutoRotationChanged = false;
    }

    if (m_controller->m_changeTracker.axisXTitleFixedChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisXTitleFixedChanged = false;
    }

    if (m_controller->m_changeTracker.axisYTitleFixedChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisYTitleFixedChanged = false;
    }

    if (m_controller->m_changeTracker.axisZTitleFixedChanged) {
        axisDirty = true;
        m_controller->m_changeTracker.axisZTitleFixedChanged = false;
    }

    updateCamera();

    QVector3D forward = camera()->forward();
    auto targetRotation = cameraTarget()->rotation();
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
        if (m_sliceView) {
            updateSliceGrid();
            updateSliceLabels();
        }
        m_gridUpdated = true;
    }

    if (m_controller->m_changeTracker.radialLabelOffsetChanged) {
        updateRadialLabelOffset();
        m_controller->m_changeTracker.radialLabelOffsetChanged = false;
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

    if (m_controller->graphPositionQueryPending())
        graphPositionAt(m_controller->scene()->graphPositionQuery());

    bool forceUpdateCustomVolumes = false;
    if (m_controller->m_changeTracker.projectionChanged) {
        forceUpdateCustomVolumes = true;
        bool useOrtho = m_controller->isOrthoProjection();
        if (useOrtho)
            setCamera(m_oCamera);
        else
            setCamera(m_pCamera);
        m_controller->m_changeTracker.projectionChanged = false;
    }

    Q3DTheme *theme = m_controller->activeTheme();
    if (m_controller->m_changeTracker.themeChanged) {
        environment()->setClearColor(theme->windowColor());
        m_controller->m_changeTracker.themeChanged = false;
    }

    if (theme->d_func()->m_dirtyBits.lightStrengthDirty) {
        light()->setBrightness(theme->lightStrength() * .2f);
        if (qFuzzyIsNull(light()->brightness()))
            light()->setBrightness(.0000001f);
        updateLightStrength();
        theme->d_func()->m_dirtyBits.lightStrengthDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.ambientLightStrengthDirty) {
        float ambientStrength = theme->ambientLightStrength();
        QColor ambientColor = QColor::fromRgbF(ambientStrength, ambientStrength, ambientStrength);
        light()->setAmbientColor(ambientColor);
        if (qFuzzyIsNull(light()->brightness()))
            light()->setBrightness(.0000001f);
        theme->d_func()->m_dirtyBits.ambientLightStrengthDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.lightColorDirty) {
        light()->setColor(theme->lightColor());
        theme->d_func()->m_dirtyBits.lightColorDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.shadowStrengthDirty) {
        light()->setShadowFactor(theme->shadowStrength());
        theme->d_func()->m_dirtyBits.shadowStrengthDirty = false;
    }

    // label Adjustments
    if (theme->d_func()->m_dirtyBits.labelBackgroundColorDirty) {
        QColor labelBackgroundColor = theme->labelBackgroundColor();
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
        theme->d_func()->m_dirtyBits.labelBackgroundColorDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.labelBackgroundEnabledDirty) {
        bool enabled = theme->isLabelBackgroundEnabled();
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
        theme->d_func()->m_dirtyBits.labelBackgroundEnabledDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.labelBorderEnabledDirty) {
        bool enabled = theme->isLabelBorderEnabled();
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
        theme->d_func()->m_dirtyBits.labelBorderEnabledDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.labelTextColorDirty) {
        QColor labelTextColor = theme->labelTextColor();
        changeLabelTextColor(m_repeaterX, labelTextColor);
        changeLabelTextColor(m_repeaterY, labelTextColor);
        changeLabelTextColor(m_repeaterZ, labelTextColor);
        m_titleLabelX->setProperty("labelTextColor", labelTextColor);
        m_titleLabelY->setProperty("labelTextColor", labelTextColor);
        m_titleLabelZ->setProperty("labelTextColor", labelTextColor);
        m_itemLabel->setProperty("labelTextColor", labelTextColor);

        if (m_sliceView) {
            changeLabelTextColor(m_sliceHorizontalLabelRepeater, labelTextColor);
            changeLabelTextColor(m_sliceVerticalLabelRepeater, labelTextColor);
            m_sliceItemLabel->setProperty("labelTextColor", labelTextColor);
            m_sliceHorizontalTitleLabel->setProperty("labelTextColor", labelTextColor);
            m_sliceVerticalTitleLabel->setProperty("labelTextColor", labelTextColor);
        }
        theme->d_func()->m_dirtyBits.labelTextColorDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.fontDirty) {
        auto font = theme->font();
        changeLabelFont(m_repeaterX, font);
        changeLabelFont(m_repeaterY, font);
        changeLabelFont(m_repeaterZ, font);
        m_titleLabelX->setProperty("labelFont", font);
        m_titleLabelY->setProperty("labelFont", font);
        m_titleLabelZ->setProperty("labelFont", font);
        m_itemLabel->setProperty("labelFont", font);
        updateLabels();

        if (m_sliceView) {
            changeLabelFont(m_sliceHorizontalLabelRepeater, font);
            changeLabelFont(m_sliceVerticalLabelRepeater, font);
            m_sliceItemLabel->setProperty("labelFont", font);
            m_sliceHorizontalTitleLabel->setProperty("labelFont", font);
            m_sliceVerticalTitleLabel->setProperty("labelFont", font);
            updateSliceLabels();
        }
        theme->d_func()->m_dirtyBits.fontDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.labelsEnabledDirty) {
        bool enabled = theme->isLabelsEnabled();
        changeLabelsEnabled(m_repeaterX, enabled);
        changeLabelsEnabled(m_repeaterY, enabled);
        changeLabelsEnabled(m_repeaterZ, enabled);
        m_titleLabelX->setProperty("visible", enabled && m_controller->axisX()->isTitleVisible());
        m_titleLabelY->setProperty("visible", enabled && m_controller->axisY()->isTitleVisible());
        m_titleLabelZ->setProperty("visible", enabled && m_controller->axisZ()->isTitleVisible());
        m_itemLabel->setProperty("visible", enabled);

        if (m_sliceView) {
            changeLabelsEnabled(m_sliceHorizontalLabelRepeater, enabled);
            changeLabelsEnabled(m_sliceVerticalLabelRepeater, enabled);
            m_sliceItemLabel->setProperty("visible", enabled);
            m_sliceHorizontalTitleLabel->setProperty("visible", enabled);
            m_sliceVerticalTitleLabel->setProperty("visible", enabled);
        }
        theme->d_func()->m_dirtyBits.labelsEnabledDirty = false;
    }

    // Grid and background adjustments
    if (theme->d_func()->m_dirtyBits.backgroundColorDirty) {
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
        bgMat->setBaseColor(theme->backgroundColor());
        theme->d_func()->m_dirtyBits.backgroundColorDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.backgroundEnabledDirty) {
        m_background->setLocalOpacity(theme->isBackgroundEnabled());
        theme->d_func()->m_dirtyBits.backgroundEnabledDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.gridEnabledDirty) {
        bool enabled = theme->isGridEnabled();
        m_segmentLineRepeaterX->setVisible(enabled);
        m_segmentLineRepeaterY->setVisible(enabled);
        m_segmentLineRepeaterZ->setVisible(enabled);

        m_subsegmentLineRepeaterX->setVisible(enabled);
        m_subsegmentLineRepeaterY->setVisible(enabled);
        m_subsegmentLineRepeaterZ->setVisible(enabled);

        if (m_sliceEnabled && m_controller->isSlicingActive()) {
            m_sliceHorizontalGridRepeater->setVisible(enabled);
            m_sliceVerticalGridRepeater->setVisible(enabled);
        }
        theme->d_func()->m_dirtyBits.gridEnabledDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.gridLineColorDirty) {
        QColor gridLineColor = theme->gridLineColor();
        changeGridLineColor(m_segmentLineRepeaterX, gridLineColor);
        changeGridLineColor(m_subsegmentLineRepeaterX, gridLineColor);
        changeGridLineColor(m_segmentLineRepeaterY, gridLineColor);
        changeGridLineColor(m_subsegmentLineRepeaterY, gridLineColor);
        changeGridLineColor(m_segmentLineRepeaterZ, gridLineColor);
        changeGridLineColor(m_subsegmentLineRepeaterZ, gridLineColor);
        theme->d_func()->m_dirtyBits.gridLineColorDirty = false;
    }

    if (theme->d_func()->m_dirtyBits.singleHighlightColorDirty) {
        updateSingleHighlightColor();
        theme->d_func()->m_dirtyBits.singleHighlightColorDirty = false;
    }

    // Other adjustments
    if (theme->d_func()->m_dirtyBits.windowColorDirty) {
        window()->setColor(theme->windowColor());
        environment()->setClearColor(theme->windowColor());
        theme->d_func()->m_dirtyBits.windowColorDirty = false;
    }
    if (m_controller->activeTheme()->windowColor() != window()->color()) {
        window()->setColor(m_controller->activeTheme()->windowColor());
    }

    if (m_controller->isCustomDataDirty()) {
        forceUpdateCustomVolumes = true;
        updateCustomData();
        m_controller->setCustomDataDirty(false);
    }

    if (m_controller->m_changedSeriesList.size()) {
        forceUpdateCustomVolumes = true;
        updateGraph();
        m_controller->m_changedSeriesList.clear();
    }

    if (m_controller->m_isDataDirty) {
        forceUpdateCustomVolumes = true;
        updateGraph();
        m_controller->m_isDataDirty = false;
    }

    if (m_controller->m_isSeriesVisualsDirty) {
        forceUpdateCustomVolumes = true;
        updateLabels();
        updateGraph();
        m_controller->m_isSeriesVisualsDirty = false;
    }

    if (m_sliceActivatedChanged)
        updateSliceGraph();

    if (m_controller->isCustomItemDirty() || forceUpdateCustomVolumes)
        updateCustomVolumes();
}

void QQuickGraphsItem::calculateSceneScalingFactors()
{
    float scaleX, scaleY, scaleZ;
    float marginH, marginV;
    float aspectRatio = m_controller->aspectRatio();
    float horizontalAspectRatio = m_controller->horizontalAspectRatio();

    float margin = m_controller->margin();
    if (margin < 0.0f) {
        marginH = .1f;
        marginV = .1f;
    } else {
        marginH = margin;
        marginV = margin;
    }

    QSizeF areaSize;
    if (qFuzzyIsNull(horizontalAspectRatio)) {
        areaSize.setHeight(m_controller->axisZ()->max() - m_controller->axisZ()->min());
        areaSize.setWidth(m_controller->axisX()->max() - m_controller->axisX()->min());
    } else {
        areaSize.setHeight(1.0);
        areaSize.setWidth(horizontalAspectRatio);
    }

    float horizontalMaxDimension;
    if (aspectRatio > 2.0f) {
        horizontalMaxDimension = 2.0f;
        scaleY = 2.0f / aspectRatio;
    } else {
        horizontalMaxDimension = aspectRatio;
        scaleY = 1.0f;
    }

    float scaleFactor = qMax(areaSize.width(), areaSize.height());
    scaleX = horizontalMaxDimension * areaSize.width() / scaleFactor;
    scaleZ = horizontalMaxDimension * areaSize.height() / scaleFactor;

    m_scale = QVector3D(scaleX, scaleY, scaleZ);
    m_scaleWithBackground = QVector3D(scaleX, scaleY, scaleZ);
    m_backgroundScaleMargin = QVector3D(marginH, marginV, marginH);
}

void QQuickGraphsItem::updateGrid()
{
    int gridLineCountX = segmentLineRepeaterX()->count();
    int subGridLineCountX = subsegmentLineRepeaterX()->count();
    int gridLineCountY = segmentLineRepeaterY()->count() / 2;
    int subGridLineCountY = subsegmentLineRepeaterY()->count() / 2;
    int gridLineCountZ = segmentLineRepeaterZ()->count();
    int subGridLineCountZ = subsegmentLineRepeaterZ()->count();

    if (!m_isFloorGridInRange) {
        gridLineCountX /= 2;
        subGridLineCountX /= 2;
        gridLineCountZ /= 2;
        subGridLineCountZ /= 2;
    }

    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    QVector3D scaleX(backgroundScale.x() * lineLengthScaleFactor(), lineWidthScaleFactor(), lineWidthScaleFactor());
    QVector3D scaleY(lineWidthScaleFactor(), backgroundScale.y() * lineLengthScaleFactor(), lineWidthScaleFactor());
    QVector3D scaleZ(backgroundScale.z() * lineLengthScaleFactor(), lineWidthScaleFactor(), lineWidthScaleFactor());

    auto axisX = m_controller->axisX();
    auto axisY = m_controller->axisY();
    auto axisZ = m_controller->axisZ();

    const bool xFlipped = isXFlipped();
    const bool yFlipped = isYFlipped();
    const bool zFlipped = isZFlipped();

    QQuaternion lineRotation(.0f, .0f, .0f, .0f);
    QVector3D rotation(90.0f, 0.0f, 0.0f);

    // Floor horizontal line
    float linePosX = 0.0f;
    float linePosY = backgroundScale.y();
    float linePosZ = 0.0f;
    float scale = m_scaleWithBackground.z();

    if (!yFlipped) {
        linePosY *= -1.0f;
        rotation.setZ(180.0f);
    }
    lineRotation = Utils::calculateRotation(rotation);
    for (int i = 0; i < subGridLineCountZ; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterZ()->objectAt(i));
        if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
            linePosZ = static_cast<QValue3DAxis *>(axisZ)->subGridPositionAt(i) * -scale * 2.0f + scale;
        } else if (axisZ->type() == QAbstract3DAxis::AxisTypeCategory) {
            linePosZ = calculateCategoryGridLinePosition(axisZ, i);
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        }
        if (isPolar()) {
            lineNode->setPosition(QVector3D(0.0f, linePosY, 0.0f));
            lineNode->setScale(QVector3D(scaleX.x() * .5f, scaleX.x() * .5f, scaleX.z()));
            lineNode->setProperty("polarRadius",
                                  static_cast<QValue3DAxis *>(axisZ)->subGridPositionAt(i)
                                  / (scaleX.x() * .5f) * 2.0f);
        } else {
            positionAndScaleLine(lineNode, scaleX, QVector3D(linePosX, linePosY, linePosZ));
        }
        lineNode->setRotation(lineRotation);
        lineNode->setProperty("isPolar", isPolar());
    }

    for (int i  = 0; i < gridLineCountZ; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterZ()->objectAt(i));
        if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
            linePosZ = static_cast<QValue3DAxis *>(axisZ)->gridPositionAt(i) * -scale * 2.0f + scale;
        } else if (axisZ->type() == QAbstract3DAxis::AxisTypeCategory) {
            linePosZ = calculateCategoryGridLinePosition(axisZ, i);
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        }
        if (isPolar()) {
            lineNode->setPosition(QVector3D(0.0f, linePosY, 0.0f));
            lineNode->setScale(QVector3D(scaleX.x() * .5f, scaleX.x() * .5f, scaleX.z()));
            lineNode->setProperty("polarRadius",
                                  static_cast<QValue3DAxis *>(axisZ)->gridPositionAt(i)
                                  / (scaleX.x() * .5f) * 2.0f);
        } else {
            positionAndScaleLine(lineNode, scaleX, QVector3D(linePosX, linePosY, linePosZ));
        }
        lineNode->setRotation(lineRotation);
        lineNode->setProperty("isPolar", isPolar());
    }

    // Side vertical line
    linePosX = -backgroundScale.x();
    linePosY = 0.0f;
    rotation = QVector3D(0.0f, 90.0f, 0.0f);
    if (xFlipped) {
        linePosX *= -1.0f;
        rotation.setY(-90.0f);
    }
    lineRotation = Utils::calculateRotation(rotation);
    if (m_hasVerticalSegmentLine) {
        for (int i  = 0; i < subGridLineCountZ; i++) {
            QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterZ()->objectAt(i + subGridLineCountZ));
            if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ)->subGridPositionAt(i) * scale * 2.0f - scale;
            } else if (axisZ->type() == QAbstract3DAxis::AxisTypeCategory) {
                linePosX = calculateCategoryGridLinePosition(axisZ, i);
                linePosY = calculateCategoryGridLinePosition(axisY, i);
            }
            positionAndScaleLine(lineNode, scaleY, QVector3D(linePosX, linePosY, linePosZ));
            lineNode->setRotation(lineRotation);
        }
        for (int i  = 0; i < gridLineCountZ; i++) {
            QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterZ()->objectAt(i + gridLineCountZ));
            if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
                linePosZ = static_cast<QValue3DAxis *>(axisZ)->gridPositionAt(i) * scale * 2.0f - scale;
            } else if (axisZ->type() == QAbstract3DAxis::AxisTypeCategory) {
                linePosX = calculateCategoryGridLinePosition(axisZ, i);
                linePosY = calculateCategoryGridLinePosition(axisY, i);
            }
            positionAndScaleLine(lineNode, scaleY, QVector3D(linePosX, linePosY, linePosZ));
            lineNode->setRotation(lineRotation);
        }
    }

    // Side horizontal line
    linePosZ = 0.0f;
    scale = m_scaleWithBackground.y();
    rotation = QVector3D(180.0f, -90.0f, 0.0f);
    if (xFlipped)
        rotation.setY(90.0f);
    lineRotation = Utils::calculateRotation(rotation);
    for (int i  = 0; i < gridLineCountY; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterY()->objectAt(i));
        if (axisY->type() == QAbstract3DAxis::AxisTypeValue)
            linePosY = static_cast<QValue3DAxis *>(axisY)->gridPositionAt(i) * scale * 2.0f - scale;
        else if (axisY->type() == QAbstract3DAxis::AxisTypeCategory)
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        positionAndScaleLine(lineNode, scaleZ, QVector3D(linePosX, linePosY, linePosZ));
        lineNode->setRotation(lineRotation);
    }

    for (int i = 0; i < subGridLineCountY; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterY()->objectAt(i));
        if (axisY->type() == QAbstract3DAxis::AxisTypeValue)
            linePosY = static_cast<QValue3DAxis *>(axisY)->subGridPositionAt(i) * scale * 2.0f - scale;
        else if (axisY->type() == QAbstract3DAxis::AxisTypeCategory)
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        positionAndScaleLine(lineNode, scaleZ, QVector3D(linePosX, linePosY, linePosZ));
        lineNode->setRotation(lineRotation);
    }

    // Floor vertical line
    linePosZ = 0.0f;
    linePosY = -backgroundScale.y();
    rotation = QVector3D(-90.0f, 90.0f, 0.0f);
    if (yFlipped) {
        linePosY *= -1.0f;
        rotation.setZ(180.0f);
    }
    scale = m_scaleWithBackground.x();
    for (int i = 0; i < subGridLineCountX; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterX()->objectAt(i));
        if (axisX->type() == QAbstract3DAxis::AxisTypeValue) {
            linePosX = static_cast<QValue3DAxis *>(axisX)->subGridPositionAt(i) * scale * 2.0f - scale;
        } else if (axisX->type() == QAbstract3DAxis::AxisTypeCategory) {
            linePosX = calculateCategoryGridLinePosition(axisX, i);
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        }
        if (isPolar()) {
            lineNode->setPosition(QVector3D(.0f, linePosY, .0f));
            rotation.setY(static_cast<QValue3DAxis *>(axisX)->gridPositionAt(i) * 360.0f);
        } else {
            positionAndScaleLine(lineNode, scaleZ, QVector3D(linePosX, linePosY, linePosZ));
        }
        lineRotation = Utils::calculateRotation(rotation);
        lineNode->setRotation(lineRotation);
    }
    for (int i  = 0; i < gridLineCountX; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterX()->objectAt(i));
        if (axisX->type() == QAbstract3DAxis::AxisTypeValue) {
            linePosX = static_cast<QValue3DAxis *>(axisX)->gridPositionAt(i) * scale * 2.0f - scale;
        } else if (axisX->type() == QAbstract3DAxis::AxisTypeCategory) {
            linePosX = calculateCategoryGridLinePosition(axisX, i);
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        }
        if (isPolar()) {
            lineNode->setPosition(QVector3D(.0f, linePosY, .0f));
            rotation.setY(static_cast<QValue3DAxis *>(axisX)->gridPositionAt(i) * 360.0f);
        } else {
            positionAndScaleLine(lineNode, scaleZ, QVector3D(linePosX, linePosY, linePosZ));
        }
        lineRotation = Utils::calculateRotation(rotation);
        lineNode->setRotation(lineRotation);
    }

    // Back horizontal line
    linePosX = 0.0f;
    linePosZ = -backgroundScale.z();
    rotation = QVector3D(0.0f, 0.0f, 0.0f);
    if (zFlipped) {
        linePosZ *= -1.0f;
        rotation.setX(180.0f);
    }
    lineRotation = Utils::calculateRotation(rotation);
    scale = m_scaleWithBackground.y();
    for (int i = 0; i < subGridLineCountY; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterY()->objectAt(i + subGridLineCountY));
        if (axisY->type() == QAbstract3DAxis::AxisTypeValue)
            linePosY = static_cast<QValue3DAxis *>(axisY)->subGridPositionAt(i) * scale * 2.0f - scale;
        else if (axisY->type() == QAbstract3DAxis::AxisTypeCategory)
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        positionAndScaleLine(lineNode, scaleX, QVector3D(linePosX, linePosY, linePosZ));
        lineNode->setRotation(lineRotation);
    }

    for (int i  = 0; i < gridLineCountY; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterY()->objectAt(i + gridLineCountY));
        if (axisY->type() == QAbstract3DAxis::AxisTypeValue)
            linePosY = static_cast<QValue3DAxis *>(axisY)->gridPositionAt(i) * scale * 2.0f - scale;
        else if (axisY->type() == QAbstract3DAxis::AxisTypeCategory)
            linePosY = calculateCategoryGridLinePosition(axisY, i);
        positionAndScaleLine(lineNode, scaleX, QVector3D(linePosX, linePosY, linePosZ));
        lineNode->setRotation(lineRotation);
    }

    // Back vertical line
    linePosY = 0.0f;
    scale = m_scaleWithBackground.x();
    rotation = QVector3D(0.0f, 0.0f, 0.0f);
    if (zFlipped)
        rotation.setY(180.0f);
    lineRotation = Utils::calculateRotation(rotation);
    if (m_hasVerticalSegmentLine) {
        for (int i  = 0; i < gridLineCountX; i++) {
            QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterX()->objectAt(i + gridLineCountX));
            if (axisX->type() == QAbstract3DAxis::AxisTypeValue) {
                linePosX = static_cast<QValue3DAxis *>(axisX)->gridPositionAt(i) * scale * 2.0f - scale;
            } else if (axisX->type() == QAbstract3DAxis::AxisTypeCategory) {
                linePosX = calculateCategoryGridLinePosition(axisX, i);
                linePosY = calculateCategoryGridLinePosition(axisY, i);
            }
            positionAndScaleLine(lineNode, scaleY, QVector3D(linePosX, linePosY, linePosZ));
            lineNode->setRotation(lineRotation);
        }

        for (int i  = 0; i < subGridLineCountX; i++) {
            QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterX()->objectAt(i + subGridLineCountX));
            if (axisX->type() == QAbstract3DAxis::AxisTypeValue) {
                linePosX = static_cast<QValue3DAxis *>(axisX)->subGridPositionAt(i) * scale * 2.0f - scale;
            } else if (axisX->type() == QAbstract3DAxis::AxisTypeCategory) {
                linePosX = calculateCategoryGridLinePosition(axisX, i);
                linePosY = calculateCategoryGridLinePosition(axisY, i);
            }
            positionAndScaleLine(lineNode, scaleY, QVector3D(linePosX, linePosY, linePosZ));
            lineNode->setRotation(lineRotation);
        }
    }
}

float QQuickGraphsItem::fontScaleFactor(float pointSize)
{
    return m_fontScaleFactorA * pointSize
            + m_fontScaleFactorB;
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

void QQuickGraphsItem::updateLabels()
{
    auto axisX = m_controller->axisX();

    auto labels = axisX->labels();
    int labelCount = labels.size();
    float labelAutoAngle = axisX->labelAutoRotation();
    float labelAngleFraction = labelAutoAngle / 90.0f;
    float fractionCamX = m_controller->scene()->activeCamera()->xRotation() * labelAngleFraction;
    float fractionCamY = m_controller->scene()->activeCamera()->yRotation() * labelAngleFraction;

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
                    labelRotation.setX(90.0f - (2.0f * labelAutoAngle - fractionCamX)
                                       * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(90.0f - (2.0f * labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(90.0f + fractionCamX
                                       * -(labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(90.0f - fractionCamX
                                       * (-labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                }
            }
        } else {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(-90.0f + (2.0f * labelAutoAngle - fractionCamX)
                                       * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(-90.0f + (2.0f * labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(-90.0f - fractionCamX
                                       * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(-90.0f + fractionCamX
                                       * -(labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                }
            }
        }
    }
    if (isPolar())
        labelRotation.setY(0.0f);
    auto totalRotation = Utils::calculateRotation(labelRotation);

    float scale = backgroundScale.x() - m_backgroundScaleMargin.x();

    float pointSize = m_controller->activeTheme()->font().pointSizeF();

    float textPadding = pointSize * .5f;

    float labelsMaxWidth = float(findLabelsMaxWidth(axisX->labels())) + textPadding;
    QFontMetrics fm(m_controller->activeTheme()->font());
    float labelHeight = fm.height() + textPadding;

    float scaleFactor = fontScaleFactor(pointSize) * pointSize;
    float fontRatio = labelsMaxWidth / labelHeight;
    QVector3D fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    auto adjustment = labelAdjustment(labelsMaxWidth);
    zPos = backgroundScale.z() + adjustment + m_labelMargin;

    adjustment *= qAbs(qSin(qDegreesToRadians(labelRotation.z())));
    yPos = backgroundScale.y() + adjustment;

    if (!yFlipped)
        yPos *= -1.0f;

    if (zFlipped)
        zPos *= -1.0f;

    auto labelTrans = QVector3D(0.0f, yPos, zPos);
    float polarLabelZPos = 0.0f;

    if (axisX->type() == QAbstract3DAxis::AxisTypeValue) {
        auto valueAxisX = static_cast<QValue3DAxis *>(axisX);
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
                labelTrans.setX(-qSin(rad) * -scale + qSin(rad) * m_labelMargin * 2.0f);
                labelTrans.setY(yPos);
                labelTrans.setZ(qCos(rad) * -scale - qCos(rad) * m_labelMargin * 2.0f);
                if (i == 0)
                    polarLabelZPos = labelTrans.z();
            } else {
                labelTrans.setX(valueAxisX->labelPositionAt(i) * scale * 2.0f - scale);
            }
            obj->setObjectName(QStringLiteral("ElementAxisXLabel"));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    } else if (axisX->type() == QAbstract3DAxis::AxisTypeCategory) {
        for (int i = 0; i < repeaterX()->count(); i++) {
            if (labelCount <= i)
                break;
            labelTrans = calculateCategoryLabelPosition(axisX, labelTrans, i);
            auto obj = static_cast<QQuick3DNode *>(repeaterX()->objectAt(i));
            obj->setObjectName(QStringLiteral("ElementAxisXLabel"));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    }

    float x = labelTrans.x();
    labelTrans.setX(0.0f);
    updateXTitle(labelRotation, labelTrans, totalRotation, labelsMaxWidth, fontScaled);
    if (isPolar()) {
        m_titleLabelX->setZ(polarLabelZPos - m_labelMargin * 2.0f);
        m_titleLabelX->setRotation(totalRotation);
    }
    labelTrans.setX(x);

    auto axisY = m_controller->axisY();
    labels = axisY->labels();
    labelCount = labels.size();
    labelAutoAngle = axisY->labelAutoRotation();
    labelAngleFraction = labelAutoAngle / 90.0f;
    fractionCamX = m_controller->scene()->activeCamera()->xRotation() * labelAngleFraction;
    fractionCamY = m_controller->scene()->activeCamera()->yRotation() * labelAngleFraction;

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
    labelsMaxWidth = float(findLabelsMaxWidth(axisY->labels())) + textPadding;
    fontRatio = labelsMaxWidth / labelHeight;
    fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);

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
        labelTrans.setY(static_cast<QValue3DAxis *>(axisY)->labelPositionAt(i) * scale * 2.0f - scale);
        obj->setObjectName(QStringLiteral("ElementAxisYLabel"));
        obj->setScale(fontScaled);
        obj->setPosition(labelTrans);
        obj->setRotation(totalRotation);
        obj->setProperty("labelText", labels[i]);
        obj->setProperty("labelWidth", labelsMaxWidth);
        obj->setProperty("labelHeight", labelHeight);
    }

    auto sideLabelTrans = labelTrans;
    auto totalSideLabelRotation = totalRotation;

    auto axisZ = m_controller->axisZ();
    labels = axisZ->labels();
    labelCount = labels.size();
    labelAutoAngle = axisZ->labelAutoRotation();
    labelAngleFraction = labelAutoAngle / 90.0f;
    fractionCamX = m_controller->scene()->activeCamera()->xRotation() * labelAngleFraction;
    fractionCamY = m_controller->scene()->activeCamera()->yRotation() * labelAngleFraction;

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
                    labelRotation.setX(90.0f - (labelAutoAngle - fractionCamX)
                                       * (-labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(90.0f + (labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(90.0f + (labelAutoAngle - fractionCamX)
                                       * -(labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(90.0f - (labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle + fractionCamY);
                }
            }
        } else {
            if (zFlipped) {
                if (xFlipped) {
                    labelRotation.setX(-90.0f + (labelAutoAngle - fractionCamX)
                                       * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                } else {
                    labelRotation.setX(-90.0f - (labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                }
            } else {
                if (xFlipped) {
                    labelRotation.setX(-90.0f - (labelAutoAngle - fractionCamX)
                                       * (-labelAutoAngle + fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(labelAutoAngle - fractionCamY);
                } else {
                    labelRotation.setX(-90.0f + (labelAutoAngle + fractionCamX)
                                       * (labelAutoAngle - fractionCamY) / labelAutoAngle);
                    labelRotation.setZ(-labelAutoAngle + fractionCamY);
                }
            }
        }
    }

    totalRotation = Utils::calculateRotation(labelRotation);

    scale = backgroundScale.z() - m_backgroundScaleMargin.z();
    labelsMaxWidth = float(findLabelsMaxWidth(axisZ->labels())) + textPadding ;
    fontRatio = labelsMaxWidth / labelHeight;
    fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
    adjustment = labelAdjustment(labelsMaxWidth);
    xPos = backgroundScale.x() + adjustment + m_labelMargin;
    if (xFlipped)
        xPos *= -1.0f;

    adjustment *= qAbs(qSin(qDegreesToRadians(labelRotation.z())));
    yPos = backgroundScale.y() + adjustment;
    if (!yFlipped)
        yPos *= -1.0f;

    labelTrans = QVector3D(xPos, yPos, 0.0f);

    if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
        auto valueAxisZ = static_cast<QValue3DAxis *>(axisZ);
        float offset = m_controller->radialLabelOffset();
        for (int i = 0; i < repeaterZ()->count(); i++) {
            if (labelCount <= i)
                break;
            auto obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
            if (isPolar()) {
                float polarX = backgroundScale.x() * offset + m_labelMargin * 2.0f;
                if (xFlipped)
                    polarX *= -1;
                labelTrans.setX(polarX);
                labelTrans.setY(yPos);
                labelTrans.setZ(-valueAxisZ->labelPositionAt(i));
            } else {
                labelTrans.setZ(valueAxisZ->labelPositionAt(i) * scale * -2.0f + scale);
            }
            obj->setObjectName(QStringLiteral("ElementAxisZLabel"));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    } else if (axisZ->type() == QAbstract3DAxis::AxisTypeCategory) {
        for (int i = 0; i < repeaterZ()->count(); i++) {
            if (labelCount <= i)
                break;
            labelTrans = calculateCategoryLabelPosition(axisZ, labelTrans, i);
            auto obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
            obj->setObjectName(QStringLiteral("ElementAxisZLabel"));
            obj->setScale(fontScaled);
            obj->setPosition(labelTrans);
            obj->setRotation(totalRotation);
            obj->setProperty("labelText", labels[i]);
            obj->setProperty("labelWidth", labelsMaxWidth);
            obj->setProperty("labelHeight", labelHeight);
        }
    }

    float z = labelTrans.z();
    labelTrans.setZ(0.0f);
    updateZTitle(labelRotation, labelTrans, totalRotation, labelsMaxWidth, fontScaled);
    labelTrans.setZ(z);

    labels = axisY->labels();
    labelCount = labels.size();
    totalRotation = Utils::calculateRotation(backLabelRotation);
    scale = backgroundScale.y() - m_backgroundScaleMargin.y();
    labelsMaxWidth = float(findLabelsMaxWidth(axisY->labels())) + textPadding;
    fontRatio = labelsMaxWidth / labelHeight;
    fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);
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
        auto obj = static_cast<QQuick3DNode *>(repeaterY()->objectAt(i + (repeaterY()->count() / 2)));
        labelTrans.setY(static_cast<QValue3DAxis *>(axisY)->labelPositionAt(i) * scale * 2.0f - scale);
        obj->setScale(fontScaled);
        obj->setPosition(labelTrans);
        obj->setRotation(totalRotation);
        obj->setProperty("labelText", labels[i]);
        obj->setProperty("labelWidth", labelsMaxWidth);
        obj->setProperty("labelHeight", labelHeight);
    }

    auto backLabelTrans = labelTrans;
    auto totalBackLabelRotation = totalRotation;
    updateYTitle(sideLabelRotation, backLabelRotation,
                 sideLabelTrans,    backLabelTrans,
                 totalSideLabelRotation, totalBackLabelRotation,
                 labelsMaxWidth, fontScaled);
}

void QQuickGraphsItem::updateRadialLabelOffset()
{
    if (!isPolar())
        return;

    QAbstract3DAxis *axisZ = m_controller->axisZ();
    QVector3D backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float offset = m_controller->radialLabelOffset();
    float scale = backgroundScale.x() + (m_backgroundScaleMargin.x());
    float polarX = scale * offset + m_labelMargin * 2.0f;
    if (isXFlipped())
        polarX *= -1;
    if (axisZ->type() == QAbstract3DAxis::AxisTypeValue) {
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

void QQuickGraphsItem::positionAndScaleLine(QQuick3DNode *lineNode, QVector3D scale, QVector3D position)
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
        m_controller->setQueriedGraphPosition(QVector3D(result.scenePosition().x(),
                                                        result.scenePosition().y(),
                                                        result.scenePosition().z()));
    }

    if (!isHit)
        m_controller->setQueriedGraphPosition(QVector3D(0,0,0));

    emit queriedGraphPositionChanged(m_controller->queriedGraphPosition());
    emit m_controller->queriedGraphPositionChanged(m_controller->queriedGraphPosition());
    m_controller->setGraphPositionQueryPending(false);
    scene()->setGraphPositionQuery(Q3DScene::invalidSelectionPoint());
}

void QQuickGraphsItem::updateShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    if (quality != QAbstract3DGraph::ShadowQualityNone) {
        light()->setCastsShadow(true);
        light()->setShadowFactor(25.f);

        QQuick3DAbstractLight::QSSGShadowMapQuality shadowMapQuality;
        switch (quality) {
        case QAbstract3DGraph::ShadowQualityLow:
        case QAbstract3DGraph::ShadowQualitySoftLow:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityMedium;
            break;
        case QAbstract3DGraph::ShadowQualityMedium:
        case QAbstract3DGraph::ShadowQualitySoftMedium:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityHigh;
            break;
        case QAbstract3DGraph::ShadowQualityHigh:
        case QAbstract3DGraph::ShadowQualitySoftHigh:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityVeryHigh;
            break;
        default:
            shadowMapQuality = QQuick3DAbstractLight::QSSGShadowMapQuality::ShadowMapQualityHigh;
            break;
        }
        light()->setShadowMapQuality(shadowMapQuality);
        if (quality >= QAbstract3DGraph::ShadowQualitySoftLow)
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
    int pointSize = m_controller->activeTheme()->font().pointSize();
    float scale = m_labelScale.x() * ((-10.0f * pointSize) + 650.0f) / pos2d.z();
    if (m_sliceView && m_sliceView->isVisible())
        m_itemLabel->setScale(scale * .2f);
    else
        m_itemLabel->setScale(scale);
    pos2d.setX(pos2d.x() - (m_itemLabel->width() / 2.f));
    pos2d.setY(pos2d.y() - (m_itemLabel->height() / 2.f) - (m_itemLabel->height() * m_itemLabel->scale()));
    m_itemLabel->setPosition(pos2d.toPointF());
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
    textureData->setTextureData(QByteArray::fromRawData(
                                    reinterpret_cast<const char *>(volume->textureData()->constData()),
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

        QByteArray colorTableBytes;
        const QList<QRgb> &colorTable = volume->colorTable();
        for (int i = 0; i < colorTable.size(); i++) {
            QRgb shifted = qRgba(qBlue(colorTable[i]), qGreen(colorTable[i]), qRed(colorTable[i]), qAlpha(colorTable[i]));
            colorTableBytes.append(QByteArray::fromRawData(reinterpret_cast<const char *>(&shifted), sizeof(shifted)));
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

    if (volume->drawSlices())
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

    material->setProperty("textureDimensions", QVector3D(1.0f / float(volume->textureWidth()),
                                                         1.0f / float(volume->textureHeight()),
                                                         1.0f / float(volume->textureDepth())));

    materialsRef.append(material);

    volumeItem.useHighDefShader = volume->useHighDefShader();
    volumeItem.drawSlices = volume->drawSlices();
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
    QQuick3DCustomMaterial *material = createQmlCustomMaterial(QStringLiteral(":/materials/VolumeFrameMaterial"));
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
    QAbstract3DAxis *axisX = m_controller->axisX();
    QAbstract3DAxis *axisY = m_controller->axisY();
    QAbstract3DAxis *axisZ = m_controller->axisZ();

    int maxX = axisX->max();
    int minX = axisX->min();
    int maxY = axisY->max();
    int minY = axisY->min();
    int maxZ = axisZ->max();
    int minZ = axisZ->min();
    QVector3D adjustment = m_scaleWithBackground * QVector3D(1.0f, 1.0f, -1.0f);
    bool isPolar = m_controller->isPolar();

    auto itemIterator = m_customItemList.constBegin();
    while (itemIterator != m_customItemList.constEnd()) {
        QCustom3DItem *item = itemIterator.key();
        QQuick3DModel *model = itemIterator.value();

        if (auto volume = qobject_cast<QCustom3DVolume *>(item)) {
            QVector3D pos = item->position();
            if (!item->isPositionAbsolute()) {
                if (item->position().x() < minX
                        || item->position().x() > maxX
                        || item->position().y() < minY
                        || item->position().y() > maxY
                        || item->position().z() < minZ
                        || item->position().z() > maxZ) {
                    model->setVisible(false);
                    ++itemIterator;
                    continue;
                }
                float xNormalizer = maxX - minX;
                float xPos = (pos.x() - minX) / xNormalizer;
                float yNormalizer = maxY - minY;
                float yPos = (pos.y() - minY) / yNormalizer;
                float zNormalizer = maxZ - minZ;
                float zPos = (pos.z() - minZ) / zNormalizer;
                pos = QVector3D(xPos, yPos, zPos);
                if (isPolar) {
                    float angle = xPos * M_PI * 2.0f;
                    float radius = zPos;
                    xPos = radius * qSin(angle) * 1.0f;
                    zPos = -(radius * qCos(angle)) * 1.0f;
                    yPos = yPos * adjustment.y() * 2.0f - adjustment.y();
                    pos = QVector3D(xPos, yPos, zPos);
                } else {
                    pos = pos * adjustment * 2.0f - adjustment;
                }
            }
            model->setPosition(pos);

            auto &&volumeItem = m_customVolumes[volume];

            QQmlListReference materialsRef(model, "materials");
            if (volumeItem.useHighDefShader != volume->useHighDefShader()) {
                materialsRef.clear();
                createVolumeMaterial(volume, volumeItem);
            }

            if (volumeItem.drawSlices != volume->drawSlices()) {
                materialsRef.clear();
                createVolumeMaterial(volume, volumeItem);
            }

            QVector3D sliceIndices((float(volume->sliceIndexX()) + 0.5f) / float(volume->textureWidth()) * 2.0 - 1.0,
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

            if (!volume->isScalingAbsolute() && !volume->isPositionAbsolute()) {
                if (m_controller->axisX()->type() == QAbstract3DAxis::AxisTypeValue) {
                    auto axis = static_cast<QValue3DAxis *>(m_controller->axisX());
                    translation.setX(axis->positionAt(volume->position().x()) + translate().x() / scale().x());
                    scaling.setX((axis->max() - axis->min()) / volume->scaling().x());
                    minBounds.setX(minBounds.x() * scaling.x() - translation.x());
                    maxBounds.setX(maxBounds.x() * scaling.x() - translation.x());
                }

                if (m_controller->axisY()->type() == QAbstract3DAxis::AxisTypeValue) {
                    auto axis = static_cast<QValue3DAxis *>(m_controller->axisY());
                    translation.setY(axis->positionAt(volume->position().y()) + translate().y() / scale().y());
                    scaling.setY((axis->max() - axis->min()) / volume->scaling().y());
                    minBounds.setY(minBounds.y() * scaling.y() + translation.y());
                    maxBounds.setY(maxBounds.y() * scaling.y() + translation.y());
                }

                if (m_controller->axisZ()->type() == QAbstract3DAxis::AxisTypeValue) {
                    auto axis = static_cast<QValue3DAxis *>(m_controller->axisZ());
                    translation.setZ(axis->positionAt(volume->position().z()) + translate().z() / scale().z());
                    scaling.setZ((axis->max() - axis->min()) / volume->scaling().z());
                    minBounds.setZ(minBounds.z() * scaling.z() - translation.z());
                    maxBounds.setZ(maxBounds.z() * scaling.z() - translation.z());
                }

                model->setPosition(QVector3D());
                model->setScale(QVector3D(qAbs(scale().x()) / 2, qAbs(scale().y()) / 2, qAbs(scale().z()) / 2));
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

                if (sliceFrameX < - 1 || sliceFrameX > 1) {
                    volumeItem.sliceFrameX->setVisible(false);
                } else {
                    volumeItem.sliceFrameX->setVisible(true);
                }

                if (sliceFrameY < - 1 || sliceFrameY > 1) {
                    volumeItem.sliceFrameY->setVisible(false);
                } else {
                    volumeItem.sliceFrameY->setVisible(true);
                }

                if (sliceFrameZ < - 1 || sliceFrameZ > 1) {
                    volumeItem.sliceFrameZ->setVisible(false);
                } else {
                    volumeItem.sliceFrameZ->setVisible(true);
                }

                volumeItem.sliceFrameX->setX(sliceFrameX);
                volumeItem.sliceFrameY->setY(-sliceFrameY);
                volumeItem.sliceFrameZ->setZ(-sliceFrameZ);
            }

            material->setProperty("alphaMultiplier", volume->alphaMultiplier());
            material->setProperty("preserveOpacity", volume->preserveOpacity());
            material->setProperty("useOrtho", m_controller->isOrthoProjection());

            int sampleCount = volume->textureWidth() + volume->textureHeight() + volume->textureDepth();
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

                textureData->setTextureData(QByteArray::fromRawData(
                                                reinterpret_cast<const char *>(volume->textureData()->constData()),
                                                volume->textureData()->size()));

                material->setProperty("textureDimensions", QVector3D(1.0f / float(volume->textureWidth()),
                                                                     1.0f / float(volume->textureHeight()),
                                                                     1.0f / float(volume->textureDepth())));

                volumeItem.updateTextureData = false;
            }

            if (volumeItem.updateColorTextureData) {
                auto colorTextureData = volumeItem.colorTextureData;
                QByteArray colorTableBytes;
                const QList<QRgb> &colorTable = volume->colorTable();
                for (int i = 0; i < colorTable.size(); i++) {
                    QRgb shifted = qRgba(qBlue(colorTable[i]), qGreen(colorTable[i]), qRed(colorTable[i]), qAlpha(colorTable[i]));
                    colorTableBytes.append(QByteArray::fromRawData(reinterpret_cast<const char *>(&shifted), sizeof(shifted)));
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
    QFontMetrics labelFM(m_controller->activeTheme()->font());

    for (const auto &label : std::as_const(labels)) {
        auto width = labelFM.horizontalAdvance(label);
        if (labelWidth < width)
            labelWidth = width;
    }
    return labelWidth;
}

QVector3D QQuickGraphsItem::calculateCategoryLabelPosition(QAbstract3DAxis *axis, QVector3D labelPosition, int index)
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

void QQuickGraphsItem::updateXTitle(const QVector3D &labelRotation, const QVector3D &labelTrans,
                                    const QQuaternion &totalRotation, float labelsMaxWidth,
                                    const QVector3D &scale)
{
    float pointSize = m_controller->activeTheme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(m_controller->activeTheme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(m_controller->axisX()->title()) + textPadding;

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
    QVector3D titleOffsetVector =
            offsetRotator.rotatedVector(QVector3D(0.0f, 0.0f, titleOffset));

    QQuaternion titleRotation;
    if (m_controller->axisX()->isTitleFixed()) {
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
    float pointSize = m_controller->activeTheme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(m_controller->activeTheme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(m_controller->axisY()->title()) + textPadding;

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
    QVector3D titleOffsetVector =
            offsetRotator.rotatedVector(QVector3D(-titleOffset, 0.0f, 0.0f));

    QQuaternion titleRotation;
    if (m_controller->axisY()->isTitleFixed()) {
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

void QQuickGraphsItem::updateZTitle(const QVector3D &labelRotation, const QVector3D &labelTrans,
                                    const QQuaternion &totalRotation, float labelsMaxWidth,
                                    const QVector3D &scale)
{
    float pointSize = m_controller->activeTheme()->font().pointSizeF();
    float textPadding = pointSize * .5f;
    QFontMetrics fm(m_controller->activeTheme()->font());
    float height = fm.height() + textPadding;
    float width = fm.horizontalAdvance(m_controller->axisZ()->title()) + textPadding;

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
    QVector3D titleOffsetVector =
            offsetRotator.rotatedVector(QVector3D(titleOffset, 0.0f, 0.0f));

    QQuaternion titleRotation;
    if (m_controller->axisZ()->isTitleFixed()) {
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
    QVector3D lookingPosition = m_controller->scene()->activeCamera()->target();
    float zoomLevel = m_controller->scene()->activeCamera()->zoomLevel();

    const float scale = qMin(width(), height() * 1.6f);
    const float magnificationScaleFactor = 1.0f / 640.0f;
    const float magnification = scale * magnificationScaleFactor;

    auto useOrtho = m_controller->isOrthoProjection();
    if (useOrtho) {
        if (m_sliceView && m_sliceView->isVisible()) {
            m_oCamera->setVerticalMagnification(zoomLevel * .4f);
            m_oCamera->setHorizontalMagnification(zoomLevel * .4f);
        } else {
            m_oCamera->setVerticalMagnification(zoomLevel * magnification);
            m_oCamera->setHorizontalMagnification(zoomLevel * magnification);
        }
    }
    cameraTarget()->setPosition(lookingPosition);
    auto rotation = QVector3D(
                -m_controller->scene()->activeCamera()->yRotation(),
                -m_controller->scene()->activeCamera()->xRotation(),
                0);
    cameraTarget()->setEulerRotation(rotation);
    float zoom = 720.f / zoomLevel;
    m_pCamera->setZ(zoom);
    updateCustomLabelsRotation();
    updateItemLabel(m_labelPosition);
}

void QQuickGraphsItem::handleSegmentLineCountChanged(QAbstract3DAxis *axis, QQuick3DRepeater *repeater)
{
    int gridLineCount = 0;
    if (axis->type() == QAbstract3DAxis::AxisTypeValue) {
        auto valueAxis = static_cast<QValue3DAxis *>(axis);
        gridLineCount = 2 * valueAxis->gridSize();
    } else if (axis->type() == QAbstract3DAxis::AxisTypeCategory) {
        gridLineCount = axis->labels().size();
    }
    repeater->setModel(gridLineCount);
    changeGridLineColor(repeater, m_controller->activeTheme()->gridLineColor());
    m_controller->handleAxisSubSegmentCountChangedBySender(axis);
}

void QQuickGraphsItem::handleSubSegmentLineCountChanged(QAbstract3DAxis *axis, QQuick3DRepeater *repeater)
{
    int subGridLineCount = 0;

    if (axis->type() == QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
        subGridLineCount = 2 * valueAxis->subGridSize();
    } else if (axis->type() == QAbstract3DAxis::AxisTypeCategory) {
        subGridLineCount = 0;
    }

    repeater->setModel(subGridLineCount);
    changeGridLineColor(repeater, m_controller->activeTheme()->gridLineColor());
}

void QQuickGraphsItem::handleLabelCountChanged(QQuick3DRepeater *repeater)
{
    Q3DTheme *theme = m_controller->activeTheme();
    changeLabelBackgroundColor(repeater, theme->labelBackgroundColor());
    changeLabelBackgroundEnabled(repeater, theme->isLabelBackgroundEnabled());
    changeLabelBorderEnabled(repeater, theme->isLabelBorderEnabled());
    changeLabelTextColor(repeater, theme->labelTextColor());
    changeLabelFont(repeater, theme->font());

    if (m_sliceView) {
        changeLabelBackgroundColor(m_sliceHorizontalLabelRepeater, theme->labelBackgroundColor());
        changeLabelBackgroundColor(m_sliceVerticalLabelRepeater, theme->labelBackgroundColor());
        changeLabelBackgroundEnabled(m_sliceHorizontalLabelRepeater, theme->isLabelBackgroundEnabled());
        changeLabelBackgroundEnabled(m_sliceVerticalLabelRepeater, theme->isLabelBackgroundEnabled());
        changeLabelBorderEnabled(m_sliceHorizontalLabelRepeater, theme->isLabelBorderEnabled());
        changeLabelBorderEnabled(m_sliceVerticalLabelRepeater, theme->isLabelBorderEnabled());
        changeLabelTextColor(m_sliceHorizontalLabelRepeater, theme->labelTextColor());
        changeLabelTextColor(m_sliceVerticalLabelRepeater, theme->labelTextColor());
        changeLabelFont(m_sliceHorizontalLabelRepeater, theme->font());
        changeLabelFont(m_sliceVerticalLabelRepeater, theme->font());
    }
}

void QQuickGraphsItem::updateCustomData()
{
    QAbstract3DAxis *axisX = m_controller->axisX();
    QAbstract3DAxis *axisY = m_controller->axisY();
    QAbstract3DAxis *axisZ = m_controller->axisZ();

    int maxX = axisX->max();
    int minX = axisX->min();
    int maxY = axisY->max();
    int minY = axisY->min();
    int maxZ = axisZ->max();
    int minZ = axisZ->min();
    QVector3D adjustment = m_scaleWithBackground * QVector3D(1.0f, 1.0f, -1.0f);
    bool isPolar = m_controller->isPolar();

    auto labelIterator = m_customLabelList.constBegin();
    while (labelIterator != m_customLabelList.constEnd()) {
        QCustom3DLabel *label = labelIterator.key();
        QQuick3DNode *customLabel = labelIterator.value();

        QVector3D pos = label->position();
        if (!label->isPositionAbsolute()) {
            if (label->position().x() < minX
                    || label->position().x() > maxX
                    || label->position().y() < minY
                    || label->position().y() > maxY
                    || label->position().z() < minZ
                    || label->position().z() > maxZ) {
                customLabel->setVisible(false);
                ++labelIterator;
                continue;
            }

            float xNormalizer = maxX - minX;
            float xPos = (pos.x() - minX) / xNormalizer;
            float yNormalizer = maxY - minY;
            float yPos = (pos.y() - minY) / yNormalizer;
            float zNormalizer = maxZ - minZ;
            float zPos = (pos.z() - minZ) / zNormalizer;
            pos = QVector3D(xPos, yPos, zPos);
            if (isPolar) {
                float angle = xPos * M_PI * 2.0f;
                float radius = zPos;
                xPos = radius * qSin(angle) * 1.0f;
                zPos = -(radius * qCos(angle)) * 1.0f;
                yPos = yPos * adjustment.y() * 2.0f - adjustment.y();
                pos = QVector3D(xPos, yPos, zPos);
            } else {
                pos = pos * adjustment * 2.0f - adjustment;
            }
        }

        QFontMetrics fm(label->font());
        int width = fm.horizontalAdvance(label->text());
        int height = fm.height();
        customLabel->setProperty("labelWidth", width);
        customLabel->setProperty("labelHeight", height);
        customLabel->setPosition(pos);
        QQuaternion rotation = label->rotation();
        if (label->isFacingCamera()) {
            rotation = Utils::calculateRotation(QVector3D(
                        -m_controller->scene()->activeCamera()->yRotation(),
                        -m_controller->scene()->activeCamera()->xRotation(),
                        0));
        }
        customLabel->setRotation(rotation);
        float pointSize = m_controller->activeTheme()->font().pointSizeF();
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
        if (!item->isPositionAbsolute()) {
            if (item->position().x() < minX
                    || item->position().x() > maxX
                    || item->position().y() < minY
                    || item->position().y() > maxY
                    || item->position().z() < minZ
                    || item->position().z() > maxZ) {
                model->setVisible(false);
                ++itemIterator;
                continue;
            }
            float xNormalizer = maxX - minX;
            float xPos = (pos.x() - minX) / xNormalizer;
            float yNormalizer = maxY - minY;
            float yPos = (pos.y() - minY) / yNormalizer;
            float zNormalizer = maxZ - minZ;
            float zPos = (pos.z() - minZ) / zNormalizer;
            pos = QVector3D(xPos, yPos, zPos);
            if (isPolar) {
                float angle = xPos * M_PI * 2.0f;
                float radius = zPos;
                xPos = radius * qSin(angle) * 1.0f;
                zPos = -(radius * qCos(angle)) * 1.0f;
                yPos = yPos * adjustment.y() * 2.0f - adjustment.y();
                pos = QVector3D(xPos, yPos, zPos);
            } else {
                pos = pos * adjustment * 2.0f - adjustment;
            }
        }
        model->setPosition(pos);

        if (auto volume = qobject_cast<QCustom3DVolume *>(item)) {
            if (!m_customVolumes.contains(volume)) {
                auto &&volumeItem = m_customVolumes[volume];

                volumeItem.model = model;
                model->setSource(QUrl(volume->meshFile()));

                volumeItem.useHighDefShader = volume->useHighDefShader();
                volumeItem.drawSlices = volume->drawSlices();

                createVolumeMaterial(volume, volumeItem);

                volumeItem.sliceFrameX = createSliceFrame(volumeItem);
                volumeItem.sliceFrameY = createSliceFrame(volumeItem);
                volumeItem.sliceFrameZ = createSliceFrame(volumeItem);

                if (volume->drawSliceFrames()) {
                    volumeItem.sliceFrameX->setVisible(true);
                    volumeItem.sliceFrameY->setVisible(true);
                    volumeItem.sliceFrameZ->setVisible(true);

                    QVector3D sliceIndices((float(volume->sliceIndexX()) + 0.5f) / float(volume->textureWidth()) * 2.0 - 1.0,
                                           (float(volume->sliceIndexY()) + 0.5f) / float(volume->textureHeight()) * 2.0 - 1.0,
                                           (float(volume->sliceIndexZ()) + 0.5f) / float(volume->textureDepth()) * 2.0 - 1.0);

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
            QQuick3DPrincipledMaterial *material = static_cast<QQuick3DPrincipledMaterial *>(materialsRef.at(0));
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
                QImage textureImage = m_controller->customTextureImage(item);
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
                textureData->setTextureData(QByteArray(reinterpret_cast<const char*>(textureImage.bits()),
                                                       textureImage.sizeInBytes()));
            }
            model->setRotation(item->rotation());
            model->setScale(item->scaling());
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
        if (label->isFacingCamera()) {
            rotation = Utils::calculateRotation(QVector3D(
                        -m_controller->scene()->activeCamera()->yRotation(),
                        -m_controller->scene()->activeCamera()->xRotation(),
                        0));
        }
        customLabel->setRotation(rotation);
        ++labelIterator;
    }
}

int QQuickGraphsItem::msaaSamples() const
{
    if (m_renderMode == QAbstract3DGraph::RenderIndirect)
        return m_samples;
    else
        return m_windowSamples;
}

void QQuickGraphsItem::setMsaaSamples(int samples)
{
    if (m_renderMode != QAbstract3DGraph::RenderIndirect) {
        qWarning("Multisampling cannot be adjusted in this render mode");
    } else if (m_samples != samples) {
        m_samples = samples;
        setAntialiasing(m_samples > 0);
        auto sceneEnv = environment();
        sceneEnv->setAntialiasingMode(m_samples > 0
                                          ? QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues::MSAA
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

Declarative3DScene *QQuickGraphsItem::scene() const
{
    return static_cast<Declarative3DScene *>(m_controller->scene());
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
    typedef void * (*EnableTouch)(QWindow*, bool);
    EnableTouch enableTouch =
            (EnableTouch)QGuiApplication::platformNativeInterface()->nativeResourceFunctionForIntegration("registertouchwindow");
    if (enableTouch)
        enableTouch(window, true);
    window->setVisible(previousVisibility);
#endif

    connect(window, &QObject::destroyed, this, &QQuickGraphsItem::windowDestroyed);

    int oldWindowSamples = m_windowSamples;
    m_windowSamples = window->format().samples();
    if (m_windowSamples < 0)
        m_windowSamples = 0;

    connect(window, &QQuickWindow::beforeSynchronizing,
            this, &QQuickGraphsItem::synchData);

    if (m_renderMode == QAbstract3DGraph::RenderDirectToBackground) {
        setAntialiasing(m_windowSamples > 0);
        if (m_windowSamples != oldWindowSamples)
            emit msaaSamplesChanged(m_windowSamples);
    }

    connect(m_controller.data(), &Abstract3DController::needRender, window, &QQuickWindow::update);
    // Force camera update before rendering the first frame
    // to workaround a Quick3D device pixel ratio bug
    connect(window, &QQuickWindow::beforeRendering, this, [this, window]() {
        m_oCamera->setClipNear(0.001f);
        disconnect(window, &QQuickWindow::beforeRendering, this, nullptr);
    });
    updateWindowParameters();

#if defined(Q_OS_IOS)
    // Scenegraph render cycle in iOS sometimes misses update after beforeSynchronizing signal.
    // This ensures we don't end up displaying the graph without any data, in case update is
    // skipped after synchDataToRenderer.
    QTimer::singleShot(0, window, SLOT(update()));
#endif
}

void QQuickGraphsItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    m_cachedGeometry = newGeometry;
    updateWindowParameters();
}

void QQuickGraphsItem::itemChange(ItemChange change, const ItemChangeData  &value)
{
    QQuick3DViewport::itemChange(change, value);
    updateWindowParameters();
}

void QQuickGraphsItem::updateWindowParameters()
{
    const QMutexLocker locker(&m_mutex);
    // Update the device pixel ratio, window size and bounding box
    QQuickWindow *win = window();
    if (win && !m_controller.isNull()) {
        Q3DScene *scene = m_controller->scene();
        if (win->devicePixelRatio() != scene->devicePixelRatio()) {
            scene->setDevicePixelRatio(win->devicePixelRatio());
            win->update();
        }

        bool directRender = m_renderMode == QAbstract3DGraph::RenderDirectToBackground;
        QSize windowSize;

        if (directRender)
            windowSize = win->size();
        else
            windowSize = m_cachedGeometry.size().toSize();

        if (windowSize != scene->d_func()->windowSize()) {
            scene->d_func()->setWindowSize(windowSize);
            win->update();
        }

        if (directRender) {
            // Origin mapping is needed when rendering directly to background
            QPointF point = QQuickItem::mapToScene(QPointF(0.0, 0.0));
            scene->d_func()->setViewport(QRect(point.x() + 0.5f, point.y() + 0.5f,
                                            m_cachedGeometry.width() + 0.5f,
                                            m_cachedGeometry.height() + 0.5f));
        } else {
            // No translation needed when rendering to FBO
            scene->d_func()->setViewport(QRect(0.0, 0.0, m_cachedGeometry.width() + 0.5f,
                                            m_cachedGeometry.height() + 0.5f));
        }
    }

    if (m_sliceView && m_sliceView->isVisible()) {
        const float scale = qMin(m_sliceView->width(), m_sliceView->height());
        QQuick3DOrthographicCamera *camera = static_cast<QQuick3DOrthographicCamera *>(m_sliceView->camera());
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
    m_controller->m_clickedType = type;
    emit selectedElementChanged(type);
}

void QQuickGraphsItem::handleOptimizationHintChange(QAbstract3DGraph::OptimizationHints hints)
{
    emit optimizationHintsChanged(hints);
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
        QObject::disconnect(oldWindow, &QObject::destroyed, this,
                            &QQuickGraphsItem::windowDestroyed);
        QObject::disconnect(oldWindow, &QQuickWindow::beforeSynchronizing, this,
                            &QQuickGraphsItem::synchData);
        if (!m_controller.isNull()) {
            QObject::disconnect(m_controller.data(), &Abstract3DController::needRender,
                                oldWindow, &QQuickWindow::update);
        }
    }

    QList<QQuickWindow *> windowList;

    foreach (QQuickGraphsItem *graph, m_graphWindowList.keys()) {
        if (graph->m_renderMode == QAbstract3DGraph::RenderDirectToBackground)
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
            QObject::connect(renderStats(), &QQuick3DRenderStats::fpsChanged,
                             this, &QQuickGraphsItem::handleFpsChanged);
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

// TODO: Check if it would make sense to remove these from Abstract3DController - QTBUG-113812
// begin..
void QQuickGraphsItem::setOrthoProjection(bool enable)
{
    m_controller->setOrthoProjection(enable);
}

bool QQuickGraphsItem::isOrthoProjection() const
{
    return m_controller->isOrthoProjection();
}

QAbstract3DGraph::ElementType QQuickGraphsItem::selectedElement() const
{
    return m_controller->selectedElement();
}

void QQuickGraphsItem::setAspectRatio(qreal ratio)
{
    m_controller->setAspectRatio(ratio);
}

qreal QQuickGraphsItem::aspectRatio() const
{
    return m_controller->aspectRatio();
}

void QQuickGraphsItem::setOptimizationHints(QAbstract3DGraph::OptimizationHints hints)
{
    int intmode = int(hints);
    m_controller->setOptimizationHints(QAbstract3DGraph::OptimizationHints(intmode));
}

QAbstract3DGraph::OptimizationHints QQuickGraphsItem::optimizationHints() const
{
    return m_controller->optimizationHints();
}

void QQuickGraphsItem::setPolar(bool enable)
{
    m_controller->setPolar(enable);
}

bool QQuickGraphsItem::isPolar() const
{
    return m_controller->isPolar();
}

void QQuickGraphsItem::setRadialLabelOffset(float offset)
{
    m_controller->setRadialLabelOffset(offset);
}

float QQuickGraphsItem::radialLabelOffset() const
{
    return m_controller->radialLabelOffset();
}

void QQuickGraphsItem::setHorizontalAspectRatio(qreal ratio)
{
    m_controller->setHorizontalAspectRatio(ratio);
}

qreal QQuickGraphsItem::horizontalAspectRatio() const
{
    return m_controller->horizontalAspectRatio();
}

void QQuickGraphsItem::setReflection(bool enable)
{
    m_controller->setReflection(enable);
}

bool QQuickGraphsItem::isReflection() const
{
    return m_controller->reflection();
}

void QQuickGraphsItem::setReflectivity(qreal reflectivity)
{
    m_controller->setReflectivity(reflectivity);
}

qreal QQuickGraphsItem::reflectivity() const
{
    return m_controller->reflectivity();
}

void QQuickGraphsItem::setLocale(const QLocale &locale)
{
    m_controller->setLocale(locale);
}

QLocale QQuickGraphsItem::locale() const
{
    return m_controller->locale();
}

QVector3D QQuickGraphsItem::queriedGraphPosition() const
{
    return m_controller->queriedGraphPosition();
}

void QQuickGraphsItem::setMargin(qreal margin)
{
    m_controller->setMargin(margin);
}

qreal QQuickGraphsItem::margin() const
{
    return m_controller->margin();
}
// ..end
// TODO: Check if it would make sense to remove these from Abstract3DController - QTBUG-113812

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
    if (m_controller->m_changeTracker.axisXTitleVisibilityChanged) {
        m_titleLabelX->setVisible(m_controller->axisX()->isTitleVisible());
        m_controller->m_changeTracker.axisXTitleVisibilityChanged = false;
    }

    if (m_controller->m_changeTracker.axisYTitleVisibilityChanged) {
        m_titleLabelY->setVisible(m_controller->axisY()->isTitleVisible());
        m_controller->m_changeTracker.axisYTitleVisibilityChanged = false;
    }

    if (m_controller->m_changeTracker.axisZTitleVisibilityChanged) {
        m_titleLabelZ->setVisible(m_controller->axisZ()->isTitleVisible());
        m_controller->m_changeTracker.axisZTitleVisibilityChanged = false;
    }

    if (m_controller->m_changeTracker.axisXTitleChanged) {
        m_titleLabelX->setProperty("labelText", m_controller->axisX()->title());
        m_controller->m_changeTracker.axisXTitleChanged = false;
    }

    if (m_controller->m_changeTracker.axisYTitleChanged) {
        m_titleLabelY->setProperty("labelText", m_controller->axisY()->title());
        m_controller->m_changeTracker.axisYTitleChanged = false;
    }

    if (m_controller->m_changeTracker.axisZTitleChanged) {
        m_titleLabelZ->setProperty("labelText", m_controller->axisZ()->title());
        m_controller->m_changeTracker.axisZTitleChanged = false;
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
        == QAbstract3DInputHandlerPrivate::InputStateSelecting) {
        QList<QQuick3DPickResult> results = pickAll(point.x(), point.y());
        if (!m_customItemList.isEmpty()) {
            // Try to pick custom item only
            for (const auto &result : results) {
                QCustom3DItem *customItem = m_customItemList.key(result.objectHit(), nullptr);

                if (customItem) {
                    int selectedIndex = m_controller->m_customItems.indexOf(customItem);
                    m_controller->m_selectedCustomItemIndex = selectedIndex;
                    handleSelectedElementChange(QAbstract3DGraph::ElementCustomItem);
                    // Don't allow picking in subclasses if custom item is picked
                    return false;
                }
            }
        }

        for (const auto &result : results) {
            if (!result.objectHit())
                continue;
            QString objName = result.objectHit()->objectName();
            if (objName.contains(QStringLiteral("ElementAxisXLabel")))
                handleSelectedElementChange(QAbstract3DGraph::ElementAxisXLabel);
            else if (objName.contains(QStringLiteral("ElementAxisYLabel")))
                handleSelectedElementChange(QAbstract3DGraph::ElementAxisYLabel);
            else if (objName.contains(QStringLiteral("ElementAxisZLabel")))
                handleSelectedElementChange(QAbstract3DGraph::ElementAxisZLabel);
            else
                continue;
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
        m_controller->setSlicingActive(false);
    } else {
        minimizeMainGraph();
        m_sliceView->setVisible(true);
        updateSliceGrid();
        updateSliceLabels();
        m_controller->setSlicingActive(true);
    }

    m_sliceActivatedChanged = false;
}

void QQuickGraphsItem::addInputHandler(QAbstract3DInputHandler *inputHandler)
{
    Q_ASSERT(inputHandler);
    QQuickGraphsItem *owner = qobject_cast<QQuickGraphsItem *>(inputHandler->parent());
    if (owner != this) {
        Q_ASSERT_X(!owner, "addInputHandler",
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
            QObject::disconnect(m_activeInputHandler, nullptr, m_controller, nullptr);
            QObject::disconnect(m_activeInputHandler, &QAbstract3DInputHandler::positionChanged,
                                this, &QQuickGraphsItem::doPicking);
        }
    }

    // Assume ownership and connect to this controller's scene
    if (inputHandler)
        addInputHandler(inputHandler);

    m_activeInputHandler = inputHandler;

    if (m_activeInputHandler) {
        m_activeInputHandler->setScene(scene());

        // Connect the input handler
        QObject::connect(m_activeInputHandler, &QAbstract3DInputHandler::inputViewChanged,
                         m_controller, &Abstract3DController::handleInputViewChanged);
        QObject::connect(m_activeInputHandler, &QAbstract3DInputHandler::positionChanged,
                         m_controller, &Abstract3DController::handleInputPositionChanged);
        QObject::connect(m_activeInputHandler, &QAbstract3DInputHandler::positionChanged,
                         this, &QQuickGraphsItem::doPicking);
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

bool QQuickGraphsItem::event(QEvent *event)
{
    return QQuickItem::event(event);
}

void QQuickGraphsItem::createSliceView()
{
    if (m_sliceView)
        return;

    connect(parentItem(), &QQuickItem::widthChanged , this, &QQuickGraphsItem::handleParentWidthChange);
    connect(parentItem(), &QQuickItem::heightChanged , this, &QQuickGraphsItem::handleParentHeightChange);

    m_sliceView = new QQuick3DViewport();
    m_sliceView->setParent(parent());
    m_sliceView->setParentItem(parentItem());
    m_sliceView->setVisible(false);

    m_sliceView->bindableHeight().setBinding([&] { return parentItem()->height(); });
    m_sliceView->bindableWidth().setBinding([&] { return parentItem()->width(); });

    auto scene = m_sliceView->scene();

    auto camera = new QQuick3DOrthographicCamera(scene);
    camera->setPosition(QVector3D(.0f, .0f, 20.0f));
    const float scale = qMin(m_sliceView->width(), m_sliceView->height());
    const float magnificationScaleFactor = 2 * window()->devicePixelRatio() * .08f; // this controls the size of the slice view
    const float magnification = scale * magnificationScaleFactor;
    camera->setHorizontalMagnification(magnification);
    camera->setVerticalMagnification(magnification);
    m_sliceView->setCamera(camera);

    auto light = new QQuick3DDirectionalLight(scene);
    light->setParent(camera);
    light->setParentItem(camera);

    auto gridDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/GridLine"));
    auto labelDelegate = createRepeaterDelegateComponent(QStringLiteral(":/axis/AxisLabel"));

    m_sliceHorizontalGridRepeater = createRepeater(scene);
    m_sliceHorizontalGridRepeater->setDelegate(gridDelegate);

    m_sliceVerticalGridRepeater = createRepeater(scene);
    m_sliceVerticalGridRepeater->setDelegate(gridDelegate);

    m_sliceHorizontalLabelRepeater = createRepeater(scene);
    m_sliceHorizontalLabelRepeater->setDelegate(labelDelegate);

    m_sliceVerticalLabelRepeater = createRepeater(scene);
    m_sliceVerticalLabelRepeater->setDelegate(labelDelegate);

    m_sliceHorizontalTitleLabel = createTitleLabel(scene);
    m_sliceHorizontalTitleLabel->setVisible(true);

    m_sliceVerticalTitleLabel = createTitleLabel(scene);
    m_sliceVerticalTitleLabel->setVisible(true);

    m_sliceItemLabel = createTitleLabel(scene);
    m_sliceItemLabel->setVisible(false);
}

void QQuickGraphsItem::updateSliceGrid()
{
    QAbstract3DAxis *horizontalAxis = nullptr;
    QAbstract3DAxis *verticalAxis = m_controller->axisY();
    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float scale;
    float translate;

    QVector3D horizontalScale = QVector3D(.0f, .0f, .0f);
    QVector3D verticalScale = QVector3D(lineWidthScaleFactor(),
                                        backgroundScale.y() * lineLengthScaleFactor(),
                                        lineWidthScaleFactor());
    auto selectionMode = m_controller->selectionMode();
    if (selectionMode.testFlag(QAbstract3DGraph::SelectionRow)) {
        horizontalAxis = m_controller->axisX();
        horizontalScale = QVector3D(backgroundScale.x() * lineLengthScaleFactor(),
                                    lineWidthScaleFactor(),
                                    lineWidthScaleFactor());
        scale = m_scaleWithBackground.x();
        translate = m_scaleWithBackground.x();
    } else if (selectionMode.testFlag(QAbstract3DGraph::SelectionColumn)) {
        horizontalAxis = m_controller->axisZ();
        horizontalScale = QVector3D(backgroundScale.z() * lineLengthScaleFactor(),
                                    lineWidthScaleFactor(),
                                    lineWidthScaleFactor());
        scale = m_scaleWithBackground.z();
        translate = m_scaleWithBackground.z();
    }

    if (horizontalAxis == nullptr) {
        qWarning("Invalid axis type");
        return;
    }

    if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(horizontalAxis);
        m_sliceVerticalGridRepeater->model().clear();
        m_sliceVerticalGridRepeater->setModel(valueAxis->gridSize()
                                              + valueAxis->subGridSize());
    } else if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeCategory) {
        m_sliceVerticalGridRepeater->model().clear();
        m_sliceVerticalGridRepeater->setModel(horizontalAxis->labels().size());
    }

    if (verticalAxis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(verticalAxis);
        m_sliceHorizontalGridRepeater->model().clear();
        m_sliceHorizontalGridRepeater->setModel(valueAxis->gridSize()
                                                + valueAxis->subGridSize());
    } else if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeCategory) {
        m_sliceHorizontalGridRepeater->model().clear();
        m_sliceHorizontalGridRepeater->setModel(verticalAxis->labels().size());
    }

    float linePosX = .0f;
    float linePosY = .0f;
    float linePosZ = -1.f; // Draw grid lines behind slice (especially for surface)

    if (horizontalAxis->type() == QAbstract3DAxis::AxisTypeCategory) {
        m_sliceVerticalGridRepeater->setVisible(false);
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisTypeValue) {
        for (int i = 0; i < m_sliceVerticalGridRepeater->count(); i++) {
            QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(m_sliceVerticalGridRepeater->objectAt(i));
            auto axis = static_cast<QValue3DAxis *>(horizontalAxis);
            if (i < axis->gridSize())
                linePosX = axis->gridPositionAt(i) * scale * 2.0f - translate;
            else
                linePosX = axis->subGridPositionAt(i - axis->gridSize()) * scale * 2.0f - translate;
            lineNode->setProperty("lineColor", QColor(0, 0, 0));
            positionAndScaleLine(lineNode, verticalScale, QVector3D(linePosX, linePosY, linePosZ));
        }
    }

    linePosX = 0;
    scale = m_scaleWithBackground.y();
    translate = m_scaleWithBackground.y();

    for (int i = 0; i < m_sliceHorizontalGridRepeater->count(); i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(m_sliceHorizontalGridRepeater->objectAt(i));
        if (verticalAxis->type() == QAbstract3DAxis::AxisTypeValue) {
            auto axis = static_cast<QValue3DAxis *>(verticalAxis);
            if (i < axis->gridSize())
                linePosY = axis->gridPositionAt(i) * scale * 2.0f - translate;
            else
                linePosY = axis->subGridPositionAt(i - axis->gridSize()) * scale * 2.0f - translate;
        } else if (verticalAxis->type() == QAbstract3DAxis::AxisTypeCategory) {
            linePosY = calculateCategoryGridLinePosition(verticalAxis, i);
        }
        lineNode->setProperty("lineColor", QColor(0, 0, 0));
        positionAndScaleLine(lineNode, horizontalScale, QVector3D(linePosX, linePosY, linePosZ));
    }
}

void QQuickGraphsItem::updateSliceLabels()
{
    QAbstract3DAxis *horizontalAxis = nullptr;
    QAbstract3DAxis *verticalAxis = m_controller->axisY();
    auto backgroundScale = m_scaleWithBackground + m_backgroundScaleMargin;
    float scale;
    float translate;
    auto selectionMode = m_controller->selectionMode();

    if (selectionMode.testFlag(QAbstract3DGraph::SelectionRow))
        horizontalAxis = m_controller->axisX();
    else if (selectionMode.testFlag(QAbstract3DGraph::SelectionColumn))
        horizontalAxis = m_controller->axisZ();

    scale = backgroundScale.x() - m_backgroundScaleMargin.x();
    translate = backgroundScale.x() - m_backgroundScaleMargin.x();

    if (horizontalAxis == nullptr) {
        qWarning("Invalid selection mode");
        return;
    }

    if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(horizontalAxis);
        m_sliceHorizontalLabelRepeater->model().clear();
        m_sliceHorizontalLabelRepeater->setModel(valueAxis->labels().size());
    } else if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeCategory) {
        m_sliceHorizontalLabelRepeater->model().clear();
        m_sliceHorizontalLabelRepeater->setModel(horizontalAxis->labels().size());
    }

    if (verticalAxis->type() & QAbstract3DAxis::AxisTypeValue) {
        QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(verticalAxis);
        m_sliceVerticalLabelRepeater->model().clear();
        m_sliceVerticalLabelRepeater->setModel(valueAxis->labels().size());
    } else if (horizontalAxis->type() & QAbstract3DAxis::AxisTypeCategory) {
        m_sliceVerticalLabelRepeater->model().clear();
        m_sliceVerticalLabelRepeater->setModel(verticalAxis->labels().size());
    }

    float textPadding = 12.0f;

    float labelsMaxWidth = float(findLabelsMaxWidth(horizontalAxis->labels())) + textPadding;
    QFontMetrics fm(m_controller->activeTheme()->font());
    float labelHeight = fm.height() + textPadding;

    float pointSize = m_controller->activeTheme()->font().pointSizeF();
    float scaleFactor = fontScaleFactor(pointSize) * pointSize;
    float fontRatio = labelsMaxWidth / labelHeight;
    QVector3D fontScaled = QVector3D(scaleFactor * fontRatio, scaleFactor, 0.00001f);

    float adjustment = labelsMaxWidth * scaleFactor;
    float yPos = backgroundScale.y() + adjustment;

    QVector3D labelTrans = QVector3D(0.0f, -yPos, 0.0f);
    QStringList labels = horizontalAxis->labels();
    Q3DTheme *theme = m_controller->activeTheme();
    QFont font = theme->font();
    bool borderEnabled = theme->isLabelBorderEnabled();
    QColor labelTextColor = theme->labelTextColor();
    bool backgroundEnabled = theme->isLabelBackgroundEnabled();
    QColor backgroundColor = theme->labelBackgroundColor();

    if (horizontalAxis->type() == QAbstract3DAxis::AxisTypeValue) {
        auto valueAxis = static_cast<QValue3DAxis *>(horizontalAxis);
        for (int i = 0; i < m_sliceHorizontalLabelRepeater->count(); i++) {
            auto obj = static_cast<QQuick3DNode *>(m_sliceHorizontalLabelRepeater->objectAt(i));
            labelTrans.setX(valueAxis->labelPositionAt(i) * scale * 2.0f - translate);
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
    } else if (horizontalAxis->type() == QAbstract3DAxis::AxisTypeCategory) {
        for (int i = 0; i < m_sliceHorizontalLabelRepeater->count(); i++) {
            labelTrans = calculateCategoryLabelPosition(horizontalAxis, labelTrans, i);
            labelTrans.setY(labelTrans.y() - (adjustment / 1.5f));
            if (m_controller->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
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
    adjustment = labelsMaxWidth * scaleFactor;
    float xPos = 0.0f;
    if (m_controller->selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
        xPos = backgroundScale.x() + adjustment;
    else if (m_controller->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
        xPos = backgroundScale.z() + adjustment;
    labelTrans = QVector3D(xPos, 0.0f, 0.0f);

    if (verticalAxis->type() == QAbstract3DAxis::AxisTypeValue) {
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
    } else if (verticalAxis->type() == QAbstract3DAxis::AxisTypeCategory) {
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
    if (m_controller->selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
        xPos = backgroundScale.x() + adjustment;
    else if (m_controller->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
        xPos = backgroundScale.z() + adjustment;
    labelTrans = QVector3D(-xPos, 0.0f, 0.0f);

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
    yPos = backgroundScale.y() * 1.5f + adjustment;
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

    m_sliceItemLabel->setScale(fontScaled);
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
    QQuick3DObjectPrivate::get(cameraTarget)->refSceneManager(
                *QQuick3DObjectPrivate::get(rootNode())->sceneManager);

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

    auto useOrtho = m_controller->isOrthoProjection();
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
