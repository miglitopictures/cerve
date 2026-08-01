#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void buildMIME(char *mime_buffer, char *file_path) {
    char *extension = strrchr(file_path, '.');
    if (extension == NULL){
        strcpy(mime_buffer, "application/octet-stream");
    } else { 
        if (strcmp(extension,".html") == 0){
            strcpy(mime_buffer, "text/html");
        } else if (strcmp(extension,".css") == 0){
            strcpy(mime_buffer, "text/css");
        } else if (strcmp(extension,".js") == 0){
            strcpy(mime_buffer, "application/javascript");
        } else if (strcmp(extension,".png") == 0){
            strcpy(mime_buffer, "image/png");
        } else if (strcmp(extension,".jpg") == 0){
            strcpy(mime_buffer, "image/jpg");
        } else {
            strcpy(mime_buffer, "application/octet-stream");
        }
    }
}

int sendFile(int client_fd, char *file_path) {
    printf("directing to %s:\n\n", file_path);
    FILE *file = fopen(file_path, "rb");

    if (file == NULL) {
        perror("failed to open file!");
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
        if(write(client_fd, response, strlen(response)) == -1){
            perror("send file 404 write failed!");
            return 1;
        };
        return 1;
    } else {
        printf("File opened successfully!\n");
        
        long fileSize;
        { // GET EXACT FILE SIZE
            fseek(file, 0, SEEK_END);
            fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);
        }

        char *content;
        { // EXTRACT CONTENT FROM FILE AND CLOSE IT
            content = malloc(fileSize);
            fread(content, fileSize, 1, file);
            fclose(file);
        }

        char mime[32];
        buildMIME(mime, file_path);

        // make and send header
        char header[256];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nCache-Control: no-cache\r\nContent-Length: %ld\r\n\r\n", mime, fileSize);
        if(write(client_fd, header, strlen(header)) == -1){
            perror("header write failed!");
            return 1;
        };
        // send content
        if(write(client_fd, content, fileSize) == -1){
            perror("content write failed!");
            free(content);
            return 1;
        };
        free(content);
        return 0;
    }
}

int send404(int client_fd, int redirect_on_404, char *not_found_page){
    if (redirect_on_404){
        return sendFile(client_fd, not_found_page);
    } else {
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nSorry!\n";
        if(write(client_fd, response, strlen(response)) == -1){
            perror("404 write failed!");
            return 1;
        };
        return 0;
    }
}

int main(int argc, char *argv[]){

    #define MAX_METHOD 128
    #define MAX_PATH 256
    #define MAX_VERSION 128

    // porta default
    int port = 4040;

    // 404 behaviour defaults
    int redirect_on_404 = 0;
    char not_found_page[MAX_PATH] = "404.html";

    /* INTEPRETANDO FLAGS [getopt()] */ 

    // -p [port]| especifica a porta para conectar o servidor
    // -f [falback_404_page]| 404 fallback: redireciona para fallback_404_page especificada ou para "404.html" como padrão.

    /*
        getopt(argc, argv, optstring)
        optstring é um string com suas flags, caso a flag precise um argumento (ex.: -f hello.txt) usamos o ":" após a letra na string optstring.
        caso o argumento seja opcional, usamos "::"

        retorna, em caso de sucesso, a opção da vez. quando todas as opcoes são lidas, retorna -1.
        se encontrar uma opção não for estabelecida em optstring, retorna "?" e coloca a opcao desconhecida em "optopt".

        quando a opcao tem um argumento, o argumento pode ser acessado em (char *optarg);
        se o argumento não existe (flags booleanas ou de argumento opcional), optarg é NULL.
    */

    // recursos de estudo para getopt():
    // - https://www.geeksforgeeks.org/c/getopt-function-in-c-to-parse-command-line-arguments/
    // - https://man7.org/linux/man-pages/man3/getopt.3.html


    int option;
    while((option = getopt(argc, argv,"p:f::")) != -1){
        switch (option)
        {
        case 'p': {
            int converted_optarg = atoi(optarg);
            if (converted_optarg != 0){
                port = converted_optarg;
            } else {
                printf("CONFIG_WARNING: [-p] port-specification | Invalid port selected, using default (:%d).\n", port);
            }
            break;
        }
        case 'f': {
            redirect_on_404 = 1;
            if (optarg != NULL) strcpy(not_found_page, optarg);
            printf("CONFIG: [-f] fallback-on-404 | redirecting to '%s' on page not found (404).\n", not_found_page);
            break;
        }
        case '?': {
            printf("CONFIG_WARNING: Unknown config [-%c]\n", optopt);
            break;
        }
        default:
            break;
        }
    }

    printf("\n");




    // criando socket ipv4
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        perror("socket failed!");
        exit(1);
    }

    // configurando socket para modo de REUSE, ignorando o tempo de espera default para protocolos TCP. (30seg - 2min). Pelo que entendi.
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // criando o endereço para conectar(bind) o socket.
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // conectando o socket ao endereço
    if (bind(socket_fd, (const struct sockaddr *) &addr, sizeof(addr)) < 0){
        perror("binding failed!");
        exit(1);
    };


    
    printf("Cerve is listening on port http://localhost:%d\n", port);
    // preparando o socket para receber pedidos.
    if (listen(socket_fd, 10) < 0){
        perror("listening failed!");
        exit(1);
    };

    
    // recurso: 
    char server_root[PATH_MAX];
    realpath(".", server_root);


    while(1){
        // aceitando o pedido, e salvando o id request
        int client_fd = accept(socket_fd, 0, 0);
        if (client_fd == -1){
            perror("accept failed!");
            exit(1);
        }

        
        pid_t pid = fork();
        if (pid == -1) { // ERRO NO FORK
            perror("fork failed!");
            exit(1);
        } else if (pid == 0) { // CHILD
            close(socket_fd);

            // simulando processo pesado // DEBUG
            // sleep(5);

            #define BUFFER_SIZE 256
            // lendo o que esta escrito no request
            char buffer[BUFFER_SIZE] = {0};
            if (read(client_fd, buffer, sizeof(buffer) - 1) == -1){
                perror("read failed!");
                exit(1);
            };
        
        
            // interpretando http header
            char method[MAX_METHOD], path[MAX_PATH], version[MAX_VERSION];
            { // PARSE HEADER
                
                char current_char;
                int cursor = 0;
    
                // pegando method
                int i = 0;
                while (1) {
                    if (i == MAX_METHOD || cursor == BUFFER_SIZE) break;
                    current_char = buffer[cursor];
                    if (current_char == ' ') break;
                    method[i] = current_char;
                    cursor++;
                    i++;
                }
                method[i] = '\0';
    
                // pulando whitespace
                while (cursor < BUFFER_SIZE && buffer[cursor] == ' ') cursor++;
    
                // pegando path
                i = 0;
                while (1) {
                    if (i == MAX_PATH || cursor == BUFFER_SIZE) break;
                    current_char = buffer[cursor];
                    if (current_char == ' ') break;
                    path[i] = current_char;
                    cursor++;
                    i++;
                }
                path[i] = '\0';
    
                // pulando whitespace
                while (cursor < BUFFER_SIZE && buffer[cursor] == ' ') cursor++;
    
                // pegando path
                i = 0;
                while (1) {
                    if (i == MAX_VERSION || cursor == BUFFER_SIZE) break;
                    current_char = buffer[cursor];
                    if (current_char == '\r') break;
                    version[i] = current_char;
                    cursor++;
                    i++;
                }
                version[i] = '\0';
                printf("\nmethod: '%s';\npath: '%s';\nversion: '%s';\n\n", method, path, version);
            }


            // get full path
            char full_path[MAX_PATH];
            if (strcmp(path, "/") == 0){
                strcpy(full_path, "index.html");
            } else {
                strcpy(full_path, path + 1);
            }

            // resolve full path (absolute)
            char resolved_path[PATH_MAX];
            if (realpath(full_path, resolved_path) == NULL) {
                printf("failed to resolve desired path!\n");
                if(send404(client_fd, redirect_on_404, not_found_page) != 0){
                    exit(1);
                };
                close(client_fd);
                exit(0);
            }

            // test path traversal
            size_t server_root_size = strlen(server_root);
            if (strncmp(resolved_path, server_root, server_root_size) != 0) {
                printf("error: path traversing!!!\n");
                if(send404(client_fd, redirect_on_404, not_found_page) != 0){
                    exit(1);
                };
                close(client_fd);
                exit(0);
            }
            if (!(resolved_path[server_root_size] == '/' || resolved_path[server_root_size] == '\0')) {
                printf("error: path traversing!!!\n");
                if(send404(client_fd, redirect_on_404, not_found_page) != 0){
                    exit(1);
                };
                close(client_fd);
                exit(0);
            }
            

            sendFile(client_fd, full_path);
            close(client_fd);
            exit(0);

        } else {
            // PARENT PROCESS GETS NEXT //index.html
            close(client_fd);
            waitpid(-1, NULL, WNOHANG);
        }

    };
    return 0;
}