#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

int delay;
int nLine;

void alarm_handler(int signo) {
  prtinf("Entering delay period...\n");
  alarm(delay);

  char command[100];
  printf("Enter command: ");

  fgets(command, sizeof(command), stdin);

  command[strcspn(command, "\n")] = '\0' ;

  if (strcmp(command, "quit") == 0) {
    printf("Exiting program ...\n");
    exit(0);
  }
  else {
    FILE *fp = popen{command, "r"}
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

int maim(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s nLine inputFile delay\n", argv[0]);
    exit(1)
  }

  nLine = atoi(argv[1]);
  delay = atoi(argv[3]);

  struct sigaction sa;
  sa.sa_handler = alarm_handler;
  sa.sa_flags = SA_RESTART;
  sigaction(SIGALRM, &sa, NULL);

  alarm(delay);

  FILE *file = fopen(argv[2], "r");
  if (file == NULL) {
    perror("Error opening inputFile");
    exit(1)
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    printf("%s", buffer);
    nLine--;

    if (nLine == 0) {
      pause();
      nLine = atoi(argv[1]);
    }
    fclose(file);
    return 0;
}
