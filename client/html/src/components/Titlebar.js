const template = `
<div class="titlebar">
    <div class="info">
        <img class="icon" src="assets/logo.png" draggable="false">
        <span class="title">MySN</span>
    </div>
    <nav class="buttons">
        <button @click="minimizeWindow()"><img src="assets/minimize.png"></button>
        <button @click="maximizeWindow()"><img src="assets/maximize.png"></button>
        <button @click="closeWindow()"><img src="assets/close.png"></button>
    </nav>
</div>
`

const { ipcRenderer } = require('electron');

export default {
	template,
	methods: {
		closeWindow() {
			ipcRenderer.send('closeWindow', null);
		},

		maximizeWindow() {
			ipcRenderer.send('maximizeWindow', null);
		},

		minimizeWindow() {
			ipcRenderer.send('minimizeWindow', null);
		},
	}
}