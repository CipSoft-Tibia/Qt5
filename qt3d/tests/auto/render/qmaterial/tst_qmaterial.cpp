// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DRender/private/qrenderstate_p.h>

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QNormalDiffuseMapMaterial>
#include <Qt3DExtras/QDiffuseSpecularMapMaterial>
#include <Qt3DExtras/QNormalDiffuseMapAlphaMaterial>
#include <Qt3DExtras/QNormalDiffuseSpecularMapMaterial>

#include <Qt3DRender/private/qmaterial_p.h>

#include "testarbiter.h"

class TestMaterial : public Qt3DRender::QMaterial
{
public:
    explicit TestMaterial(Qt3DCore::QNode *parent = 0)
        : Qt3DRender::QMaterial(parent)
        , m_effect(new Qt3DRender::QEffect(this))
        , m_technique(new Qt3DRender::QTechnique(m_effect))
        , m_renderPass(new Qt3DRender::QRenderPass(m_technique))
        , m_shaderProgram(new Qt3DRender::QShaderProgram(m_renderPass))
    {
        m_renderPass->setShaderProgram(m_shaderProgram);
        m_technique->addRenderPass(m_renderPass);
        m_effect->addTechnique(m_technique);
        setEffect(m_effect);
    }

    Qt3DRender::QEffect *m_effect;
    Qt3DRender::QTechnique *m_technique;
    Qt3DRender::QRenderPass *m_renderPass;
    Qt3DRender::QShaderProgram *m_shaderProgram;
};

class tst_QMaterial : public QObject
{
    Q_OBJECT
public:
    tst_QMaterial()
        : QObject()
    {
        qRegisterMetaType<Qt3DCore::QNode*>();
        qRegisterMetaType<Qt3DRender::QEffect*>("Qt3DRender::QEffect*");
    }

private:

    void compareEffects(const Qt3DRender::QEffect *original, const Qt3DRender::QEffect *clone)
    {
        bool isEffectNull = (original == nullptr);
        if (isEffectNull) {
            QVERIFY(!clone);
        } else {
            QVERIFY(clone);
            QCOMPARE(original->id(), clone->id());

            compareParameters(original->parameters(), clone->parameters());

            const int techniqueCounts = original->techniques().size();
            QCOMPARE(techniqueCounts, clone->techniques().size());

            for (int i = 0; i < techniqueCounts; ++i)
                compareTechniques(original->techniques().at(i), clone->techniques().at(i));
        }
    }

    void compareTechniques(const Qt3DRender::QTechnique *original, const Qt3DRender::QTechnique *clone)
    {
        QCOMPARE(original->id(), clone->id());

        compareParameters(original->parameters(), clone->parameters());

        const int passesCount = original->renderPasses().size();
        QCOMPARE(passesCount, clone->renderPasses().size());

        for (int i = 0; i < passesCount; ++i)
            compareRenderPasses(original->renderPasses().at(i), clone->renderPasses().at(i));
    }

    void compareRenderPasses(const Qt3DRender::QRenderPass *original, const Qt3DRender::QRenderPass *clone)
    {
        QCOMPARE(original->id(), clone->id());

        compareParameters(original->parameters(), clone->parameters());
        compareRenderStates(original->renderStates(), clone->renderStates());
        compareFilterKeys(original->filterKeys(), clone->filterKeys());
        compareShaderPrograms(original->shaderProgram(), clone->shaderProgram());
    }

    void compareParameters(const Qt3DRender::ParameterList &original, const Qt3DRender::ParameterList &clone)
    {
        QCOMPARE(original.size(), clone.size());
        const int parametersCount = original.size();
        for (int i = 0; i < parametersCount; ++i) {
            const Qt3DRender::QParameter *originParam = original.at(i);
            const Qt3DRender::QParameter *cloneParam = clone.at(i);
            QCOMPARE(originParam->id(), cloneParam->id());
            QCOMPARE(cloneParam->name(), originParam->name());
            QCOMPARE(cloneParam->value(), originParam->value());
        }
    }

    void compareFilterKeys(const QList<Qt3DRender::QFilterKey *> &original, const QList<Qt3DRender::QFilterKey *> &clone)
    {
        const int annotationsCount = original.size();
        QCOMPARE(annotationsCount, clone.size());

        for (int i = 0; i < annotationsCount; ++i) {
            const Qt3DRender::QFilterKey *origAnnotation = original.at(i);
            const Qt3DRender::QFilterKey *cloneAnnotation = clone.at(i);
            QCOMPARE(origAnnotation->id(), cloneAnnotation->id());
            QCOMPARE(origAnnotation->name(), cloneAnnotation->name());
            QCOMPARE(origAnnotation->value(), cloneAnnotation->value());
        }
    }

    void compareRenderStates(const QList<Qt3DRender::QRenderState *> &original, const QList<Qt3DRender::QRenderState *> &clone)
    {
        const int renderStatesCount = original.size();
        QCOMPARE(renderStatesCount, clone.size());

        for (int i = 0; i < renderStatesCount; ++i) {
            Qt3DRender::QRenderState *originState = original.at(i);
            Qt3DRender::QRenderState *cloneState = clone.at(i);
            QCOMPARE(originState->id(), originState->id());
            QVERIFY(Qt3DRender::QRenderStatePrivate::get(originState)->m_type == Qt3DRender::QRenderStatePrivate::get(cloneState)->m_type);
        }
    }

    void compareShaderPrograms(const Qt3DRender::QShaderProgram *original, const Qt3DRender::QShaderProgram *clone)
    {
        bool isOriginalNull = (original == nullptr);
        if (isOriginalNull) {
            QVERIFY(!clone);
        } else {
            QVERIFY(clone);
            QCOMPARE(original->id(), clone->id());
            QVERIFY(original->vertexShaderCode() == clone->vertexShaderCode());
            QVERIFY(original->fragmentShaderCode() == clone->fragmentShaderCode());
            QVERIFY(original->geometryShaderCode() == clone->geometryShaderCode());
            QVERIFY(original->computeShaderCode() == clone->computeShaderCode());
            QVERIFY(original->tessellationControlShaderCode() == clone->tessellationControlShaderCode());
            QVERIFY(original->tessellationEvaluationShaderCode() == clone->tessellationEvaluationShaderCode());
        }
    }

private Q_SLOTS:

    void checkEffectUpdate()
    {
        // GIVEN
        TestArbiter arbiter;
        QScopedPointer<Qt3DRender::QMaterial> material(new Qt3DRender::QMaterial());
        arbiter.setArbiterOnNode(material.data());

        // WHEN
        Qt3DRender::QEffect effect;
        material->setEffect(&effect);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material.data());

        arbiter.clear();

        // GIVEN
        TestArbiter arbiter2;
        QScopedPointer<TestMaterial> material2(new TestMaterial());
        arbiter2.setArbiterOnNode(material2.data());

        // WHEN
        material2->setEffect(&effect);

        // THEN
        QCOMPARE(arbiter2.dirtyNodes().size(), 1);
        QCOMPARE(arbiter2.dirtyNodes().front(), material2.data());

        arbiter2.clear();
    }

    void checkDynamicParametersAddedUpdates()
    {
        // GIVEN
        TestArbiter arbiter;
        TestMaterial *material = new TestMaterial();
        arbiter.setArbiterOnNode(material);

        QCoreApplication::processEvents();
        // Clear events trigger by child generation of TestMnterial
        arbiter.clear();

        // WHEN (add parameter to material)
        Qt3DRender::QParameter *param = new Qt3DRender::QParameter("testParamMaterial", QVariant::fromValue(383.0f));
        material->addParameter(param);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(param->parent(), material);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QVERIFY(material->parameters().contains(param));

        // WHEN (add parameter to effect)
        param = new Qt3DRender::QParameter("testParamEffect", QVariant::fromValue(383.0f));
        material->effect()->addParameter(param);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 2);
        QVERIFY(material->effect()->parameters().contains(param));

        // WHEN (add parameter to technique)
        param = new Qt3DRender::QParameter("testParamTechnique", QVariant::fromValue(383.0f));
        material->m_technique->addParameter(param);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 3);
        QVERIFY(material->m_technique->parameters().contains(param));


        // WHEN (add parameter to renderpass)
        param = new Qt3DRender::QParameter("testParamRenderPass", QVariant::fromValue(383.0f));
        material->m_renderPass->addParameter(param);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 4);
        QVERIFY(material->m_renderPass->parameters().contains(param));
    }

    void checkShaderProgramUpdates()
    {
        // GIVEN
        TestArbiter arbiter;
        TestMaterial *material = new TestMaterial();
        arbiter.setArbiterOnNode(material);

        // WHEN
        const QByteArray vertexCode = QByteArrayLiteral("new vertex shader code");
        material->m_shaderProgram->setVertexShaderCode(vertexCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();

        // WHEN
        const QByteArray fragmentCode = QByteArrayLiteral("new fragment shader code");
        material->m_shaderProgram->setFragmentShaderCode(fragmentCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();
        // WHEN
        const QByteArray geometryCode = QByteArrayLiteral("new geometry shader code");
        material->m_shaderProgram->setGeometryShaderCode(geometryCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();

        // WHEN
        const QByteArray computeCode = QByteArrayLiteral("new compute shader code");
        material->m_shaderProgram->setComputeShaderCode(computeCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();

        // WHEN
        const QByteArray tesselControlCode = QByteArrayLiteral("new tessellation control shader code");
        material->m_shaderProgram->setTessellationControlShaderCode(tesselControlCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();

        // WHEN
        const QByteArray tesselEvalCode = QByteArrayLiteral("new tessellation eval shader code");
        material->m_shaderProgram->setTessellationEvaluationShaderCode(tesselEvalCode);

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), material->m_shaderProgram);

        arbiter.clear();
    }

    void checkEffectBookkeeping()
    {
        // GIVEN
        QScopedPointer<Qt3DRender::QMaterial> material(new Qt3DRender::QMaterial);
        {
            // WHEN
            Qt3DRender::QEffect effect;
            material->setEffect(&effect);

            // THEN
            QCOMPARE(effect.parent(), material.data());
            QCOMPARE(material->effect(), &effect);
        }
        // THEN (Should not crash and effect be unset)
        QVERIFY(material->effect() == nullptr);

        {
            // WHEN
            Qt3DRender::QMaterial someOtherMaterial;
            QScopedPointer<Qt3DRender::QEffect> effect(new Qt3DRender::QEffect(&someOtherMaterial));
            material->setEffect(effect.data());

            // THEN
            QCOMPARE(effect->parent(), &someOtherMaterial);
            QCOMPARE(material->effect(), effect.data());

            // WHEN
            material.reset();
            effect.reset();

            // THEN Should not crash when the effect is destroyed (tests for failed removal of destruction helper)
        }
    }

    void checkParametersBookkeeping()
    {
        // GIVEN
        QScopedPointer<Qt3DRender::QMaterial> material(new Qt3DRender::QMaterial);
        {
            // WHEN
            Qt3DRender::QParameter param;
            material->addParameter(&param);

            // THEN
            QCOMPARE(param.parent(), material.data());
            QCOMPARE(material->parameters().size(), 1);
        }
        // THEN (Should not crash and parameter be unset)
        QVERIFY(material->parameters().empty());

        {
            // WHEN
            Qt3DRender::QMaterial someOtherMaterial;
            QScopedPointer<Qt3DRender::QParameter> param(new Qt3DRender::QParameter(&someOtherMaterial));
            material->addParameter(param.data());

            // THEN
            QCOMPARE(param->parent(), &someOtherMaterial);
            QCOMPARE(material->parameters().size(), 1);

            // WHEN
            material.reset();
            param.reset();

            // THEN Should not crash when the parameter is destroyed (tests for failed removal of destruction helper)
        }
    }
};

QTEST_MAIN(tst_QMaterial)

#include "tst_qmaterial.moc"
