#include "moving_avg.h"

void MovingAvg_Init(MovingAvg_t *f) {
    /* 四个字段全部清零 */
    f->count = 0;
    f->idx = 0;
    f->sum = 0;
}

int32_t MovingAvg_Update(MovingAvg_t *f, int32_t sample) {
    /* ① 若 count == N（已满）：sum -= buf[idx];   ← 减掉最老的
       ② buf[idx] = sample;  sum += sample;
       ③ idx = (idx + 1) % MOVING_AVG_WIN;        ← 回绕
       ④ count < N 时 count++                     ← 预热期
       ⑤ return sum / count;                      ← 注意预热期分母是 count 不是 N */
    if (f->count == MOVING_AVG_WIN) {
        f->sum -= f->buf[f->idx];
    }else {
        f->count++;
    }
    f->buf[f->idx] = sample;
    f->sum += sample;
    f->idx = (f->idx + 1) % MOVING_AVG_WIN;
    return f->sum/f->count;
}
