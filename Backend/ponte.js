const { spawn } = require("child_process");
const WebSocket = require("ws");

// Cria servidor WebSocket na porta 8080
const wss = new WebSocket.Server({ port: 8080 });
console.log(" Bridge Node.js rodando em ws://localhost:8080");

// Caminhos dos executáveis (ajusta conforme sua estrutura de pasta/Debug/Release)
const executaveis = {
    pipes: "./Pipes/pipes/Debug/pipes.exe",
    socketClient: "./Sockets/SocketServidor/SocketCliente/Debug/SocketCliente.exe",  // 🔹 renomeei para ficar claro
    socketServer: "./Sockets/SocketServidor/Debug/SocketServidor.exe", // 🔹 adicionei
    memcom: "./Memcomp/Debug/Memcomp.exe"
};

// Para armazenar os processos atuais
let cliente = null;
let servidor = null; // 🔹 adicionado

wss.on("connection", (ws) => {
    console.log(" Frontend conectado ao WebSocket");

    ws.on("message", (msg) => {
        try {
            const parsed = JSON.parse(msg.toString());
            const mecanismo = parsed.mechanism;   // "socket" | "memcom" | "pipe"
            const texto = parsed.mensagem || "";

            if (mecanismo === "pipe") {
                // --- PIPES: spawn por mensagem (argv[1] = texto) ---
                const proc = spawn(executaveis.pipes, [texto]);

                proc.stdout.on("data", (data) => {
                    data.toString().split(/\r?\n/).forEach((linha) => {
                        const s = linha.trim();
                        if (!s) return;
                        try {
                            const obj = JSON.parse(s);
                            let out = { role: "pipe", event: "log", status: s };
                            if (obj.type === "status") {
                                out = { role: "pipe", event: "status", status: obj.message || obj.status || "" };
                            } else if (obj.type === "result") {
                                const ev = (obj.status === "success") ? "recv" : "error";
                                const extra = obj.data_received ? ` | data: ${obj.data_received}` : "";
                                out = { role: "pipe", event: ev, status: (obj.message || "") + extra };
                            }
                            ws.send(JSON.stringify(out));
                        } catch {
                            ws.send(JSON.stringify({ role: "pipe", event: "raw", status: s }));
                        }
                    });
                });

                proc.stderr.on("data", (d) => {
                    ws.send(JSON.stringify({ role: "pipe", event: "error", status: d.toString().trim() }));
                });

                return; // não cair na lógica de socket/memcom
            }

            // --- SOCKET / MEMCOM ---
            if (mecanismo === "socket") {
                // 🔹 inicia o servidor uma vez
                if (!servidor) {
                    servidor = spawn(executaveis.socketServer);
                    console.log("Servidor de sockets iniciado:", executaveis.socketServer);

                    servidor.stdout.on("data", (d) => {
                        console.log("Servidor:", d.toString().trim());
                    });

                    servidor.stderr.on("data", (d) => {
                        console.error("Erro Servidor:", d.toString());
                    });

                    servidor.on("close", () => {
                        console.log("Servidor finalizado");
                        servidor = null;
                    });
                }
            }

            if (!executaveis[mecanismo === "socket" ? "socketClient" : mecanismo]) {
                console.error("Mecanismo desconhecido:", mecanismo);
                return;
            }

            if (!cliente) {
                const execPath = executaveis[mecanismo === "socket" ? "socketClient" : mecanismo];
                cliente = spawn(execPath);
                console.log(`Processo ${mecanismo} iniciado: ${execPath}`);

                cliente.stdout.on("data", (data) => {
                    const linhas = data.toString().split(/\r?\n/);
                    linhas.forEach(linha => {
                        if (!linha.trim()) return;
                        try {
                            JSON.parse(linha.trim());
                            ws.send(linha.trim());
                        } catch {
                            console.error("Linha inválida:", linha);
                        }
                    });
                });

                cliente.stderr.on("data", (data) => {
                    console.error("Erro do cliente:", data.toString());
                });

                cliente.on("close", (code) => {
                    console.log(`Processo ${mecanismo} finalizado (código ${code})`);
                    cliente = null;
                });
            }

            // envia texto pro stdin do cliente (socket/memcom)
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
        if (servidor) { // 🔹 encerra o servidor junto
            servidor.kill();
            servidor = null;
        }
    });
});
