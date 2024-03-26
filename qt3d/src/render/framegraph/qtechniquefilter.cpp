// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtechniquefilter.h"
#include "qtechniquefilter_p.h"
#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qparameter.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {

QTechniqueFilterPrivate::QTechniqueFilterPrivate()
    : QFrameGraphNodePrivate()
{
}

/*!
    \class Qt3DRender::QTechniqueFilter
    \inmodule Qt3DRender
    \since 5.7
    \brief A QFrameGraphNode used to select QTechniques to use.

    A Qt3DRender::QTechniqueFilter specifies which techniques are used
    by the FrameGraph when rendering the entities. QTechniqueFilter specifies
    a list of Qt3DRender::QFilterKey objects and Qt3DRender::QParameter objects.
    When QTechniqueFilter is present in the FrameGraph, only the techiques matching
    the keys in the list are used for rendering. The parameters in the list can be used
    to set values for shader parameters. The parameters in QTechniqueFilter
    override parameters in QMaterial, QEffect, QTechnique and QRenderPass, but are overridden
    by parameters in QRenderPassFilter.
*/

/*!
    \qmltype TechniqueFilter
    \inqmlmodule Qt3D.Render
    \instantiates Qt3DRender::QTechniqueFilter
    \inherits FrameGraphNode
    \since 5.7
    \brief A FrameGraphNode used to select used Techniques.

    A TechniqueFilter specifies which techniques are used by the FrameGraph
    when rendering the entities. TechniqueFilter specifies
    a list of FilterKey objects and Parameter objects.
    When TechniqueFilter is present in the FrameGraph, only the techiques matching
    the keys in list are used for rendering. The parameters in the list can be used
    to set values for shader parameters. The parameters in TechniqueFilter
    override parameters in Material, Effect, Technique and RenderPass, but are overridden
    by parameters in RenderPassFilter.
*/

/*!
    \qmlproperty list<FilterKey> TechniqueFilter::matchAll
    Holds the list of filterkeys used by the TechiqueFilter
*/

/*!
    \qmlproperty list<Parameter> TechniqueFilter::parameters
    Holds the list of parameters used by the TechiqueFilter
*/

/*!
    The constructor creates an instance with the specified \a parent.
 */
QTechniqueFilter::QTechniqueFilter(QNode *parent)
    : QFrameGraphNode(*new QTechniqueFilterPrivate, parent)
{
}

/*! \internal */
QTechniqueFilter::~QTechniqueFilter()
{
}

/*! \internal */
QTechniqueFilter::QTechniqueFilter(QTechniqueFilterPrivate &dd, QNode *parent)
    : QFrameGraphNode(dd, parent)
{
}

/*!
    Returns a vector of the current keys for the filter.
 */
QList<QFilterKey *> QTechniqueFilter::matchAll() const
{
    Q_D(const QTechniqueFilter);
    return d->m_matchList;
}

/*!
    Add the \a filterKey to the match vector.
 */
void QTechniqueFilter::addMatch(QFilterKey *filterKey)
{
    Q_ASSERT(filterKey);
    Q_D(QTechniqueFilter);
    if (!d->m_matchList.contains(filterKey)) {
        d->m_matchList.append(filterKey);

        // Ensures proper bookkeeping
        d->registerDestructionHelper(filterKey, &QTechniqueFilter::removeMatch, d->m_matchList);

        // We need to add it as a child of the current node if it has been declared inline
        // Or not previously added as a child of the current node so that
        // 1) The backend gets notified about it's creation
        // 2) When the current node is destroyed, it gets destroyed as well
        if (!filterKey->parent())
            filterKey->setParent(this);

        d->update();
    }
}

/*!
    Remove the \a filterKey from the match vector.
 */
void QTechniqueFilter::removeMatch(QFilterKey *filterKey)
{
    Q_ASSERT(filterKey);
    Q_D(QTechniqueFilter);
    if (!d->m_matchList.removeOne(filterKey))
        return;
    d->update();
    // Remove bookkeeping connection
    d->unregisterDestructionHelper(filterKey);
}

/*!
    Add \a parameter to the vector of parameters that will be passed to the graphics pipeline.
 */
void QTechniqueFilter::addParameter(QParameter *parameter)
{
    Q_ASSERT(parameter);
    Q_D(QTechniqueFilter);
    if (!d->m_parameters.contains(parameter)) {
        d->m_parameters.append(parameter);

        // Ensures proper bookkeeping
        d->registerDestructionHelper(parameter, &QTechniqueFilter::removeParameter, d->m_parameters);

        // We need to add it as a child of the current node if it has been declared inline
        // Or not previously added as a child of the current node so that
        // 1) The backend gets notified about it's creation
        // 2) When the current node is destroyed, the child parameters get destroyed as well
        if (!parameter->parent())
            parameter->setParent(this);

        d->update();
    }
}

/*!
    Remove \a parameter from the vector of parameters passed to the graphics pipeline.
 */
void QTechniqueFilter::removeParameter(QParameter *parameter)
{
    Q_ASSERT(parameter);
    Q_D(QTechniqueFilter);
    if (!d->m_parameters.removeOne(parameter))
        return;
    d->update();
    // Remove bookkeeping connection
    d->unregisterDestructionHelper(parameter);
}

/*!
    Returns the current vector of parameters.
 */
QList<QParameter *> QTechniqueFilter::parameters() const
{
    Q_D(const QTechniqueFilter);
    return d->m_parameters;
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qtechniquefilter.cpp"
