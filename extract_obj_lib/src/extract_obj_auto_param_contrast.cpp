#include "extract_obj_private.h"
#include "extract_obj_auto_param_contrast.h"

#define PS_LUMI_THRESHOLD 60         //ƫ��ƫ����ֵ60%
#define PS_BOARDER_WIDTH 20          //�߽���
#define PS_MIN_HEIGHT 36             //�������С�߶�
#define PS_VALID_HISTRATIO 200       //��Ч�ҽ׵���ֵ�������ҽ׵�����ռ�����ص�1/200(����Ҷ�ֱ��ͼ������Ե�Y������
//�ҽ׷ֲ���16��235��Ϊ���㴦������4���ҽ׺ϲ������õ�55���ҽ�)
#define PS_HALF_GRAYSCALE 31         //�ܻҽ�����1/2��������
#define PS_GRAYSCALE_NUM 7           //�ӵ͵��߻��ߴӸߵ������ܻҽ׵�1/8�ĸ���
#define PS_GRAYSCALE_EXTREME 11      //�����жϼ�������������ڽ��ҽ�����,�ܻҽ�����1/5


#define MAX_PROC_FRAME_NUM (PS_ROWS_RATIO*12)


/**
* @brief Calculating image sharpness.
* @param pSrc :��image.
* @param pHPSrc : high frequency point of 'pSrc'.
* @param width, height: image size.
* @return int : sharpness.
*/
static int Sharp(uint8_t *pSrc, uint8_t *pHPSrc, int width, int height)
{
	const int neighbor[8] = { -width - 1, -width, -width + 1, -1, 1,
		width - 1, width, width + 1 };

	int l32Num = (width - 2) * (height - 2);
	int l32HPNum, l32HPSum, l32Tmp;
	int l32Sum, l32Avg, l32HPAvg, l32BoundaryDiff;
	int l32Col, l32Row, l32Idx;
	int l32ACM, l32HPACM, l32Sharp, l32Diff;
	uint8_t *pTemp, *pSrcTemp;

	l32Sum = 0;
	for (l32Row = 1; l32Row < height - 1; l32Row++)
	{
		pSrcTemp = pSrc + width * l32Row;
		for (l32Col = 1; l32Col < width - 1; l32Col++)
		{
			l32Sum += pSrcTemp[l32Col];
		}
	}
	l32Avg = (l32Sum + (l32Num >> 1)) / l32Num;
	l32BoundaryDiff = l32Avg - 16;                //����ͼ��ҶȾ�ֵ����Ӧȷ����ͨ��ֵ,���ڴ˴��Ҷ�ֵ
												  //��16��235������ֵΪ219��Ϊ���������ֵ����ͳһ��ȥ16

												  //��ͨ�˲�
	l32HPNum = 0;
	l32HPSum = 0;
	for (l32Row = 1; l32Row < height - 1; l32Row++)
	{
		pSrcTemp = pSrc + width * l32Row;
		for (l32Col = 1; l32Col < width - 1; l32Col++)
		{
			pTemp = pSrcTemp + l32Col;

			//���˷Ǳ߽��
			l32Tmp = 0;                                     //��Χ��ֵ������ֵ�ĵ����
			for (l32Idx = 0; l32Idx < 8; l32Idx++)
			{
				l32Diff = pSrcTemp[l32Col] - pTemp[neighbor[l32Idx]];
				if (l32Diff > l32BoundaryDiff || l32Diff < -l32BoundaryDiff)
				{
					l32Tmp++;
				}
			}

			//ͳ�Ƹ�ͨ�˲���ĵ㣬����ǹ����������򲻼����Ƶ�߽��
			if (l32Tmp > 1 && l32Tmp < 8)
			{
				pHPSrc[l32HPNum] = pSrcTemp[l32Col];
				l32HPNum++;
				l32HPSum += pSrcTemp[l32Col];    //ͳ�Ƹ�Ƶ����ܻҶ�ֵ
			}
		}
	}
	l32HPAvg = (l32HPSum + (l32HPNum >> 1)) / (l32HPNum + 1);

	//����ACM��HPACM
	l32ACM = 0;
	l32HPACM = 0;
	for (l32Row = 1; l32Row < height - 1; l32Row++)
	{
		pSrcTemp = pSrc + width * l32Row;
		for (l32Col = 1; l32Col < width - 1; l32Col++)
		{
			l32ACM += ABS(pSrcTemp[l32Col] - l32Avg);
		}
	}
	for (l32Idx = 0; l32Idx < l32HPNum; l32Idx++)
	{
		l32HPACM += ABS(pHPSrc[l32Idx] - l32HPAvg);
	}
	l32Sharp = l32ACM / l32Num + l32HPACM / (l32HPNum + 1);

	return l32Sharp;
}

/**
* @brief Jugde whether image have problem or not.(���YUV420��Y����ֱ��ͼ�ֲ�Ϊ16-235)
* @param pHist : histgram.
* @param imgSize
* @return ����--10����������--2��ƫ��ƫ������--3��ƫ��ƫ��--6
*/
static int EvaContrast(int *pHist, int imgSize)
{

	int l32HighLightNum, l32LowLightNum, l32DarkNum, l32BrightNum;
	int l32LimitNum, l32Idx, l32ContrastCoe;
	int al32SmallHist[64] = { 0 };
	bool bExtremeDrak = TRUE;
	bool bExtremeBright = TRUE;

	/*��ԭֱ��ͼ��СΪ64���жϣ���Ϊ����YUV420��Y����ֱ��ͼ�Ļҽ׷ֲ�Ϊ16-235��
	������С���ֱ��ͼ��Ч�±�Ϊ4-58 */
	for (l32Idx = 0; l32Idx < 256; l32Idx++)
	{
		al32SmallHist[l32Idx / 4] += pHist[l32Idx];
	}

	/*ֱ��ͼ���ˣ���Ӧ�±�Ϊ4��58���Ļҽ׵������ڽ��Ļҽף��ܻҽ�����1/5���ҽף�
	�������˿�ʼ����11������������Ҫ��4�����ϣ�����Ϊ���м������������*/
	bExtremeDrak = TRUE;
	for (l32Idx = 0; l32Idx < PS_GRAYSCALE_EXTREME; l32Idx++)
	{
		if (al32SmallHist[4] <= 4 * al32SmallHist[l32Idx + 5])
		{
			bExtremeDrak = FALSE;
			break;
		}
	}
	bExtremeBright = TRUE;
	for (l32Idx = 0; l32Idx < PS_GRAYSCALE_EXTREME; l32Idx++)
	{
		if (al32SmallHist[58] <= 4 * al32SmallHist[57 - l32Idx])
		{
			bExtremeBright = FALSE;
			break;
		}
	}

	//�����������������������
	if (bExtremeBright || bExtremeDrak)
	{
		l32ContrastCoe = 2;
		return l32ContrastCoe;
	}

	//����Ƿ�����ƫ����ƫ�����ڴ˻����ϲ鿴ƫ��ƫ�������س̶�
	l32LowLightNum = 0;
	l32HighLightNum = 0;
	l32DarkNum = 0;
	l32BrightNum = 0;

	/*ͳ���ܻҽ���ǰ1/2���ҽ�(ֱ��ͼ�±��4��31)���ܵ����ʹӵ͵�������Ч�ҽ׿�ʼ
	�������ܻҽ�����1/8���ҽ׵ĵ���������ֱ��Ϊl32LowLightNum��l32DarkNum*/
	l32LimitNum = 0;
	for (l32Idx = 4; l32Idx < PS_HALF_GRAYSCALE; l32Idx++)
	{
		l32LowLightNum += al32SmallHist[l32Idx];
		if (al32SmallHist[l32Idx] * PS_VALID_HISTRATIO > imgSize)
		{
			if (l32LimitNum >= PS_GRAYSCALE_NUM)
			{
				continue;
			}
			l32DarkNum += al32SmallHist[l32Idx];
			l32LimitNum++;
		}
	}

	//����ƫ����������
	if (l32LowLightNum * 100 > imgSize * PS_LUMI_THRESHOLD)     //����ƫ���ĵ���������ص��60%
	{
		l32ContrastCoe = 6;
		if (l32DarkNum * 300 > imgSize * PS_LUMI_THRESHOLD)     //ƫ�����صĵ���������ص�20%
		{
			l32ContrastCoe = 3;
		}
		return l32ContrastCoe;
	}

	/*ͳ���ܻҽ�����1/2���ҽ�(ֱ��ͼ�±��31��58)���ܵ����ʹӸߵ�������Ч�ҽ׿�ʼ
	�������ܻҽ�����1/8���ҽ׵ĵ���������ֱ��Ϊl32HighLightNum��l32BrightNum*/
	l32LimitNum = 0;
	for (l32Idx = 58; l32Idx > PS_HALF_GRAYSCALE; l32Idx--)
	{
		l32HighLightNum += al32SmallHist[l32Idx];
		if (al32SmallHist[l32Idx] * PS_VALID_HISTRATIO > imgSize)
		{
			if (l32LimitNum >= PS_GRAYSCALE_NUM)
			{
				continue;
			}
			l32BrightNum += al32SmallHist[l32Idx];
			l32LimitNum++;
		}
	}

	//����ƫ����������
	if (l32HighLightNum * 100 > imgSize * PS_LUMI_THRESHOLD)    //����ƫ���ĵ���������ص��60%
	{
		l32ContrastCoe = 6;
		if (l32BrightNum * 300 > imgSize * PS_LUMI_THRESHOLD)    //ƫ�����صĵ���������ص�20%
		{
			l32ContrastCoe = 3;
		}
	}
	else
	{
		l32ContrastCoe = 10;
	}

	return l32ContrastCoe;
}

void CContrast::putOneFrame(uint8_t *pYuv, int width, int height)
{
	if (_frmNum > MAX_PROC_FRAME_NUM) {
		_frmNum++;  
		_bFinished = true;
		return ;
	}

	int al32Hist[256] = { 0 };
	int l32Row, l32Col;
	int l32Width, l32Height, l32ProcWidth, l32ProcHeight, l32ProcImgSize, l32Stride;
	int l32StartRow, l32EndRow, l32ContrastCoe, l32Contrast, l32ProcLines;
	uint8_t *pu8YTmp, *pu8Src;

	//�жϲ����Ƿ�����
	if (width <= PS_BOARDER_WIDTH * 2 + 2 ||
		height < PS_MIN_HEIGHT)
	{
		//���ͼ���Ȳ���42�����ػ��߸߶�С��36��ֱ����Ϊ��ģ���ģ��˳�
		_bFinished = true;
		_widht = width;
		return;
	}

	if (_frmNum == 0) {
		_widht = width;
		_curGray = cv::Mat(height, width, CV_8UC1);
		_filterImg = cv::Mat(height, width, CV_8UC1);
	}

	l32Stride = width;

	//���²õ�1/6�߶ȣ�ȥ����Ļ������ȥ��20�����أ�ȥ���ڱ�
	l32StartRow = l32Height / 6;
	l32EndRow = l32Height - l32Height / 6;
	l32ProcWidth = l32Width - PS_BOARDER_WIDTH * 2;
	l32ProcHeight = l32EndRow - l32StartRow;
	l32ProcImgSize = l32ProcWidth * l32ProcHeight;
	for (l32Row = l32StartRow; l32Row < l32EndRow; l32Row++)
	{
		pu8YTmp = (uint8_t *)(pYuv + l32Row * l32Stride);
		pu8Src = _curGray.data + (l32Row - l32StartRow) * l32ProcWidth;
		for (l32Col = PS_BOARDER_WIDTH; l32Col < l32Width - PS_BOARDER_WIDTH; l32Col++)
		{
			pu8Src[l32Col - PS_BOARDER_WIDTH] = pu8YTmp[l32Col];
			al32Hist[pu8YTmp[l32Col]]++;                  //ͳ��ֱ��ͼ
		}
	}

	//�����Ƶ�������ľ���������ľ�ļ�Ȩֵ��Ϊ�����ȵ�����ָ��
	l32ProcLines = l32ProcHeight / PS_ROWS_RATIO;
	if (PS_ROWS_RATIO * (l32ProcHeight - _startRow) < l32ProcHeight)
	{
		_startRow = 0;
		_pDefinition = _definition;
	}
	l32Contrast = Sharp(_curGray.data + _startRow * l32ProcWidth, _filterImg.data,
		l32ProcWidth, l32ProcLines);
	_startRow += l32ProcLines;

	//�Աȶ�����
	l32ContrastCoe = EvaContrast(al32Hist, l32ProcImgSize);

	//��ͼ��������·ֳ�N���飬ÿ��˳������ɨ�裬ȡ��Ӧ������ֵ
	if (*_pDefinition < l32ContrastCoe * l32Contrast)
	{
		*_pDefinition = l32ContrastCoe * l32Contrast;
	}
	_pDefinition++;

	_frmNum++;//ͳ�ƴ������֡��
}

int CContrast::getResult()
{
	int l32Ret = 0;
	int l32Index;
	float f32Definition;

	//Ĭ��ͼ��ϸ�ڵķḻ�̶ȸ�ͼ��ߴ��й�,���ڲ�ͬ�ֱ��ʵ���Ƶ�Ƚϣ�����ָ�������0-100֮��
	f32Definition = 0;
	for (l32Index = 0; l32Index < 8; l32Index++)
	{
		f32Definition += _definition[l32Index];
	}
	f32Definition = sqrt((float)(_widht)) * f32Definition / 3200;
	
	int l32Contrast = (f32Definition > 100) ? 100 : f32Definition;
	return l32Contrast;
}

