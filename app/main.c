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
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  int opt;
  char cwd[1024];
  char *prompt;
  
  prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt(NULL));

  // checks for extra arguments
  while((opt = getopt(argc, argv, "v")) != -1) {
    switch(opt) {
      case 'v':
        printf("Version %d\n", lab_VERSION_MAJOR);
        exit(0);
        break;
      default:
        exit(0);
        break;
    }
  }

  char *line;
  const char *changeDir = "cd ";
  using_history();

  // 
  while ((line=readline(prompt)) != NULL) {
    char *argShell[_SC_ARG_MAX];
    char *line2 = line;
    bool tracker = false;

    // turns line into an array of values separated by spaces
    char *val = strtok(line2, " ");
    int curr = 0;
    while (val != NULL) {
      argShell[curr++] = val;
      val = strtok(NULL, " ");
    }
    argShell[curr] = NULL;

    // checks to make sure the process is good to go
 
    if (strcmp(line, "exit") == 0) {
      free(line);
      exit(0);
    } else if (strncmp(line, "cd ", strlen(changeDir)) == 0 || strcmp(line, "cd") == 0) {
      tracker = true;
      change_dir(&line);

      // checks for history command, then lists all the commands that have been written
    } else if (strcmp(line, "history") == 0) {
      tracker = true;
      for (int i = 0; i < history_length; i++) {
          printf("%s\n", history_list()[i]->line);
      }

    }

    int pid = fork();
    
    if (pid == 0) {
      // if it's not one of the above commands, use execvp
      int status = execvp(argShell[0], argShell);

      // if the command is not in the execvp, tell the user the command doesn't exist
      if (status == -1 && tracker == false) {
          printf("That Command Doesn't Exist\n");
      }
    } else { // once the process finishes
      waitpid(pid, NULL, 0);
    }

    // set prompt to include the current directory
    prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt(NULL));

    add_history(line);
    free(line);
  }


  return 0;
}
