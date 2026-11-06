/* SSD1306 PB8/PB9 open-drain I2C. Font adapted from Jiangxie STM32 tutorial.
 * Drawing is RAM-only; Service sends one 16-byte chunk, with ACK/deadline checks.
 */
#include "oled.h"
#include "oled_font.h"
#include <string.h>
static uint8_t pixels[1024];
static uint64_t dirty;
static HAL_StatusTypeDef health = HAL_ERROR;
static uint32_t transactionStart, retryTick;
#define SCL(v) HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,(v)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define SDA(v) HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,(v)?GPIO_PIN_SET:GPIO_PIN_RESET)

static void delay5(void)
{
    uint32_t start = DWT->CYCCNT, cycles = SystemCoreClock / 200000u;
    /* Also bounded if the debug cycle counter is unavailable/frozen. */
    for (unsigned n = 0; n < 10000u; n++)
    {
        if (DWT->CYCCNT - start >= cycles)return;
        __NOP();
    }
    health = HAL_ERROR;
}

static uint8_t high(void)
{
    SCL(1);
    while (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8) != GPIO_PIN_SET)
    {
        if (HAL_GetTick() - transactionStart >= 5u)
        {
            health = HAL_TIMEOUT;
            return 0;
        }
    }
    delay5();
    return (uint8_t)(health == HAL_OK);
}

static uint8_t send(uint8_t b)
{
    if (health != HAL_OK || HAL_GetTick() - transactionStart >= 5u)
    {
        health = HAL_TIMEOUT;
        return 0;
    }
    for (unsigned i = 0; i < 8; i++)
    {
        SCL(0);
        SDA((b&0x80u)!=0);
        delay5();
        if (!high())return 0;
        b = (uint8_t)(b << 1);
    }
    SCL(0);
    SDA(1);
    delay5();
    if (!high())return 0;
    uint8_t ack = (uint8_t)(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == GPIO_PIN_RESET);
    SCL(0);
    delay5();
    if (!ack)health = HAL_ERROR;
    return ack;
}

static void packet(uint8_t control, const uint8_t* b, unsigned len)
{
    if (health != HAL_OK)return;
    transactionStart = HAL_GetTick();
    SDA(1);
    if (!high())goto stop;
    if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) != GPIO_PIN_SET)
    {
        health = HAL_ERROR;
        goto stop;
    }
    SDA(0);
    delay5();
    SCL(0);
    if (!send(0x78) || !send(control))goto stop;
    for (unsigned i = 0; i < len; i++)if (!send(b[i]))break;
stop:
    SCL(0);
    SDA(0);
    delay5();
    /* No extra wait on failure. Release both lines even on a stuck bus. */
    SCL(1);
    delay5();
    SDA(1);
    delay5();
    retryTick = HAL_GetTick();
}

static void setup(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_OUTPUT_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &g);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    SCL(1);
    SDA(1);
    health = HAL_OK;
    static const uint8_t init[] = {
        0xAE, 0x20, 0x02, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0, 0x40, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
        0x30, 0xA4, 0xA6, 0x8D, 0x14, 0xAF
    };
    packet(0, init, sizeof init);
    dirty = UINT64_MAX;
}

void OLED_Init(void)
{
    HAL_Delay(100);
    memset(pixels, 0, sizeof pixels);
    setup();
}

HAL_StatusTypeDef OLED_GetStatus(void) { return health; }

void OLED_Service(void)
{
    if (health != HAL_OK)
    {
        if (HAL_GetTick() - retryTick >= 1000u)setup();
        return;
    }
    if (!dirty)return;
    unsigned chunk = 0;
    while (!(dirty & (UINT64_C(1) << chunk)))chunk++;
    unsigned column = (chunk % 8u) * 16u, page = chunk / 8u;
    uint8_t cmd[3] = {(uint8_t)(0xB0u | page), (uint8_t)(0x10u | (column >> 4)), (uint8_t)(column & 15u)};
    packet(0, cmd, 3);
    packet(0x40, &pixels[chunk * 16u], 16);
    if (health == HAL_OK)dirty &= ~(UINT64_C(1) << chunk);
}

void OLED_Clear(void)
{
    memset(pixels, 0, sizeof pixels);
    dirty = UINT64_MAX;
}

void OLED_ShowChar(uint8_t line, uint8_t col, char ch)
{
    if (line < 1u || line > 4u || col < 1u || col > 16u || (unsigned char)ch < 32u || (unsigned char)ch > 126u)return;
    unsigned top = ((unsigned)line - 1u) * 256u + ((unsigned)col - 1u) * 8u;
    for (unsigned i = 0; i < 16; i++)
    {
        unsigned addr = top + (i < 8u ? i : 128u + i - 8u);
        uint8_t value = OLED_F8x16[(unsigned char)ch - 32u][i];
        if (pixels[addr] != value)
        {
            pixels[addr] = value;
            dirty |= UINT64_C(1) << (addr / 16u);
        }
    }
}

void OLED_ShowString(uint8_t line, uint8_t col, const char* s)
{
    if (!s || col < 1u || col > 16u)return;
    for (unsigned i = 0; i < 17u - col && s[i]; i++)OLED_ShowChar(line, (uint8_t)(col + i), s[i]);
}

static void number(uint8_t line, uint8_t col, uint32_t n, uint8_t len, unsigned base)
{
    static const char digits[] = "0123456789ABCDEF";
    char b[17];
    if (!len || len > 16u || col < 1u || (unsigned)col + len > 17u)return;
    b[len] = 0;
    for (unsigned i = len; i > 0; i--)
    {
        b[i - 1u] = digits[n % base];
        n /= base;
    }
    OLED_ShowString(line, col, b);
}

void OLED_ShowNum(uint8_t l, uint8_t c, uint32_t n, uint8_t len) { if (len <= 10u)number(l, c, n, len, 10); }
void OLED_ShowHexNum(uint8_t l, uint8_t c, uint32_t n, uint8_t len) { if (len <= 8u)number(l, c, n, len, 16); }
void OLED_ShowBinNum(uint8_t l, uint8_t c, uint32_t n, uint8_t len) { number(l, c, n, len, 2); }

void OLED_ShowSignedNum(uint8_t l, uint8_t c, int32_t n, uint8_t len)
{
    if (!len || len > 10u || c < 1u || (unsigned)c + len > 16u)return;
    OLED_ShowChar(l, c, n < 0 ? '-' : '+');
    number(l, (uint8_t)(c + 1u), n < 0 ? 0u - (uint32_t)n : (uint32_t)n, len, 10);
}
