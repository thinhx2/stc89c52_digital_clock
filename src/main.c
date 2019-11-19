#include <at89x51.h>

#include "common.h"
#include "led_seg.h"
#include "utils.h"

// 目标机器：普中HC6800-ES V2.0
// 目标MCU：STC89C52
// 晶体震荡频率：12MHz

// 普中HC6800-ES V2.0只有2个独立按键（K3、K4）支持中断方式

// 屏蔽P3的高4位数据，试验机只有4颗键，高4位数据会影响实际判断
#define KEY4_MASK(X) ((~X) & 0x0F)

#define SET_T0(L, H) \
    {                \
        TL0 = (L);   \
        TH0 = (H);   \
    }

bit k3_click = 0;  // 表示是否最近一次按下了K3按键，需要手动置0，这是特定设计
int t0_count = 0;

void main(void) {
    u8 _led_code = SEG_OFF;
    u8 _val_test = 0;

    k3_click = 0;

    EA = 1;       // 启用总中断
    TMOD = 0x01;  // T0中断采用方式1
    IE |= 0x01;   // 启用INT0中断
    IP |= 0x01;   // 启用INT0中断优先

    while (TRUE) {
        if (k3_click) {
            _led_code = led_seg_code[(++_val_test % 10)];
            k3_click = 0;
        }

        LED_SEG_DRAW_SHOW(0, _led_code);
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
