#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> // Biblioteca responsável pelos sockets no Windows
#include <ws2tcpip.h> // Biblioteca complementar para implementar TCP/IP
#include <cstdio> // Biblioteca para funções de entrada e saída
#include <string> // Biblioteca para criação de strings
#include <iostream> // Biblioteca para entrada e saída de dados
#include <cstring> // Biblioteca para manipulação de strings C
#pragma comment(lib, "Ws2_32.lib") // Linka a biblioteca Ws2_32.lib
#include <windows.h>

// Função de log que imprime no console em formato JSON
void logger(const char* role, const char* event, const char* status) {
	std::printf("{\"role\":\"%s\",\"event\":\"%s\",\"status\":\"%s\"}\n", role, event, status); // imprime o responsável, o evento e o status do evento em formato JSON
	std::fflush(stdout); // garante que o buffer seja esvaziado na hora
}

int main() {
	// Configura o endereço do servidor
	sockaddr_in addr{};
	addr.sin_family = AF_INET; // Define a família do socket (IPv4)
	addr.sin_port = htons(54000); // Define a porta do servidor
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // Define o IP do servidor (localhost)

	logger("client", "sockaddr_in setup", "success"); // Log de sucesso na configuração do endereço

	// Inicializa a biblioteca Winsock
	WSADATA wsadata;
	int iniciar = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iniciar != 0) {
		logger("client", "WSAStartup", "error");
		logger("client", "exit", "Erro no WSAStartup"); // Log de erro
		return 1;
	}
	logger("client", "WSAStartup", "success");
	logger("client", "info", "Winsock iniciado com sucesso!");

	// Cria o socket do cliente
	SOCKET cliente = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (cliente == INVALID_SOCKET) {
		logger("client", "socket", "error");
		logger("client", "exit", "Erro ao criar socket");
		WSACleanup();
		return 1;
	}
	logger("client", "socket", "success");

	// Conecta ao servidor
	if (connect(cliente, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		logger("client", "connect", "error");
		logger("client", "exit", "Erro ao conectar no servidor");
		closesocket(cliente);
		WSACleanup();
		return 1;
	}
	logger("client", "connect", "success");

	std::printf("Digite uma mensagem para enviar ao servidor : ");
	std::fflush(stdout);
	char buffer[4096]; // Buffer para armazenar dados enviados/recebidos

	// Loop principal para comunicação com o servidor
	while (true) {
		std::cin.getline(buffer, sizeof(buffer)); // Lê a linha inteira do usuário

		// Verifica se o usuário deseja sair
		if (strcmp(buffer, "sair") == 0 || strcmp(buffer, "SAIR") == 0 || strcmp(buffer, "Sair") == 0) {
			logger("client", "user_exit", "success");
			logger("client", "info", "Encerrando o cliente conforme solicitado pelo usuário.");
			break;
		}

		// Envia a mensagem para o servidor
		int enviar = send(cliente, buffer, strlen(buffer), 0);

		if (enviar == SOCKET_ERROR) {
			logger("client", "send", "error");
			logger("client", "exit", "Erro ao enviar dados");
			closesocket(cliente);
			WSACleanup();
			return 1;
		}

		logger("client", "send", "success");
		logger("client", "mensagem_enviada", std::string(buffer, 0, enviar).c_str()); // Log da mensagem enviada
        std::printf("Mensagem enviada: %s\n", std::string(buffer, 0, enviar).c_str());
        std::fflush(stdout);

        // Recebe a resposta do servidor
        int receber = recv(cliente, buffer, sizeof(buffer), 0);

        if (receber > 0) {
            logger("client", "recv", "success");
            logger("client", "mensagem_recebida", std::string(buffer, receber).c_str()); // Log da mensagem recebida
            std::printf("Mensagem recebida: %s\n", std::string(buffer, receber).c_str());
            std::fflush(stdout);
        }
		else if (receber == 0) {
			logger("client", "recv", "connection closed");
			logger("client", "info", "Conexão fechada pelo servidor.");
			break;
		}
		else {
			logger("client", "recv", "error");
			logger("client", "exit", "Erro ao receber dados");
			closesocket(cliente);
			WSACleanup();
			return 1;
		}

		std::printf("Digite uma mensagem para enviar ao servidor : ");
		std::fflush(stdout);
	}
	// Finaliza a conexão e limpa recursos
	closesocket(cliente);
	WSACleanup();
	logger("client", "cleanup", "success");
	return 0;
}