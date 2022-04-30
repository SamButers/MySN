const template = `
<div class="main">
    <Titlebar />
    <Room />
</div>
`

import Titlebar from './components/SubTitlebar.js';

import Room from './views/Room.js';

export default {
    template,
    components: {
        'Titlebar': Titlebar,

        'Room': Room
    },
    data() {
        return {
            user: {
                id: 0,
                picture: 0,
                name: 'User'
            }
        }
    }
}