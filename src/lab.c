#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <signal.h>
#include "../src/lab.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

  struct shell theShell;

  char *get_prompt(const char *env) {
      theShell.prompt = getenv("MY_PROMPT");

      if (theShell.prompt == NULL) {
        theShell.prompt = "$ ";
      }

      return theShell.prompt;
  }

  int change_dir(char **dir) {
    char *direct = "";

    // if there is only "cd"
     if (strcmp(*dir, "cd") == 0) {
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
     } else if (strncmp(*dir, "cd ", strlen("cd ")) == 0) {
        // check to make sure there is something there if there is a space
        if (strchr(*dir + strlen("cd "), ' ') == NULL) {
          // set direct to the directory name
          direct = *dir + strlen("cd ");

          // updates directory if it exists but also checks if it's not a directory
          if (chdir(direct) != 0) {
            printf("Couldn't Find Directory\n");
          } 
        } else {
          printf("Couldn't Find Directory\n");
        }
     }
    return 0;
  }

  char **cmd_parse(char const *line) {
    char **theArray = malloc (sizeof (char) * _SC_ARG_MAX);
    char *line2 = (char *) line;

    char *val = strtok(line2, " ");
    int curr = 0;

    // turns line into an array of values separated by spaces
    while (val != NULL) {
      theArray[curr++] = val;
      val = strtok(NULL, " ");
    }
    theArray[curr] = NULL;

    return theArray;
  }

  void cmd_free(char ** line) {
    free(line);
  }

  char *trim_white(char *line) {
    
  }

  bool do_builtin(struct shell *sh, char **argv) {}

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

  void sh_destroy(struct shell *sh) {}

  void parse_args(int argc, char **argv) {}