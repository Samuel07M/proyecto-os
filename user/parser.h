#ifndef PARSER_H
#define PARSER_H
#include "utils.h"

// comando + argumentos + archivos de redireccion (si aplica).
struct stage {
  char *argv[MAXARGS + 1];
  int argc;
  char *infile;
  char *outfile;
};

// secuencia de etapas conectadas por '|'.
struct pipeline {
  struct stage stages[MAXSTAGES];
  int nstages;
};

int parse_line(char *line, struct pipeline *pl);

#endif