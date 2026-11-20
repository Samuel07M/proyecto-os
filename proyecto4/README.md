# Proyecto de Sistemas Operativos
## Proyecto 4: Copy-on-Write y MiniDB (xv6)

### Integrantes

- Samuel Molina Garcés
- Juan Diego Parra Castañeda

### Curso

Sistemas Operativos

### Semestre

2026-2

---

# Descripción

Se partió de xv6-riscv en el commit `06aad25` con la solución de referencia del Proyecto 3 (`base_proyecto3.patch`), que ya trae `USYSCALL`, `ugetpid()`, `vmprint()` y `pgaccess()`.

**Parte A: Copy-on-Write en `fork()`.** `uvmcopy()` ya no copia la memoria del padre: mapea en el hijo las mismas páginas físicas y, si eran escribibles, les quita `PTE_W` y les pone `PTE_COW` (bit 8, reservado para el software) en las dos tablas de páginas. Cada página física tiene un conteo de referencias en `kalloc.c`, protegido con su propio spinlock; `kfree()` solo devuelve una página a la lista libre cuando su conteo llega a 0. La primera escritura sobre una página CoW provoca un page fault (`scause` = 15) que atiende `cowfault()`: si la página tiene más de una referencia, la copia en una página nueva; si ya tiene una sola, solo le devuelve `PTE_W`. `copyout()` usa la misma función antes de escribir en memoria de usuario. Si no hay memoria para la copia, el proceso muere sin pánico del kernel.

**Parte B: `cowstat()`.** Nueva llamada al sistema que llena un `struct cowstat` con las páginas libres del sistema, las páginas físicas con más de una referencia y los contadores `cowfaults` y `cowcopies` del proceso que llama (campos nuevos de `struct proc`, que empiezan en 0 en cada proceso nuevo).

**Parte C: MiniDB.** Se completó `minidb snapshot M`: el padre calcula el checksum y el saldo total de la tabla, limpia los bits de acceso con `pgaccess()` y hace `fork()`. El hijo escribe `snap.db` directamente desde la tabla, que ve congelada en el instante del `fork()`, mientras el padre aplica `M` transferencias sin esperarlo. Al final el padre reporta las páginas que tocó y los fallos y copias CoW que tuvo, espera al hijo, relee `snap.db` y verifica que tiene el checksum de antes del `fork()` y que el saldo total se conservó. `minidb consulta K` y `minidb mapa` son del esqueleto del docente y dan los resultados esperados con nuestro kernel.

---

# Archivos modificados

Todo el código agregado o modificado está marcado con comentarios `NUEVO` (una línea) o `NUEVO:` ... `FIN NUEVO` (bloques).

**kernel/**

- `riscv.h`: se define `PTE_COW` como el bit 8 de la PTE (uno de los dos bits RSW que RISC-V deja para el software).
- `kalloc.c`: se agrega el arreglo `kref.refs[]`, con una entrada por cada página entre `KERNBASE` y `PHYSTOP`, y su spinlock `kref.lock`. `kalloc()` deja la página con 1 referencia; `kfree()` quita una y solo la devuelve a la lista libre cuando llega a 0; `freerange()` pone 1 antes de llamar a `kfree()`, porque esas páginas nunca pasaron por `kalloc()`. Funciones nuevas: `krefinc()`, `krefcount()`, `kfreepages()` (con un contador `kmem.nlibres` que se mantiene bajo `kmem.lock`) y `ksharedpages()`.
- `vm.c`: `uvmcopy()` comparte las páginas en vez de copiarlas; `copyout()` llama a `cowfault()` cuando la página destino no tiene `PTE_W`; se agrega `cowfault()`, que también actualiza los contadores del proceso.
- `trap.c`: `usertrap()` atiende con `cowfault()` las escrituras (`scause` = 15) sobre páginas CoW. Si `cowfault()` falla (la página no es CoW o no hay memoria), el proceso cae en el caso de error y se mata con `setkilled()`.
- `cowstat.h` (nuevo): `struct cowstat`.
- `proc.h`: campos `cowfaults` y `cowcopies` en `struct proc`.
- `proc.c`: `allocproc()` pone los dos contadores en 0.
- `sysproc.c`: se agrega `sys_cowstat()`.
- `syscall.h`, `syscall.c`: se registra `SYS_cowstat` (25).
- `defs.h`: se declaran `krefinc()`, `krefcount()`, `kfreepages()`, `ksharedpages()` y `cowfault()`.

**user/**

- `minidb.c`: se implementan `snapshot()` y `guardar_snapshot()`, con las auxiliares `saldo_total()`, `contar_bits()` y `NPAG_TABLA`.
- `user.h`: se declara `int cowstat(struct cowstat *);` (con `struct cowstat;` antes).
- `usys.pl`: se agrega el stub de `cowstat`.

**Makefile**: se agregan `$U/_pgtbltest`, `$U/_cowtest` y `$U/_minidb` a `UPROGS`.

---

# Integración con xv6

1. Clonar xv6-riscv, hacer `git switch -c proyecto4 06aad25` y
   aplicar `git apply base_proyecto3.patch`.
2. Copiar los archivos entregados en sus rutas.
3. Copiar los archivos del docente: user/cowtest.c, user/minidb.h,
   user/pgtbltest.c y user/usertests.c.
4. Compilar con `make qemu CPUS=3`.
5. Ejecutar `cowtest`, `minidb consulta 3`, `minidb snapshot 2000`,
   `pgtbltest` y `usertests -q`.

---

# Respuestas a las preguntas

## 1. PTE antes y después de una escritura CoW

Salida de `minidb mapa` (solo las PTEs de las dos páginas de la tabla). La tabla quedó en `0x5000`:

```
mapa: hijo antes de escribir
 .. .. ..0x0000000000005000: pte 0x0000000021fbcdd3 pa 0x0000000087ef3000
 .. .. ..0x0000000000006000: pte 0x0000000021fc21d3 pa 0x0000000087f08000
mapa: hijo después de escribir en 0x5000
 .. .. ..0x0000000000005000: pte 0x0000000021fc10d7 pa 0x0000000087f04000
 .. .. ..0x0000000000006000: pte 0x0000000021fc21d3 pa 0x0000000087f08000
```

Los 10 bits bajos de la PTE son las banderas; el resto es el número de página física.

| PTE de `0x5000` | Bits bajos | V | R | W | X | U | A | D | COW (bit 8) |
|---|---|---|---|---|---|---|---|---|---|
| Antes (`...cdd3`) | `0x1d3` | 1 | 1 | **0** | 0 | 1 | 1 | 1 | **1** |
| Después (`...10d7`) | `0x0d7` | 1 | 1 | **1** | 0 | 1 | 1 | 1 | **0** |

Antes de escribir, la página está compartida con el padre: `uvmcopy()` le quitó `PTE_W` y le puso `PTE_COW` en las dos tablas, y las dos apuntan a la misma página física (`0x87ef3000`), que tiene 2 referencias. A y D vienen encendidos porque el padre ya había escrito en ella antes del `fork()` y `uvmcopy()` conserva las banderas.

Cuando el hijo escribe, el hardware no encuentra `PTE_W` y genera un page fault de escritura (`scause` = 15). `cowfault()` ve `PTE_COW` y 2 referencias, así que reserva una página nueva, copia el contenido y mapea en el hijo la página nueva (`0x87f04000`) con `PTE_W` y sin `PTE_COW`. Por eso cambia la dirección física. Luego llama a `kfree()` sobre la página vieja, que baja a 1 referencia. La página `0x6000` no se tocó y sigue compartida, con los mismos bits.

En el padre la PTE no cambia: sigue apuntando a `0x87ef3000`, sin `PTE_W` y con `PTE_COW`. Si el padre escribiera después, `cowfault()` vería que esa página ya tiene una sola referencia y no copiaría nada: solo le devolvería `PTE_W` y le quitaría `PTE_COW`. En `minidb mapa` el padre no escribe; al terminar, cada `kfree()` baja el conteo y las páginas vuelven a la lista libre cuando llega a 0.

## 2. Páginas sin PTE_W y USYSCALL en fork()

`uvmcopy()` recorre las páginas de usuario de `0` a `p->sz` y las comparte todas: mapea la misma página física en el hijo y llama a `krefinc()`. Solo cambia los permisos de las que tienen `PTE_W`.

- **Código (texto):** tiene `R X U` y no `PTE_W`. Se comparte tal cual, sin `PTE_COW`. Si un proceso intenta escribir en su código, `cowfault()` ve que la PTE no tiene `PTE_COW` y devuelve -1, así que `usertrap()` mata al proceso. Si se marcara como CoW, la escritura se atendería como una copia y el proceso podría modificar su propio código después de un `fork()`.
- **Página de guarda de la pila:** `exec()` le quita `PTE_U`, pero conserva `PTE_W`. `uvmcopy()` la trata como cualquier página escribible (le quita `PTE_W` y le pone `PTE_COW`), pero sigue siendo inaccesible: sin `PTE_U`, cualquier acceso desde modo usuario falla en el hardware, y `cowfault()` rechaza las páginas sin `PTE_U`, así que el proceso muere igual que antes. `copyout()` tampoco puede escribir en ella, porque `walkaddr()` rechaza las PTEs sin `PTE_U`.
- **`USYSCALL` y trapframe:** están encima de `p->sz` (justo debajo de `TRAMPOLINE`), así que `uvmcopy()` nunca los recorre. Cada proceso tiene los suyos: `allocproc()` reserva para el hijo una página `usyscall` nueva con su propio pid y un trapframe nuevo, y `proc_pagetable()` los mapea en la tabla del hijo. `USYSCALL` además nunca tiene `PTE_W`, así que una escritura sobre ella mata al proceso (lo verifica `cowtest solo_lectura`).

Si `USYSCALL` se compartiera, el hijo vería la página física del padre y `ugetpid()` le devolvería el pid del padre. Como la página es de solo lectura, el hijo nunca provocaría una copia, así que el error duraría toda la vida del hijo.

## 3. Carrera en el conteo de referencias

Supongamos que la página física P tiene 2 referencias: el padre A y su hijo B la comparten como CoW. Sin lock, `kfree()` y `krefinc()` hacen "leer, sumar o restar, escribir" en pasos separados:

| Paso | CPU 0: B hace `exit()` → `kfree(P)` | CPU 1: A hace `fork()` → `krefinc(P)` para el hijo C | `refs[P]` en memoria |
|---|---|---|---|
| 1 | lee `refs[P]` = 2 | | 2 |
| 2 | | lee `refs[P]` = 2 | 2 |
| 3 | escribe 2 − 1 = 1 | | 1 |
| 4 | | escribe 2 + 1 = 3 | 3 |

Al final la usan A y C, o sea 2 procesos, pero el conteo dice 3. Cuando A y C terminen, el conteo quedará en 1 y nunca llegará a 0: **la página se pierde para siempre** (fuga).

Si los pasos 3 y 4 se invierten (CPU 1 escribe 3 y después CPU 0 escribe 1), el conteo queda en 1 con dos procesos usando la página. Cuando C termine, `kfree()` la baja a 0 y **la devuelve a la lista libre mientras A todavía la usa**. Otro `kalloc()` puede entregarla a un tercer proceso, y A y ese proceso escribirían en la misma memoria física.

Con `kref.lock`, cada operación (`kalloc()`, `kfree()`, `krefinc()`, `krefcount()`) hace su lectura y su escritura con el lock tomado, así que las dos actualizaciones no se pueden intercalar. Además, `kfree()` decide dentro del lock si el conteo llegó a 0, así que solo una CPU puede ver ese 0 y liberar la página. `kref.lock` se suelta antes de tomar `kmem.lock`, así que nunca se tienen los dos a la vez y no puede haber deadlock.

## 4. Consistencia del snapshot

Salida de `minidb snapshot 2000`:

```
snapshot: checksum_inicial=0x6915D295
snapshot: hijo pid=4 escribiendo snap.db
snapshot: transferencias=2000 aplicadas=1978 rechazadas=22
snapshot: paginas_tocadas=30 fallos_cow=30 copias_cow=30
snapshot: archivo bytes=262160 checksum=0x6915D295 coincide=si
snapshot: checksum_actual=0x4AB3E209 total_conservado=si
snapshot: OK
```

**¿Por qué `snap.db` tiene el checksum de antes del fork?** Después del `fork()`, las 64 páginas de la tabla están compartidas y marcadas CoW en las dos tablas de páginas. Cuando el padre escribe una cuenta, no escribe en la página compartida: el page fault hace que `cowfault()` le dé al padre una copia propia, y la escritura cae en la copia. La página original, la que tiene mapeada el hijo, no cambia. Así el hijo siempre lee la tabla exactamente como estaba en el `fork()`, aunque el padre la esté modificando al mismo tiempo en otra CPU, y escribe en el archivo la misma tabla sobre la que se calculó `checksum_inicial`. No hace falta ningún lock ni copiar la tabla a otro buffer.

**¿Por qué `copias_cow` es menor que 64?** Solo se copian las páginas que el padre escribe, y las transferencias se concentran: 255 de cada 256 ocurren entre las primeras 1024 cuentas (`NACTIVAS`), que ocupan las primeras 8 páginas de la tabla. Solo unas 8 de las 2000 transferencias (2000/256) tocan cuentas en el resto de la tabla. Por eso el padre tocó solo 30 de las 64 páginas, y cada página tocada se copió una sola vez: el primer fallo la copia, y las escrituras siguientes ya caen en la copia propia del padre, que tiene `PTE_W`.

**¿Qué pasaría si el padre esperara al hijo antes de las transferencias?** Cuando el hijo termina, `kfree()` baja a 1 el conteo de todas las páginas de la tabla. Las PTEs del padre siguen sin `PTE_W` y con `PTE_COW`, así que la primera escritura en cada página sigue siendo un fallo CoW (`fallos_cow` seguiría en 30). Pero `cowfault()` vería una sola referencia y solo devolvería `PTE_W`, sin copiar: `copias_cow` sería 0. El precio sería que la cooperativa dejaría de aplicar transferencias mientras se escribe el snapshot, que es justo lo que se quería evitar.

## 5. Aceleración y planificador

Ticks de `minidb consulta K` (promedio de 3 ejecuciones, en un computador de 4 núcleos; 1 tick ≈ 100 ms):

| K | CPUS=1 | CPUS=3 |
|---|---|---|
| 1 | 84,3 | 78,3 |
| 2 | 81,3 | 36,7 |
| 3 | 82,3 | 29,3 |
| 4 | 81,0 | 28,0 |
| 8 | 83,0 | 30,0 |

**De 1 a 3 trabajadores con `CPUS=3` (78,3 → 29,3 ticks, 2,67x).** Cada trabajador es un proceso aparte y el trabajo costoso (la firma de 8.192 cuentas) se reparte en partes iguales. El planificador de xv6 corre en cada CPU un ciclo que recorre la tabla de procesos y toma el siguiente proceso `RUNNABLE`, así que con 3 trabajadores y 3 CPUs cada uno queda en una CPU distinta y los tres avanzan al mismo tiempo: cada uno hace un tercio del trabajo y el tiempo baja casi a un tercio. No llega exactamente a 3x porque hay una parte que no se reparte (crear los procesos con `fork()`, los fallos CoW de la pila, los pipes y los `wait()`), porque los ticks tienen una resolución de 100 ms y porque las 3 CPUs de QEMU son hilos del computador real, que también corre otras cosas. Con 2 trabajadores el tiempo baja a la mitad (36,7).

**De 3 a 8 trabajadores no mejora (29,3 → 30,0).** Solo hay 3 CPUs, así que como mucho 3 procesos corren a la vez. Con 8 trabajadores hay 8 procesos `RUNNABLE` y el planificador Round Robin los turna: cada interrupción del reloj (cada tick) el proceso que corre llama a `yield()` y la CPU pasa al siguiente. El trabajo total sigue siendo el mismo (8.192 firmas) y se sigue haciendo en 3 CPUs, así que el tiempo es el mismo. Si acaso empeora un poco, por los 5 `fork()` extra y los cambios de contexto.

**Con `CPUS=1` no mejora con ningún K (≈ 82 ticks).** Con una sola CPU los trabajadores son concurrentes pero no paralelos: Round Robin los turna en la misma CPU, uno a la vez, y cada tick solo uno avanza. La CPU hace el mismo trabajo total sin importar en cuántos procesos se divida, así que el tiempo no cambia. Tampoco empeora de forma notable, porque un cambio de contexto cada 100 ms cuesta muy poco frente al cálculo de la firma.

---

# Resultados de las pruebas

**cowtest:** pasaron las 11 pruebas (`fork_grande`, `aislamiento`, `tres_niveles`, `copyout_cow`, `perezosa_cow`, `ugetpid_hijo`, `solo_lectura`, `contadores`, `sin_fugas`, `sin_memoria` y `concurrente`).

**minidb:** `minidb consulta` con 1, 2, 3 y 8 trabajadores da `consulta: OK`, con las ocho sucursales y la firma `0x2DD2F610` esperadas. `minidb snapshot 2000` y `minidb snapshot 0` dan `snapshot: OK` con los checksums esperados (`0x6915D295` y `0x4AB3E209`) y `minidb mapa` muestra la PTE con `PTE_COW` y sin `PTE_W` antes de escribir, y con `PTE_W`, sin `PTE_COW` y en otra dirección física después. La revisión automática del script dio OK.

**pgtbltest:** pasaron las 11 pruebas del Proyecto 3.

**usertests -q:** `ALL TESTS PASSED`, sin pánicos del kernel.

Los mensajes `usertrap(): unexpected scause 0xf ...` que aparecen durante las pruebas son esperados: algunas pruebas escriben en el código o en `USYSCALL`, o agotan la memoria a propósito. La salida completa de las cuatro suites está en `pruebas/`.

---

# Decisiones de diseño

- **Conteo indexado por la distancia a `KERNBASE`.** El arreglo tiene `(PHYSTOP - KERNBASE) / PGSIZE` entradas (32.768 enteros, 128 KB), una por página física posible. Incluye las páginas del kernel, que nunca se usan, pero así el índice es una resta y una división, sin búsquedas.
- **Un solo spinlock para todo el arreglo**, distinto de `kmem.lock`. Las secciones críticas son de pocas instrucciones, así que un lock por página no compensaría su costo en memoria. Nunca se tienen `kref.lock` y `kmem.lock` a la vez.
- **`kfree()` decide dentro del lock si la página queda libre**, y solo después llena la página de basura y la pone en la lista libre. Así dos CPUs nunca pueden liberar la misma página.
- **`cowfault()` se comparte entre `usertrap()` y `copyout()`**, para que los dos caminos sigan las mismas reglas y los contadores de `cowstat()` cuenten los dos tipos de fallo.
- **Sin copia con una sola referencia.** Si nadie más usa la página, `cowfault()` solo le devuelve `PTE_W`. Esto es seguro sin mantener el lock entre la consulta y el cambio de la PTE: el conteo de una página que solo usa este proceso solo puede subir por un `fork()` del mismo proceso, y un proceso no puede estar a la vez en `fork()` y atendiendo un page fault.
- **`uvmcopy()` agrega la referencia después de mapear.** Si `mappages()` falla, `uvmunmap(..., 1)` hace `kfree()` solo sobre las páginas que sí alcanzaron a recibir su `krefinc()`.
- **Sin vaciado explícito del TLB.** El padre pierde `PTE_W` dentro de `fork()`, pero el trampolín ejecuta `sfence.vma` antes de volver a modo usuario, así que la traducción vieja con `PTE_W` no llega a usarse. Lo mismo pasa con la PTE que cambia `cowfault()`.
- **`kfreepages()` usa un contador** que se actualiza al meter y sacar páginas de la lista libre, en vez de recorrer la lista en cada llamada.
- **MiniDB mide `cowstat()` después del `printf` del padre**, para que `fallos_cow` cuente solo las páginas de la tabla y no la primera escritura en la pila del padre después del `fork()`.

---

# Limitaciones conocidas

- `cowfault()` atiende el fallo en la CPU que lo recibe. En un kernel real con hilos que comparten tabla de páginas, habría que vaciar el TLB de las otras CPUs al cambiar una PTE.
- La página de guarda de la pila queda marcada como CoW después del `fork()`. No tiene efecto (sin `PTE_U` nadie la puede usar desde modo usuario), pero aparece con el bit 8 en `vmprint()`.
- `ksharedpages()` recorre las 32.768 entradas del conteo con el lock tomado. Es suficiente para `cowstat()`, pero no sería apropiado llamarla con mucha frecuencia.
- No se comparten las páginas de las tablas de páginas ni se implementa CoW fuera de `fork()` (fuera del alcance).

---

# Declaración de uso de IA

Se usó Claude (Anthropic) como apoyo para revisar el enunciado y planear el trabajo, proponer y revisar el código de las Partes A, B y C.2, preparar un script para medir los experimentos y redactar un borrador de este README y del reporte técnico. Los integrantes revisaron el código, ejecutaron las pruebas y los experimentos en su propio equipo y asumen plena responsabilidad académica sobre el contenido entregado.
