const { spawn } = require("child_process");
const WebSocket = require("ws");

// Cria servidor WebSocket na porta 8080
const wss = new WebSocket.Server({ port: 8080 });
console.log(" Bridge Node.js rodando em ws://localhost:8080");

wss.on("connection", (ws) => {
    console.log(" Frontend conectado ao WebSocket");

    // Caminho do executável do cliente C++ (ajusta se necessário)
    const cliente = spawn("./x64/Debug/SocketCliente.exe");

    // Repasse stdout do cliente.exe para o frontend
    cliente.stdout.on("data", (data) => {
        ws.send(data.toString());
    });

    // Se der erro no cliente.exe
    cliente.stderr.on("data", (data) => {
        console.error("Erro do cliente:", data.toString());
    });

    // Se o cliente fechar
    cliente.on("close", (code) => {
        console.log(` Cliente finalizado (código ${code})`);
        ws.close();
    });

    // Mensagens do frontend → stdin do cliente.exe
    ws.on("message", (msg) => {
        cliente.stdin.write(msg + "\n");
    });

    // Se o frontend fechar → mata o cliente.exe
    ws.on("close", () => {
        cliente.kill();
    });
});
