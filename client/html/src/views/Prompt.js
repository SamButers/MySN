const template = `
    <div class="prompt">
        <label>Room Name (127 characters maximum):</label>
        <input v-model="roomName" type="text">

        <label>User limit (127 maximum)</label>
        <input v-model="userLimit" type="number" min="2" max="127">

        <button>Create</button>
    </div>
` 
const { ipcRenderer } = require('electron');

export default {
    template,
    data() {
        return {
            roomName: 'Room',
            userLimit: 24
        }
    },
    methods: {
        createRoom() {
            ipcRenderer.send('createRoom', [roomName, userLimit]);
        }
    }
}