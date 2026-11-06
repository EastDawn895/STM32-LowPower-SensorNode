#include <assert.h>
#include <limits.h>
#include "moving_avg.h"
int main(void) {
    MovingAvg_t f; MovingAvg_Init(&f);
    for (int i=0;i<20;i++) assert(MovingAvg_Update(&f,1000)==1000);
    for (int i=1;i<=8;i++) assert(MovingAvg_Update(&f,0)==(8-i)*125);
    for (int i=0;i<1000;i++) assert(MovingAvg_Update(&f,0)==0);
    MovingAvg_Init(&f);
    for(int i=0;i<100;i++)assert(MovingAvg_Update(&f,INT32_MAX)==INT32_MAX);
    MovingAvg_Init(&f);
    for(int i=0;i<100;i++)assert(MovingAvg_Update(&f,INT32_MIN)==INT32_MIN);
    return 0;
}
