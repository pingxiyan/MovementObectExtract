#ifndef _EXTRACT_OBJ_AUTO_PARAM_SENCECHANGE_H_
#define _EXTRACT_OBJ_AUTO_PARAM_SENCECHANGE_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>
#include "extract_obj_log.h"

class CSenceChange
{
public:
	CSenceChange() {};
	~CSenceChange() { };

	/**
	* @brief Inside, this module will calculate it each n frames automatically
	* @param img : bgr image.
	*/
	void putOneFrame(uint8_t *pYuv, int width, int height);

	/**
	* @brief Get statistical result.
	* @return noise level
	*/
	int getResult();

private:
	int _frmNum = 0;	// receives frames number
	int _frmNumProcessed = 0;	// had processed frames number
	int _frmNumNoise = 0;		// In all estimate frames, is color frames numbers. <=_frmNum;

};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_SENCECHANGE_H_ */