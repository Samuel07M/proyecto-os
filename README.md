# Sistemas Operativos 2026-2 – Proyectos sobre xv6-riscv

Repositorio con los proyectos desarrollados en el curso de **Sistemas Operativos** (Ingeniería de Sistemas, semestre 2026-2, docente José Luis Montoya Pareja). Ambos proyectos extienden [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) y están escritos en C para ejecutarse sobre QEMU.

## Integrantes
- Samuel Molina Garcés
- Juan Diego Parra Castañeda

## Proyectos

| Proyecto | Carpeta | Descripción | Documentación |
|---|---|---|---|
| **Proyecto 1** – Shell para xv6 | [`proyecto1/`](proyecto1/) | Shell con ejecución de comandos, argumentos, redirección (`<`, `>`), tuberías (`\|`) y comando interno `exit`. | [`proyecto1/README.md`](proyecto1/README.md) |
| **Proyecto 2** – System Calls en xv6 | [`proyecto2/`](proyecto2/) | Nuevas syscalls `trace` (monitoreo de una syscall) y `sysinfo` (estado del sistema: memoria libre, páginas usadas y disponibles, procesos RUNNABLE), con sus programas de prueba. | [`proyecto2/README.md`](proyecto2/README.md) |

## Estructura del repositorio

```text
proyecto-os/
├── README.md                # este archivo
├── proyecto1/               # Shell para xv6
│   ├── README.md
│   └── user/                # sh.c, parser.c/.h, commands.c/.h, utils.h
└── proyecto2/               # System Calls en xv6
    ├── README.md
    ├── Makefile             # UPROGS con _trace y _sysinfo + regla del shell del Proyecto 1
    ├── kernel/              # archivos del kernel modificados o creados
    ├── user/                # trace.c, sysinfo.c, user.h, usys.pl
    └── autoevaluacion/      # un archivo individual por integrante
```

En cada carpeta solo hay archivos modificados o creados por el grupo; **no se incluye el repositorio completo de xv6**. Los archivos se copian sobre xv6 (carpetas `kernel/` y `user/`) para compilar.

## Relación entre los proyectos

El Proyecto 2 se desarrolló sobre el mismo árbol de xv6 donde se usa el shell del Proyecto 1. Por eso el `Makefile` de `proyecto2/` incluye la regla de enlazado del shell (`sh.o`, `parser.o`, `commands.o`) y, para compilar, hay que copiar antes los archivos de `proyecto1/user/`. Las syscalls `trace` y `sysinfo` no dependen del shell: solo se usa como intérprete de comandos en las pruebas.

## Guía rápida (Proyecto 2)

Requisitos: Linux con `git`, `make`, `perl`, compilador cruzado RISC-V y `qemu-system-riscv64`.

```bash
# 1. xv6-riscv en el commit base usado por el proyecto
git clone https://github.com/mit-pdos/xv6-riscv.git
cd xv6-riscv
git checkout 75c46385ee4fe194dee76a5bfdc1a55b16ff4a40

# 2. Este repositorio (al lado de xv6-riscv)
git clone https://github.com/Samuel07M/proyecto-os.git ../proyecto-os

# 3. Copiar el shell (Proyecto 1) y luego los archivos del Proyecto 2
cp ../proyecto-os/proyecto1/user/* user/
cp -r ../proyecto-os/proyecto2/kernel/* kernel/
cp -r ../proyecto-os/proyecto2/user/* user/
cp ../proyecto-os/proyecto2/Makefile Makefile

# 4. Compilar y ejecutar
make qemu
```

Ya dentro de xv6:

```text
$ sysinfo
$ trace sys_kill kill 999
```

Los pasos completos, el diseño, las pruebas realizadas y la declaración de uso de IA están en el README de cada proyecto.
