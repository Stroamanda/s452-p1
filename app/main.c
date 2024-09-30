#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>

void removeAmster(char *line);
void trimEnd(char *line);

int main(int argc, char *argv[]) {
  int opt;
  int id = 1;
  // char cwd[1024];
  char *prompt;
  bool background = false;
  bool doesntExist = false;

  // create process for terminal
  struct shell theShell;
  sh_init(&theShell);
  theShell.mapCount = 0;
  theShell.endJobID = 1;
  
  // set prompt
  // prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt("MY_PROMPT"));
  prompt = get_prompt("MY_PROMPT");

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
  using_history();

  while ((line=readline(prompt)) != NULL) {
    trim_white(line);
    char keepAm[strlen(line) + 1];
    strcpy(keepAm, line);
        
    if (strcmp(line, "") != 0 && line[strlen(line) - 1] == '&') {
      removeAmster(line);
      trimEnd(line);
      background = true;
    } else {
      background = false;
    }

    // parse command into values into an array
    char **argShell = cmd_parse(line);
    bool tracker = false;
    if (strcmp(line, "") == 0) {
        tracker = true;
        int status;
        for (int i = 0; i < theShell.mapCount; i++) {
            if (theShell.backgroundArray[i].pid != 0) {
              pid_t result = waitpid(theShell.backgroundArray[i].pid, &status, WNOHANG);
              if (result < 0) {
              printf("[%d] Done %s \n", theShell.backgroundArray[i].id, theShell.backgroundArray[i].command);
              theShell.backgroundArray[i].pid = 0;
              }
            }
        }
    } else {
        tracker = do_builtin(&theShell, argShell);
    }

    // if you want directory to display
    //prompt = strcat(strcat(getcwd(cwd, sizeof(cwd)), " "), get_prompt("MY_PROMPT"));

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
      int status;
      if (tracker == false) {
        status = execvp(argShell[0], argShell);

        // if the command is also not in the execvp, tell the user the command doesn't exist
        if (status == -1) {
          printf("That Command Doesn't Exist\n");
          _exit(42);
      }
      }
      exit(0);
    } else { // once the child process finishes
        int status;

        // only wait to finish if not a background process
      if (!background) {
            waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 42) {
              doesntExist = true;
        }

          // give back control to parent
        tcsetpgrp (theShell.shell_terminal, theShell.shell_pgid);
      }

      if (!doesntExist && background) {
            printf("[%d] %d %s\n", id, pid, keepAm);
            theShell.backgroundArray[theShell.mapCount].pid = pid;
            strcpy(theShell.backgroundArray[theShell.mapCount].command, keepAm);
            theShell.backgroundArray[theShell.mapCount].id = id;
            theShell.backgroundArray[theShell.mapCount].reportedDone = false;
            theShell.backgroundArray[theShell.mapCount].reportedRunning = false;
            theShell.backgroundArray[theShell.mapCount].jobID = theShell.endJobID++;
            theShell.mapCount++;
            id++;
      }

      doesntExist = false;

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
    cmd_free(argShell);
  }


  return 0;
}

// Removes & from the end so command can be processed
void removeAmster(char * line) {
  for (int i = (int)strlen(line); i > 0; i--) {
      if (line[i] == '&') {
        line[i] = ' ';
        break;
      }
  }
}

// trims any trailing whitespace
void trimEnd(char *line) {
  int len = strlen(line);
    while (len > 0 && line[len - 1] == ' ') {
      len--;
    }
    line[len] = '\0';
}
