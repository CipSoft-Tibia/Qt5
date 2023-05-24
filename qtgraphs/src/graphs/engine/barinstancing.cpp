// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "barinstancing_p.h"

BarInstancing::BarInstancing()
{

}

BarInstancing::~BarInstancing()
{
    clearDataArray();
}

QByteArray BarInstancing::getInstanceBuffer(int *instanceCount)
{
    if (m_dirty) {
        m_instanceData.resize(0);
        int instanceNumber = 0;

        for (int i = 0; i < m_dataArray.size(); ++i) {
            auto item = m_dataArray.at(i);
            if (!item->selectedBar) {
                auto entry = calculateTableEntry(item->position, item->scale, item->eulerRotation,
                                                 item->color);
                m_instanceData.append(reinterpret_cast<char *>(&entry), sizeof(entry));
            } else {
                //Even selected bars need to be drawn in a very small scale.
                //If this is not done, the program can't find the selected bars in the graph and
                //detects the wrong bars as selected ones.
                auto entry = calculateTableEntry(item->position, QVector3D{.001f, .001f, .001f},
                                                 item->eulerRotation, QColor(Qt::white));
                m_instanceData.append(reinterpret_cast<char *>(&entry), sizeof(entry));
            }
            instanceNumber++;
        }
        m_instanceCount = instanceNumber;
        m_dirty = false;
    }

    if (instanceCount)
        *instanceCount = m_instanceCount;

    return m_instanceData;
}

bool BarInstancing::rangeGradient() const
{
    return m_rangeGradient;
}

void BarInstancing::setRangeGradient(bool newRangeGradient)
{
    m_rangeGradient = newRangeGradient;
}

void BarInstancing::clearDataArray()
{
    m_dataArray.clear();
    m_instanceData.clear();
}

void BarInstancing::markDataDirty()
{
    m_dirty = true;
    markDirty();
}

QList<BarItemHolder *> BarInstancing::dataArray() const
{
    return m_dataArray;
}

void BarInstancing::setDataArray(const QList<BarItemHolder *> &newDataArray)
{
    m_dataArray = newDataArray;
    markDataDirty();
}
