/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgmaterial.h"
#include "qsgrenderer_p.h"
#include "qsgmaterialshader_p.h"
#if QT_CONFIG(opengl)
# include <private/qsgshadersourcebuilder_p.h>
# include <private/qsgdefaultcontext_p.h>
# include <private/qsgdefaultrendercontext_p.h>
# include <QtGui/QOpenGLFunctions>
# include <QtGui/QOpenGLContext>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(opengl)
const char *QSGMaterialShaderPrivate::loadShaderSource(QOpenGLShader::ShaderType type) const
{
    const QStringList files = m_sourceFiles[type];
    QSGShaderSourceBuilder builder;
    for (const QString &file : files)
        builder.appendSourceFile(file);
    m_sources[type] = builder.source();
    return m_sources[type].constData();
}
#endif

/*!
    \class QSGMaterialShader
    \brief The QSGMaterialShader class represents an OpenGL shader program
    in the renderer.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    The QSGMaterialShader API is relatively low-level. A more convenient API,
    which provides almost all the same features, is available through
    QSGSimpleMaterialShader.

    \warning This class is only functional when running with the legacy OpenGL
    renderer of the Qt Quick scenegraph.

    The QSGMaterial and QSGMaterialShader form a tight relationship. For one
    scene graph (including nested graphs), there is one unique QSGMaterialShader
    instance which encapsulates the QOpenGLShaderProgram the scene graph uses
    to render that material, such as a shader to flat coloring of geometry.
    Each QSGGeometryNode can have a unique QSGMaterial containing the
    how the shader should be configured when drawing that node, such as
    the actual color used to render the geometry.

    An instance of QSGMaterialShader is never created explicitly by the user,
    it will be created on demand by the scene graph through
    QSGMaterial::createShader(). The scene graph will make sure that there
    is only one instance of each shader implementation through a scene graph.

    The source code returned from vertexShader() is used to control what the
    material does with the vertiex data that comes in from the geometry.
    The source code returned from the fragmentShader() is used to control
    what how the material should fill each individual pixel in the geometry.
    The vertex and fragment source code is queried once during initialization,
    changing what is returned from these functions later will not have
    any effect.

    The activate() function is called by the scene graph when a shader is
    is starting to be used. The deactivate function is called by the scene
    graph when the shader is no longer going to be used. While active,
    the scene graph may make one or more calls to updateState() which
    will update the state of the shader for each individual geometry to
    render.

    The attributeNames() returns the name of the attributes used in the
    vertexShader(). These are used in the default implementation of
    activate() and deactivate() to decide whice vertex registers are enabled.

    The initialize() function is called during program creation to allow
    subclasses to prepare for use, such as resolve uniform names in the
    vertexShader() and fragmentShader().

    A minimal example:
    \code
        class Shader : public QSGMaterialShader
        {
        public:
            const char *vertexShader() const {
                return
                "attribute highp vec4 vertex;          \n"
                "uniform highp mat4 matrix;            \n"
                "void main() {                         \n"
                "    gl_Position = matrix * vertex;    \n"
                "}";
            }

            const char *fragmentShader() const {
                return
                "uniform lowp float opacity;                            \n"
                "void main() {                                          \n"
                        "    gl_FragColor = vec4(1, 0, 0, 1) * opacity; \n"
                "}";
            }

            char const *const *attributeNames() const
            {
                static char const *const names[] = { "vertex", 0 };
                return names;
            }

            void initialize()
            {
                QSGMaterialShader::initialize();
                m_id_matrix = program()->uniformLocation("matrix");
                m_id_opacity = program()->uniformLocation("opacity");
            }

            void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
            {
                Q_ASSERT(program()->isLinked());
                if (state.isMatrixDirty())
                    program()->setUniformValue(m_id_matrix, state.combinedMatrix());
                if (state.isOpacityDirty())
                    program()->setUniformValue(m_id_opacity, state.opacity());
            }

        private:
            int m_id_matrix;
            int m_id_opacity;
        };
    \endcode

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

 */



/*!
    Creates a new QSGMaterialShader.
 */
QSGMaterialShader::QSGMaterialShader()
    : d_ptr(new QSGMaterialShaderPrivate)
{
}

/*!
    \internal
 */
QSGMaterialShader::QSGMaterialShader(QSGMaterialShaderPrivate &dd)
    : d_ptr(&dd)
{
}

/*!
    \internal
 */
QSGMaterialShader::~QSGMaterialShader()
{
}

/*!
    \fn char const *const *QSGMaterialShader::attributeNames() const

    Returns a zero-terminated array describing the names of the
    attributes used in the vertex shader.

    This function is called when the shader is compiled to specify
    which attributes exist. The order of the attribute names
    defines the attribute register position in the vertex shader.
 */

#if QT_CONFIG(opengl)
/*!
    \fn const char *QSGMaterialShader::vertexShader() const

    Called when the shader is being initialized to get the vertex
    shader source code.

    The contents returned from this function should never change.
*/
const char *QSGMaterialShader::vertexShader() const
{
    Q_D(const QSGMaterialShader);
    return d->loadShaderSource(QOpenGLShader::Vertex);
}


/*!
   \fn const char *QSGMaterialShader::fragmentShader() const

    Called when the shader is being initialized to get the fragment
    shader source code.

    The contents returned from this function should never change.
*/
const char *QSGMaterialShader::fragmentShader() const
{
    Q_D(const QSGMaterialShader);
    return d->loadShaderSource(QOpenGLShader::Fragment);
}


/*!
    \fn QOpenGLShaderProgram *QSGMaterialShader::program()

    Returns the shader program used by this QSGMaterialShader.
 */
#endif

/*!
    \fn void QSGMaterialShader::initialize()

    Reimplement this function to do one-time initialization when the
    shader program is compiled. The OpenGL shader program is compiled
    and linked, but not bound, when this function is called.
 */


/*!
    This function is called by the scene graph to indicate that geometry is
    about to be rendered using this shader.

    State that is global for all uses of the shader, independent of the geometry
    that is being drawn, can be setup in this function.
 */

void QSGMaterialShader::activate()
{
}



/*!
    This function is called by the scene graph to indicate that geometry will
    no longer to be rendered using this shader.
 */

void QSGMaterialShader::deactivate()
{
}



/*!
    This function is called by the scene graph before geometry is rendered
    to make sure the shader is in the right state.

    The current rendering \a state is passed from the scene graph. If the state
    indicates that any state is dirty, the updateState implementation must
    update accordingly for the geometry to render correctly.

    The subclass specific state, such as the color of a flat color material, should
    be extracted from \a newMaterial to update the color uniforms accordingly.

    The \a oldMaterial can be used to minimze state changes when updating
    material states. The \a oldMaterial is 0 if this shader was just activated.

    \sa activate(), deactivate()
 */

void QSGMaterialShader::updateState(const RenderState & /* state */, QSGMaterial * /* newMaterial */, QSGMaterial * /* oldMaterial */)
{
}

#if QT_CONFIG(opengl)
/*!
    Sets the GLSL source file for the shader stage \a type to \a sourceFile. The
    default implementation of the vertexShader() and fragmentShader() functions
    will load the source files set by this function.

    This function is useful when you have a single source file for a given shader
    stage. If your shader consists of multiple source files then use
    setShaderSourceFiles()

    \sa setShaderSourceFiles(), vertexShader(), fragmentShader()
 */
void QSGMaterialShader::setShaderSourceFile(QOpenGLShader::ShaderType type, const QString &sourceFile)
{
    Q_D(QSGMaterialShader);
    d->m_sourceFiles[type] = (QStringList() << sourceFile);
}

/*!
    Sets the GLSL source files for the shader stage \a type to \a sourceFiles. The
    default implementation of the vertexShader() and fragmentShader() functions
    will load the source files set by this function in the order given.

    \sa setShaderSourceFile(), vertexShader(), fragmentShader()
 */
void QSGMaterialShader::setShaderSourceFiles(QOpenGLShader::ShaderType type, const QStringList &sourceFiles)
{
    Q_D(QSGMaterialShader);
    d->m_sourceFiles[type] = sourceFiles;
}

/*!
    This function is called when the shader is initialized to compile the
    actual QOpenGLShaderProgram. Do not call it explicitly.

    The default implementation will extract the vertexShader() and
    fragmentShader() and bind the names returned from attributeNames()
    to consecutive vertex attribute registers starting at 0.
 */

void QSGMaterialShader::compile()
{
    Q_ASSERT_X(!m_program.isLinked(), "QSGSMaterialShader::compile()", "Compile called multiple times!");

    program()->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader());
    program()->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader());

    char const *const *attr = attributeNames();
#ifndef QT_NO_DEBUG
    int maxVertexAttribs = 0;
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    funcs->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    for (int i = 0; attr[i]; ++i) {
        if (i >= maxVertexAttribs) {
            qFatal("List of attribute names is either too long or not null-terminated.\n"
                   "Maximum number of attributes on this hardware is %i.\n"
                   "Vertex shader:\n%s\n"
                   "Fragment shader:\n%s\n",
                   maxVertexAttribs, vertexShader(), fragmentShader());
        }
        if (*attr[i])
            program()->bindAttributeLocation(attr[i], i);
    }
#else
    for (int i = 0; attr[i]; ++i) {
        if (*attr[i])
            program()->bindAttributeLocation(attr[i], i);
    }
#endif

    if (!program()->link()) {
        qWarning("QSGMaterialShader: Shader compilation failed:");
        qWarning() << program()->log();
    }
}

#endif

/*!
    \class QSGMaterialShader::RenderState
    \brief The QSGMaterialShader::RenderState encapsulates the current rendering state
    during a call to QSGMaterialShader::updateState().
    \inmodule QtQuick

    The render state contains a number of accessors that the shader needs to respect
    in order to conform to the current state of the scene graph.

    The instance is only valid inside a call to QSGMaterialShader::updateState() and
    should not be used outisde this function.
 */



/*!
    \enum QSGMaterialShader::RenderState::DirtyState

    \value DirtyMatrix Used to indicate that the matrix has changed and must be updated.

    \value DirtyOpacity Used to indicate that the opacity has changed and must be updated.

    \value DirtyCachedMaterialData Used to indicate that the cached material data have changed and must be updated.

    \value DirtyAll Used to indicate that everything needs to be updated.
 */



/*!
    \fn bool QSGMaterialShader::RenderState::isMatrixDirty() const

    Returns \c true if the dirtyStates() contain the dirty matrix state,
    otherwise returns \c false.
 */



/*!
    \fn bool QSGMaterialShader::RenderState::isOpacityDirty() const

    Returns \c true if the dirtyStates() contains the dirty opacity state,
    otherwise returns \c false.
 */

/*!
  \fn bool QSGMaterialShader::RenderState::isCachedMaterialDataDirty() const

  Returns \c true if the dirtyStates() contains the dirty cached material state,
  otherwise returns \c false.
 */

/*!
    \fn QSGMaterialShader::RenderState::DirtyStates QSGMaterialShader::RenderState::dirtyStates() const

    Returns which rendering states that have changed and needs to be updated
    for geometry rendered with this material to conform to the current
    rendering state.
 */



/*!
    Returns the accumulated opacity to be used for rendering.
 */

float QSGMaterialShader::RenderState::opacity() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentOpacity();
}

/*!
    Returns the modelview determinant to be used for rendering.
 */

float QSGMaterialShader::RenderState::determinant() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->determinant();
}

/*!
    Returns the matrix combined of modelview matrix and project matrix.
 */

QMatrix4x4 QSGMaterialShader::RenderState::combinedMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentCombinedMatrix();
}
/*!
   Returns the ratio between physical pixels and device-independent pixels
   to be used for rendering.
*/
float QSGMaterialShader::RenderState::devicePixelRatio() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->devicePixelRatio();
}



/*!
    Returns the model view matrix.

    If the material has the RequiresFullMatrix flag
    set, this is guaranteed to be the complete transform
    matrix calculated from the scenegraph.

    However, if this flag is not set, the renderer may
    choose to alter this matrix. For example, it may
    pre-transform vertices on the CPU and set this matrix
    to identity.

    In a situation such as the above, it is still possible
    to retrieve the actual matrix determinant by setting
    the RequiresDeterminant flag in the material and
    calling the determinant() accessor.
 */

QMatrix4x4 QSGMaterialShader::RenderState::modelViewMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentModelViewMatrix();
}

/*!
    Returns the projection matrix.
 */

QMatrix4x4 QSGMaterialShader::RenderState::projectionMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentProjectionMatrix();
}



/*!
    Returns the viewport rect of the surface being rendered to.
 */

QRect QSGMaterialShader::RenderState::viewportRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->viewportRect();
}



/*!
    Returns the device rect of the surface being rendered to
 */

QRect QSGMaterialShader::RenderState::deviceRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->deviceRect();
}

#if QT_CONFIG(opengl)

/*!
    Returns the QOpenGLContext that is being used for rendering
 */

QOpenGLContext *QSGMaterialShader::RenderState::context() const
{
    // Only the QSGDefaultRenderContext will have an OpenGL Context to query
    auto openGLRenderContext = static_cast<const QSGDefaultRenderContext *>(static_cast<const QSGRenderer *>(m_data)->context());
    if (openGLRenderContext != nullptr)
        return openGLRenderContext->openglContext();
    else
        return nullptr;
}

#endif

QT_END_NAMESPACE
