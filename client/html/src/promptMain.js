const { ipcRenderer } = require('electron');

import { createApp } from 'vue';
import App from './promptApp.js';

window.addEventListener('load', () => {
    createApp(App).mount('#main');
});
