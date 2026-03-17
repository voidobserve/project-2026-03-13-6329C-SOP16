
#include "ws2812_bsp.h"
#include "debug.h"
#include "my_effect.h"
#include "led_strand_effect.h"

#define SYS_MAX_LED_NUMBER 300 // 支持的最大灯珠数量，用于控制数组大小

static unsigned long tick_ms;
void ws281x_init()
{
}
void ws281x_show(unsigned char *pixels_pattern, unsigned short pattern_size)
{
    // extern void ledc_send_rgbbuf_isr(u8 index, u8 *rgbbuf, u32 led_num, u16 again_cnt);
    // ledc_send_rgbbuf_isr(0, pixels_pattern, pattern_size, 0);

    // 该数据处理是仅有白光的流星使用
    static volatile u8 buf[SYS_MAX_LED_NUMBER * 3] __attribute((aligned(4))); // 使用中断实现效果时，必须需要全局变量SYS_MAX_LED_NUMBER*3
    u16 i = 0;
    for (i = 0; i < pattern_size / 3; i++) // 3：是指RGB共三个字节
    {
        // 将原本的RGB数据全部合并成一个字节
        buf[i] = (*(pixels_pattern + (i * 3 + 0)) |
                  *(pixels_pattern + (i * 3 + 1)) |
                  *(pixels_pattern + (i * 3 + 2)));

        // buf[i] = *(pixels_pattern + (i * 3)) ;
    }

    // memcpy(buf, pixels_pattern, pattern_size);

    // for (i = 0; i <fc_effect.led_num; i++)
    // {
    //     printf_buf
    // }

    // printf("buf:\n");
    // printf_buf(buf, fc_effect.led_num / 3);
    // printf("pixels_pattern:\n");
    // printf_buf(pixels_pattern, pattern_size / 3);
    extern void led_send_single_white_buf_isr(u8 index, u8 * buf, u32 led_num, u16 again_cnt);
    led_send_single_white_buf_isr(0, buf, fc_effect.led_num / 3, 0);
}

// 周期10ms
unsigned long HAL_GetTick(void)
{
    return tick_ms;
}

// 每10ms调用一次
void run_tick_per_10ms(void)
{
    tick_ms += 10;
}
