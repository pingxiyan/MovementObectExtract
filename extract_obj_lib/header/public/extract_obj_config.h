/**
* Movement object extract pulbic config parameter
* Xiping Yan, Nov 9, 2017
* Linux and Windows
*/

#ifndef _EXTRACT_OBJ_CONFIG_H_
#define _EXTRACT_OBJ_CONFIG_H_

#include <stdint.h>

/**
* Rectangle struct
*/
typedef struct tagRect
{
	int x;
	int y;
	int w;
	int h;
}RoiRect;

typedef struct tagExtractObjConfig
{
	bool bIsDay;	// day or night
	bool bIsColor;	// color or gray

	int32_t noisyGrade;		// noisy grade [1,2,3,4,5]
	int32_t contrastGrade;	// contrast grade [1,2,3,4,5]
	
	int32_t processWidth;  // advise process size
	int32_t processHeight;
	int32_t vanishingPoint;  // 根据建议缩放后，消隐点距离图像顶部距离（向上为正）
	int32_t objMinWidth;     // Minimum object size
	int32_t objMinHeight;    
	
	RoiRect *pTextRect;		// Logo or caption, words regions
	int32_t textRtNum;

	int64_t validStartFrame; //此帧后面开始出现目标，在此之前可以跳过检测
}ExtractObjConfig;


#endif /* _EXTRACT_OBJ_CONFIG_H_ */
