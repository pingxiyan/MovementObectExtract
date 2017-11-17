/**
* Part 1: Auto get parameter.
* Sandy Yann, Nov 14, 2017
*/

#include <string>
#include <iostream>

#include "extract_obj.h"
#include "extract_obj_auto_param.h"

/**
* @brief Create Handle of auto get parameter.
* @param width : image width.
* @param height : image height.
* @param maxframenum : max frame number is used for initial parameter.
* default 300, recommend 300~600.
* @return handle, use 'ccAutoParamClose' to release.
*/
OBJEXT_LIB void* ccAutoParamOpen(int width, int height, int maxframenum)
{
	CAutoParam* pAutoParam = new CAutoParam();
	if (NULL == pAutoParam) {
		return NULL;
	}

	return NULL;
}

/**
* @brief Auto get parameter, process one frame.
* @param pvAPHandle : 'ccAutoParamOpen' create it.
* @param Input : Input one frame image.
* @return state, 0=error; 1=continue; 2=over(finish);
*/
OBJEXT_LIB int ccAutoParamProcess(void* pvAPHandle, const Input* ptOneFrame)
{
	return 1;
}

/**
* @brief Release handle
* @param ppvAPHandle : created by 'ccAutoParamOpen'
*/
OBJEXT_LIB void ccAutoParamClose(void** ppvAPHandle)
{
	return;
}

/**
* @brief When 'ccAutoParamProcess' return 2, means that auto get parameter finish,
* and then, we can get out parameter through 'ccGetAutoParam'
*/
OBJEXT_LIB void* ccGetAutoParam(void* pvAPHandle)
{
	return NULL;
}

/**
* @brief Release handle(returned by ccGetAutoParam), must be after 'ccObjExtractOpen"
*/
OBJEXT_LIB void ccReleaseAutoParam(void** ppvAParam)
{

}
