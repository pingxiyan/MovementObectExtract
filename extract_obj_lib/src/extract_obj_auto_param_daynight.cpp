#include "extract_obj_auto_param_daynight.h"
#include "extract_obj_private.h"

#define MAX_ESTIMATE_FRAME_NUM 5 //视频中用于测试选取的最大帧数
#define MAX_GROUP 26 //亮度的分组数
#define HIGH_GROUP 24 //该组以上亮度近似灯光
#define MIDDLE_GROUP 11 //中性亮度组
#define COLOR_DIFF_THRESH 20 //偏色阈值
#define GRAY_SCALE_THRESH 0.99 //灰度判断阈值
#define LOW_LUM_SCALE_THRESH 0.4 //低亮度比例阈值
#define HIGH_PEAK_THRESH 1.5 //判断高亮度峰值突出程度的阈值
#define PEAK_THRESH 2.5 //高低亮度峰值比值的阈值

bool CDayNight::processOneFrame(uint8_t *pY, int width, int height, uint8_t *pU, uint8_t *pV)
{
	int l32Y = 0;	//记录y,u,v值
	int l32U = 0;
	int l32V = 0;
	int l32YAvg = 0;//存储亮度均值
	int l32Red = 0;	//三色像素值
	int l32Green = 0;
	int l32Blue = 0;
	int l32HalfHeight = height / 2;//计算用到的高与宽
	int l32HalfWidth = width / 2;
	int l32Max = 0;		//三色中像素值最高值
	int l32GrayPix = 0;	//灰度像素数量统计
	int l32PixSum = l32HalfWidth * l32HalfHeight;//计算用到1/4的总分辨率
	int l32CountI = 0;	//循环变量
	int l32CountJ = 0;
	int al32YHist[MAX_GROUP] = { 0 };//亮度分为26组进行像素大小统计
	float f32GrayScale = 0;//灰度像素占比
	int l32RedAvg = 0;//三色像素平均值
	int l32BlueAvg = 0;
	int l32GreenAvg = 0;
	int l32HighPeakPos = 0;//高亮度区像素总数占比最多的组数
	int l32LowPeakPos = 0;//低亮度区像素总数占比最多的组数
	float f32LowSum = 0;   //低亮度区比例之和
	int l32HighAvg = 0;  //高亮度区各组像素点数量平均值

						 //用bgr中最高值代替亮度进行统计,取1/4的点进行计算  
	for (int l32CountI = 0; l32CountI < l32HalfHeight; l32CountI++, pY += 2 * l32HalfWidth)
	{
		for (int l32CountJ = 0; l32CountJ < l32HalfWidth; l32CountJ++, pY += 2, pU++, pV++)
		{
			l32Y = *pY;
			l32U = *pU;
			l32V = *pV;
			if ((ABS(l32U - 128) < 5) && (ABS(l32V - 128) < 5))//灰度点判断
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
			l32YAvg += l32Max;//用bgr中最高值代替亮度进行统计
			al32YHist[l32Max / 10]++;//亮度分组
		}
	}

	l32RedAvg = l32RedAvg / l32PixSum;
	l32GreenAvg = l32GreenAvg / l32PixSum;
	l32BlueAvg = l32BlueAvg / l32PixSum;
	l32YAvg = l32YAvg / l32PixSum;

	//找到低亮度区间的最高比例所在组
	for (l32CountI = 0; l32CountI < MIDDLE_GROUP - 1; l32CountI++)
	{
		if (al32YHist[l32CountI] > al32YHist[l32LowPeakPos])
		{
			l32LowPeakPos = l32CountI;
		}
	}

	//找到高亮度区间的最高比例所在组
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

	//判断
	if ((l32RedAvg - l32BlueAvg) >= COLOR_DIFF_THRESH
		|| (l32BlueAvg - l32RedAvg) >= COLOR_DIFF_THRESH
		|| (l32GreenAvg - l32BlueAvg) >= COLOR_DIFF_THRESH//红绿蓝色偏光判断为夜
		|| f32GrayScale > GRAY_SCALE_THRESH)//接近全灰度判断为夜
	{
		return false;
	}

	if (f32LowSum > LOW_LUM_SCALE_THRESH)//低亮度像素统计过高时，可能是夜晚，但先要经过2次条件判断是否为白天
	{
		if ((HIGH_PEAK_THRESH * l32HighAvg < al32YHist[l32HighPeakPos] //高亮度区像素值比较集中分布在峰位处
			&& al32YHist[l32HighPeakPos] * PEAK_THRESH > al32YHist[l32LowPeakPos]))//高亮度峰值与低亮度峰值的差值要在一定范围内     
		{
			return true;
		}
		return false;
	}

	return true;//其他情况判断为白天
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
	//如果检测为白天的帧数占到一半及以上，判断为白天
	return (2 * _frmNumDay >= _frmNumProcessed) ? true : false;
}