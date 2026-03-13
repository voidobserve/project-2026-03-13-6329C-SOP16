
#include "ws2812_bsp.h"
#include "debug.h"
#include "my_effect.h"
static unsigned long tick_ms;
void ws281x_init()
{

}
void ws281x_show(unsigned char *pixels_pattern, unsigned short pattern_size)
{
    extern void ledc_send_rgbbuf_isr(u8 index, u8 *rgbbuf, u32 led_num, u16 again_cnt);

    // ledc_send_rgbbuf(0, pixels_pattern, pattern_size, 0);
    ledc_send_rgbbuf_isr(0, pixels_pattern, pattern_size, 0);

}

// 周期10ms
unsigned long HAL_GetTick(void)
{
    return tick_ms;
}

// 每10ms调用一次
void run_tick_per_10ms(void)
{
    tick_ms+=10;
}




