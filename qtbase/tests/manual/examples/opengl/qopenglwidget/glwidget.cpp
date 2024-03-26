// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "glwidget.h"
#include <QPainter>
#include <QPaintEngine>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <qmath.h>

#include "mainwindow.h"
#include "bubble.h"

const int bubbleNum = 8;

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif

GLWidget::GLWidget(MainWindow *maybeMainWindow, const QColor &background)
    : m_mainWindow(maybeMainWindow),
      m_background(background)
{
    setMinimumSize(300, 250);
    if (QCoreApplication::arguments().contains(QStringLiteral("--srgb")))
        setTextureFormat(GL_SRGB8_ALPHA8);
}

GLWidget::~GLWidget()
{
    reset();
}

void GLWidget::reset()
{
    qDeleteAll(m_bubbles);
    // Leave everything in a state suitable for a subsequent call to
    // initialize(). This matters when coming from the context's
    // aboutToBeDestroyed signal, would not matter when invoked from the
    // destructor.
    m_bubbles.clear();

    // And now release all OpenGL resources.
    makeCurrent();
    delete m_texture;
    m_texture = nullptr;
    delete m_program1;
    m_program1 = nullptr;
    delete m_program2;
    m_program2 = nullptr;
    delete m_vshader1;
    m_vshader1 = nullptr;
    delete m_fshader1;
    m_fshader1 = nullptr;
    delete m_vshader2;
    m_vshader2 = nullptr;
    delete m_fshader2;
    m_fshader2 = nullptr;
    m_vbo1.destroy();
    m_vbo2.destroy();
    doneCurrent();

    // We are done with the current QOpenGLContext, forget it. If there is a
    // subsequent initialize(), that will then connect to the new context.
    QObject::disconnect(m_contextWatchConnection);
}

void GLWidget::setScaling(int scale)
{
    if (scale > 30)
        m_fScale = 1 + qreal(scale - 30) / 30 * 0.25;
    else if (scale < 30)
        m_fScale =  1 - (qreal(30 - scale) / 30 * 0.25);
    else
        m_fScale = 1;
}

void GLWidget::setLogo()
{
    m_qtLogo = true;
}

void GLWidget::setTexture()
{
    m_qtLogo = false;
}

void GLWidget::setShowBubbles(bool bubbles)
{
    m_showBubbles = bubbles;
}

void GLWidget::paintQtLogo()
{
    m_program1->enableAttributeArray(m_vertexAttr1);
    m_program1->enableAttributeArray(m_normalAttr1);

    m_vbo1.bind();
    // The data in the buffer is placed like this:
    // vertex1.x, vertex1.y, vertex1.z, normal1.x, normal1.y, normal1.z, vertex2.x, ...
    m_program1->setAttributeBuffer(m_vertexAttr1, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));
    m_program1->setAttributeBuffer(m_normalAttr1, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));
    m_vbo1.release();

    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

    m_program1->disableAttributeArray(m_normalAttr1);
    m_program1->disableAttributeArray(m_vertexAttr1);
}

void GLWidget::paintTexturedCube()
{
    m_texture->bind();

    if (!m_vbo2.isCreated()) {
        static GLfloat afVertices[] = {
            -0.5, 0.5, 0.5, 0.5,-0.5,0.5,-0.5,-0.5,0.5,
            0.5, -0.5, 0.5, -0.5,0.5,0.5,0.5,0.5,0.5,
            -0.5, -0.5, -0.5, 0.5,-0.5,-0.5,-0.5,0.5,-0.5,
            0.5, 0.5, -0.5, -0.5,0.5,-0.5,0.5,-0.5,-0.5,

            0.5, -0.5, -0.5, 0.5,-0.5,0.5,0.5,0.5,-0.5,
            0.5, 0.5, 0.5, 0.5,0.5,-0.5,0.5,-0.5,0.5,
            -0.5, 0.5, -0.5, -0.5,-0.5,0.5,-0.5,-0.5,-0.5,
            -0.5, -0.5, 0.5, -0.5,0.5,-0.5,-0.5,0.5,0.5,

            0.5, 0.5,  -0.5, -0.5, 0.5,  0.5,  -0.5,  0.5,  -0.5,
            -0.5,  0.5,  0.5,  0.5,  0.5,  -0.5, 0.5, 0.5,  0.5,
            -0.5,  -0.5, -0.5, -0.5, -0.5, 0.5,  0.5, -0.5, -0.5,
            0.5, -0.5, 0.5,  0.5,  -0.5, -0.5, -0.5,  -0.5, 0.5
        };

        static GLfloat afTexCoord[] = {
            0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
            1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,
            1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
            0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,

            1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
            0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,
            0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
            1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,

            0.0f,1.0f, 1.0f,0.0f, 1.0f,1.0f,
            1.0f,0.0f, 0.0f,1.0f, 0.0f,0.0f,
            1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f,
            0.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f
        };

        GLfloat afNormals[] = {

            0,0,-1, 0,0,-1, 0,0,-1,
            0,0,-1, 0,0,-1, 0,0,-1,
            0,0,1, 0,0,1, 0,0,1,
            0,0,1, 0,0,1, 0,0,1,

            -1,0,0, -1,0,0, -1,0,0,
            -1,0,0, -1,0,0, -1,0,0,
            1,0,0, 1,0,0, 1,0,0,
            1,0,0, 1,0,0, 1,0,0,

            0,-1,0, 0,-1,0, 0,-1,0,
            0,-1,0, 0,-1,0, 0,-1,0,
            0,1,0, 0,1,0, 0,1,0,
            0,1,0, 0,1,0, 0,1,0
        };

        m_vbo2.create();
        m_vbo2.bind();
        m_vbo2.allocate(36 * 8 * sizeof(GLfloat));
        m_vbo2.write(0, afVertices, sizeof(afVertices));
        m_vbo2.write(sizeof(afVertices), afTexCoord, sizeof(afTexCoord));
        m_vbo2.write(sizeof(afVertices) + sizeof(afTexCoord), afNormals, sizeof(afNormals));
        m_vbo2.release();
    }

    m_program2->setUniformValue(m_textureUniform2, 0); // use texture unit 0

    m_program2->enableAttributeArray(m_vertexAttr2);
    m_program2->enableAttributeArray(m_normalAttr2);
    m_program2->enableAttributeArray(m_texCoordAttr2);

    m_vbo2.bind();
    // In the buffer we first have 36 vertices (3 floats for each), then 36 texture
    // coordinates (2 floats for each), then 36 normals (3 floats for each).
    m_program2->setAttributeBuffer(m_vertexAttr2, GL_FLOAT, 0, 3);
    m_program2->setAttributeBuffer(m_texCoordAttr2, GL_FLOAT, 36 * 3 * sizeof(GLfloat), 2);
    m_program2->setAttributeBuffer(m_normalAttr2, GL_FLOAT, 36 * 5 * sizeof(GLfloat), 3);
    m_vbo2.release();

    glDrawArrays(GL_TRIANGLES, 0, 36);

    m_program2->disableAttributeArray(m_vertexAttr2);
    m_program2->disableAttributeArray(m_normalAttr2);
    m_program2->disableAttributeArray(m_texCoordAttr2);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_texture = new QOpenGLTexture(QImage(":/qt.png"));

    m_vshader1 = new QOpenGLShader(QOpenGLShader::Vertex);
    const char *vsrc1 =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec3 normal;\n"
        "uniform mediump mat4 matrix;\n"
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));\n"
        "    float angle = max(dot(normal, toLight), 0.0);\n"
        "    vec3 col = vec3(0.40, 1.0, 0.0);\n"
        "    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);\n"
        "    color = clamp(color, 0.0, 1.0);\n"
        "    gl_Position = matrix * vertex;\n"
        "}\n";
    m_vshader1->compileSourceCode(vsrc1);

    m_fshader1 = new QOpenGLShader(QOpenGLShader::Fragment);
    const char *fsrc1 =
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = color;\n"
        "}\n";
    m_fshader1->compileSourceCode(fsrc1);

    m_program1 = new QOpenGLShaderProgram;
    m_program1->addShader(m_vshader1);
    m_program1->addShader(m_fshader1);
    m_program1->link();

    m_vertexAttr1 = m_program1->attributeLocation("vertex");
    m_normalAttr1 = m_program1->attributeLocation("normal");
    m_matrixUniform1 = m_program1->uniformLocation("matrix");

    m_vshader2 = new QOpenGLShader(QOpenGLShader::Vertex);
    const char *vsrc2 =
        "attribute highp vec4 vertex;\n"
        "attribute highp vec4 texCoord;\n"
        "attribute mediump vec3 normal;\n"
        "uniform mediump mat4 matrix;\n"
        "varying highp vec4 texc;\n"
        "varying mediump float angle;\n"
        "void main(void)\n"
        "{\n"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));\n"
        "    angle = max(dot(normal, toLight), 0.0);\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    m_vshader2->compileSourceCode(vsrc2);

    m_fshader2 = new QOpenGLShader(QOpenGLShader::Fragment);
    const char *fsrc2 =
        "varying highp vec4 texc;\n"
        "uniform sampler2D tex;\n"
        "varying mediump float angle;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec3 color = texture2D(tex, texc.st).rgb;\n"
        "    color = color * 0.2 + color * 0.8 * angle;\n"
        "    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);\n"
        "}\n";
    m_fshader2->compileSourceCode(fsrc2);

    m_program2 = new QOpenGLShaderProgram;
    m_program2->addShader(m_vshader2);
    m_program2->addShader(m_fshader2);
    m_program2->link();

    m_vertexAttr2 = m_program2->attributeLocation("vertex");
    m_normalAttr2 = m_program2->attributeLocation("normal");
    m_texCoordAttr2 = m_program2->attributeLocation("texCoord");
    m_matrixUniform2 = m_program2->uniformLocation("matrix");
    m_textureUniform2 = m_program2->uniformLocation("tex");

    m_fAngle = 0;
    m_fScale = 1;

    createGeometry();

    // Use a vertex buffer object. Client-side pointers are old-school and should be avoided.
    m_vbo1.create();
    m_vbo1.bind();
    // For the cube all the data belonging to the texture coordinates and
    // normals is placed separately, after the vertices. Here, for the Qt logo,
    // let's do something different and potentially more efficient: create a
    // properly interleaved data set.
    const int vertexCount = m_vertices.count();
    QList<GLfloat> buf;
    buf.resize(vertexCount * 3 * 2);
    GLfloat *p = buf.data();
    for (int i = 0; i < vertexCount; ++i) {
        *p++ = m_vertices[i].x();
        *p++ = m_vertices[i].y();
        *p++ = m_vertices[i].z();
        *p++ = m_normals[i].x();
        *p++ = m_normals[i].y();
        *p++ = m_normals[i].z();
    }
    m_vbo1.allocate(buf.constData(), buf.count() * sizeof(GLfloat));
    m_vbo1.release();

    createBubbles(bubbleNum - m_bubbles.count());

    // A well-behaved QOpenGLWidget releases OpenGL resources not only upon
    // destruction, but also when the associated OpenGL context disappears. If
    // the widget continues to exist, the context's destruction will be
    // followed by a call to initialize(). This is not strictly mandatory in
    // widgets that never change their parents.
    m_contextWatchConnection = QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed, context(), [this] { reset(); });
}

void GLWidget::paintGL()
{
    createBubbles(bubbleNum - m_bubbles.count());

    QPainter painter;
    painter.begin(this);

    painter.beginNativePainting();

    glClearColor(m_background.redF(), m_background.greenF(), m_background.blueF(), m_transparent ? 0.0f : 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    QMatrix4x4 modelview;
    modelview.rotate(m_fAngle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(m_fAngle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(m_fAngle, 0.0f, 0.0f, 1.0f);
    modelview.scale(m_fScale);
    modelview.translate(0.0f, -0.2f, 0.0f);

    if (m_qtLogo) {
        m_program1->bind();
        m_program1->setUniformValue(m_matrixUniform1, modelview);
        paintQtLogo();
        m_program1->release();
    } else {
        m_program2->bind();
        m_program2->setUniformValue(m_matrixUniform2, modelview);
        paintTexturedCube();
        m_program2->release();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    painter.endNativePainting();

    if (m_showBubbles) {
        for (Bubble *bubble : std::as_const(m_bubbles))
            bubble->drawBubble(&painter);
    }

    if (const int elapsed = m_time.elapsed()) {
        QString framesPerSecond;
        framesPerSecond.setNum(m_frames /(elapsed / 1000.0), 'f', 2);
        painter.setPen(m_transparent ? Qt::black : Qt::white);
        painter.drawText(20, 40, framesPerSecond + " paintGL calls / s");
    }

    painter.end();

    for (Bubble *bubble : std::as_const(m_bubbles))
        bubble->move(rect());

    if (!(m_frames % 100)) {
        m_time.start();
        m_frames = 0;
    }
    m_fAngle += 1.0f;
    ++m_frames;

    // When requested, follow the ideal way to animate: Rely on
    // blocking swap and just schedule updates continuously.
    if (!m_mainWindow || !m_mainWindow->timerEnabled())
        update();
}

void GLWidget::createBubbles(int number)
{
    for (int i = 0; i < number; ++i) {
        QPointF position(width()*(0.1 + QRandomGenerator::global()->bounded(0.8)),
                         height()*(0.1 + QRandomGenerator::global()->bounded(0.8)));
        qreal radius = qMin(width(), height())*(0.0175 + QRandomGenerator::global()->bounded(0.0875));
        QPointF velocity(width()*0.0175*(-0.5 + QRandomGenerator::global()->bounded(1.0)),
                         height()*0.0175*(-0.5 + QRandomGenerator::global()->bounded(1.0)));

        m_bubbles.append(new Bubble(position, radius, velocity));
    }
}

void GLWidget::createGeometry()
{
    m_vertices.clear();
    m_normals.clear();

    qreal x1 = +0.06f;
    qreal y1 = -0.14f;
    qreal x2 = +0.14f;
    qreal y2 = -0.06f;
    qreal x3 = +0.08f;
    qreal y3 = +0.00f;
    qreal x4 = +0.30f;
    qreal y4 = +0.22f;

    quad(x1, y1, x2, y2, y2, x2, y1, x1);
    quad(x3, y3, x4, y4, y4, x4, y3, x3);

    extrude(x1, y1, x2, y2);
    extrude(x2, y2, y2, x2);
    extrude(y2, x2, y1, x1);
    extrude(y1, x1, x1, y1);
    extrude(x3, y3, x4, y4);
    extrude(x4, y4, y4, x4);
    extrude(y4, x4, y3, x3);

    const int NumSectors = 100;
    const qreal sectorAngle = 2 * qreal(M_PI) / NumSectors;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle = i * sectorAngle;
        qreal x5 = 0.30 * sin(angle);
        qreal y5 = 0.30 * cos(angle);
        qreal x6 = 0.20 * sin(angle);
        qreal y6 = 0.20 * cos(angle);

        angle += sectorAngle;
        qreal x7 = 0.20 * sin(angle);
        qreal y7 = 0.20 * cos(angle);
        qreal x8 = 0.30 * sin(angle);
        qreal y8 = 0.30 * cos(angle);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (int i = 0;i < m_vertices.size();i++)
        m_vertices[i] *= 2.0f;
}

void GLWidget::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);

    m_vertices << QVector3D(x3, y3, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x4 - x1, y4 - y1, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x1, y1, 0.05f);

    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x3, y3, 0.05f);

    n = QVector3D::normal
        (QVector3D(x2 - x4, y2 - y4, 0.0f), QVector3D(x1 - x4, y1 - y4, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}

void GLWidget::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    m_vertices << QVector3D(x1, y1, +0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);

    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(0.0f, 0.0f, -0.1f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}

void GLWidget::setTransparent(bool transparent)
{
    setAttribute(Qt::WA_AlwaysStackOnTop, transparent);
    m_transparent = transparent;
    // Call update() on the top-level window after toggling AlwayStackOnTop to make sure
    // the entire backingstore is updated accordingly.
    window()->update();
}

void GLWidget::resizeGL(int, int)
{
    if (m_mainWindow) {
        if (!m_btn) {
            m_btn = new QPushButton("\nAdd widget\n", this);
            connect(m_btn, &QPushButton::clicked, this, [this] { m_mainWindow->addNew(); });
        }
        m_btn->move(20, 80);
        if (!m_btn2) {
            m_btn2 = new QPushButton("\nI prefer tabbed widgets\n", this);
            connect(m_btn2, &QPushButton::clicked, this, [this] { m_mainWindow->showNewWindow(); });
        }
        m_btn2->move(20, 160);
    }
}
