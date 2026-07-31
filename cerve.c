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

    #define MAX_METHOD 128
    #define MAX_PATH 256
    #define MAX_VERSION 128

    char server_root[MAX_PATH];
    realpath(".", server_root);


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

            #define BUFFER_SIZE 256
            // lendo o que esta escrito no request
            char buffer[BUFFER_SIZE] = {0};
            if (read(clientfd, buffer, sizeof(buffer) - 1) == -1){
                perror("read failed!");
                exit(1);
            };
        
        
            // parsing http header
            //GET / HTTP/1.1\r\n
            // getting method, path and version
            char currentChar;

            
            char method[MAX_METHOD], path[MAX_PATH], version[MAX_VERSION];

            int cursor = 0;

            // getting method
            int i = 0;
            while (1) {
                if (i == MAX_METHOD || cursor == BUFFER_SIZE) break;
                currentChar = buffer[cursor];
                if (currentChar == ' ') break;
                method[i] = currentChar;
                cursor++;
                i++;
            }
            method[i] = '\0';

            // skip whitespace
            while (cursor < BUFFER_SIZE && buffer[cursor] == ' ') cursor++;

            // getting path
            i = 0;
            while (1) {
                if (i == MAX_PATH || cursor == BUFFER_SIZE) break;
                currentChar = buffer[cursor];
                if (currentChar == ' ') break;
                path[i] = currentChar;
                cursor++;
                i++;
            }
            path[i] = '\0';

            // skip whitespace
            while (cursor < BUFFER_SIZE && buffer[cursor] == ' ') cursor++;

            // getting path
            i = 0;
            while (1) {
                if (i == MAX_VERSION || cursor == BUFFER_SIZE) break;
                currentChar = buffer[cursor];
                if (currentChar == '\r') break;
                version[i] = currentChar;
                cursor++;
                i++;
            }
            version[i] = '\0';
            printf("\nmethod: '%s';\npath: '%s';\nversion: '%s';\n\n", method, path, version);


            char full_path[MAX_PATH];
            if (strcmp(path, "/") == 0){
                strcpy(full_path, "index.html");
            } else {
                strcpy(full_path, path + 1);
            }


            char resolved_path[MAX_PATH];
            if (realpath(full_path, resolved_path) == NULL) {
                printf("failed to resolve desired path!\n");
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
                if(write(clientfd, response, strlen(response)) == -1){
                    perror("write failed!");
                    exit(1);
                };
                close(clientfd);
                exit(0);
            }

            size_t server_root_size = strlen(server_root);
            if (strncmp(resolved_path, server_root, server_root_size) != 0) {
                printf("error: path traversing!!!\n");
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
                if(write(clientfd, response, strlen(response)) == -1){
                    perror("write failed!");
                    exit(1);
                };
                close(clientfd);
                exit(0);
            }

            if (!(resolved_path[server_root_size] == '/' || resolved_path[server_root_size] == '\0')) {
                printf("error: path traversing!!!\n");
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
                if(write(clientfd, response, strlen(response)) == -1){
                    perror("write failed!");
                    exit(1);
                };
                close(clientfd);
                exit(0);
            }
            
            printf("directing to %s:\n\n", full_path);

            FILE *file = fopen(full_path, "rb");
            if (file == NULL) {
                perror("failed to open file!");
                // enviando uma resposta!
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
                if(write(clientfd, response, strlen(response)) == -1){
                    perror("write failed!");
                    exit(1);
                };
                
            } else {
                printf("File opened successfully!\n");

                fseek(file, 0, SEEK_END); // go to end
                long fileSize = ftell(file); // get end pos
                fseek(file, 0, SEEK_SET);
    
                char *content = malloc(fileSize);
    
                fread(content, fileSize, 1, file);
                fclose(file);

                // make and send header
                char header[256];
                snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", fileSize);
                if(write(clientfd, header, strlen(header)) == -1){
                    perror("header write failed!");
                    exit(1);
                };
                // send content
                if(write(clientfd, content, fileSize) == -1){
                    perror("content write failed!");
                    free(content);
                    exit(1);
                };
                free(content);

            }
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