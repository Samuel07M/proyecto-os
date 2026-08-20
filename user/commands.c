#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "commands.h"

static void apply_redirections(struct stage *st) {
  if (st->infile) {
    int fd = open(st->infile, O_RDONLY);
    if (fd < 0) {
      printf("sh: no se pudo abrir %s\n", st->infile);
      exit(1);
    }
    close(0);
    dup(fd);
    close(fd);
  }

  if (st->outfile) {
    int fd = open(st->outfile, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0) {
      printf("sh: no se pudo crear %s\n", st->outfile);
      exit(1);
    }
    close(1);
    dup(fd);
    close(fd);
  }
}

void run_pipeline(struct pipeline *pl) {
  int n = pl->nstages;
  int prevfd = -1;
  int created = 0;

  for (int i = 0; i < n; i++) {
    struct stage *st = &pl->stages[i];
    int have_next = (i < n - 1);
    int fd[2];

    if (have_next && pipe(fd) < 0) {
      printf("sh: error creando pipe\n");
      if (prevfd != -1)
        close(prevfd);
      break;
    }

    int pid = fork();
    if (pid < 0) {
      printf("sh: error en fork\n");
      if (have_next) {
        close(fd[0]);
        close(fd[1]);
      }
      if (prevfd != -1)
        close(prevfd);
      break;
    }

    if (pid == 0) {
      if (prevfd != -1) {
        close(0);
        dup(prevfd);
        close(prevfd);
      }
      if (have_next) {
        close(fd[0]);
        close(1);
        dup(fd[1]);
        close(fd[1]);
      }

      apply_redirections(st);

      if (st->argc == 0)
        exit(0);

      exec(st->argv[0], st->argv);
      printf("sh: no se pudo ejecutar %s\n", st->argv[0]);
      exit(1);
    }

    created++;
    if (prevfd != -1)
      close(prevfd);

    if (have_next) {
      close(fd[1]);
      prevfd = fd[0];
    } else {
      prevfd = -1;
    }
  }

  for (int i = 0; i < created; i++)
    wait(0);
}

void free_pipeline(struct pipeline *pl) {
  for (int i = 0; i < pl->nstages; i++) {
    struct stage *st = &pl->stages[i];
    for (int j = 0; j < st->argc; j++)
      free(st->argv[j]);
    if (st->infile)
      free(st->infile);
    if (st->outfile)
      free(st->outfile);
  }
}