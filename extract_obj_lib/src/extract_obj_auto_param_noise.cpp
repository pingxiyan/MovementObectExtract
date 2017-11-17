#include "extract_obj_private.h"
#include "extract_obj_auto_param_noise.h"

#define MAX_PROC_FRAME_NUM 90

#define PS_NOISE_RATIO 5          //噪声点比例阈值，比例低于此表示较好
#define PS_STRETCH_CONST 6000     //把指标拉伸到0-100的常数

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
	_frmNum++;   //输入帧数
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
	uint8_t *pu8PreImg;                         //前帧
	uint8_t *pu8TmpCur;                         //当前帧
	uint8_t *pu8AbsDiffImg;                     //前景含噪声
	uint8_t *pu8Temp;
	int *pl32NoiseFreq;                    //统计前景点出现频数的内存临时指针
	int *pl32NoiseTemp;                    //跳变强度统计的内存临时指针
	int *pl32IntensityTemp;                //跳变处背景强度的内存临时指针
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

	//帧差
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
	l32FGDiff = 2 * l32Sum / (l32DiffCount + 1);  //根据前景点均值自适应调整阈值

	//统计前景点出现的次数和变化的强度,并去除邻域，不考虑四边框  
	for (l32Row = 1; l32Row < halfH - 1; l32Row++)
	{
		l32SkipLine = halfW * l32Row;
		pu8AbsDiffImg = _diffGrayImg.data + l32SkipLine;
		pu8PreImg = _preGrayImg.data + l32SkipLine;
		pu8TmpCur = _curGrayImg.data + l32SkipLine;
		pl32NoiseFreq = _pl32NoiseFreq + l32SkipLine;             //指向统计前景点出现频次的内存
		pl32NoiseTemp = _pl32NoiseIntensity + l32SkipLine;        //统计噪点强度的内存
		pl32IntensityTemp = _pl32Intensity + l32SkipLine;         //统计背景强度的内存
		for (l32Col = 1; l32Col < halfW - 1; l32Col++)
		{
			//去除周围3乘3的领域内有相连的点
			bNoise = TRUE;
			if (pu8AbsDiffImg[l32Col] <= l32FGDiff) //当前像素点在相邻帧变化较小
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
				(pl32NoiseFreq[l32Col])++;                          //统计孤立前景点（可能噪点）出现的频率
				pl32NoiseTemp[l32Col] += pu8AbsDiffImg[l32Col];     //变化的强度  
				pl32IntensityTemp[l32Col] += (pu8TmpCur[l32Col] + pu8PreImg[l32Col] - pu8AbsDiffImg[l32Col]);//统计对应背景的强度
			}
		}
	}

	memcpy(_preGrayImg.data, _curGrayImg.data, _quarterImgSize);	//处理完后暂存入前帧
}

int CNoise::getResult()
{
	//综合判断视频噪声水平
	int l32NoiseNum = 0;
	float f32NoiseIntensity = 0;	//统计噪点跳变总强度
	for (int l32Idx = 0; l32Idx < _quarterImgSize; l32Idx++)
	{
		//统计噪点数量、出现频次、变化强度以及背景的强度
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

	/*用孤立噪点跳变的强度变化与噪点处背景信号的强度来表征噪声的强弱,数字越大，排序越靠后，最高100*/
	int l32Noise = 100 * (float)l32NoiseNum / _quarterImgSize;    //如果噪声比例很少，直接用比例表征
	if (l32Noise > PS_NOISE_RATIO)
	{
		l32Noise = PS_STRETCH_CONST * f32NoiseIntensity / _quarterImgSize;
	}

	int l32Noisy = (l32Noise > 100) ? 100 : l32Noise;     //把指标拉伸到0-100
	return l32Noisy;
}