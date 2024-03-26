// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QMatrix4x4>
#include <Qt3DCore/private/qhandle_p.h>
#include <Qt3DCore/private/qresourcemanager_p.h>
#include <ctime>
#include <cstdlib>
#include <random>

class tst_QResourceManager : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void benchmarkAllocateSmallResources();
    void benchmarkAccessSmallResources();
    void benchmarkLookupSmallResources();
    void benchmarRandomAccessSmallResources();
    void benchmarkRandomLookupSmallResources();
    void benchmarkReleaseSmallResources();
    void benchmarkAllocateBigResources();
    void benchmarkAccessBigResources();
    void benchmarRandomAccessBigResources();
    void benchmarkLookupBigResources();
    void benchmarkRandomLookupBigResources();
    void benchmarkReleaseBigResources();
};

class tst_SmallArrayResource
{
public:
    tst_SmallArrayResource() : m_value(0)
    {}

    int m_value;
};

class tst_BigArrayResource
{
public:
    QMatrix4x4 m_matrix;
};

template<typename Resource>
void benchmarkAllocateResources()
{
    Qt3DCore::QResourceManager<Resource, int> manager;

    volatile Resource *c;
    QBENCHMARK_ONCE {
        const int max = (1 << 16) - 1;
        for (int i = 0; i < max; i++)
            c = manager.getOrCreateResource(i);
    }
    Q_UNUSED(c);
}

template<typename Resource>
void benchmarkAccessResources()
{
    Qt3DCore::QResourceManager<Resource, int> manager;
    const size_t max = (1 << 16) - 1;
    std::vector<Qt3DCore::QHandle<Resource> > handles(max);
    for (size_t i = 0; i < max; i++)
        handles[i] = manager.acquire();

    volatile Resource *c;
    QBENCHMARK {
        for (size_t i = 0; i < max; i++)
            c = manager.data(handles[i]);
    }
    Q_UNUSED(c);
}

template<typename Resource>
void benchmarkRandomAccessResource() {
    Qt3DCore::QResourceManager<Resource, int> manager;
    const size_t max = (1 << 16) - 1;
    std::vector<Qt3DCore::QHandle<Resource> > handles(max);
    for (size_t i = 0; i < max; i++)
        handles[i] = manager.acquire();

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(handles.begin(), handles.end(), g);

    volatile Resource *c;
    QBENCHMARK {
        for (size_t i = 0; i < max; i++)
            c = manager.data(handles[i]);
    }
    Q_UNUSED(c);
}

template<typename Resource>
void benchmarkLookupResources()
{
    Qt3DCore::QResourceManager<Resource, int> manager;
    const int max = (1 << 16) - 1;
    for (int i = 0; i < max; i++)
        manager.getOrCreateResource(i);

    volatile Resource *c;
    QBENCHMARK {
        for (int i = 0; i < max; i++)
            c = manager.lookupResource(i);
    }
    Q_UNUSED(c);
}

template<typename Resource>
void benchmarkRandomLookupResources()
{
    Qt3DCore::QResourceManager<Resource, int> manager;
    const int max = (1 << 16) - 1;
    QVector<int> resourcesIndices(max);
    for (int i = 0; i < max; i++) {
        manager.getOrCreateResource(i);
        resourcesIndices[i] = i;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(resourcesIndices.begin(), resourcesIndices.end(), g);

    volatile Resource *c;
    QBENCHMARK {
        for (int i = 0; i < max; i++)
            c = manager.lookupResource(resourcesIndices[i]);
    }
    Q_UNUSED(c);
}

template<typename Resource>
void benchmarkReleaseResources()
{
    Qt3DCore::QResourceManager<Resource, int> manager;
    const size_t max = (1 << 16) - 1;
    std::vector<Qt3DCore::QHandle<Resource> > handles(max);
    for (size_t i = 0; i < max; i++)
        handles[i] = manager.acquire();
    for (size_t i = 0; i < max; i++)
        manager.release(handles.at(i));
    handles.clear();

    QBENCHMARK_ONCE {
        // the release/clear should have left many unused handled in the resourcemanager,
        // so the next acquire will trigger a collection of all freed resources
        manager.acquire();
    }
}

void tst_QResourceManager::benchmarkAllocateSmallResources()
{
    benchmarkAllocateResources<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarkAccessSmallResources()
{
    benchmarkAccessResources<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarkLookupSmallResources()
{
    benchmarkLookupResources<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarRandomAccessSmallResources()
{
    benchmarkRandomAccessResource<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarkRandomLookupSmallResources()
{
    benchmarkRandomLookupResources<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarkReleaseSmallResources()
{
    benchmarkReleaseResources<tst_SmallArrayResource>();
}

void tst_QResourceManager::benchmarkAllocateBigResources()
{
    benchmarkAllocateResources<tst_BigArrayResource>();
}

void tst_QResourceManager::benchmarkAccessBigResources()
{
    benchmarkAccessResources<tst_BigArrayResource>();
}

void tst_QResourceManager::benchmarRandomAccessBigResources()
{
    benchmarkRandomAccessResource<tst_BigArrayResource>();
}

void tst_QResourceManager::benchmarkLookupBigResources()
{
    benchmarkLookupResources<tst_BigArrayResource>();
}

void tst_QResourceManager::benchmarkRandomLookupBigResources()
{
    benchmarkRandomLookupResources<tst_BigArrayResource>();
}

void tst_QResourceManager::benchmarkReleaseBigResources()
{
    benchmarkReleaseResources<tst_BigArrayResource>();
}

QTEST_APPLESS_MAIN(tst_QResourceManager)

#include "tst_bench_qresourcesmanager.moc"
