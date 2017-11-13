/**
* Movement object extract pulbic header
* Xiping Yan, Nov 9, 2017
* Linux and Windows
*/

/**
* @brief Movement object extract algorithm, divide to 2 part;
* Part 1: Auto calculate parameter and background image;
* Part 2: Movement ojects extract;
*/

#ifndef _MOVEMENT_EXTRACT_OBJ_LIB_H_
#define _MOVEMENT_EXTRACT_OBJ_LIB_H_

#include <stdint.h>

#ifdef _WIN32
#define OBJEXT_LIB extern "C" __declspec(dllexport)
#else
#define EXPORT_LIB extern "C" 
#endif // _WIN32

typedef enum tagDataType
{
	YUV = 0,	/* YYYY, UU, VV */
	NV12 = 1,   /* YYYY, UVUV */
	BGR24 = 2,	/* BGR, BGR... */
	BGR32 = 3   /* BGRA, BGRA... */
}DataType;

typedef struct tagInput
{
	uint64_t pts;	/* time stamp */
	int width;		/* image infor */
	int height;		
	DataType dataType;
	uint8_t *pData;	/* image data ptr */
}Input;

/**
* @brief Part 1: Auto get paramter.
* Create Handle of auto get paramter.
* @param width : image width.
* @param height : image height.
* @return handle, use 'ccAutoParamClose' to release.
*/
OBJEXT_LIB void* ccAutoParamOpen(int width, int height);

/**
* @brief Auto get parameter, process one frame.
* @param pvAPHandle : 'ccAutoParamOpen' create it.
* @param Input : Input one frame image.
* @return state, 0=error; 1=contine; 2=over(finish);
*/
OBJEXT_LIB int ccAutoParamProcess(void* pvAPHandle, Input tOneFrame);

OBJEXT_LIB void ccAutoParamClose(void** ppvAPHandle);
// 内部会自动开辟内存，需要使用ccReleaseAutoParam释放
OBJEXT_LIB void* ccGetAutoParam(void* pvAPHandle);
OBJEXT_LIB void ccReleaseAutoParam(void** ppvAParam);


OBJEXT_LIB void* ccObjExtractOpen(int imgW, int imgH, void* pvAParam/*自动获取的参数*/);
OBJEXT_LIB int ccObjExtractProcess(void* pvHandle, TNewVF *ptIn, TObjExtractOut *ptOut);
OBJEXT_LIB int ccObjExtractGetRemain(void* pvHandle, TObjExtractOut *ptOut);
OBJEXT_LIB void ccObjExtractClose(void** ppvHandle);


#endif /* _MOVEMENT_EXTRACT_OBJ_LIB_H_ */
