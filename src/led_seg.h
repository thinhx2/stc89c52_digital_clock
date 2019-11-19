// LED Segment Displays: 74HC245

#ifndef STC89C52_DIGITAL_CLOCK_LED_SEG_H_
#define STC89C52_DIGITAL_CLOCK_LED_SEG_H_

#include "common.h"
#include "utils.h"

#define SEG_OFF 0x00
#define SEG_NEG 0x40
#define SEG_DOT 0x80
#define SEG_OUT P0

#define LED_SEG_NUM 8

sbit LSA = P2 ^ 2;
sbit LSB = P2 ^ 3;
sbit LSC = P2 ^ 4;

// u8 code led_seg_code[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
//                           0x6d, 0x7d, 0x07, 0x7f, 0x6f};
static u8 led_seg_code[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                            0x6d, 0x7d, 0x07, 0x7f, 0x6f};

// 宏定义：LED数码管显示宏，每次调用刷新指定数码管1次
//     dig:     指定要刷新的数码管编号
//     code:    该指定数码管应显示的编码
#define LED_SEG_DRAW_SHOW(dig, code) \
    {                                \
        LSA = (dig)&0x01;            \
        LSB = (dig)&0x02;            \
        LSC = (dig)&0x04;            \
        SEG_OUT = (code);                \
    }

// 宏定义：LED数码管熄灭宏，每次调用将使当前数码管编码设置为熄灭编码
#define LED_SEG_DRAW_HIDE() \
    { SEG_OUT = SEG_OFF; }

// LED数码管刷新函数，每次调用刷新一整组数码管1次
//     buf:         LED_SEG_NUM字节的缓冲区，以读取每个数码管的码
//     interval_5us:设置每次数码管切换时等待的时间：interval_5us * 5us
void led_seg_draw_cycle(u8* buf, u8 interval_5us);

#endif
