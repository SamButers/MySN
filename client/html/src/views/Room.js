const template = `
    <div class="chatroom">
        <div class="messages" ref="messages">
            <div class="message">
                <div class="user-info">
                    <img src="assets/img/userPictures/2.png" draggable="false">
                    <span>User2</span>
                </div>

                <span class="content">Cool messageCool messageCool messageCool messageCool messageCool messageCool messageCool messageCool messageCool message</span>
            </div>

            <div class="message">
                <div class="user-info">
                    <img src="assets/img/userPictures/2.png" draggable="false">
                    <span>User2</span>
                </div>

                <span class="content">Cool messageCool messageCool messageCool messageCool messageCool messageCool messageCool messageCool messageCool message</span>
            </div>
        </div>

        <div class="input">
            <div class="toolbar"></div>
            <div class="message-area">
                <textarea id="message-input" ref="messageInput"></textarea>
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

export default {
    template,
    data() {
        return {
            messages: [
                {
                    userId: 1,
                    content: 'Cool message, bruv.'
                }
            ],
            htmlMessages: [],
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
        },

        getUsersHandler(e, users) {
            this.users = users;
        }
    },

    computed: {
        sendDisabled: function() {
            return 0;
        }
    },

    mounted() {
        ipcRenderer.on('messageUpdate', this.messageUpdateHandler);
        ipcRenderer.on('getUsers', this.getUsersHandler);

        ipcRenderer.send('getUsers', null);
    },

    unmounted() {
        ipcRenderer.removeListener('messageUpdate', this.messageUpdateHandler);
        ipcRenderer.removeListener('getUsers', this.getUsersHandler);
    }
}