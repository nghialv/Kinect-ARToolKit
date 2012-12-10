#ifndef _UTIL_H_
#define _UTIL_H_

#include "Common.h"

const float CONFIDENCE_THRESHOLD = 0.5f;
#ifndef USE_MACRO
inline bool isConfident(XnSkeletonJointPosition jointPos) {
        return jointPos.fConfidence >= CONFIDENCE_THRESHOLD;
}

inline bool isConfident(XnSkeletonJointOrientation jointOrientation)
{
        return jointOrientation.fConfidence >= CONFIDENCE_THRESHOLD;
}
#else
#define isConfident(jointPos) ((jointPos).fConfidence >= CONFIDENCE_THRESHOLD)
#endif

#ifndef USE_MACRO
inline bool i2b(int i) { return !!i; }
#else
#define i2b(i) (!!(i))
#endif

#ifndef USE_MACRO
inline float b2fNormalized(unsigned char b) { return b/255.0f; }
#else
#define b2fNormalized(b) ((b)/255.0f)
#endif

#endif
