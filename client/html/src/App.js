export default {
    template: `
        <div>
            <h1>HI, {{ username }}</h1>
        </div>
    `,
    style: `
        h1 {
            font-size: 40px;
        }
    `,
    data() {
        return {
            username: 'aa'
        };
    }
}