/*
适用：
产品ID: yxwh27s5
品类：幻彩户外串灯-蓝牙
协议：BLE
DP协议转置，提取DP点有效数据
*/
#include "cpu.h"
#include "led_strip_sys.h"
#include "paint_tool.h"
#include "dp_data_tran.h"
#include "led_strand_effect.h"
#include "ble_trans_profile.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "btstack/le/att.h"
#include "ble_user.h"
#include "btstack/le/ble_api.h"
#include "ir_key_app.h"

dp_data_header_t  dp_data_header;  //涂鸦DP数据头
dp_switch_led_t   dp_switch_led;   //DPID_SWITCH_LED开关
dp_work_mode_t    dp_work_mode;    //DPID_WORK_MODE工作模式
dp_countdown_t    dp_countdown;    //DPID_COUNTDOWN倒计时
dp_music_data_t   dp_music_data;   //DPID_MUSIC_DATA音乐灯
// dp_secene_data_t  dp_secene_data;  //DPID_RGBIC_LINERLIGHT_SCENE炫彩情景
dp_lednum_set_t   dp_lednum_set;   //DPID_LED_NUMBER_SET led点数设置
dp_draw_tool_t    dp_draw_tool;    //DPID_DRAW_TOOL 涂抹功能




/************************************************************************************
 *@  函数：string_hex_Byte
 *@  描述：把字符串转为HEX，支持小写abcdef,输出uint8、uint16、uint32类型的hex
 *@  形参1: str         <字符输入>
 *@  形参2: out_Byte    <输出的1、2、4字节>
 *@  返回：unsigned long
 ************************************************************************************/
unsigned long string_hex_Byte(char* str, unsigned char out_Byte)
{
    int i = 0;
    unsigned char temp = 0;
    unsigned long hex = 0;
    if((out_Byte == 1)||(out_Byte == 2)||(out_Byte == 4))
    {
        while(i < (out_Byte * 2))
        {
            hex <<= 8;
            if(str[i]>='0' && str[i]<='9')
            {
                temp = (str[i] & 0x0f);
            }
            else if(str[i]>='a' && str[i]<='f')
            {
                temp = ((str[i] + 0x09) & 0x0f);
            }
            ++i;
            if(out_Byte != 1)
            {
                temp <<= 4;
                if(str[i]>='0' && str[i]<='9')
                {
                    temp |= (str[i] & 0x0f);
                }
                else if(str[i]>='a' && str[i]<='f')
                {
                    temp |= ((str[i] + 0x09) & 0x0f);
                }
            }
            ++i;
            hex |= temp;
        }
    }
    else return 0xffffffff; //错误类型
    return hex;
}

/*****************************************
 *@  函数：dp_extract_data_handle
 *@  描述：dp数据提取
 *@  形参: buff
 *@  返回：DP数据长度
 ****************************************/
unsigned short dp_extract_data_handle(unsigned char *buff)
{
    unsigned char num = 0;
    unsigned char max;
    unsigned char temp;
    /*提取涂鸦DP数据头*/
    dp_data_header.id = buff[0];
    dp_data_header.type = buff[1];
    dp_data_header.len = buff[2];
    dp_data_header.len <<= 8;
    dp_data_header.len |= buff[3];

    // m_hsv_to_rgb(&colour_R,&colour_G,&colour_B,colour_H,colour_S,colour_V);
    // if(get_on_off_state())
    // {
        /*提取DP数据*/
        switch (dp_data_header.id)
        {
            case DPID_SWITCH_LED://开关(可下发可上报)
                printf("\r\n DPID_SWITCH_LED");

                fc_effect.on_off_flag = buff[4];
                if(fc_effect.on_off_flag == DEVICE_ON)
                {
                    soft_turn_on_the_light();
                }
                else
                {
                    soft_rurn_off_lights();
                }
                break;

            case DPID_WORK_MODE://工作模式(可下发可上报)
                dp_work_mode.mode = buff[4];

                printf("dp_work_mode.mode = %d\r\n",dp_work_mode.mode);
                printf("\r\n");

                break;

            case DPID_BRIGHT_VALUE://白光亮度(可下发可上报)
                break;

            case DPID_COLOUR_DATA://彩光(可下发可上报)
                break;

            case DPID_COUNTDOWN://倒计时(可下发可上报)
                //DP协议参数
                dp_countdown.time = buff[4];
                dp_countdown.time <<= 8;
                dp_countdown.time |= buff[5];
                dp_countdown.time <<= 8;
                dp_countdown.time |= buff[6];
                dp_countdown.time <<= 8;
                dp_countdown.time |= buff[7];

                printf("dp_countdown.time = %d\r\n",dp_countdown.time);
                printf("\r\n");

            //效果参数
                fc_effect.countdown.time = dp_countdown.time;

                printf("fc_effect.countdown.time = %d\r\n",fc_effect.countdown.time);
                printf("\r\n");

                break;

            case DPID_MUSIC_DATA://音乐律动(只下发)
            //DP协议参数
                dp_music_data.change_type = string_hex_Byte(&buff[4], 1);
                dp_music_data.colour.h_val = string_hex_Byte(&buff[5], 2);
                dp_music_data.colour.s_val = string_hex_Byte(&buff[9], 2);
                dp_music_data.colour.v_val = string_hex_Byte(&buff[13], 2);
                dp_music_data.white_b = string_hex_Byte(&buff[17], 2);
                dp_music_data.ct = string_hex_Byte(&buff[21], 2);


                break;

            case DPID_RGBIC_LINERLIGHT_SCENE://炫彩情景(可下发可上报)
                fc_effect.Now_state = IS_light_scene;

                //变化类型、模式
                fc_effect.dream_scene.change_type = buff[4];
                // 方向
                fc_effect.dream_scene.direction = buff[5];
                // 段大小
                fc_effect.dream_scene.seg_size = buff[6];
                if(fc_effect.dream_scene.c_n > MAX_NUM_COLORS)
                {
                    fc_effect.dream_scene.c_n = MAX_NUM_COLORS;
                }
                // 颜色数量
                fc_effect.dream_scene.c_n = buff[7];
                //清除rgb[0~n]数据
                memset(fc_effect.dream_scene.rgb, 0, sizeof(fc_effect.dream_scene.rgb));

                //数据循环传输
                for(num=0; num < fc_effect.dream_scene.c_n; num++)
                {
                    fc_effect.dream_scene.rgb[num].r = buff[8 + num*3];
                    fc_effect.dream_scene.rgb[num].g = buff[9 + num*3];
                    fc_effect.dream_scene.rgb[num].b = buff[10 + num*3];
                }
                printf("\n fc_effect.dream_scene.c_n=%d",fc_effect.dream_scene.c_n);
                printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n*sizeof(color_t));
                printf("\n fc_effect.dream_scene.direction=%d",fc_effect.dream_scene.direction);
                set_fc_effect();
                set_ir_auto(IR_PAUSE);
                break;

            case DPID_LED_NUMBER_SET://led点数设置(可下发可上报)
            //DP协议参数
                dp_lednum_set.lednum = buff[4];
                dp_lednum_set.lednum <<= 8;
                dp_lednum_set.lednum |= buff[5];
                dp_lednum_set.lednum <<= 8;
                dp_lednum_set.lednum |= buff[6];
                dp_lednum_set.lednum <<= 8;
                dp_lednum_set.lednum |= buff[7];

                printf("dp_lednum_set.lednum = %d\r\n",dp_lednum_set.lednum);
                printf("\r\n");

            //效果参数
                fc_effect.led_num = dp_lednum_set.lednum;

                if(fc_effect.led_num >= 48) fc_effect.led_num = 48;

                break;

            case DPID_DRAW_TOOL://涂抹功能(可下发可上报)
                //DP协议参数
                dp_draw_tool.tool = buff[5];

                dp_draw_tool.colour.h_val = buff[7];
                dp_draw_tool.colour.h_val <<= 8;
                dp_draw_tool.colour.h_val |= buff[8];
                dp_draw_tool.colour.s_val = buff[9]*10;
                dp_draw_tool.colour.v_val = buff[10]*10;

                dp_draw_tool.white_b = buff[11]*10;

                if(buff[13] == 0x81)
                {
                dp_draw_tool.led_place = buff[14];
                dp_draw_tool.led_place <<= 8;
                dp_draw_tool.led_place |= buff[15];
                }

                //效果参数
                if(dp_draw_tool.white_b != 0)
                {
                dp_draw_tool.colour.h_val = 0;
                dp_draw_tool.colour.s_val = 0;
                dp_draw_tool.colour.v_val = dp_draw_tool.white_b;
                }

                effect_smear_adjust_updata((smear_tool_e )dp_draw_tool.tool, &(dp_draw_tool.colour), &dp_draw_tool.led_place);

                break;

        }
  
    // }
  
  return (dp_data_header.len + sizeof(dp_data_header_t));
}
u8 Ble_Addr[6];

// extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
extern hci_con_handle_t fd_handle;
/* 解析中道数据，主要是静态模式，和动态效果的“基本”效果 */
void parse_zd_data(unsigned char *LedCommand)
{
  uint8_t Send_buffer[50];        //发送缓存
  extern void set_ir_auto(ir_auto_e auto_f);

  if(LedCommand[0] == 0x01 && LedCommand[1] == 0x03)  //与APP同步数据
  {
    // ble_user_send_version();

    memcpy(Send_buffer,Ble_Addr, 6);
    // -----------------设备信息------------------------------
    Send_buffer[6] = 0x07;
    Send_buffer[7] = 0x01;
    Send_buffer[8] = 0x01;
    Send_buffer[9] = 0x01;  //灯具类型：RGB   
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 10, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1); 
    //-------------------发送开关机状态---------------------------
    Send_buffer[6] = 0x01;
    Send_buffer[7] = 0x01;
    // 默认开机
    Send_buffer[8] = fc_effect.on_off_flag;
    printf("fc_effect.on_off_flag = %d", fc_effect.on_off_flag);
    int ret = ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    printf("ret = %d", ret);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1); 
    //-------------------发送亮度---------------------------
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x03;
    Send_buffer[8] = fc_effect.b_per;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------发送速度---------------------------
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x04;
    Send_buffer[8] = fc_effect.speed;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------发送灯带长度---------------------------
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x08;
    Send_buffer[8] = fc_effect.led_num>>8;
    Send_buffer[9] = fc_effect.led_num&0xff;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 10, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,10, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------发送灵敏度 ---------------------------
    Send_buffer[6] = 0x2F;
    Send_buffer[7] = 0x05;
    Send_buffer[8] = fc_effect.music.s;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //---------------------流星周期 ---------------------------
    Send_buffer[6] = 0x2F;
    Send_buffer[7] = 0x03;
    Send_buffer[8] = fc_effect.meteor_period;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    //-------------------- RGB 接口 ---------------------------
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x05;
    Send_buffer[8] = fc_effect.sequence;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    #if 0
    //-------------------发送工作模式---------------------------
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x02;
    if(led_state.running_task == DYNAMIC_TASK)
    {
        Send_buffer[8] = led_state.dynamic_state_flag;    //发送动态模式
        app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    }
    else if(led_state.running_task == STATIC_TASK)
    {
        Send_buffer[8] = led_state.static_state_flag;     //发送静态模式
        app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    }
    os_time_dly(1); 
    //-------------------发送静态RGB模式--------------------------
  
    Send_buffer[6] = 0x04;
    Send_buffer[7] = 0x01;
    Send_buffer[8] = 0x1e;
    Send_buffer[9] = led_state.R_flag;
    Send_buffer[10] = led_state.G_flag;
    Send_buffer[11] = led_state.B_flag;
    app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,12, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    #endif



    //-------------------发送闹钟1定时数据--------------------------
    Send_buffer[6] = 0x05;
    Send_buffer[7] = 0x00;
    Send_buffer[8] = alarm_clock[0].hour;
    Send_buffer[9] = alarm_clock[0].minute;
    Send_buffer[10] = zd_countdown[0].set_on_off;
    Send_buffer[11] = alarm_clock[0].mode;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
    // int rett = app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,12, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------发送闹钟2定时数据--------------------------
    Send_buffer[6] = 0x05;
    Send_buffer[7] = 0x01;
    Send_buffer[8] = alarm_clock[1].hour;
    Send_buffer[9] = alarm_clock[1].minute;
    Send_buffer[10] = zd_countdown[1].set_on_off;
    Send_buffer[11] = alarm_clock[1].mode;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,12, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------发送闹钟3定时数据--------------------------
    Send_buffer[6] = 0x05;
    Send_buffer[7] = 0x02;
    Send_buffer[8] = alarm_clock[2].hour;
    Send_buffer[9] = alarm_clock[2].minute;
    Send_buffer[10] = zd_countdown[2].set_on_off;
    Send_buffer[11] = alarm_clock[2].mode;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,12, ATT_OP_AUTO_READ_CCC);
    os_time_dly(1);
    //-------------------声控模式（手机麦或外麦--------------------------
    Send_buffer[6] = 0x06;
    Send_buffer[7] = 0x07;
    Send_buffer[8] = fc_effect.music.m_type;
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    //-------------------本地麦克风模式--------------------------
    Send_buffer[6] = 0x06;
    Send_buffer[7] = 0x06;
    Send_buffer[8] = fc_effect.music.m ;
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);

  }

    memcpy(Send_buffer,Ble_Addr, 6);
  {
    //---------------------------------接收到开灯命令-----------------------------------
    if(LedCommand[0]==0x01 && LedCommand[1]==0x01  )
    {
        printf("\n switch ins");
        extern void set_on_off_led(u8 on_off);
        set_on_off_led(LedCommand[2]);
        Send_buffer[6] = 0x01;
        Send_buffer[7] = 0x01;  
        Send_buffer[8] = LedCommand[2];
        // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
    }
    //---------------------------------接收到时间数据，无需返回应答-----------------------------------
    if(LedCommand[0]==0x06 && LedCommand[1]==0x02)
    {
        extern TIME_CLOCK time_clock;
        time_clock.hour = LedCommand[2];     //更新时间数据
        time_clock.minute = LedCommand[3];
        time_clock.second = LedCommand[4];
        time_clock.week = LedCommand[5];
        printf("\n time_clock.hour=%d",time_clock.hour);
        printf("\n time_clock.minute=%d",time_clock.minute);

    }
    //-----------------------------------接收到闹钟数据-----------------------------------------
    if(LedCommand[0]==0x05)
    {
        extern ALARM_CLOCK alarm_clock[3];
        Send_buffer[6] = 0x05;
        Send_buffer[7] = LedCommand[1]; //闹钟序号
        Send_buffer[8] = LedCommand[2];
        Send_buffer[9] = LedCommand[3];
        Send_buffer[10] = LedCommand[4];
        Send_buffer[11] = LedCommand[5];
        // int rett =  app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,12, ATT_OP_AUTO_READ_CCC);
        ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
        
        if(LedCommand[1]==0x00)
        {
            alarm_clock[0].hour = LedCommand[2];
            alarm_clock[0].minute = LedCommand[3];
            alarm_clock[0].on_off = LedCommand[4];
            alarm_clock[0].mode = LedCommand[5];
            parse_alarm_data(0);
                    
        }
        if(LedCommand[1]==0x01)
        {
            alarm_clock[1].hour = LedCommand[2];
            alarm_clock[1].minute = LedCommand[3];
            alarm_clock[1].on_off = LedCommand[4];
            alarm_clock[1].mode = LedCommand[5];
            parse_alarm_data(1);
        }
        if(LedCommand[1]==0x02)
        {
            alarm_clock[2].hour = LedCommand[2];
            alarm_clock[2].minute = LedCommand[3];
            alarm_clock[2].on_off = LedCommand[4];
            alarm_clock[2].mode = LedCommand[5];
            parse_alarm_data(2);
        }

        // printf("on_off[0] = %02x,hour[0] = %02x,minute[0] = %02x,mode[0] = %02x\n",alarm_clock[0].on_off,alarm_clock[0].hour,alarm_clock[0].minute,alarm_clock[0].mode);
        // printf("on_off[1] = %02x,hour[1] = %02x,minute[1] = %02x,mode[1] = %02x\n",alarm_clock[1].on_off,alarm_clock[1].hour,alarm_clock[1].minute,alarm_clock[1].mode);
        // printf("on_off[2] = %02x,hour[2] = %02x,minute[2] = %02x,mode[2] = %02x\n",alarm_clock[2].on_off,alarm_clock[2].hour,alarm_clock[2].minute,alarm_clock[2].mode); 


    }
    //--------------------------下面所有的操作都需要在开灯状态下操作-----------------------------------
    // if(led_state.OpenorCloseflag == OPEN_STATE)
    {
        /*
        memcpy(Send_buffer,Ble_Addr, 6);
        for(i=6;i<(buffer_size+6);i++)
        {
            Send_buffer[i] = buffer[i-6];
        }
        if (app_send_user_data_check(6))
        {
            app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,6+buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        */
        if(get_on_off_state())
        {
            //---------------------------------动态处理-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x02 && LedCommand[2]>=0x07 && LedCommand[2]<=0x1c)
            {
                set_ir_auto(IR_PAUSE);
                switch(LedCommand[2])
                {
                    
                    case 0x07:  //3色跳变
                        ls_set_color(0, BLUE);
                        ls_set_color(1, GREEN);
                        ls_set_color(2, RED);
                        fc_effect.dream_scene.change_type = MODE_JUMP;
                        fc_effect.dream_scene.c_n = 3;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x08:  //7色跳变
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
                        break;

                    case 0x09:  //3色渐变
                        ls_set_color(0, BLUE);
                        ls_set_color(1, GREEN);
                        ls_set_color(2, RED);
                        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
                        fc_effect.dream_scene.c_n = 3;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x0A:  //七彩渐变
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
                        break;

                    case 0x0B:
                        ls_set_color(0, RED);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x0c:
                        ls_set_color(0, BLUE);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;
                    case 0x0D:
                        ls_set_color(0, GREEN);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;
                    
                    case 0x0E:
                        ls_set_color(0, CYAN);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;
                    
                    case 0x0F:
                        ls_set_color(0, YELLOW);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x10:
                        ls_set_color(0, MAGENTA);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x11:
                        ls_set_color(0, WHITE);
                        ls_set_color(1, BLACK);
                        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x12:
                        ls_set_color(0, RED);
                        ls_set_color(1, GREEN);
                        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x13:
                        ls_set_color(0, BLUE);
                        ls_set_color(1, RED);
                        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x14:
                        ls_set_color(0, GREEN);
                        ls_set_color(1, BLUE);
                        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
                        fc_effect.dream_scene.c_n = 2;
                        fc_effect.Now_state = IS_light_scene;
                        break;

                    case 0x15:  //七色频闪
                        ls_set_color(0, BLUE);
                        ls_set_color(1, GREEN);
                        ls_set_color(2, RED);
                        ls_set_color(3, WHITE);
                        ls_set_color(4, YELLOW);
                        ls_set_color(5, CYAN);
                        ls_set_color(6, MAGENTA);

                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 7;
                        fc_effect.Now_state = IS_light_scene;

                        break;

                    case 0x16:
                        ls_set_color(0, RED);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                    
                        break;

                    case 0x17:
                        ls_set_color(0, BLUE);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                    
                        break;

                    case 0x18:
                        ls_set_color(0, GREEN);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                        
                        break;
                    case 0x19:
                        
                        ls_set_color(0, CYAN);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                        
                        break;

                    case 0x1a: 
                        
                        ls_set_color(0, YELLOW);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                        
                        break;
                    case 0x1B:
                        
                        ls_set_color(0, MAGENTA);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                    
                        break;
                    case 0x1C:
                        ls_set_color(0, WHITE);
                        fc_effect.dream_scene.change_type = MODE_STROBE;
                        fc_effect.dream_scene.c_n = 1;
                        fc_effect.Now_state = IS_light_scene;
                    
                        break;
                }
                set_fc_effect();
                save_user_data_area3();

            }
    
            //---------------------------------更新RGB-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x01 && LedCommand[2]==0x1e)
            {
                extern void set_static_mode(u8 r, u8 g, u8 b);
                set_ir_auto(IR_PAUSE);
                set_static_mode(LedCommand[3], LedCommand[4], LedCommand[5]);
                save_user_data_area3();
            }
            //---------------------------------调节亮度-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x03 )
            {
                extern void set_bright(u8 b);
                set_bright(LedCommand[2]);
                save_user_data_area3();
                Send_buffer[6] = 0x04;
                Send_buffer[7] = 0x03;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
            }
            //---------------------------------调节速度-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x04 )
            {
                // 范围0-100
                ls_set_speed(LedCommand[2]);
                save_user_data_area3();
                Send_buffer[6] = 0x04;
                Send_buffer[7] = 0x04;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);

            }
            //---------------------------------更改RGB接口-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x05 )
            {
                extern void set_rgb_sequence(u8 s); 
                set_rgb_sequence(LedCommand[2]);
                save_user_data_area3();
                Send_buffer[6] = 0x04;
                Send_buffer[7] = 0x05;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
              
            }
            //---------------------------------灯带长度-----------------------------------
            if(LedCommand[0]==0x04 && LedCommand[1]==0x08 )
            {
                extern void set_ls_lenght(u16 l);
                set_ls_lenght( LedCommand[2]<<8 | LedCommand[3]);
                save_user_data_area3();
                Send_buffer[6] = 0x04;
                Send_buffer[7] = 0x08;
                Send_buffer[8] = fc_effect.led_num>>8;
                Send_buffer[9] = fc_effect.led_num&0xff;
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 10, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,10, ATT_OP_AUTO_READ_CCC);
        

            }
            //---------------------------------外麦声控模式-----------------------------------
            if(LedCommand[0]==0x06 && LedCommand[1]==0x06 )
            {
                extern void set_music_mode(u8 m);
                if( fc_effect.music.m_type == 0x01)  //外模模式
                {
                    set_music_mode(LedCommand[2]);
                    Send_buffer[6] = 0x06;
                    Send_buffer[7] = 0x06;
                    Send_buffer[8] = LedCommand[2];
                    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
                    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                }
            
            }

            //---------------------------------设置灵敏度-----------------------------------
            if(LedCommand[0]==0x2F && LedCommand[1]==0x05 )
            {
                extern void set_music_sensitive(u8 s);
                set_music_sensitive(LedCommand[2]);
                Send_buffer[6] = 0x2F;
                Send_buffer[7] = 0x05;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
            }
            //---------------------------------手机音乐律动-----------------------------------
            if(LedCommand[0]==0x06 && LedCommand[1]==0x04 )
            {
                extern void set_static_mode(u8 r, u8 g, u8 b);
                set_bright(LedCommand[5]);
                set_ir_auto(IR_PAUSE);
                set_static_mode(LedCommand[2], LedCommand[3], LedCommand[4]);
            }
            //-------------------------------- 流星周期 -----------------------------------------
            if(LedCommand[0]==0x2F && LedCommand[1]==0x03)
            {
                extern void set_meteor_p(u8 p);
                set_meteor_p(LedCommand[2]);
                Send_buffer[6] = 0x2F;
                Send_buffer[7] = 0x03;
                Send_buffer[8] = LedCommand[2] ;
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
                save_user_data_area3();//保存参数配置到flash、
            }
            //---------------------------------设备手机麦或者外麦-----------------------------------
            if(LedCommand[0]==0x06 && LedCommand[1]==0x07)
            {
                extern void set_music_type(u8 ty);
                set_music_type(LedCommand[2]);
                Send_buffer[6] = 0x06;
                Send_buffer[7] = 0x07;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);

            }
        }
    }
  }
}

void zd_fb_2_app(u8 *p, u8 len)
{
    uint8_t Send_buffer[50];            //发送缓存
    memcpy(Send_buffer,Ble_Addr, 6);
    memcpy(Send_buffer+6, p, len);
    // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,len+6, ATT_OP_AUTO_READ_CCC);
    ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, len+6, ATT_OP_AUTO_READ_CCC);

}


void tuya_fb_sw_state(void);

void respond_led_strip_dp_query(void)
{
  int res;
  u8 buf[]={DPID_WORK_MODE,	4,	0	,1,	1}; //涂抹模式

}

/* APP数据解析入口函数 */
void parse_led_strip_data(u8 *pBuf, u8 len)
{
  // 重复解析协议此时，避免卡死
  u8 retry = 0;
  led_strip_t strip;

  u16 handler_len;

  unsigned char num = 0;

  /* 涂鸦DP协议解析 */
  handler_len = dp_extract_data_handle(pBuf);
  /* 中道孔明灯协议解析 */
  /* 为兼容全彩的协议 */


  save_user_data_area3();
}


void tuya_fb_sw_state(void)
{
    dp_data_header_t *p_dp;
    u8 dp_data[4+1];
    p_dp = (dp_data_header_t *)dp_data;
    p_dp->id = DPID_SWITCH_LED;
    p_dp->type = DP_TYPE_BOOL;
    p_dp->len = __SWP16(1);
    // dp_data[4] = fc_effect.on_off_flag;
    dp_data[4] = 1; //默认开机


}

/* -------------------------------------DPID_CONTROL_DATA 调节模式----------------------------- */

//调节(只下发)
//备注:类型：字符串;
//Value: 011112222333344445555  ;
//0：   变化方式，0表示直接输出，1表示渐变;
//1111：H（色度：0-360，0X0000-0X0168）;
//2222：S (饱和：0-1000, 0X0000-0X03E8);
//3333：V (明度：0-1000，0X0000-0X03E8);
//4444：白光亮度（0-1000）;
//5555：色温值（0-1000）
// typedef struct
// {
//     uint8_t change_type;        //变化方式，0表示直接输出，1表示渐变;
//     hsv_t c;
//     uint16_t white_b;        //白光亮度
//     uint16_t ct;             //色温
// }dp_control_t; //DPID_CONTROL_DATA


/* -------------------------------------DPID_SCENE_DATA 场景----------------------------- */

//场景(可下发可上报)
//备注:Value: 0011223344445555666677778888
//00：情景号
//11：单元切换间隔时间（0-100）
//22：单元变化时间（0-100）
//33：单元变化模式（0 静态 1 跳变 2 渐变）
//4444：H（色度：0-360，0X0000-0X0168）
//5555：S (饱和：0-1000, 0X0000-0X03E8)
//6666：V (明度：0-1000，0X0000-0X03E8)
//7777：白光亮度（0-1000）
//8888：色温值（0-1000）
//注：数字 1-8 的标号对应有多少单元就有多少组
// #pragma pack (1)
// typedef struct
// {
//     uint8_t sw_time;            //单元切换间隔时间
//     uint8_t chg_time;            //单元变化时间
//     uint8_t change_type;        //单元变化模式（0 静态 1 跳变 2 渐变）
//     hsv_t c;
//     uint16_t white_b;           //白光亮度
//     uint16_t ct;                //色温
// }dp_secene_data_t;                 //场景数据

// typedef struct
// {
//     uint8_t secene_n;           //情景号
//     dp_secene_data_t  *data;        //多组
// }dp_secene_t;
// #pragma pack ()
/*
typedef struct
{
    color_t c[MAX_CORLOR_N];               //颜色池
    uint8_t s;                              //速度
    uint8_t n;                             //当前颜色数量
    uint8_t change_type;                   //变化模式（0 静态 1 跳变 2 渐变）
}fc_effect_t;       //全彩效果

 */
// void dp_secene_data_handle(u8 *pIn)
// {
//     dp_data_header_t *header = (dp_secene_t*) pIn;
//     uint8_t len, i = 0;
//     if(header->type != DP_TYPE_STRING)
//     {
//         #ifdef MY_DEBUG
//         printf("\n dp_secene_data_handle type err");
//         #endif
//         return;
//     }

//     /* 提取涂鸦效果数据 */
//     uint8_t secene_data[ sizeof(dp_data_header_t) * MAX_CORLOR_N + 1];

//     len = string2hex(pIn + sizeof(dp_data_header_t), &secene_data, __SWP16(header->len));

//     #ifdef MY_DEBUG
//     printf("\n secene_data =");
//     printf_buf(secene_data, len);
//     #endif

//     /* 提取颜色数量 */
//     fc_effect.n = (len - 1) / sizeof(dp_secene_data_t);
//     #ifdef MY_DEBUG
//     printf("\n dp_secene_data_t =%d",sizeof(dp_secene_data_t));
//     #endif
//     /* 提取颜色 */
//     dp_secene_data_t *pdata;
//     pdata = (dp_secene_data_t *) (secene_data + 1); //第1个byte是情景号
//     m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));
//     /* 提取速度 */
//     fc_effect.s = pdata->chg_time;
//     fc_effect.change_type = pdata->change_type;

//     #ifdef MY_DEBUG
//     printf("\n rgb =");
//     printf_buf(fc_effect.c[i], 3);
//     printf("\n fc_effect.n = %d",fc_effect.n);


//     #endif
//     i++;
//     while(i < fc_effect.n)
//     {

//         pdata = (dp_secene_data_t *) (secene_data + 1 + sizeof(dp_secene_data_t) * i); //第1个byte是情景号
//         m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));

//         #ifdef MY_DEBUG
//         printf("\n rgb =");
//         printf_buf(fc_effect.c[i], 3);
//         #endif
//         i++;
//     }
// }




