#include "led_seg.h"

void led_seg_draw_cycle(u8* buf, u8 interval_5us) {
    u8 i;
    for (i = 0; i < LED_SEG_NUM; i++) {
        LED_SEG_DRAW_SHOW(i, buf[i]);
        delay_5us(interval_5us);
        LED_SEG_DRAW_HIDE();
    }
}
