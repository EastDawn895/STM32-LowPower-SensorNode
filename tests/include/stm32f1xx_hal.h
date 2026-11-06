#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
#include <stddef.h>
typedef enum {HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {int x;} UART_HandleTypeDef;
typedef struct {int x;} I2C_HandleTypeDef;
typedef struct {int x;} SPI_HandleTypeDef;
typedef struct {int x;} ADC_HandleTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t);
void HAL_GPIO_WritePin(int,int,int);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef*,uint8_t*,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef*,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef*,uint8_t*,uint16_t,uint32_t);
typedef struct {uint32_t Pin,Mode,Speed,Pull;} GPIO_InitTypeDef;
typedef struct {volatile uint32_t CTRL,CYCCNT;} TestDWT;
typedef struct {volatile uint32_t DEMCR;} TestDebug;
extern TestDWT testDWT;
extern TestDebug testDebug;
extern uint32_t SystemCoreClock;
#define DWT (&testDWT)
#define CoreDebug (&testDebug)
#define CoreDebug_DEMCR_TRCENA_Msk 1u
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define __NOP() (testDWT.CYCCNT+=72u)
#define GPIOB 1
#define GPIO_PIN_8 256
#define GPIO_PIN_9 512
#define GPIO_MODE_OUTPUT_OD 1
#define GPIO_SPEED_FREQ_HIGH 1
#define GPIO_NOPULL 0
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
int HAL_GPIO_ReadPin(int,int);
void HAL_GPIO_Init(int,GPIO_InitTypeDef*);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*,uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*);
#define PWR_MAINREGULATOR_ON 0
#define PWR_SLEEPENTRY_WFI 0
void HAL_PWR_EnterSLEEPMode(int,int);
void HAL_GPIO_TogglePin(int,int);
#endif
