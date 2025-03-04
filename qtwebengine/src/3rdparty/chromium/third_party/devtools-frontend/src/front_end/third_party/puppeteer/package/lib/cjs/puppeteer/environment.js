"use strict";
/**
 * Copyright 2020 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.USE_TAB_TARGET = exports.DEFERRED_PROMISE_DEBUG_TIMEOUT = exports.isNode = void 0;
/**
 * @internal
 */
exports.isNode = !!(typeof process !== 'undefined' && process.version);
/**
 * @internal
 */
exports.DEFERRED_PROMISE_DEBUG_TIMEOUT = typeof process !== 'undefined' &&
    typeof process.env['PUPPETEER_DEFERRED_PROMISE_DEBUG_TIMEOUT'] !== 'undefined'
    ? Number(process.env['PUPPETEER_DEFERRED_PROMISE_DEBUG_TIMEOUT'])
    : -1;
/**
 * Only used for internal testing.
 *
 * @internal
 */
exports.USE_TAB_TARGET = typeof process !== 'undefined'
    ? process.env['PUPPETEER_INTERNAL_TAB_TARGET'] === 'true'
    : false;
//# sourceMappingURL=environment.js.map