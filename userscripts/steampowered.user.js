// ==UserScript==
// @name         SGDBoop for steampowered.com
// @namespace    https://www.steamgriddb.com/
// @version      0.1
// @description  SGDBoop for steampowered.com
// @author       alvaromunoz
// @match        https://store.steampowered.com/app/*
// @icon         https://www.google.com/s2/favicons?sz=64&domain=steampowered.com
// @run-at       document-end
// ==/UserScript==

(function () {
    'use strict';
    //where to add it?
    var options_menu = document.querySelector('div.apphub_OtherSiteInfo');
    var boop_button = document.createElement('a');
    boop_button.setAttribute('style', 'margin-right: 3px;');
    boop_button.setAttribute('rel', 'noopener');
    boop_button.setAttribute('class', 'btnv6_blue_hoverfade btn_medium');
    boop_button.setAttribute('href', `sgdb://steam/all/${document.querySelector('meta[property="og:url"]').getAttribute('content').match(/http.+\/app\/([0-9]+?)\//)[1]}/nonsteam`);

    var boop_button_content = `
    <span data-tooltip-text="Boop non steam app!">
    <svg class="ico16" xmlns="http://www.w3.org/2000/svg" xml:space="preserve" width="16px" height="16px" viewBox="0 0 163.2 163">
    <path fill="#FFFFFF"
        d="M87.1 152.1c-42-.1-76.1-34.2-76-76.2C11.2 35.5 42.8 2.2 83.2 0h-1.4C36.7-.1.1 36.3 0 81.3s36.3 81.6 81.3 81.7h.8c44.9-.2 81.1-36.7 81.1-81.6 0-.7 0-1.4-.1-2.2-1.7 40.8-35.2 72.9-76 72.9z">
    </path>
    <linearGradient id="a" x1="42.5135" x2="160.6096" y1="120.6053" y2="2.5092" gradientUnits="userSpaceOnUse">
        <stop offset="0" stop-color="#FFFFFF"></stop>
        <stop offset="1" stop-color="#FFFFFF" stop-opacity="0"></stop>
    </linearGradient>
    <path fill="#FFFFFF"
        d="M154.5 0H90.2c-39 1.3-69.6 34-68.3 73 1.3 39 34 69.6 73 68.3 38.3-1.3 68.3-33.3 68.3-71.5V8.7c0-4.8-3.9-8.7-8.7-8.7zm-24.9 84.2h-28.5v28.5h-21V84.2H51.6v-21h28.5V34.7h21v28.5h28.5v21z">
    </path>
    </svg>
    </span>`;
    boop_button.innerHTML = boop_button_content;
    options_menu.insertBefore(boop_button, options_menu.children[0]);
})();
