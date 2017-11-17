#ifndef _EXTRACT_OBJ_AUTO_PARAM_H_
#define _EXTRACT_OBJ_AUTO_PARAM_H_

#include <stdint.h>
#include <opencv2/opencv.hpp>

#include "extract_obj_config.h"

class CAutoParam
{
public:
	CAutoParam() { };
	~CAutoParam() { };

private:
	/* config parameter, use define or CAutoParam generate */
	ExtractObjConfig _configParam;

	bool _bIsValid;	//��ɨ����Ϣ�Ƿ���Ч
	cv::Mat _bgImg;	// background image.
	//TEdgeVibeModel tModel;  //EdgeVibe������ģModel
};

#endif /* _EXTRACT_OBJ_AUTO_PARAM_H_ */