/**
 * Copyright 2023 Google Inc. All rights reserved.
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
import { assert } from '../../util/assert.js';
import { withSourcePuppeteerURLIfNone } from '../util.js';
import { TaskManager, WaitTask } from '../WaitTask.js';
/**
 * A unique key for {@link SandboxChart} to denote the default world.
 * Realms are automatically created in the default sandbox.
 *
 * @internal
 */
export const MAIN_SANDBOX = Symbol('mainSandbox');
/**
 * A unique key for {@link SandboxChart} to denote the puppeteer sandbox.
 * This world contains all puppeteer-internal bindings/code.
 *
 * @internal
 */
export const PUPPETEER_SANDBOX = Symbol('puppeteerSandbox');
/**
 * @internal
 */
export class Sandbox {
    #realm;
    #timeoutSettings;
    #taskManager = new TaskManager();
    constructor(context, timeoutSettings) {
        this.#realm = context;
        this.#timeoutSettings = timeoutSettings;
    }
    get taskManager() {
        return this.#taskManager;
    }
    async document() {
        // TODO(jrandolf): We should try to cache this because we need to dispose
        // this when it's unused.
        return await this.#realm.evaluateHandle(() => {
            return document;
        });
    }
    async $(selector) {
        const document = await this.document();
        return document.$(selector);
    }
    async $$(selector) {
        const document = await this.document();
        return document.$$(selector);
    }
    async $eval(selector, pageFunction, ...args) {
        pageFunction = withSourcePuppeteerURLIfNone(this.$eval.name, pageFunction);
        const document = await this.document();
        return document.$eval(selector, pageFunction, ...args);
    }
    async $$eval(selector, pageFunction, ...args) {
        pageFunction = withSourcePuppeteerURLIfNone(this.$$eval.name, pageFunction);
        const document = await this.document();
        return document.$$eval(selector, pageFunction, ...args);
    }
    async $x(expression) {
        const document = await this.document();
        return document.$x(expression);
    }
    async evaluateHandle(pageFunction, ...args) {
        pageFunction = withSourcePuppeteerURLIfNone(this.evaluateHandle.name, pageFunction);
        return this.#realm.evaluateHandle(pageFunction, ...args);
    }
    async evaluate(pageFunction, ...args) {
        pageFunction = withSourcePuppeteerURLIfNone(this.evaluate.name, pageFunction);
        return this.#realm.evaluate(pageFunction, ...args);
    }
    async adoptHandle(handle) {
        return (await this.evaluateHandle(node => {
            return node;
        }, handle));
    }
    async transferHandle(handle) {
        if (handle.context() === this.#realm) {
            return handle;
        }
        const transferredHandle = await this.evaluateHandle(node => {
            return node;
        }, handle);
        await handle.dispose();
        return transferredHandle;
    }
    waitForFunction(pageFunction, options = {}, ...args) {
        const { polling = 'raf', timeout = this.#timeoutSettings.timeout(), root, signal, } = options;
        if (typeof polling === 'number' && polling < 0) {
            throw new Error('Cannot poll with non-positive interval');
        }
        const waitTask = new WaitTask(this, {
            polling,
            root,
            timeout,
            signal,
        }, pageFunction, ...args);
        return waitTask.result;
    }
    // ///////////////////
    // // Input methods //
    // ///////////////////
    async click(selector, options) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        await handle.click(options);
        await handle.dispose();
    }
    async focus(selector) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        await handle.focus();
        await handle.dispose();
    }
    async hover(selector) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        await handle.hover();
        await handle.dispose();
    }
    async select(selector, ...values) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        const result = await handle.select(...values);
        await handle.dispose();
        return result;
    }
    async tap(selector) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        await handle.tap();
        await handle.dispose();
    }
    async type(selector, text, options) {
        const handle = await this.$(selector);
        assert(handle, `No element found for selector: ${selector}`);
        await handle.type(text, options);
        await handle.dispose();
    }
}
//# sourceMappingURL=Sandbox.js.map