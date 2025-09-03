#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> //Biblioteca responsável pelos sockets no Windows
#include <ws2tcpip.h> //Biblioteca complementar para implementar TCP/IP
#include <cstdio> //Biblioteca para funções de entrada e saída 
#include <string> //Biblioteca para criação de strings
#include <iostream> //Biblioteca para entrada e saída de dados
#pragma comment(lib, "Ws2_32.lib") // Responsável por linkar a biblioteca Ws2_32.lib
#include <windows.h>


// Função de log simples que imprime no console em formato JSON
void logger(const char* role, const char* event, const char* status) {
	std::printf("{\"role\":\"%s\",\"event\":\"%s\",\"status\":\"%s\"}\n", role, event, status); // imprime o responsável, o evento e o status do evento em formato JSON
	std::printf("\n");
	std::fflush(stdout); // garante que o buffer seja esvaziado na hora
}

int main() {
	sockaddr_in addr{}; // cria o endereço do servido
	addr.sin_family = AF_INET; // Cria a familia do socket do servidor (IPv4)
	addr.sin_port = htons(54000);// porta do servidor
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);// Endereço IP do servidor (localhost)	
	

	logger("server", "sockaddr_in setup", "success"); // log de sucesso na criação do endereço do servidor	


	WSADATA wsadata;

	int iniciar = WSAStartup(MAKEWORD(2, 2), &wsadata);

	if (iniciar != 0) {
		logger("server", "WSAStartup", "error");
		logger("server", "exit", "Erro no WSAStartup");
		return 1; // sai do programa
	}

	logger("server", "WSAStartup", "success");
	logger("server", "info", "Winsock iniciado com sucesso!");

	SOCKET servidor = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);

	if(servidor == INVALID_SOCKET) {
		logger("server", "socket", "error");
		logger("server", "exit", "Erro ao criar socket");
		WSACleanup();
		return 1;
	}

	logger("server", "socket", "success");

	if (bind(servidor, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		logger("server", "bind", "error");
		logger("server", "exit", "Erro no bind");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}

	logger("server", "bind", "success");

	if (listen(servidor, SOMAXCONN) == SOCKET_ERROR) {
		logger("server", "listen", "error");
		logger("server", "exit", "Erro no listen");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}

	logger("server", "listen", "success");
  
    SOCKET cliente = accept(servidor, nullptr, nullptr);

	if(cliente == INVALID_SOCKET){
		logger("server", "accept", "error");
		logger("server", "exit", "Erro no accept ");
		closesocket(servidor);
		WSACleanup();
		return 1;
	}

	logger("server", "accept", "success");

	char buffer[4096]; // armazenamento temporário para a mensagem


	std::string mensagem;  // Mensagem recebida// Mensagem Enviada

	while (true) { // Não para enquanto não terminar de receber a mensagem completa


		ZeroMemory(buffer, 4096); // Zera a memória do buffer


		int bytesRecebidos = recv(cliente, buffer, 4096, 0); // Recebe a mensagem do cliente byte por byte
		

		if (bytesRecebidos == SOCKET_ERROR) { //Caso de erro no recv envia o log de erro e sai do loop

			logger("server", "recv", "error");
			logger("server", "exit", "Erro no recv");
			break;
		} 



		logger("server", "recv", "success");
		

		if (bytesRecebidos == 0) { // Caso o cliente desconecte, bytesRecebidos será 0, então envia o log de desconexão e sai do loop
			
			logger("server", "client_disconnect", "success");
			logger("server", "info", "Cliente desconectou.");
			break;
		}

		mensagem.assign(buffer, bytesRecebidos);
		logger("server", "mensagem_recebida", mensagem.c_str());



		// Echo de volta para o cliente
		int bytesEnviados = send(cliente, buffer, bytesRecebidos, 0);

		if (bytesEnviados == SOCKET_ERROR) {
			logger("server", "send", "error");
		} else {
			logger("server", "send", "success");
		}
		mensagem.assign(buffer, bytesEnviados);
		logger("server", "mensagem_enviada", mensagem.c_str());
	}

		logger("server", "closing client socket", "success");
		closesocket(cliente);
		logger("server", "closing server socket", "success");
		closesocket(servidor);	
		logger("server", "WSACleanup", "success");
		WSACleanup();
		return 0;
}