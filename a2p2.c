#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define FIFO_SERVER_TO_CLIENT "fifo-0-1"
#define FIFO_CLIENT_TO_SERVER "fifo-1-0"
#define MAXWORD 32
#define MAX_CONTENT_LINES 3

typedef enum {
    PUT,
    GET,
    DELETE,
    GTIME,
    TIME,
    OK,
    ERROR
} PacketType;

typedef struct {
    PacketType type;
    char message[MAXWORD];
} Packet;

void server() {
        int server_to_client_fd, client_to_server_fd;
        char buffer[BUFSIZ];

        mkfifo("fifo-0-1", 0666);
        mkfifo("fifo-1-0", 0666);

        printf("Server Start.\n");
        server_to_client_fd = open(FIFO_SERVER_TO_CLIENT, O_WRONLY);
        client_to_server_fd = open(FIFO_CLIENT_TO_SERVER, O_RDONLY);

        while (1) {
                Packet packet;
                Packet response;
                read(server_to_client_fd, &packet, sizeof(Packet));

                if (packet.type = PUT) {
                        printf("Received from client: %s\n", packet.message);
                        response.type = OK;
                        write(server_to_client_fd, &response, sizeof(response));
                }
        }

        close(server_to_client_fd);
        close(client_to_server_fd);
}

void client(const char *input_file) {
        int server_to_client_fd, client_to_server_fd;
        char buffer[BUFSIZ];
        FILE *file;

        if (access(FIFO_SERVER_TO_CLIENT, F_OK) == -1) {
                printf("Server is not running.\n");
                exit(EXIT_FAILURE);
        }

        server_to_client_fd = open(FIFO_SERVER_TO_CLIENT, O_RDONLY);
        client_to_server_fd = open(FIFO_CLIENT_TO_SERVER, O_WRONLY);

        file = fopen(input_file, "r");
        if (!file) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
        }

        printf("Client started.\n");
        printf("HELLOWTF");
        while (fgets(buffer, BUFSIZ, file)) {
                printf("hello")
                if(buffer[0] == '#' || buffer[0] == '\n')
                        continue;

                int idNumber, delay_time;
                char action[MAXWORD], commandFile[MAXWORD];
                printf("Hello");
                sscanf(buffer, "%d %s %s", &idNumber, action, commandFile);
                printf("test %s", action);
                if (idNumber != 1)
                        continue;
                printf("test %s", action);
                if (strcmp(action, "put") == 0) {
                        printf("Hello\n");
                        Packet packet;
                        packet.type = PUT;
                        printf("Transmitted:(src = client:1) (%s) (%s)\n", action, commandFile);
                        strcpy(packet.message, commandFile);
                        write(client_to_server_fd, &packet, sizeof(Packet));
                        int i = 0;
                        int j = 0;
                        while(i != 2) {
                                printf("Hello1\n");
                                fgets(buffer, BUFSIZ, file);
                                if (buffer[0] == '{' || buffer[0] == '}')
                                        i++;
                                else    {
                                        printf("[%d]:%s", j, buffer);
                                        j++;
                                }

                        }
                }
                Packet receive;

                write(client_to_server_fd, buffer, strlen(buffer) + 1);
                read(server_to_client_fd, &receive, sizeof(receive));
                printf("Received from server: %s", receive.type);
    }

    fclose(file);
    close(server_to_client_fd);
    close(client_to_server_fd);
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        printf("Usage: %s [-s | -c input_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-s") == 0 && argc == 2) {
        server();
    } else if (strcmp(argv[1], "-c") == 0 && argc == 3) {
        client(argv[2]);
    } else {
        printf("Invalid arguments. Usage: %s [-s | -c input_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
