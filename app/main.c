#include <stdio.h>
#include <stdlib.h>
#include "../src/lab.h"
#include "../src/lab.c"
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <pwd.h>
#include <string.h>

int main(int argc, char *argv[]) {
  int opt;
  char cwd[1024];
  char *prompt;
  
  prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt(NULL));

  while((opt = getopt(argc, argv, "v")) != -1) {
    switch(opt) {
      case 'v':
        printf("Version %d\n", lab_VERSION_MAJOR);
        exit(0);
        break;
    }
  }

  char *line;
  const char *changeDir = "cd ";
  using_history();

  while ((line=readline(prompt)) != NULL) {
    printf("%s\n",line);
    if (strcmp(line, "exit") == 0) {
      free(line);
      exit(0);
    } else if (strncmp(line, "cd ", strlen(changeDir)) == 0 || strcmp(line, "cd") == 0) {
      change_dir(&line);
    } else if (strcmp(line, "history") == 0) {
      for (int i = 0; i < history_length; i++) {
          printf("%s\n", history_list()[i]->line);
      }
    } else {
       printf("Unknown Command\n");
    }

    prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt(NULL));

    add_history(line);
    free(line);
  }


  return 0;
}
