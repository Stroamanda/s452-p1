#include <stdio.h>
#include <signal.h>
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
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

struct Map {
  int id;
  int pid;
  char command[1024];
};

void removeAmster(char *line);
void trimEnd(char *line);

int main(int argc, char *argv[]) {
  int opt;
  int id = 1;
  int index = 0;
  char cwd[1024];
  char *prompt;
  struct Map backgroundArray[1024];
  bool background = false;

  // create process for terminal
  struct shell theShell;
  sh_init(&theShell);
  
  // set prompt
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
  int isBackground = false;
  using_history();

  while ((line=readline(prompt)) != NULL) {
    prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt(NULL));
    bool tracker = false;
    char *trimLine = trim_white(line);
    char keepAm[strlen(line) + 1];
    strcpy(keepAm, line);
        
    if (trimLine[strlen(trimLine) - 1] == '&') {
      removeAmster(line);
      trimEnd(line);
      background = true;
    } else {
      background = false;
    }

    // Checks for exit command
    if (strcmp(line, "exit") == 0) {
      free(line);
      exit(0);

      // checks for cd command
    } else if (strncmp(line, "cd ", strlen(changeDir)) == 0 || strcmp(line, "cd") == 0) {
      tracker = true;
      change_dir(&line);

      // checks for history command, then lists all the commands that have been written
    } else if (strcmp(line, "history") == 0) {
      tracker = true;
      for (int i = 0; i < history_length; i++) {
          printf("%s\n", history_list()[i]->line);
      }

    } else if (strcmp(line, "") == 0) {
        tracker = true;
        int status;
        for (int i = 0; i < index; i++) {
            if (backgroundArray[i].pid != 0) {
               pid_t result = waitpid(backgroundArray[i].pid, &status, WNOHANG);
               if (WIFEXITED(status)) {
                  printf("[%d] Done %s \n", backgroundArray[i].id, backgroundArray[i].command);
                  backgroundArray[i].pid = 0;
                  // free(backgroundArray[i].command);
               }
            }
        }
        index = 0;
    }
    // parse command into values into an array
    char **argShell = cmd_parse(line);
    int pid = fork();
    
    if (pid == 0) {
      pid_t child = getpid();
      setpgid(child, child);

      if (background) {
          if (kill(-child, SIGCONT) < 0) {
              perror("kill (SIGCONT)");
          }
      } else {
          // set to child
          tcsetpgrp(theShell.shell_terminal, child);
      }

      // enable signals
      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      // if it's not one of the above commands, use execvp
      int status = execvp(argShell[0], argShell);
      // if the command is also not in the execvp, tell the user the command doesn't exist
      if (status == -1 && tracker == false) {
          printf("That Command Doesn't Exist\n");
      }
      exit(0);
    } else { // once the child process finishes

      if (background) {
        printf("[%d] %d %s\n", id, pid, keepAm);
        backgroundArray[index].pid = pid;
        strcpy(backgroundArray[index].command, keepAm);
        backgroundArray[index].id = id;
        index++;
        id++;
      }

      // only wait to finish if not a background process
      if (!background) {
          waitpid(pid, NULL, 0);

          // give back control to parent
          tcsetpgrp (theShell.shell_terminal, theShell.shell_pgid);
      }

      // disable the signals again
      signal (SIGINT, SIG_IGN);
      signal (SIGQUIT, SIG_IGN);
      signal (SIGTSTP, SIG_IGN);
      signal (SIGTTIN, SIG_IGN);
      signal (SIGTTOU, SIG_IGN);
      signal (SIGCHLD, SIG_IGN);
    }

    add_history(line);
    free(line);
    free(trimLine);
    cmd_free(argShell);
  }


  return 0;
}

void removeAmster(char * line) {
  for (int i = strlen(line); i > 0; i--) {
      if (line[i] == '&') {
        line[i] = ' ';
        break;
      }
  }
}

void trimEnd(char *line) {
  int len = strlen(line);
    while (len > 0 && line[len - 1] == ' ') {
      len--;
    }
    line[len] = '\0'; // Null-terminate the string
}
