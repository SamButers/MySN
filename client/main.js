const path = require('path');
const { app, BrowserWindow, ipcMain } = require('electron');

const windows = {};

// Remove for production
require('electron-reloader')(module, {
    debug: true,
    watchRenderer: true
});

function createLoginWindow() {
    const loginWindow = new BrowserWindow({
        width: 405,
        height: 660,
        titleBarStyle: 'hidden',
        frame: false,
        backgroundColor: "#FFF",
        icon: path.join(__dirname, 'icon.ico'),
        webPreferences: {
            nodeIntegration: true, // Presents security risks, but this application will not be deployed
            contextIsolation: false // Presents security risks, but this application will not be deployed
        }
    });

    loginWindow.loadFile('html/index.html');
    windows.login = loginWindow;
}

app.whenReady().then(() => {
    createLoginWindow();

    ipcMain.on('closeWindow', (e, arg) => {
        app.quit();
    });

    ipcMain.on('maximizeWindow', (e, args) => {
        const currentWindow = BrowserWindow.getFocusedWindow();

        if(currentWindow.isMaximized())
            currentWindow.unmaximize()
        else
            currentWindow.maximize();
    });

    ipcMain.on('minimizeWindow', (e, args) => {
        const currentWindow = BrowserWindow.getFocusedWindow();
        currentWindow.minimize();
    });
});