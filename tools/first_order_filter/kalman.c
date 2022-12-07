#include "filter.h"

//һ�׿������˲��ṹ��
typedef struct
{
	Filter filter;

	float xLast; //��һʱ�̵����Ž��  X(k|k-1)
	float xPre;  //��ǰʱ�̵�Ԥ����  X(k|k-1)
	float xNow;  //��ǰʱ�̵����Ž��  X(k|k)
	float pPre;  //��ǰʱ��Ԥ������Э����  P(k|k-1)
	float pNow;  //��ǰʱ�����Ž����Э����  P(k|k)
	float pLast; //��һʱ�����Ž����Э����  P(k-1|k-1)
	float kg;     //kalman����
	float Q;
	float R;
}KalmanFilter;


Filter* Kalman_Init(ConfItem* dict);
float Kalman_Cala(Filter* filter, float data);

/**
  * @brief  ��ʼ��һ���������˲���
  * @param  kalman:  �˲���
  * @param  T_Q:ϵͳ����Э����
  * @param  T_R:��������Э����
  * @retval None
  * @attention R�̶���QԽ�󣬴���Խ���β���ֵ��Q�������ֻ�ò���ֵ
  *           ��֮��QԽС����Խ����ģ��Ԥ��ֵ��QΪ������ֻ��ģ��Ԥ��
  */
Filter* Kalman_Init(ConfItem* dict)
{
	KalmanFilter* kalman = (KalmanFilter*)FILTER_MALLOC_PORT(sizeof(KalmanFilter));
	memset(kalman,0,sizeof(KalmanFilter));

	kalman->filter.cala = Kalman_Cala;
	kalman->xLast = 0;
	kalman->pLast = 1;
	kalman->Q = Conf_GetValue(dict, "T-Q", float, 1);
	kalman->R = Conf_GetValue(dict, "T-R", float, 1);

	return (Filter*)kalman;
}


/**
  * @brief  �������˲���
  * @param  kalman:  �˲���
  * @param  data:���˲�����
  * @retval �˲��������
  * @attention Z(k)��ϵͳ����,������ֵ   X(k|k)�ǿ������˲����ֵ,���������
  *            A=1 B=0 H=1 I=1  W(K)  V(k)�Ǹ�˹������,�����ڲ���ֵ����,���Բ��ù�
  *            �����ǿ�������5�����Ĺ�ʽ
  *            һ��H'��Ϊ������,����Ϊת�þ���
  */
float Kalman_Cala(Filter* filter, float data)
{
    KalmanFilter* kalman = (KalmanFilter*)filter;

	kalman->xPre = kalman->xLast;                            //x(k|k-1) = A*X(k-1|k-1)+B*U(k)+W(K)
	kalman->pPre = kalman->pLast + kalman->Q;                     //p(k|k-1) = A*p(k-1|k-1)*A'+Q
	kalman->kg = kalman->pPre / (kalman->pPre + kalman->R);            //kg(k) = p(k|k-1)*H'/(H*p(k|k-1)*H'+R)
	kalman->xNow = kalman->xPre + kalman->kg * (data - kalman->xPre);  //x(k|k) = X(k|k-1)+kg(k)*(Z(k)-H*X(k|k-1))
	kalman->pNow = (1 - kalman->kg) * kalman->pPre;               //p(k|k) = (I-kg(k)*H)*P(k|k-1)
	kalman->pLast = kalman->pNow;                            //״̬����
	kalman->xLast = kalman->xNow;                            //״̬����
	return kalman->xNow;                                 //���Ԥ����x(k|k)
}
