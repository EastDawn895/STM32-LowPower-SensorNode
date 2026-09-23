#include <stdio.h>
#include <stdlib.h>
#include "moving_avg.h"

int main(void) {
    MovingAvg_t f;

    /* 用例1：恒定输入 → 预热爬升，8 个样本后稳定在 1000 */
    MovingAvg_Init(&f);
    printf("--- case1: constant 1000 ---\n");
    for (int i = 0; i < 20; i++) {
        printf("in=%d out=%d\n", 1000, MovingAvg_Update(&f, 1000));
    }

    /* 用例2：先灌 10 个 0，再灌 10 个 1000 → 输出应在 8 步内从 0 平滑爬到 1000 */
    printf("--- case2:  10*0   10*1000 ---\n");
    for (int i = 0; i < 10; i++) {
        printf("in=%d out=%d\n", 0, MovingAvg_Update(&f, 0));
    }
    for (int i = 0; i < 10; i++) {
        printf("in=%d out=%d\n", 1000, MovingAvg_Update(&f, 1000));
    }

    /* 用例3：灌 1000 + rand()%201 - 100 → 观察输入抖动 ±100，输出抖动缩到多少 */
    printf("--- case3:  1000 + rand() % 201 ---\n");
    for (int i = 0; i < 20; i++) {
        int32_t num = 1000 + rand() % 201;
        printf("in=%d out=%d\n", num, MovingAvg_Update(&f, num));
    }
    return 0;
}
