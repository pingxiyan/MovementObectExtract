/**
* Auto get config parameter part.
* Auto estimate video is color or gray?
*/

#ifndef _EXTRACT_OBJ_AUTO_PARAM_COLOR_H_
#define _EXTRACT_OBJ_AUTO_PARAM_COLOR_H_

#include <stdint.h>
#include <opencv/cxcore.hpp>

class CAutoParamColor
{
public:
	CAutoParamColor() {};
	~CAutoParamColor() {};

	/**
	* @brief Inside, this module will calculate it each n frames automatically 
	* @param pYuv : yuv420 image
	*/
	void putOneFrame(const uint8_t* pYuv, int width, int height);

	/**
	* @brief Judge this video is color ?
	* @return true = color, false = gray.
	*/
	bool getResult();

private:
	int _frmNum = 0;	// receives frames number
	int _frmNumProcessed = 0;	// had processed frames number
	int _frmNumColor = 0;		// In all estimate frames, is color frames numbers. <=_frmNum;
};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_COLOR_H_ */