/*
适用用2.4G遥控
基于中道版本2.4G遥控
1、app_config.h,把宏CONFIG_BT_GATT_CLIENT_NUM设置1
2、apps\spp_and_le\examples\trans_data\ble_trans.c  @bt_ble_init() 加入multi_client_init()
3、@le_gatt_client.c
   __resolve_adv_report()
   HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS 加入键值处理函数
4、在key_driver.c 注册rf24g_scan_para
5、board_ac632n_demo_cfg.h 使能TCFG_RF24GKEY_ENABLE
6、@app_tuya.c tuya_key_event_handler()加入上层应用的键值处理函数
7、底层无法判断长按，需要靠上层应用实现

以上该思路方法，在 CONFIG_APP_SPP_LE 这个demo上，实现起来很麻烦，不建议使用
 */

#include "system/includes.h"

#include "task.h"
#include "event.h"
#include "rf24g.h"
#include "led_strip_sys.h"
#include "board_ac632n_demo_cfg.h"
#include "led_strand_effect.h"
#include "btstack/btstack_typedef.h"
#include "ble_multi_profile.h"
#include "btstack/le/att.h"
#include "asm/mcpwm.h"
// #if TCFG_RF24GKEY_ENABLE

#if 1
#pragma pack(1)
typedef struct
{
    u8 pair[3];
    u8 flag; // 0:表示该数组没使用，0xAA：表示改数组已配对使用
} rf24g_pair_t;
#pragma pack()
/***********************************************************移植须修改****************************************************************/

#define PAIR_TIME_OUT 5 * 1000 // 3秒
static u16 pair_tc = 0;
// 配对计时，10ms计数一次
void rf24g_pair_tc(void)
{
    if (pair_tc <= PAIR_TIME_OUT)
    {
        pair_tc += 10;
    }
}

#define PAIR_MAX 1

/***********************************************************移植须修改 END****************************************************************/

rf24g_pair_t rf24g_pair[PAIR_MAX]; // 需要写flash

/***********************************************************API*******************************************************************/

//-------------------------------------------------效果

// -----------------------------------------------声控

// -----------------------------------------------灵敏度

/***********************************************************APP*******************************************************************/

extern rf24g_ins_t rf24g_ins;
// pair_handle是长按执行，长按时会被执行多次
// 所以执行一次后，要把pair_tc = PAIR_TIME_OUT，避免误触发2次
static void pair_handle(void)
{
    extern void save_rf24g_pair_data(void);
    u8 op = 0; // 1:配对，2：解码
    u8 i;
#if 0
    // 开机3秒内
    if(pair_tc < PAIR_TIME_OUT)
    {
        printf("\n pair_tc=%d",pair_tc);
        pair_tc = PAIR_TIME_OUT;//避免误触发2次
        

        memcpy((u8*)(&rf24g_pair[0].pair), (u8*)(&rf24g_ins.pair), 3);
        rf24g_pair[0].flag = 0xaa;
        save_rf24g_pair_data();
        printf("\n pair");
        printf_buf(&rf24g_pair[0].pair, 3);
        extern void fc_24g_pair_effect(void);
        fc_24g_pair_effect();
        // 查找表是否存在
#if 0
        for(i=0; i < PAIR_MAX; i++)
        {
            // 和现有客户码匹配,解绑
            if( memcmp( (u8*)(&rf24g_pair[i].pair), (u8*)(&rf24g_ins.pair), 3) == 0 )
            {
                op = 2;
                pair_tc = PAIR_TIME_OUT;//避免误触发2次，
                rf24g_pair[i].flag = 0;
                rf24g_pair[i].pair[0] = 0;
                rf24g_pair[i].pair[1] = 0;
                rf24g_pair[i].pair[2] = 0;
#warning "save flash"
                printf("\n dis pair");
                break;
            }
        }
        
        if(i == PAIR_MAX)
        {
            op = 1;
            for(i=0; i < PAIR_MAX; i++)
            {
                if( rf24g_pair[i].flag == 0)
                {
                    pair_tc = PAIR_TIME_OUT;//避免误触发2次
                    memcpy((u8*)(&rf24g_pair[i].pair), (u8*)(&rf24g_ins.pair), 3);
#warning "save flash"
                    printf("\n pair");
                    printf_buf(&rf24g_pair[i].pair, 3);
                    break;
                }
            }

        }
#endif

    }
#endif
}

u8 off_long_cnt = 0;
extern void parse_zd_data(unsigned char *LedCommand);
extern void set_IS_light_scene_state(void);
u8 all_mode[3] = {0x04, 0x02, 0x00};           // j模式集合
u8 sevrn_color_breath[3] = {0x04, 0x02, 0x0b}; // 七色呼吸
u8 stepmotpor_speed_cnt = 0;
u8 dynamic_speed = 0;
u8 meteor_flag;
u8 meteor_music_flag;
u8 meteor_speed[5] = {1, 25, 50, 75, 100}; // 1, 25, 50, 75, 100
u8 meteor_cycle[5] = {2, 8, 12, 16, 20};   // 2s 8s 12s 16s 20s
u8 cycle_cntt = 0;
u8 meteor_tail = 0;
u8 meteor_direction = 0; // 0：顺向  1：正向
u8 single_meteor = 0;
u8 fc_music_cnt = 0;
extern u8 Ble_Addr[6]; // 蓝牙地址
extern hci_con_handle_t fd_handle;

void rf24_key_handle(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;
    uint8_t Send_buffer[50]; // 发送缓存.

    event_type = event->u.key.event;
    key_value = event->u.key.value;

    // printf("\n event->u.key.type = %d",event->u.key.type);
    // printf("\n event->event_type = %d",event_type);
    // printf("\n key_value = %d",key_value);

    if (event->u.key.type == KEY_DRIVER_TYPE_RF24GKEY) // 按键类型
    {

        // 开/关
        if (key_value == RF24_ON_OFF && event_type == KEY_EVENT_CLICK)
        {
            if (fc_effect.on_off_flag == DEVICE_ON)
                soft_rurn_off_lights(); // 关灯.
            else
                soft_turn_on_the_light(); // 开灯
        }

        if (get_on_off_state())
        {
            // 速度/亮度 -
            if (key_value == RF24_SPEED_BRIGHT_SUB && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.Now_state == IS_STATIC)
                {

                    extern void bright_sub(void);
                    bright_sub();
                    memcpy(Send_buffer, Ble_Addr, 6);
                    Send_buffer[6] = 0x04;
                    Send_buffer[7] = 0x03;
                    Send_buffer[8] = fc_effect.b * 100 / 255;
                    ;
                    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
                if (fc_effect.Now_state == IS_light_scene || fc_effect.Now_state == ACT_CUSTOM)
                {

                    extern void ls_speed_sub(void);
                    ls_speed_sub();
                    memcpy(Send_buffer, Ble_Addr, 6);
                    Send_buffer[6] = 0x04;
                    Send_buffer[7] = 0x04;
                    Send_buffer[8] = (500 - fc_effect.dream_scene.speed) / 5;
                    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
            }
            // 速度/亮度 +
            if (key_value == RF24_SPEED_BRIGHT_PLUS && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.Now_state == IS_STATIC) // 静态
                {

                    extern void bright_plus(void);
                    bright_plus();
                    memcpy(Send_buffer, Ble_Addr, 6);
                    Send_buffer[6] = 0x04;
                    Send_buffer[7] = 0x03;
                    Send_buffer[8] = fc_effect.b * 100 / 255;
                    ;
                    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
                if (fc_effect.Now_state == IS_light_scene || fc_effect.Now_state == ACT_CUSTOM) //
                {

                    extern void ls_speed_plus(void);
                    ls_speed_plus();
                    memcpy(Send_buffer, Ble_Addr, 6);
                    Send_buffer[6] = 0x04;
                    Send_buffer[7] = 0x04;
                    Send_buffer[8] = (500 - fc_effect.dream_scene.speed) / 5;
                    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
            }

#if 0
            // 红色
            if (key_value == RF24_RED && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(0);
                save_user_data_area3();
            }
            // 绿色
            if (key_value == RF24_GREEN && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(1);
                save_user_data_area3();
            }
            // 蓝色
            if (key_value == RF24_BLUE && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(2);
                save_user_data_area3();
            }
            // 黄色
            if (key_value == RF24_YELLOW && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(4);
                save_user_data_area3();
            }
            // 天蓝色  85 250 255
            if (key_value == RF24_AZURE && event_type == KEY_EVENT_CLICK)
            {
                set_static_mode(85, 250, 255);
                save_user_data_area3();
            }
            // 玫红色  25 50 218
            if (key_value == RF24_ROSE_RED && event_type == KEY_EVENT_CLICK)
            {
                set_static_mode(255, 50, 218);
                save_user_data_area3();
            }
            // 纯白色   w b
            if (key_value == RF24_WHITE && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(3);
                save_user_data_area3();
            }
            // 模暖白光
            if (key_value == RF24_WARM_WHITE && event_type == KEY_EVENT_CLICK)
            {
                set_static_mode(255, 180, 20);
                save_user_data_area3();
            }
            // 模式集合区  流水，呼吸，星空，流星，跳变，渐变，流星雨各一个效果
            if (key_value == RF24_ALL_MODE && event_type == KEY_EVENT_CLICK)
            {

                if (fc_effect.custom_index < 9 || fc_effect.custom_index > 15)
                {
                    fc_effect.custom_index = 9;
                }
                else if (fc_effect.custom_index >= 9)
                {
                    fc_effect.custom_index++;
                }
                fc_set_style_custom(); // 自定义状态
                set_fc_effect();
                save_user_data_area3();
            }
            // 7色渐变
            if (key_value == RF24_SEVEN_COLOR_GRADUAL && event_type == KEY_EVENT_CLICK)
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
            // 七色呼吸
            if (key_value == RF24_SEVEN_COLOR_BREATHE && event_type == KEY_EVENT_CLICK)
            {

                if (fc_effect.custom_index < 22 || fc_effect.custom_index >= 28)
                    fc_effect.custom_index = 22;
                else if (fc_effect.custom_index >= 22 && fc_effect.custom_index < 28)
                    fc_effect.custom_index++;

                switch (fc_effect.custom_index)
                {
                case 22:
                    ls_set_color(0, BLUE);
                    ls_set_color(1, BLACK);
                    break;
                case 23:
                    ls_set_color(0, GREEN);
                    ls_set_color(1, BLACK);
                    break;
                case 24:
                    ls_set_color(0, RED);
                    ls_set_color(1, BLACK);
                    break;
                case 25:
                    ls_set_color(0, WHITE);
                    ls_set_color(1, BLACK);
                    break;
                case 26:
                    ls_set_color(0, YELLOW);
                    ls_set_color(1, BLACK);
                    break;
                case 27:
                    ls_set_color(0, CYAN);
                    ls_set_color(1, BLACK);
                    break;
                case 28:
                    ls_set_color(0, MAGENTA);
                    ls_set_color(1, BLACK);
                    break;
                default:
                    break;
                }
                // printf(" fc_effect.custom_index =%d", fc_effect.custom_index);
                fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH; // 单色呼吸
                fc_effect.dream_scene.c_n = 2;
                fc_effect.Now_state = IS_light_scene;
                set_fc_effect();
                save_user_data_area3();
            }
            // 7色跳变
            if (key_value == RF24_SEVEN_COLOR_JUMP && event_type == KEY_EVENT_CLICK)
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
            // 星空效果
            if (key_value == RF24_STEMPMOTOR_SPEED && event_type == KEY_EVENT_CLICK)
            {

                if (fc_effect.custom_index < 30 || fc_effect.custom_index >= 36)
                    fc_effect.custom_index = 30;
                else if (fc_effect.custom_index >= 30 && fc_effect.custom_index < 36)
                    fc_effect.custom_index++;

                switch (fc_effect.custom_index)
                {
                case 30:
                    ls_set_color(0, BLUE);
                    // ls_set_color(1, BLACK);
                    break;
                case 31:
                    ls_set_color(0, GREEN);
                    // ls_set_color(1, BLACK);
                    break;
                case 32:
                    ls_set_color(0, RED);
                    // ls_set_color(1, BLACK);
                    break;
                case 33:
                    ls_set_color(0, WHITE);
                    // ls_set_color(1, BLACK);
                    break;
                case 34:
                    ls_set_color(0, YELLOW);
                    // ls_set_color(1, BLACK);
                    break;
                case 35:
                    ls_set_color(0, CYAN);
                    // ls_set_color(1, BLACK);
                    break;
                case 36:
                    ls_set_color(0, MAGENTA);
                    // ls_set_color(1, BLACK);
                    break;
                default:
                    break;
                }
                fc_effect.dream_scene.change_type = MODE_SINGLE_FLASH_RANDOM;
                fc_effect.dream_scene.c_n = 1;
                fc_effect.Now_state = IS_light_scene;
                set_fc_effect();
                save_user_data_area3();

                // fc_set_style_custom(); //自定义效果
                // set_fc_effect();

                // printf(" \n %d,%d,%d",xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize(), xPortGetPhysiceMemorySize());
            }
            // 声控1    4音乐律动减
            if (key_value == RF24_SOUND_ONE && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.custom_index < 29 || fc_effect.custom_index > 40)
                    fc_effect.custom_index = 29;
                else if (fc_effect.custom_index < 40)
                    fc_effect.custom_index++;

                set_music_mode(fc_effect.custom_index - 29);
                save_user_data_area3();
            }
            // 声控2   4音乐律动加
            if (key_value == RF24_SOUND_TWO && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.custom_index < 29 || fc_effect.custom_index > 40)
                    fc_effect.custom_index = 40;
                else if (fc_effect.custom_index > 29)
                    fc_effect.custom_index--;

                set_music_mode(fc_effect.custom_index - 29);
                save_user_data_area3();
            }
            // 单流星  双流星
            if (key_value == RF24_ONE_TOW_METEOR && event_type == KEY_EVENT_CLICK)
            {

                if (fc_effect.custom_index != 13)
                {
                    fc_effect.custom_index = 13;
                    fc_set_style_custom(); // 自定义效果
                    set_fc_effect();
                }
                else
                {
                    fc_effect.custom_index = 17;
                    fc_set_style_custom(); // 自定义效果
                    set_fc_effect();
                }
                printf("fc_effect.custom_index = %d", fc_effect.custom_index);
            }
            // 声控流星
            if (key_value == RF24_METEOR_SOUND_ONE_TWO && event_type == KEY_EVENT_CLICK)
            {

                fc_effect.custom_index = 22;
                fc_set_style_custom(); // 自定义效果
                set_fc_effect();
            }
            // 方向控制   流星方向
            if (key_value == RF24_DIRECTION && event_type == KEY_EVENT_CLICK)
            {
                change_dir();
                fc_set_style_custom(); // 自定义效果
                set_fc_effect();
                save_user_data_area3();
            }
            // 流星速度调节 5挡  1 25 50 75 100
            if (key_value == RF24_METEOR_SPEED && event_type == KEY_EVENT_CLICK)
            {

                extern void meteor_speed_(void);
                meteor_speed_();
                memcpy(Send_buffer, Ble_Addr, 6);
                Send_buffer[6] = 0x04;
                Send_buffer[7] = 0x04;
                Send_buffer[8] = (500 - fc_effect.dream_scene.speed) / 5;
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                save_user_data_area3();
            }
            // 流星频率调节 5挡  4s 8s 12s 16s 20s
            if (key_value == RF24_METEOR_FREQUENCY && event_type == KEY_EVENT_CLICK)
            {

                extern void meteor_p_(void);
                meteor_p_();
                memcpy(Send_buffer, Ble_Addr, 6);
                Send_buffer[6] = 0x2F;
                Send_buffer[7] = 0x03;
                Send_buffer[8] = fc_effect.meteor_period;
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);

                fc_set_style_custom(); // 自定义效果
                set_fc_effect();
                save_user_data_area3();
            }
            // 流星拖尾长度调节 3 5 7
            if (key_value == RF24_METEOR_TAIL && event_type == KEY_EVENT_CLICK)
            {

                if (fc_effect.custom_index < 18 || fc_effect.custom_index >= 21)
                    fc_effect.custom_index = 18;
                else if (fc_effect.custom_index >= 18 && fc_effect.custom_index < 21)
                    fc_effect.custom_index++;

                fc_set_style_custom(); // 自定义效果
                set_fc_effect();
                save_user_data_area3();
            }
#endif
        }
    }
}

#endif
