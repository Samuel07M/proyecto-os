#include "kernel/types.h" // Tipos de datos estandar de xv6
#include "user/user.h" // Tipos de datos estandar de xv6
#include "kernel/fcntl.h" // Define las banderas de control de archivos (como O_RDWR para abrir en modo lectura/escritura)
#include "commands.h"

// Funcion auxiliar para la redireccion de entrada '<' y salida '>'
static void apply_redirections(struct stage *st) {
  // Redireccion de entrada '<'
  if (st->infile) {
    int fd = open(st->infile, O_RDONLY); // Abre el archivo especificado en modo solo lectura
    // Si fd es menor que 0, significa que hubo un error
    if (fd < 0) {
      printf("sh: no se pudo abrir %s\n", st->infile);
      exit(1);
    }
    close(0); // Cierra la entrada estandar (teclado), liberando el 0
    dup(fd); // Duplica el archivo fd y lo asigna al 0 liberado
    close(fd); // Cierra el archivo original porque ya se reasigno
  }

  // Redireccion de salida '>'
  if (st->outfile) {
    int fd = open(st->outfile, O_WRONLY | O_CREATE | O_TRUNC); // Abre el archivo para escritura, lo crea y borra su contenido
    // Si fd es menor que 0, significa que hubo un error
    if (fd < 0) {
      printf("sh: no se pudo crear %s\n", st->outfile);
      exit(1);
    }
    close(1); // Cierra la salida estandar (pantalla), liberando el 1
    dup(fd); // Duplica el archivo y lo asigna al 1 liberado
    close(fd); // Cierra el fd temporal
  }
}

// Funcion para ejecutar los comandos
void run_pipeline(struct pipeline *pl) {
  int n = pl->nstages; // Define la cantidad de estados
  int prevfd = -1; // Almacena el extremo de lectura del pipe de la etapa anterior
  int created = 0; // Cuenta cuantos procesos hijos se crearon con exito

  // Ciclo para cada comando
  for (int i = 0; i < n; i++) {
    struct stage *st = &pl->stages[i];
    int have_next = (i < n - 1);
    int fd[2];

    // Condicion para cuando se llegue al ultimo comando
    if (have_next && pipe(fd) < 0) {
      printf("sh: error creando pipe\n");
      if (prevfd != -1)
        close(prevfd);
      break;
    }

    // Creacion de los procesos hijo
    int pid = fork();

    // Condicion por si falla la creacion del proceso hijo
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

    // Bloque del proceso hijo
    if (pid == 0) {

      // 1. Si venimos de un pipe anterior, conectamos su lectura a nuestra stdin (0)
      if (prevfd != -1) {
        close(0);
        dup(prevfd);
        close(prevfd);
      }

      // 2. Si hay una etapa siguiente, conectamos nuestra stdout (1) a la escritura del pipe actual
      if (have_next) {
        close(fd[0]);
        close(1);
        dup(fd[1]);
        close(fd[1]);
      }

      // 3. Llamado a la funcion auxiliar de redirecciones si el comando lo requiere
      apply_redirections(st);

      // Comprobar que si hayan argumentos en el comando
      if (st->argc == 0)
        exit(0);

      exec(st->argv[0], st->argv); // 4. Se transforma en el ejecutable final (ej: ls, grep)
      printf("sh: no se pudo ejecutar %s\n", st->argv[0]);
      exit(1);
    }

    created++;

    // Preparar el pipe para la siguiente iteración
    if (prevfd != -1)
      close(prevfd);

    if (have_next) {
      close(fd[1]);
      prevfd = fd[0];
    } else {
      prevfd = -1;
    }
  }

  // Ciclo del proceso padre para esperar la terminacion de sus hijos
  for (int i = 0; i < created; i++)
    wait(0);
}

// Funcion para limpiar toda la memoria dinamica asignada previamente en parser.c
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