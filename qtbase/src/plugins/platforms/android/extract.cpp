// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only



#include <QtCore/QJniEnvironment>

#include <alloca.h>
#include <android/log.h>
#include <extract.h>
#include <jni.h>
#include <stdlib.h>

#define LOG_TAG    "extractSyleInfo"
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// The following part was shamelessly stolen from ResourceTypes.cpp from Android's sources
/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

static void deserializeInternal(const void* inData, Res_png_9patch* outData) {
    char* patch = (char*) inData;
    if (inData != outData) {
        memmove(&outData->wasDeserialized, patch, 4);     // copy  wasDeserialized, numXDivs, numYDivs, numColors
        memmove(&outData->paddingLeft, patch + 12, 4);     // copy  wasDeserialized, numXDivs, numYDivs, numColors
    }
    outData->wasDeserialized = true;
    char* data = (char*)outData;
    data +=  sizeof(Res_png_9patch);
    outData->xDivs = (int32_t*) data;
    data +=  outData->numXDivs * sizeof(int32_t);
    outData->yDivs = (int32_t*) data;
    data +=  outData->numYDivs * sizeof(int32_t);
    outData->colors = (uint32_t*) data;
}

Res_png_9patch* Res_png_9patch::deserialize(const void* inData)
{
    if (sizeof(void*) != sizeof(int32_t)) {
        LOGE("Cannot deserialize on non 32-bit system\n");
        return NULL;
    }
    deserializeInternal(inData, (Res_png_9patch*) inData);
    return (Res_png_9patch*) inData;
}

extern "C" JNIEXPORT jintArray JNICALL
Java_org_qtproject_qt_android_ExtractStyle_extractNativeChunkInfo20(JNIEnv *env, jobject, long addr)
{
    Res_png_9patch20* chunk = reinterpret_cast<Res_png_9patch20*>(addr);
    Res_png_9patch20::deserialize(chunk);
    //printChunkInformation(chunk);
    jintArray result;
    size_t size = 3+chunk->numXDivs+chunk->numYDivs+chunk->numColors;
    result = env->NewIntArray(size);
    if (!result)
        return 0;

    jint *data = (jint*)malloc(sizeof(jint)*size);
    size_t pos = 0;
    data[pos++] = chunk->numXDivs;
    data[pos++] = chunk->numYDivs;
    data[pos++] = chunk->numColors;

    int32_t* xDivs = chunk->getXDivs();
    int32_t* yDivs = chunk->getYDivs();
    uint32_t* colors = chunk->getColors();

    for (int x = 0; x <chunk->numXDivs; x ++)
        data[pos++]=xDivs[x];
    for (int y = 0; y <chunk->numYDivs; y ++)
        data[pos++] = yDivs[y];
    for (int c = 0; c <chunk->numColors; c ++)
        data[pos++] = colors[c];
    env->SetIntArrayRegion(result, 0, size, data);
    free(data);
    return result;
}

static inline void fill9patchOffsets(Res_png_9patch20* patch) {
    patch->xDivsOffset = sizeof(Res_png_9patch20);
    patch->yDivsOffset = patch->xDivsOffset + (patch->numXDivs * sizeof(int32_t));
    patch->colorsOffset = patch->yDivsOffset + (patch->numYDivs * sizeof(int32_t));
}

Res_png_9patch20* Res_png_9patch20::deserialize(void* inData)
{
    Res_png_9patch20* patch = reinterpret_cast<Res_png_9patch20*>(inData);
    patch->wasDeserialized = true;
    fill9patchOffsets(patch);
    return patch;
}
