#include "extract_obj_private.h"
#include "extract_obj_auto_param_noise.h"

#define MAX_PROC_FRAME_NUM 90

#define PS_NOISE_RATIO 5          //�����������ֵ���������ڴ˱�ʾ�Ϻ�
#define PS_STRETCH_CONST 6000     //��ָ�����쵽0-100�ĳ���

void CNoise::initialNeighbor(int width, int height)
{
	_neighbor[0] = -width - 1;
	_neighbor[1] = -width;
	_neighbor[2] = -width + 1;
	_neighbor[3] = -1;
	_neighbor[4] = 1;
	_neighbor[5] = width - 1;
	_neighbor[6] = width;
	_neighbor[7] = width + 1;
}

void CNoise::neighbor4PiexlDiff(uint8_t *pYuv, int width, int height, cv::Mat& gray)
{
	for (int l32Row = 0; l32Row < gray.rows; l32Row++)
	{
		uint8_t* pu8TmpY = (uint8_t *)pYuv + 2 * l32Row * width;
		uint8_t* pu8TmpCur = gray.data + l32Row * gray.cols;

		for (int l32Col = 0; l32Col < gray.cols; l32Col++)
		{
			int col2 = l32Col + l32Col;
			pu8TmpCur[l32Col] = ((int)pu8TmpY[col2] + pu8TmpY[col2 + 1] + pu8TmpY[col2 + width] + pu8TmpY[col2 + width + 1]) >> 2;
		}
	}
}

void CNoise::putOneFrame(uint8_t *pYuv, int width, int height)
{
	_frmNum++;   //����֡��
	if (_frmNum > MAX_PROC_FRAME_NUM) {
		return ;
	}

	/**
	* Only process Y channel, 
	*/

	int l32Ret = 0;
	int l32Row, l32Col, l32Index;
	int l32FGDiff, l32AbsDiff, l32DiffCount, l32Sum;
	int l32SkipLine;
	uint8_t *pu8PreImg;                         //ǰ֡
	uint8_t *pu8TmpCur;                         //��ǰ֡
	uint8_t *pu8AbsDiffImg;                     //ǰ��������
	uint8_t *pu8Temp;
	int *pl32NoiseFreq;                    //ͳ��ǰ�������Ƶ�����ڴ���ʱָ��
	int *pl32NoiseTemp;                    //����ǿ��ͳ�Ƶ��ڴ���ʱָ��
	int *pl32IntensityTemp;                //���䴦����ǿ�ȵ��ڴ���ʱָ��
	bool bNoise;

	int l32Stride = width;

	int halfH = height >> 1;
	int halfW = width >> 1;

	// Only initilize once.
	if (1 == _frmNum) {
		initialNeighbor(halfW, halfW);

		_quarterImgSize = halfH * halfW;

		// Init images
		_curGrayImg = cv::Mat(halfH, halfW, CV_8UC1);
		_preGrayImg = cv::Mat(halfH, halfW, CV_8UC1);
		_diffGrayImg = cv::Mat(halfH, halfW, CV_8UC1);

		createBuf(halfW, halfH);

		// Neighbor 4 pixel subtraction.
		neighbor4PiexlDiff(pYuv, width, height, _curGrayImg);
		memcpy(_preGrayImg.data, _curGrayImg.data, _quarterImgSize);
		return;
	}

	// Neighbor 4 pixel subtraction.
	neighbor4PiexlDiff(pYuv, width, height, _curGrayImg);

	//֡��
	l32Sum = 0;
	l32DiffCount = 0;
	for (l32Index = 0; l32Index < _quarterImgSize; l32Index++)
	{
		l32AbsDiff = ABS(_curGrayImg.data[l32Index] - _preGrayImg.data[l32Index]);
		_diffGrayImg.data[l32Index] = l32AbsDiff;
		if (l32AbsDiff > 0)
		{
			l32Sum += l32AbsDiff;
			l32DiffCount++;
		}
	}
	l32FGDiff = 2 * l32Sum / (l32DiffCount + 1);  //����ǰ�����ֵ����Ӧ������ֵ

	//ͳ��ǰ������ֵĴ����ͱ仯��ǿ��,��ȥ�����򣬲������ı߿�  
	for (l32Row = 1; l32Row < halfH - 1; l32Row++)
	{
		l32SkipLine = halfW * l32Row;
		pu8AbsDiffImg = _diffGrayImg.data + l32SkipLine;
		pu8PreImg = _preGrayImg.data + l32SkipLine;
		pu8TmpCur = _curGrayImg.data + l32SkipLine;
		pl32NoiseFreq = _pl32NoiseFreq + l32SkipLine;             //ָ��ͳ��ǰ�������Ƶ�ε��ڴ�
		pl32NoiseTemp = _pl32NoiseIntensity + l32SkipLine;        //ͳ�����ǿ�ȵ��ڴ�
		pl32IntensityTemp = _pl32Intensity + l32SkipLine;         //ͳ�Ʊ���ǿ�ȵ��ڴ�
		for (l32Col = 1; l32Col < halfW - 1; l32Col++)
		{
			//ȥ����Χ3��3���������������ĵ�
			bNoise = TRUE;
			if (pu8AbsDiffImg[l32Col] <= l32FGDiff) //��ǰ���ص�������֡�仯��С
			{
				bNoise = FALSE;
			}
			else
			{
				for (l32Index = 0; l32Index < 8; l32Index++)
				{
					pu8Temp = pu8AbsDiffImg + l32Col;
					if (pu8Temp[_neighbor[l32Index]] > l32FGDiff)
					{
						bNoise = FALSE;
						break;
					}
				}
			}
			if (bNoise)
			{
				(pl32NoiseFreq[l32Col])++;                          //ͳ�ƹ���ǰ���㣨������㣩���ֵ�Ƶ��
				pl32NoiseTemp[l32Col] += pu8AbsDiffImg[l32Col];     //�仯��ǿ��  
				pl32IntensityTemp[l32Col] += (pu8TmpCur[l32Col] + pu8PreImg[l32Col] - pu8AbsDiffImg[l32Col]);//ͳ�ƶ�Ӧ������ǿ��
			}
		}
	}

	memcpy(_preGrayImg.data, _curGrayImg.data, _quarterImgSize);	//��������ݴ���ǰ֡
}

int CNoise::getResult()
{
	//�ۺ��ж���Ƶ����ˮƽ
	int l32NoiseNum = 0;
	float f32NoiseIntensity = 0;	//ͳ�����������ǿ��
	for (int l32Idx = 0; l32Idx < _quarterImgSize; l32Idx++)
	{
		//ͳ���������������Ƶ�Ρ��仯ǿ���Լ�������ǿ��
		if (_pl32NoiseFreq[l32Idx] > 0)
		{
			l32NoiseNum++;
			if (_pl32Intensity[l32Idx] > 0)
			{
				f32NoiseIntensity += (float)(_pl32NoiseIntensity[l32Idx]) / (_pl32Intensity[l32Idx]);
			}
			else
			{
				f32NoiseIntensity += _pl32NoiseIntensity[l32Idx];
			}
		}
	}

	/*�ù�����������ǿ�ȱ仯����㴦�����źŵ�ǿ��������������ǿ��,����Խ������Խ�������100*/
	int l32Noise = 100 * (float)l32NoiseNum / _quarterImgSize;    //��������������٣�ֱ���ñ�������
	if (l32Noise > PS_NOISE_RATIO)
	{
		l32Noise = PS_STRETCH_CONST * f32NoiseIntensity / _quarterImgSize;
	}

	int l32Noisy = (l32Noise > 100) ? 100 : l32Noise;     //��ָ�����쵽0-100
	return l32Noisy;
}