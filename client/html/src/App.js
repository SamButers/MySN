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
                id: 0,
                picture: 0,
                name: 'User'
            }
        }
    },
    methods: {
        login() {
            this.view = 'hub';
        }
    }
}