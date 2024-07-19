#include <drivers/mailbox.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <kernel/exception.h>
#include <mm/mmu.h>

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define GET_BOARD_REVISION      0x10002
#define MBOX_TAG_GETSERIAL      0x10004
#define GET_ARM_MEMORY          0x10005
#define MBOX_TAG_LAST           0

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define MBOX_REQUEST    0x00000000

unsigned int __attribute__((aligned(16))) framebuffer_mbox[36];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch, unsigned int *mbox)
{
    CRITICAL_SECTION_START;
    unsigned long r = (((unsigned long)((unsigned long)mbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == PHYS_TO_VIRT(*MBOX_READ))
            /* is it a valid successful response? */
        {
            CRITICAL_SECTION_END;
            return mbox[1]==MBOX_RESPONSE;
        }
    }
    CRITICAL_SECTION_END;
    return 0;
}

int get_board_revision(unsigned int *board_revision)
{
    /* mailbox message buffer */
    unsigned int __attribute__((aligned(16))) mbox[36];

    mbox[0] = 7 * 4;              // length of the message
    mbox[1] = MBOX_REQUEST;       // request code
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4;                  // value buffer size in bytes
    mbox[4] = 0;                  // request codes : b31 clear, b30-b0 reversed
    mbox[5] = 0;                  // clear output buffer
    mbox[6] = MBOX_TAG_LAST;      // end tag
    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP, mbox))
    {
        *board_revision = mbox[5];
        return 0;
    }
    else
    {
        uart_puts("Unable to query serial!");
        *board_revision = mbox[5] = -1;
        return -1;
    }
}

int get_arm_memory_info(unsigned int *base_addr, unsigned int *size)
{
    /* mailbox message buffer */
    unsigned int __attribute__((aligned(16))) mbox[36];

    /*
        GET arm_memory address and size
    */
    mbox[0] = 8 * 4;          // length of the message
    mbox[1] = MBOX_REQUEST;   // request code
    mbox[2] = GET_ARM_MEMORY; // tag identifier
    mbox[3] = 8;              // value buffer size in bytes
    mbox[4] = 0;              // request codes : b31 clear, b30-b0 reversed
    mbox[5] = 0;              // clear output buffer ( u32: base address in bytes )
    mbox[6] = 0;              // clear output buffer ( u32: size in bytes )
    mbox[7] = MBOX_TAG_LAST;  // end tag

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP, mbox))
    {
        *base_addr = mbox[5];
        *size = mbox[6];
        return 0;
    }
    else
    {
        uart_puts("Unable to query serial!");
        return -1;
    }
}

void init_framebuffer()
{
    extern unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
    extern unsigned char *lfb;                       /* raw frame buffer address */

    // The following code is for mailbox initialize used in lab7.
    framebuffer_mbox[0] = 35 * 4;
    framebuffer_mbox[1] = MBOX_REQUEST;

    framebuffer_mbox[2] = 0x48003; // set phy wh
    framebuffer_mbox[3] = 8;
    framebuffer_mbox[4] = 8;
    framebuffer_mbox[5] = 1024; // FrameBufferInfo.width
    framebuffer_mbox[6] = 768;  // FrameBufferInfo.height

    framebuffer_mbox[7] = 0x48004; // set virt wh
    framebuffer_mbox[8] = 8;
    framebuffer_mbox[9] = 8;
    framebuffer_mbox[10] = 1024; // FrameBufferInfo.virtual_width
    framebuffer_mbox[11] = 768;  // FrameBufferInfo.virtual_height

    framebuffer_mbox[12] = 0x48009; // set virt offset
    framebuffer_mbox[13] = 8;
    framebuffer_mbox[14] = 8;
    framebuffer_mbox[15] = 0; // FrameBufferInfo.x_offset
    framebuffer_mbox[16] = 0; // FrameBufferInfo.y.offset

    framebuffer_mbox[17] = 0x48005; // set depth
    framebuffer_mbox[18] = 4;
    framebuffer_mbox[19] = 4;
    framebuffer_mbox[20] = 32; // FrameBufferInfo.depth

    framebuffer_mbox[21] = 0x48006; // set pixel order
    framebuffer_mbox[22] = 4;
    framebuffer_mbox[23] = 4;
    framebuffer_mbox[24] = 1; // RGB, not BGR preferably

    framebuffer_mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    framebuffer_mbox[26] = 8;
    framebuffer_mbox[27] = 8;
    framebuffer_mbox[28] = 4096; // FrameBufferInfo.pointer
    framebuffer_mbox[29] = 0;    // FrameBufferInfo.size

    framebuffer_mbox[30] = 0x40008; // get pitch
    framebuffer_mbox[31] = 4;
    framebuffer_mbox[32] = 4;
    framebuffer_mbox[33] = 0; // FrameBufferInfo.pitch

    framebuffer_mbox[34] = MBOX_TAG_LAST;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mbox_call(MBOX_CH_PROP, framebuffer_mbox) && framebuffer_mbox[20] == 32 && framebuffer_mbox[28] != 0)
    {
        framebuffer_mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = framebuffer_mbox[5];        // get actual physical width
        height = framebuffer_mbox[6];       // get actual physical height
        pitch = framebuffer_mbox[33];       // get number of bytes per line
        isrgb = framebuffer_mbox[24];       // get the actual channel order
        lfb = PHYS_TO_VIRT((void *)((unsigned long)framebuffer_mbox[28]));
    }
    else
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
}