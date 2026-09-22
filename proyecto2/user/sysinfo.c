#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int main(void) {
  struct sysinfo info;

  // Mensaje descriptivo por stderr y salida distinta de cero
  if (sysinfo(&info) < 0) {
    fprintf(2, "sysinfo: no se pudo obtener la informacion del sistema\n");
    exit(1);

  }

  printf("Free Memory: %d MB\n", (int)(info.freemem / (1024 * 1024)));
  printf("Used Pages: %d\n", (int)info.usedpages);
  printf("Available Pages: %d\n", (int)info.freepages);
  printf("Runnable Processes: %d\n", (int)info.runnableprocs);

  exit(0);
}
