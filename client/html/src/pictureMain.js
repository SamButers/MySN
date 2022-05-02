const { ipcRenderer } = require('electron');

import { createApp } from 'vue';
import App from './pictureApp.js';

window.addEventListener('load', () => {
    createApp(App).mount('#main');
});
