#include <windows.h>//Biblioteca do Windows, usada paraa criação do semaforo e acessar a memoria compartilhada.
#include <iostream>//Biblioteca para entrada e saida de dados, usada para a conexão de dados de entrada e saida entre a ponte e esse programa.
#include <string>//Biblioteca para uso da String, caso seja escrito frases na memoria.

int main() {
    //Declaração das variaveis usadas em identificadores do Windows. HANDLE é um identificador, usado para a criação da memoria compartilhada e do semaforo.
    //LPVOID é um tipo de endereçamento onde o ponteiro ira acessar, sendo VOID porque a informação aramzenada não é explicita.
    HANDLE hMapFile = nullptr;//Memoria compartilhada criada.
    LPVOID pData = nullptr;//Criação de um ponteiro que indica onde está as informações na memoria interna.
    HANDLE hSemaphore = nullptr;//Variavel do semaforo criado.

    const int SIZE = 256;//Tamanho da Memoria criada como 256 bytes.
    const char* sharedName = "MySharedMemory";//Nome da Memoria
    const char* semaphoreName = "MySemaphore";//Nome do Semaforo

    // 1. CRIA OS OBJETOS DE SINCRONIZAÇÃO E MEMÓRIA
    hSemaphore = CreateSemaphoreA(NULL, 1, 1, semaphoreName);//O Semaforo criaddo ele determina que ele é criado com as configurações originais do sistema(NULL), com a contagem iniciando em 1 e se limitando a 1 processo por vez. Assim, ele chama o nome do semaforo
    if (hSemaphore == NULL) {//Caso o semaforo não seja criado, ele exibirá essa mensagem e termina o programa imediatamente.
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao criar semaforo\"}" << std::endl;//Esse std::endl é responsavel por avisar o Json que o semaforo não foi criado e que o programa será encerrado
        return 1;//Encerra programa.
    }

    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIZE, sharedName);//Cria a memoria compartilhada.
    //Endereça a memoria a área da memoria interna(INVALID_HANDLE_VALUE), configuração de segurança original do sistema(NULL), permissão para ler e escrever nessa área da memoria(PAGE_READWRITE), delimitação do tamanho dessa memória(0 a SIZE), retoma o nome da memoria.
    if (hMapFile == NULL) {//Verificação da criação da memoria compartilhada. Caso não crie, printa a mensagem abaixo.
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao criar mapeamento\"}" << std::endl;//Mesma variavel que anuncia o Json que não criou a memoria compartilhada.
        CloseHandle(hSemaphore);//Fecha o semaforo para não causar uma fuga de recurso.
        return 1;//Encerra o programa.
    }

    pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIZE);//Aqui o ponteiro acessa a memoria através desses identificadores.
    //acssea a mesma memoria criada anteriormente(hMapfile), concede permissão total ao ponteiro(FILE_MAP_ALL_ACCESS), ponto de partida sendo 0 e a área da memoria sendo de 0 a SIZE.
    if (pData == nullptr) {//Verificação caso o ponteiro não consiga acessar a memoria. Assim, printa essa mensagem de erro.
        std::cerr << "{\"role\": \"memcom\", \"event\": \"error\", \"status\": \"falha ao mapear view\"}" << std::endl;//Mesma variavel que anuncia o Json que não foi possivel acessar a memoria compartilhada.
        CloseHandle(hMapFile);//Fecha a memoria compartilhada
        CloseHandle(hSemaphore);//Fecha o semaforo
        return 1;//Encerra o programa.
    }

    // Envia uma mensagem para o ponte.js dizendo que está pronto
    std::cout << "{\"role\": \"memcom\", \"event\": \"start\", \"status\": \"pronto para receber mensagens\"}" << std::endl;

    std::string line;
    // 2. LOOP PRINCIPAL QUE ESPERA POR MENSAGENS DO STDIN
    //    Ele encerra se a ponte fechar (getline falha) ou se receber "exit"
    while (std::getline(std::cin, line) && line != "exit") {//std::getline(std::cin, line) é uma verificação para leitura da mensagem dentro da memoria. Encerra o while caso não consiga ler(em caso de encerrar o programa a verificação também funciona).
        //line != "exit" verifica que caso tudo que for escrito seja diferente do Exit, ent o while continua.
        // Escreve na memória compartilhada
        WaitForSingleObject(hSemaphore, INFINITE);//Identificador de que o semaforo está em uso e determina o tempo de uso dele
        CopyMemory(pData, line.c_str(), line.length() + 1);//Função de fazer cópia. O ponteiro(pData) carrega a mensagem de origem(line.c_str()) e recebe o numero de bytes correspondete a mensagem escrita(line.length() + 1). 
        ReleaseSemaphore(hSemaphore, 1, NULL);//Libera o semaforo para o proximo programa. Ele informa qual semaforo será liberado(hSemaphore), o estado de retorno dele(1) e a disponiblidade do semaforo durante a execução do programa(NULL)

        // 3. ENVIA UMA CONFIRMAÇÃO DE VOLTA PARA A PONTE VIA STDOUT
        std::cout << "{\"role\": \"memcom\", \"event\": \"write\", \"status\": \"mensagem escrita na memoria: '" << line << "'\"}" << std::endl;
    }

    // 4. LIMPEZA DOS RECURSOS
    std::cout << "{\"role\": \"memcom\", \"event\": \"stop\", \"status\": \"encerrando...\"}" << std::endl;//Printa na tela para informar que o programa foi fechado e avisa isso ao Json através dessa ultima variavel.
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);//Fecha a memoria compartilhada 
    CloseHandle(hSemaphore);//Fecha o semaforo

    return 0;//Encerra o programa totalmente.
}