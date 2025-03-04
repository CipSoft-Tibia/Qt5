export const description = `
Floating Point unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals, unreachable } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import { FP, FPInterval, FPIntervalParam, IntervalBounds } from '../webgpu/util/floating_point.js';
import {
  reinterpretU16AsF16,
  reinterpretU32AsF32,
  reinterpretU64AsF64,
  map2DArray,
  oneULPF32,
  oneULPF16,
} from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Bounds indicating an expectation of unbounded error */
const kUnboundedBounds: IntervalBounds = [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY];

/** Interval from kUnboundedBounds */
const kUnboundedInterval = {
  f32: FP.f32.toParam(kUnboundedBounds),
  f16: FP.f16.toParam(kUnboundedBounds),
  abstract: FP.abstract.toParam(kUnboundedBounds),
};

/** @returns a number N * ULP greater than the provided number */
const kPlusNULPFunctions = {
  f32: (x: number, n: number) => {
    return x + n * oneULPF32(x);
  },
  f16: (x: number, n: number) => {
    return x + n * oneULPF16(x);
  },
};

/** @returns a number one ULP greater than the provided number */
const kPlusOneULPFunctions = {
  f32: (x: number): number => {
    return kPlusNULPFunctions['f32'](x, 1);
  },
  f16: (x: number): number => {
    return kPlusNULPFunctions['f16'](x, 1);
  },
};

/** @returns a number N * ULP less than the provided number */
const kMinusNULPFunctions = {
  f32: (x: number, n: number) => {
    return x - n * oneULPF32(x);
  },
  f16: (x: number, n: number) => {
    return x - n * oneULPF16(x);
  },
};

/** @returns a number one ULP less than the provided number */
const kMinusOneULPFunctions = {
  f32: (x: number): number => {
    return kMinusNULPFunctions['f32'](x, 1);
  },
  f16: (x: number): number => {
    return kMinusNULPFunctions['f16'](x, 1);
  },
};

/** @returns the expected IntervalBounds adjusted by the given error function
 *
 * @param expected the bounds to be adjusted
 * @param error error function to adjust the bounds via
 */
function applyError(
  expected: number | IntervalBounds,
  error: (n: number) => number
): IntervalBounds {
  // Avoiding going through FPInterval to avoid tying this to a specific kind
  const unpack = (n: number | IntervalBounds): [number, number] => {
    if (expected instanceof Array) {
      switch (expected.length) {
        case 1:
          return [expected[0], expected[0]];
        case 2:
          return [expected[0], expected[1]];
      }
      unreachable(`Tried to unpack an IntervalBounds with length other than 1 or 2`);
    } else {
      // TS doesn't narrow this to number automatically
      return [n as number, n as number];
    }
  };

  let [begin, end] = unpack(expected);

  begin -= error(begin);
  end += error(end);

  if (begin === end) {
    return [begin];
  }
  return [begin, end];
}

// FPInterval

interface ConstructorCase {
  input: IntervalBounds;
  expected: IntervalBounds;
}

g.test('constructor')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<ConstructorCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        const cases: ConstructorCase[] = [
          // Common cases
          { input: [0, 10], expected: [0, 10] },
          { input: [-5, 0], expected: [-5, 0] },
          { input: [-5, 10], expected: [-5, 10] },
          { input: [0], expected: [0] },
          { input: [10], expected: [10] },
          { input: [-5], expected: [-5] },
          { input: [2.5], expected: [2.5] },
          { input: [-1.375], expected: [-1.375] },
          { input: [-1.375, 2.5], expected: [-1.375, 2.5] },

          // Edges
          { input: [0, constants.positive.max], expected: [0, constants.positive.max] },
          { input: [constants.negative.min, 0], expected: [constants.negative.min, 0] },
          { input: [constants.negative.min, constants.positive.max], expected: [constants.negative.min, constants.positive.max] },

          // Infinities
          { input: [0, constants.positive.infinity], expected: [0, Number.POSITIVE_INFINITY] },
          { input: [constants.negative.infinity, 0], expected: [Number.NEGATIVE_INFINITY, 0] },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
        ];

        // Note: Out of range values are limited to infinities for abstract float, due to abstract
        // float and 'number' both being f64. So there are no separate OOR tests for abstract float,
        // otherwise the testing framework will consider them duplicated.
        if (p.trait !== 'abstract') {
          // prettier-ignore
          cases.push(...[
            // Out of range
            { input: [0, 2 * constants.positive.max], expected: [0, 2 * constants.positive.max] },
            { input: [2 * constants.negative.min, 0], expected: [2 * constants.negative.min, 0] },
            { input: [2 * constants.negative.min, 2 * constants.positive.max], expected: [2 * constants.negative.min, 2 * constants.positive.max] },
          ] as ConstructorCase[]);
        }

        return cases;
      })
  )
  .fn(t => {
    const i = new FPInterval(t.params.trait, ...t.params.input);
    t.expect(
      objectEquals(i.bounds(), t.params.expected),
      `new FPInterval('${t.params.trait}', [${t.params.input}]) returned ${i}. Expected [${t.params.expected}]`
    );
  });

interface ContainsNumberCase {
  bounds: number | IntervalBounds;
  value: number;
  expected: boolean;
}

g.test('contains_number')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<ContainsNumberCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        const cases: ContainsNumberCase[] = [
          // Common usage
          { bounds: [0, 10], value: 0, expected: true },
          { bounds: [0, 10], value: 10, expected: true },
          { bounds: [0, 10], value: 5, expected: true },
          { bounds: [0, 10], value: -5, expected: false },
          { bounds: [0, 10], value: 50, expected: false },
          { bounds: [0, 10], value: Number.NaN, expected: false },
          { bounds: [-5, 10], value: 0, expected: true },
          { bounds: [-5, 10], value: 10, expected: true },
          { bounds: [-5, 10], value: 5, expected: true },
          { bounds: [-5, 10], value: -5, expected: true },
          { bounds: [-5, 10], value: -6, expected: false },
          { bounds: [-5, 10], value: 50, expected: false },
          { bounds: [-5, 10], value: -10, expected: false },
          { bounds: [-1.375, 2.5], value: -10, expected: false },
          { bounds: [-1.375, 2.5], value: 0.5, expected: true },
          { bounds: [-1.375, 2.5], value: 10, expected: false },

          // Point
          { bounds: 0, value: 0, expected: true },
          { bounds: 0, value: 10, expected: false },
          { bounds: 0, value: -1000, expected: false },
          { bounds: 10, value: 10, expected: true },
          { bounds: 10, value: 0, expected: false },
          { bounds: 10, value: -10, expected: false },
          { bounds: 10, value: 11, expected: false },

          // Upper infinity
          { bounds: [0, constants.positive.infinity], value: constants.positive.min, expected: true },
          { bounds: [0, constants.positive.infinity], value: constants.positive.max, expected: true },
          { bounds: [0, constants.positive.infinity], value: constants.positive.infinity, expected: true },
          { bounds: [0, constants.positive.infinity], value: constants.negative.min, expected: false },
          { bounds: [0, constants.positive.infinity], value: constants.negative.max, expected: false },
          { bounds: [0, constants.positive.infinity], value: constants.negative.infinity, expected: false },

          // Lower infinity
          { bounds: [constants.negative.infinity, 0], value: constants.positive.min, expected: false },
          { bounds: [constants.negative.infinity, 0], value: constants.positive.max, expected: false },
          { bounds: [constants.negative.infinity, 0], value: constants.positive.infinity, expected: false },
          { bounds: [constants.negative.infinity, 0], value: constants.negative.min, expected: true },
          { bounds: [constants.negative.infinity, 0], value: constants.negative.max, expected: true },
          { bounds: [constants.negative.infinity, 0], value: constants.negative.infinity, expected: true },

          // Full infinity
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.positive.min, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.positive.max, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.positive.infinity, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.negative.min, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.negative.max, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: constants.negative.infinity, expected: true },
          { bounds: [constants.negative.infinity, constants.positive.infinity], value: Number.NaN, expected: true },

          // Maximum f32 boundary
          { bounds: [0, constants.positive.max], value: constants.positive.min, expected: true },
          { bounds: [0, constants.positive.max], value: constants.positive.max, expected: true },
          { bounds: [0, constants.positive.max], value: constants.positive.infinity, expected: false },
          { bounds: [0, constants.positive.max], value: constants.negative.min, expected: false },
          { bounds: [0, constants.positive.max], value: constants.negative.max, expected: false },
          { bounds: [0, constants.positive.max], value: constants.negative.infinity, expected: false },

          // Minimum f32 boundary
          { bounds: [constants.negative.min, 0], value: constants.positive.min, expected: false },
          { bounds: [constants.negative.min, 0], value: constants.positive.max, expected: false },
          { bounds: [constants.negative.min, 0], value: constants.positive.infinity, expected: false },
          { bounds: [constants.negative.min, 0], value: constants.negative.min, expected: true },
          { bounds: [constants.negative.min, 0], value: constants.negative.max, expected: true },
          { bounds: [constants.negative.min, 0], value: constants.negative.infinity, expected: false },

          // Subnormals
          { bounds: [0, constants.positive.min], value: constants.positive.subnormal.min, expected: true },
          { bounds: [0, constants.positive.min], value: constants.positive.subnormal.max, expected: true },
          { bounds: [0, constants.positive.min], value: constants.negative.subnormal.min, expected: false },
          { bounds: [0, constants.positive.min], value: constants.negative.subnormal.max, expected: false },
          { bounds: [constants.negative.max, 0], value: constants.positive.subnormal.min, expected: false },
          { bounds: [constants.negative.max, 0], value: constants.positive.subnormal.max, expected: false },
          { bounds: [constants.negative.max, 0], value: constants.negative.subnormal.min, expected: true },
          { bounds: [constants.negative.max, 0], value: constants.negative.subnormal.max, expected: true },
          { bounds: [0, constants.positive.subnormal.min], value: constants.positive.subnormal.min, expected: true },
          { bounds: [0, constants.positive.subnormal.min], value: constants.positive.subnormal.max, expected: false },
          { bounds: [0, constants.positive.subnormal.min], value: constants.negative.subnormal.min, expected: false },
          { bounds: [0, constants.positive.subnormal.min], value: constants.negative.subnormal.max, expected: false },
          { bounds: [constants.negative.subnormal.max, 0], value: constants.positive.subnormal.min, expected: false },
          { bounds: [constants.negative.subnormal.max, 0], value: constants.positive.subnormal.max, expected: false },
          { bounds: [constants.negative.subnormal.max, 0], value: constants.negative.subnormal.min, expected: false },
          { bounds: [constants.negative.subnormal.max, 0], value: constants.negative.subnormal.max, expected: true },
        ];

        // Note: Out of range values are limited to infinities for abstract float, due to abstract
        // float and 'number' both being f64. So there are no separate OOR tests for abstract float,
        // otherwise the testing framework will consider them duplicated.
        if (p.trait !== 'abstract') {
          // prettier-ignore
          cases.push(...[
            // Out of range high
            { bounds: [0, 2 * constants.positive.max], value: constants.positive.min, expected: true },
            { bounds: [0, 2 * constants.positive.max], value: constants.positive.max, expected: true },
            { bounds: [0, 2 * constants.positive.max], value: constants.positive.infinity, expected: false },
            { bounds: [0, 2 * constants.positive.max], value: constants.negative.min, expected: false },
            { bounds: [0, 2 * constants.positive.max], value: constants.negative.max, expected: false },
            { bounds: [0, 2 * constants.positive.max], value: constants.negative.infinity, expected: false },

            // Out of range low
            { bounds: [2 * constants.negative.min, 0], value: constants.positive.min, expected: false },
            { bounds: [2 * constants.negative.min, 0], value: constants.positive.max, expected: false },
            { bounds: [2 * constants.negative.min, 0], value: constants.positive.infinity, expected: false },
            { bounds: [2 * constants.negative.min, 0], value: constants.negative.min, expected: true },
            { bounds: [2 * constants.negative.min, 0], value: constants.negative.max, expected: true },
            { bounds: [2 * constants.negative.min, 0], value: constants.negative.infinity, expected: false },
          ] as ContainsNumberCase[]);
        }

        return cases;
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const i = trait.toInterval(t.params.bounds);
    const value = t.params.value;
    const expected = t.params.expected;

    const got = i.contains(value);
    t.expect(expected === got, `${i}.contains(${value}) returned ${got}. Expected ${expected}`);
  });

interface ContainsIntervalCase {
  lhs: number | IntervalBounds;
  rhs: number | IntervalBounds;
  expected: boolean;
}

g.test('contains_interval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<ContainsIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        const cases: ContainsIntervalCase[] = [
          // Common usage
          { lhs: [-10, 10], rhs: 0, expected: true },
          { lhs: [-10, 10], rhs: [-1, 0], expected: true },
          { lhs: [-10, 10], rhs: [0, 2], expected: true },
          { lhs: [-10, 10], rhs: [-1, 2], expected: true },
          { lhs: [-10, 10], rhs: [0, 10], expected: true },
          { lhs: [-10, 10], rhs: [-10, 2], expected: true },
          { lhs: [-10, 10], rhs: [-10, 10], expected: true },
          { lhs: [-10, 10], rhs: [-100, 10], expected: false },

          // Upper infinity
          { lhs: [0, constants.positive.infinity], rhs: 0, expected: true },
          { lhs: [0, constants.positive.infinity], rhs: [-1, 0], expected: false },
          { lhs: [0, constants.positive.infinity], rhs: [0, 1], expected: true },
          { lhs: [0, constants.positive.infinity], rhs: [0, constants.positive.max], expected: true },
          { lhs: [0, constants.positive.infinity], rhs: [0, constants.positive.infinity], expected: true },
          { lhs: [0, constants.positive.infinity], rhs: [100, constants.positive.infinity], expected: true },
          { lhs: [0, constants.positive.infinity], rhs: [Number.NEGATIVE_INFINITY, constants.positive.infinity], expected: false },

          // Lower infinity
          { lhs: [constants.negative.infinity, 0], rhs: 0, expected: true },
          { lhs: [constants.negative.infinity, 0], rhs: [-1, 0], expected: true },
          { lhs: [constants.negative.infinity, 0], rhs: [constants.negative.min, 0], expected: true },
          { lhs: [constants.negative.infinity, 0], rhs: [0, 1], expected: false },
          { lhs: [constants.negative.infinity, 0], rhs: [constants.negative.infinity, 0], expected: true },
          { lhs: [constants.negative.infinity, 0], rhs: [constants.negative.infinity, -100 ], expected: true },
          { lhs: [constants.negative.infinity, 0], rhs: [constants.negative.infinity, constants.positive.infinity], expected: false },

          // Full infinity
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: 0, expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [-1, 0], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [0, 1], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [0, constants.positive.infinity], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [100, constants.positive.infinity], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [constants.negative.infinity, 0], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [constants.negative.infinity, -100 ], expected: true },
          { lhs: [constants.negative.infinity, constants.positive.infinity], rhs: [constants.negative.infinity, constants.positive.infinity], expected: true },

          // Maximum boundary
          { lhs: [0, constants.positive.max], rhs: 0, expected: true },
          { lhs: [0, constants.positive.max], rhs: [-1, 0], expected: false },
          { lhs: [0, constants.positive.max], rhs: [0, 1], expected: true },
          { lhs: [0, constants.positive.max], rhs: [0, constants.positive.max], expected: true },
          { lhs: [0, constants.positive.max], rhs: [0, constants.positive.infinity], expected: false },
          { lhs: [0, constants.positive.max], rhs: [100, constants.positive.infinity], expected: false },
          { lhs: [0, constants.positive.max], rhs: [constants.negative.infinity, constants.positive.infinity], expected: false },

          // Minimum boundary
          { lhs: [constants.negative.min, 0], rhs: [0, 0], expected: true },
          { lhs: [constants.negative.min, 0], rhs: [-1, 0], expected: true },
          { lhs: [constants.negative.min, 0], rhs: [constants.negative.min, 0], expected: true },
          { lhs: [constants.negative.min, 0], rhs: [0, 1], expected: false },
          { lhs: [constants.negative.min, 0], rhs: [constants.negative.infinity, 0], expected: false },
          { lhs: [constants.negative.min, 0], rhs: [constants.negative.infinity, -100 ], expected: false },
          { lhs: [constants.negative.min, 0], rhs: [constants.negative.infinity, constants.positive.infinity], expected: false },
        ];

        // Note: Out of range values are limited to infinities for abstract float, due to abstract
        // float and 'number' both being f64. So there are no separate OOR tests for abstract float,
        // otherwise the testing framework will consider them duplicated.
        if (p.trait !== 'abstract') {
          // prettier-ignore
          cases.push(...[
            // Out of range high
            { lhs: [0, 2 * constants.positive.max], rhs: 0, expected: true },
            { lhs: [0, 2 * constants.positive.max], rhs: [-1, 0], expected: false },
            { lhs: [0, 2 * constants.positive.max], rhs: [0, 1], expected: true },
            { lhs: [0, 2 * constants.positive.max], rhs: [0, constants.positive.max], expected: true },
            { lhs: [0, 2 * constants.positive.max], rhs: [0, constants.positive.infinity], expected: false },
            { lhs: [0, 2 * constants.positive.max], rhs: [100, constants.positive.infinity], expected: false },
            { lhs: [0, 2 * constants.positive.max], rhs: [constants.negative.infinity, constants.positive.infinity], expected: false },

            // Out of range low
            { lhs: [2 * constants.negative.min, 0], rhs: 0, expected: true },
            { lhs: [2 * constants.negative.min, 0], rhs: [-1, 0], expected: true },
            { lhs: [2 * constants.negative.min, 0], rhs: [constants.negative.min, 0], expected: true },
            { lhs: [2 * constants.negative.min, 0], rhs: [0, 1], expected: false },
            { lhs: [2 * constants.negative.min, 0], rhs: [constants.negative.infinity, 0], expected: false },
            { lhs: [2 * constants.negative.min, 0], rhs: [constants.negative.infinity, -100 ], expected: false },
            { lhs: [2 * constants.negative.min, 0], rhs: [constants.negative.infinity, constants.positive.infinity], expected: false },
          ] as ContainsIntervalCase[]);
        }

        return cases;
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const lhs = trait.toInterval(t.params.lhs);
    const rhs = trait.toInterval(t.params.rhs);
    const expected = t.params.expected;

    const got = lhs.contains(rhs);
    t.expect(expected === got, `${lhs}.contains(${rhs}) returned ${got}. Expected ${expected}`);
  });

// Utilities

interface SpanIntervalsCase {
  intervals: (number | IntervalBounds)[];
  expected: number | IntervalBounds;
}

g.test('spanIntervals')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<SpanIntervalsCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          // Single Intervals
          { intervals: [[0, 10]], expected: [0, 10] },
          { intervals: [[0, constants.positive.max]], expected: [0, constants.positive.max] },
          { intervals: [[0, constants.positive.nearest_max]], expected: [0, constants.positive.nearest_max] },
          { intervals: [[0, constants.positive.infinity]], expected: [0, Number.POSITIVE_INFINITY] },
          { intervals: [[constants.negative.min, 0]], expected: [constants.negative.min, 0] },
          { intervals: [[constants.negative.nearest_min, 0]], expected: [constants.negative.nearest_min, 0] },
          { intervals: [[constants.negative.infinity, 0]], expected: [Number.NEGATIVE_INFINITY, 0] },

          // Double Intervals
          { intervals: [[0, 1], [2, 5]], expected: [0, 5] },
          { intervals: [[2, 5], [0, 1]], expected: [0, 5] },
          { intervals: [[0, 2], [1, 5]], expected: [0, 5] },
          { intervals: [[0, 5], [1, 2]], expected: [0, 5] },
          { intervals: [[constants.negative.infinity, 0], [0, constants.positive.infinity]], expected: kUnboundedBounds },

          // Multiple Intervals
          { intervals: [[0, 1], [2, 3], [4, 5]], expected: [0, 5] },
          { intervals: [[0, 1], [4, 5], [2, 3]], expected: [0, 5] },
          { intervals: [[0, 1], [0, 1], [0, 1]], expected: [0, 1] },

          // Point Intervals
          { intervals: [1], expected: 1 },
          { intervals: [1, 2], expected: [1, 2] },
          { intervals: [-10, 2], expected: [-10, 2] },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const intervals = t.params.intervals.map(i => trait.toInterval(i));
    const expected = trait.toInterval(t.params.expected);

    const got = trait.spanIntervals(...intervals);
    t.expect(
      objectEquals(got, expected),
      `${t.params.trait}.span({${intervals}}) returned ${got}. Expected ${expected}`
    );
  });

interface isVectorCase {
  input: (number | IntervalBounds | FPIntervalParam)[];
  expected: boolean;
}

g.test('isVector')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<isVectorCase>(p => {
        const trait = FP[p.trait];
        return [
          // numbers
          { input: [1, 2], expected: false },
          { input: [1, 2, 3], expected: false },
          { input: [1, 2, 3, 4], expected: false },

          // IntervalBounds
          { input: [[1], [2]], expected: false },
          { input: [[1], [2], [3]], expected: false },
          { input: [[1], [2], [3], [4]], expected: false },
          {
            input: [
              [1, 2],
              [2, 3],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2],
              [2, 3],
              [3, 4],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2],
              [2, 3],
              [3, 4],
              [4, 5],
            ],
            expected: false,
          },

          // FPInterval, valid dimensions
          { input: [trait.toParam([1]), trait.toParam([2])], expected: true },
          { input: [trait.toParam([1, 2]), trait.toParam([2, 3])], expected: true },
          {
            input: [trait.toParam([1]), trait.toParam([2]), trait.toParam([3])],
            expected: true,
          },
          {
            input: [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
            expected: true,
          },
          {
            input: [trait.toParam([1]), trait.toParam([2]), trait.toParam([3]), trait.toParam([4])],
            expected: true,
          },
          {
            input: [
              trait.toParam([1, 2]),
              trait.toParam([2, 3]),
              trait.toParam([3, 4]),
              trait.toParam([4, 5]),
            ],
            expected: true,
          },

          // FPInterval, invalid dimensions
          { input: [trait.toParam([1])], expected: false },
          {
            input: [
              trait.toParam([1]),
              trait.toParam([2]),
              trait.toParam([3]),
              trait.toParam([4]),
              trait.toParam([5]),
            ],
            expected: false,
          },

          // Mixed
          { input: [1, [2]], expected: false },
          { input: [1, [2], trait.toParam([3])], expected: false },
          { input: [1, trait.toParam([2]), [3], 4], expected: false },
          { input: [trait.toParam(1), 2], expected: false },
          { input: [trait.toParam(1), [2]], expected: false },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const input = t.params.input.map(e => trait.fromParam(e));
    const expected = t.params.expected;

    const got = trait.isVector(input);
    t.expect(
      got === expected,
      `${t.params.trait}.isVector([${input}]) returned ${got}. Expected ${expected}`
    );
  });

interface toVectorCase {
  input: (number | IntervalBounds | FPIntervalParam)[];
  expected: (number | IntervalBounds)[];
}

g.test('toVector')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<toVectorCase>(p => {
        const trait = FP[p.trait];
        return [
          // numbers
          { input: [1, 2], expected: [1, 2] },
          { input: [1, 2, 3], expected: [1, 2, 3] },
          { input: [1, 2, 3, 4], expected: [1, 2, 3, 4] },

          // IntervalBounds
          { input: [[1], [2]], expected: [1, 2] },
          { input: [[1], [2], [3]], expected: [1, 2, 3] },
          { input: [[1], [2], [3], [4]], expected: [1, 2, 3, 4] },
          {
            input: [
              [1, 2],
              [2, 3],
            ],
            expected: [
              [1, 2],
              [2, 3],
            ],
          },
          {
            input: [
              [1, 2],
              [2, 3],
              [3, 4],
            ],
            expected: [
              [1, 2],
              [2, 3],
              [3, 4],
            ],
          },
          {
            input: [
              [1, 2],
              [2, 3],
              [3, 4],
              [4, 5],
            ],
            expected: [
              [1, 2],
              [2, 3],
              [3, 4],
              [4, 5],
            ],
          },

          // FPInterval
          { input: [trait.toParam([1]), trait.toParam([2])], expected: [1, 2] },
          {
            input: [trait.toParam([1, 2]), trait.toParam([2, 3])],
            expected: [
              [1, 2],
              [2, 3],
            ],
          },
          {
            input: [trait.toParam([1]), trait.toParam([2]), trait.toParam([3])],
            expected: [1, 2, 3],
          },
          {
            input: [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
            expected: [
              [1, 2],
              [2, 3],
              [3, 4],
            ],
          },
          {
            input: [trait.toParam([1]), trait.toParam([2]), trait.toParam([3]), trait.toParam([4])],
            expected: [1, 2, 3, 4],
          },
          {
            input: [
              trait.toParam([1, 2]),
              trait.toParam([2, 3]),
              trait.toParam([3, 4]),
              trait.toParam([4, 5]),
            ],
            expected: [
              [1, 2],
              [2, 3],
              [3, 4],
              [4, 5],
            ],
          },

          // Mixed
          { input: [1, [2]], expected: [1, 2] },
          { input: [1, [2], trait.toParam([3])], expected: [1, 2, 3] },
          { input: [1, trait.toParam([2]), [3], 4], expected: [1, 2, 3, 4] },
          {
            input: [1, [2], [2, 3], kUnboundedInterval[p.trait]],
            expected: [1, 2, [2, 3], kUnboundedBounds],
          },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const input = t.params.input.map(e => trait.fromParam(e));
    const expected = t.params.expected.map(e => trait.toInterval(e));

    const got = trait.toVector(input);
    t.expect(
      objectEquals(got, expected),
      `${t.params.trait}.toVector([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface isMatrixCase {
  input: (number | IntervalBounds | FPIntervalParam)[][];
  expected: boolean;
}

g.test('isMatrix')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<isMatrixCase>(p => {
        const trait = FP[p.trait];
        return [
          // numbers
          {
            input: [
              [1, 2],
              [3, 4],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2],
              [3, 4],
              [5, 6],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2],
              [3, 4],
              [5, 6],
              [7, 8],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
              [10, 11, 12],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
              [13, 14, 15, 16],
            ],
            expected: false,
          },

          // IntervalBounds
          {
            input: [
              [[1], [2]],
              [[3], [4]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2]],
              [[3], [4]],
              [[5], [6]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2]],
              [[3], [4]],
              [[5], [6]],
              [[7], [8]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
              [[7], [8], [9]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
              [[7], [8], [9]],
              [[10], [11], [12]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
              [[9], [10], [11], [12]],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
              [[9], [10], [11], [12]],
              [[13], [14], [15], [16]],
            ],
            expected: false,
          },

          // FPInterval, valid dimensions
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8), trait.toParam(9)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8), trait.toParam(9)],
              [trait.toParam(10), trait.toParam(11), trait.toParam(12)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
              [trait.toParam(9), trait.toParam(10), trait.toParam(11), trait.toParam(12)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
              [trait.toParam(9), trait.toParam(10), trait.toParam(11), trait.toParam(12)],
              [trait.toParam(13), trait.toParam(14), trait.toParam(15), trait.toParam(16)],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
              [trait.toParam([5, 6]), trait.toParam([6, 7])],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
              [trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9])],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9]), trait.toParam([9, 10])],
            ],
            expected: true,
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9]), trait.toParam([9, 10])],
              [trait.toParam([10, 11]), trait.toParam([11, 12]), trait.toParam([12, 13])],
            ],
            expected: true,
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
            ],
            expected: true,
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
              [
                trait.toParam([9, 10]),
                trait.toParam([10, 11]),
                trait.toParam([11, 12]),
                trait.toParam([12, 13]),
              ],
            ],
            expected: true,
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
              [
                trait.toParam([9, 10]),
                trait.toParam([10, 11]),
                trait.toParam([11, 12]),
                trait.toParam([12, 13]),
              ],
              [
                trait.toParam([13, 14]),
                trait.toParam([14, 15]),
                trait.toParam([15, 16]),
                trait.toParam([16, 17]),
              ],
            ],
            expected: true,
          },

          // FPInterval, invalid dimensions
          { input: [[trait.toParam(1)]], expected: false },
          {
            input: [[trait.toParam(1)], [trait.toParam(3), trait.toParam(4)]],
            expected: false,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4), trait.toParam(5)],
            ],
            expected: false,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5)],
            ],
            expected: false,
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8)],
              [trait.toParam(9), trait.toParam(10)],
            ],
            expected: false,
          },

          // Mixed
          {
            input: [
              [1, [2]],
              [3, 4],
            ],
            expected: false,
          },
          {
            input: [
              [[1], [2]],
              [[3], 4],
            ],
            expected: false,
          },
          {
            input: [
              [1, 2],
              [trait.toParam([3]), 4],
            ],
            expected: false,
          },
          {
            input: [
              [[1], trait.toParam([2])],
              [trait.toParam([3]), trait.toParam([4])],
            ],
            expected: false,
          },
          {
            input: [
              [trait.toParam(1), [2]],
              [3, 4],
            ],
            expected: false,
          },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const input = t.params.input.map(a => a.map(e => trait.fromParam(e)));
    const expected = t.params.expected;

    const got = trait.isMatrix(input);
    t.expect(
      got === expected,
      `${t.params.trait}.isMatrix([${input}]) returned ${got}. Expected ${expected}`
    );
  });

interface toMatrixCase {
  input: (number | IntervalBounds | FPIntervalParam)[][];
  expected: (number | IntervalBounds)[][];
}

g.test('toMatrix')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16', 'abstract'] as const)
      .beginSubcases()
      .expandWithParams<toMatrixCase>(p => {
        const trait = FP[p.trait];
        return [
          // numbers
          {
            input: [
              [1, 2],
              [3, 4],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [1, 2],
              [3, 4],
              [5, 6],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
            ],
          },
          {
            input: [
              [1, 2],
              [3, 4],
              [5, 6],
              [7, 8],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
              [7, 8],
            ],
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
            ],
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
            ],
          },
          {
            input: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
              [10, 11, 12],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
              [10, 11, 12],
            ],
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
            ],
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
            ],
          },
          {
            input: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
              [13, 14, 15, 16],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
              [13, 14, 15, 16],
            ],
          },

          // IntervalBounds
          {
            input: [
              [[1], [2]],
              [[3], [4]],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [[1], [2]],
              [[3], [4]],
              [[5], [6]],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
            ],
          },
          {
            input: [
              [[1], [2]],
              [[3], [4]],
              [[5], [6]],
              [[7], [8]],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
              [7, 8],
            ],
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
            ],
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
              [[7], [8], [9]],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
            ],
          },
          {
            input: [
              [[1], [2], [3]],
              [[4], [5], [6]],
              [[7], [8], [9]],
              [[10], [11], [12]],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
              [10, 11, 12],
            ],
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
            ],
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
              [[9], [10], [11], [12]],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
            ],
          },
          {
            input: [
              [[1], [2], [3], [4]],
              [[5], [6], [7], [8]],
              [[9], [10], [11], [12]],
              [[13], [14], [15], [16]],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
              [13, 14, 15, 16],
            ],
          },

          // FPInterval
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6)],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2)],
              [trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8)],
            ],
            expected: [
              [1, 2],
              [3, 4],
              [5, 6],
              [7, 8],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8), trait.toParam(9)],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3)],
              [trait.toParam(4), trait.toParam(5), trait.toParam(6)],
              [trait.toParam(7), trait.toParam(8), trait.toParam(9)],
              [trait.toParam(10), trait.toParam(11), trait.toParam(12)],
            ],
            expected: [
              [1, 2, 3],
              [4, 5, 6],
              [7, 8, 9],
              [10, 11, 12],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
              [trait.toParam(9), trait.toParam(10), trait.toParam(11), trait.toParam(12)],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
            ],
          },
          {
            input: [
              [trait.toParam(1), trait.toParam(2), trait.toParam(3), trait.toParam(4)],
              [trait.toParam(5), trait.toParam(6), trait.toParam(7), trait.toParam(8)],
              [trait.toParam(9), trait.toParam(10), trait.toParam(11), trait.toParam(12)],
              [trait.toParam(13), trait.toParam(14), trait.toParam(15), trait.toParam(16)],
            ],
            expected: [
              [1, 2, 3, 4],
              [5, 6, 7, 8],
              [9, 10, 11, 12],
              [13, 14, 15, 16],
            ],
          },

          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
              ],
              [
                [3, 4],
                [4, 5],
              ],
            ],
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
              [trait.toParam([5, 6]), trait.toParam([6, 7])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
              ],
              [
                [3, 4],
                [4, 5],
              ],
              [
                [5, 6],
                [6, 7],
              ],
            ],
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3])],
              [trait.toParam([3, 4]), trait.toParam([4, 5])],
              [trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
              ],
              [
                [3, 4],
                [4, 5],
              ],
              [
                [5, 6],
                [6, 7],
              ],
              [
                [7, 8],
                [8, 9],
              ],
            ],
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
              ],
              [
                [4, 5],
                [5, 6],
                [6, 7],
              ],
            ],
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9]), trait.toParam([9, 10])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
              ],
              [
                [4, 5],
                [5, 6],
                [6, 7],
              ],
              [
                [7, 8],
                [8, 9],
                [9, 10],
              ],
            ],
          },
          {
            input: [
              [trait.toParam([1, 2]), trait.toParam([2, 3]), trait.toParam([3, 4])],
              [trait.toParam([4, 5]), trait.toParam([5, 6]), trait.toParam([6, 7])],
              [trait.toParam([7, 8]), trait.toParam([8, 9]), trait.toParam([9, 10])],
              [trait.toParam([10, 11]), trait.toParam([11, 12]), trait.toParam([12, 13])],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
              ],
              [
                [4, 5],
                [5, 6],
                [6, 7],
              ],
              [
                [7, 8],
                [8, 9],
                [9, 10],
              ],
              [
                [10, 11],
                [11, 12],
                [12, 13],
              ],
            ],
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
                [4, 5],
              ],
              [
                [5, 6],
                [6, 7],
                [7, 8],
                [8, 9],
              ],
            ],
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
              [
                trait.toParam([9, 10]),
                trait.toParam([10, 11]),
                trait.toParam([11, 12]),
                trait.toParam([12, 13]),
              ],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
                [4, 5],
              ],
              [
                [5, 6],
                [6, 7],
                [7, 8],
                [8, 9],
              ],
              [
                [9, 10],
                [10, 11],
                [11, 12],
                [12, 13],
              ],
            ],
          },
          {
            input: [
              [
                trait.toParam([1, 2]),
                trait.toParam([2, 3]),
                trait.toParam([3, 4]),
                trait.toParam([4, 5]),
              ],
              [
                trait.toParam([5, 6]),
                trait.toParam([6, 7]),
                trait.toParam([7, 8]),
                trait.toParam([8, 9]),
              ],
              [
                trait.toParam([9, 10]),
                trait.toParam([10, 11]),
                trait.toParam([11, 12]),
                trait.toParam([12, 13]),
              ],
              [
                trait.toParam([13, 14]),
                trait.toParam([14, 15]),
                trait.toParam([15, 16]),
                trait.toParam([16, 17]),
              ],
            ],
            expected: [
              [
                [1, 2],
                [2, 3],
                [3, 4],
                [4, 5],
              ],
              [
                [5, 6],
                [6, 7],
                [7, 8],
                [8, 9],
              ],
              [
                [9, 10],
                [10, 11],
                [11, 12],
                [12, 13],
              ],
              [
                [13, 14],
                [14, 15],
                [15, 16],
                [16, 17],
              ],
            ],
          },

          // Mixed
          {
            input: [
              [1, [2]],
              [3, 4],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [[1], [2]],
              [[3], 4],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [1, 2],
              [trait.toParam([3]), 4],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
          {
            input: [
              [[1], trait.toParam([2])],
              [trait.toParam([3]), trait.toParam([4])],
            ],
            expected: [
              [1, 2],
              [3, 4],
            ],
          },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const input = map2DArray(t.params.input, e => trait.fromParam(e));
    const expected = map2DArray(t.params.expected, e => trait.toInterval(e));

    const got = trait.toMatrix(input);
    t.expect(
      objectEquals(got, expected),
      `${t.params.trait}.toMatrix([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

// API - Fundamental Error Intervals

interface AbsoluteErrorCase {
  value: number;
  error: number;
  expected: number | IntervalBounds;
}

// Special values used for testing absolute error interval
// A small absolute error value is a representable value x that much smaller than 1.0,
// but 1.0 +/- x is still exactly representable.
const kSmallAbsoluteErrorValue = {
  f32: 2 ** -11, // Builtin cos and sin has a absolute error 2**-11 for f32
  f16: 2 ** -7, // Builtin cos and sin has a absolute error 2**-7 for f16
} as const;
// A large absolute error value is a representable value x that much smaller than maximum
// positive, but positive.max - x is still exactly representable.
const kLargeAbsoluteErrorValue = {
  f32: 2 ** 110, // f32.positive.max - 2**110 = 3.4028104e+38 = 0x7f7fffbf in f32
  f16: 2 ** 10, // f16.positive.max - 2**10 = 64480 = 0x7bdf in f16
} as const;
// A subnormal absolute error value is a subnormal representable value x of kind, which ensures
// that positive.subnormal.min +/- x is still exactly representable.
const kSubnormalAbsoluteErrorValue = {
  f32: 2 ** -140, // f32 0x00000200
  f16: 2 ** -20, // f16 0x0010
} as const;

g.test('absoluteErrorInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<AbsoluteErrorCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        const smallErr = kSmallAbsoluteErrorValue[p.trait];
        const largeErr = kLargeAbsoluteErrorValue[p.trait];
        const subnormalErr = kSubnormalAbsoluteErrorValue[p.trait];
        // prettier-ignore
        return [
          // Edge Cases
          // 1. Interval around infinity would be kUnboundedBounds
          { value: constants.positive.infinity, error: 0, expected: kUnboundedBounds },
          { value: constants.positive.infinity, error: largeErr, expected: kUnboundedBounds },
          { value: constants.positive.infinity, error: 1, expected: kUnboundedBounds },
          { value: constants.negative.infinity, error: 0, expected: kUnboundedBounds },
          { value: constants.negative.infinity, error: largeErr, expected: kUnboundedBounds },
          { value: constants.negative.infinity, error: 1, expected: kUnboundedBounds },
          // 2. Interval around largest finite positive/negative
          { value: constants.positive.max, error: 0, expected: constants.positive.max },
          { value: constants.positive.max, error: largeErr, expected: kUnboundedBounds},
          { value: constants.positive.max, error: constants.positive.max, expected: kUnboundedBounds},
          { value: constants.negative.min, error: 0, expected: constants.negative.min },
          { value: constants.negative.min, error: largeErr, expected: kUnboundedBounds},
          { value: constants.negative.min, error: constants.positive.max, expected: kUnboundedBounds},
          // 3. Interval around small but normal center, center should not get flushed.
          { value: constants.positive.min, error: 0, expected: constants.positive.min },
          { value: constants.positive.min, error: smallErr, expected: [constants.positive.min - smallErr, constants.positive.min + smallErr]},
          { value: constants.positive.min, error: 1, expected: [constants.positive.min - 1, constants.positive.min + 1]},
          { value: constants.negative.max, error: 0, expected: constants.negative.max },
          { value: constants.negative.max, error: smallErr, expected: [constants.negative.max - smallErr, constants.negative.max + smallErr]},
          { value: constants.negative.max, error: 1, expected: [constants.negative.max - 1, constants.negative.max + 1] },
          // 4. Subnormals, center can be flushed to 0.0
          { value: constants.positive.subnormal.max, error: 0, expected: [0, constants.positive.subnormal.max] },
          { value: constants.positive.subnormal.max, error: subnormalErr, expected: [-subnormalErr, constants.positive.subnormal.max + subnormalErr]},
          { value: constants.positive.subnormal.max, error: smallErr, expected: [-smallErr, constants.positive.subnormal.max + smallErr]},
          { value: constants.positive.subnormal.max, error: 1, expected: [-1, constants.positive.subnormal.max + 1]},
          { value: constants.positive.subnormal.min, error: 0, expected: [0, constants.positive.subnormal.min] },
          { value: constants.positive.subnormal.min, error: subnormalErr, expected: [-subnormalErr, constants.positive.subnormal.min + subnormalErr]},
          { value: constants.positive.subnormal.min, error: smallErr, expected: [-smallErr, constants.positive.subnormal.min + smallErr]},
          { value: constants.positive.subnormal.min, error: 1, expected: [-1, constants.positive.subnormal.min + 1] },
          { value: constants.negative.subnormal.min, error: 0, expected: [constants.negative.subnormal.min, 0] },
          { value: constants.negative.subnormal.min, error: subnormalErr, expected: [constants.negative.subnormal.min - subnormalErr, subnormalErr] },
          { value: constants.negative.subnormal.min, error: smallErr, expected: [constants.negative.subnormal.min - smallErr, smallErr] },
          { value: constants.negative.subnormal.min, error: 1, expected: [constants.negative.subnormal.min - 1, 1] },
          { value: constants.negative.subnormal.max, error: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: constants.negative.subnormal.max, error: subnormalErr, expected: [constants.negative.subnormal.max - subnormalErr, subnormalErr] },
          { value: constants.negative.subnormal.max, error: smallErr, expected: [constants.negative.subnormal.max - smallErr, smallErr] },
          { value: constants.negative.subnormal.max, error: 1, expected: [constants.negative.subnormal.max - 1, 1] },

          // 64-bit subnormals, expected to be treated as 0.0 or smallest subnormal of kind.
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), error: 0, expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), error: subnormalErr, expected: [-subnormalErr, constants.positive.subnormal.min + subnormalErr] },
          // Note that f32 minimum subnormal is so smaller than 1.0, adding them together may result in the f64 results 1.0.
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), error: 1, expected: [-1, constants.positive.subnormal.min + 1] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), error: 0, expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), error: subnormalErr, expected: [-subnormalErr, constants.positive.subnormal.min + subnormalErr] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), error: 1, expected: [-1, constants.positive.subnormal.min + 1] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), error: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), error: subnormalErr, expected: [constants.negative.subnormal.max - subnormalErr, subnormalErr] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), error: 1, expected: [constants.negative.subnormal.max - 1, 1] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), error: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), error: subnormalErr, expected: [constants.negative.subnormal.max - subnormalErr, subnormalErr] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), error: 1, expected: [constants.negative.subnormal.max - 1, 1] },

          // Zero
          { value: 0, error: 0, expected: 0 },
          { value: 0, error: smallErr, expected: [-smallErr, smallErr] },
          { value: 0, error: 1, expected: [-1, 1] },

          // Two
          { value: 2, error: 0, expected: 2 },
          { value: 2, error: smallErr, expected: [2 - smallErr, 2 + smallErr] },
          { value: 2, error: 1, expected: [1, 3] },
          { value: -2, error: 0, expected: -2 },
          { value: -2, error: smallErr, expected: [-2 - smallErr, -2 + smallErr] },
          { value: -2, error: 1, expected: [-3, -1] },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.absoluteErrorInterval(t.params.value, t.params.error);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.absoluteErrorInterval(${t.params.value}, ${
        t.params.error
      }) returned ${got} (${got.begin.toExponential()}, ${got.end.toExponential()}). Expected ${expected}`
    );
  });

interface CorrectlyRoundedCase {
  value: number;
  expected: number | IntervalBounds;
}

// Correctly rounded cases that input values are exactly representable normal values of target type
// prettier-ignore
const kCorrectlyRoundedNormalCases = {
  f32: [
    { value: 0, expected: [0, 0] },
    { value: reinterpretU32AsF32(0x03800000), expected: reinterpretU32AsF32(0x03800000) },
    { value: reinterpretU32AsF32(0x03800001), expected: reinterpretU32AsF32(0x03800001) },
    { value: reinterpretU32AsF32(0x83800000), expected: reinterpretU32AsF32(0x83800000) },
    { value: reinterpretU32AsF32(0x83800001), expected: reinterpretU32AsF32(0x83800001) },
  ] as CorrectlyRoundedCase[],
  f16: [
    { value: 0, expected: [0, 0] },
    { value: reinterpretU16AsF16(0x0c00), expected: reinterpretU16AsF16(0x0c00) },
    { value: reinterpretU16AsF16(0x0c01), expected: reinterpretU16AsF16(0x0c01) },
    { value: reinterpretU16AsF16(0x8c00), expected: reinterpretU16AsF16(0x8c00) },
    { value: reinterpretU16AsF16(0x8c01), expected: reinterpretU16AsF16(0x8c01) },
  ] as CorrectlyRoundedCase[],
} as const;

// 64-bit normals that fall between two conjunction normal values in target type
const kCorrectlyRoundedF64NormalCases = [
  {
    value: reinterpretU64AsF64(0x3ff0_0000_0000_0001n),
    expected: {
      f32: [reinterpretU32AsF32(0x3f800000), reinterpretU32AsF32(0x3f800001)],
      f16: [reinterpretU16AsF16(0x3c00), reinterpretU16AsF16(0x3c01)],
    },
  },
  {
    value: reinterpretU64AsF64(0x3ff0_0000_0000_0002n),
    expected: {
      f32: [reinterpretU32AsF32(0x3f800000), reinterpretU32AsF32(0x3f800001)],
      f16: [reinterpretU16AsF16(0x3c00), reinterpretU16AsF16(0x3c01)],
    },
  },
  {
    value: reinterpretU64AsF64(0x3ff0_0800_0000_0010n),
    expected: {
      f32: [reinterpretU32AsF32(0x3f804000), reinterpretU32AsF32(0x3f804001)],
      f16: [reinterpretU16AsF16(0x3c02), reinterpretU16AsF16(0x3c03)],
    },
  },
  {
    value: reinterpretU64AsF64(0x3ff0_1000_0000_0020n),
    expected: {
      f32: [reinterpretU32AsF32(0x3f808000), reinterpretU32AsF32(0x3f808001)],
      f16: [reinterpretU16AsF16(0x3c04), reinterpretU16AsF16(0x3c05)],
    },
  },
  {
    value: reinterpretU64AsF64(0xbff0_0000_0000_0001n),
    expected: {
      f32: [reinterpretU32AsF32(0xbf800001), reinterpretU32AsF32(0xbf800000)],
      f16: [reinterpretU16AsF16(0xbc01), reinterpretU16AsF16(0xbc00)],
    },
  },
  {
    value: reinterpretU64AsF64(0xbff0_0000_0000_0002n),
    expected: {
      f32: [reinterpretU32AsF32(0xbf800001), reinterpretU32AsF32(0xbf800000)],
      f16: [reinterpretU16AsF16(0xbc01), reinterpretU16AsF16(0xbc00)],
    },
  },
  {
    value: reinterpretU64AsF64(0xbff0_0800_0000_0010n),
    expected: {
      f32: [reinterpretU32AsF32(0xbf804001), reinterpretU32AsF32(0xbf804000)],
      f16: [reinterpretU16AsF16(0xbc03), reinterpretU16AsF16(0xbc02)],
    },
  },
  {
    value: reinterpretU64AsF64(0xbff0_1000_0000_0020n),
    expected: {
      f32: [reinterpretU32AsF32(0xbf808001), reinterpretU32AsF32(0xbf808000)],
      f16: [reinterpretU16AsF16(0xbc05), reinterpretU16AsF16(0xbc04)],
    },
  },
] as const;

g.test('correctlyRoundedInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<CorrectlyRoundedCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          // Edge Cases
          { value: constants.positive.infinity, expected: kUnboundedBounds },
          { value: constants.negative.infinity, expected: kUnboundedBounds },
          { value: constants.positive.max, expected: constants.positive.max },
          { value: constants.negative.min, expected: constants.negative.min },
          { value: constants.positive.min, expected: constants.positive.min },
          { value: constants.negative.max, expected: constants.negative.max },

          // Subnormals
          { value: constants.positive.subnormal.min, expected: [0, constants.positive.subnormal.min] },
          { value: constants.positive.subnormal.max, expected: [0, constants.positive.subnormal.max] },
          { value: constants.negative.subnormal.min, expected: [constants.negative.subnormal.min, 0] },
          { value: constants.negative.subnormal.max, expected: [constants.negative.subnormal.max, 0] },

          // 64-bit subnormals should be rounded down to 0 or up to smallest subnormal
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), expected: [constants.negative.subnormal.max, 0] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), expected: [constants.negative.subnormal.max, 0] },

          // Normals
          ...kCorrectlyRoundedNormalCases[p.trait],

          // 64-bit normals that fall between two conjunction normal values in target type
          ...kCorrectlyRoundedF64NormalCases.map(t => { return {value: t.value, expected: t.expected[p.trait]} as CorrectlyRoundedCase;}),
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.correctlyRoundedInterval(t.params.value);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.correctlyRoundedInterval(${t.params.value}) returned ${got}. Expected ${expected}`
    );
  });

interface ULPCase {
  value: number;
  num_ulp: number;
  expected: number | IntervalBounds;
}

// Special values used for testing ULP error interval
const kULPErrorValue = {
  f32: 4096, // 4096 ULP is required for atan accuracy on f32
  f16: 5, // 5 ULP is required for atan accuracy on f16
};

g.test('ulpInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ULPCase>(p => {
        const constants = FP[p.trait].constants();
        const ULPValue = kULPErrorValue[p.trait];
        const plusOneULP = kPlusOneULPFunctions[p.trait];
        const plusNULP = kPlusNULPFunctions[p.trait];
        const minusOneULP = kMinusOneULPFunctions[p.trait];
        const minusNULP = kMinusNULPFunctions[p.trait];
        // prettier-ignore
        return [
          // Edge Cases
          { value: constants.positive.infinity, num_ulp: 0, expected: kUnboundedBounds },
          { value: constants.positive.infinity, num_ulp: 1, expected: kUnboundedBounds },
          { value: constants.positive.infinity, num_ulp: ULPValue, expected: kUnboundedBounds },
          { value: constants.negative.infinity, num_ulp: 0, expected: kUnboundedBounds },
          { value: constants.negative.infinity, num_ulp: 1, expected: kUnboundedBounds },
          { value: constants.negative.infinity, num_ulp: ULPValue, expected: kUnboundedBounds },
          { value: constants.positive.max, num_ulp: 0, expected: constants.positive.max },
          { value: constants.positive.max, num_ulp: 1, expected: kUnboundedBounds },
          { value: constants.positive.max, num_ulp: ULPValue, expected: kUnboundedBounds },
          { value: constants.positive.min, num_ulp: 0, expected: constants.positive.min },
          { value: constants.positive.min, num_ulp: 1, expected: [0, plusOneULP(constants.positive.min)] },
          { value: constants.positive.min, num_ulp: ULPValue, expected: [0, plusNULP(constants.positive.min, ULPValue)] },
          { value: constants.negative.min, num_ulp: 0, expected: constants.negative.min },
          { value: constants.negative.min, num_ulp: 1, expected: kUnboundedBounds },
          { value: constants.negative.min, num_ulp: ULPValue, expected: kUnboundedBounds },
          { value: constants.negative.max, num_ulp: 0, expected: constants.negative.max },
          { value: constants.negative.max, num_ulp: 1, expected: [minusOneULP(constants.negative.max), 0] },
          { value: constants.negative.max, num_ulp: ULPValue, expected: [minusNULP(constants.negative.max, ULPValue), 0] },

          // Subnormals
          { value: constants.positive.subnormal.max, num_ulp: 0, expected: [0, constants.positive.subnormal.max] },
          { value: constants.positive.subnormal.max, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(constants.positive.subnormal.max)] },
          { value: constants.positive.subnormal.max, num_ulp: ULPValue, expected: [minusNULP(0, ULPValue), plusNULP(constants.positive.subnormal.max, ULPValue)] },
          { value: constants.positive.subnormal.min, num_ulp: 0, expected: [0, constants.positive.subnormal.min] },
          { value: constants.positive.subnormal.min, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(constants.positive.subnormal.min)] },
          { value: constants.positive.subnormal.min, num_ulp: ULPValue, expected: [minusNULP(0, ULPValue), plusNULP(constants.positive.subnormal.min, ULPValue)] },
          { value: constants.negative.subnormal.min, num_ulp: 0, expected: [constants.negative.subnormal.min, 0] },
          { value: constants.negative.subnormal.min, num_ulp: 1, expected: [minusOneULP(constants.negative.subnormal.min), plusOneULP(0)] },
          { value: constants.negative.subnormal.min, num_ulp: ULPValue, expected: [minusNULP(constants.negative.subnormal.min, ULPValue), plusNULP(0, ULPValue)] },
          { value: constants.negative.subnormal.max, num_ulp: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: constants.negative.subnormal.max, num_ulp: 1, expected: [minusOneULP(constants.negative.subnormal.max), plusOneULP(0)] },
          { value: constants.negative.subnormal.max, num_ulp: ULPValue, expected: [minusNULP(constants.negative.subnormal.max, ULPValue), plusNULP(0, ULPValue)] },

          // 64-bit subnormals
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), num_ulp: 0, expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(constants.positive.subnormal.min)] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0001n), num_ulp: ULPValue, expected: [minusNULP(0, ULPValue), plusNULP(constants.positive.subnormal.min, ULPValue)] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), num_ulp: 0, expected: [0, constants.positive.subnormal.min] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(constants.positive.subnormal.min)] },
          { value: reinterpretU64AsF64(0x0000_0000_0000_0002n), num_ulp: ULPValue, expected: [minusNULP(0, ULPValue), plusNULP(constants.positive.subnormal.min, ULPValue)] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), num_ulp: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), num_ulp: 1, expected: [minusOneULP(constants.negative.subnormal.max), plusOneULP(0)] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), num_ulp: ULPValue, expected: [minusNULP(constants.negative.subnormal.max, ULPValue), plusNULP(0, ULPValue)] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), num_ulp: 0, expected: [constants.negative.subnormal.max, 0] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), num_ulp: 1, expected: [minusOneULP(constants.negative.subnormal.max), plusOneULP(0)] },
          { value: reinterpretU64AsF64(0x800f_ffff_ffff_fffen), num_ulp: ULPValue, expected: [minusNULP(constants.negative.subnormal.max, ULPValue), plusNULP(0, ULPValue)] },

          // Zero
          { value: 0, num_ulp: 0, expected: 0 },
          { value: 0, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(0)] },
          { value: 0, num_ulp: ULPValue, expected: [minusNULP(0, ULPValue), plusNULP(0, ULPValue)] },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.ulpInterval(t.params.value, t.params.num_ulp);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.ulpInterval(${t.params.value}, ${t.params.num_ulp}) returned ${got}. Expected ${expected}`
    );
  });

// API - Acceptance Intervals

interface ScalarToIntervalCase {
  input: number;
  expected: number | IntervalBounds;
}

const kAbsIntervalCases = [
  {
    input: 0.1,
    expected: {
      f32: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)],
      f16: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)],
    },
  },
  {
    input: -0.1,
    expected: {
      f32: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)],
      f16: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)],
    },
  },
] as const;

g.test('absInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          // Common usages
          { input: 1, expected: 1 },
          { input: -1, expected: 1 },
          ...kAbsIntervalCases.map(t => {return {input: t.input, expected: t.expected[p.trait]} as ScalarToIntervalCase}),

          // Edge cases
          { input: constants.positive.infinity, expected: kUnboundedBounds },
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.positive.max, expected: constants.positive.max },
          { input: constants.positive.min, expected: constants.positive.min },
          { input: constants.negative.min, expected: constants.positive.max },
          { input: constants.negative.max, expected: constants.positive.min },

          // 32-bit subnormals
          { input: constants.positive.subnormal.max, expected: [0, constants.positive.subnormal.max] },
          { input: constants.positive.subnormal.min, expected: [0, constants.positive.subnormal.min] },
          { input: constants.negative.subnormal.min, expected: [0, constants.positive.subnormal.max] },
          { input: constants.negative.subnormal.max, expected: [0, constants.positive.subnormal.min] },

          // 64-bit subnormals
          { input: reinterpretU64AsF64(0x0000_0000_0000_0001n), expected: [0, constants.positive.subnormal.min] },
          { input: reinterpretU64AsF64(0x800f_ffff_ffff_ffffn), expected: [0, constants.positive.subnormal.min] },

          // Zero
          { input: 0, expected: 0 },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.absInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.absInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Acos expectation intervals are bounded by both inherited atan2(sqrt(1.0 - x*x), x) and absolute error.
// Atan2 introduce 4096ULP for f32 and 5ULP for f16, and sqrt inherited from 1.0/inverseSqrt.
// prettier-ignore
const kAcosIntervalCases = {
  f32: [
    { input: kPlusOneULPFunctions['f32'](-1), expected: [reinterpretU32AsF32(0x4048fa32), reinterpretU32AsF32(0x40491bdb)] },  // ~π
    { input: -1/2, expected: [reinterpretU32AsF32(0x4005fa90), reinterpretU32AsF32(0x40061a93)] },  // ~2π/3
    { input: 1/2, expected: [reinterpretU32AsF32(0x3f85fa8f), reinterpretU32AsF32(0x3f861a94)] },  // ~π/3
    // Input case to get smallest well-defined expected result, the expectation interval is bounded
    // by ULP (lower boundary) and absolute error (upper boundary).
    // f32 1.0-1ULP=0x3F7FFFFF=0.9999999403953552,
    // acos(0.9999999403953552)=3.4526698478747995220159699019994e-4 rounded to f32 0x39B504F3 or 0x39B504F4,
    // absolute error interval upper boundary 0x39B504F4+6.77e-5=0.00041296700619608164 i.e. f64 0x3F3B_106F_C933_4FB9.
    { input: kMinusOneULPFunctions['f32'](1), expected: [reinterpretU64AsF64(0x3f2f_fdff_6000_0000n), reinterpretU64AsF64(0x3f3b_106f_c933_4fb9n)] },  // ~0.0003
  ] as ScalarToIntervalCase[],
  f16: [
    { input: kPlusOneULPFunctions['f16'](-1), expected: [reinterpretU16AsF16(0x4233), reinterpretU16AsF16(0x4243)] },  // ~π
    { input: -1/2, expected: [reinterpretU16AsF16(0x402a), reinterpretU16AsF16(0x4037)] },  // ~2π/3
    { input: 1/2, expected: [reinterpretU16AsF16(0x3c29), reinterpretU16AsF16(0x3c38)] },  // ~π/3
    // Input case to get smallest well-defined expected result, the expectation interval is bounded
    // by ULP (lower boundary) and absolute error (upper boundary).
    // f16 1.0-1ULP=0x3BFF=0.99951171875,
    // acos(0.99951171875)=0.03125127170547389912035676677648 rounded to f16 0x2800 or 0x2801,
    // absolute error interval upper boundary 0x2801+3.91e-3=0.035190517578125 i.e. f64 0x3FA2_047D_D441_3554.
    { input: kMinusOneULPFunctions['f16'](1), expected: [reinterpretU16AsF16(0x259d), reinterpretU64AsF64(0x3fa2_047d_d441_3554n)] },  // ~0.03
  ] as ScalarToIntervalCase[],
} as const;

g.test('acosInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // The acceptance interval @ x = -1 and 1 is kUnboundedBounds, because
          // sqrt(1 - x*x) = sqrt(0), and sqrt is defined in terms of inverseqrt
          // The acceptance interval @ x = 0 is kUnboundedBounds, because atan2 is not
          // well-defined/implemented at 0.
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.negative.min, expected: kUnboundedBounds },
          { input: -1, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: 1, expected: kUnboundedBounds },
          { input: constants.positive.max, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },

          // Cases that bounded by absolute error and inherited from atan2(sqrt(1-x*x), x). Note that
          // even x is very close to 1.0 and the expected result is close to 0.0, the expected
          // interval is still bounded by ULP as well as absolute error, specifically lower boundary
          // comes from ULP error and upper boundary comes from absolute error in those cases.
          ...kAcosIntervalCases[p.trait],
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.acosInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.acosInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshAlternativeInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: kUnboundedBounds },
      { input: 0, expected: kUnboundedBounds },
      { input: 1, expected: kUnboundedBounds },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [reinterpretU64AsF64(0x3fdc_6368_8000_0000n), reinterpretU64AsF64(0x3fdc_636f_2000_0000n)] },  // ~0.443..., differs from the primary in the later digits
      { input: 10, expected: [reinterpretU64AsF64(0x4007_f21e_4000_0000n), reinterpretU64AsF64(0x4007_f21f_6000_0000n)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.acoshAlternativeInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshPrimaryInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: kUnboundedBounds },
      { input: 0, expected: kUnboundedBounds },
      { input: 1, expected: kUnboundedBounds },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [reinterpretU64AsF64(0x3fdc_6368_2000_0000n), reinterpretU64AsF64(0x3fdc_636f_8000_0000n)] }, // ~0.443..., differs from the alternative in the later digits
      { input: 10, expected: [reinterpretU64AsF64(0x4007_f21e_4000_0000n), reinterpretU64AsF64(0x4007_f21f_6000_0000n)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.acoshPrimaryInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Asin cases that bounded by inherited atan2(x, sqrt(1.0 - x*x)) rather than absolute error.
// Atan2 introduce 4096ULP for f32 and 5ULP for f16, and sqrt inherited from 1.0/inverseSqrt.
// prettier-ignore
const kAsinIntervalInheritedCases = {
  f32: [
    { input: -1/2, expected: [reinterpretU32AsF32(0xbf061a96), reinterpretU32AsF32(0xbf05fa8e)] },  // ~-π/6
    { input: 1/2, expected: [reinterpretU32AsF32(0x3f05fa8e), reinterpretU32AsF32(0x3f061a96)] },  // ~π/6
  ] as ScalarToIntervalCase[],
  f16: [
    { input: -1/2, expected: [reinterpretU16AsF16(0xb83a), reinterpretU16AsF16(0xb827)] },  // ~-π/6
    { input: 1/2, expected: [reinterpretU16AsF16(0x3827), reinterpretU16AsF16(0x383a)] },  // ~π/6
  ] as ScalarToIntervalCase[],
} as const;

g.test('asinInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        const abs_error = p.trait === 'f32' ? 6.77e-5 : 3.91e-3;
        // prettier-ignore
        return [
          // The acceptance interval @ x = -1 and 1 is kUnboundedBounds, because
          // sqrt(1 - x*x) = sqrt(0), and sqrt is defined in terms of inversqrt.
          // The acceptance interval @ x = 0 is kUnboundedBounds, because atan2 is not
          // well-defined/implemented at 0.
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.negative.min, expected: kUnboundedBounds },
          { input: -1, expected: kUnboundedBounds },
          // Subnormal input may get flushed to 0, and result in kUnboundedBounds.
          { input: constants.negative.subnormal.min, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: constants.positive.subnormal.max, expected: kUnboundedBounds },
          { input: 1, expected: kUnboundedBounds },
          { input: constants.positive.max, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },

          // When input near 0, the expected result is bounded by absolute error rather than ULP
          // error. Away from 0 the atan2 inherited error should be larger.
          { input: constants.negative.max, expected: trait.absoluteErrorInterval(Math.asin(constants.negative.max), abs_error).bounds() },  // ~0
          { input: constants.positive.min, expected: trait.absoluteErrorInterval(Math.asin(constants.positive.min), abs_error).bounds() },  // ~0

          // Cases that inherited from atan2(x, sqrt(1-x*x))
          ...kAsinIntervalInheritedCases[p.trait],
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.asinInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.asinInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('asinhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: [reinterpretU64AsF64(0xbfec_343a_8000_0000n), reinterpretU64AsF64(0xbfec_3432_8000_0000n)] },  // ~-0.88137...
      { input: 0, expected: [reinterpretU64AsF64(0xbeaa_0000_2000_0000n), reinterpretU64AsF64(0x3eb1_ffff_d000_0000n)] },  // ~0
      { input: 1, expected: [reinterpretU64AsF64(0x3fec_3435_4000_0000n), reinterpretU64AsF64(0x3fec_3437_8000_0000n)] },  // ~0.88137...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.asinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.asinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kAtanIntervalCases = {
  f32: [
    // x=-√3=-1.7320508... quantized to f32 0xBFDDB3D7,
    // atan(0xBFDDB3D7)=-1.0471975434247854181546378047331 ~ -pi/3 rounded to f32 0xBF860A92 or 0xBF860A91,
    // kValue.f32.negative.pi.third is 0xBF860A92.
    { input: reinterpretU32AsF32(0xbfddb3d7), expected: [kValue.f32.negative.pi.third, kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.third)] },
    // atan(-1)=-0.78539816339744830961566084581988 ~ -pi/4 rounded to f32 0xBF490FDB or 0xBF490FDA,
    // kValue.f32.negative.pi.quarter is 0xBF490FDB.
    { input: -1, expected: [kValue.f32.negative.pi.quarter, kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.quarter)] },
    // x=-1/√3=-0.577350269... quantized to f32 0xBF13CD3A,
    // atan(0xBF13CD3A)=-0.52359876782648663982267459646249 ~ -pi/6 rounded to f32 0xBF060A92 or 0xBF060A91,
    // kValue.f32.negative.pi.sixth is 0xBF060A92.
    { input: reinterpretU32AsF32(0xbf13cd3a), expected: [kValue.f32.negative.pi.sixth, kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.sixth)] },
    // x=1/√3=0.577350269... quantized to f32 0x3F13CD3A.
    { input: reinterpretU32AsF32(0x3f13cd3a), expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.sixth), kValue.f32.positive.pi.sixth] },
    { input: 1, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.quarter), kValue.f32.positive.pi.quarter] },
    // x=√3=1.7320508... quantized to f32 0x3FDDB3D7.
    { input: reinterpretU32AsF32(0x3fddb3d7), expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.third), kValue.f32.positive.pi.third] },
  ] as ScalarToIntervalCase[],
  f16: [
    // x=-√3=-1.7320508... quantized to f16 0xBEED,
    // atan(0xBEED)=-1.0470461377318847079113932677171 ~ -pi/3 rounded to f16 0xBC31 or 0xBC30,
    // kValue.f16.negative.pi.third is 0xBC30.
    { input: reinterpretU16AsF16(0xbeed), expected: [kMinusOneULPFunctions['f16'](kValue.f16.negative.pi.third), kValue.f16.negative.pi.third] },
    // atan(-1)=-0.78539816339744830961566084581988 ~ -pi/4 rounded to f16 0xBA49 or 0xBA48.
    // kValue.f16.negative.pi.quarter is 0xBA48.
    { input: -1, expected: [kMinusOneULPFunctions['f16'](kValue.f16.negative.pi.quarter), kValue.f16.negative.pi.quarter] },
    // x=-1/√3=-0.577350269... quantized to f16 0xB89E,
    // atan(0xB89E)=-0.52344738860166563645762619364966 ~ -pi/6 rounded to f16 0xB831 or 0xB830,
    // kValue.f16.negative.pi.sixth is 0xB830.
    { input: reinterpretU16AsF16(0xb89e), expected: [kMinusOneULPFunctions['f16'](kValue.f16.negative.pi.sixth), kValue.f16.negative.pi.sixth] },
    // x=1/√3=0.577350269... quantized to f16 0x389E
    { input: reinterpretU16AsF16(0x389e), expected: [kValue.f16.positive.pi.sixth, kPlusOneULPFunctions['f16'](kValue.f16.positive.pi.sixth)] },
    { input: 1, expected: [kValue.f16.positive.pi.quarter, kPlusOneULPFunctions['f16'](kValue.f16.positive.pi.quarter)] },
    // x=√3=1.7320508... quantized to f16 0x3EED
    { input: reinterpretU16AsF16(0x3eed), expected: [kValue.f16.positive.pi.third, kPlusOneULPFunctions['f16'](kValue.f16.positive.pi.third)] },
  ] as ScalarToIntervalCase[],
} as const;

g.test('atanInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          { input: 0, expected: 0 },
          ...kAtanIntervalCases[p.trait],

          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    const ulp_error = t.params.trait === 'f32' ? 4096 : 5;
    const error = (n: number): number => {
      return ulp_error * trait.oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.atanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.atanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: kUnboundedBounds },
      { input: -0.1, expected: [reinterpretU64AsF64(0xbfb9_af9a_6000_0000n), reinterpretU64AsF64(0xbfb9_af8c_c000_0000n)] },  // ~-0.1003...
      { input: 0, expected: [reinterpretU64AsF64(0xbe96_0000_2000_0000n), reinterpretU64AsF64(0x3e98_0000_0000_0000n)] },  // ~0
      { input: 0.1, expected: [reinterpretU64AsF64(0x3fb9_af8b_8000_0000n), reinterpretU64AsF64(0x3fb9_af9b_0000_0000n)] },  // ~0.1003...
      { input: 1, expected: kUnboundedBounds },
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.atanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.atanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Large but still representable integer
const kCeilIntervalCases = {
  f32: [
    { input: 2 ** 30, expected: 2 ** 30 },
    { input: -(2 ** 30), expected: -(2 ** 30) },
    { input: 0x80000000, expected: 0x80000000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
  f16: [
    { input: 2 ** 14, expected: 2 ** 14 },
    { input: -(2 ** 14), expected: -(2 ** 14) },
    { input: 0x8000, expected: 0x8000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
} as const;

g.test('ceilInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
        { input: 0, expected: 0 },
        { input: 0.1, expected: 1 },
        { input: 0.9, expected: 1 },
        { input: 1.0, expected: 1 },
        { input: 1.1, expected: 2 },
        { input: 1.9, expected: 2 },
        { input: -0.1, expected: 0 },
        { input: -0.9, expected: 0 },
        { input: -1.0, expected: -1 },
        { input: -1.1, expected: -1 },
        { input: -1.9, expected: -1 },

        // Edge cases
        { input: constants.positive.infinity, expected: kUnboundedBounds },
        { input: constants.negative.infinity, expected: kUnboundedBounds },
        { input: constants.positive.max, expected: constants.positive.max },
        { input: constants.positive.min, expected: 1 },
        { input: constants.negative.min, expected: constants.negative.min },
        { input: constants.negative.max, expected: 0 },
        ...kCeilIntervalCases[p.trait],

        // 32-bit subnormals
        { input: constants.positive.subnormal.max, expected: [0, 1] },
        { input: constants.positive.subnormal.min, expected: [0, 1] },
        { input: constants.negative.subnormal.min, expected: 0 },
        { input: constants.negative.subnormal.max, expected: 0 },
      ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.ceilInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.ceilInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Cos interval cases on x=π/3, the result of f32 and f16 is different because π/3 quantized to
// different direction for two types.
const kCosIntervalThirdPiCases = {
  // prettier-ignore
  f32: [
    // cos(-1.0471975803375244) = 0.499999974763
    { input: kValue.f32.negative.pi.third, expected: [kMinusOneULPFunctions['f32'](1/2), 1/2] },
    // cos(1.0471975803375244) = 0.499999974763
    { input: kValue.f32.positive.pi.third, expected: [kMinusOneULPFunctions['f32'](1/2), 1/2] },
  ],
  f16: [
    // cos(-1.046875) = 0.50027931
    {
      input: kValue.f16.negative.pi.third,
      expected: FP['f16'].correctlyRoundedInterval(0.50027931).bounds(),
    },
    // cos(1.046875) = 0.50027931
    {
      input: kValue.f16.positive.pi.third,
      expected: FP['f16'].correctlyRoundedInterval(0.50027931).bounds(),
    },
  ],
};

g.test('cosInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // This test does not include some common cases. i.e. f(x = π/2) = 0,
          // because the difference between true x and x as a f32 is sufficiently
          // large, such that the high slope of f @ x causes the results to be
          // substantially different, so instead of getting 0 you get a value on the
          // order of 10^-8 away from 0, thus difficult to express in a
          // human-readable manner.
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.negative.min, expected: kUnboundedBounds },
          { input: constants.negative.pi.whole, expected: [-1, kPlusOneULPFunctions[p.trait](-1)] },
          { input: 0, expected: [1, 1] },
          { input: constants.positive.pi.whole, expected: [-1, kPlusOneULPFunctions[p.trait](-1)] },
          { input: constants.positive.max, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },

          ...(kCosIntervalThirdPiCases[p.trait] as ScalarToIntervalCase[]),
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    const error = (_: number): number => {
      return t.params.trait === 'f32' ? 2 ** -11 : 2 ** -7;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.cosInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.cosInterval(${t.params.input}) returned ${got}. Expected ${expected}, ===${t.params.expected}===`
    );
  });

g.test('coshInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: [ reinterpretU32AsF32(0x3fc583a4), reinterpretU32AsF32(0x3fc583b1)] },  // ~1.1543...
      { input: 0, expected: [reinterpretU32AsF32(0x3f7ffffd), reinterpretU32AsF32(0x3f800002)] },  // ~1
      { input: 1, expected: [ reinterpretU32AsF32(0x3fc583a4), reinterpretU32AsF32(0x3fc583b1)] },  // ~1.1543...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.coshInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.coshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('degreesInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: kValue.f32.negative.pi.whole, expected: [kMinusOneULPFunctions['f32'](-180), kPlusOneULPFunctions['f32'](-180)] },
      { input: kValue.f32.negative.pi.three_quarters, expected: [kMinusOneULPFunctions['f32'](-135), kPlusOneULPFunctions['f32'](-135)] },
      { input: kValue.f32.negative.pi.half, expected: [kMinusOneULPFunctions['f32'](-90), kPlusOneULPFunctions['f32'](-90)] },
      { input: kValue.f32.negative.pi.third, expected: [kMinusOneULPFunctions['f32'](-60), kPlusOneULPFunctions['f32'](-60)] },
      { input: kValue.f32.negative.pi.quarter, expected: [kMinusOneULPFunctions['f32'](-45), kPlusOneULPFunctions['f32'](-45)] },
      { input: kValue.f32.negative.pi.sixth, expected: [kMinusOneULPFunctions['f32'](-30), kPlusOneULPFunctions['f32'](-30)] },
      { input: 0, expected: 0 },
      { input: kValue.f32.positive.pi.sixth, expected: [kMinusOneULPFunctions['f32'](30), kPlusOneULPFunctions['f32'](30)] },
      { input: kValue.f32.positive.pi.quarter, expected: [kMinusOneULPFunctions['f32'](45), kPlusOneULPFunctions['f32'](45)] },
      { input: kValue.f32.positive.pi.third, expected: [kMinusOneULPFunctions['f32'](60), kPlusOneULPFunctions['f32'](60)] },
      { input: kValue.f32.positive.pi.half, expected: [kMinusOneULPFunctions['f32'](90), kPlusOneULPFunctions['f32'](90)] },
      { input: kValue.f32.positive.pi.three_quarters, expected: [kMinusOneULPFunctions['f32'](135), kPlusOneULPFunctions['f32'](135)] },
      { input: kValue.f32.positive.pi.whole, expected: [kMinusOneULPFunctions['f32'](180), kPlusOneULPFunctions['f32'](180)] },
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.degreesInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.degreesInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('expInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: 0, expected: 1 },
      { input: 1, expected: [kValue.f32.positive.e, kPlusOneULPFunctions['f32'](kValue.f32.positive.e)] },
      { input: 89, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULPF32(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.expInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.expInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('exp2Interval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: 0, expected: 1 },
      { input: 1, expected: 2 },
      { input: 128, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULPF32(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.exp2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.exp2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Large but still representable integer
const kFloorIntervalCases = {
  f32: [
    { input: 2 ** 30, expected: 2 ** 30 },
    { input: -(2 ** 30), expected: -(2 ** 30) },
    { input: 0x80000000, expected: 0x80000000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
  f16: [
    { input: 2 ** 14, expected: 2 ** 14 },
    { input: -(2 ** 14), expected: -(2 ** 14) },
    { input: 0x8000, expected: 0x8000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
} as const;

g.test('floorInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          { input: 0, expected: 0 },
          { input: 0.1, expected: 0 },
          { input: 0.9, expected: 0 },
          { input: 1.0, expected: 1 },
          { input: 1.1, expected: 1 },
          { input: 1.9, expected: 1 },
          { input: -0.1, expected: -1 },
          { input: -0.9, expected: -1 },
          { input: -1.0, expected: -1 },
          { input: -1.1, expected: -2 },
          { input: -1.9, expected: -2 },

          // Edge cases
          { input: constants.positive.infinity, expected: kUnboundedBounds },
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.positive.max, expected: constants.positive.max },
          { input: constants.positive.min, expected: 0 },
          { input: constants.negative.min, expected: constants.negative.min },
          { input: constants.negative.max, expected: -1 },
          ...kFloorIntervalCases[p.trait],

          // 32-bit subnormals
          { input: constants.positive.subnormal.max, expected: 0 },
          { input: constants.positive.subnormal.min, expected: 0 },
          { input: constants.negative.subnormal.min, expected: [-1, 0] },
          { input: constants.negative.subnormal.max, expected: [-1, 0] },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.floorInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.floorInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fractInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] }, // ~0.1
      { input: 0.9, expected: [reinterpretU32AsF32(0x3f666666), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0x3f666666))] },  // ~0.9
      { input: 1.0, expected: 0 },
      { input: 1.1, expected: [reinterpretU64AsF64(0x3fb9_9998_0000_0000n), reinterpretU64AsF64(0x3fb9_999a_0000_0000n)] }, // ~0.1
      { input: -0.1, expected: [reinterpretU32AsF32(0x3f666666), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0x3f666666))] },  // ~0.9
      { input: -0.9, expected: [reinterpretU64AsF64(0x3fb9_9999_0000_0000n), reinterpretU64AsF64(0x3fb9_999a_0000_0000n)] }, // ~0.1
      { input: -1.0, expected: 0 },
      { input: -1.1, expected: [reinterpretU64AsF64(0x3fec_cccc_c000_0000n), reinterpretU64AsF64(0x3fec_cccd_0000_0000n), ] }, // ~0.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.positive.max, expected: 0 },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: 0 },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.less_than_one, 1.0] },

      // https://github.com/gpuweb/cts/issues/2766
      { input: 0x80000000, expected: 0 },
]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.fractInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.fractInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kInverseSqrtIntervalCases = {
  f32: [
    // 0.04 rounded to f32 0x3D23D70A or 0x3D23D70B,
    // 1/sqrt(0x3D23D70B)=4.9999998230487200185270893769213 rounded to f32 0x409FFFFF or 0x40A00000,
    // 1/sqrt(0x3D23D70A)=5.0000000558793553117506910583908 rounded to f32 0x40A00000 or 0x40A00001.
    { input: 0.04, expected: [reinterpretU32AsF32(0x409FFFFF), reinterpretU32AsF32(0x40A00001)] },  // ~5.0
    // 1/sqrt(100.0)=0.1 rounded to f32 0x3DCCCCCC or 0x3DCCCCCD
    { input: 100, expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    // Maximium f32 0x7F7FFFFF = 3.4028234663852886e+38,
    // 1/sqrt(0x7F7FFFFF)=5.4210110239862427800382690921791e-20 rounded to f32 0x1F800000 or 0x1F800001
    { input: kValue.f32.positive.max, expected: [reinterpretU32AsF32(0x1f800000), reinterpretU32AsF32(0x1f800001)] },  // ~5.421...e-20
  ] as ScalarToIntervalCase[],
  f16: [
    // 0.04 rounded to f16 0x291E or 0x291F,
    // 1/sqrt(0x291F)=4.9994660279328446295684795818427 rounded to f16 0x44FF or 0x4500,
    // 1/sqrt(0x291E)=5.001373857053206453045376503367 rounded to f16 0x4500 or 0x4501.
    { input: 0.04, expected: [reinterpretU16AsF16(0x44FF), reinterpretU16AsF16(0x4501)] },  // ~5.0
    // 1/sqrt(100.0)=0.1 rounded to f16 0x2E66 or 0x2E67
    { input: 100, expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },  // ~0.1
    // Maximium f16 0x7BFF = 65504,
    // 1/sqrt(0x7BFF)=0.00390720402370454101997160826062 rounded to f16 0x1C00 or 0x1C01
    { input: kValue.f16.positive.max, expected: [reinterpretU16AsF16(0x1c00), reinterpretU16AsF16(0x1c01)] },  // ~3.9072...e-3
  ] as ScalarToIntervalCase[],
} as const;

g.test('inverseSqrtInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Exactly representable cases
          { input: 1, expected: 1 },
          { input: 0.25, expected: 2 },
          { input: 64, expected: 0.125 },

          // Cases that input and/or result not exactly representable
          ...kInverseSqrtIntervalCases[p.trait],

          // Out of definition domain
          { input: -1, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    const error = (n: number): number => {
      return 2 * trait.oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.inverseSqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.inverseSqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('lengthIntervalScalar_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      //
      // length(0) = kUnboundedBounds, because length uses sqrt, which is defined as 1/inversesqrt
      {input: 0, expected: kUnboundedBounds },
      {input: 1.0, expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: -1.0, expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: 0.1, expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      {input: -0.1, expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      {input: 10.0, expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10
      {input: -10.0, expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10

      // Subnormal Cases
      { input: kValue.f32.subnormal.negative.min, expected: kUnboundedBounds },
      { input: kValue.f32.subnormal.negative.max, expected: kUnboundedBounds },
      { input: kValue.f32.subnormal.positive.min, expected: kUnboundedBounds },
      { input: kValue.f32.subnormal.positive.max, expected: kUnboundedBounds },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: kValue.f32.negative.max, expected: kUnboundedBounds },
      { input: kValue.f32.positive.min, expected: kUnboundedBounds },
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.lengthInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kLogIntervalCases = {
  f32: [
    // kValue.f32.positive.e is 0x402DF854 = 2.7182817459106445,
    // log(0x402DF854) = 0.99999996963214000677592342891704 rounded to f32 0x3F7FFFFF or 0x3F800000 = 1.0
    { input: kValue.f32.positive.e, expected: [kMinusOneULPFunctions['f32'](1.0), 1.0] },
    // kValue.f32.positive.max is 0x7F7FFFFF = 3.4028234663852886e+38,
    // log(0x7F7FFFFF) = 88.72283905206835305421152826479 rounded to f32 0x42B17217 or 0x42B17218.
    { input: kValue.f32.positive.max, expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x42b17218)), reinterpretU32AsF32(0x42b17218)] },
  ] as ScalarToIntervalCase[],
  f16: [
    // kValue.f16.positive.e is 0x416F = 2.716796875,
    // log(0x416F) = 0.99945356688393512460279716546501 rounded to f16 0x3BFE or 0x3BFF.
    { input: kValue.f16.positive.e, expected: [reinterpretU16AsF16(0x3bfe), reinterpretU16AsF16(0x3bff)] },
    // kValue.f16.positive.max is 0x7BFF = 65504,
    // log(0x7BFF) = 11.089866488461016076210728979771 rounded to f16 0x498B or 0x498C.
    { input: kValue.f16.positive.max, expected: [reinterpretU16AsF16(0x498b), reinterpretU16AsF16(0x498c)] },
  ] as ScalarToIntervalCase[],
} as const;

g.test('logInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        // prettier-ignore
        return [
          { input: -1, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: 1, expected: 0 },
          ...kLogIntervalCases[p.trait],
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const abs_error = t.params.trait === 'f32' ? 2 ** -21 : 2 ** -7;
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return abs_error;
      }
      return 3 * trait.oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.logInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.logInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kLog2IntervalCases = {
  f32: [
    // kValue.f32.positive.max is 0x7F7FFFFF = 3.4028234663852886e+38,
    // log2(0x7F7FFFFF) = 127.99999991400867200665269600978 rounded to f32 0x42FFFFFF or 0x43000000 = 128.0
    { input: kValue.f32.positive.max, expected: [kMinusOneULPFunctions['f32'](128.0), 128.0] },
  ] as ScalarToIntervalCase[],
  f16: [
    // kValue.f16.positive.max is 0x7BFF = 65504,
    // log2(0x7BFF) = 15.999295387023410627258428389903 rounded to f16 0x4BFF or 0x4C00 = 16.0
    { input: kValue.f16.positive.max, expected: [kMinusOneULPFunctions['f16'](16.0), 16.0] },
  ] as ScalarToIntervalCase[],
} as const;

g.test('log2Interval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        // prettier-ignore
        return [
          { input: -1, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: 1, expected: 0 },
          { input: 2, expected: 1 },
          { input: 16, expected: 4 },
          ...kLog2IntervalCases[p.trait],
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const abs_error = t.params.trait === 'f32' ? 2 ** -21 : 2 ** -7;
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return abs_error;
      }
      return 3 * trait.oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.log2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.log2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('negationInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] }, // ~-0.1
      { input: 1.0, expected: -1.0 },
      { input: 1.9, expected: [reinterpretU32AsF32(0xbff33334), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbff33334))] },  // ~-1.9
      { input: -0.1, expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] }, // ~0.1
      { input: -1.0, expected: 1 },
      { input: -1.9, expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3ff33334)), reinterpretU32AsF32(0x3ff33334)] },  // ~1.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.negative.min },
      { input: kValue.f32.positive.min, expected: kValue.f32.negative.max },
      { input: kValue.f32.negative.min, expected: kValue.f32.positive.max },
      { input: kValue.f32.negative.max, expected: kValue.f32.positive.min },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: kValue.f32.subnormal.positive.min, expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.negationInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.negationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('quantizeToF16Interval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: kValue.f16.negative.min, expected: kValue.f16.negative.min },
      { input: -1, expected: -1 },
      { input: -0.1, expected: [reinterpretU32AsF32(0xbdcce000), reinterpretU32AsF32(0xbdccc000)] },  // ~-0.1
      { input: kValue.f16.negative.max, expected: kValue.f16.negative.max },
      { input: kValue.f16.subnormal.negative.min, expected: [kValue.f16.subnormal.negative.min, 0] },
      { input: kValue.f16.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { input: 0, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.max, expected: [0, kValue.f16.subnormal.positive.max] },
      { input: kValue.f16.positive.min, expected: kValue.f16.positive.min },
      { input: 0.1, expected: [reinterpretU32AsF32(0x3dccc000), reinterpretU32AsF32(0x3dcce000)] },  // ~0.1
      { input: 1, expected: 1 },
      { input: kValue.f16.positive.max, expected: kValue.f16.positive.max },
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.quantizeToF16Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.quantizeToF16Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('radiansInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: -180, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.whole), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.whole)] },
      { input: -135, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.three_quarters), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.three_quarters)] },
      { input: -90, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.half), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.half)] },
      { input: -60, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.third), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.third)] },
      { input: -45, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.quarter), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.quarter)] },
      { input: -30, expected: [kMinusOneULPFunctions['f32'](kValue.f32.negative.pi.sixth), kPlusOneULPFunctions['f32'](kValue.f32.negative.pi.sixth)] },
      { input: 0, expected: 0 },
      { input: 30, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.sixth), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.sixth)] },
      { input: 45, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.quarter), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.quarter)] },
      { input: 60, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.third), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.third)] },
      { input: 90, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.half), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.half)] },
      { input: 135, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.three_quarters), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.three_quarters)] },
      { input: 180, expected: [kMinusOneULPFunctions['f32'](kValue.f32.positive.pi.whole), kPlusOneULPFunctions['f32'](kValue.f32.positive.pi.whole)] },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.radiansInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.radiansInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// Large but still representable integer
const kRoundIntervalCases = {
  f32: [
    { input: 2 ** 30, expected: 2 ** 30 },
    { input: -(2 ** 30), expected: -(2 ** 30) },
    { input: 0x80000000, expected: 0x80000000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
  f16: [
    { input: 2 ** 14, expected: 2 ** 14 },
    { input: -(2 ** 14), expected: -(2 ** 14) },
    { input: 0x8000, expected: 0x8000 }, // https://github.com/gpuweb/cts/issues/2766
  ],
} as const;

g.test('roundInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 0 },
      { input: 0.5, expected: 0 },  // Testing tie breaking
      { input: 0.9, expected: 1 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 1 },
      { input: 1.5, expected: 2 },  // Testing tie breaking
      { input: 1.9, expected: 2 },
      { input: -0.1, expected: 0 },
      { input: -0.5, expected: 0 },  // Testing tie breaking
      { input: -0.9, expected: -1 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -1 },
      { input: -1.5, expected: -2 },  // Testing tie breaking
      { input: -1.9, expected: -2 },

      // Edge cases
      { input: constants.positive.infinity, expected: kUnboundedBounds },
      { input: constants.negative.infinity, expected: kUnboundedBounds },
      { input: constants.positive.max, expected: constants.positive.max },
      { input: constants.positive.min, expected: 0 },
      { input: constants.negative.min, expected: constants.negative.min },
      { input: constants.negative.max, expected: 0 },
      ...kRoundIntervalCases[p.trait],

      // 32-bit subnormals
      { input: constants.positive.subnormal.max, expected: 0 },
      { input: constants.positive.subnormal.min, expected: 0 },
      { input: constants.negative.subnormal.min, expected: 0 },
      { input: constants.negative.subnormal.max, expected: 0 },
    ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.roundInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.roundInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('saturateInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: 0, expected: 0 },
      { input: 1, expected: 1.0 },
      { input: -0.1, expected: 0 },
      { input: -1, expected: 0 },
      { input: -10, expected: 0 },
      { input: 0.1, expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
      { input: 10, expected: 1.0 },
      { input: 11.1, expected: 1.0 },
      { input: kValue.f32.positive.max, expected: 1.0 },
      { input: kValue.f32.positive.min, expected: kValue.f32.positive.min },
      { input: kValue.f32.negative.max, expected: 0.0 },
      { input: kValue.f32.negative.min, expected: 0.0 },

      // Subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0.0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0.0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [kValue.f32.subnormal.negative.min, 0.0] },
      { input: kValue.f32.subnormal.negative.max, expected: [kValue.f32.subnormal.negative.max, 0.0] },

      // Infinities
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.saturateInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.saturationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('signInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: -1 },
      { input: -10, expected: -1 },
      { input: -1, expected: -1 },
      { input: -0.1, expected: -1 },
      { input: kValue.f32.negative.max, expected:  -1 },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
      { input: 0, expected: 0 },
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.positive.min, expected: 1 },
      { input: 0.1, expected: 1 },
      { input: 1, expected: 1 },
      { input: 10, expected: 1 },
      { input: kValue.f32.positive.max, expected: 1 },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.signInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.signInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          // This test does not include some common cases, i.e. f(x = -π|π) = 0,
          // because the difference between true x and x as a f32 is sufficiently
          // large, such that the high slope of f @ x causes the results to be
          // substantially different, so instead of getting 0 you get a value on the
          // order of 10^-8 away from it, thus difficult to express in a
          // human-readable manner.
          { input: constants.negative.infinity, expected: kUnboundedBounds },
          { input: constants.negative.min, expected: kUnboundedBounds },
          { input: constants.negative.pi.half, expected: [-1, kPlusOneULPFunctions[p.trait](-1)] },
          { input: 0, expected: 0 },
          { input: constants.positive.pi.half, expected: [kMinusOneULPFunctions[p.trait](1), 1] },
          { input: constants.positive.max, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    const error = (_: number): number => {
      return t.params.trait === 'f32' ? 2 ** -11 : 2 ** -7;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.sinInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.sinInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: [ reinterpretU32AsF32(0xbf966d05), reinterpretU32AsF32(0xbf966cf8)] },  // ~-1.175...
      { input: 0, expected: [reinterpretU32AsF32(0xb4600000), reinterpretU32AsF32(0x34600000)] },  // ~0
      { input: 1, expected: [ reinterpretU32AsF32(0x3f966cf8), reinterpretU32AsF32(0x3f966d05)] },  // ~1.175...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.sinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.sinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

// For sqrt interval inherited from 1.0 / inverseSqrt(x), errors come from:
//   1. Rounding of input x, if any;
//   2. 2 ULP from inverseSqrt;
//   3. And 2.5 ULP from division.
// The last 2.5ULP is handled in test and not included in the expected values here.
// prettier-ignore
const kSqrtIntervalCases = {
  f32: [
    // 0.01 rounded to f32 0x3C23D70A or 0x3C23D70B.
    // For inverseSqrt interval, floor_f32(1.0/sqrt(0x3C23D70B))-2ULP=0x411FFFFD,
    // ceil_f32(1.0/sqrt(0x3C23D70A))+2ULP=0x41200003.
    // For division, 1.0/0x41200003=0.09999997138977868544997855067803 rounded to f32 0x3DCCCCC8 or 0x3DCCCCC9,
    // 1.0/0x411FFFFD=0.100000028610237685454662304067 rounded to f32 0x3DCCCCD0 or 0x3DCCCCD1.
    { input: 0.01, expected: [reinterpretU32AsF32(0x3DCCCCC8), reinterpretU32AsF32(0x3DCCCCD1)] },  // ~0.1
    // For inverseSqrt interval, 1.0/sqrt(1.0)-2ULP=0x3F7FFFFE, 1.0/sqrt(1.0)+2ULP=0x3F800001.
    // For division, 1.0/0x3F800001=0.9999998807907246108530328709735 rounded to f32 0x3F7FFFFE or 0x3F7FFFFF,
    // 1.0/0x3F7FFFFE=1.0000001192093038108564210027667 rounded to f32 0x3F800001 or 0x3F800002.
    { input: 1, expected: [reinterpretU32AsF32(0x3F7FFFFE), reinterpretU32AsF32(0x3F800002)] },  // ~1
    // For inverseSqrt interval, 1.0/sqrt(4.0)-2ULP=0x3EFFFFFE, 1.0/sqrt(4.0)+2ULP=0x3F000001.
    // For division, 1.0/0x3F000001=1.999999761581449221706065741947 rounded to f32 0x3FFFFFFE or 0x3FFFFFFF,
    // 1.0/0x3EFFFFFE=2.0000002384186076217128420055334 rounded to f32 0x40000001 or 0x40000002.
    { input: 4, expected: [reinterpretU32AsF32(0x3FFFFFFE), reinterpretU32AsF32(0x40000002)] },  // ~2
    // For inverseSqrt interval, floor_f32(1.0/sqrt(100.0))-2ULP=0x3DCCCCCA,
    // ceil_f32(1.0/sqrt(100.0))+2ULP=0x3DCCCCCF.
    // For division, 1.0/0x3DCCCCCF=9.9999983608725376739278142322684 rounded to f32 0x411FFFFE or 0x411FFFFF,
    // 1.0/0x3DCCCCCA=10.000002086163002207516386565905 rounded to f32 0x41200002 or 0x41200003.
    { input: 100, expected: [reinterpretU32AsF32(0x411FFFFE), reinterpretU32AsF32(0x41200003)] },  // ~10
  ] as ScalarToIntervalCase[],
  f16: [
    // 0.01 rounded to f16 0x211E or 0x211F.
    // For inverseSqrt interval, floor_f16(1.0/sqrt(0x211F))-2ULP=0x48FD,
    // ceil_f16(1.0/sqrt(0x211E))+2ULP=0x4903.
    // For division, 1.0/0x4903=0.09976617303195635229929851909587 rounded to f16 0x2E62 or 0x2E63,
    // 1.0/0x48FD=0.10023492560689115113547376664056 rounded to f16 0x2E6A or 0x2E6B.
    { input: 0.01, expected: [reinterpretU16AsF16(0x2E62), reinterpretU16AsF16(0x2E6B)] },  // ~0.1
    // For inverseSqrt interval, 1.0/sqrt(1.0)-2ULP=0x3BFE, 1.0/sqrt(1.0)+2ULP=0x3C01.
    // For division, 1.0/0x3C01=0.99902439024390243902439024390244 rounded to f16 0x3BFE or 0x3BFF,
    // 1.0/0x3BFE=1.000977517106549364613880742913 rounded to f16 0x3C01 or 0x3C02.
    { input: 1, expected: [reinterpretU16AsF16(0x3BFE), reinterpretU16AsF16(0x3C02)] },  // ~1
    // For inverseSqrt interval, 1.0/sqrt(4.0)-2ULP=0x37FE, 1.0/sqrt(4.0)+2ULP=0x3801.
    // For division, 1.0/0x3801=1.9980487804878048780487804878049 rounded to f16 0x3FFE or 0x3FFF,
    // 1.0/0x37FE=2.001955034213098729227761485826 rounded to f16 0x4001 or 0x4002.
    { input: 4, expected: [reinterpretU16AsF16(0x3FFE), reinterpretU16AsF16(0x4002)] },  // ~2
    // For inverseSqrt interval, floor_f16(1.0/sqrt(100.0))-2ULP=0x2E64,
    // ceil_f16(1.0/sqrt(100.0))+2ULP=0x2E69.
    // For division, 1.0/0x2E69=9.9841560024374942258493264279108 rounded to f16 0x48FD or 0x48FE,
    // 1.0/0x2E64=10.014669926650366748166259168704 rounded to f16 0x4901 or 0x4902.
    { input: 100, expected: [reinterpretU16AsF16(0x48FD), reinterpretU16AsF16(0x4902)] },  // ~10
  ] as ScalarToIntervalCase[],
} as const;

g.test('sqrtInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Cases that input and/or result not exactly representable
          ...kSqrtIntervalCases[p.trait],

          // Cases out of definition domain
          { input: -1, expected: kUnboundedBounds },
          { input: 0, expected: kUnboundedBounds },
          { input: constants.positive.infinity, expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    // The expected error interval is inherited from 1.0 / inverseSqrt(x), the 2.5ULP for division
    // is handled here.
    const error = (n: number): number => {
      return 2.5 * trait.oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.sqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.sqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // All of these are hard coded, since the error intervals are difficult to
      // express in a closed human--readable form.
      // Some easy looking cases like f(x = -π|π) = 0 are actually quite
      // difficult. This is because the interval is calculated from the results
      // of sin(x)/cos(x), which becomes very messy at x = -π|π, since π is
      // irrational, thus does not have an exact representation as a f32.
      //
      // Even at 0, which has a precise f32 value, there is still the problem
      // that result of sin(0) and cos(0) will be intervals due to the inherited
      // nature of errors, so the proper interval will be an interval calculated
      // from dividing an interval by another interval and applying an error
      // function to that.
      //
      // This complexity is why the entire interval framework was developed.
      //
      // The examples here have been manually traced to confirm the expectation
      // values are correct.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: kValue.f32.negative.pi.whole, expected: [reinterpretU64AsF64(0xbf40_02bc_9000_0000n), reinterpretU64AsF64(0x3f40_0144_f000_0000n)] },  // ~0.0
      { input: kValue.f32.negative.pi.half, expected: kUnboundedBounds },
      { input: 0, expected: [reinterpretU64AsF64(0xbf40_0200_b000_0000n), reinterpretU64AsF64(0x3f40_0200_b000_0000n)] },  // ~0.0
      { input: kValue.f32.positive.pi.half, expected: kUnboundedBounds },
      { input: kValue.f32.positive.pi.whole, expected: [reinterpretU64AsF64(0xbf40_0144_f000_0000n), reinterpretU64AsF64(0x3f40_02bc_9000_0000n)] },  // ~0.0
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.tanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.tanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.negative.min, expected: kUnboundedBounds },
      { input: -1, expected: [reinterpretU64AsF64(0xbfe8_5efd_1000_0000n), reinterpretU64AsF64(0xbfe8_5ef8_9000_0000n)] },  // ~-0.7615...
      { input: 0, expected: [reinterpretU64AsF64(0xbe8c_0000_b000_0000n), reinterpretU64AsF64(0x3e8c_0000_b000_0000n)] },  // ~0
      { input: 1, expected: [reinterpretU64AsF64(0x3fe8_5ef8_9000_0000n), reinterpretU64AsF64(0x3fe8_5efd_1000_0000n)] },  // ~0.7615...
      { input: kValue.f32.positive.max, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.tanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.tanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('truncInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 0 },
      { input: 0.9, expected: 0 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 1 },
      { input: 1.9, expected: 1 },
      { input: -0.1, expected: 0 },
      { input: -0.9, expected: 0 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -1 },
      { input: -1.9, expected: -1 },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kUnboundedBounds },
      { input: kValue.f32.infinity.negative, expected: kUnboundedBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: 0 },
      { input: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, expected: 0 },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.max, expected: 0 },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.truncInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.truncInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarPairToIntervalCase {
  // input is a pair of independent values, not a range, so should not be
  // converted to a FPInterval.
  input: [number, number];
  expected: number | IntervalBounds;
}

// prettier-ignore
const kAdditionInterval64BitsNormalCases = {
  f32: [
    // 0.1 falls between f32 0x3DCCCCCC and 0x3DCCCCCD
    { input: [0.1, 0], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    { input: [0, 0.1], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    // -0.1 falls between f32 0xBDCCCCCD and 0xBDCCCCCC
    { input: [-0.1, 0], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    { input: [0, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    // f32 0x3DCCCCCC+0x3DCCCCCC=0x3E4CCCCC, 0x3DCCCCCD+0x3DCCCCCD=0x3E4CCCCD
    { input: [0.1, 0.1], expected: [reinterpretU32AsF32(0x3e4ccccc), reinterpretU32AsF32(0x3e4ccccd)] },  // ~0.2
    // f32 0xBDCCCCCD+0xBDCCCCCD=0xBE4CCCCD, 0xBDCCCCCC+0xBDCCCCCC=0xBE4CCCCD
    { input: [-0.1, -0.1], expected: [reinterpretU32AsF32(0xbe4ccccd), reinterpretU32AsF32(0xbe4ccccc)] },  // ~-0.2
    // 0.1+(-0.1) expect f32 interval [0x3DCCCCCC+0xBDCCCCCD, 0x3DCCCCCD+0xBDCCCCCC]
    { input: [0.1, -0.1], expected: [reinterpretU32AsF32(0x3dcccccc)+reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0x3dcccccd)+reinterpretU32AsF32(0xbdcccccc)] },  // ~0.0
    // -0.1+0.1 expect f32 interval [0xBDCCCCCD+0x3DCCCCCC, 0xBDCCCCCC+0x3DCCCCCD]
    { input: [-0.1, 0.1], expected: [reinterpretU32AsF32(0xbdcccccd)+reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0xbdcccccc)+reinterpretU32AsF32(0x3dcccccd)] },  // ~0.0
  ] as ScalarPairToIntervalCase[],
  f16: [
    // 0.1 falls between f16 0x2E66 and 0x2E67
    { input: [0.1, 0], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    { input: [0, 0.1], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    // -0.1 falls between f16 0xAE67 and 0xAE66
    { input: [-0.1, 0], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    { input: [0, -0.1], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    // f16 0x2E66+0x2E66=0x3266, 0x2E67+0x2E67=0x3267
    { input: [0.1, 0.1], expected: [reinterpretU16AsF16(0x3266), reinterpretU16AsF16(0x3267)] },  // ~0.2
    // f16 0xAE67+0xAE67=0xB267, 0xAE66+0xAE66=0xB266
    { input: [-0.1, -0.1], expected: [reinterpretU16AsF16(0xb267), reinterpretU16AsF16(0xb266)] },  // ~-0.2
    // 0.1+(-0.1) expect f16 interval [0x2E66+0xAE67, 0x2E67+0xAE66]
    { input: [0.1, -0.1], expected: [reinterpretU16AsF16(0x2e66)+reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0x2e67)+reinterpretU16AsF16(0xae66)] },  // ~0.0
    // -0.1+0.1 expect f16 interval [0xAE67+0x2E66, 0xAE66+0x2E67]
    { input: [-0.1, 0.1], expected: [reinterpretU16AsF16(0xae67)+reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0xae66)+reinterpretU16AsF16(0x2e67)] },  // ~0.0
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('additionInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 0], expected: 0 },
          { input: [1, 0], expected: 1 },
          { input: [0, 1], expected: 1 },
          { input: [-1, 0], expected: -1 },
          { input: [0, -1], expected: -1 },
          { input: [1, 1], expected: 2 },
          { input: [1, -1], expected: 0 },
          { input: [-1, 1], expected: 0 },
          { input: [-1, -1], expected: -2 },

          // 64-bit normals that can not be exactly represented
          ...kAdditionInterval64BitsNormalCases[p.trait],

          // Subnormals
          { input: [constants.positive.subnormal.max, 0], expected: [0, constants.positive.subnormal.max] },
          { input: [0, constants.positive.subnormal.max], expected: [0, constants.positive.subnormal.max] },
          { input: [constants.positive.subnormal.min, 0], expected: [0, constants.positive.subnormal.min] },
          { input: [0, constants.positive.subnormal.min], expected: [0, constants.positive.subnormal.min] },
          { input: [constants.negative.subnormal.max, 0], expected: [constants.negative.subnormal.max, 0] },
          { input: [0, constants.negative.subnormal.max], expected: [constants.negative.subnormal.max, 0] },
          { input: [constants.negative.subnormal.min, 0], expected: [constants.negative.subnormal.min, 0] },
          { input: [0, constants.negative.subnormal.min], expected: [constants.negative.subnormal.min, 0] },

          // Infinities
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.additionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.additionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// Cases for Atan2Interval. The positive x & y quadrant is tested in more detail, and the other
// quadrants are spot checked that values are pointing in the right direction.
// Note: atan2's parameters are labelled (y, x) instead of (x, y)
// prettier-ignore
const kAtan2IntervalCases = {
  // atan has 4096ULP error boundary for f32.
  f32: [
    // positive y, positive x
    // √3 rounded to f32 0x3FDDB3D7, atan2(1, 0x3FDDB3D7)=0.52359877749051820266056630237827 ~ pi/6 rounded to f32 0x3F060A91 or 0x3F060A92,
    // kValue.f32.positive.pi.sixth is 0x3F060A92.
    { input: [1, reinterpretU32AsF32(0x3fddb3d7)], expected: [kMinusNULPFunctions['f32'](kValue.f32.positive.pi.sixth, 4097), kPlusNULPFunctions['f32'](kValue.f32.positive.pi.sixth, 4096)] },
    // atan2(1, 1)=0.78539816339744830961566084581988 ~ pi/4 rounded to f32 0x3F490FDA or 0x3F490FDB,
    // kValue.f32.positive.pi.quarter is 0x3F490FDB.
    { input: [1, 1], expected: [kMinusNULPFunctions['f32'](kValue.f32.positive.pi.quarter, 4097), kPlusNULPFunctions['f32'](kValue.f32.positive.pi.quarter, 4096)] },
    // √3 rounded to f32 0x3FDDB3D7, atan2(0x3FDDB3D7, 1) = 1.0471975493043784165707553892615 ~ pi/3 rounded to f32 0x3F860A91 or 0x3F860A92,
    // kValue.f32.positive.pi.third is 0x3F860A92.
    { input: [reinterpretU32AsF32(0x3fddb3d7), 1], expected: [kMinusNULPFunctions['f32'](kValue.f32.positive.pi.third, 4097), kPlusNULPFunctions['f32'](kValue.f32.positive.pi.third, 4096)] },

    // positive y, negative x
    // atan2(1, -1)=pi*3/4=2.3561944901923449288469825374591 rounded to f32 0x4016CBE3 or 0x4016CBE4,
    // kValue.f32.positive.pi.three_quarters is 0x4016CBE4.
    { input: [1, -1], expected: [kMinusNULPFunctions['f32'](kValue.f32.positive.pi.three_quarters, 4097), kPlusNULPFunctions['f32'](kValue.f32.positive.pi.three_quarters, 4096)] },

    // negative y, negative x
    // atan2(-1, -1)=-pi*3/4=-2.3561944901923449288469825374591 rounded to f32 0xC016CBE4 or 0xC016CBE3,
    // kValue.f32.negative.pi.three_quarters is 0xC016CBE4.
    { input: [-1, -1], expected: [kMinusNULPFunctions['f32'](kValue.f32.negative.pi.three_quarters, 4096), kPlusNULPFunctions['f32'](kValue.f32.negative.pi.three_quarters, 4097)] },

    // negative y, positive x
    // atan2(-1, 1)=-pi/4=-0.78539816339744830961566084581988 rounded to f32 0xBF490FDB or 0xBF490FDA,
    // kValue.f32.negative.pi.quarter is 0xBF490FDB.
    { input: [-1, 1], expected: [kMinusNULPFunctions['f32'](kValue.f32.negative.pi.quarter, 4096), kPlusNULPFunctions['f32'](kValue.f32.negative.pi.quarter, 4097)] },

    // When y/x ~ 0, test that ULP applied to result of atan2, not the intermediate y/x value.
    // y/x ~ 0, y<0, x<0, atan2(y,x) ~ -pi rounded to f32 0xC0490FDB or 0xC0490FDA,
    // kValue.f32.negative.pi.whole is 0xC0490FDB.
    {input: [kValue.f32.negative.max, -1], expected: [kMinusNULPFunctions['f32'](kValue.f32.negative.pi.whole, 4096), kPlusNULPFunctions['f32'](kValue.f32.negative.pi.whole, 4097)] },
    // y/x ~ 0, y>0, x<0, atan2(y,x) ~ pi rounded to f32 0x40490FDA or 0x40490FDB,
    // kValue.f32.positive.pi.whole is 0x40490FDB.
    {input: [kValue.f32.positive.min, -1], expected: [kMinusNULPFunctions['f32'](kValue.f32.positive.pi.whole, 4097), kPlusNULPFunctions['f32'](kValue.f32.positive.pi.whole, 4096)] },
  ] as ScalarPairToIntervalCase[],
  // atan has 5ULP error boundary for f16.
  f16: [
    // positive y, positive x
    // √3 rounded to f16 0x3EED, atan2(1, 0x3EED)=0.52375018906301191131992842392268 ~ pi/6 rounded to f16 0x3830 or 0x3831,
    // kValue.f16.positive.pi.sixth is 0x3830.
    { input: [1, reinterpretU16AsF16(0x3eed)], expected: [kMinusNULPFunctions['f16'](kValue.f16.positive.pi.sixth, 5), kPlusNULPFunctions['f16'](kValue.f16.positive.pi.sixth, 6)] },
    // atan2(1, 1)=0.78539816339744830961566084581988 ~ pi/4 rounded to f16 0x3A48 or 0x3A49,
    // kValue.f16.positive.pi.quarter is 0x3A48.
    { input: [1, 1], expected: [kMinusNULPFunctions['f16'](kValue.f16.positive.pi.quarter, 5), kPlusNULPFunctions['f16'](kValue.f16.positive.pi.quarter, 6)] },
    // √3 rounded to f16 0x3EED, atan2(0x3EED, 1) = 1.0470461377318847079113932677171 ~ pi/3 rounded to f16 0x3C30 or 0x3C31,
    // kValue.f16.positive.pi.third is 0x3C30.
    { input: [reinterpretU16AsF16(0x3eed), 1], expected: [kMinusNULPFunctions['f16'](kValue.f16.positive.pi.third, 5), kPlusNULPFunctions['f16'](kValue.f16.positive.pi.third, 6)] },

    // positive y, negative x
    // atan2(1, -1)=pi*3/4=2.3561944901923449288469825374591 rounded to f16 0x40B6 or 0x40B7,
    // kValue.f16.positive.pi.three_quarters is 0x40B6.
    { input: [1, -1], expected: [kMinusNULPFunctions['f16'](kValue.f16.positive.pi.three_quarters, 5), kPlusNULPFunctions['f16'](kValue.f16.positive.pi.three_quarters, 6)] },

    // negative y, negative x
    // atan2(-1, -1)=-pi*3/4=-2.3561944901923449288469825374591 rounded to f16 0xC0B7 or 0xC0B6,
    // kValue.f16.negative.pi.three_quarters is 0xC0B6.
    { input: [-1, -1], expected: [kMinusNULPFunctions['f16'](kValue.f16.negative.pi.three_quarters, 6), kPlusNULPFunctions['f16'](kValue.f16.negative.pi.three_quarters, 5)] },

    // negative y, positive x
    // atan2(-1, 1)=-pi/4=-0.78539816339744830961566084581988 rounded to f16 0xBA49 or 0xBA48,
    // kValue.f16.negative.pi.quarter is 0xBA48.
    { input: [-1, 1], expected: [kMinusNULPFunctions['f16'](kValue.f16.negative.pi.quarter, 6), kPlusNULPFunctions['f16'](kValue.f16.negative.pi.quarter, 5)] },

    // When y/x ~ 0, test that ULP applied to result of atan2, not the intermediate y/x value.
    // y/x ~ 0, y<0, x<0, atan2(y,x) ~ -pi rounded to f16 0xC249 or 0xC248,
    // kValue.f16.negative.pi.whole is 0xC248.
    {input: [kValue.f16.negative.max, -1], expected: [kMinusNULPFunctions['f16'](kValue.f16.negative.pi.whole, 6), kPlusNULPFunctions['f16'](kValue.f16.negative.pi.whole, 5)] },
    // y/x ~ 0, y>0, x<0, atan2(y,x) ~ pi rounded to f16 0x4248 or 0x4249,
    // kValue.f16.positive.pi.whole is 0x4248.
    {input: [kValue.f16.positive.min, -1], expected: [kMinusNULPFunctions['f16'](kValue.f16.positive.pi.whole, 5), kPlusNULPFunctions['f16'](kValue.f16.positive.pi.whole, 6)] },
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('atan2Interval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const constants = FP[p.trait].constants();
        // prettier-ignore
        return [
          ...kAtan2IntervalCases[p.trait],

          // Cases that y out of bound.
          // positive y, positive x
          { input: [Number.POSITIVE_INFINITY, 1], expected: kUnboundedBounds },
          // positive y, negative x
          { input: [Number.POSITIVE_INFINITY, -1], expected: kUnboundedBounds },
          // negative y, negative x
          { input: [Number.NEGATIVE_INFINITY, -1], expected: kUnboundedBounds },
          // negative y, positive x
          { input: [Number.NEGATIVE_INFINITY, 1], expected: kUnboundedBounds },

          // Discontinuity @ origin (0,0)
          { input: [0, 0], expected: kUnboundedBounds },
          { input: [0, constants.positive.subnormal.max], expected: kUnboundedBounds },
          { input: [0, constants.negative.subnormal.min], expected: kUnboundedBounds },
          { input: [0, constants.positive.min], expected: kUnboundedBounds },
          { input: [0, constants.negative.max], expected: kUnboundedBounds },
          { input: [0, constants.positive.max], expected: kUnboundedBounds },
          { input: [0, constants.negative.min], expected: kUnboundedBounds },
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [0, 1], expected: kUnboundedBounds },
          { input: [constants.positive.subnormal.max, 1], expected: kUnboundedBounds },
          { input: [constants.negative.subnormal.min, 1], expected: kUnboundedBounds },

          // Very large |x| values should cause kUnboundedBounds to be returned, due to the restrictions on division
          { input: [1, constants.positive.max], expected: kUnboundedBounds },
          { input: [1, constants.positive.nearest_max], expected: kUnboundedBounds },
          { input: [1, constants.negative.min], expected: kUnboundedBounds },
          { input: [1, constants.negative.nearest_min], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const [y, x] = t.params.input;
    const expected = trait.toInterval(t.params.expected);
    const got = trait.atan2Interval(y, x);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.atan2Interval(${y}, ${x}) returned ${got}]. Expected ${expected}`
    );
  });

g.test('distanceIntervalScalar_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable  form due to the inherited nature
      // of the errors.
      //
      // distance(x, y), where x - y = 0 has an acceptance interval of kUnboundedBounds,
      // because distance(x, y) = length(x - y), and length(0) = kUnboundedBounds
      { input: [0, 0], expected: kUnboundedBounds },
      { input: [1.0, 0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [0.0, 1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [1.0, 1.0], expected: kUnboundedBounds },
      { input: [-0.0, -1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [0.0, -1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [-1.0, -1.0], expected: kUnboundedBounds },
      { input: [0.1, 0], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [0, 0.1], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [-0.1, 0], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [0, -0.1], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [10.0, 0], expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [0, 10.0], expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [-10.0, 0], expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [0, -10.0], expected: [reinterpretU64AsF64(0x4023_ffff_7000_0000n), reinterpretU64AsF64(0x4024_0000_b000_0000n)] },  // ~10

      // Subnormal Cases
      { input: [kValue.f32.subnormal.negative.min, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.subnormal.positive.max, 0], expected: kUnboundedBounds },

      // Edge cases
      { input: [kValue.f32.infinity.positive, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.negative.min, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.negative.max, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.positive.min, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.positive.max, 0], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.distanceInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.distanceInterval(${t.params.input[0]}, ${t.params.input[1]}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kDivisionInterval64BitsNormalCases = {
  f32: [
    // Zero divided by any non-zero finite value results in zero.
    { input: [0, 0.1], expected: 0 },
    { input: [0, -0.1], expected: 0 },
    // 0.1 rounded to f32 0x3DCCCCCC or 0x3DCCCCCD,
    // 1.0/0x3DCCCCCD = 9.9999998509883902204460179966303 rounded to f32 0x411FFFFF or 0x41200000,
    // 1.0/0x3DCCCCCC = 10.000000596046483527138934924167 rounded to f32 0x41200000 or 0x41200001.
    { input: [1, 0.1], expected: [reinterpretU32AsF32(0x411fffff), reinterpretU32AsF32(0x41200001)] },  // ~10.0
    // The same for -1/-0.1
    { input: [-1, -0.1], expected: [reinterpretU32AsF32(0x411fffff), reinterpretU32AsF32(0x41200001)] },  // ~10.0
    // -10.000000596046483527138934924167 rounded to f32 0xC1200001 or 0xC1200000,
    // -9.9999998509883902204460179966303 rounded to f32 0xC1200000 or 0xC11FFFFF.
    { input: [-1, 0.1], expected: [reinterpretU32AsF32(0xc1200001), reinterpretU32AsF32(0xc11fffff)] },  // ~-10.0
    { input: [1, -0.1], expected: [reinterpretU32AsF32(0xc1200001), reinterpretU32AsF32(0xc11fffff)] },  // ~-10.0
    // Cases that expected interval larger than +-1ULP.
    // 0.000001 rounded to f32 0x358637BD or 0x358637BE,
    // 1.0/0x358637BE = 999999.88883793195700674522548684 rounded to f32 0x497423FE or 0x497423FF,
    // 1.0/0x358637BD = 1000000.0025247573063743994399971 rounded to f32 0x49742400 or 0x49742401.
    { input: [1, 0.000001], expected: [reinterpretU32AsF32(0x497423fe), reinterpretU32AsF32(0x49742401)] },  // ~1000000.0
    { input: [1, -0.000001], expected: [reinterpretU32AsF32(0xc9742401), reinterpretU32AsF32(0xc97423fe)] },  // ~-1000000.0
  ] as ScalarPairToIntervalCase[],
  f16: [
    // Zero divided by any non-zero finite value results in zero.
    { input: [0, 0.1], expected: 0 },
    { input: [0, -0.1], expected: 0 },
    // 0.1 rounded to f16 0x2E66 or 0x2E67,
    // 1.0/0x2E67 = 9.9963392312385600976205003050641 rounded to f16 0x48FF or 0x4900,
    // 1.0/0x2E66 = 10.002442002442002442002442002442 rounded to f16 0x4900 or 0x4901.
    { input: [1, 0.1], expected: [reinterpretU16AsF16(0x48ff), reinterpretU16AsF16(0x4901)] },  // ~10.0
    // The same for -1/-0.1
    { input: [-1, -0.1], expected: [reinterpretU16AsF16(0x48ff), reinterpretU16AsF16(0x4901)] },  // ~10.0
    // -10.002442002442002442002442002442 rounded to f16 0xC901 or 0xC900,
    // -9.9963392312385600976205003050641 rounded to f16 0xC900 or 0xC8FF.
    { input: [-1, 0.1], expected: [reinterpretU16AsF16(0xc901), reinterpretU16AsF16(0xc8ff)] },  // ~-10.0
    { input: [1, -0.1], expected: [reinterpretU16AsF16(0xc901), reinterpretU16AsF16(0xc8ff)] },  // ~-10.0
    // Cases that expected interval larger than +-1ULP.
    // 0.001 rounded to f16 0x1418 or 0x1419,
    // 1.0/0x1419 = 999.59580552907535977846384072716 rounded to f16 0x63CF or 0x63D0,
    // 1.0/0x1418 = 1000.5496183206106870229007633588 rounded to f16 0x63D1 or 0x63D2.
    { input: [1, 0.001], expected: [reinterpretU16AsF16(0x63cf), reinterpretU16AsF16(0x63d2)] },  // ~1000.0
    { input: [1, -0.001], expected: [reinterpretU16AsF16(0xe3d2), reinterpretU16AsF16(0xe3cf)] },  // ~-1000.0
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('divisionInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 1], expected: 0 },
          { input: [0, -1], expected: 0 },
          { input: [1, 1], expected: 1 },
          { input: [1, -1], expected: -1 },
          { input: [-1, 1], expected: -1 },
          { input: [-1, -1], expected: 1 },
          { input: [4, 2], expected: 2 },
          { input: [-4, 2], expected: -2 },
          { input: [4, -2], expected: -2 },
          { input: [-4, -2], expected: 2 },

          // 64-bit normals that can not be exactly represented
          ...kDivisionInterval64BitsNormalCases[p.trait],

          // Denominator out of range
          { input: [1, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [1, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [1, constants.positive.max], expected: kUnboundedBounds },
          { input: [1, constants.negative.min], expected: kUnboundedBounds },
          { input: [1, 0], expected: kUnboundedBounds },
          { input: [1, constants.positive.subnormal.max], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];

    const error = (n: number): number => {
      return 2.5 * trait.oneULP(n);
    };

    const [x, y] = t.params.input;
    t.params.expected = applyError(t.params.expected, error);
    const expected = trait.toInterval(t.params.expected);

    const got = trait.divisionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.divisionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ldexpInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [0, 1], expected: 0 },
      { input: [0, -1], expected: 0 },
      { input: [1, 1], expected: 2 },
      { input: [1, -1], expected: 0.5 },
      { input: [-1, 1], expected: -2 },
      { input: [-1, -1], expected: -0.5 },

      // 64-bit normals
      { input: [0, 0.1], expected: 0 },
      { input: [0, -0.1], expected: 0 },
      { input: [1.0000000001, 1], expected: [2, kPlusNULPFunctions['f32'](2, 2)] },  // ~2, additional ULP error due to first param not being f32 precise
      { input: [-1.0000000001, 1], expected: [kMinusNULPFunctions['f32'](-2, 2), -2] },  // ~-2, additional ULP error due to first param not being f32 precise

      // Edge Cases
      { input: [1.9999998807907104, 127], expected: kValue.f32.positive.max },
      { input: [1, -126], expected: kValue.f32.positive.min },
      { input: [0.9999998807907104, -126], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [1.1920928955078125e-07, -126], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [-1.1920928955078125e-07, -126], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [-0.9999998807907104, -126], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [-1, -126], expected: kValue.f32.negative.max },
      { input: [-1.9999998807907104, 127], expected: kValue.f32.negative.min },

      // Out of Bounds
      { input: [1, 128], expected: kUnboundedBounds },
      { input: [-1, 128], expected: kUnboundedBounds },
      { input: [100, 126], expected: kUnboundedBounds },
      { input: [-100, 126], expected: kUnboundedBounds },
      { input: [kValue.f32.positive.max, kValue.i32.positive.max], expected: kUnboundedBounds },
      { input: [kValue.f32.negative.min, kValue.i32.positive.max], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.ldexpInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.ldexpInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kMaxInterval64BitsCases = {
  f32: [
    // 0.1 rounded to f32 0x3DCCCCCC or 0x3DCCCCCD, -0.1 rounded to f32 0xBDCCCCCD or 0xBDCCCCCC
    { input: [0.1, 0], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [0, 0.1], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [0.1, 0.1], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [0.1, -0.1], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [-0.1, 0.1], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [-0.1, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
  ] as ScalarPairToIntervalCase[],
  f16: [
    // 0.1 rounded to f16 0x2E66 or 0x2E67, -0.1 rounded to f16 0xAE67 or 0xAE66
    { input: [0.1, 0], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [0, 0.1], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [0.1, 0.1], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [0.1, -0.1], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [-0.1, 0.1], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [-0.1, -0.1], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('maxInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 0], expected: 0 },
          { input: [1, 0], expected: 1 },
          { input: [0, 1], expected: 1 },
          { input: [-1, 0], expected: 0 },
          { input: [0, -1], expected: 0 },
          { input: [1, 1], expected: 1 },
          { input: [1, -1], expected: 1 },
          { input: [-1, 1], expected: 1 },
          { input: [-1, -1], expected: -1 },

          // 64-bit normals that not exactly representable
          { input: [-0.1, 0], expected: 0 },
          { input: [0, -0.1], expected: 0 },
          ...kMaxInterval64BitsCases[p.trait],

          // Representable subnormals
          { input: [constants.positive.subnormal.max, 0], expected: [0, constants.positive.subnormal.max] },
          { input: [0, constants.positive.subnormal.max], expected: [0, constants.positive.subnormal.max] },
          { input: [constants.positive.subnormal.min, 0], expected: [0, constants.positive.subnormal.min] },
          { input: [0, constants.positive.subnormal.min], expected: [0, constants.positive.subnormal.min] },
          { input: [constants.negative.subnormal.max, 0], expected: [constants.negative.subnormal.max, 0] },
          { input: [0, constants.negative.subnormal.max], expected: [constants.negative.subnormal.max, 0] },
          { input: [constants.negative.subnormal.min, 0], expected: [constants.negative.subnormal.min, 0] },
          { input: [0, constants.negative.subnormal.min], expected: [constants.negative.subnormal.min, 0] },
          { input: [1, constants.positive.subnormal.max], expected: 1 },
          { input: [constants.negative.subnormal.min, constants.positive.subnormal.max], expected: [constants.negative.subnormal.min, constants.positive.subnormal.max] },

          // Infinities
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const [x, y] = t.params.input;
    const expected = trait.toInterval(t.params.expected);
    const got = trait.maxInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.maxInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kMinInterval64BitsCases = {
  f32: [
    // 0.1 rounded to f32 0x3DCCCCCC or 0x3DCCCCCD, -0.1 rounded to f32 0xBDCCCCCD or 0xBDCCCCCC
    { input: [-0.1, 0], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
    { input: [0, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
    { input: [0.1, 0.1], expected: [kMinusOneULPFunctions['f32'](reinterpretU32AsF32(0x3dcccccd)), reinterpretU32AsF32(0x3dcccccd)] },  // ~0.1
    { input: [0.1, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
    { input: [-0.1, 0.1], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
    { input: [-0.1, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd), kPlusOneULPFunctions['f32'](reinterpretU32AsF32(0xbdcccccd))] },  // ~-0.1
  ] as ScalarPairToIntervalCase[],
  f16: [
    // 0.1 rounded to f16 0x2E66 or 0x2E67, -0.1 rounded to f16 0xAE67 or 0xAE66
    { input: [-0.1, 0], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
    { input: [0, -0.1], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
    { input: [0.1, 0.1], expected: [reinterpretU16AsF16(0x2e66), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0x2e66))] },  // ~0.1
    { input: [0.1, -0.1], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
    { input: [-0.1, 0.1], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
    { input: [-0.1, -0.1], expected: [reinterpretU16AsF16(0xae67), kPlusOneULPFunctions['f16'](reinterpretU16AsF16(0xae67))] },  // ~-0.1
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('minInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 0], expected: 0 },
          { input: [1, 0], expected: 0 },
          { input: [0, 1], expected: 0 },
          { input: [-1, 0], expected: -1 },
          { input: [0, -1], expected: -1 },
          { input: [1, 1], expected: 1 },
          { input: [1, -1], expected: -1 },
          { input: [-1, 1], expected: -1 },
          { input: [-1, -1], expected: -1 },

          // 64-bit normals that not exactly representable
          { input: [0.1, 0], expected: 0 },
          { input: [0, 0.1], expected: 0 },
          ...kMinInterval64BitsCases[p.trait],

          // Representable subnormals
          { input: [constants.positive.subnormal.max, 0], expected: [0, constants.positive.subnormal.max] },
          { input: [0, constants.positive.subnormal.max], expected: [0, constants.positive.subnormal.max] },
          { input: [constants.positive.subnormal.min, 0], expected: [0, constants.positive.subnormal.min] },
          { input: [0, constants.positive.subnormal.min], expected: [0, constants.positive.subnormal.min] },
          { input: [constants.negative.subnormal.max, 0], expected: [constants.negative.subnormal.max, 0] },
          { input: [0, constants.negative.subnormal.max], expected: [constants.negative.subnormal.max, 0] },
          { input: [constants.negative.subnormal.min, 0], expected: [constants.negative.subnormal.min, 0] },
          { input: [0, constants.negative.subnormal.min], expected: [constants.negative.subnormal.min, 0] },
          { input: [-1, constants.positive.subnormal.max], expected: -1 },
          { input: [constants.negative.subnormal.min, constants.positive.subnormal.max], expected: [constants.negative.subnormal.min, constants.positive.subnormal.max] },

          // Infinities
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const trait = FP[t.params.trait];
    const [x, y] = t.params.input;
    const expected = trait.toInterval(t.params.expected);
    const got = trait.minInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.minInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kMultiplicationInterval64BitsNormalCases = {
  f32: [
    // Finite values multiply zero result in zero
    { input: [0.1, 0], expected: 0 },
    { input: [0, 0.1], expected: 0 },
    { input: [-0.1, 0], expected: 0 },
    { input: [0, -0.1], expected: 0 },
    // 0.1 falls between f32 0x3DCCCCCC and 0x3DCCCCCD
    { input: [0.1, 1], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    { input: [-1, -0.1], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    // -0.1 falls between f32 0xBDCCCCCD and 0xBDCCCCCC
    { input: [-0.1, 1], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    { input: [-1, 0.1], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    // 0.1*0.1,
    // min result 0x3DCCCCCC*0x3DCCCCCC=0.00999999880790713952713681734167 rounded to f32 0x3C23D708 or 0x3C23D709,
    // max result 0x3DCCCCCD*0x3DCCCCCD=0.01000000029802322622044605108385 rounded to f32 0x3C23D70A or 0x3C23D70B.
    { input: [0.1, 0.1], expected: [reinterpretU32AsF32(0x3c23d708), reinterpretU32AsF32(0x3c23d70b)] },  // ~0.01
    { input: [-0.1, -0.1], expected: [reinterpretU32AsF32(0x3c23d708), reinterpretU32AsF32(0x3c23d70b)] },  // ~0.01
    // -0.01000000029802322622044605108385 rounded to f32 0xBC23D70B or 0xBC23D70A,
    // -0.00999999880790713952713681734167 rounded to f32 0xBC23D709 or 0xBC23D708.
    { input: [0.1, -0.1], expected: [reinterpretU32AsF32(0xbc23d70b), reinterpretU32AsF32(0xbc23d708)] },  // ~-0.01
    { input: [-0.1, 0.1], expected: [reinterpretU32AsF32(0xbc23d70b), reinterpretU32AsF32(0xbc23d708)] },  // ~-0.01
  ] as ScalarPairToIntervalCase[],
  f16: [
    // Finite values multiply zero result in zero
    { input: [0.1, 0], expected: 0 },
    { input: [0, 0.1], expected: 0 },
    { input: [-0.1, 0], expected: 0 },
    { input: [0, -0.1], expected: 0 },
    // 0.1 falls between f16 0x2E66 and 0x2E67
    { input: [0.1, 1], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    { input: [-1, -0.1], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    // -0.1 falls between f16 0xAE67 and 0xAE66
    { input: [-0.1, 1], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    { input: [-1, 0.1], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    // 0.1*0.1,
    // min result 0x2E66*0x2E66=0.00999511778354644775390625 rounded to f16 0x211E or 0x211F,
    // max result 0x2E67*0x2E67=0.0100073255598545074462890625 rounded to f16 0x211F or 0x2120.
    { input: [0.1, 0.1], expected: [reinterpretU16AsF16(0x211e), reinterpretU16AsF16(0x2120)] },  // ~0.01
    { input: [-0.1, -0.1], expected: [reinterpretU16AsF16(0x211e), reinterpretU16AsF16(0x2120)] },  // ~0.01
    // -0.0100073255598545074462890625 rounded to f16 0xA120 or 0xA11F,
    // -0.00999511778354644775390625 rounded to f16 0xA11F or 0xA11E.
    { input: [0.1, -0.1], expected: [reinterpretU16AsF16(0xa120), reinterpretU16AsF16(0xa11e)] },  // ~-0.01
    { input: [-0.1, 0.1], expected: [reinterpretU16AsF16(0xa120), reinterpretU16AsF16(0xa11e)] },  // ~-0.01
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('multiplicationInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 0], expected: 0 },
          { input: [1, 0], expected: 0 },
          { input: [0, 1], expected: 0 },
          { input: [-1, 0], expected: 0 },
          { input: [0, -1], expected: 0 },
          { input: [1, 1], expected: 1 },
          { input: [1, -1], expected: -1 },
          { input: [-1, 1], expected: -1 },
          { input: [-1, -1], expected: 1 },
          { input: [2, 1], expected: 2 },
          { input: [1, -2], expected: -2 },
          { input: [-2, 1], expected: -2 },
          { input: [-2, -1], expected: 2 },
          { input: [2, 2], expected: 4 },
          { input: [2, -2], expected: -4 },
          { input: [-2, 2], expected: -4 },
          { input: [-2, -2], expected: 4 },

          // 64-bit normals that can not be exactly represented
          ...kMultiplicationInterval64BitsNormalCases[p.trait],

          // Infinities
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [1, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [-1, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [1, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [-1, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },

          // Edges
          { input: [constants.positive.max, constants.positive.max], expected: kUnboundedBounds },
          { input: [constants.negative.min, constants.negative.min], expected: kUnboundedBounds },
          { input: [constants.positive.max, constants.negative.min], expected: kUnboundedBounds },
          { input: [constants.negative.min, constants.positive.max], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.multiplicationInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.multiplicationInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('powInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: [-1, 0], expected: kUnboundedBounds },
      { input: [0, 0], expected: kUnboundedBounds },
      { input: [1, 0], expected: [kMinusNULPFunctions['f32'](1, 3), reinterpretU64AsF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [2, 0], expected: [kMinusNULPFunctions['f32'](1, 3), reinterpretU64AsF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [kValue.f32.positive.max, 0], expected: [kMinusNULPFunctions['f32'](1, 3), reinterpretU64AsF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [0, 1], expected: kUnboundedBounds },
      { input: [1, 1], expected: [reinterpretU64AsF64(0x3fef_fffe_dfff_fe00n), reinterpretU64AsF64(0x3ff0_0000_c000_0200n)] },  // ~1
      { input: [1, 100], expected: [reinterpretU64AsF64(0x3fef_ffba_3fff_3800n), reinterpretU64AsF64(0x3ff0_0023_2000_c800n)] },  // ~1
      { input: [1, kValue.f32.positive.max], expected: kUnboundedBounds },
      { input: [2, 1], expected: [reinterpretU64AsF64(0x3fff_fffe_a000_0200n), reinterpretU64AsF64(0x4000_0001_0000_0200n)] },  // ~2
      { input: [2, 2], expected: [reinterpretU64AsF64(0x400f_fffd_a000_0400n), reinterpretU64AsF64(0x4010_0001_a000_0400n)] },  // ~4
      { input: [10, 10], expected: [reinterpretU64AsF64(0x4202_a04f_51f7_7000n), reinterpretU64AsF64(0x4202_a070_ee08_e000n)] },  // ~10000000000
      { input: [10, 1], expected: [reinterpretU64AsF64(0x4023_fffe_0b65_8b00n), reinterpretU64AsF64(0x4024_0002_149a_7c00n)] },  // ~10
      { input: [kValue.f32.positive.max, 1], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.powInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.powInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('remainderInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 1], expected: [0, 0] },
      { input: [0, -1], expected: [0, 0] },
      { input: [1, 1], expected: [0, 1] },
      { input: [1, -1], expected: [0, 1] },
      { input: [-1, 1], expected: [-1, 0] },
      { input: [-1, -1], expected: [-1, 0] },
      { input: [4, 2], expected: [0, 2] },
      { input: [-4, 2], expected: [-2, 0] },
      { input: [4, -2], expected: [0, 2] },
      { input: [-4, -2], expected: [-2, 0] },
      { input: [2, 4], expected: [2, 2] },
      { input: [-2, 4], expected: [-2, -2] },
      { input: [2, -4], expected: [2, 2] },
      { input: [-2, -4], expected: [-2, -2] },

      // 64-bit normals
      { input: [0, 0.1], expected: [0, 0] },
      { input: [0, -0.1], expected: [0, 0] },
      { input: [1, 0.1], expected: [reinterpretU32AsF32(0xb4000000), reinterpretU32AsF32(0x3dccccd8)] }, // ~[0, 0.1]
      { input: [-1, 0.1], expected: [reinterpretU32AsF32(0xbdccccd8), reinterpretU32AsF32(0x34000000)] }, // ~[-0.1, 0]
      { input: [1, -0.1], expected: [reinterpretU32AsF32(0xb4000000), reinterpretU32AsF32(0x3dccccd8)] }, // ~[0, 0.1]
      { input: [-1, -0.1], expected: [reinterpretU32AsF32(0xbdccccd8), reinterpretU32AsF32(0x34000000)] }, // ~[-0.1, 0]

      // Denominator out of range
      { input: [1, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [1, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [1, kValue.f32.positive.max], expected: kUnboundedBounds },
      { input: [1, kValue.f32.negative.min], expected: kUnboundedBounds },
      { input: [1, 0], expected: kUnboundedBounds },
      { input: [1, kValue.f32.subnormal.positive.max], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.remainderInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.remainderInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('stepInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 1 },
      { input: [1, 1], expected: 1 },
      { input: [0, 1], expected: 1 },
      { input: [1, 0], expected: 0 },
      { input: [-1, -1], expected: 1 },
      { input: [0, -1], expected: 0 },
      { input: [-1, 0], expected: 1 },
      { input: [-1, 1], expected: 1 },
      { input: [1, -1], expected: 0 },

      // 64-bit normals
      { input: [0.1, 0.1], expected: [0, 1] },
      { input: [0, 0.1], expected: 1 },
      { input: [0.1, 0], expected: 0 },
      { input: [0.1, 1], expected: 1 },
      { input: [1, 0.1], expected: 0 },
      { input: [-0.1, -0.1], expected: [0, 1] },
      { input: [0, -0.1], expected: 0 },
      { input: [-0.1, 0], expected: 1 },
      { input: [-0.1, -1], expected: 0 },
      { input: [-1, -0.1], expected: 1 },

      // Subnormals
      { input: [0, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [0, kValue.f32.subnormal.positive.min], expected: 1 },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0, 1] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0, 1] },
      { input: [1, kValue.f32.subnormal.positive.max], expected: 0 },
      { input: [1, kValue.f32.subnormal.positive.min], expected: 0 },
      { input: [1, kValue.f32.subnormal.negative.max], expected: 0 },
      { input: [1, kValue.f32.subnormal.negative.min], expected: 0 },
      { input: [-1, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [-1, kValue.f32.subnormal.positive.min], expected: 1 },
      { input: [-1, kValue.f32.subnormal.negative.max], expected: 1 },
      { input: [-1, kValue.f32.subnormal.negative.min], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: 1 },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, 1], expected: 1 },
      { input: [kValue.f32.subnormal.positive.min, 1], expected: 1 },
      { input: [kValue.f32.subnormal.negative.max, 1], expected: 1 },
      { input: [kValue.f32.subnormal.negative.min, 1], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, -1], expected: 0 },
      { input: [kValue.f32.subnormal.positive.min, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.max, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.min, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min], expected: [0, 1] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [edge, x] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.stepInterval(edge, x);
    t.expect(
      objectEquals(expected, got),
      `f32.stepInterval(${edge}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

// prettier-ignore
const kSubtractionInterval64BitsNormalCases = {
  f32: [
    // 0.1 falls between f32 0x3DCCCCCC and 0x3DCCCCCD
    { input: [0.1, 0], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    { input: [0, -0.1], expected: [reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)] },
    // -0.1 falls between f32 0xBDCCCCCD and 0xBDCCCCCC
    { input: [-0.1, 0], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    { input: [0, 0.1], expected: [reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)] },
    // Expect f32 interval [0x3DCCCCCC-0x3DCCCCCD, 0x3DCCCCCD-0x3DCCCCCC]
    { input: [0.1, 0.1], expected: [reinterpretU32AsF32(0x3dcccccc)-reinterpretU32AsF32(0x3dcccccd), reinterpretU32AsF32(0x3dcccccd)-reinterpretU32AsF32(0x3dcccccc)] },
    // Expect f32 interval [0xBDCCCCCD-0xBDCCCCCC, 0xBDCCCCCC-0xBDCCCCCD]
    { input: [-0.1, -0.1], expected: [reinterpretU32AsF32(0xbdcccccd)-reinterpretU32AsF32(0xbdcccccc), reinterpretU32AsF32(0xbdcccccc)-reinterpretU32AsF32(0xbdcccccd)] },
    // Expect f32 interval [0x3DCCCCCC-0xBDCCCCCC, 0x3DCCCCCD-0xBDCCCCCD]
    { input: [0.1, -0.1], expected: [reinterpretU32AsF32(0x3dcccccc)-reinterpretU32AsF32(0xbdcccccc), reinterpretU32AsF32(0x3dcccccd)-reinterpretU32AsF32(0xbdcccccd)] },
    // Expect f32 interval [0xBDCCCCCD-0x3DCCCCCD, 0xBDCCCCCC-0x3DCCCCCC]
    { input: [-0.1, 0.1], expected: [reinterpretU32AsF32(0xbdcccccd)-reinterpretU32AsF32(0x3dcccccd), reinterpretU32AsF32(0xbdcccccc)-reinterpretU32AsF32(0x3dcccccc)] },
  ] as ScalarPairToIntervalCase[],
  f16: [
    // 0.1 falls between f16 0x2E66 and 0x2E67
    { input: [0.1, 0], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    { input: [0, -0.1], expected: [reinterpretU16AsF16(0x2e66), reinterpretU16AsF16(0x2e67)] },
    // -0.1 falls between f16 0xAE67 and 0xAE66
    { input: [-0.1, 0], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    { input: [0, 0.1], expected: [reinterpretU16AsF16(0xae67), reinterpretU16AsF16(0xae66)] },
    // Expect f16 interval [0x2E66-0x2E67, 0x2E67-0x2E66]
    { input: [0.1, 0.1], expected: [reinterpretU16AsF16(0x2e66)-reinterpretU16AsF16(0x2e67), reinterpretU16AsF16(0x2e67)-reinterpretU16AsF16(0x2e66)] },
    // Expect f16 interval [0xAE67-0xAE66, 0xAE66-0xAE67]
    { input: [-0.1, -0.1], expected: [reinterpretU16AsF16(0xae67)-reinterpretU16AsF16(0xae66), reinterpretU16AsF16(0xae66)-reinterpretU16AsF16(0xae67)] },
    // Expect f16 interval [0x2E66-0xAE66, 0x2E67-0xAE67]
    { input: [0.1, -0.1], expected: [reinterpretU16AsF16(0x2e66)-reinterpretU16AsF16(0xae66), reinterpretU16AsF16(0x2e67)-reinterpretU16AsF16(0xae67)] },
    // Expect f16 interval [0xAE67-0x2E67, 0xAE66-0x2E66]
    { input: [-0.1, 0.1], expected: [reinterpretU16AsF16(0xae67)-reinterpretU16AsF16(0x2e67), reinterpretU16AsF16(0xae66)-reinterpretU16AsF16(0x2e66)] },
  ] as ScalarPairToIntervalCase[],
} as const;

g.test('subtractionInterval')
  .params(u =>
    u
      .combine('trait', ['f32', 'f16'] as const)
      .beginSubcases()
      .expandWithParams<ScalarPairToIntervalCase>(p => {
        const trait = FP[p.trait];
        const constants = trait.constants();
        // prettier-ignore
        return [
          // Representable normals
          { input: [0, 0], expected: 0 },
          { input: [1, 0], expected: 1 },
          { input: [0, 1], expected: -1 },
          { input: [-1, 0], expected: -1 },
          { input: [0, -1], expected: 1 },
          { input: [1, 1], expected: 0 },
          { input: [1, -1], expected: 2 },
          { input: [-1, 1], expected: -2 },
          { input: [-1, -1], expected: 0 },

          // 64-bit normals that can not be exactly represented
          ...kSubtractionInterval64BitsNormalCases[p.trait],

          // Subnormals
          { input: [constants.positive.subnormal.max, 0], expected: [0, constants.positive.subnormal.max] },
          { input: [0, constants.positive.subnormal.max], expected: [constants.negative.subnormal.min, 0] },
          { input: [constants.positive.subnormal.min, 0], expected: [0, constants.positive.subnormal.min] },
          { input: [0, constants.positive.subnormal.min], expected: [constants.negative.subnormal.max, 0] },
          { input: [constants.negative.subnormal.max, 0], expected: [constants.negative.subnormal.max, 0] },
          { input: [0, constants.negative.subnormal.max], expected: [0, constants.positive.subnormal.min] },
          { input: [constants.negative.subnormal.min, 0], expected: [constants.negative.subnormal.min, 0] },
          { input: [0, constants.negative.subnormal.min], expected: [0, constants.positive.subnormal.max] },

          // Infinities
          { input: [0, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [0, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, 0], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.negative.infinity], expected: kUnboundedBounds },
          { input: [constants.negative.infinity, constants.positive.infinity], expected: kUnboundedBounds },
          { input: [constants.positive.infinity, constants.negative.infinity], expected: kUnboundedBounds },
        ];
      })
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const trait = FP[t.params.trait];
    const expected = trait.toInterval(t.params.expected);
    const got = trait.subtractionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `${t.params.trait}.subtractionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarTripleToIntervalCase {
  input: [number, number, number];
  expected: number | IntervalBounds;
}

g.test('clampMedianInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 0 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 1 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 1 },
      { input: [1, 10, 100], expected: 10 },
      { input: [10, 1, 100], expected: 10 },
      { input: [100, 1, 10], expected: 10 },
      { input: [-10, 1, 100], expected: 1 },
      { input: [10, 1, -100], expected: 1 },
      { input: [-10, 1, -100], expected: -10 },
      { input: [-10, -10, -10], expected: -10 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: 0 },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: 0 },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: 0 },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: kValue.f32.positive.max },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.clampMedianInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.clampMedianInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('clampMinMaxInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 0 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 0 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 1 },
      { input: [1, 10, 100], expected: 10 },
      { input: [10, 1, 100], expected: 10 },
      { input: [100, 1, 10], expected: 10 },
      { input: [-10, 1, 100], expected: 1 },
      { input: [10, 1, -100], expected: -100 },
      { input: [-10, 1, -100], expected: -100 },
      { input: [-10, -10, -10], expected: -10 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.clampMinMaxInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.clampMinMaxInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fmaInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 1 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 1 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 2 },
      { input: [1, 10, 100], expected: 110 },
      { input: [10, 1, 100], expected: 110 },
      { input: [100, 1, 10], expected: 110 },
      { input: [-10, 1, 100], expected: 90 },
      { input: [10, 1, -100], expected: -90 },
      { input: [-10, 1, -100], expected: -110 },
      { input: [-10, -10, -10], expected: 90 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: 0 },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: 0 },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [reinterpretU32AsF32(0x80000002), 0] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.fmaInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.fmaInterval(${t.params.input.join(',')}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixImpreciseInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: -1.0 },
      { input: [0.0, 1.0, 0.0], expected: 0.0 },
      { input: [0.0, 1.0, 0.1], expected: [reinterpretU64AsF64(0x3fb9_9999_8000_0000n), reinterpretU64AsF64(0x3fb9_9999_a000_0000n)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: 0.5 },
      { input: [0.0, 1.0, 0.9], expected: [reinterpretU64AsF64(0x3fec_cccc_c000_0000n), reinterpretU64AsF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: 1.0 },
      { input: [0.0, 1.0, 2.0], expected: 2.0 },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: 2.0 },
      { input: [1.0, 0.0, 0.0], expected: 1.0 },
      { input: [1.0, 0.0, 0.1], expected: [reinterpretU64AsF64(0x3fec_cccc_c000_0000n), reinterpretU64AsF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: 0.5 },
      { input: [1.0, 0.0, 0.9], expected: [reinterpretU64AsF64(0x3fb9_9999_0000_0000n), reinterpretU64AsF64(0x3fb9_999a_0000_0000n)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: 0.0 },
      { input: [1.0, 0.0, 2.0], expected: -1.0 },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: -10.0 },
      { input: [0.0, 10.0, 0.0], expected: 0.0 },
      { input: [0.0, 10.0, 0.1], expected: [reinterpretU64AsF64(0x3fef_ffff_e000_0000n), reinterpretU64AsF64(0x3ff0_0000_2000_0000n)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: 5.0 },
      { input: [0.0, 10.0, 0.9], expected: [reinterpretU64AsF64(0x4021_ffff_e000_0000n), reinterpretU64AsF64(0x4022_0000_2000_0000n)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: 10.0 },
      { input: [0.0, 10.0, 2.0], expected: 20.0 },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: -6.0 },
      { input: [2.0, 10.0, 0.0], expected: 2.0 },
      { input: [2.0, 10.0, 0.1], expected: [reinterpretU64AsF64(0x4006_6666_6000_0000n), reinterpretU64AsF64(0x4006_6666_8000_0000n)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: 6.0 },
      { input: [2.0, 10.0, 0.9], expected: [reinterpretU64AsF64(0x4022_6666_6000_0000n), reinterpretU64AsF64(0x4022_6666_8000_0000n)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: 10.0 },
      { input: [2.0, 10.0, 2.0], expected: 18.0 },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: -5.0 },
      { input: [-1.0, 1.0, 0.0], expected: -1.0 },
      { input: [-1.0, 1.0, 0.1], expected: [reinterpretU64AsF64(0xbfe9_9999_a000_0000n), reinterpretU64AsF64(0xbfe9_9999_8000_0000n)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: 0.0 },
      { input: [-1.0, 1.0, 0.9], expected: [reinterpretU64AsF64(0x3fe9_9999_8000_0000n), reinterpretU64AsF64(0x3fe9_9999_c000_0000n)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: 1.0 },
      { input: [-1.0, 1.0, 2.0], expected: 3.0 },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kUnboundedBounds },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kUnboundedBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kUnboundedBounds },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: 0.0 },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.mixImpreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.mixImpreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixPreciseInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: -1.0 },
      { input: [0.0, 1.0, 0.0], expected: 0.0 },
      { input: [0.0, 1.0, 0.1], expected: [reinterpretU64AsF64(0x3fb9_9999_8000_0000n), reinterpretU64AsF64(0x3fb9_9999_a000_0000n)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: 0.5 },
      { input: [0.0, 1.0, 0.9], expected: [reinterpretU64AsF64(0x3fec_cccc_c000_0000n), reinterpretU64AsF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: 1.0 },
      { input: [0.0, 1.0, 2.0], expected: 2.0 },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: 2.0 },
      { input: [1.0, 0.0, 0.0], expected: 1.0 },
      { input: [1.0, 0.0, 0.1], expected: [reinterpretU64AsF64(0x3fec_cccc_c000_0000n), reinterpretU64AsF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: 0.5 },
      { input: [1.0, 0.0, 0.9], expected: [reinterpretU64AsF64(0x3fb9_9999_0000_0000n), reinterpretU64AsF64(0x3fb9_999a_0000_0000n)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: 0.0 },
      { input: [1.0, 0.0, 2.0], expected: -1.0 },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: -10.0 },
      { input: [0.0, 10.0, 0.0], expected: 0.0 },
      { input: [0.0, 10.0, 0.1], expected: [reinterpretU64AsF64(0x3fef_ffff_e000_0000n), reinterpretU64AsF64(0x3ff0_0000_2000_0000n)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: 5.0 },
      { input: [0.0, 10.0, 0.9], expected: [reinterpretU64AsF64(0x4021_ffff_e000_0000n), reinterpretU64AsF64(0x4022_0000_2000_0000n)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: 10.0 },
      { input: [0.0, 10.0, 2.0], expected: 20.0 },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: -6.0 },
      { input: [2.0, 10.0, 0.0], expected: 2.0 },
      { input: [2.0, 10.0, 0.1], expected: [reinterpretU64AsF64(0x4006_6666_4000_0000n), reinterpretU64AsF64(0x4006_6666_8000_0000n)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: 6.0 },
      { input: [2.0, 10.0, 0.9], expected: [reinterpretU64AsF64(0x4022_6666_4000_0000n), reinterpretU64AsF64(0x4022_6666_a000_0000n)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: 10.0 },
      { input: [2.0, 10.0, 2.0], expected: 18.0 },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: -5.0 },
      { input: [-1.0, 1.0, 0.0], expected: -1.0 },
      { input: [-1.0, 1.0, 0.1], expected: [reinterpretU64AsF64(0xbfe9_9999_c000_0000n), reinterpretU64AsF64(0xbfe9_9999_8000_0000n)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: 0.0 },
      { input: [-1.0, 1.0, 0.9], expected: [reinterpretU64AsF64(0x3fe9_9999_8000_0000n), reinterpretU64AsF64(0x3fe9_9999_c000_0000n)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: 1.0 },
      { input: [-1.0, 1.0, 2.0], expected: 3.0 },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kUnboundedBounds },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kUnboundedBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kUnboundedBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kUnboundedBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kUnboundedBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kUnboundedBounds },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: 10.0 },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.mixPreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.mixPreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('smoothStepInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // Normals
      { input: [0, 1, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 1, 1], expected: [reinterpretU32AsF32(0x3f7ffffa), reinterpretU32AsF32(0x3f800003)] },  // ~1
      { input: [0, 1, 10], expected: 1 },
      { input: [0, 1, -10], expected: 0 },
      { input: [0, 2, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [0, 2, 0.5], expected: [reinterpretU32AsF32(0x3e1ffffb), reinterpretU32AsF32(0x3e200007)] },  // ~0.15625...
      { input: [2, 0, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [2, 0, 1.5], expected: [reinterpretU32AsF32(0x3e1ffffb), reinterpretU32AsF32(0x3e200007)] },  // ~0.15625...
      { input: [0, 100, 50], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [0, 100, 25], expected: [reinterpretU32AsF32(0x3e1ffffb), reinterpretU32AsF32(0x3e200007)] },  // ~0.15625...
      { input: [0, -2, -1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [0, -2, -0.5], expected: [reinterpretU32AsF32(0x3e1ffffb), reinterpretU32AsF32(0x3e200007)] },  // ~0.15625...

      // Subnormals
      { input: [0, 2, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.negative.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, 2, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.positive.min, 2, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.negative.max, 2, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.negative.min, 2, 1], expected: [reinterpretU32AsF32(0x3efffff8), reinterpretU32AsF32(0x3f000007)] },  // ~0.5
      { input: [0, kValue.f32.subnormal.positive.max, 1], expected: kUnboundedBounds },
      { input: [0, kValue.f32.subnormal.positive.min, 1], expected: kUnboundedBounds },
      { input: [0, kValue.f32.subnormal.negative.max, 1], expected: kUnboundedBounds },
      { input: [0, kValue.f32.subnormal.negative.min, 1], expected: kUnboundedBounds },

      // Infinities
      { input: [0, 2, Number.POSITIVE_INFINITY], expected: kUnboundedBounds },
      { input: [0, 2, Number.NEGATIVE_INFINITY], expected: kUnboundedBounds },
      { input: [Number.POSITIVE_INFINITY, 2, 1], expected: kUnboundedBounds },
      { input: [Number.NEGATIVE_INFINITY, 2, 1], expected: kUnboundedBounds },
      { input: [0, Number.POSITIVE_INFINITY, 1], expected: kUnboundedBounds },
      { input: [0, Number.NEGATIVE_INFINITY, 1], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const [low, high, x] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.smoothStepInterval(low, high, x);
    t.expect(
      objectEquals(expected, got),
      `f32.smoothStepInterval(${low}, ${high}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarToVectorCase {
  input: number;
  expected: (number | IntervalBounds)[];
}

g.test('unpack2x16floatInterval')
  .paramsSubcasesOnly<ScalarToVectorCase>(
    // prettier-ignore
    [
      // f16 normals
      { input: 0x00000000, expected: [0, 0] },
      { input: 0x80000000, expected: [0, 0] },
      { input: 0x00008000, expected: [0, 0] },
      { input: 0x80008000, expected: [0, 0] },
      { input: 0x00003c00, expected: [1, 0] },
      { input: 0x3c000000, expected: [0, 1] },
      { input: 0x3c003c00, expected: [1, 1] },
      { input: 0xbc00bc00, expected: [-1, -1] },
      { input: 0x49004900, expected: [10, 10] },
      { input: 0xc900c900, expected: [-10, -10] },

      // f16 subnormals
      { input: 0x000003ff, expected: [[0, kValue.f16.subnormal.positive.max], 0] },
      { input: 0x000083ff, expected: [[kValue.f16.subnormal.negative.min, 0], 0] },

      // f16 out of bounds
      { input: 0x7c000000, expected: [kUnboundedBounds, kUnboundedBounds] },
      { input: 0xffff0000, expected: [kUnboundedBounds, kUnboundedBounds] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.unpack2x16floatInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `unpack2x16floatInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
    );
  });

// Scope for unpack2x16snormInterval tests so that they can have constants for
// magic numbers that don't pollute the global namespace or have unwieldy long
// names.
{
  const kZeroBounds: IntervalBounds = [
    reinterpretU32AsF32(0x81400000),
    reinterpretU32AsF32(0x01400000),
  ];
  const kOneBoundsSnorm: IntervalBounds = [
    reinterpretU64AsF64(0x3fef_ffff_a000_0000n),
    reinterpretU64AsF64(0x3ff0_0000_3000_0000n),
  ];
  const kNegOneBoundsSnorm: IntervalBounds = [
    reinterpretU64AsF64(0xbff0_0000_3000_0000n),
    reinterpretU64AsF64(0xbfef_ffff_a000_0000n),
  ];

  const kHalfBounds2x16snorm: IntervalBounds = [
    reinterpretU64AsF64(0x3fe0_001f_a000_0000n),
    reinterpretU64AsF64(0x3fe0_0020_8000_0000n),
  ]; // ~0.5..., due to lack of precision in i16
  const kNegHalfBounds2x16snorm: IntervalBounds = [
    reinterpretU64AsF64(0xbfdf_ffc0_6000_0000n),
    reinterpretU64AsF64(0xbfdf_ffbf_8000_0000n),
  ]; // ~-0.5..., due to lack of precision in i16

  g.test('unpack2x16snormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds] },
        { input: 0x00007fff, expected: [kOneBoundsSnorm, kZeroBounds] },
        { input: 0x7fff0000, expected: [kZeroBounds, kOneBoundsSnorm] },
        { input: 0x7fff7fff, expected: [kOneBoundsSnorm, kOneBoundsSnorm] },
        { input: 0x80018001, expected: [kNegOneBoundsSnorm, kNegOneBoundsSnorm] },
        { input: 0x40004000, expected: [kHalfBounds2x16snorm, kHalfBounds2x16snorm] },
        { input: 0xc001c001, expected: [kNegHalfBounds2x16snorm, kNegHalfBounds2x16snorm] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack2x16snormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack2x16snormInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });
}

// Scope for unpack2x16unormInterval tests so that they can have constants for
// magic numbers that don't pollute the global namespace or have unwieldy long
// names.
{
  const kZeroBounds: IntervalBounds = [
    reinterpretU32AsF32(0x8140_0000),
    reinterpretU32AsF32(0x0140_0000),
  ]; // ~0
  const kOneBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fef_ffff_a000_0000n),
    reinterpretU64AsF64(0x3ff0_0000_3000_0000n),
  ]; // ~1
  const kHalfBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fe0_000f_a000_0000n),
    reinterpretU64AsF64(0x3fe0_0010_8000_0000n),
  ]; // ~0.5..., due to the lack of accuracy in u16

  g.test('unpack2x16unormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds] },
        { input: 0x0000ffff, expected: [kOneBounds, kZeroBounds] },
        { input: 0xffff0000, expected: [kZeroBounds, kOneBounds] },
        { input: 0xffffffff, expected: [kOneBounds, kOneBounds] },
        { input: 0x80008000, expected: [kHalfBounds, kHalfBounds] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack2x16unormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack2x16unormInterval(${t.params.input})\n\tReturned [${got}]\n\tExpected [${expected}]`
      );
    });
}

// Scope for unpack4x8snormInterval tests so that they can have constants for
// magic numbers that don't pollute the global namespace or have unwieldy long
// names.
{
  const kZeroBounds: IntervalBounds = [
    reinterpretU32AsF32(0x8140_0000),
    reinterpretU32AsF32(0x0140_0000),
  ]; // ~0
  const kOneBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fef_ffff_a000_0000n),
    reinterpretU64AsF64(0x3ff0_0000_3000_0000n),
  ]; // ~1
  const kNegOneBounds: IntervalBounds = [
    reinterpretU64AsF64(0xbff0_0000_3000_0000n),
    reinterpretU64AsF64(0xbfef_ffff_a0000_000n),
  ]; // ~-1
  const kHalfBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fe0_2040_2000_0000n),
    reinterpretU64AsF64(0x3fe0_2041_0000_0000n),
  ]; // ~0.50196..., due to lack of precision in i8
  const kNegHalfBounds: IntervalBounds = [
    reinterpretU64AsF64(0xbfdf_bf7f_6000_0000n),
    reinterpretU64AsF64(0xbfdf_bf7e_8000_0000n),
  ]; // ~-0.49606..., due to lack of precision in i8

  g.test('unpack4x8snormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x0000007f, expected: [kOneBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x00007f00, expected: [kZeroBounds, kOneBounds, kZeroBounds, kZeroBounds] },
        { input: 0x007f0000, expected: [kZeroBounds, kZeroBounds, kOneBounds, kZeroBounds] },
        { input: 0x7f000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kOneBounds] },
        { input: 0x00007f7f, expected: [kOneBounds, kOneBounds, kZeroBounds, kZeroBounds] },
        { input: 0x7f7f0000, expected: [kZeroBounds, kZeroBounds, kOneBounds, kOneBounds] },
        { input: 0x7f007f00, expected: [kZeroBounds, kOneBounds, kZeroBounds, kOneBounds] },
        { input: 0x007f007f, expected: [kOneBounds, kZeroBounds, kOneBounds, kZeroBounds] },
        { input: 0x7f7f7f7f, expected: [kOneBounds, kOneBounds, kOneBounds, kOneBounds] },
        {
          input: 0x81818181,
          expected: [kNegOneBounds, kNegOneBounds, kNegOneBounds, kNegOneBounds]
        },
        {
          input: 0x40404040,
          expected: [kHalfBounds, kHalfBounds, kHalfBounds, kHalfBounds]
        },
        {
          input: 0xc1c1c1c1,
          expected: [kNegHalfBounds, kNegHalfBounds, kNegHalfBounds, kNegHalfBounds]
        },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack4x8snormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack4x8snormInterval(${t.params.input})\n\tReturned [${got}]\n\tExpected [${expected}]`
      );
    });
}

// Scope for unpack4x8unormInterval tests so that they can have constants for
// magic numbers that don't pollute the global namespace or have unwieldy long
// names.
{
  const kZeroBounds: IntervalBounds = [
    reinterpretU32AsF32(0x8140_0000),
    reinterpretU32AsF32(0x0140_0000),
  ]; // ~0
  const kOneBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fef_ffff_a000_0000n),
    reinterpretU64AsF64(0x3ff0_0000_3000_0000n),
  ]; // ~1
  const kHalfBounds: IntervalBounds = [
    reinterpretU64AsF64(0x3fe0_100f_a000_0000n),
    reinterpretU64AsF64(0x3fe0_1010_8000_0000n),
  ]; // ~0.50196..., due to lack of precision in u8

  g.test('unpack4x8unormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x000000ff, expected: [kOneBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x0000ff00, expected: [kZeroBounds, kOneBounds, kZeroBounds, kZeroBounds] },
        { input: 0x00ff0000, expected: [kZeroBounds, kZeroBounds, kOneBounds, kZeroBounds] },
        { input: 0xff000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kOneBounds] },
        { input: 0x0000ffff, expected: [kOneBounds, kOneBounds, kZeroBounds, kZeroBounds] },
        { input: 0xffff0000, expected: [kZeroBounds, kZeroBounds, kOneBounds, kOneBounds] },
        { input: 0xff00ff00, expected: [kZeroBounds, kOneBounds, kZeroBounds, kOneBounds] },
        { input: 0x00ff00ff, expected: [kOneBounds, kZeroBounds, kOneBounds, kZeroBounds] },
        { input: 0xffffffff, expected: [kOneBounds, kOneBounds, kOneBounds, kOneBounds] },
        {
          input: 0x80808080,
          expected: [kHalfBounds, kHalfBounds, kHalfBounds, kHalfBounds]
        },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack4x8unormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack4x8unormInterval(${t.params.input})\n\tReturned [${got}]\n\tExpected [${expected}]`
      );
    });
}

interface VectorToIntervalCase {
  input: number[];
  expected: number | IntervalBounds;
}

g.test('lengthIntervalVector_f32')
  .paramsSubcasesOnly<VectorToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // vec2
      {input: [1.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0], expected: [reinterpretU64AsF64(0x3ff6_a09d_b000_0000n), reinterpretU64AsF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [-1.0, -1.0], expected: [reinterpretU64AsF64(0x3ff6_a09d_b000_0000n), reinterpretU64AsF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [-1.0, 1.0], expected: [reinterpretU64AsF64(0x3ff6_a09d_b000_0000n), reinterpretU64AsF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [0.1, 0.0], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0, 1.0], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [-1.0, -1.0, -1.0], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [1.0, -1.0, -1.0], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [0.1, 0.0, 0.0], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0, 0.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 1.0, 0.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 0.0, 1.0], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0, 1.0, 1.0], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [-1.0, -1.0, -1.0, -1.0], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [-1.0, 1.0, -1.0, 1.0], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [0.1, 0.0, 0.0, 0.0], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // Test that dot going OOB bounds in the intermediate calculations propagates
      { input: [kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], expected: kUnboundedBounds },
      { input: [kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], expected: kUnboundedBounds },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], expected: kUnboundedBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.lengthInterval([${t.params.input}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorPairToIntervalCase {
  input: [number[], number[]];
  expected: number | IntervalBounds;
}

g.test('distanceIntervalVector_f32')
  .paramsSubcasesOnly<VectorPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      //
      // distance(x, y), where x - y = 0 has an acceptance interval of kUnboundedBounds,
      // because distance(x, y) = length(x - y), and length(0) = kUnboundedBounds

      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: kUnboundedBounds },
      { input: [[1.0, 0.0], [0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0], [1.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[-1.0, 0.0], [0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0], [-1.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0], [-1.0, 0.0]], expected: [reinterpretU64AsF64(0x3ff6_a09d_b000_0000n), reinterpretU64AsF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      { input: [[0.1, 0.0], [0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: kUnboundedBounds },
      { input: [[1.0, 0.0, 0.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0, 0.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [0.0, 1.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [0.0, 0.0, 1.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[1.0, 1.0, 1.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.0, 0.0, 0.0], [1.0, 1.0, 1.0]], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[-1.0, -1.0, -1.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.0, 0.0, 0.0], [-1.0, -1.0, -1.0]], expected: [reinterpretU64AsF64(0x3ffb_b67a_1000_0000n), reinterpretU64AsF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.1, 0.0, 0.0], [0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [[0.0, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: kUnboundedBounds },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]], expected: [reinterpretU64AsF64(0x3fef_ffff_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[1.0, 1.0, 1.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[-1.0, 1.0, -1.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, -1.0, 1.0, -1.0]], expected: [reinterpretU64AsF64(0x3fff_ffff_7000_0000n), reinterpretU64AsF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.1, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9998_9000_0000n), reinterpretU64AsF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.distanceInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.distanceInterval([${t.params.input[0]}, ${t.params.input[1]}]) returned ${got}. Expected ${expected}`
    );
  });

g.test('dotInterval_f32')
  .paramsSubcasesOnly<VectorPairToIntervalCase>(
    // prettier-ignore
    [
      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0], [0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0], [1.0, 1.0]], expected: 2.0 },
      { input: [[-1.0, -1.0], [-1.0, -1.0]], expected: 2.0 },
      { input: [[-1.0, 1.0], [1.0, -1.0]], expected: -2.0 },
      { input: [[0.1, 0.0], [1.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9999_8000_0000n), reinterpretU64AsF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0, 0.0], [0.0, 1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: 3.0 },
      { input: [[-1.0, -1.0, -1.0], [-1.0, -1.0, -1.0]], expected: 3.0 },
      { input: [[1.0, -1.0, -1.0], [-1.0, 1.0, -1.0]], expected: -1.0 },
      { input: [[0.1, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9999_8000_0000n), reinterpretU64AsF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0]], expected: 4.0 },
      { input: [[-1.0, -1.0, -1.0, -1.0], [-1.0, -1.0, -1.0, -1.0]], expected: 4.0 },
      { input: [[-1.0, 1.0, -1.0, 1.0], [1.0, -1.0, 1.0, -1.0]], expected: -4.0 },
      { input: [[0.1, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [reinterpretU64AsF64(0x3fb9_9999_8000_0000n), reinterpretU64AsF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // Test that going out of bounds in the intermediate calculations is caught correctly.
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },
      { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },
      { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: kUnboundedBounds },

      // https://github.com/gpuweb/cts/issues/2155
      { input: [[kValue.f32.positive.max, 1.0, 2.0, 3.0], [-1.0, kValue.f32.positive.max, -2.0, -3.0]], expected: [-13, 0] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.dotInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.dotInterval([${x}], [${y}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorToVectorCase {
  input: number[];
  expected: (number | IntervalBounds)[];
}

g.test('normalizeInterval_f32')
  .paramsSubcasesOnly<VectorToVectorCase>(
    // prettier-ignore
    [
      // vec2
      {input: [1.0, 0.0], expected: [[reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [0.0, 1.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~1.0]
      {input: [-1.0, 0.0], expected: [[reinterpretU64AsF64(0xbff0_0000_b000_0000n), reinterpretU64AsF64(0xbfef_fffe_7000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [1.0, 1.0], expected: [[reinterpretU64AsF64(0x3fe6_a09d_5000_0000n), reinterpretU64AsF64(0x3fe6_a09f_9000_0000n)], [reinterpretU64AsF64(0x3fe6_a09d_5000_0000n), reinterpretU64AsF64(0x3fe6_a09f_9000_0000n)]] },  // [ ~1/√2, ~1/√2]

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [[reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 1.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0], expected: [[reinterpretU64AsF64(0xbff0_0000_b000_0000n), reinterpretU64AsF64(0xbfef_fffe_7000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0], expected: [[reinterpretU64AsF64(0x3fe2_79a6_5000_0000n), reinterpretU64AsF64(0x3fe2_79a8_5000_0000n)], [reinterpretU64AsF64(0x3fe2_79a6_5000_0000n), reinterpretU64AsF64(0x3fe2_79a8_5000_0000n)], [reinterpretU64AsF64(0x3fe2_79a6_5000_0000n), reinterpretU64AsF64(0x3fe2_79a8_5000_0000n)]] },  // [ ~1/√3, ~1/√3, ~1/√3]

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [[reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0, 0.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0, ~0.0]
      {input: [0.0, 0.0, 1.0, 0.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~0.0, ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 0.0, 1.0], expected: [[reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU64AsF64(0x3fef_fffe_7000_0000n), reinterpretU64AsF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0, 0.0], expected: [[reinterpretU64AsF64(0xbff0_0000_b000_0000n), reinterpretU64AsF64(0xbfef_fffe_7000_0000n)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)], [reinterpretU32AsF32(0x81200000), reinterpretU32AsF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0, 1.0], expected: [[reinterpretU64AsF64(0x3fdf_fffe_7000_0000n), reinterpretU64AsF64(0x3fe0_0000_b000_0000n)], [reinterpretU64AsF64(0x3fdf_fffe_7000_0000n), reinterpretU64AsF64(0x3fe0_0000_b000_0000n)], [reinterpretU64AsF64(0x3fdf_fffe_7000_0000n), reinterpretU64AsF64(0x3fe0_0000_b000_0000n)], [reinterpretU64AsF64(0x3fdf_fffe_7000_0000n), reinterpretU64AsF64(0x3fe0_0000_b000_0000n)]] },  // [ ~1/√4, ~1/√4, ~1/√4]
    ]
  )
  .fn(t => {
    const x = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.normalizeInterval(x);
    t.expect(
      objectEquals(expected, got),
      `f32.normalizeInterval([${x}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorPairToVectorCase {
  input: [number[], number[]];
  expected: (number | IntervalBounds)[];
}

g.test('crossInterval_f32')
  .paramsSubcasesOnly<VectorPairToVectorCase>(
    // prettier-ignore
    [
      // parallel vectors, AXB == 0
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0], [0.0, 1.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[-1.0, -1.0, -1.0], [-1.0, -1.0, -1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.1, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[kValue.f32.subnormal.positive.max, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },

      // non-parallel vectors, AXB != 0
      // f32 normals
      { input: [[1.0, -1.0, -1.0], [-1.0, 1.0, -1.0]], expected: [2.0, 2.0, 0.0] },
      { input: [[1.0, 2, 3], [1.0, 5.0, 7.0]], expected: [-1, -4, 3] },

      // f64 normals
      { input: [[0.1, -0.1, -0.1], [-0.1, 0.1, -0.1]],
        expected: [[reinterpretU32AsF32(0x3ca3d708), reinterpretU32AsF32(0x3ca3d70b)],  // ~0.02
          [reinterpretU32AsF32(0x3ca3d708), reinterpretU32AsF32(0x3ca3d70b)],  // ~0.02
          [reinterpretU32AsF32(0xb1400000), reinterpretU32AsF32(0x31400000)]] },  // ~0

      // f32 subnormals
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, kValue.f32.subnormal.negative.min],
          [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max]],
        expected: [[0.0, reinterpretU32AsF32(0x00000002)],  // ~0
          [0.0, reinterpretU32AsF32(0x00000002)],  // ~0
          [reinterpretU32AsF32(0x80000001), reinterpretU32AsF32(0x00000001)]] },  // ~0
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.crossInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.crossInterval([${x}], [${y}]) returned ${got}. Expected ${expected}`
    );
  });

g.test('reflectInterval_f32')
  .paramsSubcasesOnly<VectorPairToVectorCase>(
    // prettier-ignore
    [
      // vec2s
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: [-1.0, 0.0] },
      { input: [[1.0, 0.0], [0.0, 1.0]], expected: [1.0, 0.0] },
      { input: [[0.0, 1.0], [0.0, 1.0]], expected: [0.0, -1.0] },
      { input: [[0.0, 1.0], [1.0, 0.0]], expected: [0.0, 1.0] },
      { input: [[1.0, 1.0], [1.0, 1.0]], expected: [-3.0, -3.0] },
      { input: [[-1.0, -1.0], [1.0, 1.0]], expected: [3.0, 3.0] },
      { input: [[0.1, 0.1], [1.0, 1.0]], expected: [[reinterpretU32AsF32(0xbe99999a), reinterpretU32AsF32(0xbe999998)], [reinterpretU32AsF32(0xbe99999a), reinterpretU32AsF32(0xbe999998)]] },  // [~-0.3, ~-0.3]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max], [1.0, 1.0]], expected: [[reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00800001)], [reinterpretU32AsF32(0x80ffffff), reinterpretU32AsF32(0x00000002)]] },  // [~0.0, ~0.0]

      // vec3s
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [-1.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 1.0, 0.0] },
      { input: [[0.0, 0.0, 1.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 1.0] },
      { input: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]], expected: [1.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0], [0.0, 0.0, 1.0]], expected: [1.0, 0.0, 0.0] },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: [-5.0, -5.0, -5.0] },
      { input: [[-1.0, -1.0, -1.0], [1.0, 1.0, 1.0]], expected: [5.0, 5.0, 5.0] },
      { input: [[0.1, 0.1, 0.1], [1.0, 1.0, 1.0]], expected: [[reinterpretU32AsF32(0xbf000001), reinterpretU32AsF32(0xbefffffe)], [reinterpretU32AsF32(0xbf000001), reinterpretU32AsF32(0xbefffffe)], [reinterpretU32AsF32(0xbf000001), reinterpretU32AsF32(0xbefffffe)]] },  // [~-0.5, ~-0.5, ~-0.5]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, 0.0], [1.0, 1.0, 1.0]], expected: [[reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00800001)], [reinterpretU32AsF32(0x80ffffff), reinterpretU32AsF32(0x00000002)], [reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00000002)]] },  // [~0.0, ~0.0, ~0.0]

      // vec4s
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [-1.0, 0.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 1.0, 0.0, 0.0] },
      { input: [[0.0, 0.0, 1.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 0.0, 1.0, 0.0] },
      { input: [[0.0, 0.0, 0.0, 1.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0, 1.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[-1.0, -1.0, -1.0, -1.0], [1.0, 1.0, 1.0, 1.0]], expected: [7.0, 7.0, 7.0, 7.0] },
      { input: [[0.1, 0.1, 0.1, 0.1], [1.0, 1.0, 1.0, 1.0]], expected: [[reinterpretU32AsF32(0xbf333335), reinterpretU32AsF32(0xbf333332)], [reinterpretU32AsF32(0xbf333335), reinterpretU32AsF32(0xbf333332)], [reinterpretU32AsF32(0xbf333335), reinterpretU32AsF32(0xbf333332)], [reinterpretU32AsF32(0xbf333335), reinterpretU32AsF32(0xbf333332)]] },  // [~-0.7, ~-0.7, ~-0.7, ~-0.7]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]], expected: [[reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00800001)], [reinterpretU32AsF32(0x80ffffff), reinterpretU32AsF32(0x00000002)], [reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00000002)], [reinterpretU32AsF32(0x80fffffe), reinterpretU32AsF32(0x00000002)]] },  // [~0.0, ~0.0, ~0.0, ~0.0]

      // Test that dot going OOB bounds in the intermediate calculations propagates
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },

      // Test that post-dot going OOB propagates
      { input: [[kValue.f32.positive.max, 1.0, 2.0, 3.0], [-1.0, kValue.f32.positive.max, -2.0, -3.0]], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.reflectInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.reflectInterval([${x}], [${y}]) returned ${JSON.stringify(
        got
      )}. Expected ${JSON.stringify(expected)}`
    );
  });

interface MatrixToScalarCase {
  input: number[][];
  expected: number | IntervalBounds;
}

g.test('determinantInterval_f32')
  .paramsSubcasesOnly<MatrixToScalarCase>([
    // Extreme values, i.e. subnormals, very large magnitudes, and those lead to
    // non-precise products, are intentionally not tested, since the accuracy of
    // determinant is restricted to well behaving inputs. Handling all cases
    // requires ~23! options to be calculated in the 4x4 case, so is not
    // feasible.
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: -2,
    },
    {
      input: [
        [-1, 2],
        [-3, 4],
      ],
      expected: 2,
    },
    {
      input: [
        [11, 22],
        [33, 44],
      ],
      expected: -242,
    },
    {
      input: [
        [5, 6],
        [8, 9],
      ],
      expected: -3,
    },
    {
      input: [
        [4, 6],
        [7, 9],
      ],
      expected: -6,
    },
    {
      input: [
        [4, 5],
        [7, 8],
      ],
      expected: -3,
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: 0,
    },
    {
      input: [
        [-1, 2, 3],
        [-4, 5, 6],
        [-7, 8, 9],
      ],
      expected: 0,
    },
    {
      input: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
      expected: 0,
    },
    {
      input: [
        [4, 1, -1],
        [-3, 0, 5],
        [5, 3, 2],
      ],
      expected: -20,
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: 0,
    },
    {
      input: [
        [4, 0, 0, 0],
        [3, 1, -1, 3],
        [2, -3, 3, 1],
        [2, 3, 3, 1],
      ],
      expected: -240,
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.determinantInterval(input);
    t.expect(
      objectEquals(expected, got),
      `f32.determinantInterval([${JSON.stringify(input)}]) returned '${got}. Expected '${expected}'`
    );
  });

interface MatrixToMatrixCase {
  input: number[][];
  expected: (number | IntervalBounds)[][];
}

g.test('transposeInterval_f32')
  .paramsSubcasesOnly<MatrixToMatrixCase>([
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: [
        [1, 3],
        [2, 4],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      expected: [
        [1, 3, 5],
        [2, 4, 6],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      expected: [
        [1, 3, 5, 7],
        [2, 4, 6, 8],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      expected: [
        [1, 4],
        [2, 5],
        [3, 6],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: [
        [1, 4, 7],
        [2, 5, 8],
        [3, 6, 9],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      expected: [
        [1, 4, 7, 10],
        [2, 5, 8, 11],
        [3, 6, 9, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      expected: [
        [1, 5],
        [2, 6],
        [3, 7],
        [4, 8],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      expected: [
        [1, 5, 9],
        [2, 6, 10],
        [3, 7, 11],
        [4, 8, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: [
        [1, 5, 9, 13],
        [2, 6, 10, 14],
        [3, 7, 11, 15],
        [4, 8, 12, 16],
      ],
    },
    {
      input: [
        [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min],
        [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max],
      ],
      expected: [
        [
          [0, kValue.f32.subnormal.positive.max],
          [kValue.f32.subnormal.negative.min, 0],
        ],
        [
          [0, kValue.f32.subnormal.positive.min],
          [kValue.f32.subnormal.negative.max, 0],
        ],
      ],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.transposeInterval(input);
    t.expect(
      objectEquals(expected, got),
      `f32.transposeInterval([${JSON.stringify(input)}]) returned '[${JSON.stringify(
        got
      )}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixPairToMatrixCase {
  input: [number[][], number[][]];
  expected: (number | IntervalBounds)[][];
}

g.test('additionMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // additionMatrixMatrixInterval uses AdditionIntervalOp for calculating intervals,
    // so the testing for additionInterval covers the actual interval
    // calculations.
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [10, 20],
          [30, 40],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
        [55, 66],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
          [70, 80],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
        [55, 66],
        [77, 88],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [10, 11, 12],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
          [1000, 1100, 1200],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
        [1010, 1111, 1212],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 10, 11, 12],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 10, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
          [1300, 1400, 1500, 1600],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
        [1313, 1414, 1515, 1616],
      ],
    },
  ])
  .fn(t => {
    const x = t.params.input[0];
    const y = t.params.input[1];
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.additionMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.additionMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('subtractionMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // subtractionMatrixMatrixInterval uses SubtractionIntervalOp for calculating intervals,
    // so the testing for subtractionInterval covers the actual interval
    // calculations.
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
        ],
        [
          [10, 20],
          [30, 40],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
      ],
    },
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
          [-5, -6],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
        [-55, -66],
      ],
    },
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
          [-5, -6],
          [-7, -8],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
          [70, 80],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
        [-55, -66],
        [-77, -88],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
          [-7, -8, -9],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
        [-77, -88, -99],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
          [-7, -8, -9],
          [-10, -11, -12],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
          [1000, 1100, 1200],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
        [-77, -88, -99],
        [-1010, -1111, -1212],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
          [-9, -10, -11, -12],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
        [-99, -1010, -1111, -1212],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
          [-9, -10, -11, -12],
          [-13, -14, -15, -16],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
          [1300, 1400, 1500, 1600],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
        [-99, -1010, -1111, -1212],
        [-1313, -1414, -1515, -1616],
      ],
    },
  ])
  .fn(t => {
    const x = t.params.input[0];
    const y = t.params.input[1];
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.subtractionMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.subtractionMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('multiplicationMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixMatrixInterval uses and transposeInterval &
    // dotInterval for calculating intervals, so the testing for those functions
    // will cover the actual interval calculations.
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
        [253, 374],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
        [253, 374],
        [341, 506],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
        [319, 440, 561],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
        [319, 440, 561],
        [429, 594, 759],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [121, 154, 187, 220],
        [253, 330, 407, 484],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [121, 154, 187, 220],
        [253, 330, 407, 484],
        [385, 506, 627, 748],
        [517, 682, 847, 1012],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
        [836, 1100],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
        [836, 1100],
        [10403, 13736],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
        [1122, 1386, 1650],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
        [1122, 1386, 1650],
        [13938, 17271, 20604],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
        [1408, 1771, 1936, 2200],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
        [1408, 1771, 1936, 2200],
        [17473, 22018, 24139, 27472],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
        [17168, 20600],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
        [17168, 20600],
        [24442, 30300],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
        [25248, 27468, 30900],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
        [25248, 27468, 30900],
        [35350, 39592, 45450],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [990, 1133, 1210, 1320],
        [2222, 2585, 2794, 3080],
        [30904, 35447, 37768, 41200],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [990, 1133, 1210, 1320],
        [2222, 2585, 2794, 3080],
        [30904, 35447, 37768, 41200],
        [43026, 50399, 54742, 60600],
      ],
    },
  ])
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.multiplicationMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixScalarToMatrixCase {
  matrix: number[][];
  scalar: number;
  expected: (number | IntervalBounds)[][];
}

g.test('multiplicationMatrixScalarInterval_f32')
  .paramsSubcasesOnly<MatrixScalarToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixScalarInterval uses MultiplicationIntervalOp for calculating intervals,
    // so the testing for multiplcationInterval covers the actual interval
    // calculations.
    {
      matrix: [
        [1, 2],
        [3, 4],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
      ],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
        [50, 60],
      ],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
        [50, 60],
        [70, 80],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
        [70, 80, 90],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
        [70, 80, 90],
        [100, 110, 120],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
        [90, 100, 110, 120],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
        [90, 100, 110, 120],
        [130, 140, 150, 160],
      ],
    },
  ])
  .fn(t => {
    const matrix = t.params.matrix;
    const scalar = t.params.scalar;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.multiplicationMatrixScalarInterval(matrix, scalar);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationMatrixScalarInterval([${JSON.stringify(
        matrix
      )}], ${scalar}) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

// There are no explicit tests for multiplicationScalarMatrixInterval, since it
// is just a pass-through to multiplicationMatrixScalarInterval

interface MatrixVectorToVectorCase {
  matrix: number[][];
  vector: number[];
  expected: (number | IntervalBounds)[];
}

g.test('multiplicationMatrixVectorInterval_f32')
  .paramsSubcasesOnly<MatrixVectorToVectorCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixVectorInterval uses DotIntervalOp &
    // TransposeIntervalOp for calculating intervals, so the testing for
    // dotInterval & transposeInterval covers the actual interval
    // calculations.
    {
      matrix: [
        [1, 2],
        [3, 4],
      ],
      vector: [11, 22],
      expected: [77, 110],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      vector: [11, 22],
      expected: [99, 132, 165],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      vector: [11, 22],
      expected: [121, 154, 187, 220],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      vector: [11, 22, 33],
      expected: [242, 308],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      vector: [11, 22, 33],
      expected: [330, 396, 462],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      vector: [11, 22, 33],
      expected: [418, 484, 550, 616],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      vector: [11, 22, 33, 44],
      expected: [550, 660],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      vector: [11, 22, 33, 44],
      expected: [770, 880, 990],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      vector: [11, 22, 33, 44],
      expected: [990, 1100, 1210, 1320],
    },
  ])
  .fn(t => {
    const matrix = t.params.matrix;
    const vector = t.params.vector;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.multiplicationMatrixVectorInterval(matrix, vector);
    t.expect(
      objectEquals(expected, got),
      `f32multiplicationMatrixVectorInterval([${JSON.stringify(matrix)}], [${JSON.stringify(
        vector
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface VectorMatrixToVectorCase {
  vector: number[];
  matrix: number[][];
  expected: (number | IntervalBounds)[];
}

g.test('multiplicationVectorMatrixInterval_f32')
  .paramsSubcasesOnly<VectorMatrixToVectorCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationVectorMatrixInterval uses DotIntervalOp for calculating
    // intervals, so the testing for dotInterval covers the actual interval
    // calculations.
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
      ],
      expected: [55, 121],
    },
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
        [55, 66],
      ],
      expected: [55, 121, 187],
    },
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
        [55, 66],
        [77, 88],
      ],
      expected: [55, 121, 187, 253],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
      ],
      expected: [154, 352],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
      expected: [154, 352, 550],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
        [1010, 1111, 1212],
      ],
      expected: [154, 352, 550, 6868],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
      ],
      expected: [330, 770],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
      ],
      expected: [330, 770, 10300],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
        [1313, 1414, 1515, 1616],
      ],
      expected: [330, 770, 10300, 15150],
    },
  ])
  .fn(t => {
    const vector = t.params.vector;
    const matrix = t.params.matrix;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.multiplicationVectorMatrixInterval(vector, matrix);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationVectorMatrixInterval([${JSON.stringify(vector)}], [${JSON.stringify(
        matrix
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

// API - Acceptance Intervals w/ bespoke implementations

interface FaceForwardCase {
  input: [number[], number[], number[]];
  expected: ((number | IntervalBounds)[] | undefined)[];
}

g.test('faceForwardIntervals_f32')
  .paramsSubcasesOnly<FaceForwardCase>(
    // prettier-ignore
    [
      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0], [1.0, 0.0]], expected: [[-1.0, 0.0]] },
      { input: [[-1.0, 0.0], [1.0, 0.0], [1.0, 0.0]], expected: [[1.0, 0.0]] },
      { input: [[1.0, 0.0], [-1.0, 1.0], [1.0, -1.0]], expected: [[1.0, 0.0]] },
      { input: [[-1.0, 0.0], [-1.0, 1.0], [1.0, -1.0]], expected: [[-1.0, 0.0]] },
      { input: [[10.0, 0.0], [10.0, 0.0], [10.0, 0.0]], expected: [[-10.0, 0.0]] },
      { input: [[-10.0, 0.0], [10.0, 0.0], [10.0, 0.0]], expected: [[10.0, 0.0]] },
      { input: [[10.0, 0.0], [-10.0, 10.0], [10.0, -10.0]], expected: [[10.0, 0.0]] },
      { input: [[-10.0, 0.0], [-10.0, 10.0], [10.0, -10.0]], expected: [[-10.0, 0.0]] },
      { input: [[0.1, 0.0], [0.1, 0.0], [0.1, 0.0]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0]] },
      { input: [[-0.1, 0.0], [0.1, 0.0], [0.1, 0.0]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0]] },
      { input: [[0.1, 0.0], [-0.1, 0.1], [0.1, -0.1]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0]] },
      { input: [[-0.1, 0.0], [-0.1, 0.1], [0.1, -0.1]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0]] },

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0], [1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0]] },
      { input: [[1.0, 0.0, 0.0], [-1.0, 1.0, 0.0], [1.0, -1.0, 0.0]], expected: [[1.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0], [-1.0, 1.0, 0.0], [1.0, -1.0, 0.0]], expected: [[-1.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0], [10.0, 0.0, 0.0], [10.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0], [10.0, 0.0, 0.0], [10.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0], [-10.0, 10.0, 0.0], [10.0, -10.0, 0.0]], expected: [[10.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0], [-10.0, 10.0, 0.0], [10.0, -10.0, 0.0]], expected: [[-10.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0], [0.1, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0], [0.1, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0], [-0.1, 0.0, 0.0], [0.1, -0.0, 0.0]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0], [-0.1, 0.0, 0.0], [0.1, -0.0, 0.0]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0, 0.0]] },

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0, 0.0]] },
      { input: [[1.0, 0.0, 0.0, 0.0], [-1.0, 1.0, 0.0, 0.0], [1.0, -1.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0, 0.0], [-1.0, 1.0, 0.0, 0.0], [1.0, -1.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0, 0.0], [-10.0, 10.0, 0.0, 0.0], [10.0, -10.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0, 0.0], [-10.0, 10.0, 0.0, 0.0], [10.0, -10.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0, 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0, 0.0], [-0.1, 0.0, 0.0, 0.0], [0.1, -0.0, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0x3dcccccc), reinterpretU32AsF32(0x3dcccccd)], 0.0, 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0, 0.0], [-0.1, 0.0, 0.0, 0.0], [0.1, -0.0, 0.0, 0.0]], expected: [[[reinterpretU32AsF32(0xbdcccccd), reinterpretU32AsF32(0xbdcccccc)], 0.0, 0.0, 0.0]] },

      // dot(y, z) === 0
      { input: [[1.0, 1.0], [1.0, 0.0], [0.0, 1.0]], expected:  [[-1.0, -1.0]] },

      // subnormals, also dot(y, z) spans 0
      { input: [[kValue.f32.subnormal.positive.max, 0.0], [kValue.f32.subnormal.positive.min, 0.0], [kValue.f32.subnormal.negative.min, 0.0]], expected:  [[[0.0, kValue.f32.subnormal.positive.max], 0.0], [[kValue.f32.subnormal.negative.min, 0], 0.0]] },

      // dot going OOB returns [undefined, x, -x]
      { input: [[1.0, 1.0], [kValue.f32.positive.max, kValue.f32.positive.max], [kValue.f32.positive.max, kValue.f32.positive.max]], expected: [undefined, [1, 1], [-1, -1]] },

    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = t.params.expected.map(e => (e !== undefined ? FP.f32.toVector(e) : undefined));
    const got = FP.f32.faceForwardIntervals(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.faceForwardInterval([${x}], [${y}], [${z}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface ModfCase {
  input: number;
  fract: number | IntervalBounds;
  whole: number | IntervalBounds;
}

g.test('modfInterval_f32')
  .paramsSubcasesOnly<ModfCase>(
    // prettier-ignore
    [
      // Normals
      { input: 0, fract: 0, whole: 0 },
      { input: 1, fract: 0, whole: 1 },
      { input: -1, fract: 0, whole: -1 },
      { input: 0.5, fract: 0.5, whole: 0 },
      { input: -0.5, fract: -0.5, whole: 0 },
      { input: 2.5, fract: 0.5, whole: 2 },
      { input: -2.5, fract: -0.5, whole: -2 },
      { input: 10.0, fract: 0, whole: 10 },
      { input: -10.0, fract: 0, whole: -10 },

      // Subnormals
      { input: kValue.f32.subnormal.negative.min, fract: [kValue.f32.subnormal.negative.min, 0], whole: 0 },
      { input: kValue.f32.subnormal.negative.max, fract: [kValue.f32.subnormal.negative.max, 0], whole: 0 },
      { input: kValue.f32.subnormal.positive.min, fract: [0, kValue.f32.subnormal.positive.min], whole: 0 },
      { input: kValue.f32.subnormal.positive.max, fract: [0, kValue.f32.subnormal.positive.max], whole: 0 },

      // Boundaries
      { input: kValue.f32.negative.min, fract: 0, whole: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, fract: kValue.f32.negative.max, whole: 0 },
      { input: kValue.f32.positive.min, fract: kValue.f32.positive.min, whole: 0 },
      { input: kValue.f32.positive.max, fract: 0, whole: kValue.f32.positive.max },
    ]
  )
  .fn(t => {
    const expected = {
      fract: FP.f32.toInterval(t.params.fract),
      whole: FP.f32.toInterval(t.params.whole),
    };

    const got = FP.f32.modfInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.modfInterval([${t.params.input}) returned { fract: [${got.fract}], whole: [${got.whole}] }. Expected { fract: [${expected.fract}], whole: [${expected.whole}] }`
    );
  });

interface RefractCase {
  input: [number[], number[], number];
  expected: (number | IntervalBounds)[];
}

// Scope for refractInterval tests so that they can have constants for magic
// numbers that don't pollute the global namespace or have unwieldy long names.
{
  const kNegativeOneBounds: IntervalBounds = [
    reinterpretU64AsF64(0xbff0_0000_c000_0000n),
    reinterpretU64AsF64(0xbfef_ffff_4000_0000n),
  ];

  g.test('refractInterval_f32')
    .paramsSubcasesOnly<RefractCase>(
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // prettier-ignore
      [
        // k < 0
        { input: [[1, 1], [0.1, 0], 10], expected: [0, 0] },

        // k contains 0
        { input: [[1, 1], [0.1, 0], 1.005038], expected: [kUnboundedBounds, kUnboundedBounds] },

        // k > 0
        // vec2
        { input: [[1, 1], [1, 0], 1], expected: [kNegativeOneBounds, 1] },
        { input: [[1, -2], [3, 4], 5], expected: [[reinterpretU32AsF32(0x40ce87a4), reinterpretU32AsF32(0x40ce8840)],  // ~6.454...
            [reinterpretU32AsF32(0xc100fae8), reinterpretU32AsF32(0xc100fa80)]] },  // ~-8.061...

        // vec3
        { input: [[1, 1, 1], [1, 0, 0], 1], expected: [kNegativeOneBounds, 1, 1] },
        { input: [[1, -2, 3], [-4, 5, -6], 7], expected: [[reinterpretU32AsF32(0x40d24480), reinterpretU32AsF32(0x40d24c00)],  // ~6.571...
            [reinterpretU32AsF32(0xc1576f80), reinterpretU32AsF32(0xc1576ad0)],  // ~-13.464...
            [reinterpretU32AsF32(0x41a2d9b0), reinterpretU32AsF32(0x41a2dc80)]] },  // ~20.356...

        // vec4
        { input: [[1, 1, 1, 1], [1, 0, 0, 0], 1], expected: [kNegativeOneBounds, 1, 1, 1] },
        { input: [[1, -2, 3,-4], [-5, 6, -7, 8], 9], expected: [[reinterpretU32AsF32(0x410ae480), reinterpretU32AsF32(0x410af240)],  // ~8.680...
            [reinterpretU32AsF32(0xc18cf7c0), reinterpretU32AsF32(0xc18cef80)],  // ~-17.620...
            [reinterpretU32AsF32(0x41d46cc0), reinterpretU32AsF32(0x41d47660)],  // ~26.553...
            [reinterpretU32AsF32(0xc20dfa80), reinterpretU32AsF32(0xc20df500)]] },  // ~-35.494...

        // Test that dot going OOB bounds in the intermediate calculations propagates
        { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
        { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
        { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
        { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
        { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
        { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0], 1], expected: [kUnboundedBounds, kUnboundedBounds, kUnboundedBounds] },
      ]
    )
    .fn(t => {
      const [i, s, r] = t.params.input;
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.refractInterval(i, s, r);
      t.expect(
        objectEquals(expected, got),
        `refractIntervals([${i}], [${s}], ${r}) returned [${got}]. Expected [${expected}]`
      );
    });
}
