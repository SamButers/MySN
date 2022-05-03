const commonVariables = require('./commonVariables.js');

function loopbackMessage(content) {
    commonVariables.windows.room.webContents.send('messageUpdate', {
        userId: commonVariables.user.id,
        content
    });
}

module.exports = { loopbackMessage }