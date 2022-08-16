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
        let textnode = document.createTextNode("Boop this!");
        boopnode.appendChild(textnode);
        element.after(boopnode);
        element.after(dividertext);
    });

})();
