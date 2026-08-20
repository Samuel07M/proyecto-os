#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "utils.h"
#include "parser.h"
#include "commands.h"

// Funcion para asegurar que que la shell tenga sus fd's predeterminados al iniciar su ejecucion
static void ensure_std_fds(void) {
  int fd;
  while ((fd = open("console", O_RDWR)) >= 0) {
    if (fd >= 3) {
      close(fd);
      break;
    }
  }
}

// Funcion principal con ciclo REPL (Read, Eval, Print Loop)
int main(void) {
  printf("\n>>> SOY LA NUEVA SHELL <<<\n\n");
  static char line[MAXLINE];

  ensure_std_fds();

  while (1) {
    printf("$ ");
    memset(line, 0, sizeof(line));
    gets(line, sizeof(line));

    // Condicion por si el usuario no ingresa nada por teclado
    if (line[0] == 0)
      break;

    // Bloque para limpiar los saltos de linea \n y \r
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0';
      len--;
    }

    // Bloque de analisis lexico y sintactico
    struct pipeline pl;
    int r = parse_line(line, &pl);
    if (r <= 0)
      continue;

    // Condicion para procesar el comando "exit"
    if (pl.nstages == 1 && pl.stages[0].argc >= 1 && strcmp(pl.stages[0].argv[0], "exit") == 0) {
      free_pipeline(&pl);
      exit(0);
    }

    run_pipeline(&pl);
    free_pipeline(&pl);
  }

  exit(0);
}
