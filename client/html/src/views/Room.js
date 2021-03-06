const template = `
    <div class="chatroom">
        <div class="messages" ref="messages">
        </div>

        <div class="input">
            <div class="toolbar"></div>
            <div class="message-area">
                <textarea id="message-input" ref="messageInput" @keydown.enter.prevent="sendMessage()"></textarea>
                <button @click="sendMessage()">Send</button>
            </div>
            <div class="toolbar"></div>
        </div>

        <div class="users">
            <div class="user" v-for="user in users">
                <img src="assets/img/personicon.png">
                <span :title="user.name">{{ user.name }}</span>
            </div>
        </div>
    </div>
` 

const { ipcRenderer } = require('electron');
const sanitizeHtml = require('sanitize-html');

let notificationAudio;

export default {
    template,
    data() {
        return {
            users: {}
        }
    },

    methods: {
        appendMessage(message) {
            this.$refs.messages.innerHTML += `
                <div class="message">
                    <div class="user-info">
                        <img src="assets/img/userPictures/${this.users[message.userId].pictureId}.png" draggable="false">
                        <span>${sanitizeHtml(this.users[message.userId].name)}</span>
                    </div>

                    <span class="content">${sanitizeHtml(message.content)}</span>
                </div>
            `;
        },

        messageUpdateHandler(e , message) {
            this.appendMessage(message);
            this.$refs.messages.scrollTop = this.$refs.messages.scrollHeight;
        },

        sendMessage() {
            const messageContent = this.$refs.messageInput.value;

            ipcRenderer.send('sendMessage', messageContent);

            this.$refs.messageInput.value = '';
        },

        getUsersHandler(e, users) {
            this.users = users;
        },

        userJoinUpdateHandler(e, user) {
            this.users[user.id] = user;
        },

        userLeaveUpdateHandler(e, id) {
            delete this.users[id];
        },

        errorHandler(e, err) {
            alert(err);
        },

        userInfoUpdateHandler(e, userInfo) {
            this.users[userInfo.id].pictureId = userInfo.pictureId;
        },

        playNotificationHandler(e, _) {
            notificationAudio.currentTime = 0;
            notificationAudio.play();
        }
    },

    computed: {
        sendDisabled: function() {
            return 0;
        }
    },

    mounted() {
        notificationAudio = new Audio('assets/snd/notification.mp3');

        ipcRenderer.on('getUsers', this.getUsersHandler);
        ipcRenderer.on('error', this.errorHandler);

        ipcRenderer.on('messageUpdate', this.messageUpdateHandler);
        ipcRenderer.on('userJoinUpdate', this.userJoinUpdateHandler);
        ipcRenderer.on('userLeaveUpdate', this.userLeaveUpdateHandler);
        ipcRenderer.on('userInfoUpdate', this.userInfoUpdateHandler);
        ipcRenderer.on('playNotification', this.playNotificationHandler);

        ipcRenderer.send('getUsers', null);
    },

    unmounted() {
        ipcRenderer.removeListener('getUsers', this.getUsersHandler);
        ipcRenderer.removeListener('error', this.errorHandler);

        ipcRenderer.removeListener('messageUpdate', this.messageUpdateHandler);
        ipcRenderer.removeListener('userJoinUpdate', this.userJoinUpdateHandler);
        ipcRenderer.removeListener('userLeaveUpdate', this.userLeaveUpdateHandler);
        ipcRenderer.removeListener('userInfoUpdate', this.userInfoUpdateHandler);
    }
}