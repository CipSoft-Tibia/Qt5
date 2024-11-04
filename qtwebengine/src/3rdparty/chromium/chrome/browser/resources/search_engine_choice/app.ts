// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://search-engine-choice/tangible_sync_style_shared.css.js';
import 'chrome://resources/cr_components/localized_link/localized_link.js';
import 'chrome://resources/cr_elements/cr_shared_vars.css.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_radio_group/cr_radio_group.js';
import 'chrome://resources/cr_elements/cr_radio_button/cr_radio_button.js';
import 'chrome://resources/cr_elements/cr_icons.css.js';
import 'chrome://resources/cr_elements/icons.html.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import './strings.m.js';

import {CrButtonElement} from 'chrome://resources/cr_elements/cr_button/cr_button.js';
import {CrDialogElement} from 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {afterNextRender, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './app.html.js';
import {SearchEngineChoice, SearchEngineChoiceBrowserProxy} from './browser_proxy.js';

export interface SearchEngineChoiceAppElement {
  $: {
    infoDialog: CrDialogElement,
    submitButton: CrButtonElement,
  };
}

export class SearchEngineChoiceAppElement extends PolymerElement {
  static get is() {
    return 'search-engine-choice-app';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /**
       * We pass the choice list as JSON because it doesn't change
       * dynamically, so it would be better to have it available as loadtime
       * data.
       */
      choiceList_: {
        type: Array,
        value() {
          return JSON.parse(loadTimeData.getString('choiceList'));
        },
      },

      // The choice will always be > 0 when selected for prepopulated engines
      // and == 0 for a custom search engine.
      selectedChoice_: {
        type: Number,
        value: -1,
      },

      isSubmitDisabled_: {
        type: Boolean,
        computed: 'isSubmitButtonDisabled_(selectedChoice_)',
      },
    };
  }

  private choiceList_: SearchEngineChoice[];
  private selectedChoice_: number;

  override connectedCallback() {
    super.connectedCallback();

    afterNextRender(this, () => {
      // Prefer using `document.body.offsetHeight` instead of
      // `document.body.scrollHeight` as it returns the correct height of the
      // page even when the page zoom in Chrome is different than 100%.
      SearchEngineChoiceBrowserProxy.getInstance().handler.displayDialog(
          document.body.offsetHeight);
    });
  }

  private onLinkClicked_() {
    this.$.infoDialog.showModal();
  }

  private isSubmitButtonDisabled_() {
    return this.selectedChoice_ === -1;
  }

  private onSubmitClicked_() {
    SearchEngineChoiceBrowserProxy.getInstance()
        .handler.handleSearchEngineChoiceSelected(this.selectedChoice_);
  }

  private onInfoDialogButtonClicked_() {
    this.$.infoDialog.close();
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'search-engine-choice-app': SearchEngineChoiceAppElement;
  }
}

customElements.define(
    SearchEngineChoiceAppElement.is, SearchEngineChoiceAppElement);
