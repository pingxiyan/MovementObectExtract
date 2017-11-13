/**
*
*/

#include "extract_obj.h"

#include <string>
#include <iostream>

extern "C" __declspec(dllexport) void extractObj(const char* fn)
{
	std::cout << "this is a test function " << std::endl;

}