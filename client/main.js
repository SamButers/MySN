const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const net = require('net');

const client = new net.Socket();

const SERVER_IP = '127.0.0.1';
const SERVER_PORT = 8080;

const windows = {};

let user = {
    id: null,
    username: null,
    picture: 0,
    room: null
};

let rooms = [];

// Remove for production
require('electron-reloader')(module, {
    debug: true,
    watchRenderer: true
});

function createMainWindow() {
    const mainWindow = new BrowserWindow({
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

    mainWindow.loadFile('html/index.html');
    mainWindow.winType = 'main';

    windows.main = mainWindow;
}

app.whenReady().then(() => {
    createMainWindow();

    ipcMain.on('closeWindow', (e, arg) => {
        app.quit();
    });

    ipcMain.on('closeSubWindow', (e, arg) => {
        const currentWindow = BrowserWindow.getFocusedWindow();

        if(currentWindow.winType = 'room')
            windows.room = null;

        else if(currentWindow.winType == 'prompt')
            windows.prompt = null;

        currentWindow.close();
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

    ipcMain.on('popupPrompt', (e, args) => {
        if(windows.prompt)
            return;

        windows.prompt = new BrowserWindow({
            width: 250,
            height: 175,
            titleBarStyle: 'hidden',
            frame: false,
            backgroundColor: "#FFF",
            icon: path.join(__dirname, 'icon.ico'),
            webPreferences: {
                nodeIntegration: true, // Presents security risks, but this application will not be deployed
                contextIsolation: false // Presents security risks, but this application will not be deployed
            }
        });

        windows.prompt.loadFile('html/prompt.html');
        windows.prompt.winType = 'prompt';
    });

    ipcMain.on('login', (e, username) => {
        client.connect(SERVER_PORT, SERVER_IP, () => {
            const buffer = Buffer.alloc(1 + 1 + username.length);
            try {
                buffer.writeInt8(0);
                buffer.writeInt8(username.length, 1);
                buffer.write(username, 2);

                user.name = username;

                client.write(buffer);
            } catch(e) {
                console.log(e);
            }
        });

        client.on('data', (data) => {
            const dataType = data.readInt8();
            const functionId = data.readInt8(1);

            // Response
            if(dataType == 0) {
                switch(functionId) {
                    case 0:
                        const userId = data.readInt32LE(2);

                        user.id = userId;
                        windows.main.webContents.send('login', userId);
                        break;

                    case 7:
                        let bytes = 6;

                        let id, userLimit, users, nameLength, name;

                        const roomCount = data.readInt32LE(2);

                        for(let c = 0; c < roomCount; c += 1) {
                            id = data.readInt32LE(bytes);
                            userLimit = data.readInt8(bytes + 4);
                            users = data.readInt8(bytes + 5);
                            nameLength = data.readInt8(bytes + 6);
                            name = data.toString('utf8', bytes + 7, bytes + 7 + nameLength);

                            rooms.push({
                                id,
                                userLimit,
                                users,
                                name
                            });
                        }

                        rooms.push({
                            id: 666,
                            userLimit: 24,
                            users: 0,
                            name: 'Test room (Not joinable)'
                        });

                        windows.main.webContents.send('getRooms', rooms);
                        break;

                    default:
                        console.log('Default case')
                }
            }
        });
    });

    ipcMain.on('getRooms', (e, _) => {
        const buffer = Buffer.alloc(1);
        try {
            buffer.writeInt8(7);

            client.write(buffer);
        } catch(e) {
            console.log(e);
        }
    });

    ipcMain.on('joinRoom', (e, arg) => {
        if(windows.room)
            windows.room.close();

        windows.room = new BrowserWindow({
            width: 475,
            height: 335,
            titleBarStyle: 'hidden',
            frame: false,
            backgroundColor: "#FFF",
            icon: path.join(__dirname, 'icon.ico'),
            webPreferences: {
                nodeIntegration: true, // Presents security risks, but this application will not be deployed
                contextIsolation: false // Presents security risks, but this application will not be deployed
            }
        });

        windows.room.loadFile('html/room.html');
        windows.room.winType = 'room';
    });
});
