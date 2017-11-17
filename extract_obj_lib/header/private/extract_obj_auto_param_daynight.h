/**
* Auto get config parameter part.
* Auto estimate video is day or night?
*/

#ifndef _EXTRACT_OBJ_AUTO_PARAM_DAYNIGHT_H_
#define _EXTRACT_OBJ_AUTO_PARAM_DAYNIGHT_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>

class CDayNight
{
public:
	CDayNight() {};
	~CDayNight() {};

	/**
	* @brief Inside, this module will calculate it each n frames automatically 
	* @param img : bgr image.
	*/
	void putOneFrame(uint8_t *pYuv, int width, int height);

	/**
	* @brief Judge this video is color ?
	* @return true = color, false = gray.
	*/
	bool getResult();

private:
	bool processOneFrame(uint8_t *pY, int width, int height, uint8_t *pU, uint8_t *pV);

	int _frmNum = 0;	// receives frames number
	int _frmNumProcessed = 0;	// had processed frames number
	int _frmNumDay = 0;		// In all estimate frames, is color frames numbers. <=_frmNum;
};


#endif /* _EXTRACT_OBJ_AUTO_PARAM_DAYNIGHT_H_ */