const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const net = require('net');

const commonVariables = require('./src/commonVariables.js');

const { createMainWindow, createRoomWindow, createPromptWindow, createPictureWindow, closeWindow, joinRoom, leaveRoom } = require('./src/windows.js');
const { loopbackMessage } = require('./src/handlerUtils.js');

const client = new net.Socket();

const { SERVER_IP, SERVER_PORT } = require('./config.json');

app.whenReady().then(() => {
    createMainWindow();

    // Quit app when main window is closed
    commonVariables.windows.main.on('closed', () => {
        app.quit();
    });

    ipcMain.on('closeWindow', (e, arg) => {
        app.quit();
    });

    // Closes a window other than the main window
    ipcMain.on('closeSubWindow', (e, arg) => {
        closeWindow(null, client);
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

    // Creates room creation prompt
    ipcMain.on('popupPrompt', (e, args) => {
        createPromptWindow();
    });

    ipcMain.on('getPicture', (e, _) => {
        commonVariables.windows.picture.webContents.send('getPicture', commonVariables.user.pictureId);
    });

    // Creates picture selection window
    ipcMain.on('openPictureWindow', (e, _) => {
        createPictureWindow();
    });

    ipcMain.on('login', (e, username) => {
        const utf8Username = new TextEncoder('utf-8').encode(username);
        const usernameLength = utf8Username.length;

        if(usernameLength > 127) {
            commonVariables.windows.main.webContents.send('error', `Username length is past 127 characters! ${usernameLength} characters currently. (Special characters use multiple characters!)`);
            return;
        }

        client.connect(SERVER_PORT, SERVER_IP, () => {
            const buffer = Buffer.alloc(1 + 1 + utf8Username.length);
            try {
                buffer.writeInt8(0);
                buffer.writeInt8(usernameLength, 1);

                // Writes each UTF8 character as an UInt8
                for(let c = 0; c < usernameLength; c++)
                    buffer.writeUInt8(utf8Username[c], 2 + c);

                commonVariables.user.name = username;

                client.write(buffer);

                client.on('data', (data) => {
                    let unreadData = data;

                    // Loop while there is still unread data
                    dataLoop:
                    while(unreadData.length) {
                        const dataType = unreadData.readInt8();
                        const functionId = unreadData.readInt8(1);

                        // User operation response
                        if(dataType == 0) {
                            switch(functionId) {
                                // Login response
                                case 0: {
                                    const userId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    if(userId == -1) {
                                        commonVariables.windows.main.webContents.send('error', "A login error has occurred.");

                                        break dataLoop;
                                    }

                                    commonVariables.user.id = userId;
                                    commonVariables.windows.main.webContents.send('login', userId);

                                    break;
                                }

                                // Create room response
                                case 2: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    switch(roomId) {
                                        case -1:
                                            commonVariables.windows.prompt.webContents.send('error', "An error has occurred during room creation.");

                                            break dataLoop;

                                        case -3:
                                            commonVariables.windows.prompt.webContents.send('error', "Cannot create room while in a room.");
                                            
                                            break dataLoop;

                                        case -4:
                                            commonVariables.windows.prompt.webContents.send('error', "Room user limit needs to be at least 2.");
                                            
                                            break dataLoop;
                                    }

                                    commonVariables.pendingRoom.id = roomId;
                                    commonVariables.pendingRoom.users += 1;

                                    commonVariables.windows.main.webContents.send('roomUpdate', commonVariables.pendingRoom);

                                    commonVariables.windows.prompt.close();
                                    commonVariables.windows.prompt = null;

                                    joinRoom(roomId);

                                    break;
                                }

                                // Room join response
                                case 3: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    switch(roomId) {
                                        case -1:
                                            commonVariables.windows.main.webContents.send('error', "An error has occurred during room join.");

                                            break dataLoop;

                                        case -2:
                                            commonVariables.windows.main.webContents.send('error', "Cannot join. Room is full.");

                                            break dataLoop;

                                        case -3:
                                            commonVariables.windows.main.webContents.send('error', "Cannot join. Already in a room.");

                                            break dataLoop;
                                    }

                                    commonVariables.rooms[roomId].users += 1;

                                    commonVariables.windows.main.webContents.send('roomUpdate', commonVariables.rooms[roomId]);

                                    joinRoom(roomId);

                                    break;
                                }

                                // Message sending response
                                case 4: {
                                    const status = unreadData.readInt8(2);

                                    unreadData = unreadData.subarray(3);

                                    switch(status) {
                                        case -1:
                                            commonVariables.windows.room.webContents.send('error', "An error has occurred during message sending.");

                                            break dataLoop;

                                        case -2:
                                            commonVariables.windows.room.webContents.send('error', "Cannot send message. Not in a room.");

                                            break dataLoop;
                                    }

                                    break;
                                }

                                // Room leave response
                                case 5: {
                                    const roomId = unreadData.readInt32LE(2);

                                    unreadData = unreadData.subarray(6);

                                    if(!roomId)
                                        commonVariables.user.room = null;

                                    else {
                                        commonVariables.windows.main.webContents.send('error', "An error has occurred during room leaving.");
                                        break dataLoop;
                                    }

                                    break;
                                }

                                // User info response
                                case 6: {
                                    const pictureId = unreadData.readInt8(2);

                                    unreadData = unreadData.subarray(3);

                                    if(pictureId == -1) {
                                        commonVariables.windows.main.webContents.send('error', "An error has occurred during picture changing.");
                                        
                                        break dataLoop;
                                    }

                                    commonVariables.user.pictureId = pictureId;
                                    commonVariables.windows.main.webContents.send('pictureUpdate', pictureId);

                                    if(commonVariables.windows.room)
                                        commonVariables.windows.room.webContents.send('userInfoUpdate', {
                                            id: commonVariables.user.id,
                                            pictureId
                                        });

                                    break;
                                }

                                // Get rooms response
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

                                        commonVariables.rooms[id] = {
                                            id,
                                            userLimit,
                                            users,
                                            name
                                        };

                                        bytes += 7 + nameLength;
                                    }

                                    commonVariables.windows.main.webContents.send('getRooms', commonVariables.rooms);

                                    unreadData = unreadData.subarray(bytes);

                                    break;
                                }

                                // Get users response
                                case 8: {
                                    let bytes = 3;

                                    let id, pictureId, nameLength, name;

                                    const userCount = unreadData.readInt8(2);

                                    commonVariables.users = {};

                                    for(let c = 0; c < userCount; c += 1) {
                                        id = unreadData.readInt32LE(bytes);
                                        pictureId = unreadData.readInt8(bytes + 4);
                                        nameLength = unreadData.readInt8(bytes + 5);
                                        name = unreadData.toString('utf8', bytes + 6, bytes + 6 + nameLength);

                                        commonVariables.users[id] = {
                                            id,
                                            pictureId,
                                            name
                                        };

                                        bytes += 6 + nameLength;
                                    }

                                    commonVariables.windows.room.webContents.send('getUsers', commonVariables.users);

                                    unreadData = unreadData.subarray(bytes);
                                    break;
                                }

                                default:
                                    console.log('Default case')
                            }
                        }

                        // Updates
                        else {
                            switch(functionId) {
                                // Room update
                                case 0: {
                                    const id = unreadData.readInt32LE(2);
                                    const userLimit = unreadData.readInt8(6);

                                    if(userLimit == -1) {
                                        delete commonVariables.rooms[id];

                                        commonVariables.windows.main.webContents.send('roomUpdate', {
                                            id,
                                            userLimit: -1
                                        });

                                        unreadData = unreadData.subarray(7);
                                        break;
                                    }

                                    const users = unreadData.readInt8(7);
                                    const nameLength = unreadData.readInt8(8);
                                    const name = unreadData.toString('utf8', 9, 9 + nameLength);

                                    commonVariables.rooms[id] = {
                                        id,
                                        userLimit,
                                        users,
                                        name
                                    };

                                    commonVariables.windows.main.webContents.send('roomUpdate', commonVariables.rooms[id]);

                                    unreadData = unreadData.subarray(9 + nameLength);
                                    break;
                                }

                                // Message update
                                case 1: {
                                    const userId = unreadData.readInt32LE(2);
                                    const messageLength = unreadData.readInt16LE(6);
                                    const messageContent = unreadData.toString('utf8', 8 , 8 + messageLength);

                                    commonVariables.windows.room.webContents.send('messageUpdate', {
                                        userId,
                                        content: messageContent
                                    });

                                    if(!commonVariables.windows.room.isFocused())
                                        commonVariables.windows.room.webContents.send('playNotification', null);

                                    unreadData = unreadData.subarray(8 + messageLength);
                                    break;
                                }

                                // User join update
                                case 2: {
                                    const id = unreadData.readInt32LE(2);
                                    const pictureId = unreadData.readInt8(6);
                                    const nameLength = unreadData.readInt8(7);
                                    const name = unreadData.toString('utf8', 8, 8 + nameLength);

                                    commonVariables.users[id] = {
                                        id,
                                        pictureId,
                                        name
                                    };

                                    commonVariables.windows.room.webContents.send('userJoinUpdate', commonVariables.users[id]);

                                    unreadData = unreadData.subarray(8 + nameLength);
                                    break;
                                }

                                // User leave update
                                case 3: {
                                    const id = unreadData.readInt32LE(2);

                                    delete commonVariables.user[id];

                                    commonVariables.windows.room.webContents.send('userLeaveUpdate', id);

                                    unreadData = unreadData.subarray(6);
                                    break;
                                }

                                // User info update
                                case 4: {
                                    const id = unreadData.readInt32LE(2);
                                    const pictureId = unreadData.readInt8(6);

                                    commonVariables.users[id].pictureId = pictureId;

                                    commonVariables.windows.room.webContents.send('userInfoUpdate', {
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
            commonVariables.windows.prompt.webContents.send('error', `Room name length is past 127 characters! ${roomNameLength} characters currently. (Special characters use multiple characters!)`);
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

            commonVariables.pendingRoom = {
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
            commonVariables.windows.room.webContents.send('error', `Message length is past 32.767 characters! ${messageLength} characters currently. (Special characters use multiple characters!)`);
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

            closeWindow(commonVariables.windows.picture);
        } catch(e) {
            console.log(e);
        }
    });
});
