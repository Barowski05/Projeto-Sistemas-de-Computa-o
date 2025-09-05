#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    HANDLE hMapFile = nullptr;
    LPVOID pData = nullptr;
    HANDLE hSemaphore = nullptr;
    HANDLE hEvent = nullptr;

    const int SIZE = 256;
    const char* sharedName = "MySharedMemory";
    const char* semaphoreName = "MySemaphore";
    const char* eventName = "MyProcess1FinishedEvent";

    hSemaphore = CreateSemaphoreA(NULL, 1, 1, semaphoreName);
    if (hSemaphore == NULL) { return 1; }

    hEvent = CreateEventA(NULL, TRUE, FALSE, eventName);
    if (hEvent == NULL) { CloseHandle(hSemaphore); return 1; }

    // --- MODO LANÇADOR AUTOMÁTICO ---
    // --- MODO LANÇADOR AUTOMÁTICO ---
    if (argc < 2) {
        std::cout << "Modo Lancador: Iniciando processos 1 e 2..." << std::endl;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        // Lança o Processo 1
        std::string cmd1 = "\"" + std::string(exePath) + "\" 1";
        if (!CreateProcessA(NULL, &cmd1[0], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            std::cerr << "ERRO AO CRIAR PROCESSO 1: " << GetLastError() << std::endl;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Lança o Processo 2
        std::string cmd2 = "\"" + std::string(exePath) + "\" 2";
        if (!CreateProcessA(NULL, &cmd2[0], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            std::cerr << "ERRO AO CRIAR PROCESSO 2: " << GetLastError() << std::endl;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // --- ADIÇÃO PARA DEBUG ---
        // Mantém a janela do lançador aberta para vermos as mensagens de erro
        std::cout << "Lancador finalizado. Pressione Enter para fechar." << std::endl;
        std::cin.get();
    }
    // --- PROCESSO 1 (ESCRITOR) ---
    else if (strcmp(argv[1], "1") == 0) {
        hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIZE, sharedName);
        pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIZE);

        std::string userInput;
        std::cout << "Processo 1: Digite a mensagem para compartilhar: ";
        std::getline(std::cin, userInput);

        WaitForSingleObject(hSemaphore, INFINITE);
        std::cout << "Processo 1: Escrevendo na memoria..." << std::endl;
        CopyMemory(pData, userInput.c_str(), userInput.length() + 1);
        ReleaseSemaphore(hSemaphore, 1, NULL);

        std::cout << "Processo 1: Sinalizando que o trabalho foi concluido." << std::endl;
        SetEvent(hEvent);

        std::cout << "Pressione Enter para fechar." << std::endl;
        std::cin.get();
        UnmapViewOfFile(pData);
        CloseHandle(hMapFile);
    }
    // --- PROCESSO 2 (LEITOR/ATUALIZADOR) ---
    else if (strcmp(argv[1], "2") == 0) {
        std::cout << "Processo 2: Aguardando o Processo 1 terminar de escrever..." << std::endl;

        WaitForSingleObject(hEvent, INFINITE);

        std::cout << "Processo 2: Sinal recebido! Acessando a memoria..." << std::endl;
        hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedName);
        pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIZE);

        WaitForSingleObject(hSemaphore, INFINITE);
        std::cout << "Processo 2: Leu: '" << (char*)pData << "'" << std::endl;
        const char* updateMsg = "Memoria atualizada pelo Processo 2!";
        CopyMemory(pData, updateMsg, strlen(updateMsg) + 1);
        std::cout << "Processo 2: Mensagem atualizada." << std::endl;
        ReleaseSemaphore(hSemaphore, 1, NULL);

        std::cout << "Pressione Enter para fechar." << std::endl;
        std::cin.get();
        UnmapViewOfFile(pData);
        CloseHandle(hMapFile);
    }

    CloseHandle(hEvent);
    CloseHandle(hSemaphore);
    return 0;
}