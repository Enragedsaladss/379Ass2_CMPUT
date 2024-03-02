#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 80
#define MAXWORD 32
#define MAX_CONTENT_LINES 4
#define MAXOBJECT 16
#define MAXCLIENTS 3
#define FIFOSIZE "fifo-0-0"

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
	int client_id;
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
	int client_id;
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

void server_PUT(int fd_Write, int fd_Read, int *object_Counter, Object *objects[MAXOBJECT], Packet packet) {
  	
	printf("Received (src=client:%d) (PUT) (%s)\n", packet.client_id, packet.message);
 	
	strcpy(objects[*object_Counter]->lines[0], packet.message);
	objects[*object_Counter]->client_id = packet.client_id;
	PutPacket put_Packet;
	PacketType status; 
	char status_Message[MAXWORD];	
	int i = 0;
	int found = server_find(object_Counter, objects, packet.message);
	
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

void server_GET(int fd_Write, int fd_Read, int *object_Counter, Object *objects[MAXOBJECT], Packet packet) {

	printf("Received (src=client:%d) (GET) (%s)\n",packet.client_id, packet.message);
	
	PacketType status;
	PutPacket put_Packet;
	char status_Message[MAX_LINE_LENGTH];
	int found = server_find(object_Counter, objects, packet.message);
	
	if(found >= 0) {
		status = OK;
		status_send(fd_Write, status, packet.message);
		printf("Transmitted (src= server) (OK) (%s)\n", packet.message);
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

void server_DELETE(int fd_Write, int *object_Counter, Object *objects[MAXOBJECT], Packet packet) {
	
 	printf("Received (src=client:%d) (DELETE) (%s)\n", packet.client_id, packet.message);
	
	PacketType status;
	char status_Message[MAX_LINE_LENGTH];
	int found = server_find(object_Counter, objects, packet.message);
	
	if (found >=0) {
		status = OK;
		strcpy(status_Message, "");
		for (int i = found; i < *object_Counter; i++) {
			*objects[i] = *objects[i+1];
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

void server_GTIME(int fd_Write, struct timespec *start, Packet packet) {
	
	printf("Received (src=client:%d) (GTIME)\n", packet.client_id);
	
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
	int server_to_client_fds[MAXCLIENTS], client_to_server_fds[MAXCLIENTS];
	char server_to_client_fd[10], client_to_server_fd[10];
	int object_Counter = 0, max_fd = 0;
        char buffer[BUFSIZ];
	struct timeval timeout;
	Object *objects[MAXOBJECT];
	fd_set readfds;

	FD_ZERO(&readfds);
	
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	for (int i = 0; i < 16; i++) {
        	objects[i] = (Object *)malloc(sizeof(Object));
	}
	for (int i = 0; i < MAXCLIENTS; i++) {
		printf("test1\n");
		snprintf(server_to_client_fd, sizeof(FIFOSIZE), "fifo-0-%d", i+1);
		snprintf(client_to_server_fd, sizeof(FIFOSIZE), "fifo-%d-0", i+1);	
		mkfifo(server_to_client_fd, 0666);
		mkfifo(client_to_server_fd, 0666);
		server_to_client_fds[i] = open(server_to_client_fd, O_WRONLY);
        	client_to_server_fds[i] = open(client_to_server_fd, O_RDONLY);
		FD_SET(client_to_server_fds[i], &readfds);
		printf("test2\n");
		if (client_to_server_fds[i] > max_fd) {
            		max_fd = client_to_server_fds[i];
        	}		
	}
	

	printf("Server Start.\n");
	
	while (1) {
               	Packet packet;  

		int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
		if (activity = -1) {
			break;
		}

		for(int i = 0; i<MAXCLIENTS; i++) {
			if (FD_ISSET(client_to_server_fds[i], &readfds)) {
           			read(client_to_server_fds[i], &packet, sizeof(Packet));

				switch (packet.type) {
            				case PUT:
              					server_PUT(server_to_client_fds[i], client_to_server_fds[i], &object_Counter, objects, packet);
              					break;
            				case GET:
              					server_GET(server_to_client_fds[i], client_to_server_fds[i], &object_Counter, objects, packet);
             					break;
            				case DELETE:
              					server_DELETE(server_to_client_fds[i], &object_Counter, objects, packet);
              					break;
            				case GTIME:
              					server_GTIME(server_to_client_fds[i], &start, packet);
              					break;
            				case QUIT:
              					printf("Exiting\n");
              					break;
            				default:
             					printf("Unknown");
              					break;
				}
			}	
		}
	}

	for (int j = 0; j < 16; j++) {
                free(objects[j]);
	}
	for (int i = 0; i < MAXCLIENTS; i++) {
                close(server_to_client_fds[i]);
                close(client_to_server_fds[i]);
        }
}

void client_PUT(int fd_Write, int fd_Read, Packet packet, FILE *file) {
	packet.type = PUT;
 	char buffer[BUFSIZ];
	
	printf("Transmitted:(src = client:%d) (PUT) (%s)\n",packet.client_id, packet.message);

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

void client_GET(int fd_Write, int fd_Read, Packet packet) {
  
	packet.type = GET;
	
	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src = client:%d) (GET) (%s)\n", packet.client_id, packet.message);
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

void client_DELETE(int fd_Write, int fd_Read, Packet packet) {

	packet.type = DELETE;

	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src= client:%d) (DELETE) (%s)\n", packet.client_id, packet.message);	
	read(fd_Read, &packet, sizeof(Packet));
	
	if (packet.type == OK) {
		printf("Received: (src= server) (OK)\n\n");
	}
	
	else {
		printf("Receieved (src= server) (ERROR: %s)\n\n", packet.message);
	}
}

void client_GTIME(int fd_Write, int fd_Read, Packet packet) {
 
	TimePacket time_Packet;
	packet.type = GTIME;
  	write(fd_Write, &packet, sizeof(Packet));
	printf("Transmitted:(src= client:%d) (GTIME)\n", packet.client_id);
	read(fd_Read, &time_Packet, sizeof(TimePacket));
	printf("Received: (src= server)(TIME: %.2f)\n\n", time_Packet.time);
}

void client_DELAY(char message[MAXWORD]) {

	printf("*** Entering a delay period of  %s msec\n", message);
	float delay = atof(message)*1000.0f;
	usleep(delay);
	printf("*** Exiting Delay period\n\n");
}

void client(int client_id, const char *input_file) {
 	printf("Client Started\n");
	char server_to_client_name[10], client_to_server_name[10];
	int server_to_client_fd, client_to_server_fd;
  	char buffer[BUFSIZ];
  	FILE *file;

	snprintf(server_to_client_name, sizeof(FIFOSIZE), "fifo-0-%d", client_id);
	snprintf(client_to_server_name, sizeof(FIFOSIZE), "fifo-%d-0", client_id);
	
	server_to_client_fd = open(server_to_client_name, O_RDONLY);
        client_to_server_fd = open(client_to_server_name, O_WRONLY);

  	if (access(server_to_client_name, F_OK) == -1) {
    		printf("Server is not running.\n");
    		exit(EXIT_FAILURE);
    	}

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
         	packet.client_id = idNumber;
		strcpy(packet.message, message);
		if (idNumber != client_id)
        		continue;
		  
		if (strcmp(action, "put") == 0) {
        		client_PUT(client_to_server_fd, server_to_client_fd, packet, file);
      		}
      		
		else if(strcmp(action, "get") == 0) {
			client_GET(client_to_server_fd, server_to_client_fd, packet);
		}
		  
		else if (strcmp(action, "delete") == 0) {
			client_DELETE(client_to_server_fd, server_to_client_fd, packet);
		}

		else if(strcmp(action, "gtime")==0) {
			client_GTIME(client_to_server_fd, server_to_client_fd, packet);
		}

		else if(strcmp(action, "delay") == 0) {
			client_DELAY(message);
      		} 

		else if (strcmp(action, "quit") == 0) {
			
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
    if (argc > 4) {
        printf("Usage: %s [-s | -c idnumber input_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-s") == 0 && argc == 2) {
	server();
    } else if (strcmp(argv[1], "-c") == 0 && argc == 4 && isdigit(*argv[2]) == 0){
        client((int) *argv[2], argv[3]);
    } else {
        printf("Invalid arguments. Usage: %s [-s | -c idnumber input_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
