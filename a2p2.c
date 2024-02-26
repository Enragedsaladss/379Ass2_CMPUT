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
    ERROR,
    DELAY,
    QUIT
} PacketType;

typedef struct {
    PacketType type;
    char message[MAXWORD];
} Packet;

typedef struct {
	int putLoop;
	char arguement[MAXWORD];
} PutPacket;

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
		PutPacket putPacketServer;
                Packet response;
		read(client_to_server_fd, &packet, sizeof(Packet));
		if (packet.type == PUT) {;
                        printf("Received (src=client:1) (PUT) (%s)\n", packet.message);
			int i = 0;
			while(i < 3) {
				read(client_to_server_fd, &putPacketServer, sizeof(PutPacket));
				if(putPacketServer.putLoop < 0)
					break;
				printf("[%d]:%s", putPacketServer.putLoop, putPacketServer.arguement);
				i++;
			};
			response.type = OK;
                	write(server_to_client_fd, &response, sizeof(Packet));
                	printf("Transmitted (src= server) (OK)\n\n");
			
		}
		else if(packet.type  == GTIME) {
			printf("return time here\n");
			response.type = OK;
                	write(server_to_client_fd, &response, sizeof(Packet));
                	printf("Transmitted (src= server) (OK)\n\n");
		}
		else if(packet.type == DELAY) {
			printf("DELAY HERE\n");
			response.type = OK;
                	write(server_to_client_fd, &response, sizeof(Packet));
                	printf("Transmitted (src= server) (OK)\n\n");
		}
		else if(packet.type == GET) {
			printf("Received (src=client:1) (GET) (%s)\n", packet.message);
			response.type = OK;
                	write(server_to_client_fd, &response, sizeof(Packet));
                	printf("Transmitted (src= server) (OK)\n\n");
		}
		else if(packet.type == DELETE) {
                        printf("Received (src=client:1) (DELETE) (%s)\n", packet.message);
                        response.type = OK;
                        write(server_to_client_fd, &response, sizeof(Packet));
                        printf("Transmitted (src= server) (OK)\n\n");
                }
		else if (packet.type == QUIT) {
			printf("Exiting server\n");
			break;	
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
	
        
        while (fgets(buffer, BUFSIZ, file)) {
	       		
        	if(buffer[0] == '#' || buffer[0] == '\n')
                        continue;
		
                int idNumber, delay_time;
                char action[MAXWORD], commandFile[MAXWORD];
                Packet packet;
		
		sscanf(buffer, "%d %s %s", &idNumber, action, commandFile);
         
		if (idNumber != 1)
                        continue;

		if (strcmp(action, "put") == 0) {

                        packet.type = PUT;
                        printf("Transmitted:(src = client:1) (%s) (%s)\n", action, commandFile);
                        strcpy(packet.message, commandFile);

			write(client_to_server_fd, &packet, sizeof(Packet));
                        
			int i = 0;
                        int j = 0;
			
			PutPacket putPacket;

                        while(i != 2) {
                                fgets(buffer, BUFSIZ, file);
                                if (buffer[0] == '{' || buffer[0] == '}')
					i++;
                                else    {
					putPacket.putLoop = j;
                                        strcpy(putPacket.arguement,buffer);
					write(client_to_server_fd, &putPacket, sizeof(PutPacket));
                                        printf("[%d]:%s", j, buffer);
                                        j++;
                                }
                        }
			putPacket.putLoop = -1;
                	write(client_to_server_fd, &putPacket, sizeof(putPacket));
			printf("Server Exited %d\n", putPacket.putLoop);
		}
		else if(strcmp(action, "gtime") == 0) {
			packet.type = GTIME;
			write(client_to_server_fd, &packet, sizeof(Packet));
			printf("print the time here\n");
		}
		else if (strcmp(action, "delay") == 0) {
			packet.type = DELAY;
			strcpy(packet.message,commandFile);
			write(client_to_server_fd, &packet, sizeof(Packet));
			printf("delay\n");
		}
		else if(strcmp(action, "get")==0) {
			packet.type = GET;
			strcpy(packet.message,commandFile);
			write(client_to_server_fd, &packet, sizeof(Packet));
			printf("get\n");
		}
		else if(strcmp(action, "delete") == 0) {
			packet.type = DELETE;
                        strcpy(packet.message,commandFile);
                        write(client_to_server_fd, &packet, sizeof(Packet));
			printf("delete\n");
		}
		else if (strcmp(action, "quit") == 0) {
			packet.type = QUIT;
                        strcpy(packet.message,commandFile);
                        write(client_to_server_fd, &packet, sizeof(Packet));
                        printf("quit\n");	
		}
                
		Packet receive;
		write(client_to_server_fd, buffer, strlen(buffer) + 1);
                read(server_to_client_fd, &receive, sizeof(Packet));
                if(receive.type == OK) {	
			printf("Received from server: (OK)\n\n");
    
		}
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
