// Importa módulo para criar processos filhos
const { spawn } = require("child_process");
// Importa biblioteca WebSocket
const WebSocket = require("ws");

// Cria servidor WebSocket na porta 8080
const wss = new WebSocket.Server({ port: 8080 });
console.log(" Bridge Node.js rodando em ws://localhost:8080");

// Caminhos dos executáveis 
const executaveis = {
    pipes: "./Pipes/pipes/pipes/pipes/Debug/pipes.exe",
    socketClient: "./Sockets/SocketServidor/SocketCliente/Debug/SocketCliente.exe",  
    socketServer: "./Sockets/SocketServidor/SocketServidor/Debug/SocketServidor.exe", 
    memcom: "./Memcomp/Debug/Memcomp.exe"
};

// Para armazenar os processos atuais
let cliente = null;
let servidor = null; 

// Evento de nova conexão WebSocket
wss.on("connection", (ws) => {
    console.log(" Frontend conectado ao WebSocket");

    // Evento de recebimento de mensagem do frontend
    ws.on("message", (msg) => {
        try {
            const parsed = JSON.parse(msg.toString());
            const mecanismo = parsed.mechanism;   // socket | memcom |"pipe
            const texto = parsed.mensagem || "";

            if (mecanismo === "pipe") {
                // Cria processo para pipes.exe passando o texto como argumento
                const proc = spawn(executaveis.pipes, [texto]);

                // Trata saída padrão do processo
                proc.stdout.on("data", (data) => {
                    data.toString().split(/\r?\n/).forEach((linha) => {
                        const s = linha.trim();
                        if (!s) return;
                        try {
                            // Tenta interpretar como JSON
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

                // Trata saída de erro do processo
                proc.stderr.on("data", (d) => {
                    ws.send(JSON.stringify({ role: "pipe", event: "error", status: d.toString().trim() }));
                });

                return; // não cair na lógica de socket/memcom
            }

            // --- SOCKET / MEMCOM ---
            if (mecanismo === "socket") {
                // Inicia servidor de socket se não estiver rodando
                if (!servidor) {
                    servidor = spawn(executaveis.socketServer);
                    console.log("Servidor de sockets iniciado:", executaveis.socketServer);

                    servidor.stdout.on("data", (d) => {
                        console.log("Servidor:", d.toString().trim());
                    });

                    servidor.stderr.on("data", (d) => {
                        console.error("Erro Servidor:", d.toString());
                    });

                    // Quando servidor termina, limpa variável
                    servidor.on("close", () => {
                        console.log("Servidor finalizado");
                        servidor = null;
                    });
                }
            }

            // Verifica se mecanismo é válido
            if (!executaveis[mecanismo === "socket" ? "socketClient" : mecanismo]) {
                console.error("Mecanismo desconhecido:", mecanismo);
                return;
            }

            // Inicia processo cliente se não estiver rodando
            if (!cliente) {
                const execPath = executaveis[mecanismo === "socket" ? "socketClient" : mecanismo];
                cliente = spawn(execPath);
                console.log(`Processo ${mecanismo} iniciado: ${execPath}`);

                cliente.stdout.on("data", (data) => {
                    const linhas = data.toString().split(/\r?\n/);
                    linhas.forEach(linha => {
                        if (!linha.trim()) return;
                        try {
                            // Se for JSON, envia para frontend
                            JSON.parse(linha.trim());
                            ws.send(linha.trim());
                        } catch {
                            console.error("Linha inválida:", linha);
                        }
                    });
                });

                // Trata saída de erro do cliente
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
            // Erro ao processar mensagem do frontend
            console.error("Erro ao processar mensagem do frontend:", msg, err);
        }
    });

    // Evento de desconexão do frontend
    ws.on("close", () => {
        console.log(" Frontend desconectou.");
        // Finaliza processos se existirem
        if (cliente) {
            cliente.kill();
            cliente = null;
        }
        if (servidor) { 
            servidor.kill();
            servidor = null;
        }
    });
});
