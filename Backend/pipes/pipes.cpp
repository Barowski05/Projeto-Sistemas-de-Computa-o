#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

// Fun��o escape_json 
std::string escape_json(const std::string& s) {
    std::string escaped;
    escaped.reserve(s.length());
    for (char c : s) {
        switch (c) {
        case '"':  escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b";  break;
        case '\f': escaped += "\\f";  break;
        case '\n': escaped += "\\n";  break;
        case '\r': escaped += "\\r";  break;
        case '\t': escaped += "\\t";  break;
        default:   escaped += c;     break;
        }
    }
    return escaped;
}

// L�gica do Filho 
void RunChildProcess(HANDLE hReadPipe) {
    std::vector<char> buffer(1024);
    DWORD bytesRead = 0;

    if (ReadFile(hReadPipe, buffer.data(), buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
        std::string mensagem_recebida(buffer.data(), bytesRead);

        
        std::cout << "{"
            << "\"type\": \"result\","
            << "\"status\": \"success\","
            << "\"message\": \"Mensagem recebida pelo processo Filho.\","
            << "\"data_received\": \"" << escape_json(mensagem_recebida) << "\""
            << "}" << std::endl;
    }
    else {
        std::cout << "{\"type\": \"result\", \"status\": \"error\", \"message\": \"Filho falhou ao ler do pipe.\"}" << std::endl;
    }
    CloseHandle(hReadPipe);
}

// L�gica do Pai 
void RunParentProcess(const std::string& messageFromUser) {
   
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return;
    SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    char szFileName[MAX_PATH];
    GetModuleFileNameA(NULL, szFileName, MAX_PATH);

    std::string cmdLine = "\"" + std::string(szFileName) + "\" modo_filho " + std::to_string((unsigned long long)hReadPipe);

    if (!CreateProcessA(NULL, &cmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        std::cout << "{\"type\": \"result\", \"status\": \"error\", \"message\": \"Pai falhou ao criar processo filho.\"}" << std::endl;
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }


    std::cout << "{\"type\": \"status\", \"source\": \"Sistema\", \"message\": \"Processo Pai (PID: " << GetCurrentProcessId() << ") iniciou com sucesso.\"}" << std::endl;
    std::cout << "{\"type\": \"status\", \"source\": \"Sistema\", \"message\": \"Processo Filho (PID: " << pi.dwProcessId << ") foi criado.\"}" << std::endl;

   
    CloseHandle(hReadPipe);
    DWORD bytesWritten = 0;
    WriteFile(hWritePipe, messageFromUser.c_str(), messageFromUser.length() + 1, &bytesWritten, NULL);
    CloseHandle(hWritePipe);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// Fun��o Main 
int main(int argc, char* argv[]) {
    
    if (argc == 3 && std::string(argv[1]) == "modo_filho") {
        HANDLE hPipe = (HANDLE)std::stoull(argv[2]);
        RunChildProcess(hPipe);
    }
    else {
       
        std::cout << "Digite sua mensagem para teste:" << std::endl;
        std::string messageFromTerminal;
        std::getline(std::cin, messageFromTerminal);
        if (!messageFromTerminal.empty()) {
            RunParentProcess(messageFromTerminal);
        }
    }
    return 0;
}