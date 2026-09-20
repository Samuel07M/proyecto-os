# Sistemas Operativos 2026-2 - xv6-riscv

Repositorio con los proyectos desarrollados en el curso de Sistemas Operativos (Ingeniería de Sistemas - Universidad EAFIT, profesor: José Luis Montoya Pareja). Ambos proyectos extienden [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) y están escritos en C para ejecutarse sobre QEMU.

## Integrantes
- Samuel Molina Garcés
- Juan Diego Parra Castañeda

## Proyectos

| Proyecto | Carpeta | Descripción | Documentación |
|---|---|---|---|
| **Proyecto 1** – Shell para xv6 | [`proyecto1/`](proyecto1/) | Shell con ejecución de comandos, argumentos, redirección (`<`, `>`), tuberías (`\|`) y comando interno `exit`. | [`proyecto1/README.md`](proyecto1/README.md) |
| **Proyecto 2** – System Calls en xv6 | [`proyecto2/`](proyecto2/) | Nuevas syscalls `trace` (monitoreo de una syscall) y `sysinfo` (estado del sistema: memoria libre, páginas usadas y disponibles, procesos RUNNABLE). | [`proyecto2/README.md`](proyecto2/README.md) |

## Estructura del repositorio

```text
proyecto-os/
├── README.md
├── proyecto1/
│   ├── README.md
│   └── user/
└── proyecto2/
    ├── README.md
    ├── Makefile
    ├── kernel/
    ├── user/
    └── autoevaluacion/
```

En cada carpeta solo hay archivos modificados o creados por el grupo; no se incluye el repositorio completo de xv6. Los archivos se copian sobre xv6 (carpetas `kernel/` y `user/`) para compilar.

## Relación entre los proyectos

El Proyecto 2 se desarrolló sobre el mismo árbol de xv6 donde se usa el shell del Proyecto 1. Por eso el `Makefile` de `proyecto2/` incluye la regla de enlazado del shell (`sh.o`, `parser.o`, `commands.o`) y, para compilar, hay que copiar antes los archivos de `proyecto1/user/`. Las syscalls `trace` y `sysinfo` no dependen del shell: solo se usa como intérprete de comandos en las pruebas.