const template = `
    <div class="picture-selector">
        <p>Select a picture that represents you and your contacts will see you as in conversations:</p>
        <div class="pictures">
            <img v-for="id in quantity"
                :class="id == target ? 'target' : null"
                :src="'assets/img/userPictures/' + id + '.png'"
                @click="setTarget(id)"
            >
        </div>

        <div class="controls-and-preview">
            <div class="controls">
                <button @click="updateInfo()">Apply</button>
                <button @click="closeWindow()">Cancel</button>
            </div>

            <div class="preview">
                <label>Preview:</label>
                <img :src="'assets/img/userPictures/' + target + '.png'">
            </div>
        </div>
    </div>
` 
const { ipcRenderer } = require('electron');

export default {
    template,
    data() {
        return {
            quantity: 25,
            target: 0
        }
    },

    methods: {
        setTarget(id) {
            this.target = id;
        },

        getPictureHandler(e, id) {
            this.target = id;
        },

        updateInfo() {
            ipcRenderer.send('updateInfo', this.target);
        },

        closeWindow() {
            ipcRenderer.send('closeSubWindow', null);
        },

        errorHandler(e, err) {
            alert(err);
        }
    },

    mounted() {
        ipcRenderer.on('error', this.errorHandler);
        ipcRenderer.on('getPicture', this.getPictureHandler);

        ipcRenderer.send('getPicture', null);
    },

    unmounted() {
        ipcRenderer.removeListener('error', this.errorHandler);
        ipcRenderer.removeListener('getPicture', this.getPictureHandler);

    }
}