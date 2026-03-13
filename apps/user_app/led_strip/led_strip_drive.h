
#ifndef led_strip_drive_h
#define led_strip_drive_h

#include "board_ac632n_demo_cfg.h"
#include "asm/ledc.h"
#include "asm/gpio.h"


#define MIC_PIN     IO_PORTA_09   //天奕幻彩使用8脚的芯片 ax3128a2
#define LEDC_PIN    IO_PORT_DM //天奕幻彩使用8脚的芯片 ax3128a2
#define MIC_AD_CH   AD_CH_PA9
// #define MIC_PIN     IO_PORTA_08   //一般16脚芯片
// #define LEDC_PIN    IO_PORTB_07   //一般16脚芯片的幻彩输出

typedef enum
{
    OFF,    //mic关闭
    ON,     //mic打开
}MIC_OFFON;

extern MIC_OFFON MIC_ENABLE;

void led_state_init(void);
void mic_gpio_init();

void ledc_init(const struct ledc_platform_data *arg);

#endif







