/**
* Common functions for sample.
* Sandy Yann, Nov 14, 2017
*/

#ifndef _SAMPLE_COMMON_H_
#define _SAMPLE_COMMON_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>

#include <extract_obj.h>

void bgr24_to_yuv(uint8_t *pYUV, uint8_t *pBGR24, int width, int height, int step);

cv::Rect RoiRect2Rect(RoiRect rt);

#endif /* _SAMPLE_COMMON_H_ */