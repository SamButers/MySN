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
        updateMessages(messages) {
            for(let message in messages) {

            }
        },

        messagesUpdateHandler(e , messages) {
            this.updateMessages(messages);
        },

        sendMessage() {
            const messageContent = this.$refs.messageInput.value;

            if(messageContent.length > 0)
                alert(messageContent);
        },

        getUsersHandler(e, users) {
            this.users = users;
        }
    },

    computed: {
        sendDisabled: function() {
            return 1;
        }
    },

    mounted() {
        ipcRenderer.on('messagesUpdate', this.messagesUpdateHandler);
        ipcRenderer.on('getUsers', this.getUsersHandler);

        ipcRenderer.send('getUsers', null);

        this.$refs.messages.scrollTop = this.$refs.messages.scrollHeight;
    },

    unmounted() {
        ipcRenderer.removeListener('messagesUpdate', this.messagesUpdateHandler);
        ipcRenderer.removeListener('getUsers', this.getUsersHandler);
    }
}