#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "spi.h"

//SPI�����Ϣ
typedef struct {
	SPI_HandleTypeDef* hspi;
	uint8_t number; //SPIX�е�X
	SoftBusFastHandle fastHandle;
}SPIInfo;
typedef	struct 
{
	SPIInfo* SPIinfo;//���ݻ�����ָ��SPI
	uint8_t  *data;  	
	uint8_t *BufSize;
	uint8_t buffNum;
	uint8_t buffsize;
}SPIBuffer;


//SPI��������
typedef struct {
	SPIInfo* spiList;
	SPIBuffer* bufs;
	uint8_t spiNum;
	uint8_t initFinished;
}SPIService;

SPIService spiService={0};
//��������

void BSP_SPI_Init(ConfItem* dict);
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict);
void BSP_SPI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_SPI_InitBuffer(SPIBuffer* buffer, ConfItem* dict);



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

}

//SPI����ص�����
void BSP_SPI_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_SPI_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}
void BSP_SPI_Init(ConfItem* dict)
{
	//�����û����õ�spi����
	spiService.spiNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "spis/_";
		confName[5] = num + '0';
		if(Conf_ItemExist(dict, confName))
			spiService.spiNum++;
		else
			break;
	}
	//��ʼ����spi��Ϣ
	spiService.spiList = pvPortMalloc(spiService.spiNum * sizeof(SPIInfo));
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		char confName[] = "spis/_";
		confName[5] = num + '0';
		BSP_SPI_InitInfo(&spiService.spiList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		char confName[] = "spibuffer/_";
		confName[10] = num + '0';
		//��ʼ�����ջ�����
		BSP_SPI_InitBuffer(&spiService.bufs[num],Conf_GetPtr(dict, confName, ConfItem));

	}
	//���Ļ���
	spiService.initFinished = 1;
}
//��ʼ��spi��Ϣ
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
  info->hspi=Conf_GetPtr(dict,"hspi",SPI_HandleTypeDef);
	info->number=Conf_GetValue(dict,"number",uint8_t,0);
	char topic[] = "/spi_/exchange";
	topic[4] = info->number + '0';
	info->fastHandle=SoftBus_CreateFastHandle(topic);
}

//��ʼ��spi������
void BSP_SPI_InitBuffer(SPIBuffer* buffer, ConfItem* dict)
{
	uint8_t spiX = Conf_GetValue(dict, "spi_num", uint8_t, 0);
	for(uint8_t i = 0; i < spiService.spiNum; ++i)
		if(spiService.spiList[i].number == spiX)
			buffer->SPIinfo = &spiService.spiList[i];
  //����buff����������		
	buffer->buffNum=0;	
	buffer->buffsize=0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "buff/size_";
		confName[9] = num + '0';
		if(Conf_ItemExist(dict, confName))
		{
			buffer->BufSize[num]=Conf_GetValue(dict,confName,uint8_t,0);
			buffer->buffsize+=buffer->BufSize[num];
			buffer->buffNum++;
		}
		else
			break;
	}
	buffer->data=pvPortMalloc(buffer->buffsize);
	memset(buffer->data,0,buffer->buffsize);

}

void BSP_SPI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{

}