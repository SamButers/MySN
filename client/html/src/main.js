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

$(document).ready(() => {
    console.log($('.buttons'))
});