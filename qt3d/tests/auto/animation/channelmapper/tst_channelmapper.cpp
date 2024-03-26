// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include <Qt3DAnimation/private/handler_p.h>
#include <Qt3DAnimation/private/channelmapper_p.h>
#include <Qt3DAnimation/private/channelmapping_p.h>
#include <Qt3DAnimation/private/managers_p.h>
#include <Qt3DAnimation/qchannelmapper.h>
#include <Qt3DAnimation/qchannelmapping.h>
#include <Qt3DAnimation/private/qchannelmapper_p.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/private/qbackendnode_p.h>
#include "testarbiter.h"

class tst_ChannelMapper : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:
    void checkPeerPropertyMirroring()
    {
        // GIVEN
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ChannelMapper backendMapper;
        backendMapper.setHandler(&handler);
        Qt3DAnimation::QChannelMapper mapper;

        mapper.addMapping(new Qt3DAnimation::QChannelMapping);
        mapper.addMapping(new Qt3DAnimation::QChannelMapping);

        // WHEN
        simulateInitializationSync(&mapper, &backendMapper);

        // THEN
        QCOMPARE(backendMapper.peerId(), mapper.id());
        QCOMPARE(backendMapper.isEnabled(), mapper.isEnabled());

        const int mappingCount = backendMapper.mappingIds().size();
        if (mappingCount > 0) {
            for (int i = 0; i < mappingCount; ++i)
                QCOMPARE(backendMapper.mappingIds().at(i), mapper.mappings().at(i)->id());
        }
    }

    void checkInitialAndCleanedUpState()
    {
        // GIVEN
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ChannelMapper backendMapper;
        backendMapper.setHandler(&handler);

        // THEN
        QVERIFY(backendMapper.peerId().isNull());
        QCOMPARE(backendMapper.isEnabled(), false);
        QCOMPARE(backendMapper.mappingIds(), QList<Qt3DCore::QNodeId>());

        // GIVEN
        Qt3DAnimation::QChannelMapper mapper;
        mapper.addMapping(new Qt3DAnimation::QChannelMapping());

        // WHEN
        simulateInitializationSync(&mapper, &backendMapper);
        backendMapper.cleanup();

        // THEN
        QCOMPARE(backendMapper.isEnabled(), false);
        QCOMPARE(backendMapper.mappingIds(), QList<Qt3DCore::QNodeId>());
    }

    void checkPropertyChanges()
    {
        // GIVEN
        Qt3DAnimation::QChannelMapper mapper;
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ChannelMapper backendMapper;
        backendMapper.setHandler(&handler);
        simulateInitializationSync(&mapper, &backendMapper);

        // WHEN
        mapper.setEnabled(false);
        backendMapper.syncFromFrontEnd(&mapper, false);

        // THEN
        QCOMPARE(backendMapper.isEnabled(), false);

        // WHEN
        Qt3DAnimation::QChannelMapping mapping;
        const Qt3DCore::QNodeId mappingId = mapping.id();
        Qt3DAnimation::Animation::ChannelMapping *backendMapping
                = handler.channelMappingManager()->getOrCreateResource(mappingId);
        backendMapping->setHandler(&handler);
        simulateInitializationSync(&mapping, backendMapping);

        mapper.addMapping(&mapping);
        backendMapper.syncFromFrontEnd(&mapper, false);

        // THEN
        QCOMPARE(backendMapper.mappingIds().size(), 1);
        QCOMPARE(backendMapper.mappingIds().first(), mappingId);
        QCOMPARE(backendMapper.mappings().size(), 1);
        QCOMPARE(backendMapper.mappings().first(), backendMapping);

        // WHEN
        Qt3DAnimation::QChannelMapping mapping2;
        const Qt3DCore::QNodeId mappingId2 = mapping2.id();
        Qt3DAnimation::Animation::ChannelMapping *backendMapping2
                = handler.channelMappingManager()->getOrCreateResource(mappingId2);
        backendMapping2->setHandler(&handler);
        simulateInitializationSync(&mapping2, backendMapping2);

        mapper.addMapping(&mapping2);
        backendMapper.syncFromFrontEnd(&mapper, false);

        // THEN
        QCOMPARE(backendMapper.mappingIds().size(), 2);
        QCOMPARE(backendMapper.mappingIds().first(), mappingId);
        QCOMPARE(backendMapper.mappingIds().last(), mappingId2);
        QCOMPARE(backendMapper.mappings().size(), 2);
        QCOMPARE(backendMapper.mappings().first(), backendMapping);
        QCOMPARE(backendMapper.mappings().last(), backendMapping2);

        // WHEN
        mapper.removeMapping(&mapping);
        backendMapper.syncFromFrontEnd(&mapper, false);

        // THEN
        QCOMPARE(backendMapper.mappingIds().size(), 1);
        QCOMPARE(backendMapper.mappingIds().first(), mappingId2);
        QCOMPARE(backendMapper.mappings().size(), 1);
        QCOMPARE(backendMapper.mappings().first(), backendMapping2);
    }
};

QTEST_MAIN(tst_ChannelMapper)

#include "tst_channelmapper.moc"
