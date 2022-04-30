const { ipcRenderer } = require('electron');

import { createApp } from 'vue';
import App from './roomApp.js';

window.addEventListener('load', () => {
    createApp(App).mount('#main');
});
