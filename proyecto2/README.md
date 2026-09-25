# Proyecto 2 – System Calls en xv6: `trace` y `sysinfo`

## Información General

| Información | Detalle |
|---|---|
| **Nombre del proyecto** | Proyecto 2 – System Calls en xv6 (`trace` y `sysinfo`) |
| **Sistema base** | [xv6-riscv](https://github.com/mit-pdos/xv6-riscv), commit `75c46385ee4fe194dee76a5bfdc1a55b16ff4a40` (27-jul-2026) |

### Integrantes
- Samuel Molina Garcés
- Juan Diego Parra Castañeda

---

## Descripción de la Solución

Se extendió el kernel de xv6-riscv con dos nuevas llamadas al sistema orientadas al monitoreo e introspección del sistema, junto con un programa de usuario para probar cada una.

- **`trace`**: recibe el nombre de una syscall (por ejemplo `sys_kill`) y activa su monitoreo para el proceso que la invoca. Cada vez que ese proceso (o un hijo suyo) ejecuta la syscall monitoreada, el kernel imprime el **PID**, el **nombre de la syscall**, el **valor de retorno** y **registros RISC-V relevantes** (`s0`, `s1`, `a0`, `a1`). El programa `user/trace.c` permite usarla desde la línea de comandos: `trace <syscall> <comando> [args...]`.
- **`sysinfo`**: recopila el estado actual del sistema y lo entrega al espacio de usuario mediante una estructura definida por el equipo (`struct sysinfo`, en `kernel/sysinfo.h`): **memoria libre (MB)**, **páginas usadas**, **páginas disponibles** y **procesos en estado RUNNABLE**. El programa `user/sysinfo.c` muestra esa información.

La solución compila sin errores dentro de xv6-riscv, se ejecuta en QEMU y no altera el funcionamiento normal de xv6 (se verificó con `ls`, `cat`, `echo`, redirecciones y tuberías).

### Estructura del repositorio

```text
proyecto-os-2/
├── README.md
├── Makefile                 # UPROGS con _trace y _sysinfo
├── kernel/                  # archivos del kernel modificados o creados
│   ├── syscall.h
│   ├── syscall.c
│   ├── sysproc.c
│   ├── sysinfo.h            # (nuevo)
│   ├── proc.h
│   ├── proc.c
│   ├── kalloc.c
│   └── defs.h
├── user/
│   ├── trace.c              # (nuevo) programa de prueba de trace
│   ├── sysinfo.c            # (nuevo) programa de prueba de sysinfo
│   ├── user.h
│   └── usys.pl
└── autoevaluacion/          
proyecto-os-1/               # shell del Proyecto 1
```

---

## Archivos Modificados

Todos los cambios están marcados en el código con el comentario `// Nuevo` (o comentarios que inician con "Nuevo:").

### `kernel/`

| Archivo | Tipo | Descripción |
|---|---|---|
| `syscall.h` | Modificado | Define los números de las nuevas syscalls: `SYS_trace` (23) y `SYS_sysinfo` (24). |
| `syscall.c` | Modificado | Registra `sys_trace` y `sys_sysinfo` en la tabla `syscalls[]`. Agrega la tabla `syscallnames[]` (número → nombre `sys_xxx`), la función `syscall_exists()` para validar nombres, la función `trace_report()` que imprime la información de monitoreo y la lógica dentro de `syscall()` que decide cuándo reportar. |
| `sysproc.c` | Modificado | Implementa `sys_trace()` (lee y valida el nombre de la syscall y lo guarda en el proceso) y `sys_sysinfo()` (llena `struct sysinfo` y la copia al espacio de usuario con `copyout`). |
| `sysinfo.h` | **Nuevo** | Define `struct sysinfo`, la estructura compartida entre kernel y usuario para transferir la información. |
| `proc.h` | Modificado | Agrega el campo `trace_name[16]` a `struct proc`: nombre de la syscall monitoreada por el proceso (cadena vacía = sin trace). |
| `proc.c` | Modificado | Limpia `trace_name` en `freeproc()`, lo hereda al hijo en `kfork()` y agrega `runnable_count()`, que cuenta los procesos en estado `RUNNABLE`. |
| `kalloc.c` | Modificado | Agrega `kfreepages()` (páginas libres, recorriendo la lista de libres bajo lock) y `ktotalpages()` (total de páginas administradas por el asignador). |
| `defs.h` | Modificado | Prototipos de `kfreepages`, `ktotalpages`, `runnable_count` y `syscall_exists`. |

### `user/`

| Archivo | Tipo | Descripción |
|---|---|---|
| `trace.c` | **Nuevo** | Programa de usuario: `trace <syscall> <comando> [args...]`. Activa el monitoreo y ejecuta el comando con `exec`. |
| `sysinfo.c` | **Nuevo** | Programa de usuario: invoca `sysinfo()` e imprime los cuatro datos requeridos. |
| `user.h` | Modificado | Prototipos de `trace()` y `sysinfo()`. |
| `usys.pl` | Modificado | Genera los stubs de usuario (`li a7, SYS_xxx; ecall; ret`) de las dos nuevas syscalls. |

### Raíz

| Archivo | Tipo | Descripción |
|---|---|---|
| `Makefile` | Modificado | Agrega `$U/_trace` y `$U/_sysinfo` a `UPROGS` para que se compilen e incluyan en `fs.img`. |

---

## Diseño Realizado

### Flujo general de una syscall en xv6 (base del diseño)
El programa de usuario invoca el stub generado por `usys.pl`, que carga el número de la syscall en `a7` y ejecuta `ecall`. El hardware entra al kernel por el mecanismo de traps (`usertrap`), que guarda los registros en el `trapframe` y llama a `syscall()`. Esta función usa `a7` para buscar la implementación en `syscalls[]`, la ejecuta y deja el valor de retorno en `a0` del `trapframe`, que se restaura al volver a modo usuario. Ambas syscalls nuevas se integran en este mismo flujo.

### `trace`

- **Estado por proceso.** El nombre de la syscall monitoreada se guarda en `p->trace_name` (`struct proc`). Así el monitoreo es independiente para cada proceso y no requiere estructuras globales ni locks adicionales (el campo es privado del proceso).
- **Dónde se reporta.** Dentro de `syscall()`, **después** de ejecutar la syscall, se compara `p->trace_name` con el nombre de la syscall recién ejecutada (`syscallnames[num]`). Se reporta después porque el valor de retorno solo existe una vez ejecutada. Consecuencia: la salida del trace aparece después de lo que la propia syscall haya escrito (por ejemplo, con `write` se ve `hola` antes del bloque `PID:`).
- **Registros mostrados.** `a0` y `a1` se capturan **antes** de ejecutar la syscall, porque `a0` se sobrescribe con el valor de retorno; así el reporte muestra los argumentos originales con los que se invocó (p. ej. `a0 = 0x3e7` para `kill 999`) y el retorno aparece por separado en `RETURN`. `s0` y `s1` son registros *callee-saved* de RISC-V y se leen del `trapframe`.
- **Validación del nombre.** `sys_trace()` rechaza (retorna `-1`) cualquier nombre que no exista en `syscallnames[]` mediante `syscall_exists()`. Los nombres se escriben con el prefijo `sys_`, igual que en el enunciado (`trace sys_kill`). También se rechaza un nombre que no cabe en el buffer de 16 bytes.
- **Herencia.** `kfork()` copia `trace_name` al proceso hijo, de modo que un comando monitoreado que crea procesos también se monitorea en sus descendientes. `freeproc()` limpia el campo cuando el proceso termina, para que una ranura reutilizada de la tabla de procesos no herede un trace anterior.
- **Programa `trace.c`.** Valida los argumentos, invoca `trace()` y luego hace `exec` del comando. Como `trace_name` vive en el `struct proc` y `exec` conserva el proceso, el monitoreo sigue activo sobre el comando ejecutado.

### `sysinfo`

- **Estructura de transferencia.** `struct sysinfo` (`kernel/sysinfo.h`) tiene cuatro campos `uint64`: `freemem`, `usedpages`, `freepages` y `runnableprocs`. El mismo archivo se incluye desde el kernel y desde `user/sysinfo.c`, de modo que ambos lados comparten exactamente el mismo formato.
- **Páginas y memoria libre.** `kfreepages()` recorre la lista de páginas libres del asignador (`kmem.freelist`) con `kmem.lock` tomado. `ktotalpages()` calcula el total de páginas administradas a partir del rango `[end, PHYSTOP)` que `kinit()` entrega al asignador. Con eso: `freepages = kfreepages()`, `usedpages = total − freepages` y `freemem = freepages × PGSIZE` (en bytes; el programa de usuario lo convierte a MB).
- **Procesos RUNNABLE.** `runnable_count()` recorre `proc[NPROC]` tomando `p->lock` de cada entrada para leer el estado de forma consistente.
- **Transferencia a usuario.** `sys_sysinfo()` obtiene la dirección de destino con `argaddr()` y copia la estructura con `copyout()` usando la tabla de páginas del proceso que invoca. Si la copia falla, retorna `-1`.

### Manejo de errores
- **Kernel:** las syscalls retornan `-1` ante nombre inválido, nombre demasiado largo o dirección de usuario inválida.
- **Programas de usuario:** los mensajes de error son descriptivos, se envían a **stderr** (descriptor 2) y el programa termina con `exit(1)`. Casos cubiertos: argumentos insuficientes, syscall inválida, comando que no se puede ejecutar y fallo de `sysinfo`.

### Integración con el Proyecto 1
Este proyecto se desarrolló sobre el mismo árbol de xv6 en el que se usa el shell implementado en el Proyecto 1 (`proyecto-os-1/`). Por eso el `Makefile` incluye la regla de enlazado `$U/_sh` con `parser.o` y `commands.o`, y en la compilación es necesario copiar antes los archivos de `proyecto-os-1/user/`. Las syscalls `trace` y `sysinfo` no dependen del shell nuevo, solo se usa como intérprete de comandos para las pruebas.

---

## Compilación

### Pasos

```bash
# 1. Obtener xv6-riscv en el commit base usado por el proyecto
git clone https://github.com/mit-pdos/xv6-riscv.git

# 2. Entrar y mover los archivos a un commit especifico de la historia del repositorio
cd xv6-riscv
git checkout 75c46385ee4fe194dee76a5bfdc1a55b16ff4a40

# 3. Obtener el repositorio del proyecto
git clone https://github.com/Samuel07M/proyecto-os

# 4. Copiar el shell del Proyecto 1
cp ~/.../proyecto-os/proyecto1/user/* ~/.../xv6-riscv/user/

# 5. Copiar los archivos del Proyecto 2
cp ~/.../proyecto-os/proyecto2/user/* ~/.../xv6-riscv/user/
cp ~/.../proyecto-os/proyecto1/kernel/* ~/.../xv6-riscv/kernel/
cp ~/.../proyecto-os/proyecto2/Makefile ~/.../xv6-riscv/
```

---

## Ejecución

```bash
make clean && make qemu
```

Cuando aparece el prompt `$` se pueden usar los programas nuevos. Para salir de QEMU: `Ctrl-A` y luego `X`.

### Ejemplos de una ejecución normal de las syscalls implementadas

### `sysinfo`
```
$ sysinfo
Free Memory: 127 MB
Used Pages: 216
Available Pages: 32516
Runnable Processes: 0
```

### `trace <syscall> <comando> [argumentos...]`
```
$ trace sys_kill kill 999
PID: 7
SYSCALL: sys_kill
RETURN: -1
s0: 0x0000000000003fb0
s1: 0x0000000000003fc8
a0: 0x00000000000003e7
a1: 0x0000000000003fd0
```
---

## Uso de IA

Se utilizó la herramienta de IA generativa **Claude (Anthropic)** como apoyo durante el desarrollo, en las siguientes tareas:

- Comprensión del flujo de una syscall en xv6-riscv (stub de usuario, `ecall`, trap, `syscall()`, retorno).
- Revisión del código del equipo frente a los requerimientos y la rúbrica del enunciado, y propuesta de mejoras (validación del nombre en `trace`, captura de los argumentos originales `a0`/`a1` antes de que se sobrescriban con el retorno, y herencia del monitoreo a procesos hijos).
- Diseño de las pruebas y verificación de la compilación y la ejecución en QEMU.
- Apoyo en la redacción y estructura de este README.
