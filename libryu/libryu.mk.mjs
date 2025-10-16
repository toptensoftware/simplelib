export default async function()
{
    await this.use("c-cpp");

    this.set({
        projectKind: "lib",
        sourceFiles: p => p.filter(x => x != "generic_128.c"),
        define: [
            "_CRT_SECURE_NO_WARNINGS"
        ],
        includePath: [ "../../ryu" ], 
        sourceDir: "../../ryu/ryu",
    })
}