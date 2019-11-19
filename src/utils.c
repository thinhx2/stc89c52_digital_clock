#include "utils.h"


void delay_5us(u8 n)
{
    do
    {
        _nop_();
        _nop_();
        _nop_();
        n--;
    } while (n);
}
