// Archivo de configuracion para definir los limites de todo el sistema (incluidos todos los archivos de código)

#ifndef UTILS_H // GUARDAS DE INCLUSION
#define UTILS_H // GUARDAS DE INCLUSION

#define MAXLINE   128   // Longitud maxima de caracteres que se pueden escribir en la terminal
#define MAXARGS   10    // Maximo de argumentos por comando
#define MAXSTAGES 10    // maximo de comandos encadenados por tuberias ('|')

#endif // Fin de la condicion del archivo de configuracion

/*
IMPORTANCIA:
Este archivo es importante porque ayuda a gestionar el uso de la memoria para SO limitados como xv6, y evitar problemas 
de desbordamiento del buffer.
*/