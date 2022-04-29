const template = `
<div id="app" class="login">
    <span class="title">Messenger</span>
    <img class="avatar" src="assets/avatar.png" draggable="false">
    <div class="fields">
        <label>Username:</label>
        <input type="text" id="username"></div>

        <button @click="login()">Sign In</button>

        <img class="faded-logo" src="assets/logo.png">
    </div>
</div>
`

const { ipcRenderer } = require('electron');

export default {
	template,
	methods: {
		login() {
			alert("To be implemented");
		}
	}
}