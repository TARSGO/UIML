#ifndef _JUDGEMENT_H_
#define _JUDGEMENT_H_
#include "string.h"
#include "crc_dji.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#define    JUDGE_DATA_ERROR      0
#define    JUDGE_DATA_CORRECT    1

#define    LEN_HEADER    5        //֡ͷ��
#define    LEN_CMDID     2        //�����볤��
#define    LEN_TAIL      2	      //֡βCRC16

//��ʼ�ֽ�,Э��̶�Ϊ0xA5
#define    JUDGE_FRAME_HEADER         (0xA5)
#define    JUDGE_MAX_TX_LENGTH   64
//��������ɫ
typedef enum
{
	RobotColor_Red,
	RobotColor_Blue
}RobotColor;

typedef enum 
{
	FRAME_HEADER         = 0,
	CMD_ID               = 5,
	DATA                 = 7,
}JudgeFrameOffset;

//5�ֽ�֡ͷ,ƫ��λ��
typedef enum
{
	SOF          = 0,//��ʼλ
	DATA_LENGTH  = 1,//֡�����ݳ���,�����������ȡ���ݳ���
	SEQ          = 3,//�����
	CRC8         = 4 //CRC8
	
}FrameHeaderOffset;

/***************������ID********************/

/* 

	ID: 0x0001  Byte:  11    ����״̬����       			����Ƶ�� 1Hz      
	ID: 0x0002  Byte:  1    �����������         		������������      
	ID: 0x0003  Byte:  32    ����������Ѫ������   		1Hz����  
	ID: 0x0101  Byte:  4    �����¼�����   				�¼��ı����
	ID: 0x0102  Byte:  4    ���ز���վ������ʶ����    	�����ı����
	ID: 0x0104	Byte: 	2		���о�����Ϣ
	ID: 0x0105	Byte: 	1		���ڷ���ڵ���ʱ
	ID: 0X0201  Byte: 27    ������״̬����        		10Hz
	ID: 0X0202  Byte: 14    ʵʱ������������   			50Hz       
	ID: 0x0203  Byte: 16    ������λ������           	10Hz
	ID: 0x0204  Byte:  1    ��������������           	����״̬�ı����
	ID: 0x0205  Byte:  1    ���л���������״̬����      10Hz
	ID: 0x0206  Byte:  1    �˺�״̬����           		�˺���������
	ID: 0x0207  Byte:  7    ʵʱ�������           		�ӵ��������
	ID: 0x0208  Byte:  6    �ӵ�ʣ�෢����
	ID: 0x0209  Byte:  4    ������RFID״̬
	ID: 0x020A  Byte:  6    ���ڻ����˿ͻ���ָ������
	ID: 0x0301  Byte:  n    �����˼佻������           	���ͷ���������,10Hz
*/


//������ID,�����жϽ��յ���ʲô����
typedef enum
{ 
	ID_game_state       				= 0x0001,//����״̬����
	ID_game_result 	   					= 0x0002,//�����������
	ID_game_robot_HP      			= 0x0003,//����������Ѫ������
	ID_event_data  							= 0x0101,//�����¼����� 
	ID_supply_projectile_action = 0x0102,//���ز���վ������ʶ����
	ID_referee_warning					= 0x0104,//���о�����Ϣ
	ID_dart_remaining_time			= 0x0105,//���ڷ���ڵ���ʱ
	ID_game_robot_state    			= 0x0201,//������״̬����
	ID_power_heat_data    			= 0x0202,//ʵʱ������������
	ID_game_robot_pos        		= 0x0203,//������λ������
	ID_buff_musk								= 0x0204,//��������������
	ID_aerial_robot_energy			= 0x0205,//���л���������״̬����
	ID_robot_hurt								= 0x0206,//�˺�״̬����
	ID_shoot_data								= 0x0207,//ʵʱ�������
	ID_bullet_remaining					= 0x0208,//�ӵ�ʣ�෢����
	ID_rfid_status							= 0x0209,//������RFID״̬
	ID_dart_client_cmd					= 0x020A,//���ڻ����˿ͻ���ָ������
} CmdID;

//���������ݶγ�,���ݹٷ�Э�������峤��
typedef enum
{
	LEN_game_state       					= 11,	//0x0001
	LEN_game_result       				= 1,	//0x0002
	LEN_game_robot_HP							= 32,	//0x0003
	LEN_event_data  							= 4,	//0x0101
	LEN_supply_projectile_action  = 4,	//0x0102
	LEN_referee_warning						= 2,	//0x0104
	LEN_dart_remaining_time				= 1,	//0x0105
	LEN_game_robot_state    			= 27,	//0x0201
	LEN_power_heat_data   				= 16,	//0x0202
	LEN_game_robot_pos        		= 16,	//0x0203
	LEN_buff_musk        					= 1,	//0x0204
	LEN_aerial_robot_energy       = 1,	//0x0205
	LEN_robot_hurt        				= 1,	//0x0206
	LEN_shoot_data       					= 7,	//0x0207
	LEN_bullet_remaining					= 6,	//0x0208
	LEN_rfid_status								= 4,	//0x0209
	LEN_dart_client_cmd						= 6,	//0x020A
} JudgeDataLength;

/* �Զ���֡ͷ */
typedef __packed struct
{
	uint8_t  SOF;
	uint16_t DataLength;
	uint8_t  Seq;
	uint8_t  CRC8;
} xFrameHeader;

/* ID: 0x0001  Byte:  11    ����״̬���� */
typedef __packed struct
{
	uint8_t game_type : 4;
	uint8_t game_progress : 4;
	uint16_t stage_remain_time;
	uint64_t SyncTimeStamp;
} ext_game_status_t;

/* ID: 0x0002  Byte:  1    ����������� */
typedef __packed struct 
{ 
	uint8_t winner;
} ext_game_result_t; 

/* ID: 0x0003  Byte:  32    ����������Ѫ������ */
typedef __packed struct
{
	uint16_t red_1_robot_HP;
	uint16_t red_2_robot_HP;
	uint16_t red_3_robot_HP;
	uint16_t red_4_robot_HP;
	uint16_t red_5_robot_HP;
	uint16_t red_7_robot_HP;
	uint16_t red_outpost_HP;
	uint16_t red_base_HP;
	uint16_t blue_1_robot_HP;
	uint16_t blue_2_robot_HP;
	uint16_t blue_3_robot_HP;
	uint16_t blue_4_robot_HP;
	uint16_t blue_5_robot_HP;
	uint16_t blue_7_robot_HP;
	uint16_t blue_outpost_HP;
	uint16_t blue_base_HP;
} ext_game_robot_HP_t;

/* ID: 0x0101  Byte:  4    �����¼����� */
typedef __packed struct 
{ 
	uint32_t event_type;
} ext_event_data_t; 

/* ID: 0x0102  Byte:  4    ���ز���վ������ʶ���� */
typedef __packed struct
{
	uint8_t supply_projectile_id;
	uint8_t supply_robot_id;
	uint8_t supply_projectile_step;
	uint8_t supply_projectile_num;
} ext_supply_projectile_action_t;

/* ID: 0x104    Byte: 2    ���о�����Ϣ */
typedef __packed struct
{
	uint8_t level;
	uint8_t foul_robot_id;
} ext_referee_warning_t;

/* ID: 0x105    Byte: 1    ���ڷ���ڵ���ʱ */
typedef __packed struct
{
	uint8_t dart_remaining_time;
} ext_dart_remaining_time_t;

/* ID: 0X0201  Byte: 27    ������״̬���� */
typedef __packed struct
{
	uint8_t robot_id;
	uint8_t robot_level;
	uint16_t remain_HP;
	uint16_t max_HP;
	uint16_t shooter_id1_17mm_cooling_rate;
	uint16_t shooter_id1_17mm_cooling_limit;
	uint16_t shooter_id1_17mm_speed_limit;
	uint16_t shooter_id2_17mm_cooling_rate;
	uint16_t shooter_id2_17mm_cooling_limit;
	uint16_t shooter_id2_17mm_speed_limit;
	uint16_t shooter_id1_42mm_cooling_rate;
	uint16_t shooter_id1_42mm_cooling_limit;
	uint16_t shooter_id1_42mm_speed_limit;
	uint16_t chassis_power_limit;
	uint8_t mains_power_gimbal_output : 1;
	uint8_t mains_power_chassis_output : 1;
	uint8_t mains_power_shooter_output : 1;
} ext_game_robot_status_t;


/* ID: 0X0202  Byte: 16    ʵʱ������������ */
typedef __packed struct
{
	uint16_t chassis_volt;
	uint16_t chassis_current;
	float chassis_power;
	uint16_t chassis_power_buffer;
	uint16_t shooter_id1_17mm_cooling_heat;
	uint16_t shooter_id2_17mm_cooling_heat;
	uint16_t shooter_id1_42mm_cooling_heat;
} ext_power_heat_data_t;


/* ID: 0x0203  Byte: 16    ������λ������ */
typedef __packed struct 
{   
	float x;   
	float y;   
	float z;   
	float yaw; 
} ext_game_robot_pos_t; 


/* ID: 0x0204  Byte:  1    �������������� */
typedef __packed struct 
{ 
	uint8_t power_rune_buff; 
} ext_buff_musk_t; 


/* ID: 0x0205  Byte:  1    ���л���������״̬���� */
typedef __packed struct
{
	uint8_t attack_time;
} aerial_robot_energy_t;


/* ID: 0x0206  Byte:  1    �˺�״̬���� */
typedef __packed struct 
{ 
	uint8_t armor_id : 4; 
	uint8_t hurt_type : 4; 
} ext_robot_hurt_t; 


/* ID: 0x0207  Byte:  7    ʵʱ������� */
typedef __packed struct
{
	uint8_t bullet_type;
	uint8_t shooter_id;
	uint8_t bullet_freq;
	float bullet_speed;
} ext_shoot_data_t;

/* ID: 0x0208  Byte:  6    �ӵ�ʣ�෢���� */
typedef __packed struct
{
	uint16_t bullet_remaining_num_17mm;
	uint16_t bullet_remaining_num_42mm;
	uint16_t coin_remaining_num;
} ext_bullet_remaining_t;

/* ID: 0x0209  Byte:  4    ������RFID״̬ */
typedef __packed struct
{
	uint32_t rfid_status;
} ext_rfid_status_t;

/* ID: 0x020A  Byte:  6    ���ڻ����˿ͻ���ָ������ */
typedef __packed struct
{
	uint8_t dart_launch_opening_status;
	uint8_t dart_attack_target;
	uint16_t target_change_time;
	uint16_t operate_launch_cmd_time;
} ext_dart_client_cmd_t;

/* 
	
	�������ݣ�����һ��ͳһ�����ݶ�ͷ�ṹ��
	���������� ID���������Լ������ߵ� ID ���������ݶΣ�
	�����������ݵİ��ܹ������Ϊ 128 ���ֽڣ�
	��ȥ frame_header,cmd_id,frame_tail �Լ����ݶ�ͷ�ṹ�� 6 ���ֽڣ�
	�ʶ����͵��������ݶ����Ϊ 113��
	������������ 0x0301 �İ�����Ƶ��Ϊ 10Hz��

	������ ID��
	1��Ӣ��(��)��
	2������(��)��
	3/4/5������(��)��
	6������(��)��
	7���ڱ�(��)��
	11��Ӣ��(��)��
	12������(��)��
	13/14/15������(��)��
	16������(��)��
	17���ڱ�(��)�� 
	�ͻ��� ID�� 
	0x0101 ΪӢ�۲����ֿͻ���( ��) ��
	0x0102 �����̲����ֿͻ��� ((�� )��
	0x0103/0x0104/0x0105�����������ֿͻ���(��)��
	0x0106�����в����ֿͻ���((��)�� 
	0x0111��Ӣ�۲����ֿͻ���(��)��
	0x0112�����̲����ֿͻ���(��)��
	0x0113/0x0114/0x0115�������ֿͻ��˲���(��)��
	0x0116�����в����ֿͻ���(��)�� 
*/
/* �������ݽ�����Ϣ��0x0301  */
typedef __packed struct 
{ 
	uint16_t data_cmd_id;    
	uint16_t send_ID;    
	uint16_t receiver_ID; 
} ext_student_interactive_header_data_t; 

/* 
	ѧ�������˼�ͨ�� cmd_id 0x0301������ ID:0x0200~0x02FF
	�������� �����˼�ͨ�ţ�0x0301��
	����Ƶ�ʣ����� 10Hz  

	�ֽ�ƫ���� 	��С 	˵�� 			��ע 
	0 			2 		���ݵ����� ID 	0x0200~0x02FF 
										���������� ID ��ѡȡ������ ID �����ɲ������Զ��� 
	
	2 			2 		�����ߵ� ID 	��ҪУ�鷢���ߵ� ID ��ȷ�ԣ� 
	
	4 			2 		�����ߵ� ID 	��ҪУ������ߵ� ID ��ȷ�ԣ�
										���粻�ܷ��͵��жԻ����˵�ID 
	
	6 			n 		���ݶ� 			n ��ҪС�� 113 

*/
typedef __packed struct 
{ 
	uint8_t data[100]; //���ݶ�,n��ҪС��113
} robot_interactive_data_t;


/* �ͻ��� �ͻ����Զ���ͼ�Σ�cmd_id:0x030 */

//ͼ������
typedef __packed struct {
	uint8_t graphic_name[3]; 
	uint32_t operate_tpye:3; 
	uint32_t graphic_tpye:3; 
	uint32_t layer:4; 
	uint32_t color:4; 
	uint32_t start_angle:9; 
	uint32_t end_angle:9; 
	uint32_t width:10; 
	uint32_t start_x:11; 
	uint32_t start_y:11;
	uint32_t radius:10; 
	uint32_t end_x:11; 
	uint32_t end_y:11; 
} graphic_data_struct_t;

//ɾ��ͼ�� data_cmd_id=0x0100
typedef __packed struct
{
	uint8_t operate_tpye;
	uint8_t layer;
} ext_client_custom_graphic_delete_t;

//����һ��ͼ�� data_cmd_id=0x0101
typedef __packed struct
{
	graphic_data_struct_t grapic_data_struct;
} ext_client_custom_graphic_single_t;

//�������� data_cmd_id=0x0110
typedef __packed struct {
	graphic_data_struct_t grapic_data_struct; 
	uint8_t data[30];
} ext_client_custom_character_t;

//�����˽�����Ϣ
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//֡ͷ
	uint16_t								CmdID;//������
	ext_student_interactive_header_data_t   dataFrameHeader;//���ݶ�ͷ�ṹ
	robot_interactive_data_t  	 			interactData;//���ݶ�
	uint16_t		 						FrameTail;//֡β
}ext_CommunatianData_t;

//�ͻ����Զ���ͼ����Ϣ
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//֡ͷ
	uint16_t								CmdID;//������
	ext_student_interactive_header_data_t   dataFrameHeader;//���ݶ�ͷ�ṹ
	ext_client_custom_graphic_single_t  	 			graphData;//���ݶ�
	uint16_t		 						FrameTail;//֡β
}ext_GraphData_t;

//�ͻ����Զ���������Ϣ
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//֡ͷ
	uint16_t								CmdID;//������
	ext_student_interactive_header_data_t   dataFrameHeader;//���ݶ�ͷ�ṹ
	ext_client_custom_character_t  	 			textData;//���ݶ�
	uint16_t		 						FrameTail;//֡β
}ext_TextData_t;

//�ͻ����Զ���UIɾ����״
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//֡ͷ
	uint16_t								CmdID;//������
	ext_student_interactive_header_data_t   dataFrameHeader;//���ݶ�ͷ�ṹ
	ext_client_custom_graphic_delete_t  	 			deleteData;//���ݶ�
	uint16_t		 						FrameTail;//֡β
}ext_DeleteData_t;

//����ϵͳ��������֡
typedef struct
{
	uint8_t data[JUDGE_MAX_TX_LENGTH];
	uint16_t frameLength;
}JudgeTxFrame;




/*****************ϵͳ���ݶ���**********************/
ext_game_status_t       				GameState;							//0x0001
ext_game_result_t            		GameResult;							//0x0002
ext_game_robot_HP_t          		GameRobotHP;						//0x0003
ext_event_data_t        				EventData;							//0x0101
ext_supply_projectile_action_t	SupplyProjectileAction;	//0x0102
ext_referee_warning_t						RefereeWarning;					//0x0104
ext_dart_remaining_time_t				DartRemainingTime;			//0x0105
ext_game_robot_status_t			  	GameRobotStat;					//0x0201
ext_power_heat_data_t		  			PowerHeatData;					//0x0202
ext_game_robot_pos_t						GameRobotPos;						//0x0203
ext_buff_musk_t									BuffMusk;								//0x0204
aerial_robot_energy_t						AerialRobotEnergy;			//0x0205
ext_robot_hurt_t								RobotHurt;							//0x0206
ext_shoot_data_t								ShootData;							//0x0207
ext_bullet_remaining_t					BulletRemaining;				//0x0208
ext_rfid_status_t								RfidStatus;							//0x0209
ext_dart_client_cmd_t						DartClientCmd;					//0x020A

xFrameHeader              			FrameHeader;						//����֡ͷ��Ϣ
/****************************************************/
bool Judge_Data_TF = FALSE;//���������Ƿ����,������������

typedef struct _Judge
{

	uint8_t uartX;
	uint8_t taskInterval;	
	
}Judge;

void Judge_Init(Judge *judge,ConfItem* dict);
void Judge_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool JUDGE_Read_Data(uint8_t *ReadFromUsart);

void Judge_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Judge judge={0};
	Judge_Init(&judge, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();
	while (1)
	{
		
		osDelayUntil(&tick,judge.taskInterval);
	}		
}

void Judge_Init(Judge *judge,ConfItem* dict)
{
	char name[] = "/uart_/recv";
	judge->uartX = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	name[5] = judge->uartX + '0';
	Bus_RegisterReceiver(NULL, Judge_SoftBusCallback, name);
	judge->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 150);
}
void Judge_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 0);
	if(data)
		JUDGE_Read_Data(data);
}

/**************����ϵͳ���ݸ���****************/

/**
  * @brief  ��ȡ��������,�ж��ж�ȡ��֤�ٶ�
  * @param  ��������
  * @retval �Ƿ�������ж�������
  * @attention  �ڴ��ж�֡ͷ��CRCУ��,������д�����ݣ����ظ��ж�֡ͷ
  */
bool JUDGE_Read_Data(uint8_t *ReadFromUsart)
{
	bool retval_tf = FALSE;//������ȷ����־,ÿ�ε��ö�ȡ����ϵͳ���ݺ�������Ĭ��Ϊ����
	
	uint16_t judge_length;//ͳ��һ֡���ݳ��� 
	
	int CmdID = 0;//�������������
	
	/***------------------*****/
	//�����ݰ��������κδ���
	if (ReadFromUsart == NULL)
	{
		return -1;
	}
	
	//д��֡ͷ����,�����ж��Ƿ�ʼ�洢��������
	memcpy(&FrameHeader, ReadFromUsart, LEN_HEADER);
	
	//�ж�֡ͷ�����Ƿ�Ϊ0xA5
	if(ReadFromUsart[ SOF ] == JUDGE_FRAME_HEADER)
	{
		//֡ͷCRC8У��
		if (Verify_CRC8_Check_Sum( ReadFromUsart, LEN_HEADER ) == TRUE)
		{
			//ͳ��һ֡���ݳ���,����CR16У��
			judge_length = ReadFromUsart[ DATA_LENGTH ] + LEN_HEADER + LEN_CMDID + LEN_TAIL;;

			//֡βCRC16У��
			if(Verify_CRC16_Check_Sum(ReadFromUsart,judge_length) == TRUE)
			{
				retval_tf = TRUE;//��У�������˵�����ݿ���
				
				CmdID = (ReadFromUsart[6] << 8 | ReadFromUsart[5]);
				//��������������,�����ݿ�������Ӧ�ṹ����(ע�⿽�����ݵĳ���)
				switch(CmdID)
				{
					case ID_game_state:        			//0x0001
						memcpy(&GameState, (ReadFromUsart + DATA), LEN_game_state);
					break;
					
					case ID_game_result:          		//0x0002
						memcpy(&GameResult, (ReadFromUsart + DATA), LEN_game_result);
					break;
					
					case ID_game_robot_HP:       //0x0003
						memcpy(&GameRobotHP, (ReadFromUsart + DATA), LEN_game_robot_HP);
					break;
					
					case ID_event_data:    				//0x0101
						memcpy(&EventData, (ReadFromUsart + DATA), LEN_event_data);
					break;
					
					case ID_supply_projectile_action:   //0x0102
						memcpy(&SupplyProjectileAction, (ReadFromUsart + DATA), LEN_supply_projectile_action);
					break;
					
					case ID_referee_warning:  //0x0104
						memcpy(&RefereeWarning, (ReadFromUsart + DATA), LEN_referee_warning);
					break;
					
					case ID_dart_remaining_time:  //0x0105
						memcpy(&DartRemainingTime, (ReadFromUsart + DATA), LEN_dart_remaining_time);
					break;
					
					case ID_game_robot_state:      		//0x0201
						memcpy(&GameRobotStat, (ReadFromUsart + DATA), LEN_game_robot_state);
					break;
					
					case ID_power_heat_data:      		//0x0202
						memcpy(&PowerHeatData, (ReadFromUsart + DATA), LEN_power_heat_data);
					break;
					
					case ID_game_robot_pos:      		//0x0203
						memcpy(&GameRobotPos, (ReadFromUsart + DATA), LEN_game_robot_pos);
					break;
					
					case ID_buff_musk:      			//0x0204
						memcpy(&BuffMusk, (ReadFromUsart + DATA), LEN_buff_musk);
					break;
					
					case ID_aerial_robot_energy:      	//0x0205
						memcpy(&AerialRobotEnergy, (ReadFromUsart + DATA), LEN_aerial_robot_energy);
					break;
					
					case ID_robot_hurt:      			//0x0206
						memcpy(&RobotHurt, (ReadFromUsart + DATA), LEN_robot_hurt);
					break;
					
					case ID_shoot_data:      			//0x0207
						memcpy(&ShootData, (ReadFromUsart + DATA), LEN_shoot_data);
						//Vision_SendShootSpeed(ShootData.bullet_speed);
					break;
					
					case ID_bullet_remaining:      			//0x0208
						memcpy(&BulletRemaining, (ReadFromUsart + DATA), LEN_bullet_remaining);
					break;
					
					case ID_rfid_status:      			//0x0209
						memcpy(&RfidStatus, (ReadFromUsart + DATA), LEN_rfid_status);
					break;
					
					case ID_dart_client_cmd:      			//0x020A
						memcpy(&DartClientCmd, (ReadFromUsart + DATA), LEN_dart_client_cmd);
					break;
				}
				//�׵�ַ��֡����,ָ��CRC16��һ�ֽ�,�����ж��Ƿ�Ϊ0xA5,�����ж�һ�����ݰ��Ƿ��ж�֡����
				if(*(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL) == 0xA5)
				{
					//���һ�����ݰ������˶�֡����,���ٴζ�ȡ
					JUDGE_Read_Data(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL);
				}
			}
		}
		//�׵�ַ��֡����,ָ��CRC16��һ�ֽ�,�����ж��Ƿ�Ϊ0xA5,�����ж�һ�����ݰ��Ƿ��ж�֡����
		if(*(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL) == 0xA5)
		{
			//���һ�����ݰ������˶�֡����,���ٴζ�ȡ
			JUDGE_Read_Data(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL);
		}
	}
	
	if (retval_tf == TRUE)
	{
		Judge_Data_TF = TRUE;//����������
	}
	else		//ֻҪCRC16У�鲻ͨ����ΪFALSE
	{
		Judge_Data_TF = FALSE;//����������
	}
	
	return retval_tf;//����������������
}

/*
void JUDGE_SendTextStruct(graphic_data_struct_t *textConf,uint8_t text[30],uint8_t len)
{
	JudgeTxFrame txFrame;
	ext_TextData_t textData;
	textData.txFrameHeader.SOF=0xA5;
	textData.txFrameHeader.DataLength=sizeof(ext_student_interactive_header_data_t)+sizeof(ext_client_custom_character_t);
	textData.txFrameHeader.Seq=0;
	memcpy(txFrame.data, &textData.txFrameHeader, sizeof(xFrameHeader));//д��֡ͷ����
	Append_CRC8_Check_Sum(txFrame.data, sizeof(xFrameHeader));//д��֡ͷCRC8У����
	
	textData.CmdID=0x301;//����֡ID
	textData.dataFrameHeader.data_cmd_id=0x0110;//���ݶ�ID
	textData.dataFrameHeader.send_ID 	 = JUDGE_GetSelfID();//�����ߵ�ID
	textData.dataFrameHeader.receiver_ID = JUDGE_GetClientID();//�ͻ��˵�ID��ֻ��Ϊ�����߻����˶�Ӧ�Ŀͻ���
	
	textData.textData.grapic_data_struct=*textConf;
	memcpy(textData.textData.data,text,len);
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&textData.CmdID,
		sizeof(textData.CmdID)+sizeof(textData.dataFrameHeader)+sizeof(textData.textData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(textData));
		
	txFrame.frameLength=sizeof(textData);
  //Queue_Enqueue(&judgeQueue,&txFrame);
 
}

void JUDGE_SendGraphStruct(graphic_data_struct_t *data)
{
	JudgeTxFrame txFrame;
	ext_GraphData_t graphData;
	graphData.txFrameHeader.SOF=0xA5;
	graphData.txFrameHeader.DataLength=sizeof(ext_student_interactive_header_data_t)+sizeof(ext_client_custom_graphic_single_t);
	graphData.txFrameHeader.Seq=0;
	memcpy(txFrame.data, &graphData.txFrameHeader, sizeof(xFrameHeader));//д��֡ͷ����
	Append_CRC8_Check_Sum(txFrame.data, sizeof(xFrameHeader));//д��֡ͷCRC8У����
	
	graphData.CmdID=0x301;//����֡ID
	graphData.dataFrameHeader.data_cmd_id=0x0101;//���ݶ�ID
	graphData.dataFrameHeader.send_ID 	 = JUDGE_GetSelfID();//�����ߵ�ID
	graphData.dataFrameHeader.receiver_ID = JUDGE_GetClientID();//�ͻ��˵�ID��ֻ��Ϊ�����߻����˶�Ӧ�Ŀͻ���
	
	graphData.graphData.grapic_data_struct=*data;
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&graphData.CmdID,
		sizeof(graphData.CmdID)+sizeof(graphData.dataFrameHeader)+sizeof(graphData.graphData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(graphData));
		
	txFrame.frameLength=sizeof(graphData);
 // Queue_Enqueue(&judgeQueue,&txFrame);
}

*/
#endif


