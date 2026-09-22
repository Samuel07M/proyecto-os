#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  // Valida argumentos con mensaje por stderr y salida distinta de cero 
  if (argc < 3) {
    fprintf(2, "uso: trace <syscall> <comando> [argumentos...]\n");
    exit(1);
  }

  // Activa el monitoreo de la syscall solicitada
  if (trace(argv[1]) < 0) {
    fprintf(2, "trace: no se pudo activar el monitoreo de %s\n", argv[1]);
    exit(1);
  }

  // Reemplaza la imagen del proceso por el comando a monitorear
  exec(argv[2], &argv[2]);

  // Solo se llega aqui si exec falla
  fprintf(2, "trace: no se pudo ejecutar %s\n", argv[2]);
  exit(1);
}
