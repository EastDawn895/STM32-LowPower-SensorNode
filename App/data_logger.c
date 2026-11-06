#include "data_logger.h"
#include "w25q64.h"
#include "sensor_state.h"
#include "light_state.h"
#include "motion_state.h"
#include <string.h>

#define SECTOR 4096u
#define SLOT 32u
#define HEADER_MAGIC 0x534E4831u
#define RECORD_MAGIC 0x534E5231u
#define COMMIT 0x434D5431u
#define QUEUE 4u
static LoggerStatus_t health = LOGGER_IO_ERROR;
static MotionState_t lastMotion;
static LightState_t lastLight;
static SensorLog_t queue[QUEUE];
static uint8_t head, count, current, target, haveHeader;
static uint32_t generation, sequence, cursor, verifyOffset, dropped;
static uint8_t image[SLOT];

typedef enum { IDLE, ERASE_WAIT, ERASE_VERIFY, HEADER_BODY, HEADER_COMMIT, RECORD_BODY, RECORD_COMMIT } Stage;

static Stage stage;

static uint32_t get32(const uint8_t* b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static void put32(uint8_t* b, uint32_t n)
{
    for (unsigned i = 0; i < 4; i++) b[i] = (uint8_t)(n >> (8u * i));
}

static uint32_t crc(const uint8_t* b)
{
    uint32_t c = UINT32_MAX;
    for (unsigned i = 0; i < 24; i++)
    {
        c ^= b[i];
        for (unsigned j = 0; j < 8; j++) c = (c >> 1) ^ ((c & 1u) ? 0xEDB88320u : 0u);
    }
    return ~c;
}

static uint8_t blank(const uint8_t* b, unsigned n)
{
    for (unsigned i = 0; i < n; i++) if (b[i] != 255u) return 0;
    return 1;
}

static uint8_t valid(const uint8_t* b, uint32_t magic)
{
    return (uint8_t)(get32(b) == magic && get32(b + 24) == crc(b) && get32(b + 28) == COMMIT);
}

static uint8_t newer(uint32_t a, uint32_t b)
{
    return (uint8_t)(a != b && a - b < 0x80000000u);
}

static void finish(void)
{
    put32(image + 24, crc(image));
    put32(image + 28,COMMIT);
}

static void failed(void)
{
    health = LOGGER_IO_ERROR;
    dropped += count;
    count = 0;
    stage = IDLE;
}

LoggerStatus_t DataLogger_GetStatus(void) { return health; }
uint32_t DataLogger_GetDropped(void) { return dropped; }
uint8_t DataLogger_IsIdle(void) { return (uint8_t)(stage == IDLE && count == 0); }

HAL_StatusTypeDef DataLogger_Init(void)
{
    uint8_t b[SLOT], headers[2][SLOT], good[2] = {0, 0}, empty[2] = {1, 1};
    uint32_t lastSlot[2] = {SLOT,SLOT}, latest = 0;
    uint8_t haveSequence = 0;
    health = LOGGER_IO_ERROR;
    head = count = 0;
    stage = IDLE;
    dropped = 0;
    sequence = 0;
    current = 1;
    generation = UINT32_MAX;
    cursor = SECTOR;
    haveHeader = 0;
    lastMotion = MotionState_Get();
    lastLight = LightState_Get();
    for (uint8_t sector = 0; sector < 2; sector++)
    {
        if (W25Q64_ReadData((uint32_t)sector * SECTOR, headers[sector],SLOT) != HAL_OK) return HAL_ERROR;
        good[sector] = (uint8_t)(valid(headers[sector],HEADER_MAGIC) && get32(headers[sector] + 8) == 1u && get32(
            headers[sector] + 12) == sector);
        for (uint32_t offset = 0; offset < SECTOR; offset += SLOT)
        {
            if (W25Q64_ReadData((uint32_t)sector * SECTOR + offset, b,SLOT) != HAL_OK) return HAL_ERROR;
            if (!blank(b,SLOT))
            {
                empty[sector] = 0;
                if (offset) lastSlot[sector] = offset + SLOT;
            }
            if (offset && good[sector] && valid(b,RECORD_MAGIC) && b[18] == 1u &&
                b[16] >= 1u && b[16] <= 2u && b[17] <= (b[16] == 1u ? 1u : 2u) && get32(b + 20) == get32(
                    headers[sector] + 4))
            {
                uint32_t seq = get32(b + 4);
                if (!haveSequence || newer(seq, latest))
                {
                    latest = seq;
                    haveSequence = 1;
                }
            }
        }
    }
    if (good[0] && good[1])
    {
        uint32_t a = get32(headers[0] + 4), bgen = get32(headers[1] + 4);
        if (a == bgen || a - bgen == 0x80000000u)
        {
            health = LOGGER_FORMAT_ERROR;
            return HAL_ERROR;
        }
        current = (uint8_t)(newer(a, bgen) ? 0u : 1u);
    }
    else if (good[0] || good[1]) current = (uint8_t)(good[0] ? 0u : 1u);
    else if (!empty[0] || !empty[1])
    {
        health = LOGGER_FORMAT_ERROR;
        return HAL_ERROR;
    }
    if (good[current])
    {
        haveHeader = 1;
        generation = get32(headers[current] + 4);
        cursor = lastSlot[current];
    }
    sequence = haveSequence ? latest + 1u : 0u;
    health = LOGGER_READY;
    return HAL_OK;
}

HAL_StatusTypeDef DataLogger_Enqueue(uint8_t source, uint8_t state, int32_t value)
{
    if (source < 1u || source > 2u || (source == 1u && state > 1u) || (source == 2u && state > 2u)) return HAL_ERROR;
    if (health != LOGGER_READY || count == QUEUE)
    {
        dropped++;
        return HAL_ERROR;
    }
    SensorLog_t* q = &queue[(head + count) % QUEUE];
    q->Log_Time = HAL_GetTick();
    q->Log_Value = value;
    q->Log_Source = source;
    q->Log_LState = state;
    count++;
    return HAL_OK;
}

static void collect(void)
{
    MotionState_t m = MotionState_Get();
    LightState_t l = LightState_Get();
    if (g_sensorData.mpuValid && m != lastMotion && m == MOTION_ACTIVE)
        (void)DataLogger_Enqueue(1, (uint8_t)m, g_sensorData.gxFiltered);
    if (g_sensorData.lightValid && l != lastLight && l != LIGHT_NORMAL)
        (void)DataLogger_Enqueue(2, (uint8_t)l, g_sensorData.lightFiltered);
    lastMotion = m;
    lastLight = l;
}

void DataLogger_Update(void)
{
    collect();
    if (health != LOGGER_READY) return;
    uint8_t check[256];
    HAL_StatusTypeDef rc;
    if (stage == ERASE_VERIFY)
    {
        rc = W25Q64_ReadData((uint32_t)target * SECTOR + verifyOffset, check, sizeof(check));
        if (rc != HAL_OK || !blank(check, sizeof(check)))
        {
            failed();
            return;
        }
        verifyOffset += sizeof(check);
        if (verifyOffset < SECTOR) return;
        memset(image, 0, sizeof(image));
        put32(image,HEADER_MAGIC);
        put32(image + 4, generation + 1u);
        put32(image + 8, 1);
        put32(image + 12, target);
        finish();
        if (W25Q64_BeginProgram((uint32_t)target * SECTOR, image, 28) != HAL_OK)
        {
            failed();
            return;
        }
        stage = HEADER_BODY;
        return;
    }
    if (stage != IDLE)
    {
        rc = W25Q64_Poll();
        if (rc == HAL_BUSY) return;
        if (rc != HAL_OK)
        {
            failed();
            return;
        }
        if (stage == ERASE_WAIT)
        {
            verifyOffset = 0;
            stage = ERASE_VERIFY;
            return;
        }
        uint32_t addr = (stage == HEADER_BODY || stage == HEADER_COMMIT)
                            ? (uint32_t)target * SECTOR
                            : (uint32_t)current * SECTOR + cursor;
        if (stage == HEADER_BODY || stage == RECORD_BODY)
        {
            if (W25Q64_BeginProgram(addr + 28, image + 28, 4) != HAL_OK)
            {
                failed();
                return;
            }
            stage = stage == HEADER_BODY ? HEADER_COMMIT : RECORD_COMMIT;
            return;
        }
        if (W25Q64_ReadData(addr, check,SLOT) != HAL_OK || memcmp(check, image,SLOT) != 0)
        {
            failed();
            return;
        }
        if (stage == HEADER_COMMIT)
        {
            current = target;
            generation++;
            cursor = SLOT;
            haveHeader = 1;
        }
        else
        {
            cursor += SLOT;
            sequence++;
            head = (uint8_t)((head + 1u) % QUEUE);
            count--;
        }
        stage = IDLE;
        return;
    }
    if (!count) return;
    if (!haveHeader || cursor >= SECTOR)
    {
        target = (uint8_t)(current ^ 1u);
        if (W25Q64_BeginErase((uint32_t)target * SECTOR) != HAL_OK)
        {
            failed();
            return;
        }
        stage = ERASE_WAIT;
        return;
    }
    SensorLog_t* q = &queue[head];
    memset(image, 0, sizeof(image));
    put32(image,RECORD_MAGIC);
    put32(image + 4, sequence);
    put32(image + 8, q->Log_Time);
    put32(image + 12, (uint32_t)q->Log_Value);
    image[16] = q->Log_Source;
    image[17] = q->Log_LState;
    image[18] = 1;
    put32(image + 20, generation);
    finish();
    if (W25Q64_BeginProgram((uint32_t)current * SECTOR + cursor, image, 28) != HAL_OK)
    {
        failed();
        return;
    }
    stage = RECORD_BODY;
}
