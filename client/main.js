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

let rooms = {};
let users = {};

let pendingRoom = {};

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
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    mainWindow.loadFile('html/index.html');
    mainWindow.winType = 'main';

    windows.main = mainWindow;
}

function createRoomWindow() {
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
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    windows.room.loadFile('html/room.html');
    windows.room.winType = 'room';
}

function createPromptWindow() {
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
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    windows.prompt.loadFile('html/prompt.html');
    windows.prompt.winType = 'prompt';
}

function createPictureWindow() {
    if(windows.picture)
        windows.picture.close();

    windows.picture = new BrowserWindow({
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

    windows.picture.loadFile('html/picture.html');
    windows.picture.winType = 'picture';
}

function joinRoom(roomId) {
    user.room = roomId;
    createRoomWindow();
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

function loopbackMessage(content) {
    windows.room.webContents.send('messageUpdate', {
        userId: user.id,
        content
    });
}

function closeWindow(targetWindow = undefined) {
    if(!targetWindow)
        targetWindow = BrowserWindow.getFocusedWindow();

    if(targetWindow.winType == 'room') {
        windows.room = null;
        leaveRoom();
    }

    else if(targetWindow.winType == 'prompt')
        windows.prompt = null;

    else if(targetWindow.winType == 'picture')
        windows.picture = null;

    targetWindow.close();
}

app.whenReady().then(() => {
    createMainWindow();

    ipcMain.on('closeWindow', (e, arg) => {
        app.quit();
    });

    ipcMain.on('closeSubWindow', (e, arg) => {
        closeWindow();
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
        createPromptWindow();
    });

    ipcMain.on('getPicture', (e, _) => {
        windows.picture.webContents.send('getPicture', user.pictureId);
    });

    ipcMain.on('openPictureWindow', (e, _) => {
        createPictureWindow();
    });

    ipcMain.on('login', (e, username) => {
        const utf8Username = new TextEncoder('utf-8').encode(username);
        const usernameLength = utf8Username.length;

        if(usernameLength > 127) {
            windows.main.webContents.send('error', `Username length is past 127 characters! ${usernameLength} characters currently. (Special characters use multiple characters!)`);
            return;
        }

        client.connect(SERVER_PORT, SERVER_IP, () => {
            const buffer = Buffer.alloc(1 + 1 + utf8Username.length);
            try {
                buffer.writeInt8(0);
                buffer.writeInt8(usernameLength, 1);

                for(let c = 0; c < usernameLength; c++)
                    buffer.writeUInt8(utf8Username[c], 2 + c);

                user.name = username;

                client.write(buffer);

                client.on('data', (data) => {
                    let unreadData = data;

                    dataLoop:
                    while(unreadData.length) {
                        const dataType = unreadData.readInt8();
                        const functionId = unreadData.readInt8(1);

                        if(dataType == 0) {
                            switch(functionId) {
                                case 0: {
                                    const userId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    if(userId == -1) {
                                        windows.main.webContents.send('error', "A login error has occurred.");

                                        break dataLoop;
                                    }

                                    user.id = userId;
                                    windows.main.webContents.send('login', userId);

                                    break;
                                }

                                case 2: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    switch(roomId) {
                                        case -1:
                                            windows.prompt.webContents.send('error', "An error has occurred during room creation.");

                                            break dataLoop;

                                        case -3:
                                            windows.prompt.webContents.send('error', "Cannot create room while in a room.");
                                            
                                            break dataLoop;

                                        case -4:
                                            windows.prompt.webContents.send('error', "Room user limit needs to be at least 2.");
                                            
                                            break dataLoop;
                                    }

                                    pendingRoom.id = roomId;
                                    pendingRoom.users += 1;

                                    windows.main.webContents.send('roomUpdate', pendingRoom);

                                    windows.prompt.close();
                                    windows.prompt = null;

                                    joinRoom(roomId);

                                    break;
                                }

                                case 3: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    switch(roomId) {
                                        case -1:
                                            windows.main.webContents.send('error', "An error has occurred during room join.");

                                            break dataLoop;

                                        case -2:
                                            windows.main.webContents.send('error', "Cannot join. Room is full.");

                                            break dataLoop;

                                        case -3:
                                            windows.main.webContents.send('error', "Cannot join. Already in a room.");

                                            break dataLoop;
                                    }

                                    rooms[roomId].users += 1;

                                    windows.main.webContents.send('roomUpdate', rooms[roomId]);

                                    joinRoom(roomId);

                                    break;
                                }

                                case 4: {
                                    const status = unreadData.readInt8(2);

                                    unreadData = unreadData.subarray(3);

                                    switch(status) {
                                        case -1:
                                            windows.room.webContents.send('error', "An error has occurred during message sending.");

                                            break dataLoop;

                                        case -2:
                                            windows.room.webContents.send('error', "Cannot send message. Not in a room.");

                                            break dataLoop;
                                    }

                                    break;
                                }

                                case 5: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    if(!roomId)
                                        user.room = null;

                                    else {
                                        windows.main.webContents.send('error', "An error has occurred during room leaving.");
                                        break dataLoop;
                                    }

                                    break;
                                }

                                case 6: {
                                    const pictureId = unreadData.readInt8(2);

                                    unreadData = unreadData.subarray(3);

                                    if(pictureId == -1) {
                                        windows.main.webContents.send('error', "An error has occurred during picture changing.");
                                        
                                        break dataLoop;
                                    }

                                    user.pictureId = pictureId;
                                    windows.main.webContents.send('pictureUpdate', pictureId);

                                    if(windows.room)
                                        windows.room.webContents.send('userInfoUpdate', {
                                            id: user.id,
                                            pictureId
                                        });

                                    break;
                                }

                                case 7: {
                                    let bytes = 6;

                                    let id, userLimit, users, nameLength, name;

                                    const roomCount = unreadData.readInt32LE(2);

                                    for(let c = 0; c < roomCount; c += 1) {
                                        id = unreadData.readInt32LE(bytes);
                                        userLimit = unreadData.readInt8(bytes + 4);
                                        users = unreadData.readInt8(bytes + 5);
                                        nameLength = unreadData.readInt8(bytes + 6);
                                        name = unreadData.toString('utf8', bytes + 7, bytes + 7 + nameLength);

                                        rooms[id] = {
                                            id,
                                            userLimit,
                                            users,
                                            name
                                        };

                                        bytes += 7 + nameLength;
                                    }

                                    windows.main.webContents.send('getRooms', rooms);

                                    unreadData = unreadData.subarray(bytes);

                                    break;
                                }

                                case 8: {
                                    let bytes = 3;

                                    let id, pictureId, nameLength, name;

                                    const userCount = unreadData.readInt8(2);

                                    users = {};

                                    for(let c = 0; c < userCount; c += 1) {
                                        id = unreadData.readInt32LE(bytes);
                                        pictureId = unreadData.readInt8(bytes + 4);
                                        nameLength = unreadData.readInt8(bytes + 5);
                                        name = unreadData.toString('utf8', bytes + 6, bytes + 6 + nameLength);

                                        users[id] = {
                                            id,
                                            pictureId,
                                            name
                                        };

                                        bytes += 6 + nameLength;
                                    }

                                    windows.room.webContents.send('getUsers', users);

                                    unreadData = unreadData.subarray(bytes);
                                    break;
                                }

                                default:
                                    console.log('Default case')
                            }
                        }

                        else {
                            switch(functionId) {
                                case 0: {
                                    const id = unreadData.readInt32LE(2);
                                    const userLimit = unreadData.readInt8(6);

                                    if(userLimit == -1) {
                                        delete rooms[id];

                                        windows.main.webContents.send('roomUpdate', {
                                            id,
                                            userLimit: -1
                                        });

                                        unreadData = unreadData.subarray(7);
                                        break;
                                    }

                                    const users = unreadData.readInt8(7);
                                    const nameLength = unreadData.readInt8(8);
                                    const name = unreadData.toString('utf8', 9, 9 + nameLength);

                                    rooms[id] = {
                                        id,
                                        userLimit,
                                        users,
                                        name
                                    };

                                    windows.main.webContents.send('roomUpdate', rooms[id]);

                                    unreadData = unreadData.subarray(9 + nameLength);
                                    break;
                                }

                                case 1: {
                                    const userId = unreadData.readInt32LE(2);
                                    const messageLength = unreadData.readInt16LE(6);
                                    const messageContent = unreadData.toString('utf8', 8 , 8 + messageLength);

                                    windows.room.webContents.send('messageUpdate', {
                                        userId,
                                        content: messageContent
                                    });

                                    unreadData = unreadData.subarray(8 + messageLength);
                                    break;
                                }

                                case 2: {
                                    const id = unreadData.readInt32LE(2);
                                    const pictureId = unreadData.readInt8(6);
                                    const nameLength = unreadData.readInt8(7);
                                    const name = unreadData.toString('utf8', 8, 8 + nameLength);

                                    users[id] = {
                                        id,
                                        pictureId,
                                        name
                                    };

                                    windows.room.webContents.send('userJoinUpdate', users[id]);

                                    unreadData = unreadData.subarray(8 + nameLength);
                                    break;
                                }

                                case 3: {
                                    const id = unreadData.readInt32LE(2);

                                    delete user[id];

                                    windows.room.webContents.send('userLeaveUpdate', id);

                                    unreadData = unreadData.subarray(6);
                                    break;
                                }

                                case 4: {
                                    const id = unreadData.readInt32LE(2);
                                    const pictureId = unreadData.readInt8(6);

                                    users[id].pictureId = pictureId;

                                    windows.room.webContents.send('userInfoUpdate', {
                                        id,
                                        pictureId
                                    });

                                    unreadData = unreadData.subarray(7);
                                    break;
                                }

                                default:
                                    console.log('Default case')
                            }

                        }

                    }

                });
            } catch(e) {
                console.log(e);
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
        const roomName = new TextEncoder('utf-8').encode(roomInfo[0]);
        const roomNameLength = roomName.length;
        const userLimit = roomInfo[1];

        if(roomNameLength > 127) {
            windows.prompt.webContents.send('error', `Room name length is past 127 characters! ${roomNameLength} characters currently. (Special characters use multiple characters!)`);
            return;
        }

        const buffer = Buffer.alloc(roomName.length + 3);
        try {
            buffer.writeInt8(2);
            buffer.writeInt8(roomName.length, 1);

            for(let c = 0; c < roomNameLength; c += 1)
                buffer.writeUInt8(roomName[c], 2 + c);

            buffer.writeInt8(userLimit, roomNameLength + 2);

            client.write(buffer);

            pendingRoom = {
                name: roomInfo[0],
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

    ipcMain.on('sendMessage', (e, messageContent) => {
        const utf8Content = new TextEncoder('utf-8').encode(messageContent);
        const messageLength = utf8Content.length;

        if(messageLength > 32767) {
            windows.room.webContents.send('error', `Message length is past 32.767 characters! ${messageLength} characters currently. (Special characters use multiple characters!)`);
            return;
        }

        const buffer = Buffer.alloc(3 + messageLength);
        try {
            buffer.writeInt8(4);
            buffer.writeInt16LE(messageLength, 1);

            for(let c = 0; c < messageLength; c += 1)
                buffer.writeUInt8(utf8Content[c], 3 + c);

            client.write(buffer);

            loopbackMessage(messageContent);
        } catch(e) {
            console.log(e);
        }
    });

    ipcMain.on('updateInfo', (e, pictureId) => {
        
        const buffer = Buffer.alloc(2);
        try {
            buffer.writeInt8(6);
            buffer.writeInt8(pictureId, 1);

            client.write(buffer);

            closeWindow(windows.picture);
        } catch(e) {
            console.log(e);
        }
    });
});
