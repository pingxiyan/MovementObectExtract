#include "extract_obj_private.h"
#include "extract_obj_auto_param_contrast.h"

#define PS_LUMI_THRESHOLD 60         //偏亮偏暗阈值60%
#define PS_BOARDER_WIDTH 20          //边界宽度
#define PS_MIN_HEIGHT 36             //处理的最小高度
#define PS_VALID_HISTRATIO 200       //有效灰阶的阈值，单个灰阶点数量占总像素的1/200(这里灰度直方图都是针对的Y分量，
//灰阶分布从16到235，为方便处理，相邻4个灰阶合并，共得到55个灰阶)
#define PS_HALF_GRAYSCALE 31         //总灰阶数的1/2处的坐标
#define PS_GRAYSCALE_NUM 7           //从低到高或者从高到低数总灰阶的1/8的个数
#define PS_GRAYSCALE_EXTREME 11      //用于判断极亮极暗情况的邻近灰阶数量,总灰阶数的1/5


#define MAX_PROC_FRAME_NUM (PS_ROWS_RATIO*12)


/**
* @brief Calculating image sharpness.
* @param pSrc :　image.
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
	l32BoundaryDiff = l32Avg - 16;                //根据图像灰度均值自适应确定高通阈值,由于此处灰度值
												  //从16到235，最大差值为219，为避免出现阈值过大，统一减去16

												  //高通滤波
	l32HPNum = 0;
	l32HPSum = 0;
	for (l32Row = 1; l32Row < height - 1; l32Row++)
	{
		pSrcTemp = pSrc + width * l32Row;
		for (l32Col = 1; l32Col < width - 1; l32Col++)
		{
			pTemp = pSrcTemp + l32Col;

			//过滤非边界点
			l32Tmp = 0;                                     //周围差值大于阈值的点计数
			for (l32Idx = 0; l32Idx < 8; l32Idx++)
			{
				l32Diff = pSrcTemp[l32Col] - pTemp[neighbor[l32Idx]];
				if (l32Diff > l32BoundaryDiff || l32Diff < -l32BoundaryDiff)
				{
					l32Tmp++;
				}
			}

			//统计高通滤波后的点，如果是孤立点噪声则不计入高频边界点
			if (l32Tmp > 1 && l32Tmp < 8)
			{
				pHPSrc[l32HPNum] = pSrcTemp[l32Col];
				l32HPNum++;
				l32HPSum += pSrcTemp[l32Col];    //统计高频点的总灰度值
			}
		}
	}
	l32HPAvg = (l32HPSum + (l32HPNum >> 1)) / (l32HPNum + 1);

	//计算ACM和HPACM
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
* @brief Jugde whether image have problem or not.(针对YUV420的Y分量直方图分布为16-235)
* @param pHist : histgram.
* @param imgSize
* @return 正常--10；极亮极暗--2；偏亮偏暗严重--3；偏亮偏暗--6
*/
static int EvaContrast(int *pHist, int imgSize)
{

	int l32HighLightNum, l32LowLightNum, l32DarkNum, l32BrightNum;
	int l32LimitNum, l32Idx, l32ContrastCoe;
	int al32SmallHist[64] = { 0 };
	bool bExtremeDrak = TRUE;
	bool bExtremeBright = TRUE;

	/*将原直方图缩小为64阶判断，因为这里YUV420的Y分量直方图的灰阶分布为16-235，
	所以缩小后的直方图有效下标为4-58 */
	for (l32Idx = 0; l32Idx < 256; l32Idx++)
	{
		al32SmallHist[l32Idx / 4] += pHist[l32Idx];
	}

	/*直方图两端（对应下标为4和58）的灰阶点数比邻近的灰阶（总灰阶数的1/5个灰阶，
	即从两端开始数各11个）的数量都要大4倍以上，则认为是有极端亮暗的情况*/
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

	//给极亮极暗的情况降低评分
	if (bExtremeBright || bExtremeDrak)
	{
		l32ContrastCoe = 2;
		return l32ContrastCoe;
	}

	//检测是否整体偏亮或偏暗，在此基础上查看偏亮偏暗的严重程度
	l32LowLightNum = 0;
	l32HighLightNum = 0;
	l32DarkNum = 0;
	l32BrightNum = 0;

	/*统计总灰阶数前1/2个灰阶(直方图下标从4到31)的总点数和从低到高由有效灰阶开始
	连续数总灰阶数的1/8个灰阶的点的数量，分别记为l32LowLightNum和l32DarkNum*/
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

	//整体偏暗降低评分
	if (l32LowLightNum * 100 > imgSize * PS_LUMI_THRESHOLD)     //整体偏暗的点大于总像素点的60%
	{
		l32ContrastCoe = 6;
		if (l32DarkNum * 300 > imgSize * PS_LUMI_THRESHOLD)     //偏暗严重的点大于总像素的20%
		{
			l32ContrastCoe = 3;
		}
		return l32ContrastCoe;
	}

	/*统计总灰阶数后1/2个灰阶(直方图下标从31到58)的总点数和从高到低由有效灰阶开始
	连续数总灰阶数的1/8个灰阶的点的数量，分别记为l32HighLightNum和l32BrightNum*/
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

	//整体偏暗降低评分
	if (l32HighLightNum * 100 > imgSize * PS_LUMI_THRESHOLD)    //整体偏亮的点大于总像素点的60%
	{
		l32ContrastCoe = 6;
		if (l32BrightNum * 300 > imgSize * PS_LUMI_THRESHOLD)    //偏亮严重的点大于总像素的20%
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

	//判断参数是否正常
	if (width <= PS_BOARDER_WIDTH * 2 + 2 ||
		height < PS_MIN_HEIGHT)
	{
		//如果图像宽度不满42个像素或者高度小于36，直接认为是模糊的，退出
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

	//上下裁掉1/6高度，去除字幕，两边去掉20个像素，去掉黑边
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
			al32Hist[pu8YTmp[l32Col]]++;                  //统计直方图
		}
	}

	//计算高频绝对中心距与基本中心距的加权值作为清晰度的量化指标
	l32ProcLines = l32ProcHeight / PS_ROWS_RATIO;
	if (PS_ROWS_RATIO * (l32ProcHeight - _startRow) < l32ProcHeight)
	{
		_startRow = 0;
		_pDefinition = _definition;
	}
	l32Contrast = Sharp(_curGray.data + _startRow * l32ProcWidth, _filterImg.data,
		l32ProcWidth, l32ProcLines);
	_startRow += l32ProcLines;

	//对比度评级
	l32ContrastCoe = EvaContrast(al32Hist, l32ProcImgSize);

	//把图像从上至下分成N个块，每次顺序往下扫描，取对应块的最大值
	if (*_pDefinition < l32ContrastCoe * l32Contrast)
	{
		*_pDefinition = l32ContrastCoe * l32Contrast;
	}
	_pDefinition++;

	_frmNum++;//统计处理的总帧数
}

int CContrast::getResult()
{
	int l32Ret = 0;
	int l32Index;
	float f32Definition;

	//默认图像细节的丰富程度跟图像尺寸有关,便于不同分辨率的视频比较，并把指标调整到0-100之间
	f32Definition = 0;
	for (l32Index = 0; l32Index < 8; l32Index++)
	{
		f32Definition += _definition[l32Index];
	}
	f32Definition = sqrt((float)(_widht)) * f32Definition / 3200;
	
	int l32Contrast = (f32Definition > 100) ? 100 : f32Definition;
	return l32Contrast;
}

