#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <signal.h>
#include "../src/lab.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

  struct shell theShell;

  char *get_prompt(const char *env) {
    char * prompt;
    if (env != NULL) {
      prompt = getenv(env);
    } 
    
    if (prompt == NULL) {
        prompt = "shell>";
    }

      return strdup(prompt);
  }

  int change_dir(char **dir) {
    char *direct = "";

    // if there is only "cd"
     if (dir[1] == NULL) {
      // if env doesn't know what home is
      if (getenv("HOME") == NULL) {
        // grab home directory of user
        direct = getpwuid(getuid())->pw_dir;
        chdir(direct);
      } else {
        // grab home directory if known
        direct = getenv("HOME");
        chdir(direct);
      }
      // if there is a directory input with cd
     } else {
        // updates directory if it exists but also checks if it's not a directory
        if (chdir(dir[1]) != 0) {
          printf("Couldn't Find Directory\n");
        } 

     }
    return 0;
  }

  char **cmd_parse(char const *line) {
    int argMax = sysconf(_SC_ARG_MAX);

    char **theArray = malloc (sizeof (char*) * argMax);
    char *line2 = strdup(line);

    char *val = strtok(line2, " ");
    int curr = 0;

    // turns line into an array of values separated by spaces
    while (val != NULL) {
      theArray[curr++] = strdup(val);
      val = strtok(NULL, " ");
    }
    theArray[curr] = NULL;
    free(line2);

    return theArray;
  }

  void cmd_free(char ** line) {
    if (line != NULL) {
      for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);
      }
      free(line);
    }
  }

  char *trim_white(char *line) {
    int len = strlen(line);
      while (len > 0 && line[len - 1] == ' ') {
        len--;
      }
      line[len] = '\0';

    char *start = line;
    while (*start != '\0' && isspace((unsigned char)*start)) {
      start++;
    }

    if (start != line) {
        memmove(line, start, len - (start - line) + 1);
    }
    return line;
  }

  bool do_builtin(struct shell *sh, char **argv) {
    char *command = argv[0];
    bool tracker = false;

    if (strcmp(command, "exit") == 0) {
      exit(0);

      // checks for cd command
    } else if (strcmp(command, "cd") == 0) {
      tracker = true;
      change_dir(argv);

      // checks for history command, then lists all the commands that have been written
    } else if (strcmp(command, "history") == 0) {
      tracker = true;
      for (int i = 0; i < history_length; i++) {
          printf("%s\n", history_list()[i]->line);
      }
    // Checks for enter command, then lists all recently completed jobs since the last time it was entered
    } else if (strcmp(command, "jobs") == 0) {
        tracker = true;
        int status;
        for (int i = 0; i < sh->mapCount; i++) {
            if (sh->backgroundArray[i].pid != 0) {
               pid_t result = waitpid(sh->backgroundArray[i].pid, &status, WNOHANG);
               if (result == 0 && sh->backgroundArray[i].reportedRunning == false) {
                  printf("[%d] %d Running %s \n", sh->backgroundArray[i].jobID, sh->backgroundArray[i].pid, sh->backgroundArray[i].command);
                  sh->backgroundArray[i].reportedRunning = true;
               } else if (result < 0 && sh->backgroundArray[i].reportedDone == false) {
                  printf("[%d] Done %s \n", sh->backgroundArray[i].jobID, sh->backgroundArray[i].command);
                  sh->backgroundArray[i].reportedDone = true;
               }
            }
        }

        bool finishedAllJobs = true;
        for (int i = 0; i < sh->mapCount; i++) {
            if (sh->backgroundArray[i].reportedDone == false) {
                finishedAllJobs = false;
                break;
            } else {
              finishedAllJobs = true;
            }
        }

        if (finishedAllJobs == true) {
            sh->endJobID = 1;
        }

    }

    return tracker;
  }

  void sh_init(struct shell *sh) {
    // code from the manual: https://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html
    *sh = theShell;
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty (sh->shell_terminal);

    if (sh->shell_is_interactive) {
      /* Loop until we are in the foreground.  */
      while (tcgetpgrp (sh->shell_terminal) != (sh->shell_pgid = getpgrp ()))
        kill (- sh->shell_pgid, SIGTTIN);

      /* Ignore interactive and job-control signals.  */
      signal (SIGINT, SIG_IGN);
      signal (SIGQUIT, SIG_IGN);
      signal (SIGTSTP, SIG_IGN);
      signal (SIGTTIN, SIG_IGN);
      signal (SIGTTOU, SIG_IGN);
      signal (SIGCHLD, SIG_IGN);

      /* Put ourselves in our own process group.  */
      sh->shell_pgid = getpid ();
      if (setpgid (sh->shell_pgid, sh->shell_pgid) < 0) {
          perror ("Couldn't put the shell in its own process group");
          exit (1);
        }

      /* Grab control of the terminal.  */
      tcsetpgrp (sh->shell_terminal, sh->shell_pgid);

      /* Save default terminal attributes for shell.  */
      tcgetattr (sh->shell_terminal, &sh->shell_tmodes);
    }
  }

  void sh_destroy(struct shell *sh) {
    free(sh);
  }

 // void parse_args(int argc, char **argv) {}