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

let pendingRoom = {};
let rooms = {};
let users = {};

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

function joinRoom(roomId) {
    if(windows.room)
        windows.room.close();

    user.room = roomId;

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
}

function leaveRoom() {
    const buffer = Buffer.alloc(1);
    try {
        buffer.writeInt8(5);

        client.write(buffer);
    } catch(e) {
        console.log(e);
    }
}

app.whenReady().then(() => {
    createMainWindow();

    ipcMain.on('closeWindow', (e, arg) => {
        app.quit();
    });

    ipcMain.on('closeSubWindow', (e, arg) => {
        const currentWindow = BrowserWindow.getFocusedWindow();

        if(currentWindow.winType = 'room') {
            windows.room = null;
            leaveRoom();
        }

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
                buffer.write(username, 2, username.length);

                user.name = username;

                client.write(buffer);
            } catch(e) {
                console.log(e);
            }
        });

        client.on('data', (data) => {
            const dataType = data.readInt8();
            const functionId = data.readInt8(1);

            if(dataType == 0) {
                switch(functionId) {
                    case 0: {
                        const userId = data.readInt32LE(2);

                        user.id = userId;
                        windows.main.webContents.send('login', userId);
                        break;
                    }

                    case 2: {
                        const roomId = data.readInt32LE(2);

                        pendingRoom.id = roomId;
                        pendingRoom.users += 1;

                        windows.main.webContents.send('roomUpdate', pendingRoom);

                        windows.prompt.close();
                        windows.prompt = null;

                        joinRoom(roomId);
                        break;
                    }

                    case 3: {
                        const roomId = data.readInt32LE(2);

                        rooms[roomId].users += 1;

                        windows.main.webContents.send('roomUpdate', rooms[roomId]);

                        joinRoom(roomId);
                        break;
                    }

                    case 5: {
                        const roomId = data.readInt32LE(2);

                        if(!roomId)
                            user.room = null;

                        break;
                    }

                    case 7: {
                        let bytes = 6;

                        let id, userLimit, users, nameLength, name;

                        const roomCount = data.readInt32LE(2);

                        for(let c = 0; c < roomCount; c += 1) {
                            id = data.readInt32LE(bytes);
                            userLimit = data.readInt8(bytes + 4);
                            users = data.readInt8(bytes + 5);
                            nameLength = data.readInt8(bytes + 6);
                            name = data.toString('utf8', bytes + 7, bytes + 7 + nameLength);

                            rooms[id] = {
                                id,
                                userLimit,
                                users,
                                name
                            };

                            bytes += 7 + nameLength;
                        }

                        windows.main.webContents.send('getRooms', rooms);
                        break;
                    }

                    case 8: {
                        let bytes = 3;

                        let id, pictureId, nameLength, name;

                        const userCount = data.readInt8(2);

                        users = {};

                        for(let c = 0; c < userCount; c += 1) {
                            id = data.readInt32LE(bytes);
                            pictureId = data.readInt8(bytes + 4);
                            nameLength = data.readInt8(bytes + 5);
                            name = data.toString('utf8', bytes + 6, bytes + 6 + nameLength);

                            users[id] = {
                                id,
                                pictureId,
                                nameLength,
                                name
                            };

                            bytes += 6 + nameLength;
                        }

                        windows.room.webContents.send('getUsers', users);
                        break;
                    }

                    default:
                        console.log('Default case')
                }
            }

            else {
                switch(functionId) {
                    case 0: {
                        const id = data.readInt32LE(2);
                        const userLimit = data.readInt8(6);
                        const users = data.readInt8(7);
                        const nameLength = data.readInt8(8);
                        const name = data.toString('utf8', 9, 9 + nameLength);

                        rooms[id] = {
                            id,
                            userLimit,
                            users,
                            name
                        };

                        windows.main.webContents.send('roomUpdate', rooms[id]);
                        break;
                    }

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

    ipcMain.on('getUsers', (e, _) => {
        const buffer = Buffer.alloc(1);
        try {
            buffer.writeInt8(8);

            client.write(buffer);
        } catch(e) {
            console.log(e);
        }
    });

    ipcMain.on('createRoom', (e, roomInfo) => {
        const roomName = roomInfo[0];
        const userLimit = roomInfo[1];

        const buffer = Buffer.alloc(roomName.length + 3);
        try {
            buffer.writeInt8(2);
            buffer.writeInt8(roomName.length, 1);
            buffer.write(roomName, 2, roomName.length);
            buffer.writeInt8(userLimit, roomName.length + 2);

            client.write(buffer);

            pendingRoom = {
                name: roomName,
                userLimit,
                id: null,
                users: 0
            };
        } catch(e) {
            console.log(e);
        }
    });

    ipcMain.on('joinRoom', (e, roomId) => {
        const buffer = Buffer.alloc(5);
        try {
            buffer.writeInt8(3);
            buffer.writeInt32LE(roomId, 1);

            client.write(buffer);
        } catch(e) {
            console.log(e);
        }

        joinRoom();
    });
});
