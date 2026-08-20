#ifndef COMMANDS_H // GUARDAS DE INCLUSION
#define COMMANDS_H // GUARDAS DE INCLUSION

#include "parser.h" // Se requiere usar la estructura "struct pipeline" definida en parser.h

// Corazon ejecutor de la shell, recibe etapas, comandos, argumentos y archivos de redireccion
void run_pipeline(struct pipeline *pl);

// Funcion encargada de la liberacion de memoria asignada a nombres de argumentos y archivos de redireccion 
void free_pipeline(struct pipeline *pl);

#endif // Fin de la condicion del archivo de configuracion