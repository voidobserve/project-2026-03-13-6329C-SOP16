/****************************
@led_strand_effect.c
适用：
产品ID: yxwh27s5
品类：幻彩户外串灯-蓝牙
协议：BLE
负责幻彩灯串效果制作
*****************************/
#include "system/includes.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "ws2812fx_effect.h"
#include "Adafruit_NeoPixel.H"
#include "tuya_ble_type.h"

extern void printf_buf(u8 *buf, u32 len);
static void static_mode(void);
static void fc_smear_adjust(void);
static void fc_pair_effect(void);
static void ls_scene_effect(void);
void fc_set_style_custom(void);
void custom_effect(void);
static void strand_rainbow(void);
void jump_mutil_c(void) ;
void strand_breath(void) ;
void strand_twihkle(void) ;
void strand_flow_water(void) ;
void strand_chas_light(void) ;
void strand_colorful(void) ;
void mutil_seg_grandual(void) ;
void mutil_c_grandual(void);
void ls_set_colors(uint8_t n, color_t *c);
void single_c_flash_random(void);
void fb_led_on_off_state(void);
void clean_period_cnt(void);

fc_effect_t fc_effect;//幻彩灯串效果数据
static u8 custom_index;
// 和通信协议对应
u8 rgb_sequence_map[6]=
{
    NEO_RGB,
    NEO_RBG,
    NEO_GRB,
    NEO_GBR,
    NEO_BRG,
    NEO_BGR,
};
// 效果数据初始化
void fc_data_init(void)
{
    u16 num;
    fc_effect.on_off_flag = DEVICE_ON;
    // fc_effect.led_num = 75;
    fc_effect.led_num = 75 * 3; // 原本是75个灯的RGB幻彩，但是改成单色、白光的流星灯之后，3个字节的RGB数据合成一个字节，导致实际的数据少了 1/3，这里要乘以3
    fc_effect.Now_state = IS_light_scene;
    fc_effect.rgb.r = 255;
    fc_effect.rgb.g = 0;
    fc_effect.rgb.b = 0;
    fc_effect.dream_scene.speed = 100;
    fc_effect.sequence = NEO_GBR;  //天奕幻彩gbr
    fc_effect.b = 255;
    fc_effect.b_per = 100;
    fc_effect.speed = 90;
    fc_effect.music.m = 1;
    fc_effect.music.s = 90;
    //流星周期控制
    // fc_effect.meteor_period = 8;            //默认8秒  周期值
    fc_effect.meteor_period = 60;
    fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器
    fc_effect.mode_cycle = 0;   //模式完成一个循环的标志

    zd_countdown[0].set_on_off = DEVICE_OFF;
    zd_countdown[1].set_on_off = DEVICE_OFF;
    zd_countdown[2].set_on_off = DEVICE_OFF;
    fc_effect.dream_scene.c_n =1;
    fc_effect.dream_scene.rgb[0].r =  0;
    fc_effect.dream_scene.rgb[0].g =  255;
    fc_effect.dream_scene.rgb[0].b =  0;

    fc_effect.dream_scene.change_type = MODE_COLOR_METEOR;
}

void full_color_init(void)
{

    WS2812FX_init(fc_effect.led_num,fc_effect.sequence);
    WS2812FX_setBrightness( fc_effect.b );
    soft_turn_on_the_light();
    WS2812FX_start();
    set_fc_effect();


}

/***************************************************软件关机*****************************************************/
static u8 pwr_on_effect_f=0;  //0:无需执行开机效果
void soft_rurn_off_lights(void) //软关灯处理
{
    uint16_t power_off_effect(void);
    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num,                  //起始位置，结束位置
    &power_off_effect,                  //效果
    0,                                      //颜色，WS2812FX_setColors设置
    100,                                   //速度
    FADE_GLACIAL);                            //选项，这里像素点大小：1

    WS2812FX_start();
    fc_effect.on_off_flag = DEVICE_OFF;
    fb_led_on_off_state();
    printf("soft_rurn_off_light!!\n");
   

}


/**************************************************软件开机*****************************************************/
static float bb =1;
void soft_turn_on_the_light(void)   //软开灯处理
{
    if(fc_effect.on_off_flag != DEVICE_ON)
    {
        pwr_on_effect_f = 1;
        bb = 1;
        WS2812FX_setBrightness(0);
        WS2812FX_start();
        fc_effect.on_off_flag = DEVICE_ON;
        set_fc_effect();
        fb_led_on_off_state();

        printf("soft_turn_on_the_light!!\n");
        printf("fc_effect.custom_index = %d",fc_effect.custom_index);
    }
}

// 开机效果,控制亮度渐亮,定时10ms执行
// 应用判断亮度达到最大亮度
void power_on_effect(void)
{

  if(pwr_on_effect_f == 0) return;


  if(bb<128)
  {
      bb = bb+0.5;
  }
  else
  {
      bb+=1;
  }

  if(bb >= fc_effect.b)
  {
    WS2812FX_setBrightness(fc_effect.b);
    WS2812FX_show();
    pwr_on_effect_f = 0;
    bb =1;
  }
  else
  {
    WS2812FX_setBrightness(bb);
    WS2812FX_show();
  }
}

ON_OFF_FLAG get_on_off_state(void)
{
    return fc_effect.on_off_flag;
}

void set_on_off_led(u8 on_off)
{
    // fc_effect.on_off_flag = on_off;
    printf("\n on_off=%d",on_off);
    printf("\n fc_effect.on_off_flag=%d",fc_effect.on_off_flag);

    if(on_off == DEVICE_ON)
    {
        printf("\n set_on_off_led");

        soft_turn_on_the_light();
    }
    else
    {
        soft_rurn_off_lights();
    }
}




extern uint16_t power_off_effect(void);
// 特殊模式
#define COLOR_MODE_INDEX 3  //采光模式索引
u16 ir_color_cnt;   //
void ir_color_plus(void)
{
    ir_color_cnt+=2;
    ir_color_cnt&=0xff;
}

void ir_color_sub(void)
{
    if(ir_color_cnt > 0)
    {
        ir_color_cnt-=2;
    }
    else
    {
        ir_color_cnt = 255;
    }
}

uint16_t get_ir_color_cnt(void)
{
    return ir_color_cnt;
}

// #define MAX_BUILD_MODE      //最大内置效果数量

void build_in_mode_plus(void)
{
    fc_set_style_custom();
    if(fc_effect.custom_index < 10 || fc_effect.custom_index > 23)
    {
        fc_effect.custom_index=10;
    }

    fc_effect.custom_index++;
    if(fc_effect.custom_index > 23)
    {
        fc_effect.custom_index = 10;
    }
    set_fc_effect();
}

void build_in_mode_sub(void)
{
    fc_set_style_custom();
    if(fc_effect.custom_index < 10 || fc_effect.custom_index > 23)
    {
        fc_effect.custom_index=10;
    }
    fc_effect.custom_index--;
    if(fc_effect.custom_index < 10)
    {
        fc_effect.custom_index = 23;
    }
    set_fc_effect();
}



void set_custom_effect(u8 m)
{
    fc_effect.custom_index = m;
    fc_set_style_custom(); //自定义效果
    set_fc_effect();
}

//关机效果
void set_power_off(void)
{
    fc_effect.custom_index = 1; //关机效果
    fc_set_style_custom(); //自定义效果
    set_fc_effect();
}

//双流星
void double_meteor(void)
{

    extern uint16_t fc_double_meteor(void);
    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else{
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                                    //起始位置，结束位置
        &fc_double_meteor,                          //效果
        WHITE,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,           //速度
        option);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_start();

}
#define MAX_STATIC_N    11
// 静态效果颜色map
const u32 fc_static_map[MAX_STATIC_N] = 
{
    RED,    //0
    GREEN,  //1
    BLUE,   //2
    WHITE,  //3
    YELLOW, //4
    CYAN,   //5
    MAGENTA,//6
    PURPLE, //7
    ORANGE, //8
    PINK,   //9
    GRAY,
};
// 利用fc_effect结构体，构建内置效果
void fc_static_effect(u8 n)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.dream_scene.c_n = 1;
    // if(fc_static_map[n] != WHITE)
    // {
        fc_effect.rgb.r = (fc_static_map[n] >> 16) & 0xff;
        fc_effect.rgb.g = (fc_static_map[n] >> 8) & 0xff;
        fc_effect.rgb.b = (fc_static_map[n] ) & 0xff;
        // fc_effect.w = 0;
       
    // }
    // else
    // {
    //    fc_effect.b = 255;
    // //    fc_effect.w = 255;
    // }
   
    set_fc_effect();
}



//----------------------------------静态模式
void set_static_mode(u8 r, u8 g, u8 b)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.rgb.r = r;
    fc_effect.rgb.g = g;
    fc_effect.rgb.b = b;
    set_fc_effect();
}

static void static_mode(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num,                  //起始位置，结束位置
        &WS2812FX_mode_static,                  //效果
        0,                                      //颜色，WS2812FX_setColors设置
        100,                                   //速度
        FADE_GLACIAL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(1, &fc_effect.rgb);
    WS2812FX_start();
}
/******************************************************************
 * 函数：更新涂抹效果数据
 * 形参1：tool       油桶、画笔、橡皮擦
 * 形参2：colour     hsv颜色
 * 形参3：led_place  灯点位置（0~47）
 * 返回：无
 *
 * 注：若选择IS_drum油桶，led_place参数无效
 *     若选择IS_eraser橡皮擦，colour参数无效，内部将colour设为黑色
 *****************************************************************/
void effect_smear_adjust_updata(smear_tool_e tool, hsv_t *colour,unsigned short *led_place)
{
    unsigned char num = 0;
    unsigned char max;

    //更新为涂抹功能状态
    fc_effect.Now_state = IS_smear_adjust;

    //更新工具
    fc_effect.smear_adjust.smear_tool = tool;
    printf("fc_effect.smear_adjust.smear_tool = %d\r\n",(uint8_t)fc_effect.smear_adjust.smear_tool);
    printf("\r\n");

    //清除rgb[0~n]数据
    // memset(fc_effect.smear_adjust.rgb, 0, sizeof(fc_effect.smear_adjust.rgb));

    /*HSV转换RGB*/
    if(fc_effect.smear_adjust.smear_tool == IS_drum) //油桶
    {
      m_hsv_to_rgb(&fc_effect.smear_adjust.rgb[0].r,
                   &fc_effect.smear_adjust.rgb[0].g,
                   &fc_effect.smear_adjust.rgb[0].b,
                   colour->h_val,
                   colour->s_val,
                   colour->v_val);
      max = fc_effect.led_num;
      for(num = 1; num < max; ++num)
      {
        fc_effect.smear_adjust.rgb[num].r = fc_effect.smear_adjust.rgb[0].r;
        fc_effect.smear_adjust.rgb[num].g = fc_effect.smear_adjust.rgb[0].g;
        fc_effect.smear_adjust.rgb[num].b = fc_effect.smear_adjust.rgb[0].b;

        // printf("fc_effect.smear_adjust.rgb[%d].r = %d\r\n", num,fc_effect.smear_adjust.rgb[num].r);
        // printf("fc_effect.smear_adjust.rgb[%d].g = %d\r\n", num,fc_effect.smear_adjust.rgb[num].g);
        // printf("fc_effect.smear_adjust.rgb[%d].b = %d\r\n", num,fc_effect.smear_adjust.rgb[num].b);
        // printf("\r\n");
      }
    }
    else if((fc_effect.smear_adjust.smear_tool == IS_pen) ||   //画笔
            (fc_effect.smear_adjust.smear_tool == IS_eraser))  //橡皮擦
    {
        m_hsv_to_rgb(&fc_effect.smear_adjust.rgb[*led_place].r,
                     &fc_effect.smear_adjust.rgb[*led_place].g,
                     &fc_effect.smear_adjust.rgb[*led_place].b,
                     colour->h_val,
                     colour->s_val,
                     colour->v_val);

        // printf("fc_effect.smear_adjust.rgb[%d].r = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].r);
        // printf("fc_effect.smear_adjust.rgb[%d].g = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].g);
        // printf("fc_effect.smear_adjust.rgb[%d].b = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].b);
        // printf("\r\n");
    }
}
//----------------------------------涂抹模式
extern  Segment* _seg;
extern  uint16_t _seg_len;
extern Segment_runtime* _seg_rt;


static uint16_t ls_smear_adjust_effect(void)
{
  unsigned char num;
  unsigned char max = fc_effect.led_num;
  if(max >= _seg_len) max = _seg_len;
  for(num = 0; num < max; ++num)
  {
      WS2812FX_setPixelColor_rgb(num,
        fc_effect.smear_adjust.rgb[num].r,
        fc_effect.smear_adjust.rgb[num].g,
        fc_effect.smear_adjust.rgb[num].b);
  }
  return _seg->speed;
}

// ----------------------------------动态效果

static void fc_smear_adjust(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&ls_smear_adjust_effect,BLUE,100,0);
    WS2812FX_start();
}

//----------------------------------涂鸦配网效果
static void fc_pair_effect(void)
{
    extern uint16_t unbind_effect(void);
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&unbind_effect,0,0,0);
    WS2812FX_start();
}



//----------------------------------彩虹模式  目前是一个标准，不能改动
static void strand_rainbow(void)
{
    // //WS2812FX_stop();
    // printf("\n fc_effect.dream_scene.c_n=%d",fc_effect.dream_scene.c_n);
    // printf("\n fc_effect.led_num=%d",fc_effect.led_num);
    // printf("\n fc_effect.dream_scene.speed=%d",fc_effect.dream_scene.speed);
    // printf("\n fc_effect.dream_scene.rgb");
    // printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n*sizeof(color_t));
    // printf("\n fc_effect.dream_scene.direction=%d",fc_effect.dream_scene.direction);
    extern uint16_t WS2812FX_mode_mutil_fade(void);
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_fade,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}


void rainbow_flow(void)
{

  extern uint16_t WS2812FX_mode_rainbow_cycle(void);
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_rainbow_cycle,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                            //选项，这里像素点大小：1

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();



}



//----------------------------------跳变模式
// 多段颜色跳变
void jump_mutil_c(void)
{
    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else{
        option =  REVERSE;
    }
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_single_block_scan,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 标准跳变，整体颜色跳变
void standard_jump(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

//----------------------------------频闪模式
void ls_strobe(void)
{
    extern uint16_t WS2812FX_mutil_strobe(void);

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mutil_strobe,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}


//----------------------------------呼吸模式
void strand_breath(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

void single_c_breath(void)
{
    extern uint16_t WS2812FX_mode_breath(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &WS2812FX_mode_breath,            //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}


//----------------------------------闪烁模式
void strand_twihkle(void)
{
    uint8_t option;
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_twihkle,           //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();

}

//----------------------------------流水模式，多种颜色块同时流水
void strand_flow_water(void)
{
    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else{
        option = SIZE_MEDIUM | REVERSE;
    }

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 两种颜色混合流水效果，渐变色流水
void tow_color_fix_flow(void)
{
    extern uint16_t WS2812FX_mode_running_lights(void) ;

    uint8_t option;
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_running_lights,             //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}
//----------------------------------追光模式
void strand_chas_light(void)
{
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        WS2812FX_setSegment_colorOptions(
            0,                                      //第0段
            0,fc_effect.led_num-1,                  //起始位置，结束位置
            &WS2812FX_mode_multi_forward_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);                                     //选项
    }
    else
    {
        WS2812FX_setSegment_colorOptions(
            0,                                      //第0段
            0,fc_effect.led_num-1,                  //起始位置，结束位置
            &WS2812FX_mode_multi_back_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);
    }
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

//----------------------------------炫彩模式
void strand_colorful(void)
{
    uint8_t option;
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

//----------------------------------渐变模式
// 多段，不同颜色渐变
void mutil_seg_grandual(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_fade,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
void mutil_c_grandual(void)
{
    extern uint16_t WS2812FX_mutil_c_gradual(void);

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mutil_c_gradual,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

/* -------------------------------------- 星空效果实现 ------------------------------------*/

// 单点单色随机闪现
void single_c_flash_random(void)
{
    extern uint16_t WS2812FX_mode_fire_flicker_intense(void);
    extern uint16_t starry_sky_(void);

    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &starry_sky_,    //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

// 从的颜色池抽取颜色闪现，以段为单位，闪现位置随机
void seg_mutil_c_flash_random(void)
{
    extern uint16_t WS2812FX_mode_fireworks(void);
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &WS2812FX_mode_fireworks,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------------流星效果实现---------------------------------*/

void single_c_meteor(void)
{
    extern uint16_t meteor_signle_c(void) ;
    uint8_t option;
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0|FADE_XSLOW;
    }
    else
    {
        option = REVERSE|FADE_XSLOW;
    }

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &meteor_signle_c,                       //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    option);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}
// 带有背景色的流星，每次只有一个流星，流星时间间隔3秒
void background_meteor(void)
{
    extern uint16_t meteor_bc(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &meteor_bc,                       //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

//自定义 - 三段流星追逐
void three_seg_meteor(void)
{
	WS2812FX_setSegment_colorOptions(
			0,                                      //第0段
			0,59,                  //起始位置，结束位置
			&WS2812FX_mode_larson_scanner,               //效果
			WHITE,                                      //颜色，WS2812FX_setColors设置
			2000,            //速度
			FADE_FAST);                            //选项，这里像素点大小：1

	WS2812FX_setSegment_colorOptions(
			1,                                      //第0段
			0,59,                  //起始位置，结束位置
			&WS2812FX_mode_color_sweep_random,               //效果
			WHITE,                                      //颜色，WS2812FX_setColors设置
			800,            //速度
			0);                            //选项，这里像素点大小：1

	WS2812FX_setSegment_colorOptions(
			2,                                      //第0段
			0,59,                  //起始位置，结束位置
			&WS2812FX_mode_color_sweep_random,               //效果
			WHITE,                                      //颜色，WS2812FX_setColors设置
			800,            				//速度
			0);                            //选项，这里像素点大小：1

    WS2812FX_start();
	set_seg_forward_out(2,800);
    set_seg_forward_out(1,1600);
}

//单个，全彩颜色的流星效果
void color_meteor(void)
{

    extern uint16_t single_color_meteor(void) ;
    uint8_t option;
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0|SIZE_XLARGE;
        // option = SIZE_SMALL;
    }
    else
    {
        option = REVERSE|SIZE_XLARGE;
        // option = REVERSE | SIZE_SMALL;
    }

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &single_color_meteor,                   //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    option);                                //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();

}





//跳变的流星效果
void color_jump_meteor(void)
{

    extern uint16_t jump_meteor(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &jump_meteor,                       //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();

}

//渐变的流星效果
void color_gradual_meteor(void)
{
    extern uint16_t gradual_meteor(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &gradual_meteor,                       //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();


}

/*----------------------------------------开幕闭幕效果实现---------------------------------*/
/* 开幕模式 */
void open_mode(void)
{
    extern uint16_t open_effect(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &open_effect,                             //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    0);                                     //选项

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/* 闭幕模式 */
void close_mode(void)
{
    extern uint16_t close_effect(void) ;

    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,fc_effect.led_num-1,                  //起始位置，结束位置
    &close_effect,                          //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    FADE_XXSLOW);                                     //选项

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

void dot_running(void)
{
    uint8_t option;
    extern uint16_t multi_dot_running(void);

    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else{
        option = SIZE_MEDIUM | REVERSE;
    }

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &multi_dot_running,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}


/*********************************************************/
/*                                                       */
/*                     自定义效果                         */
/*                可以做客制化的开关机效果                 */
/*********************************************************/
// 提供API，供遥控，按键，调用实现内置效果
// 提高实现开机效果，关机效果
// 提示效果，如亮度调整到最大值，最小值，显示提示效果

//流星的长度
const u8 size_type[4] =
{
    SIZE_SMALL,SIZE_MEDIUM,SIZE_LARGE,SIZE_XLARGE
};

void custom_effect(void)
{
    extern uint16_t WS2812FX_adj_rgb_sequence(void);
    extern uint16_t ir_color_key(void);
    clean_period_cnt();
    if(fc_effect.custom_index==0) //开机效果
    {
        //WS2812FX_stop();
        // WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&power_on_effect,0,0,0);
        // WS2812FX_start();
    }
    else if(fc_effect.custom_index == 1) //关机效果
    {
        //WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&power_off_effect,0,0,0);
        WS2812FX_start();
    }
    else if(fc_effect.custom_index == 2)  //调整RGB顺序效果
    {
        //WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&WS2812FX_adj_rgb_sequence,0,0,0);
        WS2812FX_start();
    }
    else if(fc_effect.custom_index == 3)    //遥控的color颜色
    {
        WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&ir_color_key,0,0,0);
        WS2812FX_start();
    }

    //------------------------内置效果实现，从10开始，预留7个备用
    else if(fc_effect.custom_index == 9) //循环彩虹
    {
        rainbow_flow();
    }
    else if(fc_effect.custom_index == 10) //流水
    {
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        fc_effect.dream_scene.c_n = 2;
        fc_effect.dream_scene.direction = IS_forward;
        strand_flow_water();
    }
    else if(fc_effect.custom_index == 11)//单色呼吸
    {

        ls_set_color(0, MAGENTA);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
        save_user_data_area3();
    }
    else if(fc_effect.custom_index == 12)//星空
    {
        ls_set_color(0, WHITE);
        fc_effect.dream_scene.change_type = MODE_SINGLE_FLASH_RANDOM;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
        save_user_data_area3();
    }
    else if(fc_effect.custom_index == 13)//流星
    {
        ls_set_color(0, WHITE);
         fc_effect.dream_scene.c_n = 1;
        color_meteor();
    }
    else if(fc_effect.custom_index == 14)//七色跳变
    {
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, MAGENTA);
        fc_effect.dream_scene.change_type = MODE_JUMP;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
        save_user_data_area3();
    }
    else if(fc_effect.custom_index == 15)//七色渐变
    {
       ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, MAGENTA);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
        save_user_data_area3();
    }
    else if(fc_effect.custom_index == 16)//流星雨
    {
        ls_set_color(0, WHITE);
        fc_effect.dream_scene.c_n = 1;
        single_c_meteor();
    }

    else if(fc_effect.custom_index == 17)//双流星
    {
        double_meteor();
    }
    //fc_effect.custom_index 18 19 20 21 用在了2.4G遥控的流星拖尾长度
    else if(fc_effect.custom_index == 18 || fc_effect.custom_index == 19 || fc_effect.custom_index == 20 ||fc_effect.custom_index == 21)
    {

        extern uint16_t single_color_meteor(void) ;
        uint8_t option;
        if(fc_effect.dream_scene.direction == IS_forward)
        {
            option = 0;
        }
        else
        {
            option = REVERSE;
        }

        WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,fc_effect.led_num-1,                  //起始位置，结束位置
        &single_color_meteor,                   //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        size_type[fc_effect.custom_index - 18] |  option);                                //选项，这里像素点大小：3

        WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
        ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

        WS2812FX_start();
    }
      //fc_effect.custom_index 22 23 24 25 26 27 28用2.4G遥控呼吸颜色选择

    else if(fc_effect.custom_index == 29) //声控流星
    {
            extern uint16_t music_meteor(void);
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                0,                          //第0段
                0,fc_effect.led_num-1,                       //起始位置，结束位置
                music_meteor,              //效果
                WHITE,                      //颜色，WS2812FX_setColors设置
                100,                        //速度
                SIZE_MEDIUM|FADE_XSLOW);               //选项，这里像素点大小：3,反向/反向
            WS2812FX_start();

    }
    else if(  
            fc_effect.custom_index == 30 || 
            fc_effect.custom_index == 31 ||
            fc_effect.custom_index == 32 ||
            fc_effect.custom_index == 33 ||
            fc_effect.custom_index == 34 ||
            fc_effect.custom_index == 35 ||
            fc_effect.custom_index == 36 
            )
    {

            extern uint16_t starry_sky_(void);
            WS2812FX_setSegment_colorOptions(
                0,                                      //第0段
                0,fc_effect.led_num-1,                  //起始位置，结束位置
                &starry_sky_,    //效果
                0,                                      //颜色，WS2812FX_setColors设置
                fc_effect.dream_scene.speed,            //速度
                0);                           //选项，这里像素点大小：3,反向/反向

            WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
            ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
            WS2812FX_start();


    }



}


/*-----------------------------------------情景效果实现-----------------------------------*/
static void ls_scene_effect(void)
{

    switch (fc_effect.dream_scene.change_type)
    {

        case MODE_MUTIL_RAINBOW:      //彩虹
            strand_rainbow();
            // printf("\n IS_SCENE_RAINBOW");
        break;
        case MODE_MUTIL_JUMP://跳变模式
            jump_mutil_c();
            // printf("\n IS_SCENE_JUMP_CHANGE");
            break;

        case MODE_MUTIL_BRAETH://呼吸模式
            strand_breath();
            break;

        case MODE_MUTIL_TWIHKLE://闪烁模式
            strand_twihkle();
            printf("\n IS_SCENE_TWIHKLE");
            break;

        case MODE_MUTIL_FLOW_WATER://流水模式
            strand_flow_water();
            printf("\n IS_SCENE_FLOW_WATER");
            break;

        case MODE_CHAS_LIGHT://追光模式
            strand_chas_light();
            printf("\n IS_SCENE_CHAS_LIGHT");
            break;

        case MODE_MUTIL_COLORFUL://炫彩模式
            strand_colorful();
            break;

        case MODE_MUTIL_SEG_GRADUAL://渐变模式
            mutil_seg_grandual();
            printf("\n IS_SCENE_GRADUAL_CHANGE");
            break;

        case MODE_JUMP:     //标准跳变
            standard_jump();
            break;

        case MODE_STROBE:   //标准频闪
            ls_strobe();
            break;

        case MODE_MUTIL_C_GRADUAL:  //多段同时渐变
            mutil_c_grandual();
            break;
        case MODE_2_C_FIX_FLOW:      //两种颜色混色流水
            tow_color_fix_flow();
            break;
        case MODE_SINGLE_FLASH_RANDOM:
            single_c_flash_random();
            break;
        case MODE_SEG_FLASH_RANDOM:
            seg_mutil_c_flash_random();
            break;
        case MODE_SINGLE_C_BREATH:   //单色呼吸
            printf("\n MODE_SINGLE_C_BREATH");
            single_c_breath();
            break;
        case MODE_SINGLE_METEOR:
            printf("\n MODE_SINGLE_METEOR");
            single_c_meteor();
            break;
        case MODE_B_G_METEOR:
            printf("\n MODE_B_G_METEOR");
            background_meteor();
            break;
        case MODE_OPEN:
            open_mode();
            break;
        case MODE_CLOSE:
            close_mode();
            break;
        case MODE_DOT_RUNNING:
            dot_running();
            break;
        case MODE_COLOR_METEOR:   //无背景颜色的流星
            printf("\n MODE_COLOR_METEOR");
            color_meteor();
            break;
        case MODE_JUMP_METEORR:   //跳变效果的流星
            printf("\n MODE_JUMP_METEORR");
            color_jump_meteor();
            break;
        case MODE_GRADUAL_METEOR:   //渐变效果的流星
            printf("\n MODE_GRADUAL_METEOR");
            color_gradual_meteor();
            break;
        case MODE_RAINBOW_FLOW:
            rainbow_flow();
            break;
        default:
            break;
    }
}

/*-----------------------------------------声控效果实现------------------------------------*/
void music_mode_plus(void)
{

    fc_effect.music.m++;
    fc_effect.music.m %= 12;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_type(u8 ty)
{
    fc_effect.music.m_type = ty;

}

void music_mode_sub(void)
{
    if(fc_effect.music.m > 0)
        fc_effect.music.m--;
    else fc_effect.music.m = 11;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_mode(u8 m)
{
    printf("\n set_music_mode = %d", m);
    fc_effect.music.m = m;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_sensitive(u8 s)
{
    printf("\n sensitive = %d", s);

    fc_effect.music.s = s;

}

void fc_music(void)
{
    extern uint16_t music_meteor(void);

    //频谱
    extern uint16_t music_fs(void) ;
    extern uint16_t music_fs_bc(void) ;
    extern uint16_t music_fs_green_blue(void) ;
    // 节奏
    extern uint16_t music_2_side_oc(void) ;
    extern uint16_t music_oc_2(void);
    extern uint16_t music_rainbow_flash(void);


    // 滚动
    extern uint16_t music_energy(void);
    extern uint16_t music_multi_c_flow(void) ;
    extern uint16_t music_meteor(void);


    // 能量
    extern uint16_t music_star(void) ; //七彩

    extern void set_music_s_m(u8 m);

    void *p;
    p = &music_star;  //防止p是空指针
    switch(fc_effect.music.m)
    {
        case 0: //能量1
            set_music_s_m(0);
            p = &music_star;
            break;
        case 1: //能量2
            set_music_s_m(1);

            p = &music_star;
            break;
        case 2: //能量3
            set_music_s_m(2);

            p = &music_star;
            break;

        case 3://节奏1
            p = &music_2_side_oc;
            break;

        case 4://节奏2
            p = &music_oc_2;
            break;

        case 5://节奏3
            p = &music_rainbow_flash;
            break;

        case 6://频谱1
            p = &music_fs;
            break;
        case 7://频谱2
            p = &music_fs_bc;
            break;
        case 8://频谱3
            p = &music_fs_green_blue;
            break;

        case 9://滚动1
            p = &music_energy;
            break;
        case 10://滚动2
            p = &music_multi_c_flow;
            break;
        case 11://滚动3
            p = &music_meteor;
            break;

    }

    WS2812FX_stop();

    WS2812FX_setSegment_colorOptions(
        0,                          //第0段
        0,fc_effect.led_num-1,                       //起始位置，结束位置
        p,              //效果
        WHITE,                      //颜色，WS2812FX_setColors设置
        100,                        //速度
        SIZE_MEDIUM|FADE_XSLOW);               //选项，这里像素点大小：3,反向/反向
    WS2812FX_start();

}


/*********************************************************/
/*                                                       */
/*                     API                               */
/*                                                       */
/*********************************************************/
// 触发提示效果，白光闪烁
void run_white_tips(void)
{
    extern uint16_t white_tips(void);
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&white_tips,0,0,0);
    WS2812FX_start();
}
//------------------------------------------------- 速度
const uint16_t speed_map[]=
{
    0,
    25,
    50,
    75,
    100

};
//s:1%-100%
u16 get_max_sp(void)
{
    u16 s;
    s = fc_effect.led_num*30/1000;
    if(s<10) s=10;
    return s; //每个LED30us
    // return 50;
}
// 最慢500ms一帧,s:1-100%

void ls_set_speed(uint8_t s)
{
    fc_effect.speed = s;
    fc_effect.dream_scene.speed =  500 - (500 * s / 100);
    if(fc_effect.dream_scene.speed <= get_max_sp())
    {
        fc_effect.dream_scene.speed = get_max_sp();
    }
    ls_scene_effect();
}
u8 speed_index = 0;
void ls_speed_plus(void)
{

    if( speed_index < 4 )
    {
        speed_index++;
    }
    // printf("speed_index = %d", speed_index);
    ls_set_speed(speed_map[speed_index]);
    set_fc_effect();
}

void ls_speed_sub(void)
{

    if( speed_index > 0 )
    {
        speed_index--;
    }
    // printf("speed_index = %d", speed_index);
    ls_set_speed(speed_map[speed_index]);

    set_fc_effect();
}
void speed_up(void)
{
    // 调整速度
    if( (fc_effect.Now_state == IS_light_scene || fc_effect.Now_state == ACT_CUSTOM) && fc_effect.custom_index != COLOR_MODE_INDEX)//采光模式不调整速度
    {
        if(fc_effect.dream_scene.speed > 30)
        {
            fc_effect.dream_scene.speed -=30;
        }
        else
        {
            fc_effect.dream_scene.speed = get_max_sp();

        }
        set_fc_effect();
        if(fc_effect.dream_scene.speed == get_max_sp())
        {
            run_white_tips();
        }
    }
    // 调整灵敏度
    if(fc_effect.Now_state == IS_light_music )
    {
        if(fc_effect.music.s < 100)
        {
            fc_effect.music.s+=10;
        }
        else
        {
            fc_effect.music.s = 100;
        }
        if(fc_effect.music.s == 100)
        {
            run_white_tips();
        }
    }

    // 采光模式，调颜色
    if(fc_effect.Now_state == ACT_CUSTOM && fc_effect.custom_index == COLOR_MODE_INDEX)
    {
        ir_color_plus();
    }
}

void speed_down(void)
{
    // 调整速度
    if( (fc_effect.Now_state == IS_light_scene || fc_effect.Now_state == ACT_CUSTOM) && fc_effect.custom_index != COLOR_MODE_INDEX)//采光模式不调整速度
    {
        if(fc_effect.dream_scene.speed < 500)
        {
            fc_effect.dream_scene.speed += 30;
        }
        else
        {
            fc_effect.dream_scene.speed = 500;
        }
        set_fc_effect();
        if(fc_effect.dream_scene.speed == 500)
        {
            run_white_tips();
        }
    }
    // 调整灵敏度
    if(fc_effect.Now_state == IS_light_music )
    {
        if(fc_effect.music.s > 10)
        {
            fc_effect.music.s-=10;
        }
        else
        {
            fc_effect.music.s = 10;
        }
        if(fc_effect.music.s == 10)
        {
            run_white_tips();
        }
    }

    // 采光模式，调颜色
    if(fc_effect.Now_state == ACT_CUSTOM && fc_effect.custom_index == COLOR_MODE_INDEX)
    {
        ir_color_sub();
    }

}

void updata_sp(void)
{
    if(get_max_sp() > fc_effect.dream_scene.speed)
    {
        fc_effect.dream_scene.speed = get_max_sp();
        printf("\n updata_sp=%d",get_max_sp());
    }
}


// ------------------------------------------------亮度
// 0-100
void set_bright(u8 b)
{
    fc_effect.b_per = b;
    fc_effect.b = 255*b/100;
    WS2812FX_setBrightness(fc_effect.b);
}

void bright_plus(void)
{
    if(fc_effect.b < 255 - 20)
    {
        fc_effect.b += 20;
    }
    else
    {
        fc_effect.b = 255;
        // run_white_tips();
    }
    WS2812FX_setBrightness(fc_effect.b);
}

void bright_sub(void)
{

    if(fc_effect.b > 20)
    {
        fc_effect.b -= 20;
    }
    else
    {
        fc_effect.b = 20;
        // run_white_tips();
    }

    WS2812FX_setBrightness(fc_effect.b);
}


// ------------------------------------------------样式
// 自定义样式
void fc_set_style_custom(void)
{
    fc_effect.Now_state = ACT_CUSTOM;
}

// 涂鸦配对样式
void fc_set_style_ty_pair(void)
{
    fc_effect.Now_state = ACT_TY_PAIR;
}

// ------------------------------------------------RGB顺序
// s:0-5
void set_rgb_sequence(u8 s)
{
    if(s < 6)
    {
        fc_effect.sequence = rgb_sequence_map[s];
        //WS2812FX_stop();
        WS2812FX_init(fc_effect.led_num,fc_effect.sequence);
        fc_effect.custom_index = 2;       //调整RGB顺序效果
        fc_set_style_custom();  //自定义效果
        set_fc_effect();
    }
}

// ------------------------------------------------设置颜色
// 设置各段颜色
// n颜色数量
// c颜色池指针
void ls_set_colors(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;
    for(i=0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }
    WS2812FX_setColors(0,colors);
}

// 设置fc_effect.dream_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位
void ls_set_color(uint8_t n, uint32_t c)
{
    if(n < MAX_NUM_COLORS)
    {
        fc_effect.dream_scene.rgb[n].r = (c>>16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c>>8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;
    }
}

// ------------------------------------------------灯带长度
void set_ls_lenght(u16 l)
{
    if(l>2048)
        l=2048;
    fc_effect.led_num = l;
    WS2812FX_stop();
    WS2812FX_init(fc_effect.led_num,fc_effect.sequence);
    WS2812FX_setBrightness( fc_effect.b );
    WS2812FX_start();
    updata_sp();

    set_fc_effect();

}



// ------------------------------------------------流星
// 0:计时完成
// 1：计时中
u8 get_effect_p(void)
{
    if(fc_effect.period_cnt > 0) return 1;
    else return 0;
}

const uint16_t meteor_p_map[]=
{
    4,
    8,
    12,
    16,
    20

};

//流星灯周期
void set_meteor_p(u8 p)
{
    if(p >= 2 && p <= 20)
    {
        fc_effect.meteor_period = p;
        fc_effect.period_cnt = 0;
    }
    // ls_scene_effect();
}


//流星周期调节
void meteor_p_(void)
{

    if(  fc_effect.meteor_period < 20 )
    {
         fc_effect.meteor_period += 2;
       
    }
    else 
         fc_effect.meteor_period = 2;

    ls_scene_effect();
}
//流星速度调节
void meteor_speed_(void)
{

    if( fc_effect.dream_scene.speed < 500 )
    {
        fc_effect.dream_scene.speed += 50;
        if( fc_effect.dream_scene.speed > 500 )
            fc_effect.dream_scene.speed = 500;
    }
    else 
        fc_effect.dream_scene.speed = 0;

    fc_set_style_custom(); //自定义效果
    set_fc_effect();

}



// 时间递减
// sub:减数，ms
//放在了while循环，10ms减一次
// fc_effect.meteor_period = 8;//默认8秒  周期值
// fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器 8000ms
void meteor_period_sub(void)
{

    if(fc_effect.period_cnt > 10)
    {
        fc_effect.period_cnt -= 10;
    }
    else{
        fc_effect.period_cnt = 0;   //计数器清零
        if(fc_effect.mode_cycle)    //模式循环完成，更新
        {
            fc_effect.period_cnt = fc_effect.meteor_period*1000;
            fc_effect.mode_cycle = 0;
        }
    }
}

//清空计时
//作用：使用数组实现流星效果的函数，在切换的流星效果时，立即切换
void clean_period_cnt(void)
{
    fc_effect.period_cnt = 0;
}

//改变方向
void change_dir(void)
{
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        fc_effect.dream_scene.direction = IS_back;

    }
    else{
        fc_effect.dream_scene.direction = IS_forward;
    }
}



/**************************************************效果调度函数*****************************************************/

void set_fc_effect(void)
{

    if(fc_effect.on_off_flag == DEVICE_ON)
    {
        switch (fc_effect.Now_state)
        {
            case IS_light_scene:

                clean_period_cnt(); //计时清零，切换效果时立即执行效果

                ls_scene_effect();
                /* code */
                break;
            case ACT_TY_PAIR:
                printf("\n ACT_TY_PAIR");

                // 配对完成，要恢复fc_effect.Now_state
                fc_pair_effect();
                /* code */
                break;
            case ACT_CUSTOM:
                custom_effect();
                /* code */
                break;
            case IS_light_music:
                /* code */
                fc_music();
                break;
            case IS_smear_adjust:
                printf("\n IS_smear_adjust");
                fc_smear_adjust();
                break;
            case IS_STATIC:
                static_mode();
                break;

            default:
                break;
        }

    }
}

/*********************************************************/
/*                                                       */
/*                     中道闹钟                           */
/*                                                       */
/*********************************************************/
u8 set_week[ALARM_NUMBER][7];

void set_zd_countdown_state(u8 s,u8 index)
{

   zd_countdown[index].set_on_off = s;   //计时开关或者闹钟开关

}

/**
 * @brief 闹钟数据出解析
 *
 * @param index
 */


void parse_alarm_data(int index)
{

    /*解析循环星期*/
    u8 p_mode;
    p_mode = alarm_clock[index].mode;
    for(int i = 0; i < 7; i++)
    {
        if(p_mode & 0x01)
            set_week[index][i] =  i + 1;

        else
            set_week[index][i] = 0;
        p_mode = p_mode >> 1;

    }


    if(alarm_clock[index].on_off == 0x80)  //闹钟开
    {
        printf("open alarm");
        set_zd_countdown_state(DEVICE_ON,index);    //开启闹钟
    }

    if(alarm_clock[index].on_off == 0x00)  //闹钟关
    {
        printf("close alarm");
        set_zd_countdown_state(DEVICE_OFF,index);  //关闭闹钟
    }



}

void close_alarm(int index)
{
    uint8_t Send_buffer[6];        //发送缓存

    zd_countdown[index].set_on_off = 0;
    Send_buffer[0] = 0x05;
    Send_buffer[1] = index;
    Send_buffer[2] = alarm_clock[index].hour;
    Send_buffer[3] = alarm_clock[index].minute;
    Send_buffer[4] = 0;
    Send_buffer[5] = alarm_clock[index].mode;
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 6);
}

void fb_led_on_off_state(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[0] = 0x01;
    Send_buffer[1] = 0x01;
    Send_buffer[2] = get_on_off_state(); //
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);

}



/**
 * @brief 闹钟处理
 *
 * @param index 闹钟编号
 */

void countdown_handler(int index)
{
    if(zd_countdown[index].set_on_off)  //闹钟开启
    {
        if(time_clock.hour == alarm_clock[index].hour &&  time_clock.minute == alarm_clock[index].minute)
        {

            if((alarm_clock[index].mode & 0x7f) == 0)  //没有星期
            {
                if((alarm_clock[index].mode >> 7))  //闹钟设置里灯的状态
                {
                    soft_turn_on_the_light();
                     close_alarm(index);
                    save_user_data_area3();
                }
                else
                {
                    soft_rurn_off_lights();
                     close_alarm(index);
                    save_user_data_area3();
                }


            }
            else
            {
                for(int i = 0; i < 7; i++)
                {

                    if(time_clock.week == set_week[index][i])
                    {


                        if((alarm_clock[index].mode >> 7))
                        {
                            soft_turn_on_the_light();
                            close_alarm(index);
                            save_user_data_area3();
                        }
                        else
                        {
                            soft_rurn_off_lights();
                            close_alarm(index);
                            save_user_data_area3();
                        }

                    }

                }


            }


        }

    }
}



u8 calculate_ms = 0;
/**
 * @brief 放在主函数的while中，10ms调用一次
 *
 */
void time_clock_handler(void)
{

    calculate_ms++;
    if(calculate_ms == 100)   //秒
    {
        calculate_ms = 0;
        time_clock.second++;

        if(time_clock.second == 60)  //分
        {
            time_clock.second = 0;
            time_clock.minute++;

            if(time_clock.minute == 60) //时
            {
                time_clock.minute = 0;
                time_clock.hour++;

                if(time_clock.hour == 24) //日
                {
                    time_clock.hour = 0;
                    time_clock.week++;
                    if(time_clock.week == 8)  //周
                    {
                        time_clock.week = 1;
                    }

                }
            }

        }
        // printf("time_clock.hour  = %d",time_clock.hour );
        // printf("time_clock.minute  = %d",time_clock.minute );
        // printf("time_clock.second  = %d",time_clock.second );
        // printf("time_clock.week  = %d",time_clock.week );

    }

    countdown_handler(0);  //闹钟0
    countdown_handler(1);
    countdown_handler(2);

}


