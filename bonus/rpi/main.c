#include "uart.h"
#include "gpio.h"
#include "delays.h"
#include "mbox.h"
#include "sprintf.h"
#include "power.h"
#include "math.h"
#define assert(n)


#define RPI

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

bool back_buffer;
#include "lfb.h"


#define W 1024
#define H 768
#define make_color(r, g, b, a) ((Color)((a << 24) | (b << 16) | (g << 8) | r))
#include "../drawing.h"

void poll_input();
#include "space_mission.h"
void poll_input(){
#define GET_INPUTSTATE 0xFE
    int received_char = uart_try_getc();
    if(received_char != -1){
        //printf("received something: %d", received_char);
        if(received_char == GET_INPUTSTATE) {
            uart_send(0x06); //ack value

            wait_msec(300);
            g.input.up = uart_getc();
            uart_send(0x06); //ack value

            wait_msec(300);
            g.input.down = uart_getc();
            uart_send(0x06); //ack value
            
            wait_msec(300);
            g.input.left = uart_getc();
            uart_send(0x06); //ack value
            
            wait_msec(300);
            g.input.right = uart_getc();
            uart_send(0x06); //ack value

            wait_msec(300);
            g.input.fire = uart_getc();
            uart_send(0x06); //ack value

            wait_msec(300);
            g.input.start = uart_getc();
            uart_send(0x06); //ack value
        }

        // if(received_char == GET_INPUTSTATE) {
        //     printf("up: %d, down: %d, left: %d, right: %d, fire: %d, start: %d\n",
        //         g.input.up,
        //         g.input.down,
        //         g.input.left,
        //         g.input.right,
        //         g.input.fire,
        //         g.input.start
        //     );
        // }
#define VK_ESCAPE 27
        if(received_char == VK_ESCAPE) { //escape key
            uart_send(0x06); //ack value
            wait_msec(1500);
            uart_send(0xFF); //alert client of shutdown
            wait_msec(1500);
            power_off();
        }
    }
}


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
        if(back_buffer)
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
    //printf("uart initialized\n");
    lfb_init();

    draw_target = (Texture) {
        .width = W,
        .height = H,
        .pixels = (Color*)lfb,
    };

    init();
    last = clock();

    while(1) {
        simulate();
        swap_buffers();
        //printf("swapped that buffer\n");
        //printf("buffer swap completed\n");
        //printf("successfully did nothing for 5 milliseconds\n");
    }

    power_off();
}
