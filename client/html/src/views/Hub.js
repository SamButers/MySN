const template = `
<div id="app" class="hub">
    <div class="user-section">
        <span class="title"><img src="assets/img/mysn.png">Messenger</span>
        <div class="user-info">
            <img :src="userPicturePath">
            <span>{{ username }}</span>
        </div>
    </div>

    <div class="room-section">
        <div class="add">
            <img src="assets/img/addRoom.png">
            <span @click="createRoom()">Create a Room</span>
        </div>

        <div class="rooms">
            <div class="room" v-for="room in rooms">
                <img src="assets/img/personicon.png">
                <span class="name" @click="joinRoom(room.id)">{{ room.name }}</span>
                <span class="users">{{ room.users }}/{{ room.userLimit }}</span>
            </div>
        </div>
    </div>
</div>
`

const { ipcRenderer } = require('electron');

export default {
	template,
    data() {
        return {
            rooms: {}
        }
    },
    props: {
        userPicture: {
            type: Number,
            default: 0
        },

        username: {
            type: String,
            default: "User"
        }
    },
    computed: {
        userPicturePath: function() {
            return `assets/img/userPictures/${this.userPicture}.png`
        }
    },
    methods: {
        joinRoom(id) {
            ipcRenderer.send('joinRoom', id);
        },

        createRoom() {
            ipcRenderer.send('popupPrompt', null);
        },

        getRoomsHandler(e, rooms) {
            this.rooms = rooms;
        },

        roomUpdateHandler(e, update) {
            this.rooms[update.id] = update;

            console.log(this.rooms)
        }
    },
    mounted() {
        ipcRenderer.on('getRooms', this.getRoomsHandler);
        ipcRenderer.on('roomUpdate', this.roomUpdateHandler);

        ipcRenderer.send('getRooms', null);
    },
    unmounted() {
        ipcRenderer.removeListener('getRooms', this.getRoomsHandler);
        ipcRenderer.removeListener('roomUpdate', this.roomUpdateHandler);
    }
}