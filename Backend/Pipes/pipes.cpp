#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

// Função para imprimir logs em JSON para o stdout (saída padrão).
// O "std::unitbuf" garante que cada linha seja enviada imediatamente,
// o que é essencial para a interface gráfica capturar os logs em tempo real.
void print_json_log(const std::string& source, const std::string& event, const std::string& data) {
    std::cout << std::unitbuf
              << "{"
              << "\"source\": \"" << source << "\", "
              << "\"event\": \"" << event << "\", "
              << "\"data\": \"" << data << "\""
              << "}\n";
}

// Lógica que será executada quando o programa estiver no "modo filho"
void RunChildProcess(HANDLE hReadPipe) {
    print_json_log("Filho", "Iniciado", "Processo filho iniciado com sucesso.");

    std::vector<char> buffer(1024);
    DWORD bytesRead = 0; // DWORD é um tipo de inteiro do Windows

    // Lê os dados do pipe. Esta função pausa (bloqueia) a execução do filho
    // até que o pai envie alguma coisa ou feche o pipe.
    if (ReadFile(hReadPipe, buffer.data(), buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
        std::string mensagem_recebida(buffer.data(), bytesRead);
        print_json_log("Filho", "Mensagem Recebida", mensagem_recebida);
    } else {
        print_json_log("Filho", "Erro", "Falha ao ler do pipe.");
    }

    CloseHandle(hReadPipe); // Limpa o recurso
    print_json_log("Filho", "Finalizado", "Tarefa concluída.");
}

// Lógica que será executada quando o programa estiver no "modo pai"
void RunParentProcess() {
    print_json_log("Sistema", "Iniciando módulo de Pipes (Windows)", "---");

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; // Permite que o handle seja herdado pelo processo filho
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        print_json_log("Pai", "Erro", "Falha ao criar o pipe.");
        return;
    }
    print_json_log("Pai", "Pipe Criado", "Pipe criado com sucesso.");

    // Evita que o filho herde o handle de escrita desnecessariamente
    SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    char szFileName[MAX_PATH];
    GetModuleFileNameA(NULL, szFileName, MAX_PATH);

    // Linha de comando para o filho: "meu_programa.exe modo_filho <handle_para_leitura>"
    std::string cmdLine = "\"" + std::string(szFileName) + "\" modo_filho " + std::to_string((unsigned long long)hReadPipe);
    
    print_json_log("Pai", "Iniciando Filho", "Invocando processo filho...");

    if (!CreateProcessA(NULL, &cmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        print_json_log("Pai", "Erro", "Falha ao criar o processo filho.");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }
    
    print_json_log("Pai", "Filho Criado", "PID: " + std::to_string(pi.dwProcessId));
    
    // O pai não precisa mais da ponta de leitura, o filho já a herdou.
    CloseHandle(hReadPipe);

    std::string mensagem = "Olá do processo Pai! Projeto de IPC funcionando!";
    DWORD bytesWritten = 0;
    
    print_json_log("Pai", "Enviando Mensagem", mensagem);
    if (!WriteFile(hWritePipe, mensagem.c_str(), mensagem.length(), &bytesWritten, NULL)) {
        print_json_log("Pai", "Erro", "Falha ao escrever no pipe.");
    }
    
    // Fecha o handle de escrita. Isso sinaliza para o filho que não há mais dados a serem enviados.
    CloseHandle(hWritePipe);

    // Espera o processo filho terminar sua execução
    WaitForSingleObject(pi.hProcess, INFINITE);
    print_json_log("Pai", "Finalizado", "Processo filho terminou. Encerrando.");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(int argc, char* argv[]) {
    // Se o programa foi chamado com 2 argumentos e o primeiro é "modo_filho"
    if (argc == 3 && std::string(argv[1]) == "modo_filho") {
        // Converte o segundo argumento (o handle) de string para HANDLE
        HANDLE hPipe = (HANDLE)std::stoull(argv[2]);
        RunChildProcess(hPipe);
    } else {
        // Caso contrário, executa como processo Pai
        RunParentProcess();
    }
    return 0;
}