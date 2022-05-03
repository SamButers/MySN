const commonVariables = require('./commonVariables.js');

// Sends message back to the renderer process as a message update
function loopbackMessage(content) {
    commonVariables.windows.room.webContents.send('messageUpdate', {
        userId: commonVariables.user.id,
        content
    });
}

module.exports = { loopbackMessage }