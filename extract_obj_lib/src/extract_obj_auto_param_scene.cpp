#include "extract_obj_private.h"
#include "extract_obj_auto_param_scense.h"

#include <opencv2/opencv.hpp>
#include <array>
#include <algorithm>
#include <vector>

using std::vector;

#define ERR_VANISHING_POINT 9999             //�޷��������ʱ�����������ֵ
#define MAX_RATIO_THRESH 2                   //��������߱�
#define MIN_RATIO_THRESH 0.3                 //������С��߱�
#define MIN_OBJ_FRAMES 20                    //Ŀ����ٵ�����֡��
#define MIN_WHOLE_OBJ_FRAMES 10              //Ŀ�걣������״̬�����Ӵ�4���߽磩������֡��
#define MAX_FRAMES 250
#define OBJ_TRECT 200                        //ÿ��Ŀ������¼��������
#define OBJ_NUM 300                          //���ٵ�Ŀ����������
#define EDGE_MODIFY 1 / 50                   //�����Ե������ֵ
#define DISTANCE_THRESH 1 / 10               //Ŀ��λ�Ʊ�����ֵ����Ի���ߴ磩
#define RATIO_THRESH 0.2                     //Ŀ������֡�仯��ֵ
#define SIZE_CHANGE_THRESH1 1 / 30           //���ٳ�ʼ�����ʱĿ���С��Ϊ�����Ա仯�ı�����ֵ����Ի���ߴ磩
#define SIZE_CHANGE_THRESH2 1 / 3            //���ٳ�ʼ�����ʱĿ���С��Ϊ�����Ա仯�ı�����ֵ�����Ŀ��ߴ磩
#define MIN_SUGGESTED_WIDTH 192              //�������С����������
#define MAX_SUGGESTED_WIDTH 416              //����������������
#define RESIZE_HEIGHT 216                    //����ʱ���Ÿ߶�
#define RESIZE_WIDTH 384                     //����ʱ���ſ��
#define MIN_TRACE_HEIGHT 15                  //���ٵ���С�߶�Ĭ��ֵ
#define MIN_TRACE_WIDTH 8                    //���ٵ���С���Ĭ��ֵ

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
* @return �뵱ǰ������ƥ���ǰһ֡��������; -1=�޿���ƥ��
*/
static int CompareRect(TDetRgn *ptPrevRgn, TDetRgn *ptCurRgn, int l32PrevNum)
{
	int l32i = 0;                        //ѭ������
	int l32OutNum = 0;                   //������
	int l32CrossedCnt = 0;               //��������ص��ļ���
	int l32DistX = 0;                    //X�������
	int l32DistY = 0;                    //Y�������
	int l32PrevHeight = 0;               //ǰĿ����
	int l32PrevWidth = 0;                //ǰĿ����
	int l32CurHeight = 0;                //��ǰĿ����
	int l32CurWidth = 0;                 //��ǰĿ����
	float f32HeightRatio = 0;              //ǰ���߶ȱ�
	float f32WidthRatio = 0;               //ǰ����ȱ�
	for (l32i = 0; l32i < l32PrevNum; l32i++)
	{
		l32DistX = MIN(ptPrevRgn[l32i].tRect.x + ptPrevRgn[l32i].tRect.width, ptCurRgn->tRect.x + ptCurRgn->tRect.width) - MAX(ptPrevRgn[l32i].tRect.x, ptCurRgn->tRect.x);
		l32DistY = MIN(ptPrevRgn[l32i].tRect.y + ptPrevRgn[l32i].tRect.height, ptCurRgn->tRect.y + ptCurRgn->tRect.height) - MAX(ptPrevRgn[l32i].tRect.y, ptCurRgn->tRect.y);

		//l32DistX��l32DistY������0��ʾ�������ص�
		if (l32DistX > 0 && l32DistY > 0)
		{
			//ǰĿ����������ڶ���ص������Ŀ�����Ϊ��Ч
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

			//ǰ���ص�����ĳ���仯Ӧ��һ����Χ��
			if (abs(l32CurHeight - l32PrevHeight) < RATIO_THRESH * l32PrevHeight
				&& abs(l32CurWidth - l32PrevWidth) < MAX(5, RATIO_THRESH * l32PrevWidth))
			{
				l32OutNum = l32i;
				l32CrossedCnt++;
				//�ص���������1����Ŀ�������Ч
				if (l32CrossedCnt > 1)
				{
					ptCurRgn->l32Label = -1;
					return -1;
				}
			}
		}
	}

	//ǰ�����Ψһ��Ӧʱ�����ǰ�����ӦptPrevRgn���±�
	if (l32CrossedCnt == 1)
	{
		return l32OutNum;
	}
	return -1;
}

/**
* @brief ����Ŀ���ȡ��������
* @param vtRgn			    ��ǰ֡ǰ����������
* @param tObj               Ŀ���¼�ṹ��
* @param l32Frames          �Ѷ�ȡ֡��
* @param ptPrev             ǰһ֡��¼�Ŀ�������
* @param pl32PrevNum        ǰһ֡��¼������
* @param pl32ObjNum         �Ѽ�¼Ŀ����
* @return �ɹ�--0, ʧ��--������
*/
static int TraceObj(vector<TDetRgn> &vtRgn, TPreScanObj *tObj, int l32Frames,
	TDetRgn *ptPrev, int *pl32PrevNum, int *pl32ObjNum)
{
	TDetRgn atCur[OBJ_TRECT];                         //��ǰ֡��Ŀ�����
	int l32CompResult = 0;                                 //CompareRect�����Ľ��
	int l32ObjLable = 0;                                   //Obj���
	int l32ObjFrames = 0;                                  //����Obj�Ѽ�¼֡��
	int l32CntI = 0;                                       //ѭ������
	int l32CntJ = 0;                                       //ѭ������
	int l32CntK = 0;                                       //ѭ������
	int l32ObjNum = 0;                                     //��¼��Ŀ������
	float f32WHRatio = 0;                                    //��߱�

														   //��¼��Ŀ�������ﵽ�涨��������ǰ�˳�
	if (*pl32ObjNum >= OBJ_NUM - 1)
	{
		return 1;
	}

	//��һ֡���������Ŀ����¼��ptPrev[]�ṹ�������У���֡Ŀ������¼��������FRAME_TRECT_NUM
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
		*pl32PrevNum = l32CntJ;//ptPrev[]�ṹ������Ĵ洢����
	}

	if (l32Frames > 1)
	{
		//tObj[]���Ѹ�����ϵ���¼֡������MIN_OBJ_FRAMES���ڴ����������
		for (l32CntI = 1; l32CntI <= *pl32ObjNum; l32CntI++)
		{
			if (l32Frames - tObj[l32CntI].l32StartFrame > tObj[l32CntI].l32Frames && tObj[l32CntI].l32Frames < MIN_OBJ_FRAMES)
			{
				memset(&tObj[l32CntI], 0, sizeof(TPreScanObj));
			}
		}

		for (l32CntI = 0, l32CntJ = 0; l32CntI < vtRgn.size() && l32CntJ < OBJ_TRECT; l32CntI++)
		{
			//�ڶ�֡��ʼ���������Ŀ����¼��tCur[]�ṹ��������
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

			//������Ч��tCur[]���ٺ�ptPrev�е����п�����бȽϣ��õ�Ψһ��Ӧ����֡�еĿ��壬�±�Ϊl32CompResult
			l32CompResult = CompareRect(ptPrev, &atCur[l32CntJ - 1], *pl32PrevNum);
			if (l32CompResult > -1)
			{
				//����Ӧ��ǰ����l32LableΪ0��˵������������Ϊ��Ŀ���¼��ǰ�������壬��Ӧ��¼����
				if (ptPrev[l32CompResult].l32Label == 0)
				{
					l32ObjNum = *pl32ObjNum;
					//��ͷ����tObj[]�еĵ�һ���յ�λ�ý��м�¼
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
					//��¼��Ŀ�������ﵽ�涨�������˳�ѭ��
					if (*pl32ObjNum >= OBJ_NUM - 1)
					{
						break;
					}
				}

				//����Ӧ��ǰ����l32Lable����0���򽫵�ǰ�������ݼ�¼��tObj[l32Lable]��
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

		//��tCur�����ݸ�����ptPrev��
		*pl32PrevNum = l32CntJ;
		memcpy(ptPrev, atCur, OBJ_TRECT * sizeof(TDetRgn));
	}

	return 0;
}

/**
* @brief ��X����������ʹ�������ǰ����������Ҹ�2���ش�С������
* @param pY : ��ֵͼ�����׵�ַ
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