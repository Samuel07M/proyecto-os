#ifndef SYSINFO_H
#define SYSINFO_H

// Estructura utilizada para transferir informacion del estado interno del
// kernel hacia los programas de usuario mediante la syscall sysinfo.
// Es compartida entre kernel (sysproc.c) y espacio de usuario (sysinfo.c).
struct sysinfo {
  uint64 freemem;       // Memoria fisica libre, en bytes
  uint64 usedpages;     // Cantidad de paginas fisicas actualmente en uso
  uint64 freepages;     // Cantidad de paginas fisicas disponibles
  uint64 runnableprocs; // Cantidad de procesos en estado RUNNABLE
};

#endif
