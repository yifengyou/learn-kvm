
#define PAGE_SIZE 4096ul
#define LARGE_PAGE_SIZE (512 * PAGE_SIZE)

static void *free = 0;
static void *vfree_top = 0;

static unsigned long virt_to_phys(const void *virt) 
{ 
    return (unsigned long)virt;
}

static void *phys_to_virt(unsigned long phys)
{
    return (void *)phys;
}

void *memset(void *data, int c, unsigned long len)
{
    char *s = data;

    while (len--)
	*s++ = c;

    return data;
}

static void free_memory(void *mem, unsigned long size)
{
    while (size >= PAGE_SIZE) {
	*(void **)mem = free;
	free = mem;
	mem += PAGE_SIZE;
	size -= PAGE_SIZE;
    }
}

void *alloc_page()
{
    void *p;

    if (!free)
	return 0;
    
    p = free;
    free = *(void **)free;

    return p;
}

void free_page(void *page)
{
    *(void **)page = free;
    free = page;
}

extern char edata, end_of_memory;

#define PTE_PRESENT (1ull << 0)
#define PTE_PSE     (1ull << 7)
#define PTE_WRITE   (1ull << 1)
#define PTE_ADDR    (0xffffffffff000ull)

static void install_pte(unsigned long *cr3, 
			int pte_level, 
			void *virt,
			unsigned long pte)
{
    int level;
    unsigned long *pt = cr3;
    unsigned offset;

    for (level = 4; level > pte_level; --level) {
	offset = ((unsigned long)virt >> ((level-1) * 9 + 12)) & 511;
	if (!(pt[offset] & PTE_PRESENT)) {
	    unsigned long *new_pt = alloc_page();
	    memset(new_pt, 0, PAGE_SIZE);
	    pt[offset] = virt_to_phys(new_pt) | PTE_PRESENT | PTE_WRITE;
	}
	pt = phys_to_virt(pt[offset] & 0xffffffffff000ull);
    }
    offset = (((unsigned long)virt >> ((level-1) * 9) + 12)) & 511;
    pt[offset] = pte;
}

static unsigned long get_pte(unsigned long *cr3, void *virt)
{
    int level;
    unsigned long *pt = cr3, pte;
    unsigned offset;

    for (level = 4; level > 1; --level) {
	offset = ((unsigned long)virt >> ((level-1) * 9 + 12)) & 511;
	pte = pt[offset];
	if (!(pte & PTE_PRESENT))
	    return 0;
	if (level == 2 && (pte & PTE_PSE))
	    return pte;
	pt = phys_to_virt(pte & 0xffffffffff000ull);
    }
    offset = (((unsigned long)virt >> ((level-1) * 9) + 12)) & 511;
    pte = pt[offset];
    return pte;
}

static void install_large_page(unsigned long *cr3, 
			       unsigned long phys,
			       void *virt)
{
    install_pte(cr3, 2, virt, phys | PTE_PRESENT | PTE_WRITE | PTE_PSE);
}

static void install_page(unsigned long *cr3, 
			 unsigned long phys,
			 void *virt)
{
    install_pte(cr3, 1, virt, phys | PTE_PRESENT | PTE_WRITE);
}

static void load_cr3(unsigned long cr3)
{
    asm ( "mov %0, %%cr3" : : "r"(cr3) );
}

static unsigned long read_cr3()
{
    unsigned long cr3;

    asm volatile ( "mov %%cr3, %0" : "=r"(cr3) );
    return cr3;
}

static void load_cr0(unsigned long cr0)
{
    asm volatile ( "mov %0, %%cr0" : : "r"(cr0) );
}

static unsigned long read_cr0()
{
    unsigned long cr0;

    asm volatile ( "mov %%cr0, %0" : "=r"(cr0) );
    return cr0;
}

static void load_cr4(unsigned long cr4)
{
    asm volatile ( "mov %0, %%cr4" : : "r"(cr4) );
}

static unsigned long read_cr4()
{
    unsigned long cr4;

    asm volatile ( "mov %%cr4, %0" : "=r"(cr4) );
    return cr4;
}

struct gdt_table_descr
{
    unsigned short len;
    unsigned long *table;
} __attribute__((packed));

static void load_gdt(unsigned long *table, int nent)
{
    struct gdt_table_descr descr;

    descr.len = nent * 8 - 1;
    descr.table = table;
    asm volatile ( "lgdt %0" : : "m"(descr) );
}

#define SEG_CS_32 8
#define SEG_CS_64 16

struct ljmp {
    void *ofs;
    unsigned short seg;
};

static void setup_mmu(unsigned long len)
{
    unsigned long *cr3 = alloc_page();
    unsigned long phys = 0;

    memset(cr3, 0, PAGE_SIZE);
    while (phys + LARGE_PAGE_SIZE <= len) {
	install_large_page(cr3, phys, (void *)phys);
	phys += LARGE_PAGE_SIZE;
    }
    while (phys + PAGE_SIZE <= len) {
	install_page(cr3, phys, (void *)phys);
	phys += PAGE_SIZE;
    }
    
    load_cr3(virt_to_phys(cr3));
    print("paging enabled\n");
}

void setup_vm()
{
    free_memory(&edata, &end_of_memory - &edata);
    setup_mmu((long)&end_of_memory);
}

void *vmalloc(unsigned long size)
{
    void *mem, *p;
    unsigned pages;

    size += sizeof(unsigned long);
    
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    vfree_top -= size;
    mem = p = vfree_top;
    pages = size / PAGE_SIZE;
    while (pages--) {
	install_page(phys_to_virt(read_cr3()), virt_to_phys(alloc_page()), p);
	p += PAGE_SIZE;
    }
    *(unsigned long *)mem = size;
    mem += sizeof(unsigned long);
    return mem;
}

void *vfree(void *mem)
{
    unsigned long size = ((unsigned long *)mem)[-1];
    
    while (size) {
	free_page(phys_to_virt(get_pte(phys_to_virt(read_cr3()), mem) & PTE_ADDR));
	mem += PAGE_SIZE;
	size -= PAGE_SIZE;
    }
}
