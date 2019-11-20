#include "ds1302.h"

byte bcd2dec(byte bcd) { return ((bcd & 0x70) >> 4) * 10 + (bcd & 0x0F); }

byte dec2bcd(byte dec) { return ((dec / 10) << 4 | (dec % 10)); }

// 这些被注释的代码的原理是利用ACC累加器处理各个位，书上的做法
// 强烈建议不要使用，尽管代码上似乎看起来更好
// C语言屏蔽了底层汇编的实际运行情况，所以直接操作ACC将是不可靠的
// 尤其是在有中断切出等情况下，切回当前指令地址时很可能ACC已经被改动

// void ds1302_input_byte(byte dat) {
//     byte i;
//     ACC = dat;
//     for (i = 0; i < 8; i++) {
//         DS1302_IO = ACC_0;
//         _nop_();
//         DS1302_SC = 1;
//         _nop_();
//         DS1302_SC = 0;
//         _nop_();
//         ACC >>= 1;
//     }
// }

// byte ds1302_output_byte() {
//     byte i;
//     for (i = 0; i < 8; i++) {
//         ACC >>= 1;
//         ACC_7 = DS1302_IO;
//         _nop_();
//         DS1302_SC = 1;
//         _nop_();
//         DS1302_SC = 0;
//         _nop_();
//     }
//     return ACC;
// }

// 只需要基础的位运算知识，利用掩码就可以解决这个问题

void ds1302_input_byte(byte dat) {
    byte mask;
    for (mask = 0x01; mask; mask <<= 1) {
        DS1302_IO = ((dat & mask) != 0);
        // _nop_();
        DS1302_SC = 1;
        // _nop_();
        DS1302_SC = 0;
        // _nop_();
    }
}

byte ds1302_output_byte() {
    byte mask, dat = 0;
    for (mask = 0x01; mask; mask <<= 1) {
        if (DS1302_IO) {
            dat |= mask;
        }
        // _nop_();
        DS1302_SC = 1;
        // _nop_();
        DS1302_SC = 0;
        // _nop_();
    }
    return dat;
}

byte ds1302_read(byte cmd) {
    byte dat;
    DS1302_SEQ_BEGIN();

    ds1302_input_byte(cmd | 0x01);
    dat = ds1302_output_byte();

    DS1302_SEQ_END();
    DS1302_IO = 0;
    return dat;
}

void ds1302_write(byte cmd, byte dat) {
    DS1302_SEQ_BEGIN();

    ds1302_input_byte(cmd);
    ds1302_input_byte(dat);

    DS1302_SEQ_END();
    DS1302_IO = 0;
}

void ds1302_set_datetime(struct DateTime* datetime) {
    byte ch = bcd2dec(ds1302_read(DS1302_CMD_SEC)) & 0x80;
    // 读取小时命令返回的字节的高3位
    byte hour_high = bcd2dec(ds1302_read(DS1302_CMD_HOUR)) & 0xE0;

    DS1302_CLR_WRITE_PROTECT();

    // if (ds1302_read(DS1302_CMD_SEC) & 0x80) {
    //     // 初始化时间
    //     ds1302_cmd_set(DS1302_CMD_SEC, 0);
    // }
    ds1302_write(DS1302_CMD_YEAR, dec2bcd((byte)(datetime->year - 2000)));
    ds1302_write(DS1302_CMD_MON, dec2bcd(datetime->mon));
    ds1302_write(DS1302_CMD_DAY, dec2bcd(datetime->day));
    // TODO: 实现12/24小时制切换时需要关照此处
    ds1302_write(DS1302_CMD_HOUR, dec2bcd(datetime->hour | hour_high));
    ds1302_write(DS1302_CMD_MIN, dec2bcd(datetime->min));
    ds1302_write(DS1302_CMD_SEC,
                 dec2bcd(ch ? (datetime->sec | 0x80) : (datetime->sec & 0x7F)));
    // 设置秒时注意保留原先的ch位

    DS1302_SET_WRITE_PROTECT();
}

void ds1302_get_datetime(struct DateTime* datetime) {
    datetime->year = bcd2dec(ds1302_read(DS1302_CMD_YEAR)) + 2000;
    datetime->mon = bcd2dec(ds1302_read(DS1302_CMD_MON));
    datetime->day = bcd2dec(ds1302_read(DS1302_CMD_DAY));
    // TODO: 实现12/24小时制切换时需要关照此处
    datetime->hour = bcd2dec(ds1302_read(DS1302_CMD_HOUR)) & 0x1F;
    datetime->min = bcd2dec(ds1302_read(DS1302_CMD_MIN));
    datetime->sec = bcd2dec(ds1302_read(DS1302_CMD_SEC)) & 0x7F;
}

void ds1302_run(bit is_run) {
    byte sec = bcd2dec(ds1302_read(DS1302_CMD_SEC));
    DS1302_CLR_WRITE_PROTECT();
    ds1302_write(DS1302_CMD_SEC,
                 is_run ? dec2bcd(sec & 0x7F) : dec2bcd(sec | 0x80));
    DS1302_SET_WRITE_PROTECT();
}
