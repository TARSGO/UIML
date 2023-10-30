#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "dependency.h"

#include "strict.h"
#include <stdio.h>
#include <string.h>

typedef struct 
{
    const char* name;
    GPIO_TypeDef* gpioX;
    uint16_t pin;
}CSInfo;

//SPI句柄信息
typedef struct {
    SPI_HandleTypeDef* hspi;
    uint8_t number; //SPIX中的X

    osSemaphoreId lock;
    CSInfo* csList;
    uint8_t csNum;
    struct 
    {
        uint8_t *data;
        uint16_t maxBufSize;
        uint16_t dataLen;
    }recvBuffer;
    SoftBusReceiverHandle fastHandle;
}SPIInfo;

//SPI服务数据
typedef struct {
    SPIInfo* spiList;
    uint8_t spiNum;
    uint8_t initFinished;
}SPIService;

SPIService spiService = {0};

//函数声明
void BSP_SPI_Init(ConfItem* dict);
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict);
void BSP_SPI_InitCS(SPIInfo* info, ConfItem* dict);
bool BSP_SPI_DMACallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_SPI_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    // 查找对应 SPI 外设
    SPIInfo* dev = NULL;
    for(uint8_t num = 0; num < spiService.spiNum; num++)
        if(hspi == spiService.spiList[num].hspi)
            dev = NULL;
    if (!dev) return;

    Bus_PublishTopicFast(dev->fastHandle, {{spiService.spiList->recvBuffer.data},
                                                {.U16 = spiService.spiList->recvBuffer.dataLen}}); // 发送数据
    // 将片选全部拉高
    for(uint8_t i = 0; i < dev->csNum; i++)
    {
        HAL_GPIO_WritePin(dev->csList[i].gpioX, dev->csList[i].pin, GPIO_PIN_SET);
    }

    // 解锁
    osSemaphoreRelease(dev->lock);
}

//SPI任务回调函数
void BSP_SPI_TaskCallback(void const * argument)
{
    //进入临界区
    portENTER_CRITICAL();
    BSP_SPI_Init((ConfItem*)argument);
    portEXIT_CRITICAL();

    Depends_SignalFinished(svc_spi);
    
    vTaskDelete(NULL);
}
void BSP_SPI_Init(ConfItem* dict)
{
    //计算用户配置的spi数量
    spiService.spiNum = 0;
    for(uint8_t num = 0; ; num++)
    {
        char confName[8] = {0};
        sprintf(confName, "spis/%d", num);
        if(Conf_ItemExist(dict, confName))
            spiService.spiNum++;
        else
            break;
    }
    
    //初始化各spi信息
    spiService.spiList = pvPortMalloc(spiService.spiNum * sizeof(SPIInfo));
    for(uint8_t num = 0; num < spiService.spiNum; num++)
    {
        char confName[8] = {0};
        sprintf(confName, "spis/%d", num);
        BSP_SPI_InitInfo(&spiService.spiList[num], Conf_GetNode(dict, confName));
    }

    //注册远程服务
    Bus_RemoteFuncRegister(NULL, BSP_SPI_BlockCallback, "/spi/block");
    Bus_RemoteFuncRegister(NULL,BSP_SPI_DMACallback, "/spi/trans/dma");
    spiService.initFinished = 1;
}
//初始化spi信息
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
    info->number = Conf_GetValue(dict,"number",uint8_t,0);

    char spiName[] = "spi_";
    spiName[3] = info->number + '0';
    info->hspi = Conf_GetPeriphHandle(spiName, SPI_HandleTypeDef);
    UIML_FATAL_ASSERT(info->hspi != NULL, "Missing SPI Device");

    char name[17] = {0};
    sprintf(name, "/spi%d/recv", info->number);
    info->fastHandle=Bus_GetFastTopicHandle(name);
    osSemaphoreDef(lock);
    info->lock = osSemaphoreCreate(osSemaphore(lock), 1);
    //初始化片选引脚
    BSP_SPI_InitCS(info, Conf_GetNode(dict, "cs"));
    //初始化缓冲区
    info->recvBuffer.maxBufSize = Conf_GetValue(dict, "max-recv-size", uint16_t, 1);
    info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
}

//初始化片选引脚
void BSP_SPI_InitCS(SPIInfo* info, ConfItem* dict)
{
    for(uint8_t num = 0; ; num++)
    {
        char confName[4] = {0};
        sprintf(confName, "%d", num);
        if(Conf_ItemExist(dict, confName))
            info->csNum++;
        else
            break;
    }
    //初始化各spi信息
    info->csList = pvPortMalloc(info->csNum * sizeof(CSInfo));
    for(uint8_t num = 0; num < info->csNum; num++)
    {
        char confName[11] = {0};
        sprintf(confName, "%d/pin", num);
        //重新映射至GPIO_PIN=2^pin
        info->csList[num].pin =1<<Conf_GetValue(dict, confName, uint8_t, 0);
        sprintf(confName, "%d/name", num);
        info->csList[num].name = Conf_GetValue(dict, confName, const char*, NULL);
        sprintf(confName, "%d/gpio-x", num);
        const char* gpioX = Conf_GetValue(dict, confName, const char*, "A");
        sprintf(confName, "gpio%s", gpioX);
        info->csList[num].gpioX = Conf_GetPeriphHandle(confName, GPIO_TypeDef);
    }
}

bool BSP_SPI_DMACallback(const char* name, SoftBusFrame* frame, void* bindData)
{
    if(!Bus_CheckMapKeysExist(frame,{"spi-x", "tx-data", "len", "cs-name", "is-block"}))
        return false;
    uint8_t spiX = Bus_GetMapValue(frame, "spi-x").U8;
    uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "tx-data").Ptr;
    uint8_t* rxData = NULL;
    uint16_t len = Bus_GetMapValue(frame, "len").U16;
    char* csName = (char*)Bus_GetMapValue(frame, "cs-name").Ptr;
    uint32_t waitTime = (Bus_GetMapValue(frame, "is-block").Bool) ? osWaitForever : 0;

    // 查找对应 SPI 外设
    SPIInfo* dev = NULL;
    for(uint8_t num = 0; num < spiService.spiNum; num++)
        if(spiX == spiService.spiList[num].number)
            dev = &spiService.spiList[num];
    if (!dev) return false;

    // 如未指定接收缓冲区，则指向 SPI 缓冲区
    if(Bus_CheckMapKeyExist(frame, "rx-data"))
        rxData = (uint8_t*)Bus_GetMapValue(frame, "rx-data").Ptr; //不检查该项是因为若为null则指向spi缓冲区
    else
        rxData = dev->recvBuffer.data;
    
    // 若接收长度大于缓冲区长度，则返回错误
    if(len > dev->recvBuffer.maxBufSize)
        return false;

    // 找到对应的片选引脚
    CSInfo* chipSel = NULL;
    for (uint8_t i = 0; i < dev->csNum; i++)
        if(!strcmp(csName, dev->csList[i].name))
            chipSel = &dev->csList[i];
    if (!chipSel) return false;

    // 上锁
    if(osSemaphoreWait(dev->lock, waitTime) != osOK)
        return false;

    // 开始通信
    HAL_GPIO_WritePin(chipSel->gpioX, chipSel->pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive_DMA(dev->hspi, txData, rxData, len); // 传输
    return true;
}

bool BSP_SPI_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
    if(!Bus_CheckMapKeysExist(frame,{"spi-x", "tx-data", "len", "cs-name", "is-block"}))
        return false;
    uint8_t spiX = Bus_GetMapValue(frame, "spi-x").U8;
    uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "tx-data").Ptr;
    uint8_t* rxData = NULL;
    uint16_t len = Bus_GetMapValue(frame, "len").U16;
    char* csName = (char*)Bus_GetMapValue(frame, "cs-name").Ptr;
    uint32_t waitTime = (Bus_GetMapValue(frame, "is-block").Bool) ? osWaitForever : 0;
    uint32_t timeout = Bus_GetMapValue(frame, "timeout").U32;

    // 查找对应 SPI 外设
    SPIInfo* dev = NULL;
    for(uint8_t num = 0; num < spiService.spiNum; num++)
        if(spiX == spiService.spiList[num].number)
            dev = &spiService.spiList[num];
    if (!dev) return false;

    // 如未指定接收缓冲区，则指向 SPI 缓冲区
    if(Bus_CheckMapKeyExist(frame, "rx-data"))
        rxData = (uint8_t*)Bus_GetMapValue(frame, "rx-data").Ptr; //不检查该项是因为若为null则指向spi缓冲区
    else
        rxData = dev->recvBuffer.data;
    
    // 若接收长度大于缓冲区长度，则返回错误
    if(len > dev->recvBuffer.maxBufSize)
        return false;

    // 找到对应的片选引脚
    CSInfo* chipSel = NULL;
    for (uint8_t i = 0; i < dev->csNum; i++)
        if(!strcmp(csName, dev->csList[i].name))
            chipSel = &dev->csList[i];
    if (!chipSel) return false;

    // 上锁
    if(osSemaphoreWait(dev->lock, waitTime) != osOK)
        return false;

    UIML_FATAL_ASSERT(((chipSel->gpioX == GPIOA && chipSel->pin == GPIO_PIN_4) ||
                       (chipSel->gpioX == GPIOB && chipSel->pin == GPIO_PIN_0)), "Invalid SPI chip select");
    if (chipSel->gpioX == GPIOA)
        UIML_FATAL_ASSERT((GPIOB->ODR & 1), "Both CS Active (Acc)");
    if (chipSel->gpioX == GPIOB)
        UIML_FATAL_ASSERT((GPIOA->ODR & 0x10), "Both CS Active (Gyro)");

    HAL_GPIO_WritePin(chipSel->gpioX, chipSel->pin, GPIO_PIN_RESET); // 拉低片选,开始通信
    HAL_SPI_TransmitReceive(dev->hspi, txData, rxData, len, timeout); // 开始传输
    HAL_GPIO_WritePin(chipSel->gpioX, chipSel->pin, GPIO_PIN_SET); // 拉高片选,结束通信

    //解锁
    osSemaphoreRelease(dev->lock);
    return true;
}
