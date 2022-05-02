const template = `
<div class="main">
    <Titlebar />
    <Picture />
</div>
`

import Titlebar from './components/SubTitlebar.js';

import Picture from './views/Picture.js';

export default {
    template,
    components: {
        'Titlebar': Titlebar,

        'Picture': Picture
    }
}