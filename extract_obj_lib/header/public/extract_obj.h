/**
* Movement object extract pulbic header
* Xiping Yan, Nov 9, 2017
* Linux and Windows
*/

/**
* @brief Movement object extract algorithm, divide to 2 part;
* Part 1: Auto calculate parameter and background image;
* Part 2: Movement objects extract;
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

typedef struct tagOutput
{
	uint64_t pts;
}Output;


/***************************************************************/
/*** Part 1: Auto get parameter. *******************************/
/***************************************************************/

/**
* @brief Create Handle of auto get parameter.
* @param width : image width.
* @param height : image height.
* @return handle, use 'ccAutoParamClose' to release.
*/
OBJEXT_LIB void* ccAutoParamOpen(int width, int height);

/**
* @brief Auto get parameter, process one frame.
* @param pvAPHandle : 'ccAutoParamOpen' create it.
* @param Input : Input one frame image.
* @return state, 0=error; 1=continue; 2=over(finish);
*/
OBJEXT_LIB int ccAutoParamProcess(void* pvAPHandle, const Input* ptOneFrame);

/**
 * @brief Release handle
 * @param ppvAPHandle : created by 'ccAutoParamOpen'
 */
OBJEXT_LIB void ccAutoParamClose(void** ppvAPHandle);

/**
 * @brief When 'ccAutoParamProcess' return 2, means that auto get parameter finish,
 * and then, we can get out parameter through 'ccGetAutoParam'
 */
OBJEXT_LIB void* ccGetAutoParam(void* pvAPHandle);

/**
 * @brief Release handle(returned by ccGetAutoParam), must be after 'ccObjExtractOpen"
 */
OBJEXT_LIB void ccReleaseAutoParam(void** ppvAParam);


/***************************************************************/
/*** Part 2: Movement objects extract **************************/
/***************************************************************/

/**
 * @brief Create object extract handle.
 * @param width, height : same to 'width height of ccAutoParamOpen'
 * @param pvAParam : parameter, be created by 'ccGetAutoParam',
 * and you can release after "ccObjExtractOpen".
 * @return void* : handle, ccObjExtractClose to release.
 */
OBJEXT_LIB void* ccObjExtractOpen(int width, int height, void* pvAParam);

/**
 * @brief Process one frame.
 * @param pvHandle : created by 'ccObjExtractOpen'
 * @param ptIn : one frame image information.
 * @param ptOut : report result.
 * @return int : 0=;1=;2=;
 */
OBJEXT_LIB int ccObjExtractProcess(void* pvHandle, const Input *ptIn, Output *ptOut);

/**
 * @brief Process one frame.
 * @param pvHandle : created by 'ccObjExtractOpen'
 * @param ptOut : Get last remaining report result after processing all frames.
 * @return int : 0=;1=;
 */
OBJEXT_LIB int ccObjExtractGetRemain(void* pvHandle, Output *ptOut);

/**
 * @brief Release handle and all middle buffers.
 * @param ppvHandle : created by 'ccObjExtractOpen'
 */
OBJEXT_LIB void ccObjExtractClose(void** ppvHandle);

/**
 * samples
 */

#endif /* _MOVEMENT_EXTRACT_OBJ_LIB_H_ */
