#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "parser.h"
#include "commands.h"

static void ensure_std_fds(void) {
  int fd;
  while ((fd = open("console", O_RDWR)) >= 0) {
    if (fd >= 3) {
      close(fd);
      break;
    }
  }
}

int main(void) {
  printf("\n>>> SOY LA NUEVA SHELL <<<\n\n");
  static char line[MAXLINE];

  ensure_std_fds();

  while (1) {
    printf("$ ");
    memset(line, 0, sizeof(line));
    gets(line, sizeof(line));

    if (line[0] == 0)
      break;

    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0';
      len--;
    }

    struct pipeline pl;
    int r = parse_line(line, &pl);
    if (r <= 0)
      continue;

    if (pl.nstages == 1 && pl.stages[0].argc >= 1 &&
        strcmp(pl.stages[0].argv[0], "exit") == 0) {
      free_pipeline(&pl);
      exit(0);
    }

    run_pipeline(&pl);
    free_pipeline(&pl);
  }

  exit(0);
}