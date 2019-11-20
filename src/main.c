#include <at89x51.h>

#include "common.h"
#include "ds1302.h"
#include "keyboard.h"
#include "led_seg.h"
#include "utils.h"

// 作者：张志林
// 日期：2019/11/20

// 目标机器：普中HC6800-ES V2.0
// 目标MCU：STC89C52
// 晶体震荡频率：12MHz
// 普中HC6800-ES V2.0只有2个独立按键（K3、K4）支持中断方式

// 振荡器频率：12MHz
#define SYS_CLK_XTAL 12
// 系统时钟频率：振荡器频率/12 = 1
#define SYS_CLK_MAIN 1

// 屏蔽P3的高4位数据，试验机只有4颗键，高4位数据会影响实际判断
#define KEY4_MASK(X) ((~X) & 0x0F)

#define SET_T0(L, H) \
    {                \
        TL0 = (L);   \
        TH0 = (H);   \
    }

#define SET_LED_SEG_BUF_ERROR()                                     \
    {                                                               \
        led_seg_buf[0] = 0x50;                                      \
        led_seg_buf[1] = 0x5C;                                      \
        led_seg_buf[2] = 0x50;                                      \
        led_seg_buf[3] = 0x50;                                      \
        led_seg_buf[4] = 0x79;                                      \
        led_seg_buf[5] = led_seg_buf[6] = led_seg_buf[7] = SEG_OFF; \
    }

bit k3_click = 0;  // 表示是否最近一次按下了K3按键，需要手动置0，这是特定设计
int t0_count = 0;

byte clock_mode = 0;
#define CLOCK_MODE_NUM 3
#define CLOCK_MODE_MOD(X) ((X) % CLOCK_MODE_NUM)
#define CLOCK_MODE_NEXT() \
    { clock_mode = CLOCK_MODE_MOD((clock_mode + 1)); }

// 宏定义：判断YEAR是否为闰年
#define IS_LEAP_YEAR(YEAR) ((!(YEAR % 100)) ? (!(YEAR % 400)) : (!(YEAR % 4)))
// 每个月的天数（2月份需特判）
byte all_mon_day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
byte led_seg_buf[8];

struct DateTime datetime = {2005, 12, 31, 23, 59, 58};
// struct DateTime datetime = {2005, 12, 31, 11, 59, 58};

struct ModeInfo {
    byte mode_0_inited;
    byte mode_1_inited;
    byte mode_2_inited;
    byte mode_3_inited;
} modeinfo = {0, 0, 0, 0};

void main(void) {
    // u8 _led_code = SEG_OFF;
    uint year;
    byte hour;
    byte no_or_up_or_down;  // 取值：0、1、2，分别表示：未按下、已按上、已按下

    bit change_date_light, change_date_press;
    byte change_date_ticks, change_date_select;
    bit change_time_light, change_time_press;
    byte change_time_ticks, change_time_select;

    clock_mode = 0;
    k3_click = 0;

    ds1302_run(0);
    ds1302_set_datetime(&datetime);
    ds1302_run(1);

    EA = 1;       // 启用总中断
    TMOD = 0x01;  // T0中断采用方式1
    IE |= 0x01;   // 启用INT0中断
    IP |= 0x01;   // 启用INT0中断优先

    while (TRUE) {
        // if (k3_click) {
        //     _led_code = led_seg_code[clock_mode];
        //     k3_click = 0;
        // }
        // LED_SEG_DRAW_SHOW(0, _led_code);

        if (clock_mode == 0) {
            if (!modeinfo.mode_0_inited) {
                // 保证从其它模式进入该模式时全部数据都及时更新
                // 读取全部日期
                datetime.day = DS1302_GET_DAY();
                datetime.mon = DS1302_GET_MON();
                datetime.year = DS1302_GET_YEAR();
                // 设置整组LED
                led_seg_buf[0] = led_seg_code[datetime.day % 10];
                led_seg_buf[1] = led_seg_code[datetime.day / 10];
                led_seg_buf[2] = led_seg_code[datetime.mon % 10] | SEG_DOT;
                led_seg_buf[3] = led_seg_code[datetime.mon / 10];
                year = datetime.year;
                led_seg_buf[4] = led_seg_code[year % 10] | SEG_DOT;
                year /= 10;
                led_seg_buf[5] = led_seg_code[year % 10];
                year /= 10;
                led_seg_buf[6] = led_seg_code[year % 10];
                year /= 10;
                led_seg_buf[7] = led_seg_code[year % 10];
                // 设置与日期修改相关的参数
                change_date_light = 1;  // select选定的时间单位目前是否应该点亮
                change_date_ticks = 100 - 1;  // 点亮的tick次数
                change_date_select = 0;       // 默认不处于修改状态
                // 本模式已初始化
                modeinfo.mode_0_inited = 1;
            } else {
                if (!change_date_select) {
                    KEY4_PORT = 0xFF;
                    if (!KEY_K4) {
                        delay_5us(1500);
                        if (!KEY_K4) {
                            change_date_ticks = 100;
                            change_date_press = 1;
                            no_or_up_or_down = 0;
                            change_date_select = (change_date_select + 1) % 4;
                        }
                    }

                    // 局部更新：更长级别的时间单位并不是每次都更新
                    // 只在前一级的时间单位发生溢出的情况下，才考虑本级时间的获取是否应该更新
                    datetime.day = DS1302_GET_DAY();
                    led_seg_buf[0] = led_seg_code[datetime.day % 10];
                    led_seg_buf[1] = led_seg_code[datetime.day / 10];
                    if (datetime.day == 1) {
                        datetime.mon = DS1302_GET_MON();
                        led_seg_buf[2] =
                            led_seg_code[datetime.mon % 10] | SEG_DOT;
                        led_seg_buf[3] = led_seg_code[datetime.mon / 10];
                        if (datetime.mon == 1) {
                            datetime.year = DS1302_GET_YEAR();
                            year = datetime.year;
                            led_seg_buf[4] = led_seg_code[year % 10] | SEG_DOT;
                            year /= 10;
                            led_seg_buf[5] = led_seg_code[year % 10];
                            year /= 10;
                            led_seg_buf[6] = led_seg_code[year % 10];
                            year /= 10;
                            led_seg_buf[7] = led_seg_code[year % 10];
                        }
                    }
                } else {
                    if (!change_date_ticks) {
                        change_date_light = !change_date_light;
                        change_date_ticks = 100;
                        change_date_press = 0;
                        no_or_up_or_down = 0;
                    }

                    // 扫描式检测独立按键
                    if (!change_date_press) {
                        KEY4_PORT = 0xFF;
                        switch ((~KEY4_PORT) & 0x0F) {
                            case 0x1:
                                delay_5us(1500);
                                if (!KEY_K1) {
                                    no_or_up_or_down = 1;
                                }
                                change_date_press = 1;
                                break;
                            case 0x2:
                                delay_5us(1500);
                                if (!KEY_K2) {
                                    no_or_up_or_down = 2;
                                }
                                change_date_press = 1;
                                break;
                            case 0x8:
                                delay_5us(1500);
                                if (!KEY_K4) {
                                    change_date_ticks = 100;
                                    change_date_select =
                                        (change_date_select + 1) % 4;
                                }
                                change_date_press = 1;
                                break;
                            default:
                                break;
                        }
                    }

                    if (change_date_select == 1) {  // 修改年的状态
                        datetime.mon = DS1302_GET_MON();
                        datetime.day = DS1302_GET_DAY();
                        if (no_or_up_or_down == 1) {  // 按下UP（K1）键
                            datetime.year = (datetime.year > 2099)
                                                ? 2000
                                                : (datetime.year + 1);
                        } else if (no_or_up_or_down == 2) {  // 按下DOWN（K2）键
                            datetime.year = (datetime.year < 2001)
                                                ? 2100
                                                : (datetime.year - 1);
                        }
                        // TODO: 立即设置新的年份
                        no_or_up_or_down = 0;
                    } else if (change_date_select == 2) {  // 修改月的状态
                        datetime.year = DS1302_GET_YEAR();
                        datetime.day = DS1302_GET_DAY();
                        if (no_or_up_or_down == 1) {  // 按下UP（K1）键
                            datetime.mon = (datetime.mon + 1) % 13;
                        } else if (no_or_up_or_down == 2) {  // 按下DOWN（K2）键
                            datetime.mon =
                                (datetime.mon < 2) ? 12 : (datetime.mon - 1);
                        }
                        // TODO: 立即设置新的月份
                        no_or_up_or_down = 0;
                    } else if (change_date_select == 3) {  // 修改日的状态
                        datetime.year = DS1302_GET_YEAR();
                        datetime.mon = DS1302_GET_MON();
                        if (no_or_up_or_down == 1) {  // 按下UP（K1）键
                            if (datetime.mon == 2) {
                                datetime.day =
                                    (datetime.day + 1) %
                                    (IS_LEAP_YEAR(datetime.year) ? 30 : 29);
                            } else {
                                datetime.day = (datetime.day + 1) %
                                               (all_mon_day[datetime.mon] + 1);
                            }
                        } else if (no_or_up_or_down == 2) {  // 按下DOWN（K2）键
                            if (datetime.mon == 2) {
                                datetime.day =
                                    (datetime.day < 2)
                                        ? (IS_LEAP_YEAR(datetime.year) ? 29
                                                                       : 28)
                                        : (datetime.day - 1);
                            } else {
                                datetime.day = (datetime.day < 2)
                                                   ? all_mon_day[datetime.mon]
                                                   : (datetime.day - 1);
                            }
                        }
                        // TODO: 立即设置新的日
                        no_or_up_or_down = 0;
                    }

                    if (change_date_select != 3 || change_date_light) {
                        led_seg_buf[0] = led_seg_code[datetime.day % 10];
                        led_seg_buf[1] = led_seg_code[datetime.day / 10];
                    } else {
                        led_seg_buf[0] = led_seg_buf[1] = SEG_OFF;
                    }
                    if (change_date_select != 2 || change_date_light) {
                        led_seg_buf[2] =
                            led_seg_code[datetime.mon % 10] | SEG_DOT;
                        led_seg_buf[3] = led_seg_code[datetime.mon / 10];
                    } else {
                        led_seg_buf[2] = SEG_OFF | SEG_DOT;
                        led_seg_buf[3] = SEG_OFF;
                    }
                    if (change_date_select != 1 || change_date_light) {
                        year = datetime.year;
                        led_seg_buf[4] = led_seg_code[year % 10] | SEG_DOT;
                        year /= 10;
                        led_seg_buf[5] = led_seg_code[year % 10];
                        year /= 10;
                        led_seg_buf[6] = led_seg_code[year % 10];
                        year /= 10;
                        led_seg_buf[7] = led_seg_code[year % 10];
                    } else {
                        led_seg_buf[4] = SEG_OFF | SEG_DOT;
                        led_seg_buf[5] = led_seg_buf[6] = led_seg_buf[7] =
                            SEG_OFF;
                    }

                    change_date_ticks--;
                }
            }
            led_seg_draw_cycle(led_seg_buf, 100);
        } else if (clock_mode == 1) {
            if (!modeinfo.mode_1_inited) {
                // 保证从其它模式进入该模式时全部数据都及时更新
                // 读取全部时间
                datetime.sec = DS1302_GET_SEC();
                datetime.min = DS1302_GET_MIN();
                datetime.hour = DS1302_GET_HOUR();
                // 设置整组LED
                led_seg_buf[0] = led_seg_code[datetime.sec % 10];
                led_seg_buf[1] = led_seg_code[datetime.sec / 10];
                led_seg_buf[2] = led_seg_code[datetime.min % 10] | SEG_DOT;
                led_seg_buf[3] = led_seg_code[datetime.min / 10];
                hour = datetime.hour;
                led_seg_buf[4] = led_seg_code[hour % 10] | SEG_DOT;
                hour /= 10;
                led_seg_buf[5] = led_seg_code[hour % 10];
                led_seg_buf[6] = SEG_OFF;
                led_seg_buf[7] = SEG_OFF;
                // 本模式已初始化
                modeinfo.mode_1_inited = 1;
            } else {
                // 判断需要发生更新的流程设计在多数情况下是合理的
                // 不必担心局部更新会导致某些情况下错过特定刷新条件导致该刷新的时间（时/分/秒）没被刷新
                // 但前提是，中断切出本模式的时间尽可能低于1s，否则有时候可能会导致分钟未能更新的bug
                // 设计时尚未发生以上提到的这种情况，这里先提个醒，防止将来踩坑
                // 制成实际产品时不应有任何致命问题
                // 当然，预防这种中断切出超过1s的情况目前想到了一种野路子做法：
                // 多判断sec == 1, sec == 2, ....的情况
                datetime.sec = DS1302_GET_SEC();
                led_seg_buf[0] = led_seg_code[datetime.sec % 10];
                led_seg_buf[1] = led_seg_code[datetime.sec / 10];
                if (!datetime.sec) {  // datetime.sec == 0，需要刷新分钟
                    datetime.min = DS1302_GET_MIN();
                    led_seg_buf[2] = led_seg_code[datetime.min % 10] | SEG_DOT;
                    led_seg_buf[3] = led_seg_code[datetime.min / 10];
                    if (!datetime.min) {  // datetime.min == 0，需要刷新小时
                        datetime.hour = DS1302_GET_HOUR();
                        hour = datetime.hour;
                        led_seg_buf[4] = led_seg_code[hour % 10] | SEG_DOT;
                        hour /= 10;
                        led_seg_buf[5] = led_seg_code[hour % 10];
                    }
                }
            }
            led_seg_draw_cycle(led_seg_buf, 100);
        } else {  // 未知模式
            SET_LED_SEG_BUF_ERROR();
            led_seg_draw_cycle(led_seg_buf, 100);
        }
    }
}

// 中断：外部中断INT0
void on_change_mode() interrupt 0 {
    u8 key_detect;
    EX0 = 0;  // 禁用INT0中断
    ET0 = 0;  // 禁用T0中断
    k3_click = 0;
    P3 = 0xFF;
    key_detect = KEY4_MASK(P3);
    delay_5us(2000);  // 延时10ms
    if (key_detect == KEY4_MASK(P3)) {
        if (key_detect & 0x04) {
            k3_click = 1;
            switch (clock_mode) {
                case 0:
                    // 使mode0处在未初始化状态
                    modeinfo.mode_0_inited = 0;
                    break;
                case 1:
                    // 使mode1处在未初始化状态
                    modeinfo.mode_1_inited = 0;
                    break;
                default:
                    break;
            }
            CLOCK_MODE_NEXT();
        }
    }
    // EX0 = 1;  // 启用INT0中断

    // 设置t0_count * 50ms的按键时间间隔
    t0_count = 4;        // 装填T0执行次数
    SET_T0(0xB0, 0x3C);  // 装填T0时间
    ET0 = 1;  // 启用T0中断，在T0中断触发时恢复该按键的INT0中断
    TR0 = 1;  // 启动T0定时器
}

// 中断：定时器中断T0
void on_recover_int0() interrupt 1 {
    if (t0_count-- > 0) {
        SET_T0(0xB0, 0x3C);
    } else {
        TR0 = 0;  // 停止T0定时器
        ET0 = 0;  // 禁用T0中断
        EX0 = 1;  // 恢复INT0中断
    }
}
