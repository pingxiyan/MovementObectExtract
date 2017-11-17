#include "extract_obj_auto_param_daynight.h"
#include "extract_obj_private.h"

#define MAX_ESTIMATE_FRAME_NUM 5 //��Ƶ�����ڲ���ѡȡ�����֡��
#define MAX_GROUP 26 //���ȵķ�����
#define HIGH_GROUP 24 //�����������Ƚ��Ƶƹ�
#define MIDDLE_GROUP 11 //����������
#define COLOR_DIFF_THRESH 20 //ƫɫ��ֵ
#define GRAY_SCALE_THRESH 0.99 //�Ҷ��ж���ֵ
#define LOW_LUM_SCALE_THRESH 0.4 //�����ȱ�����ֵ
#define HIGH_PEAK_THRESH 1.5 //�жϸ����ȷ�ֵͻ���̶ȵ���ֵ
#define PEAK_THRESH 2.5 //�ߵ����ȷ�ֵ��ֵ����ֵ

bool CDayNight::processOneFrame(uint8_t *pY, int width, int height, uint8_t *pU, uint8_t *pV)
{
	int l32Y = 0;	//��¼y,u,vֵ
	int l32U = 0;
	int l32V = 0;
	int l32YAvg = 0;//�洢���Ⱦ�ֵ
	int l32Red = 0;	//��ɫ����ֵ
	int l32Green = 0;
	int l32Blue = 0;
	int l32HalfHeight = height / 2;//�����õ��ĸ����
	int l32HalfWidth = width / 2;
	int l32Max = 0;		//��ɫ������ֵ���ֵ
	int l32GrayPix = 0;	//�Ҷ���������ͳ��
	int l32PixSum = l32HalfWidth * l32HalfHeight;//�����õ�1/4���ֱܷ���
	int l32CountI = 0;	//ѭ������
	int l32CountJ = 0;
	int al32YHist[MAX_GROUP] = { 0 };//���ȷ�Ϊ26��������ش�Сͳ��
	float f32GrayScale = 0;//�Ҷ�����ռ��
	int l32RedAvg = 0;//��ɫ����ƽ��ֵ
	int l32BlueAvg = 0;
	int l32GreenAvg = 0;
	int l32HighPeakPos = 0;//����������������ռ����������
	int l32LowPeakPos = 0;//����������������ռ����������
	float f32LowSum = 0;   //������������֮��
	int l32HighAvg = 0;  //���������������ص�����ƽ��ֵ

						 //��bgr�����ֵ�������Ƚ���ͳ��,ȡ1/4�ĵ���м���  
	for (int l32CountI = 0; l32CountI < l32HalfHeight; l32CountI++, pY += 2 * l32HalfWidth)
	{
		for (int l32CountJ = 0; l32CountJ < l32HalfWidth; l32CountJ++, pY += 2, pU++, pV++)
		{
			l32Y = *pY;
			l32U = *pU;
			l32V = *pV;
			if ((ABS(l32U - 128) < 5) && (ABS(l32V - 128) < 5))//�Ҷȵ��ж�
			{
				l32GrayPix++;
			}
			l32Red = RANGE_UCHAR((298 * (l32Y - 16) + 408 * (l32V - 128)) >> 8);
			l32Green = RANGE_UCHAR((298 * (l32Y - 16) - 208 * (l32V - 128) - 100 * (l32U - 128)) >> 8);
			l32Blue = RANGE_UCHAR((298 * (l32Y - 16) + 516 * (l32U - 128)) >> 8);

			l32RedAvg += l32Red;
			l32GreenAvg += l32Green;
			l32BlueAvg += l32Blue;
			l32Max = (l32Red > l32Green) ? l32Red : l32Green;
			l32Max = (l32Max > l32Blue) ? l32Max : l32Blue;
			l32YAvg += l32Max;//��bgr�����ֵ�������Ƚ���ͳ��
			al32YHist[l32Max / 10]++;//���ȷ���
		}
	}

	l32RedAvg = l32RedAvg / l32PixSum;
	l32GreenAvg = l32GreenAvg / l32PixSum;
	l32BlueAvg = l32BlueAvg / l32PixSum;
	l32YAvg = l32YAvg / l32PixSum;

	//�ҵ��������������߱���������
	for (l32CountI = 0; l32CountI < MIDDLE_GROUP - 1; l32CountI++)
	{
		if (al32YHist[l32CountI] > al32YHist[l32LowPeakPos])
		{
			l32LowPeakPos = l32CountI;
		}
	}

	//�ҵ��������������߱���������
	l32HighPeakPos = MIDDLE_GROUP;
	for (l32CountI = MIDDLE_GROUP; l32CountI < HIGH_GROUP; l32CountI++)
	{
		if (al32YHist[l32CountI] > al32YHist[l32HighPeakPos])
		{
			l32HighPeakPos = l32CountI;
		}
	}

	for (l32CountI = 0; l32CountI <= MIDDLE_GROUP; l32CountI++)
	{
		f32LowSum += (float)(al32YHist[l32CountI]) / l32PixSum;
	}

	for (l32CountI = MIDDLE_GROUP + 1; l32CountI < HIGH_GROUP; l32CountI++)
	{
		l32HighAvg += al32YHist[l32CountI] / (HIGH_GROUP - MIDDLE_GROUP - 1);
	}

	f32GrayScale = (float)(l32GrayPix) / l32PixSum;

	//�ж�
	if ((l32RedAvg - l32BlueAvg) >= COLOR_DIFF_THRESH
		|| (l32BlueAvg - l32RedAvg) >= COLOR_DIFF_THRESH
		|| (l32GreenAvg - l32BlueAvg) >= COLOR_DIFF_THRESH//������ɫƫ���ж�Ϊҹ
		|| f32GrayScale > GRAY_SCALE_THRESH)//�ӽ�ȫ�Ҷ��ж�Ϊҹ
	{
		return false;
	}

	if (f32LowSum > LOW_LUM_SCALE_THRESH)//����������ͳ�ƹ���ʱ��������ҹ������Ҫ����2�������ж��Ƿ�Ϊ����
	{
		if ((HIGH_PEAK_THRESH * l32HighAvg < al32YHist[l32HighPeakPos] //������������ֵ�Ƚϼ��зֲ��ڷ�λ��
			&& al32YHist[l32HighPeakPos] * PEAK_THRESH > al32YHist[l32LowPeakPos]))//�����ȷ�ֵ������ȷ�ֵ�Ĳ�ֵҪ��һ����Χ��     
		{
			return true;
		}
		return false;
	}

	return true;//��������ж�Ϊ����
}

void CDayNight::putOneFrame(uint8_t *pYuv, int width, int height)
{
	/**
	* Each 10 frame detect once, max detect 10 frames.
	*/
	if (_frmNum > MAX_ESTIMATE_FRAME_NUM) {
		return;
	}

	int ySize = width * height;
	uint8_t *pU = pYuv + ySize;
	uint8_t *pV = pU + (ySize >> 2);

	bool bDay = processOneFrame(pYuv, width, height, pU, pV);
	if (bDay) {
		_frmNumDay++;
	}

	_frmNumProcessed++;
	_frmNum++;
}

bool CDayNight::getResult()
{
	//������Ϊ�����֡��ռ��һ�뼰���ϣ��ж�Ϊ����
	return (2 * _frmNumDay >= _frmNumProcessed) ? true : false;
}