# Proyecto de Sistemas Operativos

## Proyecto 3: Tablas de páginas en acción (xv6)

### Integrantes
- Samuel Molina Garcés
- Juan Diego Parra Castañeda

### Curso
Sistemas Operativos

### Semestre
2026-2

---

# Descripción

Se extendió xv6-riscv (commit `06aad25`) con tres mecanismos que usan directamente la tabla de páginas de cada proceso.

**Parte A: página compartida y `ugetpid()`.** En cada proceso, el kernel reserva una página física con `kalloc()`, guarda en ella el pid y la mapea en `USYSCALL` (la página justo debajo de `TRAPFRAME`) con permisos `PTE_R | PTE_U`. La función de usuario `ugetpid()` lee el pid directamente de esa dirección, sin hacer una llamada al sistema. La página se libera junto con el proceso, y los límites de `growproc()` y `sys_sbrk()` se movieron de `TRAPFRAME` a `USYSCALL` para que el heap no la invada.

**Parte B: `vmprint()`.** Nueva llamada al sistema que imprime en consola la tabla de páginas del proceso que la invoca, recorriendo los tres niveles de Sv39 de forma recursiva (siguiendo el modelo de `freewalk()`). Imprime solo las PTEs válidas, por lo que las páginas de `sbrklazy()` que no se han tocado no aparecen.

**Parte C: `pgaccess()`.** Nueva llamada al sistema que consulta y limpia el bit de acceso `PTE_A` (bit 6) de un rango de hasta 64 páginas, y devuelve en una máscara cuáles fueron leídas o escritas desde la última consulta. Valida los argumentos y no asigna memoria a las páginas perezosas que no se han tocado.

---

# Archivos modificados

**kernel/**

- `memlayout.h`: se define `USYSCALL` (`TRAPFRAME - PGSIZE`) y `struct usyscall { int pid; }`, dentro de `#ifndef __ASSEMBLER__` porque `trampoline.S` también incluye este archivo.
- `riscv.h`: se define `PTE_A` (bit 6 de la PTE), que el hardware enciende cuando una página se lee o se escribe.
- `proc.h`: se agrega a `struct proc` el campo `usyscall`, que apunta a la página compartida.
- `proc.c`: `allocproc()` reserva y llena la página (antes de crear la tabla de páginas); `proc_pagetable()` la mapea con `PTE_R | PTE_U` y deshace los mapeos anteriores si falla; `proc_freepagetable()` desmapea `USYSCALL` y `freeproc()` libera la página física; `growproc()` compara contra `USYSCALL`.
- `vm.c`: se agrega `vmprint()` y su auxiliar recursiva `vmprint_level()`.
- `sysproc.c`: `sys_sbrk()` compara contra `USYSCALL`; se agregan `sys_vmprint()` y `sys_pgaccess()`.
- `syscall.h`, `syscall.c`: se registran `SYS_vmprint` (23) y `SYS_pgaccess` (24) en la tabla de llamadas.
- `defs.h`: se declara `vmprint(pagetable_t)`.

**user/**

- `ulib.c`: se agrega `ugetpid()`, que lee el pid de `USYSCALL`.
- `user.h`: se declaran `ugetpid()`, `vmprint()` y `pgaccess()`.
- `usys.pl`: se agregan los stubs de `vmprint` y `pgaccess`.

---

# Integración con xv6

1. Clonar xv6-riscv y hacer `git switch -c proyecto3 06aad25`.
2. Copiar los archivos entregados en sus rutas.
3. Agregar `$U/_pgtbltest` a UPROGS en el Makefile.
4. Compilar con `make qemu`.
5. Ejecutar `pgtbltest` y `usertests -q`.

---

# Respuestas a las preguntas

## 1. Interpretación de vmprint()

Salida de `vmprint()` del proceso de la prueba `vmprint` de `pgtbltest` (se omiten las líneas de los niveles intermedios). Las banderas se leen de los bits bajos de la PTE: V=bit 0, R=bit 1, W=bit 2, X=bit 3, U=bit 4, A=bit 6, D=bit 7.

```
 .. .. ..0x0000000000000000: pte 0x0000000021fc9c5b pa 0x0000000087f27000
 .. .. ..0x0000000000001000: pte 0x0000000021fc985b pa 0x0000000087f26000
 .. .. ..0x0000000000002000: pte 0x0000000021fc94d7 pa 0x0000000087f25000
 .. .. ..0x0000000000003000: pte 0x0000000021fd1407 pa 0x0000000087f45000
 .. .. ..0x0000000000004000: pte 0x0000000021fd08d7 pa 0x0000000087f42000
 .. .. ..0x0000000000008000: pte 0x000000002073b8d7 pa 0x0000000081cee000
 .. .. ..0x0000003fffffd000: pte 0x0000000021fc9013 pa 0x0000000087f24000
 .. .. ..0x0000003fffffe000: pte 0x0000000021fca4c7 pa 0x0000000087f29000
 .. .. ..0x0000003ffffff000: pte 0x000000002000184b pa 0x0000000080006000
```

| Dirección virtual | Banderas | Contenido | Permisos |
|---|---|---|---|
| `0x0000` y `0x1000` | `0x5b` = V R X U (+A) | Texto (código del programa) | Lectura y ejecución desde modo usuario; no se puede escribir, así el código no se modifica por accidente |
| `0x2000` | `0xd7` = V R W U (+A, D) | Datos y bss | Lectura y escritura desde modo usuario; no se ejecuta |
| `0x3000` | `0x07` = V R W | Página de guarda | Sin `PTE_U`: el modo usuario no puede acceder |
| `0x4000` | `0xd7` = V R W U (+A, D) | Pila | Lectura y escritura desde modo usuario |
| `0x8000` | `0xd7` = V R W U (+A, D) | Página del heap que la prueba tocó (de `sbrklazy()`) | Lectura y escritura desde modo usuario |
| `0x3fffffd000` | `0x13` = V R U | `USYSCALL` (página compartida con el pid) | El usuario solo puede leerla |
| `0x3fffffe000` | `0xc7` = V R W (+A, D) | Trapframe | Sin `PTE_U`: solo el kernel y el trampolín lo usan |
| `0x3ffffff000` | `0x4b` = V R X (+A) | Trampolín | Ejecutable pero sin `PTE_U`: solo se usa al entrar y salir del kernel |

Las páginas `0x5000`, `0x6000` y `0x7000` pertenecen a la región reservada con `sbrklazy()` que nadie tocó. No tienen PTE válida, por eso no aparecen.

## 2. Página de guarda

La página de guarda (`0x3000`) está mapeada y es válida, pero `exec()` le quita `PTE_U`. Así, cualquier acceso desde modo usuario produce un fallo de página y el kernel termina al proceso. Está justo debajo de la pila, que crece hacia direcciones menores.

Si un programa desborda la pila, la primera escritura que cae en la página de guarda provoca el fallo de página y el proceso es terminado. Sin esa página, la pila seguiría creciendo sobre la zona de datos y corrompería variables globales en silencio, sin ningún aviso.

## 3. Otras llamadas que se pueden acelerar

Se pueden acelerar las llamadas que solo leen información que el kernel ya tiene y que el proceso no necesita proteger ni modificar.

- `getpid()`: es el caso implementado. El pid no cambia mientras el proceso vive.
- `uptime()`: devuelve la variable global `ticks`. Se podría mapear en modo solo lectura una página compartida por todos los procesos, que el kernel actualice en cada interrupción del reloj.

No se pueden acelerar las que cambian estado del kernel, necesitan privilegios o pueden bloquear al proceso:

- `fork`, `exec`, `exit`, `wait`, `kill`: crean, reemplazan o destruyen procesos y modifican la tabla de procesos.
- `sbrk`: cambia `p->sz` y la tabla de páginas.
- `pause` (sleep): debe dormir al proceso y planificar otro.
- `open`, `close`, `read`, `write`, `pipe`, `dup`, `chdir`, `fstat`, `link`, `unlink`, `mkdir`, `mknod`, `sync`: usan la tabla de archivos, inodos, el disco o los locks del kernel.

## 4. Riesgo de USYSCALL con PTE_W

Si `USYSCALL` se mapeara con `PTE_W`, el proceso podría escribir en una página física que pertenece al kernel. Podría falsificar el pid que lee `ugetpid()` y, en general, cualquier dato que el kernel ponga ahí. En nuestra implementación el kernel nunca vuelve a leer el pid desde esa página (usa `p->pid`), por lo que el daño se limitaría al propio proceso. Pero si en el futuro el kernel o alguna librería confiaran en esos datos para tomar decisiones, un programa malicioso podría engañarlos. Por eso el kernel no debe dar escritura al usuario sobre memoria en la que confía, y se mapea solo con `PTE_R | PTE_U`.

Con `PTE_R` solamente, un intento de escritura causa un fallo de página y el kernel termina al proceso, que es lo que verifica la prueba `usyscall_solo_lectura`.

## 5. pgaccess() y páginas perezosas

Una página de `sbrklazy()` que no se ha tocado todavía no tiene memoria física ni una PTE válida. El bit `PTE_A` lo enciende el hardware cuando se accede a una página mapeada, así que mientras no haya mapeo no hay acceso que reportar y el resultado correcto es 0.

`pgaccess()` no debe asignarle memoria por tres razones: la gracia de `sbrklazy()` es diferir la asignación hasta el primer uso real; una consulta de diagnóstico no debería tener efectos secundarios ni gastar memoria; y si el propio kernel la asignara, el estado del proceso cambiaría por haber medido. Por eso se usa `walk()` con `alloc = 0`, que solo mira las tablas existentes y nunca crea nada.

---

# Resultados de las pruebas

**pgtbltest:** pasaron las 11 pruebas (`ugetpid_basico`, `ugetpid_hijos`, `ugetpid_exec`, `usyscall_solo_lectura`, `heap_no_invade`, `sin_fugas_usyscall`, `vmprint`, `pgaccess_basico`, `pgaccess_64`, `pgaccess_argumentos` y `pgaccess_perezoso`). La revisión automática del formato de `vmprint()` dio OK.

**usertests -q:** la suite completa pasó (`ALL TESTS PASSED`, 108 s), sin pánicos del kernel. Los mensajes `usertrap(): unexpected scause ...` que aparecen durante la ejecución son esperados: algunas pruebas provocan fallos de página a propósito.

La salida completa de ambas pruebas está en `pruebas/pgtbltest.txt` y `pruebas/usertests.txt`.

---

# Decisiones de diseño

- **La página se reserva antes de crear la tabla de páginas.** `proc_pagetable()` mapea `p->usyscall`, así que la página debe existir antes de llamarla. Si cualquier paso falla, se deshacen los mapeos anteriores y `freeproc()` libera lo que se alcanzó a reservar.
- **La página pertenece al proceso, no a una tabla de páginas.** `exec()` crea una tabla nueva y libera la vieja. Por eso `proc_freepagetable()` desmapea `USYSCALL` con `do_free = 0` y es `freeproc()` quien libera la página física, una sola vez.
- **La página se llena de ceros antes de escribir el pid.** El resto de la página es legible por el usuario, y así no se filtran datos viejos del kernel.
- **`vmprint()` sigue el modelo de `freewalk()`.** Una PTE sin bits R, W ni X apunta a otra tabla y se recorre; una con alguno de ellos es una hoja. La dirección virtual se reconstruye con `PXSHIFT(nivel)`, y solo se imprimen PTEs con `PTE_V`.
- **`pgaccess()` trabaja en dos pasadas.** Primero lee los bits y arma la máscara; solo después de que `copyout()` tiene éxito limpia `PTE_A`. Si el puntero de resultado es inválido, devuelve -1 sin haber borrado el historial de accesos.
- **`sfence_vma()` después de limpiar.** El hardware solo vuelve a encender `PTE_A` al cargar la traducción en el TLB. Si la entrada vieja sigue en el TLB con A encendido, un acceso posterior no se notaría en la siguiente consulta.
- **El rango se compara contra `PGROUNDUP(p->sz)`**, con una comparación que no se desborda ante direcciones enormes, porque una página parcialmente cubierta por `sz` sigue siendo parte del proceso.

---

# Limitaciones conocidas

- `sfence_vma()` solo vacía el TLB de la CPU que ejecuta la llamada. En un kernel real habría que avisar a las demás CPUs donde el proceso pudo haber corrido.
- Solo `getpid()` usa la página compartida; no se aceleran otras llamadas (fuera del alcance del proyecto).
- No se usa el bit `PTE_D` ni se implementan algoritmos de reemplazo de páginas (fuera del alcance).

---

# Declaración de uso de IA

Se usó Claude (Anthropic) como apoyo para comparar la versión de xv6 usada en el Proyecto 2 con el commit `06aad25`, configurar el entorno de desarrollo, proponer y revisar el código de las Partes A, B y C, y redactar un borrador de este README. Los integrantes revisaron el código, ejecutaron las pruebas en su propio equipo y asumen plena responsabilidad académica sobre el contenido entregado.
