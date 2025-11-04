INCLUDE PERFETTO MODULE ext.loading_benchmark_amazon_product;
INCLUDE PERFETTO MODULE ext.loading_benchmark_cnn_article;
INCLUDE PERFETTO MODULE ext.loading_benchmark_globo_homepage;
INCLUDE PERFETTO MODULE ext.loading_benchmark_google_doc;
INCLUDE PERFETTO MODULE ext.loading_benchmark_google_search_result;
INCLUDE PERFETTO MODULE ext.loading_benchmark_wikipedia_article;
INCLUDE PERFETTO MODULE ext.loading_benchmark_youtube_video;

CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_get_name()
RETURNS STRING
AS
SELECT DISTINCT substr(name, length('PresetPageLoadBenchmark/') + 1)
FROM slice
WHERE category = 'blink.user_timing' AND name GLOB 'PresetPageLoadBenchmark/*'
LIMIT 1;

CREATE OR REPLACE PERFETTO FUNCTION loading_benchmark_score()
RETURNS FLOAT
AS
SELECT
  CASE loading_benchmark_get_name()
    WHEN 'loading-phone/amazon_product' THEN loading_benchmark_amazon_product_score()
    WHEN 'loading-tablet/amazon_product' THEN loading_benchmark_amazon_product_score()
    WHEN 'loading-phone/cnn_article' THEN loading_benchmark_phone_cnn_article_score()
    WHEN 'loading-tablet/cnn_article' THEN loading_benchmark_tablet_cnn_article_score()
    WHEN 'loading-phone/globo_homepage' THEN loading_benchmark_globo_homepage_score()
    WHEN 'loading-tablet/google_doc' THEN loading_benchmark_google_doc_score()
    WHEN 'loading-phone/google_search_result' THEN loading_benchmark_google_search_result_score()
    WHEN 'loading-tablet/google_search_result' THEN loading_benchmark_google_search_result_score()
    WHEN 'loading-phone/wikipedia_article' THEN loading_benchmark_wikipedia_article_score()
    WHEN 'loading-tablet/youtube_video' THEN loading_benchmark_youtube_video_score()
    ELSE NULL
    END AS score;
