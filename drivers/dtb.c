#include <mm/mmu.h>
#include <drivers/dtb.h>
#include <utils.h>

// manipulate device tree with dtb file format
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

extern char* cpio_start;
extern char* cpio_end;

char *dtb_base_address;

void traverse_device_tree(dtb_callback callback)
{
    struct fdt_header *header = (struct fdt_header *)dtb_base_address;
    if (endian_big2little(header->magic) != 0xD00DFEED)
    {
        uart_puts("traverse_device_tree : wrong magic in traverse_device_tree");
        return;
    }

    uint32_t struct_size = endian_big2little(header->size_dt_struct);
    char *dt_struct_ptr = (char *)((char *)header + endian_big2little(header->off_dt_struct));
    char *dt_strings_ptr = (char *)((char *)header + endian_big2little(header->off_dt_strings));

    char *end = (char *)dt_struct_ptr + struct_size;
    char *pointer = dt_struct_ptr;

    while (pointer < end)
    {
        uint32_t token_type = endian_big2little(*(uint32_t *)pointer);

        pointer += 4;
        if (token_type == FDT_BEGIN_NODE)
        {
            callback(token_type, pointer, 0, 0);
            pointer += strlen(pointer);
            pointer += 4 - (unsigned long long)pointer % 4; // alignment 4 byte
        }
        else if (token_type == FDT_END_NODE)
        {
            callback(token_type, 0, 0, 0);
        }
        else if (token_type == FDT_PROP)
        {
            uint32_t len = endian_big2little(*(uint32_t *)pointer);
            pointer += 4;
            char *name = (char *)dt_strings_ptr + endian_big2little(*(uint32_t *)pointer);
            pointer += 4;
            callback(token_type, name, pointer, len);
            pointer += len;
            if ((unsigned long long)pointer % 4 != 0)
                pointer += 4 - (unsigned long long)pointer % 4; // alignment 4 byte
        }
        else if (token_type == FDT_NOP)
        {
            callback(token_type, 0, 0, 0);
        }
        else if (token_type == FDT_END)
        {
            callback(token_type, 0, 0, 0);
        }
        else
        {
            uart_printf("error type:%x\n", token_type);
            return;
        }
    }
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0;
    if (node_type == FDT_BEGIN_NODE)
    {
        for (int i = 0; i < level; i++)
            uart_printf("   ");
        uart_printf("%s{\n", name);
        level++;
    }
    else if (node_type == FDT_END_NODE)
    {
        level--;
        for (int i = 0; i < level; i++)
            uart_printf("   ");
        uart_printf("}\n");
    }
    else if (node_type == FDT_PROP)
    {
        for (int i = 0; i < level; i++)
            uart_printf("   ");
        uart_printf("%s\n", name);
    }
}

void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
    if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0)
        cpio_start = (void *)PHYS_TO_VIRT((unsigned long long)endian_big2little(*(uint32_t *)value));

    if (node_type == FDT_PROP && strcmp(name, "linux,initrd-end") == 0)
        cpio_end = (void *)PHYS_TO_VIRT((unsigned long long)endian_big2little(*(uint32_t *)value));
}