-- LCP
CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_google_doc_score()
RETURNS FLOAT
AS
SELECT 1e9 / dur
FROM slice
WHERE name = 'PageLoadMetrics.NavigationToLargestContentfulPaint'
ORDER BY ts
LIMIT 1;
