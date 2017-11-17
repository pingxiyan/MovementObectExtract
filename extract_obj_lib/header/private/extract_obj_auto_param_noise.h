#ifndef _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_
#define _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>
#include "extract_obj_log.h"

class CNoise
{
public:
	CNoise() {};
	~CNoise() { releaseBuf(); };

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

	int _neighbor[8]; // 3*3 window, coordinate offset relative centor point.
	void initialNeighbor(int width, int height);

	/**
	* @brief gray = cv::Size(width/2,height/2);
	* pYuv neighbor 4 pixel mean = gray;
	* @param pYuv : yuv420 image.
	* @param gray : out.
	*/
	void neighbor4PiexlDiff(uint8_t *pYuv, int width, int height, cv::Mat &gray); 
	int _quarterImgSize = 0;

	cv::Mat _curGrayImg;	// src resize to cv::Size(w/2, h/2);
	cv::Mat _preGrayImg;	// before _curGrayImg,
	cv::Mat _diffGrayImg;	// _curGrayImg - _preGrayImg;

	int *_pl32NoiseFreq = NULL;			//前景点出现频率统计
	int *_pl32NoiseIntensity = NULL;	//统计噪声的强度
	int *_pl32Intensity = NULL;			//统计去除噪声后强度
	void createBuf(int width, int height) {
		releaseBuf();
#define NEW_INT_BUF(x) {x = new int[width * height]; \
		if (NULL == x)	{PERR("New int error"); exit(-1);}}

		NEW_INT_BUF(_pl32NoiseFreq);
		NEW_INT_BUF(_pl32NoiseIntensity);
		NEW_INT_BUF(_pl32Intensity);
	};
	void releaseBuf() {
		if (_pl32NoiseFreq) { delete[] _pl32NoiseFreq; _pl32NoiseFreq = NULL; }
		if (_pl32NoiseIntensity) { delete[] _pl32NoiseIntensity; _pl32NoiseIntensity = NULL;}
		if (_pl32Intensity) { delete[] _pl32Intensity; _pl32Intensity = NULL; }
	};
};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_NOISE_H_ */