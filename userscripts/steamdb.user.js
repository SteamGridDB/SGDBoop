// ==UserScript==
// @name         SGDBoop for steamDB
// @namespace    https://www.steamgriddb.com/
// @version      0.1
// @description  SGDBoop for SteamDB
// @author       alvaromunoz
// @match        https://steamdb.info/app/*
// @icon         https://www.google.com/s2/favicons?sz=64&domain=steamdb.info
// @run-at       document-idle
// ==/UserScript==

(function () {
    'use strict';

    // https://stackoverflow.com/questions/37098405/javascript-queryselector-find-div-by-innertext/37098508
    function contains(selector, text) {
        var elements = document.querySelectorAll(selector);
        return Array.prototype.filter.call(elements, function(element){
            return RegExp(text).test(element.textContent);
        });
    }

    const libraryAssetsTd = contains('td', 'library_assets')[0];
    libraryAssetsTd.innerText = libraryAssetsTd.innerText + "... booped!";
    libraryAssetsTd.parentNode.querySelectorAll('a.image-hover').forEach(function(element) {
        let dividertext = document.createTextNode(" - ");
        let x = element.parentNode.parentNode.querySelector('td').innerText;
        let boopnode = document.createElement("a");
        boopnode.setAttribute('href', `sgdb://steam/${x.substring(x.indexOf('_')+1).replace('capsule','grid')}/${document.querySelector('div.scope-app').attributes['data-appid'].value}/nonsteam`);
        //<svg version="1.1" width="16" height="16" viewBox="0 0 16 16" class="octicon octicon-image" aria-hidden="true"><path fill-rule="evenodd" d="M1.75 2.5a.25.25 0 00-.25.25v10.5c0 .138.112.25.25.25h.94a.76.76 0 01.03-.03l6.077-6.078a1.75 1.75 0 012.412-.06L14.5 10.31V2.75a.25.25 0 00-.25-.25H1.75zm12.5 11H4.81l5.048-5.047a.25.25 0 01.344-.009l4.298 3.889v.917a.25.25 0 01-.25.25zm1.75-.25V2.75A1.75 1.75 0 0014.25 1H1.75A1.75 1.75 0 000 2.75v10.5C0 14.216.784 15 1.75 15h12.5A1.75 1.75 0 0016 13.25zM5.5 6a.5.5 0 11-1 0 .5.5 0 011 0zM7 6a2 2 0 11-4 0 2 2 0 014 0z"></path></svg>
        let boopcontent = `
    <svg class="octicon octicon-image" xmlns="http://www.w3.org/2000/svg" xml:space="preserve" width="16px" height="16px" viewBox="0 0 163.2 163">
    <path fill-rule="evenodd"
        d="M87.1 152.1c-42-.1-76.1-34.2-76-76.2C11.2 35.5 42.8 2.2 83.2 0h-1.4C36.7-.1.1 36.3 0 81.3s36.3 81.6 81.3 81.7h.8c44.9-.2 81.1-36.7 81.1-81.6 0-.7 0-1.4-.1-2.2-1.7 40.8-35.2 72.9-76 72.9z">
    </path>
    <linearGradient id="a" x1="42.5135" x2="160.6096" y1="120.6053" y2="2.5092" gradientUnits="userSpaceOnUse">
        <stop offset="0" stop-color="#FFFFFF"></stop>
        <stop offset="1" stop-color="#FFFFFF" stop-opacity="0"></stop>
    </linearGradient>
    <path fill-rule="evenodd"
        d="M154.5 0H90.2c-39 1.3-69.6 34-68.3 73 1.3 39 34 69.6 73 68.3 38.3-1.3 68.3-33.3 68.3-71.5V8.7c0-4.8-3.9-8.7-8.7-8.7zm-24.9 84.2h-28.5v28.5h-21V84.2H51.6v-21h28.5V34.7h21v28.5h28.5v21z">
    </path>
    </svg> Boop non steam app!`;
        boopnode.innerHTML = boopcontent;
        element.after(boopnode);
        element.after(dividertext);
    });

})();
