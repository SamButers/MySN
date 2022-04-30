const template = `
<div class="main">
    <Titlebar />
    <Prompt />
</div>
`

import Titlebar from './components/SubTitlebar.js';

import Prompt from './views/Prompt.js';

export default {
    template,
    components: {
        'Titlebar': Titlebar,

        'Prompt': Prompt
    }
}