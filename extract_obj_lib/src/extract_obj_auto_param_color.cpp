#include "extract_obj_auto_param_color.h"
#include "extract_obj_private.h"

#define SKIP_FRAME_NUM 10
#define MAX_ESTIMATE_FRAME_NUM (10*SKIP_FRAME_NUM)

#define UV_DIFF 4				// UV and 128 difference threshold
#define COLOR_PIX_RATIO 3		// One image color pixel percent threshold
#define COLOR_FRAME_RATIO 10	// Color images percent threshold


/*Inside, this module will calculate it each n frames automatically */
void CAutoParamColor::putOneFrame(const uint8_t* pYuv, int width, int height)
{
	/**
	* Each 10 frame detect once, max detect 10 frames.
	*/
	if (_frmNum % SKIP_FRAME_NUM == 0 && _frmNum < MAX_ESTIMATE_FRAME_NUM) {
		int ySize = width * height;
		int uvSize = ySize >> 2;
		uint8_t* pU = (uint8_t *)(pYuv + ySize);
		uint8_t* pV = (uint8_t *)(pYuv + ySize + uvSize); 

		int colorPixelNum = 0;

		/**
		* The difference between U,V and 120 is used as judge 
		* if this pixel is a color point or not.
		*/
		for (int i = 0; i < uvSize; i++, pU++, pV++)
		{
			if (ABS(*pU - 128) > UV_DIFF || ABS(*pV - 128) > UV_DIFF)
			{
				colorPixelNum++;	// Color pixel numbers.
			}
		}

		/**
		* If color pixels number > threshold, as a color image.
		*/
		if (100 * colorPixelNum > COLOR_PIX_RATIO * uvSize)
		{
			_frmNumColor++;	// color image numbers
		}

		_frmNumProcessed++;	// had processed image numbers.
	}
}

bool CAutoParamColor::getResult()
{
	/**
	* Is color video ?
	*/
	return (100 * _frmNumColor > COLOR_FRAME_RATIO * _frmNumProcessed) ? true : false;
}