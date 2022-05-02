const template = `
<div id="app" class="login">
    <span class="title"><img src="assets/img/mysn.png">Messenger</span>
    <img class="avatar" src="assets/img/avatar.png" draggable="false">
    <div class="fields">
        <label>Username:</label>
        <input v-model="username" type="text" id="username" @keydown.enter="login()" ref="input">
    </div>

        <button @click="login()">Sign In</button>

        <img class="faded-logo" src="assets/img/icon.png">
    </div>
</div>
`

export default {
	template,
    data() {
        return {
            username: 'John Doe'
        }
    },

	methods: {
		login() {
			this.$emit('login', this.$refs.input.value);
		}
	}
}