// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import * as Common from '../../core/common/common.js';
import * as Helpers from '../../models/trace/helpers/helpers.js';
import * as TraceEngine from '../../models/trace/trace.js';
import * as TimingTypes from '../../models/trace/types/types.js';
import * as PerfUI from '../../ui/legacy/components/perf_ui/perf_ui.js';
import * as UI from '../../ui/legacy/legacy.js';

import * as TimelineComponents from './components/components.js';
import {type PerformanceModel} from './PerformanceModel.js';
import {
  type TimelineEventOverview,
  TimelineEventOverviewCPUActivity,
  TimelineEventOverviewMemory,
  TimelineEventOverviewNetwork,
  TimelineEventOverviewResponsiveness,
  TimelineFilmStripOverview,
} from './TimelineEventOverview.js';
import miniMapStyles from './timelineMiniMap.css.js';
import {TimelineUIUtils} from './TimelineUIUtils.js';

export interface OverviewData {
  performanceModel: PerformanceModel|null;
  traceParsedData: TraceEngine.Handlers.Migration.PartialTraceData|null;
  settings: {
    showScreenshots: boolean,
    showMemory: boolean,
  };
}

/**
 * This component wraps the generic PerfUI Overview component and configures it
 * specifically for the Performance Panel, including injecting the CSS we use
 * to customise how the components render within the Performance Panel.
 */
export class TimelineMiniMap extends
    Common.ObjectWrapper.eventMixin<PerfUI.TimelineOverviewPane.EventTypes, typeof UI.Widget.VBox>(UI.Widget.VBox) {
  #overviewComponent = new PerfUI.TimelineOverviewPane.TimelineOverviewPane('timeline');
  #controls: TimelineEventOverview[] = [];
  #breadcrumbs: TimelineComponents.Breadcrumbs.Breadcrumbs|null = null;
  #breadcrumbsUI: TimelineComponents.BreadcrumbsUI.BreadcrumbsUI;
  #minTime: TimingTypes.Timing.MilliSeconds = TimingTypes.Timing.MilliSeconds(0);

  constructor() {
    super();
    this.element.classList.add('timeline-minimap');
    this.#breadcrumbsUI = new TimelineComponents.BreadcrumbsUI.BreadcrumbsUI();

    this.#overviewComponent.show(this.element);
    // Push the event up into the parent component so the panel knows when the window is changed.
    this.#overviewComponent.addEventListener(PerfUI.TimelineOverviewPane.Events.WindowChanged, event => {
      this.dispatchEventToListeners(PerfUI.TimelineOverviewPane.Events.WindowChanged, event.data);
    });
  }

  activateBreadcrumbs(): void {
    this.element.prepend(this.#breadcrumbsUI);
    this.#overviewComponent.addEventListener(PerfUI.TimelineOverviewPane.Events.WindowChanged, event => {
      this.addBreadcrumb(
          TraceEngine.Types.Timing.MilliSeconds(event.data.startTime),
          TraceEngine.Types.Timing.MilliSeconds(event.data.endTime));
    });
  }

  addBreadcrumb(start: TraceEngine.Types.Timing.MilliSeconds, end: TraceEngine.Types.Timing.MilliSeconds): void {
    const startWithoutMin = start - this.#minTime;
    const endWithoutMin = end - this.#minTime;

    const traceWindow: TraceEngine.Types.Timing.TraceWindow = {
      min: TraceEngine.Types.Timing.MicroSeconds(startWithoutMin),
      max: TraceEngine.Types.Timing.MicroSeconds(endWithoutMin),
      range: TraceEngine.Types.Timing.MicroSeconds(endWithoutMin - startWithoutMin),
    };
    if (this.#breadcrumbs === null) {
      this.#breadcrumbs = new TimelineComponents.Breadcrumbs.Breadcrumbs(traceWindow);

    } else {
      this.#breadcrumbs.add(traceWindow);
      this.setBounds(start, end);

      this.#overviewComponent.scheduleUpdate(start, end);
    }

    this.#breadcrumbsUI.data = {
      breadcrumb: this.#breadcrumbs.initialBreadcrumb,
    };
  }

  override wasShown(): void {
    super.wasShown();
    this.registerCSSFiles([miniMapStyles]);
  }

  reset(): void {
    this.#overviewComponent.reset();
  }

  setBounds(min: TraceEngine.Types.Timing.MilliSeconds, max: TraceEngine.Types.Timing.MilliSeconds): void {
    this.#overviewComponent.setBounds(min, max);
  }

  setWindowTimes(left: number, right: number): void {
    this.#overviewComponent.setWindowTimes(left, right);
  }

  #setMarkers(traceParsedData: TraceEngine.Handlers.Migration.PartialTraceData): void {
    const markers = new Map<number, Element>();

    const {Meta, PageLoadMetrics} = traceParsedData;

    // Add markers for navigation start times.
    const navStartEvents = Meta.mainFrameNavigations;
    const minTimeInMilliseconds = TraceEngine.Helpers.Timing.microSecondsToMilliseconds(Meta.traceBounds.min);

    for (const event of navStartEvents) {
      const {startTime} = TraceEngine.Legacy.timesForEventInMilliseconds(event);
      markers.set(startTime, TimelineUIUtils.createEventDivider(event, minTimeInMilliseconds));
    }

    // Now add markers for the page load events
    for (const event of PageLoadMetrics.allMarkerEvents) {
      const {startTime} = TraceEngine.Legacy.timesForEventInMilliseconds(event);
      markers.set(startTime, TimelineUIUtils.createEventDivider(event, minTimeInMilliseconds));
    }

    this.#overviewComponent.setMarkers(markers);
  }

  #setNavigationStartEvents(traceParsedData: TraceEngine.Handlers.Migration.PartialTraceData): void {
    this.#overviewComponent.setNavStartTimes(traceParsedData.Meta.mainFrameNavigations);
  }

  setData(data: OverviewData): void {
    this.#controls = [];
    if (data.traceParsedData?.Meta.traceBounds.min !== undefined) {
      this.#minTime = Helpers.Timing.microSecondsToMilliseconds(data.traceParsedData?.Meta.traceBounds.min);
    }

    if (data.traceParsedData) {
      this.#setMarkers(data.traceParsedData);
      this.#setNavigationStartEvents(data.traceParsedData);
      this.#controls.push(new TimelineEventOverviewResponsiveness(data.traceParsedData));
    }

    // CPU Activity is the only component that relies on the old model and will
    // do so until we have finished migrating the Main Thread track to the new
    // trace engine
    // TODO(crbug.com/1428024) Migrate CPU track to the new model once the Main thread is migrated to the trace engine
    if (data.performanceModel) {
      this.#controls.push(new TimelineEventOverviewCPUActivity(data.performanceModel));
    }

    if (data.traceParsedData) {
      this.#controls.push(new TimelineEventOverviewNetwork(data.traceParsedData));
    }
    if (data.settings.showScreenshots && data.traceParsedData) {
      const filmStrip = TraceEngine.Extras.FilmStrip.fromTraceData(data.traceParsedData);
      if (filmStrip.frames.length) {
        this.#controls.push(new TimelineFilmStripOverview(filmStrip));
      }
    }
    if (data.settings.showMemory && data.traceParsedData) {
      this.#controls.push(new TimelineEventOverviewMemory(data.traceParsedData));
    }
    this.#overviewComponent.setOverviewControls(this.#controls);
  }
}
