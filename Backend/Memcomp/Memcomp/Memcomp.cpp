#include <windows.h>
#include <iostream>
#include <string>

int main() {
    HANDLE hMapFile = nullptr;
    LPVOID pData = nullptr;
    HANDLE hSemaphore = nullptr;

    const int SIZE = 256;
    const char* sharedName = "MySharedMemory";
    const char* semaphoreName = "MySemaphore";

    // 1. CRIA OS OBJETOS DE SINCRONIZAÇÃO E MEMÓRIA
    //    (Lógica que antes era do Processo 1)
    hSemaphore = CreateSemaphoreA(NULL, 1, 1, semaphoreName);
    if (hSemaphore == NULL) {
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao criar semaforo\"}" << std::endl;
        return 1;
    }

    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIZE, sharedName);
    if (hMapFile == NULL) {
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao criar mapeamento\"}" << std::endl;
        CloseHandle(hSemaphore);
        return 1;
    }

    pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIZE);
    if (pData == nullptr) {
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao mapear view\"}" << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hSemaphore);
        return 1;
    }

    // Envia uma mensagem para o ponte.js dizendo que está pronto
    std::cout << "{\"role\": \"memcom\", \"event\": \"start\", \"status\": \"pronto para receber mensagens\"}" << std::endl;

    std::string line;
    // 2. LOOP PRINCIPAL QUE ESPERA POR MENSAGENS DO STDIN
    //    Ele encerra se a ponte fechar (getline falha) ou se receber "exit"
    while (std::getline(std::cin, line) && line != "exit") {

        // Escreve na memória compartilhada
        WaitForSingleObject(hSemaphore, INFINITE);
        CopyMemory(pData, line.c_str(), line.length() + 1);
        ReleaseSemaphore(hSemaphore, 1, NULL);

        // 3. ENVIA UMA CONFIRMAÇÃO DE VOLTA PARA A PONTE VIA STDOUT
        std::cout << "{\"role\": \"memcom\", \"event\": \"write\", \"status\": \"mensagem escrita na memoria: '" << line << "'\"}" << std::endl;
    }

    // 4. LIMPEZA DOS RECURSOS
    std::cout << "{\"role\": \"memcom\", \"event\": \"stop\", \"status\": \"encerrando...\"}" << std::endl;
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);

    return 0;
}