INCLUDE PERFETTO MODULE ext.first_presentation_time;
INCLUDE PERFETTO MODULE ext.navigation_start;

-- This metric returns the time the cookie banner takes to disappear.
CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_youtube_video_score()
RETURNS FLOAT
AS
SELECT
  1e9
  / (get_first_presentation_time_for_event('cookie_banner_gone') - first_navigation_start());
