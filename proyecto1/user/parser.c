#include "kernel/types.h" // Tipos de datos estandar de xv6
#include "user/user.h" // Tipos de datos estandar de xv6
#include "parser.h" 
#include "commands.h"
#define TOK_WORD 1 // Palabras normales (ej: "ls", "-l", "archivo.txt")
#define TOK_PIPE 2 // El símbolo '|'
#define TOK_LT   3 // El símbolo '<'
#define TOK_GT   4 // El símbolo '>'
#define TOK_END  5 // Fin de la cadena de texto
#define TOK_ERR  6 // Error al extraer un token

// Funcion auxiliar para saltarse espacios en blanco
static char* skip_blanks(char *p, char *end) {
  while (p < end && (*p == ' ' || *p == '\t'))
    p++;
  return p;
}

// 
static int gettoken(char **pp, char *end, char *buf, int bufsize) {
  char *p = skip_blanks(*pp, end);

  if (p >= end) {
    *pp = p;
    return TOK_END;
  }

  if (*p == '|') {
    *pp = p + 1;
    return TOK_PIPE;
  }
  if (*p == '<') {
    *pp = p + 1;
    return TOK_LT;
  }
  if (*p == '>') {
    *pp = p + 1;
    return TOK_GT;
  }

  char *start = p;
  while (p < end && *p != ' ' && *p != '\t' && *p != '|' && *p != '<' && *p != '>') 
    p++;

  int len = p - start;
  if (len <= 0 || len >= bufsize) {
    *pp = p;
    return TOK_ERR;
  }

  memmove(buf, start, len);
  buf[len] = '\0';

  *pp = p;
  return TOK_WORD;
}

// Funcion auxiliar para copiar cadenas de texto de forma dinamica en memoria
static char* dupword(char *word) {
  int len = strlen(word);
  char *copy = malloc(len + 1);
  if (copy == 0) {
    printf("sh: error de memoria\n");
    return 0;
  }
  strcpy(copy, word);
  return copy;
}

// Funcion auxiliar para manejar errores de sintaxis
static int parse_fail(struct pipeline *pl, int pending) {
  if (pending)
    pl->nstages++;
  free_pipeline(pl);
  pl->nstages = 0;
  return -1;
}

// Funcion principal para armar el pipeline
int parse_line(char *line, struct pipeline *pl) {
  char *p = line; // Prepara puntero para recorrer la cadena line
  char *end = line + strlen(line); // Prepara puntero para recorrer la cadena line
  char tokbuf[MAXLINE];

  pl->nstages = 0; // Inicializa la primera etapa
  struct stage *st = &pl->stages[0];
  st->argc = 0;
  st->infile = 0;
  st->outfile = 0;

  int have_command = 0; // Indicador para verificar que cada etapa tenga un comando antes de '|' o del final

  // Ciclo del parseo
  for (;;) {
    int tok = gettoken(&p, end, tokbuf, sizeof(tokbuf));

    if (tok == TOK_END)
      break;

    if (tok == TOK_ERR) {
      printf("sh: token invalido\n");
      return parse_fail(pl, 1);
    }

    // Condicion para guardar las palabras duplicadas
    if (tok == TOK_WORD) {
      if (!have_command && pl->nstages >= MAXSTAGES) {
        printf("sh: demasiados comandos encadenados con '|'\n");
        return parse_fail(pl, 1);
      }
      if (st->argc >= MAXARGS - 1) {
        printf("sh: demasiados argumentos\n");
        return parse_fail(pl, 1);
      }
      char *w = dupword(tokbuf);
      if (w == 0)
        return parse_fail(pl, 1);
      st->argv[st->argc++] = w;
      have_command = 1;
      continue;
    }

    // Condicion para validar argumentos redirecciones de entrada o salida
    if (tok == TOK_LT || tok == TOK_GT) {
      int t2 = gettoken(&p, end, tokbuf, sizeof(tokbuf));
      if (t2 != TOK_WORD) {
        printf("sh: se esperaba un nombre de archivo despues de '%s'\n",
               tok == TOK_LT ? "<" : ">");
        return parse_fail(pl, 1);
      }
      char *w = dupword(tokbuf);
      if (w == 0)
        return parse_fail(pl, 1);
      if (tok == TOK_LT) {
        if (st->infile)
          free(st->infile);
        st->infile = w;
      } else {
        if (st->outfile)
          free(st->outfile);
        st->outfile = w;
      }
      continue;
    }

    // Condicion para validar los argumentos entre pipelines
    if (tok == TOK_PIPE) {
      if (!have_command) {
        printf("sh: se esperaba un comando antes de '|'\n");
        return parse_fail(pl, 1);
      }
      st->argv[st->argc] = 0;
      pl->nstages++;
      if (pl->nstages >= MAXSTAGES) {
        printf("sh: demasiados comandos encadenados con '|'\n");
        return parse_fail(pl, 0);
      }
      st = &pl->stages[pl->nstages];
      st->argc = 0;
      st->infile = 0;
      st->outfile = 0;
      have_command = 0;
      continue;
    }
  }

  // Condicion para validar el cierre del comando ingresado por el usuario
  if (!have_command) {
    if (pl->nstages == 0)
      return 0;
    printf("sh: se esperaba un comando despues de '|'\n");
    return parse_fail(pl, 1);
  }

  st->argv[st->argc] = 0;
  pl->nstages++;

  return 1;
}