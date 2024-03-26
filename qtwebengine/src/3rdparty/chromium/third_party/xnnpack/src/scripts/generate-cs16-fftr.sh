#!/bin/sh
# Copyright 2022 Google LLC
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

################################### SCALAR ###################################
tools/xngen src/cs16-fftr/scalar.c.in -D SAMPLE_TILE=1 -o src/cs16-fftr/gen/cs16-fftr-scalar-x1.c &
tools/xngen src/cs16-fftr/scalar.c.in -D SAMPLE_TILE=2 -o src/cs16-fftr/gen/cs16-fftr-scalar-x2.c &
tools/xngen src/cs16-fftr/scalar.c.in -D SAMPLE_TILE=4 -o src/cs16-fftr/gen/cs16-fftr-scalar-x4.c &

################################## Unit tests #################################
tools/generate-fftr-test.py --spec test/cs16-fftr.yaml --output test/cs16-fftr.cc &

wait
