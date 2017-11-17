#include "common.h"
#include "extract_obj.h"

#include <opencv2/opencv.hpp>


#define RANGE_UCHAR(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))

/**
* @brief Color space convert: BGR24_to_YUV
* @param pNV12: src YUV, also called I420
* @param pBGR24: dst BGR24
* @param width: image width
* @param height: image height
* @param ystep:
*/
void bgr24_to_yuv(uint8_t *pYUV, uint8_t *pBGR24, int width, int height, int step)
{
	int w, h, sizeY, sizeOff;
	uint8_t *pY, *pU, *pV, *pTmpBufBGR;
	int halfW = width >> 1, halfH = height >> 1;

	// Y
	for (h = height - 1; h >= 0; h--) {
		pY = pYUV + h * width;
		pTmpBufBGR = pBGR24 + h * step;

		for (w = 0; w < width; w++) {
			pY[0] = RANGE_UCHAR(((66 * pTmpBufBGR[2] + 129 * pTmpBufBGR[1] + 25 * pTmpBufBGR[0] + 128) >> 8) + 16);
			pY++;
			pTmpBufBGR += 3;
		}
	}

	// u
	sizeY = width * height;
	for (h = 0; h < halfH; h++) {
		pU = pYUV + h * halfW + sizeY;
		pTmpBufBGR = pBGR24 + h * 2 * step + step;

		for (w = 0; w < halfW; w++) {
			pU[0] = RANGE_UCHAR(((-38 * pTmpBufBGR[2] - 74 * pTmpBufBGR[1] + 112 * pTmpBufBGR[0] + 128) >> 8) + 128);
			pU++;
			pTmpBufBGR += 6;
		}
	}

	// v
	sizeOff = sizeY + (sizeY >> 2);
	for (h = 0; h < halfH; h++) {
		pV = pYUV + h * halfW + sizeOff;
		pTmpBufBGR = pBGR24 + h * 2 * step + step + 3;

		for (w = 0; w < halfW; w++) {
			pV[0] = RANGE_UCHAR(((112 * pTmpBufBGR[2] - 94 * pTmpBufBGR[1] - 18 * pTmpBufBGR[0] + 128) >> 8) + 128);
			pV++;
			pTmpBufBGR += 6;
		}
	}
}

cv::Rect RoiRect2Rect(RoiRect rt)
{
	return cv::Rect(rt.x, rt.y, rt.w, rt.h);
}