const { ipcRenderer } = require('electron');

function closeWindow() {
    ipcRenderer.send('closeWindow', null);
}

function maximizeWindow() {
    ipcRenderer.send('maximizeWindow', null);
}

function minimizeWindow() {
    ipcRenderer.send('minimizeWindow', null);
}

function shit() {
    console.log('shit')
}

import { createApp } from 'vue';
import App from './App.js';

createApp(App).mount('#app');

/*$(document).ready(() => {
    console.log($('.buttons'))
});*/