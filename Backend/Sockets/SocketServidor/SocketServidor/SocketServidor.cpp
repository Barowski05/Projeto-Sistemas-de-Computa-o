#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> // Biblioteca responsável pelos sockets no Windows
#include <ws2tcpip.h> // Biblioteca complementar para implementar TCP/IP
#include <cstdio> // Biblioteca para funções de entrada e saída
#include <string> // Biblioteca para criação de strings
#include <iostream> // Biblioteca para entrada e saída de dados
#pragma comment(lib, "Ws2_32.lib") // Responsável por linkar a biblioteca Ws2_32.lib
#include <windows.h>

// Função de log simples que imprime no console em formato JSON
void logger(const char* role, const char* event, const char* status) {
	std::printf("{\"role\":\"%s\",\"event\":\"%s\",\"status\":\"%s\"}\n", role, event, status); // imprime o responsável, o evento e o status do evento em formato JSON
	std::printf("\n");
	std::fflush(stdout); // garante que o buffer seja esvaziado na hora
}

int main() {
	// Configura o endereço do servidor
	sockaddr_in addr{};
	addr.sin_family = AF_INET; // Define a família do socket (IPv4)
	addr.sin_port = htons(54000); // Define a porta do servidor
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // Define o IP do servidor (localhost)

	logger("server", "sockaddr_in setup", "success"); // Log de sucesso na configuração do endereço

	// Inicializa a biblioteca Winsock
	WSADATA wsadata;
	int iniciar = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iniciar != 0) {
		logger("server", "WSAStartup", "error");
		logger("server", "exit", "Erro no WSAStartup"); // Log de erro
		return 1;
	}
	logger("server", "WSAStartup", "success");
	logger("server", "info", "Winsock iniciado com sucesso!");

	// Cria o socket do servidor
	SOCKET servidor = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
	if(servidor == INVALID_SOCKET) {
		logger("server", "socket", "error");
		logger("server", "exit", "Erro ao criar socket");
		WSACleanup();
		return 1;
	}
	logger("server", "socket", "success");

	// Faz o bind do socket ao endereço e porta
	if (bind(servidor, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		logger("server", "bind", "error");
		logger("server", "exit", "Erro no bind");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}
	logger("server", "bind", "success");

	// Coloca o socket em modo de escuta
	if (listen(servidor, SOMAXCONN) == SOCKET_ERROR) {
		logger("server", "listen", "error");
		logger("server", "exit", "Erro no listen");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}
	logger("server", "listen", "success");
  
    // Aceita uma conexão de cliente
    SOCKET cliente = accept(servidor, nullptr, nullptr);
	if(cliente == INVALID_SOCKET){
		logger("server", "accept", "error");
		logger("server", "exit", "Erro no accept ");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}
	logger("server", "accept", "success");

	char buffer[4096]; // Buffer para armazenar dados recebidos/enviados
	std::string mensagem; // String para armazenar a mensagem recebida

	// Loop principal para comunicação com o cliente
	while (true) {
		ZeroMemory(buffer, 4096); // Limpa o buffer

		int bytesRecebidos = recv(cliente, buffer, 4096, 0); // Recebe dados do cliente
		if (bytesRecebidos == SOCKET_ERROR) {
			logger("server", "recv", "error");
			logger("server", "exit", "Erro no recv");
			break;
		}
		logger("server", "recv", "success");

		if (bytesRecebidos == 0) { // Cliente desconectou
			logger("server", "client_disconnect", "success");
			logger("server", "info", "Cliente desconectou.");
			break;
		}

		mensagem.assign(buffer, bytesRecebidos); // Armazena a mensagem recebida
		logger("server", "mensagem_recebida", mensagem.c_str()); // Log da mensagem recebida

		// Envia de volta para o cliente (echo)
		int bytesEnviados = send(cliente, buffer, bytesRecebidos, 0);

		if (bytesEnviados == SOCKET_ERROR) {
			logger("server", "send", "error");
		} else {
			logger("server", "send", "success");
		}
		mensagem.assign(buffer, bytesEnviados); // Armazena a mensagem enviada
		logger("server", "mensagem_enviada", mensagem.c_str()); // Log da mensagem enviada
	}

	// Finaliza a conexão e limpa recursos
	logger("server", "closing client socket", "success");
	closesocket(cliente);
	logger("server", "closing server socket", "success");
	closesocket(servidor);
	logger("server", "WSACleanup", "success");
	WSACleanup();
	return 0;
}