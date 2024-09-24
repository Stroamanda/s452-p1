/**Update this file with the starter code**/
#include "../src/lab.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>

  char *get_prompt(const char *env) {
      char* prompt;

      prompt = getenv("MY_PROMPT");

      if (prompt == NULL) {
        prompt = "$ ";
      }

      return prompt;
  }

  int change_dir(char **dir) {
    char *direct = "";
     if (strcmp(*dir, "cd") == 0) {
      if (getenv("HOME") == NULL) {
        direct = getpwuid(getuid())->pw_dir;
        chdir(direct);
      } else {
        direct = getenv("HOME");
        chdir(direct);
      }
     } else if (strncmp(*dir, "cd ", strlen("cd ")) == 0) {
        if (strchr(*dir + strlen("cd "), ' ') == NULL) {
          direct = *dir + strlen("cd ");

          if (chdir(direct) != 0) {
            printf("Couldn't Find Directory\n");
          } 
        } else {
          printf("Couldn't Find Directory\n");
        }
     }
    return 0;
  }