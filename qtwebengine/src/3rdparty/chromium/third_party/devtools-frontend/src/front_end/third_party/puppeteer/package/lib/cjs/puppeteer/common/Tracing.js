"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Tracing = void 0;
/**
 * Copyright 2017 Google Inc. All rights reserved.
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
const assert_js_1 = require("../util/assert.js");
const Deferred_js_1 = require("../util/Deferred.js");
const ErrorLike_js_1 = require("../util/ErrorLike.js");
const util_js_1 = require("./util.js");
/**
 * The Tracing class exposes the tracing audit interface.
 * @remarks
 * You can use `tracing.start` and `tracing.stop` to create a trace file
 * which can be opened in Chrome DevTools or {@link https://chromedevtools.github.io/timeline-viewer/ | timeline viewer}.
 *
 * @example
 *
 * ```ts
 * await page.tracing.start({path: 'trace.json'});
 * await page.goto('https://www.google.com');
 * await page.tracing.stop();
 * ```
 *
 * @public
 */
class Tracing {
    #client;
    #recording = false;
    #path;
    /**
     * @internal
     */
    constructor(client) {
        this.#client = client;
    }
    /**
     * @internal
     */
    updateClient(client) {
        this.#client = client;
    }
    /**
     * Starts a trace for the current page.
     * @remarks
     * Only one trace can be active at a time per browser.
     *
     * @param options - Optional `TracingOptions`.
     */
    async start(options = {}) {
        (0, assert_js_1.assert)(!this.#recording, 'Cannot start recording trace while already recording trace.');
        const defaultCategories = [
            '-*',
            'devtools.timeline',
            'v8.execute',
            'disabled-by-default-devtools.timeline',
            'disabled-by-default-devtools.timeline.frame',
            'toplevel',
            'blink.console',
            'blink.user_timing',
            'latencyInfo',
            'disabled-by-default-devtools.timeline.stack',
            'disabled-by-default-v8.cpu_profiler',
        ];
        const { path, screenshots = false, categories = defaultCategories } = options;
        if (screenshots) {
            categories.push('disabled-by-default-devtools.screenshot');
        }
        const excludedCategories = categories
            .filter(cat => {
            return cat.startsWith('-');
        })
            .map(cat => {
            return cat.slice(1);
        });
        const includedCategories = categories.filter(cat => {
            return !cat.startsWith('-');
        });
        this.#path = path;
        this.#recording = true;
        await this.#client.send('Tracing.start', {
            transferMode: 'ReturnAsStream',
            traceConfig: {
                excludedCategories,
                includedCategories,
            },
        });
    }
    /**
     * Stops a trace started with the `start` method.
     * @returns Promise which resolves to buffer with trace data.
     */
    async stop() {
        const contentDeferred = Deferred_js_1.Deferred.create();
        this.#client.once('Tracing.tracingComplete', async (event) => {
            try {
                const readable = await (0, util_js_1.getReadableFromProtocolStream)(this.#client, event.stream);
                const buffer = await (0, util_js_1.getReadableAsBuffer)(readable, this.#path);
                contentDeferred.resolve(buffer ?? undefined);
            }
            catch (error) {
                if ((0, ErrorLike_js_1.isErrorLike)(error)) {
                    contentDeferred.reject(error);
                }
                else {
                    contentDeferred.reject(new Error(`Unknown error: ${error}`));
                }
            }
        });
        await this.#client.send('Tracing.end');
        this.#recording = false;
        return contentDeferred.valueOrThrow();
    }
}
exports.Tracing = Tracing;
//# sourceMappingURL=Tracing.js.map