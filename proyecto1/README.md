# Proyecto de Sistemas Operativos
## Implementación de un Shell para xv6
### Integrantes
- Samuel Molina Garcés
- Juan Diego Parra Castañeda
### Curso
Sistemas Operativos
### Semestre
2026-2
---
# Descripción
Este proyecto implementa un shell para el sistema operativo xv6. El programa
permite interpretar comandos ingresados por el usuario desde la entrada
estándar y ejecutar los programas existentes en xv6 mediante la creación de
nuevos procesos.
La implementación soporta:
- Ejecución de comandos simples.
- Ejecución de comandos con argumentos.
- Redirección de entrada (`<`).
- Redirección de salida (`>`).
- Tuberías simples y múltiples (`|`).
- Comando interno `exit` para finalizar el shell.
La solución fue desarrollada utilizando lenguaje C y se integra al proceso de
compilación estándar de xv6 ejecutado sobre QEMU.
---
# Estructura de archivos
```text
user/
├── sh.c
├── parser.c
├── parser.h
├── commands.c
├── commands.h
└── utils.h
```
## Descripción de los archivos
### sh.c
Archivo principal del shell.
### parser.c
Contiene las funciones encargadas de analizar la línea de comandos ingresada por
el usuario.
### parser.h
Definiciones y prototipos asociados al parser.
### commands.c
Implementación de rutinas relacionadas con la ejecución de comandos y
tuberías.
### commands.h
Prototipos de funciones relacionadas con la ejecución.
### utils.h
Constantes y funciones auxiliares utilizadas en el proyecto.
---
# Integración con xv6
## Paso 1. Obtener xv6
```bash
git clone https://github.com/mit-pdos/xv6-riscv.git
```
## Paso 2. Copiar archivos
Copiar todos los archivos entregados dentro del directorio `user/`.
## Paso 3. Modificar el Makefile
Agregar el programa al listado de aplicaciones de usuario.
## Paso 4. Compilar xv6
```bash
make qemu
```
## Paso 5. Ejecutar el shell
```bash
sh
```
---
# Casos de prueba
- Ejecución simple.
- Argumentos.
- Redirección de entrada.
- Redirección de salida.
- Tuberías simples.
- Tuberías múltiples.
- Comando `exit`.
---
# Decisiones de diseño
- Separación entre análisis sintáctico y ejecución.
- Uso de procesos independientes para cada etapa de una tubería.
- Manejo explícito de descriptores de archivo durante las redirecciones.
---
# Limitaciones conocidas
- No soporta comillas.
- No soporta variables de entorno.
- No soporta ejecución en segundo plano.
---
# Declaración de uso de IA
Durante el desarrollo del proyecto se utilizaron herramientas de inteligencia
artificial generativa como apoyo para comprensión de conceptos, revisión de
código y generación de ejemplos.
Los integrantes asumen plena responsabilidad académica sobre el contenido
entregado.