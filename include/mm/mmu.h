#ifndef MMU_H
#define MMU_H

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

/* Page descriptor attributes */
#define PD_TABLE 0b11L
#define PD_BLOCK 0b01L
#define PD_PAGE  0b11L
#define PD_UXN (1UL << 54)
#define PD_KXN (1UL << 53)
#define PD_ACCESS (1UL << 10)
#define PD_RDONLY    (1UL << 7)
#define PD_UK_ACCESS (1UL << 6)

/* Page table attributes */
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR PD_TABLE | PD_ACCESS
#define BOOT_PMD_DEVICE_ATTR (PD_KXN | PD_UXN | PD_ACCESS | PD_UK_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_PMD_MEMORY_ATTR (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)

/* Page table macros */
#define PGD_INDEX(x) (((x) >> 39) & 0x1ff) // PGD index
#define PUD_INDEX(x) (((x) >> 30) & 0x1ff) // PUD index
#define PMD_INDEX(x) (((x) >> 21) & 0x1ff) // PMD index
#define PTE_INDEX(x) (((x) >> 12) & 0x1ff) // PTE index
#define PAGE_OFFSET(x) ((x) & 0xfff)       // offset within page

/* Page permissions */
#define USER_TABLE (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE) 
#define USER_PAGE (PD_ACCESS | PD_UK_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE) 
#define PROT_NONE 0
#define PROT_READ (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC (1 << 2)

#define PHYS_TO_VIRT(x) ((x) + 0xffff000000000000)
#define VIRT_TO_PHYS(x) ((x) - 0xffff000000000000)
#define PD_ADDR_MASK 0x0000fffffffff000

#endif
