#ifndef _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_
#define _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>
#include "extract_obj_log.h"

class CBackgroundModel
{
public:
	CBackgroundModel() {};
	~CBackgroundModel() { };

	virtual bool putOneFrame() = 0;
	virtual cv::Mat getBackgroundImg() = 0;
	virtual cv::Mat getMask() = 0;

};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_ */