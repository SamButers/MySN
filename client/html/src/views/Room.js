const template = `
    <div class="chatroom">
        <div class="messages">
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
        <div class="input"></div>
        <div class="users"></div>
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
            users: [
                {
                    id: 1,
                    name: 'User1',
                    picture: 2
                }
            ]
        }
    },
    methods: {
        updateMessages(messages) {
            for(let message in messages) {

            }
        }
    },
    mounted() {
        ipcRenderer.on('messagesUpdate', (e, messages) => {
            this.updateMessages(messages);
        });
    },
    unmounted() {

    }
}