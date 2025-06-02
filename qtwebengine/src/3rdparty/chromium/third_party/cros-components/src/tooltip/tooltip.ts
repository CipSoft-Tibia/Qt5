/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * Allow popover show and hide functions. See
 * https://developer.mozilla.org/en-US/docs/Web/API/Popover_API
 * go/typescript-reopening
 */
declare interface HTMLElementWithPopoverAPI extends HTMLElement {
  showPopover: () => void;
  hidePopover: () => void;
}

/** A cros-compliant tooltip component. */
export class Tooltip extends LitElement {
  static override styles: CSSResultGroup = css`
  `;
  /** @nocollapse */
  static override properties = {
    anchor: {type: String},
    label: {type: String},
  };

  /** @export */
  anchor: string;
  /** @export */
  label: string;

  constructor() {
    super();
    this.anchorElement = null;
    this.anchor = '';
    this.label = '';
  }

  /** @export */
  get anchorElement(): HTMLElement|null {
    if (this.anchor) {
      return (this.getRootNode() as Document | ShadowRoot)
          .getElementById(`${this.anchor}`);
    }
    return this.currentAnchorElement;
  }

  /** @export */
  set anchorElement(element: HTMLElement|null) {
    this.currentAnchorElement = element;
    this.requestUpdate('anchorElement');
  }

  private currentAnchorElement: HTMLElement|null = null;

  override firstUpdated() {
    // Adds needed popover properties to cros-tooltip.
    this.setAttribute('popover', 'auto');
    this.setAttribute('id', 'tooltip');

    // Sets anchor to this element's ID & adds listeners on hover.
    if (this.anchorElement) {
      this.anchorElement.setAttribute('popovertarget', 'tooltip');
      this.anchorElement.addEventListener('mouseover', () => {
        this.openPopover();
      });
      this.anchorElement.addEventListener('mouseout', () => {
        this.closePopover();
      });
    }
  }

  override disconnectedCallback() {
    if (this.anchorElement) {
      this.anchorElement.removeEventListener('mouseover', this.openPopover);
      this.anchorElement.removeEventListener('mouseout', this.closePopover);
    }
    super.disconnectedCallback();
  }

  private openPopover() {
    const tooltip = this as unknown as HTMLElementWithPopoverAPI;
    tooltip.showPopover();
  }

  private closePopover() {
    const tooltip = this as unknown as HTMLElementWithPopoverAPI;
    tooltip.hidePopover();
  }

  override render() {
    return html`<div>${this.label}</div>`;
  }
}

customElements.define('cros-tooltip', Tooltip);

declare global {
  interface HTMLElementTagNameMap {
    'cros-tooltip': Tooltip;
  }
}
