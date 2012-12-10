#include "MyKinect.h"

//-----------------------------------------------------------------------------------------------------
//init
//-----------------------------------------------------------------------------------------------------
int MyKinect::Init()
{
	XnStatus nRetVal = XN_STATUS_OK;
	xn::EnumerationErrors errors;
	nRetVal = context.InitFromXmlFile(SAMPLE_XML_PATH, &errors);
	if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
	{
		XnChar strError[1024];
		errors.ToString(strError, 1024);
		printf("%s\n", strError);
		return nRetVal;
	}
	else if (nRetVal != XN_STATUS_OK)
	{
		printf("Open failed: %s\n", xnGetStatusString(nRetVal));
		return nRetVal;
	}

	//config depth generator
	nRetVal = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depthGen);
	CHECK_RC(nRetVal, "Find depth generator");
	
	//config image generator
	nRetVal = context.FindExistingNode(XN_NODE_TYPE_IMAGE, imageGen);
	CHECK_RC(nRetVal, "Find image generator");

	depthGen.GetMetaData(depthMD);
	imageGen.GetMetaData(imageMD);

	 // Hybrid mode isn't supported in this sample
	if (imageMD.FullXRes() != depthMD.FullXRes() || imageMD.FullYRes() != depthMD.FullYRes())
	{
		printf ("The device depth and image resolution must be equal!\n");
		return -1;
	}

	// RGB is the only image format supported.
	if (imageMD.PixelFormat() != XN_PIXEL_FORMAT_RGB24)
	{
		printf("The device image format must be RGB24\n");
		return -1;
	}

#ifdef USE_USERDETECTOR
	//config user generator
	nRetVal = context.FindExistingNode(XN_NODE_TYPE_USER, userGen);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = userGen.Create(context);
		CHECK_RC(nRetVal, "Find user generator");
	}

	//init userDetector
	userDetector = new UserDetector(&userGen, &userStatus);
#endif

	depthGen.GetAlternativeViewPointCap().SetViewPoint(imageGen);

#ifndef USE_MIRROR
	context.SetGlobalMirror(FALSE);
#endif

	//context.StartGeneratingAll();
	return 1;
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void MyKinect::StartGeneratingAll()
{
	context.StartGeneratingAll();
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void MyKinect::Update()
{
	context.WaitAnyUpdateAll();
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void MyKinect::Exit()
{
#ifdef USE_USERDETECTOR
	delete userDetector;
#endif
	context.Shutdown();
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
unsigned char * MyKinect::GetBGRA32Image()
{	
	depthGen.GetMetaData(depthMD);
	imageGen.GetMetaData(imageMD);

	const XnDepthPixel* pDepth = depthMD.Data();
	const XnUInt8* pImage = imageMD.Data();

	const XnRGB24Pixel* pImageRow = imageMD.RGB24Data();
	unsigned char* pBuffRow = imageBGRA32Buf + imageMD.YOffset();

	for (XnUInt y = 0; y < imageMD.YRes(); ++y)
	{
		const XnRGB24Pixel* pImage = pImageRow;
		unsigned char* pBuff = pBuffRow + imageMD.XOffset();

		for (XnUInt x = 0; x < imageMD.XRes(); ++x, ++pImage)
		{
		  pBuff[0] = pImage->nBlue;
		  pBuff[1] = pImage->nGreen;
		  pBuff[2] = pImage->nRed;
		  pBuff[3] = 0;
		  pBuff +=4;
		}
		pImageRow += imageMD.XRes();
		pBuffRow += imageMD.XRes()*4;
	}

	return imageBGRA32Buf;
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
unsigned char * MyKinect::GetDepthDrewByColor()
{
	depthGen.GetMetaData(depthMD);
	const XnDepthPixel* pDepth = depthMD.Data();
	
	// Calculate the accumulative histogram (the yellow display...)
	xnOSMemSet(depthHist, 0, MAX_DEPTH*sizeof(float));

	unsigned int nNumberOfPoints = 0;
	for (XnUInt y = 0; y < depthMD.YRes(); ++y)
	{
		for (XnUInt x = 0; x < depthMD.XRes(); ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				depthHist[*pDepth]++;
				nNumberOfPoints++;
			}
		}
	}

	for (int nIndex=1; nIndex< MAX_DEPTH; nIndex++)
	{
		depthHist[nIndex] += depthHist[nIndex-1];
	}
	
	if (nNumberOfPoints)
	{
		for (int nIndex=1; nIndex< MAX_DEPTH; nIndex++)
		{
			depthHist[nIndex] = (unsigned int)(256 * (1.0f - (depthHist[nIndex] / nNumberOfPoints)));
		}
	}

	xnOSMemSet(depthDrewByColorBuf, 0, sizeof(depthDrewByColorBuf));

	const XnDepthPixel* pDepthRow = depthMD.Data();
	unsigned char* pBuffRow = depthDrewByColorBuf + depthMD.YOffset();

	for (XnUInt y = 0; y < depthMD.YRes(); ++y)
	{
		const XnDepthPixel* pDepth = pDepthRow;
		unsigned char* pBuff = pBuffRow + depthMD.XOffset();

		for (XnUInt x = 0; x < depthMD.XRes(); ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				int nHistValue = depthHist[*pDepth];
				pBuff[2] = nHistValue;
				pBuff[1] = nHistValue;
				pBuff[0] = 0;
				pBuff[3] = 0;
			 }
			 pBuff += 4;
		}
		pDepthRow += depthMD.XRes();
		pBuffRow += depthMD.FullXRes()*4;
	}

	return depthDrewByColorBuf;
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
unsigned char * MyKinect::GetDepthMixedImage()
{
	xnOSMemSet(depthMixedImageBuf, 0, sizeof(depthMixedImageBuf));

	depthGen.GetMetaData(depthMD);
	const XnDepthPixel* pDepth = depthMD.Data();
	const XnDepthPixel* pDepthRow = depthMD.Data();
	unsigned char* pBuffRow = depthMixedImageBuf + depthMD.YOffset();
	unsigned char* pImgRow = imageBGRA32Buf + depthMD.YOffset();

	for (XnUInt y = 0; y < depthMD.YRes(); ++y)
	{
		const XnDepthPixel* pDepth = pDepthRow;
		unsigned char* pBuff = pBuffRow + depthMD.XOffset();
		unsigned char* pImg = pImgRow + depthMD.XOffset();

		for (XnUInt x = 0; x < depthMD.XRes(); ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				pBuff[2] = pImg[2];
				pBuff[1] = pImg[1];
				pBuff[0] = pImg[0];
				pBuff[3] = 0;
			}
			pBuff += 4;
			pImg += 4;
		}

		pDepthRow += depthMD.XRes();
		pBuffRow += depthMD.FullXRes()*4;
		pImgRow += depthMD.FullXRes()*4;
	}

	return depthMixedImageBuf;
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void MyKinect::GetImageSize(int *xSize, int *ySize)
{
	*xSize = (int)depthMD.FullXRes();
	*ySize = (int)depthMD.FullYRes();
}