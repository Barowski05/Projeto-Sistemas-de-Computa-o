const { spawn } = require("child_process");
const WebSocket = require("ws");

// Cria servidor WebSocket na porta 8080
const wss = new WebSocket.Server({ port: 8080 });
console.log("🚀 Bridge Node.js rodando em ws://localhost:8080");

// Caminhos dos executáveis (ajusta conforme sua estrutura de pasta/Debug/Release)
const executaveis = {
    pipes: "./Pipes/pipes_backend.exe",
    socket: "./Sockets/SocketServidor/SocketCliente/Debug/SocketCliente.exe",
    memcom: "./Memcomp/Debug/Memcomp.exe"
};

// Para armazenar o processo atual
let cliente = null;

wss.on("connection", (ws) => {
    console.log(" Frontend conectado ao WebSocket");

    ws.on("message", (msg) => {
        try {
            const parsed = JSON.parse(msg);
            const mecanismo = parsed.mechanism;   // "socket" ou "memcom"
            const texto = parsed.mensagem;

            if (!executaveis[mecanismo]) {
                console.error(" Mecanismo desconhecido:", mecanismo);
                return;
            }

            // Inicia o processo se ainda não foi iniciado
            if (!cliente) {
                cliente = spawn(executaveis[mecanismo]);
                console.log(` Processo ${mecanismo} iniciado: ${executaveis[mecanismo]}`);

                // Repasse stdout do cliente.exe → frontend
                cliente.stdout.on("data", (data) => {
                    const linhas = data.toString().split(/\r?\n/);
                    linhas.forEach(linha => {
                        if (linha.trim() !== "") {
                            try {
                                JSON.parse(linha.trim()); // valida se é JSON
                                ws.send(linha.trim());
                            } catch {
                                console.error("Linha inválida:", linha);
                            }
                        }
                    });
                });

                // Captura stderr do cliente.exe
                cliente.stderr.on("data", (data) => {
                    console.error("Erro do cliente:", data.toString());
                });

                // Quando o cliente terminar
                cliente.on("close", (code) => {
                    console.log(` Processo ${mecanismo} finalizado (código ${code})`);
                    cliente = null;
                });
            }

            // Envia mensagem do frontend → stdin do processo
            if (cliente) {
                cliente.stdin.write(texto + "\n");
            }

        } catch (err) {
            console.error("Erro ao processar mensagem do frontend:", msg, err);
        }
    });

    ws.on("close", () => {
        console.log(" Frontend desconectou.");
        if (cliente) {
            cliente.kill();
            cliente = null;
        }
    });
});
