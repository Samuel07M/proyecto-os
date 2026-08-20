#include "kernel/types.h" // Tipos de datos estandar de xv6
#include "user/user.h" // Tipos de datos estandar de xv6
#include "kernel/fcntl.h" // Define las banderas de control de archivos (como O_RDWR para abrir en modo lectura/escritura)
#include "utils.h"
#include "parser.h"
#include "commands.h"

// Funcion para asegurar que que la shell tenga sus fd's predeterminados al iniciar su ejecucion
static void ensure_std_fds(void) {
  int fd;
  while ((fd = open("console", O_RDWR)) >= 0) {
    // El ciclo termina cuando esta condicion sea verdadera, porque los fd's predeterminados ya están ocupados
    if (fd >= 3) {
      close(fd);
      break;
    }
  }
}

int main(void) {
  printf("\n>>> SOY LA NUEVA SHELL <<<\n\n");
  static char line[MAXLINE]; // declara un arreglo "line" de maximo MAXLINE = 128 caracteres

  // Funcion para asegurar los canales de entrada y salida predeterminados
  ensure_std_fds();

  // Ciclo REPL (Read, Eval, Print Loop)
  while (1) {
    printf("$ ");
    memset(line, 0, sizeof(line));
    gets(line, sizeof(line));

    if (line[0] == 0)
      break;

    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0'; // Ubica al final del arreglo de linea un caracter nulo
      len--;
    }

    struct pipeline pl;
    int r = parse_line(line, &pl); // parse_line procesa el arreglo line 
    // Si parse_line devuelve 0 o un # negativo, la shell ignora y vuelve al inicio del while principal
    if (r <= 0)
      continue;

    if (pl.nstages == 1 && pl.stages[0].argc >= 1 &&
        strcmp(pl.stages[0].argv[0], "exit") == 0) {
      free_pipeline(&pl);
      exit(0);
    }

    run_pipeline(&pl); // Envia la estructura procesada para que se ejecute lo que se tenga que ejecutar 
    free_pipeline(&pl); // Libera la memoria asignada a la ejecucion anterior luego de finalizar
  }

  exit(0); // Cuando se salga del bucle, la shell acaba de forma controlada.
}