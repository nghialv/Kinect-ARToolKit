#ifndef _COMMON_H_
#define _COMMON_H_

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>

using namespace xn;

//#define USE_USERDETECTOR
//#define USE_MIRROR

#define MAX_DEPTH 10000
#define SAMPLE_XML_PATH "Data/SamplesConfig.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return nRetVal;												\
	}


#endif