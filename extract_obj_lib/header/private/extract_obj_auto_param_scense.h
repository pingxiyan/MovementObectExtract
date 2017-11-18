#ifndef _EXTRACT_OBJ_AUTO_PARAM_SENCE_H_
#define _EXTRACT_OBJ_AUTO_PARAM_SENCE_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>
#include "extract_obj_log.h"

class CSence
{
public:
	CSence() {};
	~CSence() {};

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
	bool _bFinished = false;

	int _procWidth = 0;
	int _procHeight = 0;
	cv::Mat _procY;
	cv::Mat _procU;
	cv::Mat _procV;
	void resize2procYUV(uint8_t* pYuv, int width, int height);
};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_SENCE_H_ */