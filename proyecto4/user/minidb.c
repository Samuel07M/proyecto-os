// minidb: base de datos de cuentas en memoria (Proyecto 4).
// Sistemas Operativos 2026-2, Universidad EAFIT.
//
// ESQUELETO entregado por el docente. Ya están hechos:
//   minidb consulta K     reporte por sucursal con K procesos trabajadores
//   minidb mapa           vmprint() de un hijo antes y después de escribir
// El grupo debe implementar (busquen "TODO"):
//   minidb snapshot M     guarda snap.db en un hijo mientras el padre
//                         aplica M transferencias (sección C.2 del enunciado)
//
// Estudien consulta(): es el modelo de multiprogramación del proyecto
// (varios hijos con fork(), un pipe por hijo, el padre combina).
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"   //NUEVO
#include "kernel/cowstat.h"
#include "user/user.h"
#include "user/minidb.h"

static struct cuenta *tabla;

static void
morir(char *msg)
{
  printf("minidb: error: %s\n", msg);
  exit(1);
}

static void
cargar_tabla(void)
{
  if((tabla = reservar_tabla()) == 0)
    morir("no hay memoria para la tabla");
  for(int i = 0; i < NCUENTAS; i++)
    generar_cuenta(i, &tabla[i]);
}

static uint64
libres(void)
{
  struct cowstat st;
  if(cowstat(&st) < 0)
    morir("cowstat() falló");
  return st.freepages;
}

static void
agregar(struct resultado *r, int desde, int hasta, int con_firma)
{
  memset(r, 0, sizeof(*r));
  r->desde = desde;
  r->hasta = hasta;
  for(int i = desde; i < hasta; i++){
    struct cuenta *c = &tabla[i];
    struct agregado *a = &r->suc[c->sucursal];
    a->cuentas++;
    a->saldo_total += c->saldo;
    if(c->saldo > a->saldo_max)
      a->saldo_max = c->saldo;
    if(con_firma)
      a->firmas += firma(c);
  }
}

static int
leer_todo(int fd, void *buf, int n)
{
  int total = 0;
  while(total < n){
    int r = read(fd, (char *)buf + total, n - total);
    if(r <= 0)
      return -1;
    total += r;
  }
  return total;
}

static int
escribir_todo(int fd, void *buf, int n)
{
  int total = 0;
  while(total < n){
    int r = write(fd, (char *)buf + total, n - total);
    if(r <= 0)
      return -1;
    total += r;
  }
  return total;
}

static void
consulta(int k)
{
  if(k < 1 || k > MAXWORKERS)
    morir("K debe estar entre 1 y 8");
  cargar_tabla();
  printf("consulta: trabajadores=%d\n", k);

  int fds[MAXWORKERS][2];
  uint64 libres0 = libres();
  int t0 = uptime();
  for(int w = 0; w < k; w++){
    if(pipe(fds[w]) < 0)
      morir("pipe() falló");
    int pid = fork();
    if(pid < 0)
      morir("fork() falló");
    if(pid == 0){
      close(fds[w][0]);
      struct resultado r;
      agregar(&r, w * NCUENTAS / k, (w + 1) * NCUENTAS / k, 1);
      r.pid = ugetpid();
      if(escribir_todo(fds[w][1], &r, sizeof(r)) < 0)
        exit(1);
      exit(0);
    }
    close(fds[w][1]);
  }
  uint64 libres1 = libres();

  struct agregado total[NSUCURSALES];
  memset(total, 0, sizeof(total));
  int ok = 1, cubiertas = 0;
  for(int w = 0; w < k; w++){
    struct resultado r;
    if(leer_todo(fds[w][0], &r, sizeof(r)) < 0)
      morir("un trabajador no envió su resultado");
    close(fds[w][0]);
    printf("trabajador pid=%d cuentas=[%d,%d)\n", r.pid, r.desde, r.hasta);
    if(r.desde != w * NCUENTAS / k || r.hasta != (w + 1) * NCUENTAS / k)
      ok = 0;
    cubiertas += r.hasta - r.desde;
    for(int s = 0; s < NSUCURSALES; s++){
      total[s].cuentas += r.suc[s].cuentas;
      total[s].saldo_total += r.suc[s].saldo_total;
      total[s].firmas += r.suc[s].firmas;
      if(r.suc[s].saldo_max > total[s].saldo_max)
        total[s].saldo_max = r.suc[s].saldo_max;
    }
  }
  for(int w = 0; w < k; w++){
    int estado;
    if(wait(&estado) < 0 || estado != 0)
      ok = 0;
  }
  int t1 = uptime();

  // Verificación secuencial (sin la firma, que es la parte costosa).
  struct resultado sec;
  agregar(&sec, 0, NCUENTAS, 0);
  uint firmas = 0;
  for(int s = 0; s < NSUCURSALES; s++){
    printf("sucursal %d: cuentas=%d total=%ld max=%d\n", s,
           total[s].cuentas, total[s].saldo_total, total[s].saldo_max);
    if(total[s].cuentas != sec.suc[s].cuentas ||
       total[s].saldo_total != sec.suc[s].saldo_total ||
       total[s].saldo_max != sec.suc[s].saldo_max)
      ok = 0;
    firmas += total[s].firmas;
  }
  if(cubiertas != NCUENTAS)
    ok = 0;
  printf("consulta: firma=0x%x\n", firmas);
  printf("consulta: paginas_libres antes=%ld con_trabajadores=%ld\n", libres0, libres1);
  printf("consulta: ticks=%d\n", t1 - t0);
  printf("consulta: %s\n", ok ? "OK" : "FALLO");
  exit(ok ? 0 : 1);
}

//NUEVO:
// Número de páginas de la tabla (8192 cuentas x 32 bytes = 64 páginas).
#define NPAG_TABLA (NCUENTAS * sizeof(struct cuenta) / PGSIZE)

// Suma de los saldos de toda la tabla.
static uint64
saldo_total(void)
{
  uint64 total = 0;
  for(int i = 0; i < NCUENTAS; i++)
    total += tabla[i].saldo;
  return total;
}

// Cuántos bits están en 1 en mask (páginas reportadas por pgaccess()).
static int
contar_bits(uint64 mask)
{
  int n = 0;
  for(; mask; mask >>= 1)
    n += mask & 1;
  return n;
}   //FIN NUEVO

// El hijo escribe en SNAP_ARCHIVO un struct snap_encabezado (con el
// checksum calculado ANTES del fork) seguido de las NCUENTAS cuentas,
// escritas directamente desde la tabla (sin copiarlas a otro buffer).
// Termina con exit(0) si todo salió bien.
static void
guardar_snapshot(uint checksum)
{
  //NUEVO:
  struct snap_encabezado enc;
  enc.magia = SNAP_MAGIA;
  enc.n = NCUENTAS;
  enc.checksum = checksum;
  enc.reservado = 0;

  int fd = open(SNAP_ARCHIVO, O_CREATE | O_WRONLY | O_TRUNC);
  if(fd < 0)
    exit(1);
  // El hijo solo lee la tabla: gracias a CoW ve la tabla congelada en el
  // momento del fork() sin que se copie ninguna de sus páginas.
  if(escribir_todo(fd, &enc, sizeof(enc)) < 0 ||
     escribir_todo(fd, tabla, NCUENTAS * sizeof(struct cuenta)) < 0){
    close(fd);
    exit(1);
  }
  close(fd);
  exit(0);   //FIN NUEVO
}

// Snapshot consistente: el hijo guarda la tabla en disco mientras el padre
// sigue aplicando transferencias. El formato exacto de cada línea está en
// el Anexo B del enunciado.
static void
snapshot(int m)
{
  cargar_tabla();
  //NUEVO:
  // 1. Checksum y saldo total de la tabla antes del fork.
  uint checksum0 = checksum_tabla(tabla, NCUENTAS);
  uint64 total0 = saldo_total();
  printf("snapshot: checksum_inicial=0x%x\n", checksum0);

  // 2. Limpiar los bits PTE_A de la tabla (el checksum la acaba de leer).
  uint64 mask;
  if(pgaccess(tabla, NPAG_TABLA, &mask) < 0)
    morir("pgaccess() falló");

  // 3. fork(): el hijo escribe snap.db; el padre no lo espera todavía.
  int pid = fork();
  if(pid < 0)
    morir("fork() falló");
  if(pid == 0)
    guardar_snapshot(checksum0);
  printf("snapshot: hijo pid=%d escribiendo %s\n", pid, SNAP_ARCHIVO);

  // 4. El padre aplica las m transferencias mientras el hijo escribe.
  // cowstat() se mide aquí, después del printf, para no contar los
  // fallos CoW de la pila del padre sino solo los de la tabla.
  struct cowstat st0, st1;
  if(cowstat(&st0) < 0)
    morir("cowstat() falló");
  uint semilla = 4;
  int aplicadas = 0, rechazadas = 0;
  for(int i = 0; i < m; i++){
    struct transferencia t;
    generar_transferencia(&semilla, &t);
    if(t.origen == t.destino || tabla[t.origen].saldo < t.monto){
      rechazadas++;
      continue;
    }
    tabla[t.origen].saldo -= t.monto;
    tabla[t.destino].saldo += t.monto;
    tabla[t.origen].nmov++;
    tabla[t.destino].nmov++;
    aplicadas++;
  }
  if(cowstat(&st1) < 0)
    morir("cowstat() falló");

  // 5. Páginas de la tabla que tocó el padre durante las transferencias.
  if(pgaccess(tabla, NPAG_TABLA, &mask) < 0)
    morir("pgaccess() falló");
  printf("snapshot: transferencias=%d aplicadas=%d rechazadas=%d\n",
         m, aplicadas, rechazadas);
  printf("snapshot: paginas_tocadas=%d fallos_cow=%ld copias_cow=%ld\n",
         contar_bits(mask), st1.cowfaults - st0.cowfaults,
         st1.cowcopies - st0.cowcopies);

  // 6. Esperar al hijo y verificar el archivo.
  int ok = 1, estado;
  if(wait(&estado) != pid || estado != 0){
    printf("snapshot: el hijo no pudo escribir %s\n", SNAP_ARCHIVO);
    ok = 0;
  }

  int fd = open(SNAP_ARCHIVO, O_RDONLY);
  if(fd < 0)
    morir("no se pudo abrir snap.db");
  struct stat info;
  if(fstat(fd, &info) < 0)
    morir("fstat() falló");
  // La pila es de una sola página: el buffer de 256 KB se pide con sbrk().
  struct snap_encabezado enc;
  struct cuenta *copia = (struct cuenta *)sbrk(NCUENTAS * sizeof(struct cuenta));
  if(copia == (struct cuenta *)-1)
    morir("sbrk() falló");
  uint checksum_archivo = 0;
  if(leer_todo(fd, &enc, sizeof(enc)) < 0 ||
     leer_todo(fd, copia, NCUENTAS * sizeof(struct cuenta)) < 0)
    ok = 0;
  else
    checksum_archivo = checksum_tabla(copia, NCUENTAS);
  close(fd);

  int coincide = ok && enc.magia == SNAP_MAGIA && enc.n == NCUENTAS &&
                 enc.reservado == 0 && enc.checksum == checksum0 &&
                 checksum_archivo == checksum0 &&
                 info.size == sizeof(enc) + NCUENTAS * sizeof(struct cuenta);
  printf("snapshot: archivo bytes=%ld checksum=0x%x coincide=%s\n",
         info.size, checksum_archivo, coincide ? "si" : "no");

  // El saldo total no cambia con las transferencias (solo se mueve).
  int conservado = saldo_total() == total0;
  printf("snapshot: checksum_actual=0x%x total_conservado=%s\n",
         checksum_tabla(tabla, NCUENTAS), conservado ? "si" : "no");

  ok = ok && coincide && conservado;
  printf("snapshot: %s\n", ok ? "OK" : "FALLO");
  exit(ok ? 0 : 1);   //FIN NUEVO
}

static void
mapa(void)
{
  // Una tabla pequeña para que la salida de vmprint() sea corta.
  char *t = sbrk(2 * PGSIZE);
  if(t == (char *)-1)
    morir("sbrk() falló");
  t[0] = t[PGSIZE] = 1;
  printf("mapa: tabla en 0x%lx (2 páginas)\n", (uint64)t);
  int pid = fork();
  if(pid < 0)
    morir("fork() falló");
  if(pid == 0){
    printf("mapa: hijo antes de escribir\n");
    vmprint();
    t[0] = 2;
    printf("mapa: hijo después de escribir en 0x%lx\n", (uint64)t);
    vmprint();
    exit(0);
  }
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[])
{
  if(argc >= 2 && strcmp(argv[1], "consulta") == 0)
    consulta(argc >= 3 ? atoi(argv[2]) : 1);
  if(argc >= 2 && strcmp(argv[1], "snapshot") == 0)
    snapshot(argc >= 3 ? atoi(argv[2]) : 2000);
  if(argc >= 2 && strcmp(argv[1], "mapa") == 0)
    mapa();
  printf("uso: minidb consulta K | minidb snapshot M | minidb mapa\n");
  exit(1);
}
