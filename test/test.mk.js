export default async function ()
{
    await this.use("c-cpp");
    await this.loadSubProject("../libryu");

    this.set({
        define: [ 
            "_CRT_SECURE_NO_WARNINGS",
            "_SIMPLELIB_USE_RYU"
        ],
        includePath: [
            "../../ryu/ryu",
        ],
    });
};