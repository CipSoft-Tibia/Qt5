-- Copyright 2023 The Chromium Authors
-- Use of this source code is governed by a BSD-style license that can be
-- found in the LICENSE file.
--

SELECT IMPORT('common.slices');

-- Hardware info is useful when using sql metrics for analysis
-- in BTP.
SELECT IMPORT('chrome.metadata');
SELECT IMPORT('chrome.scroll_jank.event_latency_scroll_jank_cause');

-- Grabs all gesture updates that were not coalesced with their
-- respective scroll ids and start/end timestamps.
--
-- @column id                       Slice id.
-- @column start_ts                 The start timestamp of the scroll.
-- @column end_ts                   The end timestamp of the scroll.
-- @column last_coalesced_input_ts  The timestamp of the last coalesced input.
-- @column scroll_update_id         The id of the scroll update event.
-- @column scroll_id                The id of the scroll.
CREATE VIEW chrome_presented_gesture_scrolls AS
WITH
  chrome_gesture_scrolls AS (
    SELECT
      ts AS start_ts,
      ts + dur AS end_ts,
      id,
      -- TODO(b/250089570) Add trace_id to EventLatency and update this script to use it.
      EXTRACT_ARG(arg_set_id, 'chrome_latency_info.trace_id') AS scroll_update_id,
      EXTRACT_ARG(arg_set_id, 'chrome_latency_info.gesture_scroll_id') AS scroll_id,
      EXTRACT_ARG(arg_set_id, 'chrome_latency_info.is_coalesced') AS is_coalesced
    FROM slice
    WHERE name = "InputLatency::GestureScrollUpdate"
          AND dur != -1),
  updates_with_coalesce_info AS (
    SELECT
      chrome_updates.*,
      (
        SELECT
          MAX(id)
        FROM chrome_gesture_scrolls updates
        WHERE updates.is_coalesced = false
          AND updates.start_ts <= chrome_updates.start_ts) AS coalesced_inside_id
        FROM
          chrome_gesture_scrolls chrome_updates)
SELECT
  MIN(id) AS id,
  MIN(start_ts) AS start_ts,
  MAX(end_ts) AS end_ts,
  MAX(start_ts) AS last_coalesced_input_ts,
  scroll_update_id,
  MIN(scroll_id) AS scroll_id
FROM updates_with_coalesce_info
GROUP BY coalesced_inside_id;

-- Associate every trace_id with it's perceived delta_y on the screen after
-- prediction.
--
-- @column scroll_update_id         The id of the scroll update event.
-- @column delta_y                  The perceived delta_y on the screen post
--                                  prediction.
CREATE VIEW chrome_scroll_updates_with_deltas AS
SELECT
  EXTRACT_ARG(arg_set_id, 'scroll_deltas.trace_id') AS scroll_update_id,
  EXTRACT_ARG(arg_set_id, 'scroll_deltas.provided_to_compositor_delta_y') AS delta_y
FROM slice
WHERE name = "InputHandlerProxy::HandleGestureScrollUpdate_Result";

-- Extract event latency timestamps, to later use it for joining
-- with gesture scroll updates, as event latencies don't have trace
-- ids associated with it.
--
-- @column start_ts                     Start timestampt for the EventLatency.
-- @column event_latency_id             Slice id of the EventLatency.
-- @column dur                          Duration of the EventLatency.
-- @column input_latency_end_ts         End timestamp for input aka the
--                                      timestamp of the LatchToSwapEnd
--                                      substage.
-- @column presentation_timestamp       Frame presentation timestamp aka the
--                                      timestamp of the
--                                      SwapEndToPresentationCompositorFrame
--                                      substage.
-- @column event_type                   EventLatency event type.
CREATE VIEW chrome_gesture_scroll_event_latencies AS
SELECT
  slice.ts AS start_ts,
  slice.id AS event_latency_id,
  slice.dur AS dur,
  descendant_slice_end(slice.id, "LatchToSwapEnd") AS input_latency_end_ts,
  descendant_slice_end(slice.id, "SwapEndToPresentationCompositorFrame") AS presentation_timestamp,
  EXTRACT_ARG(arg_set_id, 'event_latency.event_type') AS event_type
FROM slice
WHERE name = "EventLatency"
      AND event_type in (
          "GESTURE_SCROLL_UPDATE",
          "FIRST_GESTURE_SCROLL_UPDATE",
          "INERTIAL_GESTURE_SCROLL_UPDATE")
      AND has_descendant_slice_with_name(slice.id, "SwapEndToPresentationCompositorFrame");

-- Join presented gesture scrolls with their respective event
-- latencies based on |LatchToSwapEnd| timestamp, as it's the
-- end timestamp for both the gesture scroll update slice and
-- the LatchToSwapEnd slice.
--
-- @column id                           ID of the frame.
-- @column start_ts                     Start timestamp of the frame.
-- @column last_coalesced_input_ts      The timestamp of the last coalesced
--                                      input.
-- @column scroll_id                    ID of the associated scroll.
-- @column scroll_update_id             ID of the associated scroll update.
-- @column event_latency_id             ID of the associated EventLatency.
-- @column dur                          Duration of the associated EventLatency.
-- @column presentation_timestamp       Frame presentation timestamp.
CREATE VIEW chrome_full_frame_view AS
SELECT
  frames.id,
  frames.start_ts,
  frames.last_coalesced_input_ts,
  frames.scroll_id,
  frames.scroll_update_id,
  events.event_latency_id,
  events.dur,
  events.presentation_timestamp
FROM chrome_presented_gesture_scrolls frames
JOIN chrome_gesture_scroll_event_latencies events
  ON frames.start_ts = events.start_ts
  AND events.input_latency_end_ts = frames.end_ts;

-- Join deltas with EventLatency data.
--
-- @column id                           ID of the frame.
-- @column start_ts                     Start timestamp of the frame.
-- @column scroll_id                    ID of the associated scroll.
-- @column scroll_update_id             ID of the associated scroll update.
-- @column last_coalesced_input_ts      The timestamp of the last coalesced
--                                      input.
-- @column delta_y                      The perceived delta_y on the screen post
-- --                                   prediction.
-- @column event_latency_id             ID of the associated EventLatency.
-- @column dur                          Duration of the associated EventLatency.
-- @column presentation_timestamp       Frame presentation timestamp.
CREATE VIEW chrome_full_frame_delta_view AS
SELECT
  frames.id,
  frames.start_ts,
  frames.scroll_id,
  frames.scroll_update_id,
  frames.last_coalesced_input_ts,
  deltas.delta_y,
  frames.event_latency_id,
  frames.dur,
  frames.presentation_timestamp
FROM chrome_full_frame_view frames
LEFT JOIN chrome_scroll_updates_with_deltas deltas
  ON deltas.scroll_update_id = frames.scroll_update_id;

-- Join the frame view with scroll jank cause and subcause based
-- on event latency id.
--
-- @column id                           ID of the frame.
-- @column start_ts                     Start timestamp of the frame.
-- @column scroll_id                    ID of the associated scroll.
-- @column scroll_update_id             ID of the associated scroll update.
-- @column last_coalesced_input_ts      The timestamp of the last coalesced
--                                      input.
-- @column delta_y                      The perceived delta_y on the screen post
-- --                                   prediction.
-- @column event_latency_id             ID of the associated EventLatency.
-- @column dur                          Duration of the associated EventLatency.
-- @column presentation_timestamp       Frame presentation timestamp.
-- @column cause_of_jank                The stage of the EventLatency that is
--                                      the cause of jank.
-- @column sub_cause_of_jank            The sub-stage of the cause_of_jank that
--                                      is the cause of jank, if it exists.
CREATE PERFETTO TABLE chrome_frame_view_with_jank AS
SELECT
  frames.*,
  jank_cause.cause_of_jank,
  jank_cause.sub_cause_of_jank
FROM chrome_event_latency_scroll_jank_cause jank_cause
RIGHT JOIN chrome_full_frame_delta_view frames
  ON jank_cause.slice_id = frames.event_latency_id;

-- Group all gestures presented at the same timestamp together in
-- a single row.
--
-- @column id                           ID of the frame.
-- @column max_start_ts                 The timestamp of the last coalesced
--                                      input.
-- @column min_start_ts                 The earliest frame start timestamp.
-- @column scroll_id                    ID of the associated scroll.
-- @column scroll_update_id             ID of the associated scroll update.
-- @column encapsulated_scroll_ids      All scroll updates associated with the
--                                      frame presentation timestamp.
-- @column total_delta                  Sum of all perceived delta_y values at
--                                      the frame presentation timestamp.
-- @column segregated_delta_y           Lists all of the perceived delta_y
--                                      values at the frame presentation
--                                      timestamp.
-- @column event_latency_id             ID of the associated EventLatency.
-- @column dur                          Maximum duration of the associated
--                                      EventLatency.
-- @column presentation_timestamp       Frame presentation timestamp.
-- @column cause_of_jank                The stage of the EventLatency that is
--                                      the cause of jank.
-- @column sub_cause_of_jank            The sub-stage of the cause_of_jank that
--                                      is the cause of jank, if it exists.
CREATE VIEW chrome_merged_frame_view_with_jank AS
SELECT
  id,
  MAX(last_coalesced_input_ts) AS max_start_ts,
  MIN(start_ts) AS min_start_ts,
  scroll_id,
  scroll_update_id,
  GROUP_CONCAT(scroll_update_id,',') AS encapsulated_scroll_ids,
  SUM(delta_y) AS total_delta,
  GROUP_CONCAT(delta_y, ',') AS segregated_delta_y,
  event_latency_id,
  MAX(dur) AS dur,
  presentation_timestamp,
  cause_of_jank,
  sub_cause_of_jank
FROM chrome_frame_view_with_jank
GROUP BY presentation_timestamp
ORDER BY presentation_timestamp;

-- View contains all chrome presented frames during gesture updates
-- while calculating delay since last presented which usually should
-- equal to |VSYNC_INTERVAL| if no jank is present.
--
-- @column id                      gesture scroll slice id.
-- @column min_start_ts            OS timestamp of the first touch move arrival
--                                 within a frame.
-- @column max_start_ts            OS timestamp of the last touch move arrival
--                                 within a frame.
-- @column scroll_id               The scroll which the touch belongs to.
-- @column encapsulated_scroll_ids Trace ids of all frames presented in at this
--                                 vsync.
-- @column total_delta             Summation of all delta_y of all gesture
--                                 scrolls in this frame.
-- @column segregated_delta_y      All delta y of all gesture scrolls comma
--                                 separated, summing those gives |total_delta|.
-- @column event_latency_id        Event latency id of the presented frame.
-- @column dur                     Duration of the EventLatency.
-- @column presentation_timestamp  Timestamp at which the frame was shown on the
--                                 screen.
-- @column cause_of_jank           Cause of jank will be present if a frame
--                                 takes more than 1/2 a vsync than it's
--                                 neighbours, will be filtered to real
--                                 positives later.
-- @column sub_cause_of_jank       If the cause is GPU related, a sub cause is
--                                 present for further breakdown.
-- @column delay_since_last_frame  Time elapsed since the previous frame was
--                                 presented, usually equals |VSYNC| if no frame
--                                 drops happened.
-- @column delay_since_last_input  Difference in OS timestamps of inputs in the
--                                 current and the previous frame.
CREATE VIEW chrome_janky_frame_info_with_delay AS
SELECT
  *,
  (presentation_timestamp -
  LAG(presentation_timestamp, 1, presentation_timestamp)
  OVER (PARTITION BY scroll_id ORDER BY presentation_timestamp)) / 1e6 AS delay_since_last_frame,
  (min_start_ts -
  LAG(max_start_ts, 1, min_start_ts)
  OVER (PARTITION BY scroll_id ORDER BY min_start_ts)) / 1e6 AS delay_since_last_input
FROM chrome_merged_frame_view_with_jank;

-- Calculate |VSYNC_INTERVAL| as the lowest delay between frames larger than
-- zero.
-- TODO(b/286222128): Emit this data from Chrome instead of calculating it.
--
-- @column vsync_interval           The lowest delay between frames larger than
--                                  zero.
CREATE VIEW chrome_vsyncs AS
SELECT
  MIN(delay_since_last_frame) AS vsync_interval
FROM chrome_janky_frame_info_with_delay
WHERE delay_since_last_frame > 0;

-- Filter the frame view only to frames that had missed vsyncs.
--
-- @column cause_of_jank          The reason the Vsync was missed.
-- @column sub_cause_of_jank      Further breakdown if the root cause was GPU
--                                related.
-- @column delay_since_last_frame Time elapsed since the previous frame was
--                                presented, will be more than |VSYNC| in this
--                                view.
-- @column event_latency_id       Event latency id of the presented frame.
-- @column vsync_interval         Vsync interval at the time of recording the
--                                trace.
-- @column hardware_class         Device brand and model.
-- @column scroll_id              The scroll corresponding to this frame.
CREATE VIEW chrome_janky_frames AS
SELECT
  cause_of_jank,
  sub_cause_of_jank,
  delay_since_last_frame,
  event_latency_id,
  (SELECT vsync_interval FROM chrome_vsyncs) AS vsync_interval,
  chrome_hardware_class() AS hardware_class,
  scroll_id
FROM chrome_janky_frame_info_with_delay
WHERE delay_since_last_frame > (select vsync_interval + vsync_interval / 2 from chrome_vsyncs)
      AND delay_since_last_input < (select vsync_interval + vsync_interval / 2 from chrome_vsyncs);

-- Counting all unique frame presentation timestamps.
--
-- @column presentation_timestamp     The unique frame presentation timestamp.
CREATE VIEW chrome_unique_frame_presentation_ts AS
SELECT DISTINCT
presentation_timestamp
FROM chrome_gesture_scroll_event_latencies;

-- Dividing missed frames over total frames to get janky frame percentage.
-- This represents the v3 scroll jank metrics.
-- Reflects Event.Jank.DelayedFramesPercentage UMA metric.
--
-- @column delayed_frame_percentage       The percent of missed frames relative
--                                        to total frames - aka the percent of
--                                        janky frames.
CREATE VIEW chrome_janky_frames_percentage AS
SELECT
(SELECT
  COUNT()
 FROM chrome_janky_frames) * 1.0
/ (SELECT
    COUNT()
  FROM chrome_unique_frame_presentation_ts) * 100 AS delayed_frame_percentage;

-- Number of frames and janky frames per scroll.
--
-- @column scroll_id                  The ID of the scroll.
-- @column num_frames                 The number of frames in the scroll.
-- @column num_janky_frames           The number of delayed/janky frames.
-- @column scroll_jank_percentage     The percentage of janky frames relative to
--                                    total frames.
CREATE VIEW chrome_frames_per_scroll AS
WITH
  frames AS (
    SELECT scroll_id, COUNT(*) AS num_frames
    FROM
      chrome_janky_frame_info_with_delay
    GROUP BY scroll_id
  ),
  janky_frames AS (
    SELECT scroll_id, COUNT(*) AS num_janky_frames
    FROM
      chrome_janky_frames
    GROUP BY scroll_id
  )
SELECT
  frames.scroll_id AS scroll_id,
  frames.num_frames AS num_frames,
  janky_frames.num_janky_frames AS num_janky_frames,
  100.0 * janky_frames.num_janky_frames / frames.num_frames
    AS scroll_jank_percentage
FROM frames
LEFT JOIN janky_frames
  ON frames.scroll_id = janky_frames.scroll_id;

-- Scroll jank causes per scroll.
--
-- @column scroll_id                   The ID of the scroll.
-- @column max_delay_since_last_frame  The maximum time a frame was delayed
--                                     after the presentation of the previous
--                                     frame.
-- @column vsync_interval              The expected vsync interval.
-- @column scroll_jank_causes          A proto amalgamation of each scroll
--                                     jank cause including cause name, sub
--                                     cause and the duration of the delay
--                                     since the previous frame was presented.
CREATE VIEW chrome_causes_per_scroll AS
SELECT
  scroll_id,
  MAX(1.0 * delay_since_last_frame / vsync_interval)
    AS max_delay_since_last_frame,
  -- MAX does not matter, since `vsync_interval` is the computed as the
  -- same value for a single trace.
  MAX(vsync_interval) AS vsync_interval,
  RepeatedField(
    ChromeScrollJankV3_Scroll_ScrollJankCause(
      'cause',
      cause_of_jank,
      'sub_cause',
      sub_cause_of_jank,
      'delay_since_last_frame',
      1.0 * delay_since_last_frame / vsync_interval))
    AS scroll_jank_causes
FROM
  chrome_janky_frames
GROUP BY scroll_id;

-- An "intermediate" view for computing `chrome_scroll_jank_v3_output` below.
--
-- @column trace_num_frames          The number of frames in the trace.
-- @column trace_num_janky_frames    The number of delayed/janky frames in the
--                                   trace.
-- @column vsync_interval            The standard vsync interval.
-- @column scrolls                   A proto amalgamation of metrics per scroll
--                                   including the number of frames, number of
--                                   janky frames, percent of janky frames,
--                                   maximum presentation delay, and the causes
--                                   of jank (cause, sub-cause, delay).
CREATE VIEW chrome_scroll_jank_v3_intermediate AS
SELECT
  -- MAX does not matter for these aggregations, since the values are the
  -- same across rows.
  (SELECT COUNT(*) FROM chrome_janky_frame_info_with_delay)
    AS trace_num_frames,
  (SELECT COUNT(*) FROM chrome_janky_frames)
    AS trace_num_janky_frames,
  causes.vsync_interval,
  RepeatedField(
    ChromeScrollJankV3_Scroll(
      'num_frames',
      frames.num_frames,
      'num_janky_frames',
      frames.num_janky_frames,
      'scroll_jank_percentage',
      frames.scroll_jank_percentage,
      'max_delay_since_last_frame',
      causes.max_delay_since_last_frame,
      'scroll_jank_causes',
      causes.scroll_jank_causes))
    AS scrolls
FROM
  chrome_frames_per_scroll AS frames
INNER JOIN chrome_causes_per_scroll AS causes
  ON frames.scroll_id = causes.scroll_id;

-- For producing a "native" Perfetto UI metric.
--
-- @column scroll_jank_summary     A proto amalgamation summarizing all of the
--                                 scroll jank in a trace, including the number
--                                 of frames, janky frames, percentage of janky
--                                 frames, vsync interval, and a summary of this
--                                 data (including individual causes) for each
--                                 scroll.
CREATE VIEW chrome_scroll_jank_v3_output AS
SELECT
  ChromeScrollJankV3(
    'trace_num_frames',
    trace_num_frames,
    'trace_num_janky_frames',
    trace_num_janky_frames,
    'trace_scroll_jank_percentage',
    100.0 * trace_num_janky_frames / trace_num_frames,
    'vsync_interval_ms',
    vsync_interval,
    'scrolls',
    scrolls) AS scroll_jank_summary
FROM
  chrome_scroll_jank_v3_intermediate;
