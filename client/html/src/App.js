const template = `
<div class="main">
    <Titlebar />
    <Login @login="login" v-if="view == 'login'" />
    <Hub v-if="view == 'hub'" :userPicture="user.picture" :username="user.name" />
</div>
`

import Titlebar from './components/Titlebar.js';

import Login from './views/Login.js';
import Hub from './views/Hub.js';

const { ipcRenderer } = require('electron');

export default {
    template,
    components: {
        'Titlebar': Titlebar,

        'Login': Login,
        'Hub': Hub
    },
    data() {
        return {
            view: 'login',
            user: {
                id: null,
                picture: 0,
                name: 'User',
                room: null
            }
        }
    },
    methods: {
        login(username) {
            this.user.name = username;
            ipcRenderer.send('login', username);
        },
        loginHandler(e, userId) {
            this.user.id = userId;
            this.view = 'hub';
        }
    },
    mounted() {
        ipcRenderer.on('login', this.loginHandler);
    },
    unmounted() {
        ipcRenderer.removeListener('login', this.loginHandler);
    }
}