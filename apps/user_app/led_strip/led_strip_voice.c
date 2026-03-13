#include "led_strip_voice.h"
#include "asm/adc_api.h"
#include "led_strip_drive.h"

MIC_OFFON MIC_ENABLE;           //0-关闭麦克风，1-开启麦克风
extern u16 check_mic_adc(void);
#define SAMPLE_N 20
u8 i,j;
u32 adc,adc_av,adc_all;
u16 adc_v[SAMPLE_N];    //记录20个ADC值
u32 adc_avrg[10];        //记录5个平均值
u32 adc_total[15];// __attribute__((aligned(4)));

u16 find_max(void)
{
    u8 i;
    u32 max = 0;
    u8 max_index;

    for(i=0; i<SAMPLE_N; i++)
    {
        if(adc_total[i] > max) max = adc_total[i];
    }
    for(i=0; i<SAMPLE_N; i++)
    {
        if(adc_total[i] == max)
        {

            break;
        }
    }
    if(i==10)
    {
        if(adc_total[10] / SAMPLE_N > adc_av*1.2 )
        return 1000;
    }
    return 0;
}

u8 adc_v_n, adc_avrg_n, adc_total_n;
u32 adc_sum = 0, adc_sum_n = 0;
extern uint8_t met_trg;
extern uint8_t trg_en;
extern void set_music_oc_trg(u8 p);

void sound_handle(void)
{
extern u32 adc_get_value(u32 ch);

    u16 adc;
    u8 i,trg,trg_v;
    u32 adc_all, adc_ttl;

    extern u32 adc_sample(u32 ch);
    // 记录adc值
    adc = adc_get_value(MIC_AD_CH);

    // adc = adc_sample(AD_CH_PA8);
    // printf("adc = %d",adc);
    if(fc_effect.Now_state == IS_light_music)
    {
        if(adc < 1000)
        {

            if(adc_sum_n < 2000)
            {
                adc_sum_n++;
            }
            if(adc_sum_n == 2000)
            {
                if(adc / (adc_sum/adc_sum_n) > 3) return ; //adc突变，大于平均值的3倍，丢弃改值
                adc_sum = adc_sum - adc_sum/adc_sum_n;
            }
            adc_sum+=adc;

            adc_v_n %= SAMPLE_N;
            adc_v[adc_v_n] = adc;
            adc_v_n++;
            adc_all = 0;
            for(i=0; i<SAMPLE_N; i++)
            {
                adc_all += adc_v[i];
            }

            adc_avrg_n %= 10;
            adc_avrg[adc_avrg_n] = adc_all / SAMPLE_N;
            adc_avrg_n++;
            // printf("%d,",adc_all / SAMPLE_N);
            adc_ttl = 0;
            for(i=0; i<10; i++)
            {
                adc_ttl += adc_avrg[i];
            }
            memmove((u8*)adc_total, (u8*)adc_total+4, 14*4);
            adc_total[14] = adc_ttl/10; //总数平均值
            // 查找峰值
            trg = 0;     
            if(adc_sum_n!=0)
            {
                extern void set_mss(uint16_t s);
                set_mss(adc + (adc) * fc_effect.music.s / 100  );
                if(adc * fc_effect.music.s / 100 > adc_sum/adc_sum_n)
                {
                    // printf("\n adc=%d",adc);
                    // printf("\n adc_sum/adc_sum_n=%d",adc_sum/adc_sum_n);

                    // set_music_oc_trg((adc - adc_sum/adc_sum_n)*100 * fc_effect.music.s / 100/(adc_sum/adc_sum_n));

                    extern void WS2812FX_trg(void);
                    if(fc_effect.led_num < 90) //太多点数处理不过来
                        WS2812FX_trg();
                    // extern void set_music_fs_trg(u8 p);
                    // set_music_fs_trg((adc - adc_sum/adc_sum_n)*100 * fc_effect.music.s / 100/(adc_sum/adc_sum_n));

                    trg = 200;
                    met_trg = 1;
                    trg_en = 1;


                }

                if(adc > adc_sum/adc_sum_n)
                {
                    set_music_oc_trg((adc - adc_sum/adc_sum_n)*100 * fc_effect.music.s / 100/(adc_sum/adc_sum_n));
                    extern void set_music_fs_trg(u8 p);
                    set_music_fs_trg((adc - adc_sum/adc_sum_n)*100 * fc_effect.music.s / 100/(adc_sum/adc_sum_n));

                }
            }

        }

    }
 
}