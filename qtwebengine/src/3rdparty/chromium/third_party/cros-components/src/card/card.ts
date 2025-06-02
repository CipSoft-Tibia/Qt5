/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/ripple/ripple.js';
import '@material/web/focus/md-focus-ring.js';

import {css, CSSResultGroup, html, LitElement, nothing} from 'lit';

const DEFAULT_TICK_MARGIN = css`14px`;

function renderTick() {
  return html`
      <svg
          class="selected-tick"
          width="20"
          height="20"
          xmlns="http://www.w3.org/2000/svg">
        <path
            fill-rule="evenodd"
            clip-rule="evenodd"
            d="M19 10a9 9 0 1 1-18 0 9 9 0 0 1 18 0Zm-4.83-3.045a1.125 1.125 0 0
                0-1.59 0l-3.705 3.704L7.42 9.204a1.125 1.125 0 0 0-1.59
                1.591l2.25 2.25c.439.44 1.151.44 1.59
                0l4.5-4.5c.44-.439.44-1.151 0-1.59Z"/>
      </svg>`;
}

/** A chromeOS compliant card. */
export class Card extends LitElement {
  /**
   * Note about #content display: Because `display: block` / `inline-block`
   * resize depending on any applicable line-height they often don't neatly wrap
   * user content. Instead we use flex here with direction column to "emulate"
   * `display: block` while ignoring line-height. Users are free to set this
   * to `block` via the shadow part and deal with the line-height sizing
   * themselves if they wish, but by default we want this to tightly wrap
   * user content exactly.
   * @nocollapse
   */
  static override styles: CSSResultGroup = css`
    :host {
      border-radius: 12px;
      color: var(--cros-sys-on_surface);
      display: block;
      font: var(--cros-body-0-font);
      width: fit-content;
      height: fit-content;
      min-width: 50px;
      min-height: 50px;
    }

    :host([interactive]) * {
      cursor: pointer;
    }

    #container {
      align-items: center;
      background-color: var(--cros-card-background-color, var(--cros-sys-app_base));
      border: none;
      border-radius: 12px;
      box-sizing: border-box;
      display: grid;
      font: inherit;
      color: inherit;
      height: 100%;
      outline: 1px solid var(--cros-card-border-color, var(--cros-sys-separator));
      padding: var(--cros-card-padding, 16px);
      position: relative;
      text-align: start;
      width: 100%;
    }

    #content {
      border-radius: inherit;
      width: inherit;
      height: inherit;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      z-index: 1;
    }

    #background {
      border-radius: inherit;
      inset: 0;
      overflow: hidden;
      position: absolute;
      z-index: 0;
    }

    #background slot::slotted(img) {
      height: 100%;
      object-fit: cover;
      width: 100%;
    }

    :host([cardStyle="filled"]) #container {
      outline: none;
    }

    :host([cardStyle="elevated"]) #container {
      background-color: var(--cros-card-background-color, var(--cros-sys-base_elevated));
      box-shadow: var(--cros-card-elevation-shadow, var(--cros-elevation-1-shadow));
      outline: none;
    }

    :host([selected]) #container {
      background-color: var(--cros-card-selected-background-color, var(--cros-sys-primary_container));
    }

    md-ripple {
      color: var(--cros-sys-ripple_primary);
      --md-ripple-hover-color: var(--cros-card-hover-color, var(--cros-sys-hover_on_subtle));
      --md-ripple-hover-opacity: 1;
      --md-ripple-pressed-color: var(--cros-card-pressed-color, var(--cros-sys-ripple_neutral_on_subtle));
      --md-ripple-pressed-opacity: 1;
      z-index: 3;
    }

    md-focus-ring {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-focus-ring-duration: 0s;
      --md-focus-ring-outward-offset: 0px;
      --md-focus-ring-shape: 12px;
    }

    :host([disabled]) {
      opacity: var(--cros-disabled-opacity);
    }

    .selected-tick {
      display: none;
    }

    :host([selected]) .selected-tick {
      bottom: var(--cros-card-icon-bottom, ${DEFAULT_TICK_MARGIN});
      display: unset;
      fill: var(--cros-card-icon-color, var(--cros-sys-on_primary_container));
      height: 20px;
      inset-inline-end: var(--cros-card-icon-inline-end, ${
      DEFAULT_TICK_MARGIN});
      position: absolute;
      width: 20px;
      z-index: 2;
    }
  `;

  /** @nocollapse */
  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  /** @export */
  disabled = false;

  /** @export */
  selected = false;

  /** @export */
  interactive = false;

  /** @export */
  cardStyle: 'outline'|'filled'|'elevated' = 'outline';

  /**
   * The card's aria role. Allowed values are a short list of expected roles for
   * a card. They can be extended with types from
   * https://developer.mozilla.org/en-US/docs/Web/Accessibility/ARIA/Roles
   * @export
   */
  override role: 'button'|'dialog'|'none'|'presentation'|'link'|'img'|'cell'|
      null = null;

  /** @nocollapse */
  static override properties = {
    cardStyle: {type: String, reflect: true},
    disabled: {type: Boolean, reflect: true},
    selected: {type: Boolean, reflect: true},
    interactive: {type: Boolean, reflect: true},
    ariaLabel: {type: String, attribute: 'aria-label'},
    ariaRoleDescription: {type: String, attribute: 'aria-roledescription'},
    role: {type: String},
    tabIndex: {type: Number},
  };

  override render() {
    const interactive = !this.disabled && this.interactive;
    const hasTabstop = this.hasAttribute('tabIndex') && this.tabIndex > -1;

    const maybeRipple =
        interactive ? html`<md-ripple for="container"></md-ripple>` : nothing;
    const maybeFocusRing = hasTabstop ?
        html`<md-focus-ring for="container"></md-focus-ring>` :
        nothing;

    return html`
        <div
            aria-label=${this.ariaLabel ?? nothing}
            aria-roledescription=${this.ariaRoleDescription ?? nothing}
            tabindex=${this.tabIndex ?? nothing}
            role=${this.role ?? 'none'}
            @keydown=${this.onKeyDown}
            id="container">
          <div id="background" part="background">
            <slot name="background"></slot>
          </div>
          ${maybeRipple}
          ${maybeFocusRing}
          ${renderTick()}
          <div id="content" part="content">
            <slot></slot>
          </div>
        </div>
    `;
  }

  private onKeyDown(e: KeyboardEvent) {
    switch (e.key) {
      case 'Enter':
      case ' ':
        if (e.composedPath()[0] !==
            this.shadowRoot!.querySelector('#container')) {
          // If a child element of this card was keyboard-activated, it should
          // not trigger a click on this card.
          break;
        }
        e.currentTarget?.dispatchEvent(
            new MouseEvent('click', {bubbles: true, composed: true}));
        break;
      default:
        break;
    }
  }
}

customElements.define('cros-card', Card);

declare global {
  interface HTMLElementTagNameMap {
    'cros-card': Card;
  }
}
