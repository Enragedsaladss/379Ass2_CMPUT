#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int delay;
int nLine;

void alarm_handler(int signo) {
		
	int timed_out = 0;
	printf("*** Entering delay period of %d msec\n\n", delay);
  	
	char command[100];
  	printf("User command: ");

	fflush(stdout);

	fd_set readfds;
   	
	struct timeval timeout;
    	FD_ZERO(&readfds);
    	FD_SET(STDIN_FILENO, &readfds);
   	timeout.tv_sec = (int)delay/1000;
   	timeout.tv_usec = (delay - (timeout.tv_sec*1000))*1000;
	printf("Timing %ld, %ld", timeout.tv_sec, timeout.tv_usec);
    	int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
	printf("\n*** Delay period ended\n");
	
	if (ready == 0) {
		timed_out = 1;
	}
	if (timed_out) {
		return;
	}
	
	else {
		fgets(command, sizeof(command), stdin);
 		command[strcspn(command, "\n")] = '\0';
	}

	if (strcmp(command, "quit") == 0) {
    		printf("Exiting program ...\n");
    		exit(0);
  	}
 	
       	else {
   	 	FILE *fp = popen(command, "r");
    
		if (fp == NULL) {
      			perror("Error executing command");
    		}
    		
		else {
    			char buffer[256];
     
			while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        		
				printf("%s", buffer);
      			}
     			pclose(fp);
    		}
  	}
		
}

int main(int argc, char *argv[]) {
	
	if (argc != 4) {
    		fprintf(stderr, "Usage: %s nLine inputFile delay\n", argv[0]);
    		exit(1);
  	}	

	printf("a2p1 starts: (nLine= %s, inFile='%s', delay= %s)\n", argv[1], argv[2], argv[3]);
  	nLine = atoi(argv[1]);
  	delay = atof(argv[3]); 

  	FILE *file = fopen(argv[2], "r");
  	if (file == NULL) {
    		perror("Error opening inputFile");
    		exit(1);
  	}
  
	char buffer[256];
  	int linesPrinted = 1;

  	while (fgets(buffer, sizeof(buffer), file) != NULL) {
  		printf("[%04d]: %s", linesPrinted++, buffer);
    		nLine--;

    		if (nLine == 0) {
      			alarm_handler(SIGALRM);		
      			nLine = atoi(argv[1]);
    		}	
	}
	fclose(file);
    	return 0;
}
