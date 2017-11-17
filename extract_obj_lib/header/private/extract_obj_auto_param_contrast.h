#ifndef _EXTRACT_OBJ_AUTO_PARAM_CONTRAST_H_
#define _EXTRACT_OBJ_AUTO_PARAM_CONTRAST_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>

#define PS_ROWS_RATIO 8              //每帧处理的行数与图像高度的比例（1/PS_ROWS_RATIO）

class CContrast
{
public:
	CContrast() {};
	~CContrast() {};

	/**
	* @brief Inside, this module will calculate it each n frames automatically 
	* @param img : bgr image.
	*/
	void putOneFrame(uint8_t *pYuv, int width, int height);

	/**
	* @brief Get statistical result.
	* @return noise level[0~100]
	*/
	int getResult();

private:
	int _frmNum = 0;	// receives frames number
	bool _bFinished = false;
	int _widht = 0;

	cv::Mat _curGray;
	cv::Mat _filterImg;
	
	int _startRow = 0;
	int _definition[PS_ROWS_RATIO];		//分块清晰度
	int *_pDefinition = _definition;	//指向分块清晰度数组
};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_CONTRAST_H_ */