/**
* Common function: digital image process
*/

#ifndef _EXTRACT_OBJ_PRIVATE_H_
#define _EXTRACT_OBJ_PRIVATE_H_

#include <stdint.h>

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

#ifndef RANGE_UCHAR
#define RANGE_UCHAR(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))
#endif /* RANGE_UCHAR */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

typedef struct tagYUV420
{
	uint8_t *pY;
	uint8_t *pU;
	uint8_t *pV;
	int l32Width;
	int l32Height;

	void Init(uint8_t *pBufYuv, int width, int height, bool bGray = false)
	{
		l32Height = height;
		l32Width = width;
		pY = pBufYuv;
		if (bGray)
		{
			pU = NULL;
			pV = NULL;
		}
		else
		{
			pU = pBufYuv + height * width;
			pV = pU + (height >> 1) * (width >> 1);
		}
	}
}TYUV420;

#endif /* _EXTRACT_OBJ_PRIVATE_H_ */