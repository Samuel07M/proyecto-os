// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 nlibres;   // NUEVO: páginas en freelist (para cowstat)
} kmem;

//NUEVO:
// Conteo de referencias por página física.
// Hay una entrada por cada página entre KERNBASE y PHYSTOP. El índice
// de una página es su distancia a KERNBASE medida en páginas.
#define NPAGINAS ((PHYSTOP - KERNBASE) / PGSIZE)
#define PA2IDX(pa) (((uint64)(pa) - KERNBASE) / PGSIZE)

struct {
  struct spinlock lock; // protege refs[]; es distinto de kmem.lock
  int refs[NPAGINAS];
} kref;   //FIN NUEVO

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock, "kref");   //NUEVO
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {   //NUEVO: se agregaron llaves al for
    //NUEVO:
    // Estas páginas nunca pasaron por kalloc(): se les pone una
    // referencia para que el kfree() siguiente la baje a 0 y las
    // agregue a la lista libre.
    kref.refs[PA2IDX(p)] = 1;   //FIN NUEVO
    kfree(p);
  }   //NUEVO
}

//NUEVO:
// Revisa que pa sea una página física válida del asignador.
static void
revisar_pa(void *pa, char *quien)
{
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic(quien);
}   //FIN NUEVO

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// Quita una referencia a la página pa; la página vuelve a la lista
// libre solo cuando ya nadie la referencia.   //NUEVO
void
kfree(void *pa)
{
  struct run *r;
  int quedan;   //NUEVO

  revisar_pa(pa, "kfree");   //NUEVO: se reemplazo el if de validación por revisar_pa()

  //NUEVO:
  acquire(&kref.lock);
  if (kref.refs[PA2IDX(pa)] <= 0)
    panic("kfree: página sin referencias");
  quedan = --kref.refs[PA2IDX(pa)];
  release(&kref.lock);

  if (quedan > 0)
    return;   //FIN NUEVO

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.nlibres++;   //NUEVO
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// La página sale con una referencia.   //NUEVO
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {   //NUEVO: se agregaron llaves al if
    kmem.freelist = r->next;
    kmem.nlibres--;   //NUEVO
  }   //NUEVO
  release(&kmem.lock);

  if (r) {   //NUEVO: se agregaron llaves al if
    //NUEVO:
    acquire(&kref.lock);
    kref.refs[PA2IDX(r)] = 1;
    release(&kref.lock);   //FIN NUEVO
    memset((char *)r, 5, PGSIZE); // fill with junk
  }   //NUEVO
  return (void *)r;
}

//NUEVO:
// Agrega una referencia a la página pa (por ejemplo, cuando fork()
// la mapea también en el hijo).
void
krefinc(void *pa)
{
  revisar_pa(pa, "krefinc");
  acquire(&kref.lock);
  if (kref.refs[PA2IDX(pa)] <= 0)
    panic("krefinc: página libre");
  kref.refs[PA2IDX(pa)]++;
  release(&kref.lock);
}   //FIN NUEVO

//NUEVO:
// Devuelve cuántas referencias tiene la página pa.
int
krefcount(void *pa)
{
  int n;

  revisar_pa(pa, "krefcount");
  acquire(&kref.lock);
  n = kref.refs[PA2IDX(pa)];
  release(&kref.lock);
  return n;
}   //FIN NUEVO

//NUEVO:
// Número de páginas físicas libres en todo el sistema.
uint64
kfreepages(void)
{
  uint64 n;

  acquire(&kmem.lock);
  n = kmem.nlibres;
  release(&kmem.lock);
  return n;
}

// Número de páginas físicas con más de una referencia.
uint64
ksharedpages(void)
{
  uint64 n = 0;

  acquire(&kref.lock);
  for (int i = 0; i < NPAGINAS; i++)
    if (kref.refs[i] > 1)
      n++;
  release(&kref.lock);
  return n;
}   //FIN NUEVO
