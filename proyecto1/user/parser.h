#ifndef PARSER_H // GUARDA DE INCLUSION
#define PARSER_H // GUARDA DE INCLUSION
#include "utils.h"

// Comando + argumentos + archivos de redireccion (si aplica).
struct stage {
  char *argv[MAXARGS + 1]; // argv[0..argc-1] + NULL final
  int argc;
  char *infile;             // != 0 si hay '<'
  char *outfile;            // != 0 si hay '>'
};

// Secuencia de etapas conectadas por '|'.
struct pipeline {
  struct stage stages[MAXSTAGES];
  int nstages;
};

// Funcion declarada para procesar el comando ingresado por el usuario y llenar la estructura pipeline
int parse_line(char *line, struct pipeline *pl);

#endif