#include <assert.h>
#include <string.h>
#include "data_logger.h"
#include "w25q64.h"
#include "sensor_state.h"
#include "motion_state.h"
#include "light_state.h"
static uint8_t flash[8192],snapshot[8192];
static uint32_t tick,lastErase;
static int cut=-1;static unsigned prefix;
SensorData_t g_sensorData;
static MotionState_t motion = MOTION_IDLE;
static LightState_t light = LIGHT_NORMAL;
uint32_t HAL_GetTick(void){return tick++;}
MotionState_t MotionState_Get(void){return motion;}
LightState_t LightState_Get(void){return light;}
HAL_StatusTypeDef W25Q64_ReadData(uint32_t a,uint8_t *b,uint16_t n){assert(a+n<=8192u);memcpy(b,flash+a,n);return HAL_OK;}
HAL_StatusTypeDef W25Q64_Poll(void){return HAL_OK;}
HAL_StatusTypeDef W25Q64_BeginErase(uint32_t a){
    assert(a%4096u==0 && a<8192);lastErase=a;
    unsigned n=4096;int fail=cut==0;if(cut>=0)cut--;
    if(fail && prefix<n)n=prefix;
    memset(flash+a,255,n);return fail?HAL_ERROR:HAL_OK;
}
HAL_StatusTypeDef W25Q64_BeginProgram(uint32_t a,const uint8_t *b,uint16_t len){
    assert(a+len<=8192u && a%256u+len<=256u);unsigned n=len;int fail=cut==0;if(cut>=0)cut--;
    if(fail && prefix<n)n=prefix;
    for(unsigned i=0;i<n;i++){assert((flash[a+i]&b[i])==b[i]);flash[a+i]&=b[i];}
    return fail?HAL_ERROR:HAL_OK;
}
static uint32_t read32(const uint8_t *b){return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
static void write32(uint8_t *b,uint32_t v){for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(v>>(8u*i));}
static void checksum(uint8_t *b){uint32_t c=UINT32_MAX;for(unsigned i=0;i<24;i++){c^=b[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((c&1u)?0xEDB88320u:0u);}write32(b+24,~c);}
static void drain(void){for(int i=0;i<100 && !DataLogger_IsIdle();i++)DataLogger_Update();assert(DataLogger_IsIdle());}
static void append(void){assert(DataLogger_Enqueue(1,1,31000)==HAL_OK);drain();}
int main(void){
    memset(flash,255,sizeof flash);assert(DataLogger_Init()==HAL_OK);
    for(int i=0;i<381;i++){append();if(i%17==0)assert(DataLogger_Init()==HAL_OK);}
    memcpy(snapshot,flash,sizeof flash);assert(DataLogger_Init()==HAL_OK);append();
    assert(lastErase==4096u && memcmp(flash,snapshot,4096)==0);
    append();append();
    assert(DataLogger_Init()==HAL_OK);append();assert(lastErase==4096u); /* no erase of newest sector0 */
    for(int i=0;i<700;i++){append();assert(DataLogger_Init()==HAL_OK);}
    memset(flash,255,sizeof flash);assert(DataLogger_Init()==HAL_OK);
    for(int i=0;i<254;i++)append();
    memcpy(snapshot,flash,sizeof flash);
    const unsigned cuts[]={0,1,4,16,27,28,31,32,33,255,2048,4095,4096};
    for(int op=0;op<5;op++)for(unsigned k=0;k<sizeof cuts/sizeof cuts[0];k++){
        memcpy(flash,snapshot,sizeof flash);cut=-1;assert(DataLogger_Init()==HAL_OK);
        cut=op;prefix=cuts[k];append();cut=-1;
        /* Sector1 contains the latest committed record and must survive every cut. */
        assert(memcmp(flash+4096,snapshot+4096,4096)==0);
        assert(DataLogger_Init()==HAL_OK);append();assert(DataLogger_GetStatus()==LOGGER_READY);
    }
    memcpy(flash,snapshot,sizeof flash);flash[8192-16]^=1;assert(DataLogger_Init()==HAL_OK);append();
    memset(flash,255,sizeof flash);flash[0]=0;memcpy(snapshot,flash,sizeof flash);
    assert(DataLogger_Init()==HAL_ERROR);assert(DataLogger_GetStatus()==LOGGER_FORMAT_ERROR);
    DataLogger_Update();assert(memcmp(flash,snapshot,sizeof flash)==0);
    /* Offline edge rejection explains Drop with Log=2; recovery edges do not log. */
    g_sensorData.lightValid=1;g_sensorData.mpuValid=1;
    light=LIGHT_HIGH;DataLogger_Update();assert(DataLogger_GetDropped()==1);
    for(int i=0;i<100;i++)DataLogger_Update();
    assert(DataLogger_GetDropped()==1);
    light=LIGHT_NORMAL;DataLogger_Update();assert(DataLogger_GetDropped()==1);
    light=LIGHT_LOW;DataLogger_Update();assert(DataLogger_GetDropped()==2);
    light=LIGHT_NORMAL;DataLogger_Update();assert(DataLogger_GetDropped()==2);
    motion=MOTION_ACTIVE;DataLogger_Update();assert(DataLogger_GetDropped()==3);
    motion=MOTION_IDLE;DataLogger_Update();assert(DataLogger_GetDropped()==3);
    assert(DataLogger_GetStatus()==LOGGER_FORMAT_ERROR);
    assert(memcmp(flash,snapshot,sizeof flash)==0);
    memset(flash,255,sizeof flash);assert(DataLogger_Init()==HAL_OK);
    for(int i=0;i<4;i++)assert(DataLogger_Enqueue(2,0,3500)==HAL_OK);
    assert(DataLogger_Enqueue(2,0,3500)==HAL_ERROR);assert(DataLogger_GetDropped()==1);drain();
    assert(DataLogger_GetStatus()==LOGGER_READY); /* Queue full is not an I/O fault. */
    memset(flash,255,sizeof flash);assert(DataLogger_Init()==HAL_OK);
    assert(DataLogger_Enqueue(1,1,3000)==HAL_OK);
    assert(DataLogger_Enqueue(2,2,1000)==HAL_OK);
    cut=0;prefix=0;DataLogger_Update();cut=-1;
    assert(DataLogger_GetStatus()==LOGGER_IO_ERROR && DataLogger_GetDropped()==2);
    assert(DataLogger_IsIdle());
    assert(DataLogger_Enqueue(1,1,3000)==HAL_ERROR);
    assert(DataLogger_GetDropped()==3);
    memset(flash,255,sizeof flash);assert(DataLogger_Init()==HAL_OK);append();
    write32(flash+4,UINT32_MAX);checksum(flash);
    write32(flash+32+4,UINT32_MAX);write32(flash+32+20,UINT32_MAX);checksum(flash+32);
    assert(DataLogger_Init()==HAL_OK);append();assert(read32(flash+64+4)==0u);
    for(int i=0;i<126;i++)append();
    assert(read32(flash+4096+4)==0u);assert(DataLogger_Init()==HAL_OK);append();
    return 0;
}
