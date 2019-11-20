#ifndef STC89C52_DIGITAL_DS1302_H_
#define STC89C52_DIGITAL_DS1302_H_

#include <at89x51.h>
#include <intrins.h>

#include "common.h"

// extern void _nop_(void);

#define DS1302_CMD_SEC 0x80   // DS1302地址：秒
#define DS1302_CMD_MIN 0x82   // DS1302地址：分
#define DS1302_CMD_HOUR 0x84  // DS1302地址：时
#define DS1302_CMD_DAY 0x86   // DS1302地址：日（本月内）
#define DS1302_CMD_MON 0x88   // DS1302地址：月
#define DS1302_CMD_WDAY 0x8A  // DS1302地址：星期几
#define DS1302_CMD_YEAR 0x8C  // DS1302地址：年

// sbit ACC_0 = ACC ^ 0;
// sbit ACC_7 = ACC ^ 7;

sbit DS1302_SC = P3 ^ 6;  // SCLK
sbit DS1302_IO = P3 ^ 4;  // I/O
sbit DS1302_CE = P3 ^ 5;  // CE

// 合适地使用宏定义展开可以起到类似内联优化的效果
// 避免函数开销的同时，亦能保证可读性

#define DS1302_SEQ_BEGIN() \
    {                      \
        DS1302_CE = 0;     \
        DS1302_SC = 0;     \
        DS1302_CE = 1;     \
    }

#define DS1302_SEQ_END() \
    {                    \
        DS1302_SC = 1;   \
        DS1302_CE = 0;   \
    }

/*
#define DS1302_SEQ_BEGIN() \
    { DS1302_CE = 1; }

#define DS1302_SEQ_END() \
    { DS1302_CE = 0; }
*/

struct DateTime {
    uint year;  // 转换成DS1302表示法需要-2000
    byte mon;
    byte day;
    byte hour;
    byte min;
    byte sec;
};

// DS1302内的数据以BCD码方式存储，处理时注意转换

#define DS1302_BCD_2_DEC(X) (((X)&0x70) >> 4) * 10 + ((X)&0x0F)
#define DS1302_DEC_2_BCD(X) (((X) / 10) << 4 | ((X) % 10))

// BCD转十进制
byte bcd2dec(byte bcd);

// 十进制转BCD
byte dec2bcd(byte dec);

// 向DS1302写入一个请求字节
void ds1302_input_byte(byte dat);

// 从DS1302读取一个答复字节
byte ds1302_output_byte();

// 从DS1302以特定指令读取一个数据字节
byte ds1302_read(byte cmd);

// 向DS1302以特定指令写入一个数据字节
void ds1302_write(byte cmd, byte dat);

// 宏定义：对DS1302启用写保护
#define DS1302_SET_WRITE_PROTECT() ds1302_write(0x8E, 0x80)

// 宏定义：对DS1302禁用写保护
#define DS1302_CLR_WRITE_PROTECT() ds1302_write(0x8E, 0x00)

// TODO: 细致的年/月/日/...的单独设置/读取（友好的用户调用接口）
#define DS1302_GET_YEAR() (bcd2dec(ds1302_read(DS1302_CMD_YEAR)) + 2000)
#define DS1302_GET_MON() (bcd2dec(ds1302_read(DS1302_CMD_MON)))
#define DS1302_GET_DAY() (bcd2dec(ds1302_read(DS1302_CMD_DAY)))
#define DS1302_GET_HOUR() (bcd2dec(ds1302_read(DS1302_CMD_HOUR)) & 0x1F)
#define DS1302_GET_MIN() (bcd2dec(ds1302_read(DS1302_CMD_MIN)))
#define DS1302_GET_SEC() (bcd2dec(ds1302_read(DS1302_CMD_SEC)) & 0x7F)

// TODO: 突发模式读写

// DS1302设置日期时间
void ds1302_set_datetime(struct DateTime* datetime);

// DS1302获取日期时间
void ds1302_get_datetime(struct DateTime* datetime);

// 是否使时钟处于运行状态
//     is_run: 是否运行
void ds1302_run(bit is_run);

#endif
