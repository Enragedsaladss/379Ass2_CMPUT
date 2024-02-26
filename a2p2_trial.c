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

void Status_Send(int fd_write, PacketType status, char message[MAXWORD]) {
	Packet packet;
	packet.type = status;
	strcpy(packet.message, message);
	write(fd_write, &packet, sizeof(Packet));
	printf("Transmitted (src= server) (OK)\n\n");
}

void command_PUT(int fd_Write, int fd_Read, char message[MAXWORD]) {
  
	printf("Received (src=client:1) (PUT) (%s)\n", message);
  
	PutPacket putPacket;
  	int i = 0;

  	while(1) {
		read(fd_Read, &putPacket, sizeof(PutPacket));
    		
		if(putPacket.putLoop < 0)
			break;
		
    		printf("[%d]:%s", putPacket.putLoop, putPacket.arguement);
    		i++; 
	};
	PacketType status = OK;
	char status_Message[MAXWORD] = "Placement";
	Status_Send(fd_Write, status, status_Message);

}


void command_GET(int fd_Write, int fd_Read, char message[MAXWORD]) {
  	printf("Received (src=client:1) (GET) (%s)\n", message);
	PacketType status = OK;
        char status_Message[MAXWORD] = "Placement";
        Status_Send(fd_Write, status, status_Message);

}


void command_DELETE(int fd_Write, int fd_Read, char message[MAXWORD]) {
 	printf("Received (src=client:1) (DELETE) (%s)\n", message);
	PacketType status = OK;
        char status_Message[MAXWORD] = "Placement";
        Status_Send(fd_Write, status, status_Message);

}


void command_GTIME(int fd_Write, int fd_Read, char message[MAXWORD]) {
  	printf("return time here\n");
	PacketType status = OK;
        char status_Message[MAXWORD] = "Placement";
        Status_Send(fd_Write, status, status_Message);

}


void command_DELAY(int fd_Write, int fd_Read, char message[MAXWORD]) {
  	printf("Received (src=client:1) (DELAY) (%s)\n", message);
	PacketType status = OK;
        char status_Message[MAXWORD] = "Placement";
        Status_Send(fd_Write, status, status_Message);

}

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
		read(client_to_server_fd, &packet, sizeof(Packet));

		switch (packet.type) {
            		case PUT:
              			command_PUT(server_to_client_fd, client_to_server_fd, packet.message);
              			break;
            		case GET:
              			command_GET(server_to_client_fd, client_to_server_fd, packet.message);
             			break;
            		case DELETE:
              			command_DELETE(server_to_client_fd, client_to_server_fd, packet.message);
              			break;
            		case GTIME:
              			command_GTIME(server_to_client_fd, client_to_server_fd, packet.message);
              			break;
            		case DELAY:
              			command_DELAY(server_to_client_fd, client_to_server_fd, packet.message);
              			break;
            		case QUIT:
              			printf("Exiting\n");
              			break;
            		default:
             			printf("Unknown");
              			break;
          	}
          	if (packet.type == QUIT) break;
      		
	}
	
        close(server_to_client_fd);
        close(client_to_server_fd);
}

void client_PUT(int fd_write, int fd_read, char message[MAXWORD], FILE *file) {
  	Packet packet;
	packet.type = PUT;
 	char buffer[BUFSIZ];
	
	printf("Transmitted:(src = client:1) (PUT) (%s)\n", message);
  	strcpy(packet.message, message);

	write(fd_write, &packet, sizeof(Packet));
                        
	int i = 0;
 	int j = 0;
			
	PutPacket putPacket;

  	while(i != 2) {
    		fgets(buffer, BUFSIZ, file);
    		if (buffer[0] == '{' || buffer[0] == '}')
			i++;
		else {
			putPacket.putLoop = j;
      			strcpy(putPacket.arguement,buffer);
			write(fd_write, &putPacket, sizeof(PutPacket));
      			printf("[%d]:%s", j, buffer);
      			j++;
    		}	
  	}
	putPacket.putLoop = -1;
  	write(fd_write, &putPacket, sizeof(putPacket));
}

void client_GET(int fd_write, int fd_read, char message[MAXWORD]) {
  	Packet packet;
	packet.type = GET;
	strcpy(packet.message, message);
	write(fd_write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:1) (GET (%s)\n", message);
}


void client_DELETE(int fd_write, int fd_read, char message[MAXWORD]) {
  	Packet packet;
	packet.type = DELETE;
  	strcpy(packet.message,message);
	write(fd_write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:1) (DELETE) (%s)\n", message);
}


void client_GTIME(int fd_write, int fd_read, char message[MAXWORD]) {
 	Packet packet;
	packet.type = GTIME;
  	strcpy(packet.message,message);
	write(fd_write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:1) (GTIME) (%s)\n", message);
}


void client_DELAY(int fd_write, int fd_read, char message[MAXWORD]) {
  	Packet packet;
	packet.type = DELAY;
	strcpy(packet.message,message);
	write(fd_write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:1) (DELAY) (%s)\n", message);
}

void Status_Check(int fd_read){
  	Packet packetStatus;
  	read(fd_read, &packetStatus, sizeof(Packet));
  	if(packetStatus.type == OK) {
		printf("Received from server: (OK)\n\n");
	}
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
        		client_PUT(client_to_server_fd, server_to_client_fd, commandFile, file);
      		}
      		
		else if(strcmp(action, "get") == 0) {
			client_GET(client_to_server_fd, server_to_client_fd, commandFile);
		}
		  
		else if (strcmp(action, "delete") == 0) {
			client_DELETE(client_to_server_fd, server_to_client_fd,commandFile);
		}

		else if(strcmp(action, "gtime")==0) {
			client_GTIME(client_to_server_fd, server_to_client_fd, commandFile);
		}

		else if(strcmp(action, "delay") == 0) {
			client_DELAY(client_to_server_fd, server_to_client_fd, commandFile);
      		} 
		else if (strcmp(action, "quit") == 0) {
			packet.type = QUIT;
			strcpy(packet.message,"\n");
			write(client_to_server_fd, &packet, sizeof(Packet));
		    	printf("Something to do with quit");
		  }
    	Status_Check(server_to_client_fd);
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