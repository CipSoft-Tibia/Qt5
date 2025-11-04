# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.jetstream.jetstream_3 import (JetStream3Benchmark,
                                                         JetStream3Probe,
                                                         JetStream3Story,
                                                         ProbeClsTupleT)


class JetStream30Probe(JetStream3Probe):
  __doc__ = JetStream3Probe.__doc__
  NAME: str = "jetstream_3.0"


class JetStream30Story(JetStream3Story):
  __doc__ = JetStream3Story.__doc__
  NAME: str = "jetstream_3.0"
  URL: str = "https://chromium-workloads.web.app/jetstream/v3.0/"
  URL_OFFICIAL: str = "https://browserbench.org/JetStream3.0/"
  SUBSTORIES: Tuple[str, ...] = (
      "WSL",
      "UniPoker",
      "uglify-js-wtb",
      "typescript",
      "tsf-wasm",
      "tfjs-wasm-simd",
      "tfjs-wasm",
      "tagcloud-SP",
      "sync-fs",
      "string-unpack-code-SP",
      "stanford-crypto-sha256",
      "stanford-crypto-pbkdf2",
      "stanford-crypto-aes",
      "splay",
      "segmentation",
      "richards-wasm",
      "richards",
      "regexp",
      "regex-dna-SP",
      "raytrace-public-class-fields",
      "raytrace-private-class-fields",
      "raytrace",
      "quicksort-wasm",
      "proxy-vue",
      "proxy-mobx",
      "prepack-wtb",
      "pdfjs",
      "OfflineAssembler",
      "octane-zlib",
      "octane-code-load",
      "navier-stokes",
      "n-body-SP",
      "multi-inspector-code-load",
      "ML",
      "mandreel",
      "lebab-wtb",
      "lazy-collections",
      "json-stringify-inspector",
      "json-parse-inspector",
      "jshint-wtb",
      "js-tokens",
      "HashSet-wasm",
      "hash-map",
      "gcc-loops-wasm",
      "gbemu",
      "gaussian-blur",
      "float-mm.c",
      "FlightPlanner",
      "first-inspector-code-load",
      "espree-wtb",
      "earley-boyer",
      "doxbee-promise",
      "doxbee-async",
      "delta-blue",
      "date-format-xparb-SP",
      "date-format-tofte-SP",
      "crypto-sha1-SP",
      "crypto-md5-SP",
      "crypto-aes-SP",
      "crypto",
      "coffeescript-wtb",
      "chai-wtb",
      "cdjs",
      "Box2D",
      "bomb-workers",
      "bigint-paillier",
      "bigint-noble-secp256k1",
      "bigint-noble-ed25519",
      "bigint-noble-bls12-381",
      "bigint-bigdenary",
      "Basic",
      "base64-SP",
      "babylon-wtb",
      "Babylon",
      "async-fs",
      "argon2-wasm-simd",
      "argon2-wasm",
      "Air",
      "ai-astar",
      "acorn-wtb",
      "8bitbench-wasm",
      "3d-raytrace-SP",
      "3d-cube-SP",
  )


class JetStream30Benchmark(JetStream3Benchmark):
  """
  Benchmark runner for JetStream 3.0.
  """

  NAME: str = "jetstream_3.0"
  DEFAULT_STORY_CLS = JetStream30Story
  PROBES: ProbeClsTupleT = (JetStream30Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (3, 0)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("js3", "jetstream_3") + super().aliases()
