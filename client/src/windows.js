const commonVariables = require('./commonVariables.js');

const { BrowserWindow } = require('electron');
const path = require('path');

function createMainWindow() {
    const mainWindow = new BrowserWindow({
        width: 405,
        height: 660,
        titleBarStyle: 'hidden',
        frame: false,
        backgroundColor: "#FFF",
        icon: path.join(__dirname, 'icon.ico'),
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    mainWindow.loadFile('html/index.html');
    mainWindow.winType = 'main';

    commonVariables.windows.main = mainWindow;
}

function createRoomWindow() {
    if(commonVariables.windows.room)
        commonVariables.windows.room.close();

    commonVariables.windows.room = new BrowserWindow({
        width: 475,
        height: 335,
        titleBarStyle: 'hidden',
        frame: false,
        backgroundColor: "#FFF",
        icon: path.join(__dirname, 'icon.ico'),
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    commonVariables.windows.room.loadFile('html/room.html');
    commonVariables.windows.room.winType = 'room';
}

function createPromptWindow() {
    if(commonVariables.windows.prompt)
        return;

    commonVariables.windows.prompt = new BrowserWindow({
        width: 250,
        height: 175,
        titleBarStyle: 'hidden',
        frame: false,
        backgroundColor: "#FFF",
        icon: path.join(__dirname, 'icon.ico'),
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    commonVariables.windows.prompt.loadFile('html/prompt.html');
    commonVariables.windows.prompt.winType = 'prompt';
}

function createPictureWindow() {
    if(commonVariables.windows.picture)
        commonVariables.windows.picture.close();

    commonVariables.windows.picture = new BrowserWindow({
        width: 475,
        height: 475,
        titleBarStyle: 'hidden',
        frame: false,
        backgroundColor: "#FFF",
        icon: path.join(__dirname, 'icon.ico'),
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    commonVariables.windows.picture.loadFile('html/picture.html');
    commonVariables.windows.picture.winType = 'picture';
}

module.exports = { createMainWindow, createRoomWindow, createPromptWindow, createPictureWindow };