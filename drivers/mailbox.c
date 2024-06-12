#include <drivers/mailbox.h>

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

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

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
    unsigned int r = (((unsigned int)((unsigned long)&mbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

int get_board_revision(unsigned int *board_revision)
{
    mbox[0] = 7 * 4;              // length of the message
    mbox[1] = MBOX_REQUEST;       // request code
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4;                  // value buffer size in bytes
    mbox[4] = 0;                  // request codes : b31 clear, b30-b0 reversed
    mbox[5] = 0;                  // clear output buffer
    mbox[6] = MBOX_TAG_LAST;      // end tag
    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
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
    if (mbox_call(MBOX_CH_PROP))
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