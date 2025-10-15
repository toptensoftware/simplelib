export default async function ()
{
    await this.use("c-cpp");
    this.set({
        define: [ "_CRT_SECURE_NO_WARNINGS" ],
    });
};