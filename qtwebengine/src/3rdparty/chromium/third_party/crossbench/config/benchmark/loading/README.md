# Loading benchmark

This folder contains configs for the loading benchmark. The goal of the benchmark is to facilitate web performance optimization based on a realistic workload. The benchmark has two workload variants:

* A general-purpose workload representative of the web usage on mobile phones ("phone");

* A workload representing the projected web usage on Android tablets ("tablet").

To maintain reproducibility, the benchmark uses the [web page replay](https://chromium.googlesource.com/catapult/+/HEAD/web_page_replay_go/README.md) mechanism (requires golang, check [go.mod](https://chromium.googlesource.com/catapult/+/HEAD/web_page_replay_go/go.mod) for the minimum version). Archives of the web pages are stored in the `chrome-partner-telemetry` cloud bucket, so you'll need access to that bucket to run the benchmark on recorded pages (you can still run the benchmark on live sites if you don't have the access, but there's no guarantee that results will be reproducible/comparable).

## Running the benchmark

With the "phone" workload:

```
./cb.py loading \
  --page-config config/benchmark/loading/page_config_phone.hjson \
  --probe-config config/benchmark/loading/probe_config.hjson \
  --network-config config/benchmark/loading/network_config_phone.hjson \
  --separate --browser <browser>
```

With the "tablet" workload:

```
./cb.py loading \
  --page-config config/benchmark/loading/page_config_tablet.hjson \
  --probe-config config/benchmark/loading/probe_config.hjson \
  --network-config config/benchmark/loading/network_config_tablet.hjson \
  --separate --browser <browser>  -- --request-desktop-sites
```

The browser can be `android:chrome-canary`, `android:chrome-stable` etc. See crossbench docs for the full list of options.

Metric results for each page will be located in `results/latest/runs/*/trace_processor/lcp_metric.json`.

## Common issues

### Problems finding wpr.go

If you see a `Could not find wpr.go binary` error:

* If you have chromium checked out locally: set `CHROMIUM_SRC` environment variable to the path of your chromium/src folder.

* If not (or if you're still getting this error): see the next section.

### Running the benchmark without full chromium checkout

Follow the [crossbench development instructions](https://chromium.googlesource.com/crossbench/#development)
to check out code and run crossbench standalone.

### Problems accessing the cloud bucket

In some cases you might need to download the web page archive manually. In this case, save the archive file corresponding to the version you are running (`gs://chrome-partner-telemetry/loading_benchmark/archive_*.wprgo`) locally and run the benchmark as follows:

```
./cb.py loading \
  --page-config config/benchmark/loading/page_config_phone.hjson \
  --probe-config config/benchmark/loading/probe_config.hjson \
  --network <path to archive.wprgo> \
  --separate --browser <browser>
```

## Other running options

### Run the benchmark on live sites

```
./cb.py loading \
  --page-config config/benchmark/loading/page_config_phone.hjson \
  --probe-config config/benchmark/loading/probe_config.hjson \
  --separate --browser <browser>
```

### Record a new WPR archive

Uncomment the `wpr: {},` line in the probe config and run the benchmark on live sites (see the command above). The archive will be located in `results/latest/archive.wprgo`

### Run the benchmark with full set of experimental metrics

```
./cb.py loading \
  --page-config config/benchmark/loading/page_config_phone.hjson \
  --probe-config config/benchmark/loading/probe_config_experimental.hjson \
  --network-config config/benchmark/loading/network_config_phone.hjson \
  --separate --browser <browser>
```

Note that computing extra metrics takes additional time and the trace size can be quite large as well.
