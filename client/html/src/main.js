const { ipcRenderer } = require('electron');

import { createApp } from 'vue';
import App from './App.js';

window.addEventListener('load', () => {
    createApp(App).mount('#main');
});

/*$(document).ready(() => {
    console.log($('.buttons'))
});*/