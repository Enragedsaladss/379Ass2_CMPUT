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
#define MAX_LINE_LENGTH 80
#define MAXWORD 32
#define MAX_CONTENT_LINES 4
#define MAXOBJECT 16

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
    	char message[MAX_LINE_LENGTH];
} Packet;

typedef struct {
	int iteration;
	char arguement[MAX_LINE_LENGTH];
} PutPacket;

typedef struct {
	PacketType type;
	double time;
}TimePacket;

typedef struct {
	int values;
	char lines[MAX_CONTENT_LINES][MAX_LINE_LENGTH];
} Object;

void status_send(int fd_Write, PacketType status, char message[MAX_LINE_LENGTH]) {
	Packet packet;
	packet.type = status;
	strcpy(packet.message, message);
	write(fd_Write, &packet, sizeof(Packet));
}

int server_find(int *object_Counter, Object *objects[MAXOBJECT], char message[MAXWORD]) {
        for(int i = 0; i <*object_Counter; i++) {
                if (strcmp(objects[i]->lines[0], message) == 0)
                        return i;
        }
        return -1;
}

void server_PUT(int fd_Write, int fd_Read, int *object_Counter, Object *objects[MAXOBJECT], char message[MAX_LINE_LENGTH]) {
  
	printf("Received (src=client:1) (PUT) (%s)\n", message);
 	
	strcpy(objects[*object_Counter]->lines[0], message);
	PutPacket put_Packet;
	PacketType status; 
	char status_Message[MAXWORD];	
	int i = 0;
	int found = server_find(object_Counter, objects, message);
	
	if (found < 0 ) {
		status = OK;
		strcpy(status_Message, "");
		status_send(fd_Write, status, status_Message);
  		
		while(1) {
			read(fd_Read, &put_Packet, sizeof(PutPacket));
    		
			if(put_Packet.iteration < 0) {
				break;
			}
			
    			printf("[%d]:%s", put_Packet.iteration, put_Packet.arguement);
			strcpy(objects[*object_Counter]->lines[i+1], put_Packet.arguement);
			i++; 
		};
		objects[*object_Counter]->values = i;
		*object_Counter += 1;

		status_send(fd_Write, status, status_Message);
		printf("Transmitted (src= server) (OK)\n\n");
	}
	else {
		status = ERROR;
                strcpy(status_Message,"object already exists");
                status_send(fd_Write, status, status_Message);
                printf("Transmitted (src= server) (Error: %s)\n\n", status_Message);	
	}
}

void server_GET(int fd_Write, int fd_Read, int *object_Counter, Object *objects[MAXOBJECT], char message[MAXWORD]) {
  	printf("Received (src=client:1) (GET) (%s)\n", message);
	
	PacketType status;
	PutPacket put_Packet;
	char status_Message[MAX_LINE_LENGTH];
	int found = server_find(object_Counter, objects, message);
	
	if(found >= 0) {
		status = OK;
		status_send(fd_Write, status, message);
		printf("Transmitted (src= server) (OK) (%s)\n", message);
		int k = 0;
		
		while(k < objects[found]->values) {
			put_Packet.iteration = k;
			strcpy(put_Packet.arguement, objects[found]->lines[k+1]);
			printf("[%d]:%s", put_Packet.iteration, put_Packet.arguement);
			write(fd_Write, &put_Packet, sizeof(put_Packet));	
			k++;
		}
		put_Packet.iteration = -1;
		write(fd_Write, &put_Packet, sizeof(put_Packet));
		printf("\n");
	}
	else {
		status = ERROR;
		strcpy(status_Message,"object not found");
		status_send(fd_Write, status, status_Message);
		printf("Transmitted (src= server) (Error: %s)\n\n", status_Message);
	}

}

void server_DELETE(int fd_Write, int *object_Counter, Object *objects[MAXOBJECT], char message[MAXWORD]) {
 	printf("Received (src=client:1) (DELETE) (%s)\n", message);
	
	PacketType status;
	char status_Message[MAX_LINE_LENGTH];
	int found = server_find(object_Counter, objects, message);
	
	if (found >=0) {
		status = OK;
		strcpy(status_Message, "");
		for (int i = found; i < *object_Counter; i++) {
			objects[i] = objects[i+1];
		}
		status_send(fd_Write, status, status_Message);
		*object_Counter--;
		printf("Transmitted (src= server) (OK)\n\n");
	}

	else {
		status == ERROR;
		strcpy(status_Message, "object not found");
		status_send(fd_Write, status, status_Message);
		printf("Transmitted (src = server) (ERROR: %s\n", status_Message);
	}
}

void server_GTIME(int fd_Write, struct timespec *start, char message[MAXWORD]) {
	printf("Received (src=client:1) (GTIME)\n");
	
	struct timespec end;
	TimePacket time_Packet;
	time_Packet.type = TIME;
	
	clock_gettime(CLOCK_REALTIME, &end);
	time_Packet.time = (end.tv_sec - start->tv_sec) + (end.tv_nsec - start->tv_nsec) / 1e9;
	write(fd_Write, &time_Packet, sizeof(TimePacket));
	printf("Transmitted (src= server) (TIME: %.2f)\n\n", time_Packet.time);
	

}

void server() {
	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);	
        int server_to_client_fd, client_to_server_fd;
	

	int object_Counter = 0;
        char buffer[BUFSIZ];
	
	Object *objects[MAXOBJECT];
	for (int i = 0; i < 16; i++) {
        	objects[i] = (Object *)malloc(sizeof(Object));
	}
		
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
              			server_PUT(server_to_client_fd, client_to_server_fd, &object_Counter, objects, packet.message);
              			break;
            		case GET:
              			server_GET(server_to_client_fd, client_to_server_fd,&object_Counter,  objects, packet.message);
             			break;
            		case DELETE:
              			server_DELETE(server_to_client_fd, &object_Counter, objects, packet.message);
              			break;
            		case GTIME:
              			server_GTIME(server_to_client_fd, &start,  packet.message);
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
	for (int j = 0; j < 16; j++) {
                free(objects[j]);
            }
        close(server_to_client_fd);
        close(client_to_server_fd);
}

void client_PUT(int fd_Write, int fd_Read, char message[MAXWORD], FILE *file) {
  	Packet packet;
	packet.type = PUT;
 	char buffer[BUFSIZ];
	
	printf("Transmitted:(src = client:1) (PUT) (%s)\n", message);
  	strcpy(packet.message, message);

	write(fd_Write, &packet, sizeof(Packet));
                        
	int i = 0;
 	int j = 0;
			
	PutPacket put_Packet;
	read(fd_Read, &packet, sizeof(packet));

	if (packet.type == OK) {
  		while(i != 2) {
    			fgets(buffer, BUFSIZ, file);
    			if (buffer[0] == '{' || buffer[0] == '}')
				i++;
			else {
				put_Packet.iteration = j;

      				strcpy(put_Packet.arguement,buffer);
				write(fd_Write, &put_Packet, sizeof(PutPacket));
      			
				printf("[%d]:%s", j, buffer);
      			
				j++;
    			}		
  		}
	put_Packet.iteration = -1;
  	write(fd_Write, &put_Packet, sizeof(PutPacket));
	read(fd_Read, &packet, sizeof(Packet));
		if (packet.type == OK) 
			printf("Received (src= server) (OK)\n\n");
		else {
			printf("Received (src= server) (ERROR: Unknown)");
		}
	}
	
	else {
		printf("Received (src= server) (ERROR: %s)\n\n", packet.message);
	}
}

void client_GET(int fd_Write, int fd_Read, char message[MAXWORD]) {
  	Packet packet;
	packet.type = GET;
	strcpy(packet.message, message);
	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:1) (GET) (%s)\n", message);
	PutPacket put_Packet;
  	
	int i = 0;
	read(fd_Read, &packet, sizeof(packet));
	if(packet.type == OK) {
		printf("Received (src= server) (OK)\n");
  		while(1) {
			read(fd_Read, &put_Packet, sizeof(PutPacket));

			if(put_Packet.iteration < 0)
				
				break;
    			
			printf("[%d]:%s", put_Packet.iteration, put_Packet.arguement); 
		};
		printf("\n");
	}
	else if (packet.type == ERROR) {
		printf("Received (src= server) (ERROR: %s)\n\n", packet.message);
	}
	else {
		printf("Unknown\n\n");
	}
}

void client_DELETE(int fd_Write, int fd_Read, char message[MAXWORD]) {
  	Packet packet;
	packet.type = DELETE;
  	strcpy(packet.message,message);
	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src= client:1) (DELETE) (%s)\n", message);	
	read(fd_Read, &packet, sizeof(Packet));
	
	if (packet.type == OK) {
		printf("Received: (src= server) (OK)\n\n");
	}
	
	else {
		printf("Receieved (src= server) (ERROR: %s)\n\n", packet.message);
	}
}

void client_GTIME(int fd_Write, int fd_Read, char message[MAXWORD]) {
 	Packet packet;
	TimePacket time_Packet;
	packet.type = GTIME;
  	strcpy(packet.message,message);
	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src= client:1) (GTIME)\n");
	read(fd_Read, &time_Packet, sizeof(TimePacket));
	printf("Received: (src= server)(TIME: %.2f)\n\n", time_Packet.time);
}

void client_DELAY(char message[MAXWORD]) {

	printf("*** Entering a delay period of  %s msec\n", message);
	float delay = atof(message)*1000.0f;
	usleep(delay);
	printf("*** Exiting Delay period\n\n");
}

void client(const char *input_file) {
 	printf("Client Started\n");
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
      		char action[MAXWORD], message[MAXWORD];
      		Packet packet;
		
		sscanf(buffer, "%d %s %s", &idNumber, action, message);
         
		if (idNumber != 1)
        		continue;
		  
		if (strcmp(action, "put") == 0) {
        		client_PUT(client_to_server_fd, server_to_client_fd, message, file);
      		}
      		
		else if(strcmp(action, "get") == 0) {
			client_GET(client_to_server_fd, server_to_client_fd, message);
		}
		  
		else if (strcmp(action, "delete") == 0) {
			client_DELETE(client_to_server_fd, server_to_client_fd, message);
		}

		else if(strcmp(action, "gtime")==0) {
			client_GTIME(client_to_server_fd, server_to_client_fd, message);
		}

		else if(strcmp(action, "delay") == 0) {
			client_DELAY(message);
      		} 
		else if (strcmp(action, "quit") == 0) {
			packet.type = QUIT;
			strcpy(packet.message,"\n");
			write(client_to_server_fd, &packet, sizeof(Packet));
		}
		else {
			printf("Unknown command");
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
