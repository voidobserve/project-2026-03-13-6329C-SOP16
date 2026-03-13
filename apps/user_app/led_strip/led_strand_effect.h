#ifndef led_strand_effect_h
#define led_strand_effect_h
#include "cpu.h"
#include "led_strip_sys.h"
#include "WS2812FX.H"
#define MAX_SMEAR_LED_NUM 48  //最多48个灯/48段
//当前模式枚举
typedef enum
{
  ACT_TY_PAIR,          //配对效果
  ACT_CUSTOM,           //自定义效果
  IS_STATIC,            //静态模式
  IS_light_music = 27,    //音乐律动
  IS_light_scene = 56,   //炫彩情景
  IS_smear_adjust = 59  //涂抹功能


} Now_state_e;

//涂抹工具
typedef enum
{
  IS_drum = 1,  //油桶
  IS_pen = 2,   //画笔
  IS_eraser = 3 //橡皮檫
} smear_tool_e;

//方向
typedef enum
{
  IS_forward = 0, //正向
  IS_back = 16    //反向
} direction_e;

//变化方式
typedef enum
{
  MODE_MUTIL_RAINBOW = 2,           //彩虹(多段颜色)
  MODE_MUTIL_JUMP = 10,             //跳变模式(多段颜色)
  MODE_MUTIL_BRAETH = 11,           //呼吸模式(多段颜色)
  MODE_MUTIL_TWIHKLE = 12,          //闪烁模式(多段颜色)
  MODE_MUTIL_FLOW_WATER = 13,       //流水模式(多段颜色)
  MODE_CHAS_LIGHT = 14,             //追光模式
  MODE_MUTIL_COLORFUL = 15,         //炫彩模式(多段颜色)
  MODE_MUTIL_SEG_GRADUAL = 16,      //渐变模式(多段颜色)
  MODE_JUMP,                        //标准跳变
  MODE_STROBE,                      //频闪，颜色之间插入黑mode
  MODE_MUTIL_C_GRADUAL,             //多种颜色切换整条渐变
  MODE_2_C_FIX_FLOW,                //两种颜色混合流水，渐变色流水
  MODE_SINGLE_FLASH_RANDOM = 21 ,   //星空效果，单灯随机闪烁
  MODE_SEG_FLASH_RANDOM = 22,       //星云效果，一段随机闪烁
  MODE_SINGLE_METEOR = 23,          //流星效果
  MODE_SINGLE_C_BREATH = 24,        //单色呼吸
  MODE_B_G_METEOR = 25,             //带背景色流星
  MODE_OPEN = 26,                   //开幕式
  MODE_CLOSE = 27,                  //闭幕式
  MODE_DOT_RUNNING = 28,            //多个点跑马 ，点和点直接固定间隔5，支持每个点不同颜色，支持设置背景色
  MODE_COLOR_METEOR = 29,           //无背景色，指定颜色的流星
  MODE_JUMP_METEORR = 30,           //跳变效果的流星
  MODE_GRADUAL_METEOR =31,          //渐变效果的流星
  MODE_RAINBOW_FLOW = 32,

} change_type_e;



#define ALARM_NUMBER  3

#pragma pack (1)
/*----------------------------涂抹功能结构体----------------------------------*/
typedef struct
{
  smear_tool_e smear_tool;
  color_t rgb[MAX_SMEAR_LED_NUM];
} smear_adjust_t;

/*----------------------------静态模式结构体----------------------------------*/
  

/*----------------------------幻彩情景结构体----------------------------------*/
typedef struct
{
  change_type_e change_type;  //变化类型、模式
  direction_e direction; 
  unsigned char seg_size;     //段大小
  unsigned char c_n;          //颜色数量
  color_t rgb[MAX_NUM_COLORS];
  unsigned short speed;       //由档位决定 
} dream_scene_t;

/*----------------------------倒计时结构体----------------------------------*/
typedef struct
{
  unsigned char set_on_off;
  unsigned long time;
} countdown_t;

typedef struct
{
  unsigned char m;  //模式
  unsigned char s;  //灵敏度
  unsigned char m_type;  //区分音乐的模式，手机麦或者外麦
}music_t;

typedef struct 
{

  u8 hour;
  u8 minute;
  u8 second;
  u8 week;

}TIME_CLOCK;

TIME_CLOCK time_clock;

typedef struct 
{
    u8 week;
    u8 hour;
    u8 minute;
    u8 on_off;
    u8 mode;

}ALARM_CLOCK;



/*----------------------------幻彩灯串效果大结构体----------------------------------*/
typedef struct
{
  unsigned char on_off_flag;    //开关状态
  unsigned short led_num;       //灯点数
  color_t rgb;                  //静态模式颜色
  unsigned char b;              //最终亮度0-255
  unsigned char b_per;          //亮度档位，APP下发
  unsigned char speed;          //档位1-100
  unsigned char sequence;       //RGB通道顺序
  Now_state_e Now_state;        //当前运行模式
  smear_adjust_t smear_adjust;  //涂抹功能
  dream_scene_t dream_scene;    //幻彩情景
  countdown_t countdown;        //倒计时
  music_t music;                //音乐 or 声控
  unsigned char custom_index;   //自定义
  unsigned char mode_cycle;     //1:模式完成一个循环。0：正在跑，和meteor_period搭配用   流星
  u16 period_cnt;               //ms,运行时的计数器   流星
  unsigned char meteor_period;  //周期值，单位秒   流星
} fc_effect_t;

#pragma pack ()
ALARM_CLOCK alarm_clock[ALARM_NUMBER];
countdown_t zd_countdown[ALARM_NUMBER];  //中道闹钟
extern fc_effect_t fc_effect;

void effect_smear_adjust_updata(smear_tool_e tool, hsv_t *colour,unsigned short *led_place);

void set_fc_effect(void);

void full_color_init(void);
void soft_rurn_off_lights(void); //软关灯处理
void soft_turn_on_the_light(void) ;  //软开灯处理
void ls_set_speed(uint8_t s);
void ls_set_color(uint8_t n, uint32_t c);


#endif
