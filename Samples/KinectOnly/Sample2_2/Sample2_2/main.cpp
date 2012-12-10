#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>

#include "Common.h"
#include "MyKinect.h"
#include "HandDetectorOpenNI.h"

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

int             xsize = 640;
int				ysize = 480;
int             thresh = 100;
int             count = 0;

int             mode = 1;

char           *cparam_name    = "Data/MyCameraParameter4.dat";
ARParam         cparam;

char           *patt_name      = "Data/patt.hiro";
int             patt_id;
int             patt_width     = 80.0;
double          patt_center[2] = {0.0, 0.0};
double          patt_trans[3][4];

float			size = 40.0f;
int				displayMode =1;
//MyKinect object
MyKinect		g_MyKinect;
HandDetectorOpenNI g_HandDetectorOpenNI;

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw( double trans[3][4] , bool markerVisible, float handPosX, float handPosY, float handPosZ);

void ClickPointerFunction()
{
	size += 10.0f;
}

int main(int argc, char **argv)
{
	printf("Sample4_2\n");
	glutInit(&argc, argv);
    init();

	//init for Kinect
	g_MyKinect.Init();
	g_HandDetectorOpenNI.Init(g_MyKinect.context);
	//add Gesture
	g_HandDetectorOpenNI.AddGesture("Click", NULL);
	g_HandDetectorOpenNI.ClickPointerFunc = &ClickPointerFunction;
	g_MyKinect.StartGeneratingAll();
	
   // arVideoCapStart();
    argMainLoop( NULL, keyEvent, mainLoop );
	return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }

    if( key == 'c' ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;

        mode = 1 - mode;
        if( mode ) printf("Continuous mode: Using arGetTransMatCont.\n");
         else      printf("One shot mode: Using arGetTransMat.\n");
    }
	if(key == '1')
		displayMode = 1;
	if(key == '2')
		displayMode = 2;
	if(key == '3')
		displayMode = 3;
}

/* main loop */
static void mainLoop(void)
{
    static int      contF = 0;
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             j, k;

	//update new data
	g_MyKinect.Update();

    /* grab a vide frame */
   /* if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }*/
	
	//get image data to detect marker
	if( (dataPtr = (ARUint8 *)g_MyKinect.GetBGRA32Image()) == NULL ) {
        arUtilSleep(2);
        return;
    }

    if( count == 0 ) arUtilTimerReset();
    count++;

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }

	//option . You can choose many display mode. image, Depth by Color, depth mixed image
	if(displayMode == 2)
		dataPtr = (ARUint8 *)g_MyKinect.GetDepthDrewByColor();
	else
		if(displayMode == 3)
			dataPtr = (ARUint8 *)g_MyKinect.GetDepthMixedImage();

	argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

   // arVideoCapNext();

    /* check for object visibility */
    k = -1;
    for( j = 0; j < marker_num; j++ ) {
        if( patt_id == marker_info[j].id ) {
            if( k == -1 ) k = j;
            else if( marker_info[k].cf < marker_info[j].cf ) k = j;
        }
    }
    if( k == -1 ) {
        contF = 0;

#ifdef USE_USERDETECTOR
		if(g_MyKinect.userStatus.isPlayerVisible())
		{
			XV3 tmp = g_MyKinect.userDetector->getSkeletonJointPosition(XN_SKEL_LEFT_HAND);
			printf("Right hand position: %.2f %.2f %.2f\n", tmp.X, tmp.Y, tmp.Z);
			draw( patt_trans, false, tmp.X, -tmp.Y, tmp.Z);
		}
#endif
        argSwapBuffers();
        return;
    }

    /* get the transformation between the marker and the real camera */
    if( mode == 0 || contF == 0 ) {
        arGetTransMat(&marker_info[k], patt_center, patt_width, patt_trans);
    }
    else {
        arGetTransMatCont(&marker_info[k], patt_trans, patt_center, patt_width, patt_trans);
    }
    contF = 1;

	printf("MARKER POSITION	   : %.2f %.2f %.2f\n", patt_trans[0][3], patt_trans[1][3], patt_trans[2][3]);
#ifdef USE_USERDETECTOR
	if(g_MyKinect.userStatus.isPlayerVisible())
	{
		XV3 tmp = g_MyKinect.userDetector->getSkeletonJointPosition(XN_SKEL_LEFT_HAND);
		printf("Right hand position: %.2f %.2f %.2f\n", tmp.X, tmp.Y, tmp.Z);
		draw( patt_trans, true, tmp.X, -tmp.Y, tmp.Z);
	}
#endif

    argSwapBuffers();
}

static void init( void )
{
    ARParam  wparam;

    ///* open the video path */
    //if( arVideoOpen( vconf ) < 0 ) exit(0);
    ///* find the size of the window */
    //if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    //printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }

    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

    if( (patt_id=arLoadPatt(patt_name)) < 0 ) {
        printf("pattern load error !!\n");
        exit(0);
    }

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
	//arVideoCapStop();
    arVideoClose();
    argCleanup();
	//clean MyKinect
	g_MyKinect.Exit();
}

static void draw( double trans[3][4], bool markerVisible, float handPosX, float handPosY, float handPosZ)
{
    double    gl_para[16];
    GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    argConvGlpara(trans, gl_para);
	//printf("%.2f  %.2f  %.2f\n", trans[0][3], trans[1][3], trans[2][3]);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

	if(markerVisible)
	{
		glPushMatrix();
		glLoadMatrixd( gl_para );
		// glMatrixMode(GL_MODELVIEW);
		glTranslatef( 0.0, 0.0, size/2.0f);
		glutSolidCube(size);
		glPopMatrix();
	}
	glPushMatrix();
	glTranslatef(handPosX, handPosY, handPosZ);
	glutSolidCube(40.0f);
	glPopMatrix();

    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}
