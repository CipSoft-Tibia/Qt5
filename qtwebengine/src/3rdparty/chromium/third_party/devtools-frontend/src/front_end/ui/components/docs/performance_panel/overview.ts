// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import * as EnvironmentHelpers from '../../../../../test/unittests/front_end/helpers/EnvironmentHelpers.js';
import * as TraceLoader from '../../../../../test/unittests/front_end/helpers/TraceLoader.js';
import * as TraceEngine from '../../../../models/trace/trace.js';
import * as Timeline from '../../../../panels/timeline/timeline.js';
import * as ComponentSetup from '../../helpers/helpers.js';

await EnvironmentHelpers.initializeGlobalVars();
await ComponentSetup.ComponentServerSetup.setup();
const container = document.querySelector<HTMLElement>('div.container');
if (!container) {
  throw new Error('could not find container');
}

const params = new URLSearchParams(window.location.search);
const fileName = (params.get('trace') || 'web-dev') + '.json.gz';

async function renderMiniMap(containerSelector: string, options: {showMemory: boolean}) {
  const container = document.querySelector<HTMLElement>(containerSelector);
  if (!container) {
    throw new Error('could not find container');
  }
  const minimap = new Timeline.TimelineMiniMap.TimelineMiniMap();
  minimap.activateBreadcrumbs();
  minimap.markAsRoot();
  minimap.show(container);

  const models = await TraceLoader.TraceLoader.allModels(null, fileName);

  minimap.setBounds(
      TraceEngine.Types.Timing.MilliSeconds(models.timelineModel.minimumRecordTime()),
      TraceEngine.Types.Timing.MilliSeconds(models.timelineModel.maximumRecordTime()),
  );
  minimap.setData({
    traceParsedData: models.traceParsedData,
    performanceModel: models.performanceModel,
    settings: {
      showMemory: options.showMemory,
      showScreenshots: true,
    },
  });
  minimap.setWindowTimes(models.performanceModel.window().left, models.performanceModel.window().right);
}

await renderMiniMap('.container', {showMemory: false});
await renderMiniMap('.container-with-memory', {showMemory: true});
