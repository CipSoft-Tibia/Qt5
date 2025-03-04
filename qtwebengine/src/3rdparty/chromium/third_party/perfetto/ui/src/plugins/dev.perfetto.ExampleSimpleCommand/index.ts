// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import {
  Command,
  Plugin,
  PluginContext,
  PluginInfo,
} from '../../public';

// This is just an example plugin, used to prove that the plugin system works.
class ExampleSimpleCommand implements Plugin {
  onActivate(_: PluginContext): void {
    //
  }

  commands(_: PluginContext): Command[] {
    return [
      {
        id: 'dev.perfetto.ExampleSimpleCommand#LogHelloWorld',
        name: 'Log "Hello, world!"',
        callback: () => console.log('Hello, world!'),
      },
    ];
  }
}

export const plugin: PluginInfo = {
  pluginId: 'dev.perfetto.ExampleSimpleCommand',
  plugin: ExampleSimpleCommand,
};
