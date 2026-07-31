#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CERVE_PORT 4040

// This is me, Miguel Duarte, trying to understand servers a little bit.
// Resources: Making Minimalist Web Server in C on Linux | Nir Lichtman [https://www.youtube.com/watch?v=2HrYIl6GpYg]

/*
1 - cria o socket
2 - socket() - configura o socket
3 - cria um endereco
4 - bind(socket, endereco) - conecta o sockete ao endereco
5 - listen() - prepara o socket para escutar pedidos numa fila
6 - accept() - aceita o pedido que estiver na frente da fila
7 - agora para ler o pedido e mandar uma resposta, é uma questao de ler o que esta no arquivo (clientfd) e escrever no arquivo (clientfd)
8 - fechar conexoes
*/

int main(){
    // criando socket ipv4
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0){
        perror("socket failed!");
        exit(1);
    }

    // configurando socket para modo de REUSE, ignorando o tempo de espera default para protocolos TCP. (30seg - 2min). Pelo que entendi.
    int opt = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // criando o endereço para conectar(bind) o socket.
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CERVE_PORT);

    // conectando o socket ao endereço
    if (bind(socketfd, (const struct sockaddr *) &addr, sizeof(addr)) < 0){
        perror("binding failed!");
        exit(1);
    };


    
    printf("Cerve is listening on port %d\n", CERVE_PORT);
    // preparando o socket para receber pedidos.
    if (listen(socketfd, 10) < 0){
        perror("listening failed!");
        exit(1);
    };


    while(1){
        // aceitando o pedido, e salvando o id request
        int clientfd = accept(socketfd, 0, 0);
        if (clientfd == -1){
            perror("accept failed!");
            exit(1);
        }

        
        pid_t pid = fork();
        if (pid == -1) { // ERRO NO FORK
            perror("fork failed!");
            exit(1);
        } else if (pid == 0) { // CHILD
            close(socketfd);

            // simulando processo pesado
            // sleep(5);

            // lendo o que esta escrito no request
            char buffer[256] = {0};
            if (read(clientfd, buffer, sizeof(buffer) - 1) == -1){
                perror("read failed!");
                exit(1);
            };
        
            // enviando uma resposta!
            char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nHola que tal!\n";
            if(write(clientfd, response, strlen(response)) == -1){
                perror("write failed!");
                exit(1);
            };

            close(clientfd);
            exit(0);
        } else {
            // PARENT PROCESS GETS NEXT //
            close(clientfd);
            waitpid(-1, NULL, WNOHANG);
        }

    };
    return 0;
}