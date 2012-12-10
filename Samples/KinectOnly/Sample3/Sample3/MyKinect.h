#ifndef _MYKINECT_H_
#define _MYKINECT_H_

#include "Common.h"

#ifdef USE_USERDETECTOR
	#include "UserDetector.h"
	#include "PlayerStatus.h"
#endif

class MyKinect
{
private:
	unsigned char	imageBGRA32Buf[640*480*4];			//BGRA
	unsigned char	depthDrewByColorBuf[640*480*4];		//BGRA
	unsigned char	depthMixedImageBuf[640*480*4];		//BGRA
	float			depthHist[MAX_DEPTH];

public:
	//OpenNI
	Context			context;

#ifdef USE_USERDETECTOR
	UserGenerator	userGen;
	PlayerStatus	userStatus;
	UserDetector*	userDetector;
#endif

	DepthGenerator	depthGen;
	ImageGenerator	imageGen;
	DepthMetaData	depthMD;
	ImageMetaData	imageMD;

public:
	MyKinect(){}
	~MyKinect(){}

	int Init();
	void StartGeneratingAll();
	void Update();
	void Exit();

	unsigned char * GetBGRA32Image();
	unsigned char * GetDepthDrewByColor();
	unsigned char * GetDepthMixedImage();
	void GetImageSize(int *xSize, int *ySize);
};

#endif