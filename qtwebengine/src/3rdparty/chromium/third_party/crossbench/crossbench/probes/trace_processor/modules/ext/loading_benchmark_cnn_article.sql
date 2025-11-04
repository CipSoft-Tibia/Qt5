INCLUDE PERFETTO MODULE ext.first_presentation_time;
INCLUDE PERFETTO MODULE ext.navigation_start;

-- This metric returns the time the headline text element takes to show up.
CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_phone_cnn_article_score()
RETURNS FLOAT
AS
SELECT
  1e9 / (
    get_first_presentation_time_for_event('maincontent.created')
    - first_navigation_start());

-- This metric returns the time the headline text element takes to show up
-- after the second (which is also the last) page load.
-- The first page load is "incomplete" - it shows the cookie banner and
-- doesn't load some of the content like ads. We click on the cookie
-- banner, triggering the second ("complete") page load.
CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_tablet_cnn_article_score()
RETURNS FLOAT
AS
WITH
  last_navigation_maincontent_created AS (
    SELECT ts
    FROM slice
    WHERE
      name = 'maincontent.created'
      AND cat = 'blink.user_timing'
      AND ts > last_navigation_start()
    ORDER BY ts
    LIMIT 1
  )
SELECT
  1e9
  / (
    get_next_presentation_time(
      (SELECT ts FROM last_navigation_maincontent_created))
    - last_navigation_start())
  / 1e6;
