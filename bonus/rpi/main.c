#include "uart.h"
#include "gpio.h"
#include "delays.h"
#include "mbox.h"
#include "sprintf.h"
#include "power.h"
#include "math.h"
#define assert(n)

typedef unsigned long long size_t;
void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    unsigned char val = (unsigned char)value;
    while (num--) {
        *p++ = val;
    }
    return ptr;
}
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    while (n--)
        *d++ = *s++;

    return dst;
}

typedef unsigned int u32;
typedef unsigned char bool;
#define true 1
#define false 0

typedef unsigned int clock_t;
#define CLOCKS_PER_SEC 1000000
clock_t clock() {
    return get_system_timer();
}



#define W 1024
#define H 768
#define make_color(r, g, b, a) ((Color)((a << 24) | (b << 16) | (g << 8) | r))
#include "../drawing.h"

void poll_input();
#include "space_mission.h"
void poll_input(){
        g.input.start = !g.input.start;
        g.input.up = !g.input.up;
        g.input.left = !g.input.left;
        //g.input.fire = !g.input.fire;
}

#include "lfb.h"

bool back_buffer;
void swap_buffers()
{
    //printf("pixels when about to swap: %d\n", draw_target.pixels);
    back_buffer = !back_buffer;
    int offset = back_buffer*768;
    mbox[0] = 8*4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48009; //set virt offset
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 0;           //FrameBufferInfo.x_offset
    mbox[6] = offset;           //FrameBufferInfo.y.offset
    mbox[7] = MBOX_TAG_LAST;

    int result = mbox_call(MBOX_CH_PROP);

    if(result) {
        //printf("successful call\n");
        wait_msec(500);
        if(!back_buffer)
            draw_target.pixels=((unsigned int *)(lfb)); //don't modify draw target, modify SCREEN texture, since draw-target can point to other things
        else
            draw_target.pixels=((unsigned int *)(lfb))+(768*1024);        
    }
    else{
        //printf("failed call\n");
    }



    //printf("pixels after swap: %d\n", draw_target.pixels);
}

u32 gamepad_state;
void main()
{
    uart_init();
    printf("uart initialized\n");
    lfb_init();

    draw_target = (Texture) {
        .width = W,
        .height = H,
        .pixels = (Color*)lfb,
    };

    init();
    last = clock();
#define GET_INPUTSTATE 0xFE    
    while(1) {
        printf("start of main loop\n");
        int received_char = uart_try_getc();
        if(received_char != -1){
            printf("received something: %d", received_char);
            if(received_char == GET_INPUTSTATE) {
                gamepad_state = 0;
                printf("%x ", uart_getc());
                printf("%x ", uart_getc());
                printf("%x ", uart_getc());
                printf("%x\n", uart_getc());
                //gamepad_state = uart_getc();
                //gamepad_state = uart_getc() | (gamepad_state << 8);
                //gamepad_state = uart_getc() | (gamepad_state << 8);
                //gamepad_state = uart_getc() | (gamepad_state << 8);
            }
            
            uart_send(0x06); //ack value

            if(received_char == GET_INPUTSTATE){
                printf("gamepad_state: %d\n", gamepad_state);
            }

            if(received_char == 27) { //escape key
                uart_send(0xFF); //alert client of shutdown
                wait_msec(100000);
                power_off();
            }
        }

        simulate();
        printf("finished simulate function\n");
        swap_buffers();
        //printf("swapped that buffer\n");
        //printf("buffer swap completed\n");
        //printf("successfully did nothing for 5 milliseconds\n");
    }

    power_off();
}
