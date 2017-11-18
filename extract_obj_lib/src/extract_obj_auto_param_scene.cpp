#include "extract_obj_private.h"
#include "extract_obj_auto_param_scense.h"

#include <opencv2/opencv.hpp>
#include <array>
#include <algorithm>
#include <vector>

using std::vector;

#define ERR_VANISHING_POINT 9999             //无法给出结果时，消隐点输出值
#define MAX_RATIO_THRESH 2                   //框体最大宽高比
#define MIN_RATIO_THRESH 0.3                 //框体最小宽高比
#define MIN_OBJ_FRAMES 20                    //目标跟踪的最少帧数
#define MIN_WHOLE_OBJ_FRAMES 10              //目标保持完整状态（不接触4个边界）的最少帧数
#define MAX_FRAMES 250
#define OBJ_TRECT 200                        //每个目标框体记录数量上限
#define OBJ_NUM 300                          //跟踪的目标数量上限
#define EDGE_MODIFY 1 / 50                   //画面边缘处理阈值
#define DISTANCE_THRESH 1 / 10               //目标位移比例阈值（相对画面尺寸）
#define RATIO_THRESH 0.2                     //目标相邻帧变化阈值
#define SIZE_CHANGE_THRESH1 1 / 30           //跟踪初始与结束时目标大小认为有明显变化的比例阈值（相对画面尺寸）
#define SIZE_CHANGE_THRESH2 1 / 3            //跟踪初始与结束时目标大小认为有明显变化的比例阈值（相对目标尺寸）
#define MIN_SUGGESTED_WIDTH 192              //输出的最小建议调整宽度
#define MAX_SUGGESTED_WIDTH 416              //输出的最大建议调整宽度
#define RESIZE_HEIGHT 216                    //处理时缩放高度
#define RESIZE_WIDTH 384                     //处理时缩放宽度
#define MIN_TRACE_HEIGHT 15                  //跟踪的最小高度默认值
#define MIN_TRACE_WIDTH 8                    //跟踪的最小宽度默认值

static int Median(int x, int y, int z)
{
	std::array<int, 3> arr = { x, y, z };
	// default ascending order.
	std::sort(arr.begin(), arr.end());
	return arr[1];
}

typedef struct tagDetRgn
{
	cv::Rect tRect;
	int l32Label;
	tagDetRgn()
	{
		l32Label = 0;
	}
}TDetRgn;

typedef struct
{
	TDetRgn atDetRgn[OBJ_TRECT];
	int l32Frames;
	int l32StartFrame;
}TPreScanObj;

/**
* @brief Current rectangle and previous rectangles compare.
* @return 与当前框体相匹配的前一帧框体的序号; -1=无框体匹配
*/
static int CompareRect(TDetRgn *ptPrevRgn, TDetRgn *ptCurRgn, int l32PrevNum)
{
	int l32i = 0;                        //循环变量
	int l32OutNum = 0;                   //输出结果
	int l32CrossedCnt = 0;               //框体出现重叠的计数
	int l32DistX = 0;                    //X方向距离
	int l32DistY = 0;                    //Y方向距离
	int l32PrevHeight = 0;               //前目标框高
	int l32PrevWidth = 0;                //前目标框宽
	int l32CurHeight = 0;                //当前目标框高
	int l32CurWidth = 0;                 //当前目标框宽
	float f32HeightRatio = 0;              //前后框高度比
	float f32WidthRatio = 0;               //前后框宽度比
	for (l32i = 0; l32i < l32PrevNum; l32i++)
	{
		l32DistX = MIN(ptPrevRgn[l32i].tRect.x + ptPrevRgn[l32i].tRect.width, ptCurRgn->tRect.x + ptCurRgn->tRect.width) - MAX(ptPrevRgn[l32i].tRect.x, ptCurRgn->tRect.x);
		l32DistY = MIN(ptPrevRgn[l32i].tRect.y + ptPrevRgn[l32i].tRect.height, ptCurRgn->tRect.y + ptCurRgn->tRect.height) - MAX(ptPrevRgn[l32i].tRect.y, ptCurRgn->tRect.y);

		//l32DistX和l32DistY均大于0表示框体有重叠
		if (l32DistX > 0 && l32DistY > 0)
		{
			//前目标框体若存在多次重叠，则该目标框体为无效
			if (ptPrevRgn[l32i].l32Label == -1)
			{
				return -1;
			}

			l32PrevHeight = (ptPrevRgn[l32i].tRect.height);
			l32PrevWidth = (ptPrevRgn[l32i].tRect.width);
			l32CurHeight = ptCurRgn->tRect.height;
			l32CurWidth = ptCurRgn->tRect.width;
			f32HeightRatio = (float)(l32CurHeight) / (float)(l32PrevHeight);
			f32WidthRatio = (float)(l32CurWidth) / (float)(l32PrevWidth);

			//前后重叠框体的长宽变化应在一定范围内
			if (abs(l32CurHeight - l32PrevHeight) < RATIO_THRESH * l32PrevHeight
				&& abs(l32CurWidth - l32PrevWidth) < MAX(5, RATIO_THRESH * l32PrevWidth))
			{
				l32OutNum = l32i;
				l32CrossedCnt++;
				//重叠数若超过1，则目标框体无效
				if (l32CrossedCnt > 1)
				{
					ptCurRgn->l32Label = -1;
					return -1;
				}
			}
		}
	}

	//前后框体唯一对应时，输出前框体对应ptPrevRgn的下标
	if (l32CrossedCnt == 1)
	{
		return l32OutNum;
	}
	return -1;
}

/**
* @brief 跟踪目标获取框体数据
* @param vtRgn			    当前帧前景框体数据
* @param tObj               目标记录结构体
* @param l32Frames          已读取帧数
* @param ptPrev             前一帧记录的框体数据
* @param pl32PrevNum        前一帧记录框体数
* @param pl32ObjNum         已记录目标数
* @return 成功--0, 失败--错误码
*/
static int TraceObj(vector<TDetRgn> &vtRgn, TPreScanObj *tObj, int l32Frames,
	TDetRgn *ptPrev, int *pl32PrevNum, int *pl32ObjNum)
{
	TDetRgn atCur[OBJ_TRECT];                         //当前帧的目标框体
	int l32CompResult = 0;                                 //CompareRect函数的结果
	int l32ObjLable = 0;                                   //Obj序号
	int l32ObjFrames = 0;                                  //单个Obj已记录帧数
	int l32CntI = 0;                                       //循环变量
	int l32CntJ = 0;                                       //循环变量
	int l32CntK = 0;                                       //循环变量
	int l32ObjNum = 0;                                     //记录的目标数量
	float f32WHRatio = 0;                                    //宽高比

														   //记录的目标数量达到规定数量则提前退出
	if (*pl32ObjNum >= OBJ_NUM - 1)
	{
		return 1;
	}

	//第一帧符合条件的框体记录在ptPrev[]结构体数组中，单帧目标框体记录数量少于FRAME_TRECT_NUM
	if (l32Frames == 1)
	{
		for (l32CntI = 0; l32CntI< vtRgn.size() && l32CntJ < OBJ_TRECT; l32CntI++)
		{
			f32WHRatio = (float)(vtRgn.at(l32CntI).tRect.width) / (float)(vtRgn.at(l32CntI).tRect.height);
			if (f32WHRatio <= MAX_RATIO_THRESH && f32WHRatio >= MIN_RATIO_THRESH)
			{
				ptPrev[l32CntJ].tRect = vtRgn.at(l32CntI).tRect;
				l32CntJ++;
			}
		}
		*pl32PrevNum = l32CntJ;//ptPrev[]结构体数组的存储数量
	}

	if (l32Frames > 1)
	{
		//tObj[]中已跟踪完毕但记录帧数低于MIN_OBJ_FRAMES的内存清空再利用
		for (l32CntI = 1; l32CntI <= *pl32ObjNum; l32CntI++)
		{
			if (l32Frames - tObj[l32CntI].l32StartFrame > tObj[l32CntI].l32Frames && tObj[l32CntI].l32Frames < MIN_OBJ_FRAMES)
			{
				memset(&tObj[l32CntI], 0, sizeof(TPreScanObj));
			}
		}

		for (l32CntI = 0, l32CntJ = 0; l32CntI < vtRgn.size() && l32CntJ < OBJ_TRECT; l32CntI++)
		{
			//第二帧开始符合条件的框体记录在tCur[]结构体数组中
			f32WHRatio = (float)(vtRgn.at(l32CntI).tRect.width) / (float)(vtRgn.at(l32CntI).tRect.height);
			if (f32WHRatio < MAX_RATIO_THRESH && f32WHRatio > MIN_RATIO_THRESH)
			{
				atCur[l32CntJ].tRect = vtRgn.at(l32CntI).tRect;
				l32CntJ++;
			}
			else
			{
				continue;
			}

			//对于有效的tCur[]，再和ptPrev中的所有框体进行比较，得到唯一对应的上帧中的框体，下标为l32CompResult
			l32CompResult = CompareRect(ptPrev, &atCur[l32CntJ - 1], *pl32PrevNum);
			if (l32CompResult > -1)
			{
				//若对应的前框体l32Lable为0，说明这两个框体为该目标记录的前两个框体，都应记录下来
				if (ptPrev[l32CompResult].l32Label == 0)
				{
					l32ObjNum = *pl32ObjNum;
					//从头遍历tObj[]中的第一个空的位置进行记录
					for (l32CntK = 1; l32CntK <= l32ObjNum; l32CntK++)
					{
						if (tObj[l32CntK].l32Frames == 0)
						{
							tObj[l32CntK].l32Frames = 2;
							tObj[l32CntK].l32StartFrame = l32Frames - 1;
							memcpy(&(tObj[l32CntK].atDetRgn[0]), &(ptPrev[l32CompResult]), sizeof(TDetRgn));
							memcpy(&(tObj[l32CntK].atDetRgn[1]), &(atCur[l32CntJ - 1]), sizeof(TDetRgn));
							atCur[l32CntJ - 1].l32Label = l32CntK;
							break;
						}
						if (l32CntK == l32ObjNum)
						{
							(*pl32ObjNum)++;
							tObj[*pl32ObjNum].l32Frames = 2;
							tObj[*pl32ObjNum].l32StartFrame = l32Frames - 1;
							memcpy(&(tObj[*pl32ObjNum].atDetRgn[0]), &(ptPrev[l32CompResult]), sizeof(TDetRgn));
							memcpy(&(tObj[*pl32ObjNum].atDetRgn[1]), &(atCur[l32CntJ - 1]), sizeof(TDetRgn));
							atCur[l32CntJ - 1].l32Label = *pl32ObjNum;
						}
					}
					//记录的目标数量达到规定数量则退出循环
					if (*pl32ObjNum >= OBJ_NUM - 1)
					{
						break;
					}
				}

				//若对应的前框体l32Lable大于0，则将当前框体数据记录在tObj[l32Lable]下
				if (ptPrev[l32CompResult].l32Label > 0)
				{
					l32ObjLable = ptPrev[l32CompResult].l32Label;
					l32ObjFrames = tObj[l32ObjLable].l32Frames;
					if (l32ObjFrames >= OBJ_TRECT)
					{
						continue;
					}
					tObj[l32ObjLable].l32Frames++;
					memcpy(&(tObj[l32ObjLable].atDetRgn[l32ObjFrames]), &(atCur[l32CntJ - 1]), sizeof(TDetRgn));
					atCur[l32CntJ - 1].l32Label = l32ObjLable;
				}
			}
		}

		//将tCur的数据覆盖至ptPrev中
		*pl32PrevNum = l32CntJ;
		memcpy(ptPrev, atCur, OBJ_TRECT * sizeof(TDetRgn));
	}

	return 0;
}

/**
* @brief 在X方向进行膨胀处理，对于前景点进行左右各2像素大小的膨胀
* @param pY : 二值图数据首地址
* @param height, width of pY.
*/
static void DilateByX(uint8_t *pY, int height, int width)
{
	int l32i = 0;
	int l32j = 0;
	int l32k = 0;
	int l32pc = height * width;
	uint8_t *pu8Temp = NULL;

	for (l32i = 0; l32i < height; l32i++)
	{
		for (l32j = 2; l32j < width - 2; l32j++)
		{
			pu8Temp = pY + l32i * width + l32j;
			if (*pu8Temp == 255)
			{
				if (*(pu8Temp + 2) != 255)
				{
					*(pu8Temp + 2) = 255;
					l32j += 2;
				}
				*(pu8Temp + 1) = 255;
				*(pu8Temp - 1) = 255;
				*(pu8Temp - 2) = 255;
			}
		}
	}
}

void CSence::resize2procYUV(uint8_t* pYuv, int width, int height)
{
	cv::Mat srcY = cv::Mat(height, width, CV_8UC1, pYuv);
	cv::Mat srcU = cv::Mat(height/2, width/2, CV_8UC1, pYuv + width * height);
	cv::Mat srcV = cv::Mat(height/2, width/2, CV_8UC1, pYuv + width * height * 5 / 4);
	cv::resize(srcY, _procY, _procY.size());
	cv::resize(srcU, _procU, _procY.size());
	cv::resize(srcV, _procV, _procY.size());
}

void CSence::putOneFrame(uint8_t *pYuv, int width, int height)
{
	if (_frmNum >= MAX_FRAMES) {
		_bFinished = true;
		return;
	}

	// Initialize param
	if (_frmNum == 0) {
		_procWidth = RESIZE_HEIGHT;
		_procHeight = RESIZE_WIDTH;
		_procY = cv::Mat(_procHeight, _procWidth, CV_8UC1);
		_procU = cv::Mat(_procHeight / 2, _procWidth / 2, CV_8UC1);
		_procV = cv::Mat(_procHeight / 2, _procWidth / 2, CV_8UC1);
	}

	/**
	* (RESIZE_HEIGHT, RESIZE_WIDTH) is processing size, firstly resize 
	* procYUV = _procY, _procU, _procV.
	*/
	resize2procYUV(pYuv, width, height);



	_frmNum++;
}

int CSence::getResult()
{
	return 0;
}