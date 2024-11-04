#!/usr/bin/env python
# Copyright 2023 Google LLC
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import argparse
import codecs
import math
import os
import re
import sys
import yaml

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from primes import next_prime
import xngen
import xnncommon


parser = argparse.ArgumentParser(description='PackW microkernel test generator')
parser.add_argument("-s", "--spec", metavar="FILE", required=True,
                    help="Specification (YAML) file")
parser.add_argument("-o", "--output", metavar="FILE", required=True,
                    help='Output (C++ source) file')
parser.set_defaults(defines=list())

def split_ukernel_name(name):
  match = re.fullmatch(r"xnn_(x16|x32)_packw_gemm_goi_ukernel_x(\d+)(c(\d+))?(s(\d+))?__(.+)_x(\d+)", name)
  assert match is not None
  nr = int(match.group(2))
  if match.group(3):
    kr = int(match.group(4))
  else:
    kr = 1
  if match.group(5):
    sr = int(match.group(6))
  else:
    sr = 1
  k_unroll = int(match.group(8))
  arch, isa, assembly = xnncommon.parse_target_name(target_name=match.group(7))
  return nr, kr, sr, k_unroll, arch, isa


PACKW_TEST_TEMPLATE = """\
TEST(${TEST_NAME}, k_eq_${K_UNROLL}) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  PackWMicrokernelTester()
    .n(${NR})
    .k(${K_UNROLL})
    .nr(${NR})
    .kr(${KR})
    .sr(${SR})
    .Test(${", ".join(TEST_ARGS)});
}

$if K_UNROLL > 1:
  TEST(${TEST_NAME}, k_div_${K_UNROLL}) {
    $if ISA_CHECK:
      ${ISA_CHECK};
    PackWMicrokernelTester()
      .n(${NR})
      .k(${K_UNROLL*5})
      .nr(${NR})
      .kr(${KR})
      .sr(${SR})
      .Test(${", ".join(TEST_ARGS)});
  }

  TEST(${TEST_NAME}, k_lt_${K_UNROLL}) {
    $if ISA_CHECK:
      ${ISA_CHECK};
    for (size_t k = 1; k < ${K_UNROLL}; k++) {
      PackWMicrokernelTester()
        .n(${NR})
        .k(k)
        .nr(${NR})
        .kr(${KR})
        .sr(${SR})
        .Test(${", ".join(TEST_ARGS)});
    }
  }

TEST(${TEST_NAME}, k_gt_${K_UNROLL}) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  for (size_t k = ${K_UNROLL+1}; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
    PackWMicrokernelTester()
      .n(${NR})
      .k(k)
      .nr(${NR})
      .kr(${KR})
      .sr(${SR})
      .Test(${", ".join(TEST_ARGS)});
  }
}

TEST(${TEST_NAME}, n_eq_${NR}) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
    PackWMicrokernelTester()
      .n(${NR})
      .k(k)
      .nr(${NR})
      .kr(${KR})
      .sr(${SR})
      .Test(${", ".join(TEST_ARGS)});
  }
}

$if NR > 1:
  TEST(${TEST_NAME}, n_div_${NR}) {
    $if ISA_CHECK:
      ${ISA_CHECK};
    for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
      PackWMicrokernelTester()
        .n(${NR*2})
        .k(k)
        .nr(${NR})
        .kr(${KR})
        .sr(${SR})
        .Test(${", ".join(TEST_ARGS)});
    }
  }

  TEST(${TEST_NAME}, n_lt_${NR}) {
    $if ISA_CHECK:
      ${ISA_CHECK};
    for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
      for (size_t n = 1; n < ${NR}; n++) {
        PackWMicrokernelTester()
          .n(n)
          .k(k)
          .nr(${NR})
          .kr(${KR})
          .sr(${SR})
          .Test(${", ".join(TEST_ARGS)});
      }
    }
  }

TEST(${TEST_NAME}, n_gt_${NR}) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
    for (size_t n = ${NR+1}; n < ${4 if NR == 1 else NR*2}; n++) {
      PackWMicrokernelTester()
        .n(n)
        .k(k)
        .nr(${NR})
        .kr(${KR})
        .sr(${SR})
        .Test(${", ".join(TEST_ARGS)});
    }
  }
}

TEST(${TEST_NAME}, g_gt_1) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  for (size_t g = 2; g <= 3; g++) {
    for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
      for (size_t n = ${NR+1}; n < ${4 if NR == 1 else NR*2}; n++) {
        PackWMicrokernelTester()
          .g(2)
          .n(n)
          .k(k)
          .nr(${NR})
          .kr(${KR})
          .sr(${SR})
          .Test(${", ".join(TEST_ARGS)});
      }
    }
  }
}

TEST(${TEST_NAME}, null_bias) {
  $if ISA_CHECK:
    ${ISA_CHECK};
  for (size_t g = 2; g <= 3; g++) {
    for (size_t k = 1; k < ${4 if K_UNROLL == 1 else K_UNROLL*2}; k++) {
      for (size_t n = ${NR+1}; n < ${4 if NR == 1 else NR*2}; n++) {
        PackWMicrokernelTester()
          .nullbias(true)
          .g(2)
          .n(n)
          .k(k)
          .nr(${NR})
          .kr(${KR})
          .sr(${SR})
          .Test(${", ".join(TEST_ARGS)});
      }
    }
  }
}

"""


def generate_test_cases(ukernel, nr, kr, sr, k_unroll, isa):
  """Generates all tests cases for a PACKW micro-kernel.

  Args:
    ukernel: C name of the micro-kernel function.
    nr: NR parameter of the PACKW micro-kernel.
    kr: KR parameter of the PACKW micro-kernel.
    sr: SR parameter of the PACKW micro-kernel.
    k_unroll: unroll factor along the K dimension.
    isa: instruction set required to run the micro-kernel. Generated unit test
         will skip execution if the host processor doesn't support this ISA.

  Returns:
    Code for the test case.
  """
  _, test_name = ukernel.split("_", 1)
  _, datatype, ukernel_type, _ = ukernel.split("_", 3)
  return xngen.preprocess(PACKW_TEST_TEMPLATE, {
      "TEST_NAME": test_name.upper().replace("UKERNEL_", ""),
      "TEST_ARGS": [ukernel],
      "NR": nr,
      "KR": kr,
      "SR": sr,
      "K_UNROLL": k_unroll,
      "ISA_CHECK": xnncommon.generate_isa_check_macro(isa),
      "next_prime": next_prime,
    })


def main(args):
  options = parser.parse_args(args)

  with codecs.open(options.spec, "r", encoding="utf-8") as spec_file:
    spec_yaml = yaml.safe_load(spec_file)
    if not isinstance(spec_yaml, list):
      raise ValueError("expected a list of micro-kernels in the spec")

    tests = """\
// Copyright 2023 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.
//
// Auto-generated file. Do not edit!
//   Specification: {specification}
//   Generator: {generator}


#include <gtest/gtest.h>

#include <xnnpack/common.h>
#include <xnnpack/isa-checks.h>

#include <xnnpack/packw.h>
#include "packw-microkernel-tester.h"
""".format(specification=options.spec, generator=sys.argv[0])

    for ukernel_spec in spec_yaml:
      name = ukernel_spec["name"]
      nr, kr, sr, k_unroll, arch, isa = split_ukernel_name(name)

      test_case = generate_test_cases(name, nr, kr, sr, k_unroll, isa)
      tests += "\n\n" + xnncommon.postprocess_test_case(test_case, arch, isa)

    txt_changed = True
    if os.path.exists(options.output):
      with codecs.open(options.output, "r", encoding="utf-8") as output_file:
        txt_changed = output_file.read() != tests

    if txt_changed:
      with codecs.open(options.output, "w", encoding="utf-8") as output_file:
        output_file.write(tests)


if __name__ == "__main__":
  main(sys.argv[1:])
