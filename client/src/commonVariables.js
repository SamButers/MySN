let user = {
    id: null,
    username: null,
    picture: 0,
    room: null
};

let users = {};
const rooms = {};
const windows = {};

let pendingRoom = {};

module.exports = { user, users, rooms, windows, pendingRoom }