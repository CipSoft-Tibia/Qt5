// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {data as metaHandlerData} from './MetaHandler.js';

import {type TraceEventHandlerName, HandlerState} from './types.js';

import * as Types from '../types/types.js';
import * as Helpers from '../helpers/helpers.js';

let handlerState = HandlerState.UNINITIALIZED;

// Each thread contains events. Events indicate the thread and process IDs, which are
// used to store the event in the correct process thread entry below.
const eventsInProcessThread =
    new Map<Types.TraceEvents.ProcessID, Map<Types.TraceEvents.ThreadID, Types.TraceEvents.TraceEventGPUTask[]>>();

let mainGPUThreadTasks: Types.TraceEvents.TraceEventGPUTask[] = [];

export function reset(): void {
  eventsInProcessThread.clear();
  mainGPUThreadTasks = [];

  handlerState = HandlerState.UNINITIALIZED;
}

export function initialize(): void {
  if (handlerState !== HandlerState.UNINITIALIZED) {
    throw new Error('GPU Handler was not reset before being initialized');
  }

  handlerState = HandlerState.INITIALIZED;
}

export function handleEvent(event: Types.TraceEvents.TraceEventData): void {
  if (handlerState !== HandlerState.INITIALIZED) {
    throw new Error('GPU Handler is not initialized');
  }

  if (!Types.TraceEvents.isTraceEventGPUTask(event)) {
    return;
  }

  Helpers.Trace.addEventToProcessThread(event, eventsInProcessThread);
}

export async function finalize(): Promise<void> {
  if (handlerState !== HandlerState.INITIALIZED) {
    throw new Error('GPU Handler is not initialized');
  }

  const {gpuProcessId, gpuThreadId} = metaHandlerData();
  const gpuThreadsForProcess = eventsInProcessThread.get(gpuProcessId);
  if (gpuThreadsForProcess && gpuThreadId) {
    mainGPUThreadTasks = gpuThreadsForProcess.get(gpuThreadId) || [];
  }
  handlerState = HandlerState.FINALIZED;
}

export interface GPUHandlerReturnData {
  mainGPUThreadTasks: readonly Types.TraceEvents.TraceEventGPUTask[];
}

export function data(): GPUHandlerReturnData {
  if (handlerState !== HandlerState.FINALIZED) {
    throw new Error('GPU Handler is not finalized');
  }
  return {
    mainGPUThreadTasks: [...mainGPUThreadTasks],
  };
}

export function deps(): TraceEventHandlerName[] {
  return ['Meta'];
}
