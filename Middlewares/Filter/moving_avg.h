#ifndef MOVING_AVG_H
#define MOVING_AVG_H
#include <stdint.h>

#define MOVING_AVG_WIN  8u   /* 窗口长度，先定 8 */

typedef struct {
    int32_t buf[MOVING_AVG_WIN]; /* 环形缓冲：最近 N 个样本 */
    uint8_t idx;                 /* 下一个写入位置 */
    uint8_t count;               /* 已存入的有效样本数（预热期 < N） */
    int32_t sum;                 /* 窗口内样本之和 */
} MovingAvg_t;

void    MovingAvg_Init(MovingAvg_t *f);
int32_t MovingAvg_Update(MovingAvg_t *f, int32_t sample);

#endif