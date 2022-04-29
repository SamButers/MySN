const template = `
<div class="main">
    <Titlebar></Titlebar>
    <Login v-if="view == 'login'"></Login>
</div>
`

import Titlebar from './components/Titlebar.js';

import Login from './views/Login.js';

export default {
    template,
    components: {
        'Titlebar': Titlebar,

        'Login': Login
    },
    data() {
        return {
            view: 'login'
        }
    }
}