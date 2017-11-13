/**
* Movement object extract pulbic header
* Xiping Yan, Nov 9, 2017
* Linux and Windows
*/

#ifndef _MOVEMENT_EXTRACT_OBJ_LIB_H_
#define _MOVEMENT_EXTRACT_OBJ_LIB_H_

#include <stdint.h>

#ifdef _WIN32
#define EXPORT_LIB extern "C" __declspec(dllexport)
#else
#define EXPORT_LIB extern "C" 
#endif // _WIN32


EXPORT_LIB void extractObj(const char* fn);


#endif /* _MOVEMENT_EXTRACT_OBJ_LIB_H_ */
