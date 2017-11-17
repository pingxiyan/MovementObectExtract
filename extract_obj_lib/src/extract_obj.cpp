/**
* Part 2: Movement objects extract
* Sandy Yann, Nov 14, 2017
*/

#include <string>
#include <iostream>

#include "extract_obj.h"
#include "extract_obj_log.h"

/**
* @brief Initialize log model
*/
const bool bInitLog = InitialLog("extract_obj_lib", false, false, false);

/**
* @brief Create object extract handle.
* @param width, height : same to 'width height of ccAutoParamOpen'
* @param pvAParam : parameter, be created by 'ccGetAutoParam',
* and you can release after "ccObjExtractOpen".
* @return void* : handle, ccObjExtractClose to release.
*/
OBJEXT_LIB void* ccObjExtractOpen(int width, int height, void* pvAParam)
{
	return NULL;
}

/**
* @brief Process one frame. Inside we only process yuv420 image, so we
* convert input to yuv420 firtly.
* @param pvHandle : created by 'ccObjExtractOpen'
* @param ptIn : one frame image information.
* @param ptOut : report result.
* @return int : 0=;1=;2=;
*/
OBJEXT_LIB int ccObjExtractProcess(void* pvHandle, const Input *pIn, Output *pOut)
{
	return 0;
}

/**
* @brief Process one frame.
* @param pvHandle : created by 'ccObjExtractOpen'
* @param ptOut : Get last remaining report result after processing all frames.
* @return int : 0=;1=;
*/
OBJEXT_LIB int ccObjExtractGetRemain(void* pvHandle, Output *ptOut)
{
	return 0;
}

/**
* @brief Release handle and all middle buffers.
* @param ppvHandle : created by 'ccObjExtractOpen'
*/
OBJEXT_LIB void ccObjExtractClose(void** ppvHandle)
{
	
}
