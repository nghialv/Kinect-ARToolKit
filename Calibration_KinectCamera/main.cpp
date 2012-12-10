/*
 * 
 * This file is part of ARToolKit.
 * 
 * ARToolKit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * ARToolKit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with ARToolKit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

// ============================================================================
//	Includes
// ============================================================================

#ifdef _WIN32
#  include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#  include <GL/glut.h>
#  ifdef GL_VERSION_1_2
#    include <GL/glext.h>
#  endif
#else
#  include <GLUT/glut.h>
#  include <OpenGL/glext.h>
#endif
#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include <AR/gsub_lite.h>
#include <AR/ar.h>
#include <AR/matrix.h>

#include "MyKinect.h"

#define  H_NUM        6
#define  V_NUM        4
#define  LOOP_MAX    20
#define  THRESH     100

typedef struct {
    double   x_coord;
    double   y_coord;
} CALIB_COORD_T;

typedef struct patt {
    unsigned char  *savedImage[LOOP_MAX];
	ARGL_CONTEXT_SETTINGS_REF arglSettings[LOOP_MAX];
    CALIB_COORD_T  *world_coord;
    CALIB_COORD_T  *point[LOOP_MAX];
    int            h_num;				// Number of dots horizontally in the calibration pattern.
    int            v_num;				// Number of dots vertically in the calibration pattern.
    int            loop_num;			// How many images of the complete calibration patterns we have completed.
} CALIB_PATT_T;

// ============================================================================
//	Constants
// ============================================================================

#define CALIB_CAMERA2_DEBUG 0

// ============================================================================
//	Global variables
// ============================================================================

/* set up the video format globals */

#if defined(__sgi)
static char            *vconf = "-size=FULL";
#elif defined(__linux)
#  if defined(AR_INPUT_GSTREAMER)
char 			*vconf = "videotestsrc ! capsfilter caps=video/x-raw-rgb,bpp=24 ! identity name=artoolkit ! fakesink";
#  elif defined(AR_INPUT_V4L)
static char            *vconf = "-width=640 -height=480";
#  elif defined(AR_INPUT_1394CAM)
static char            *vconf = "-mode=640x480_YUV411";
#  elif defined(AR_INPUT_DV)
static char            *vconf = "";
#  endif
#elif defined(_WIN32)
static char			*vconf = "Data\\WDM_camera_flipV.xml";
#elif defined(__APPLE__)
static char			*vconf = "-width=640 -height=480";
#else
static char			*vconf = "";
#endif

static ARUint8		*gARTImage = NULL;
static ARParam		gARTCparam; // Dummy parameter, to supply to gsub_lite.
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;


static int             gWin;
static int             gXsize = 640;
static int             gYsize = 480;
static int             gThresh = THRESH;
static unsigned char   *gClipImage = NULL;

static CALIB_PATT_T    gPatt;          
static double          dist_factor[4];
static double          mat[3][4];

static int             point_num;
static int             gDragStartX = -1, gDragStartY = -1, gDragEndX = -1, gDragEndY = -1;	// x and y coordinates of start and end of mouse drag.

static int             gStatus;	// 0 = Waiting to grab image, 1 = Drawing bounding boxes, 2 = Placing warp lines.
static int             check_num;

MyKinect				g_MyKinect;
// ============================================================================
//	Functions
// ============================================================================

int             main(int argc, char *argv[]);
static int      init(int argc, char *argv[]);
static void     Mouse(int button, int state, int x, int y);
static void     Motion(int x, int y);
static void     Keyboard(unsigned char key, int x, int y);
static void     Quit(void);
static void     Idle(void);
static void     Visibility(int visible);
static void     Reshape(int w, int h);
static void     Display(void);
static void     draw_warp_line(double a, double b , double c);
static void     draw_line(void);
static void     draw_line2(double *x, double *y, int num);
static void     draw_warp_line(double a, double b , double c);
static void     print_comment(int status);
static void     save_param(void);


static int calc_inp2( CALIB_PATT_T *patt, CALIB_COORD_T *screen, double *pos2d, double *pos3d,
                      double dist_factor[4], double x0, double y0, double f[2], double *err );
static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, double para[3][3] );
static int  get_fl( double *p , double *q, int num, double f[2] );
static int  check_rotation( double rot[2][3] );

static double   get_fitting_error( CALIB_PATT_T *patt, double dist_factor[4] );
static double   check_error( double *x, double *y, int num, double dist_factor[4] );
static double   calc_distortion2( CALIB_PATT_T *patt, double dist_factor[4] );
static double   get_size_factor( double dist_factor[4], int xsize, int ysize );

int calc_inp( CALIB_PATT_T *patt, double dist_factor[4], int xsize, int ysize, double mat[3][4] )
{
    CALIB_COORD_T  *screen, *sp;
    double         *pos2d, *pos3d, *pp;
    double         f[2];
    double         x0, y0;
    double         err, minerr;
    int            res;
    int            i, j, k;

    sp = screen = (CALIB_COORD_T *)malloc(sizeof(CALIB_COORD_T) * patt->h_num * patt->v_num * patt->loop_num);
    pp = pos2d = (double *)malloc(sizeof(double) * patt->h_num * patt->v_num * patt->loop_num * 2);
    pos3d = (double *)malloc(sizeof(double) * patt->h_num * patt->v_num * 2);
    for( k = 0; k < patt->loop_num; k++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( i = 0; i < patt->h_num; i++ ) {
                arParamObserv2Ideal( dist_factor, 
                                     patt->point[k][j*patt->h_num+i].x_coord,
                                     patt->point[k][j*patt->h_num+i].y_coord,
                                     &(sp->x_coord), &(sp->y_coord) );
                *(pp++) = sp->x_coord;
                *(pp++) = sp->y_coord;
                sp++;
            }
        }
    }
    pp = pos3d;
    for( j = 0; j < patt->v_num; j++ ) {
        for( i = 0; i < patt->h_num; i++ ) {
            *(pp++) = patt->world_coord[j*patt->h_num+i].x_coord;
            *(pp++) = patt->world_coord[j*patt->h_num+i].y_coord;
        }
    }

    minerr = 100000000000000000000000.0;
    for( j = -50; j <= 50; j++ ) {
        printf("-- loop:%d --\n", j);
        y0 = dist_factor[1] + j;
/*      y0 = ysize/2 + j;   */
        if( y0 < 0 || y0 >= ysize ) continue;

        for( i = -50; i <= 50; i++ ) {
            x0 = dist_factor[0] + i;
/*          x0 = xsize/2 + i;  */
            if( x0 < 0 || x0 >= xsize ) continue;

            res = calc_inp2( patt, screen, pos2d, pos3d, dist_factor, x0, y0, f, &err );
            if( res < 0 ) continue;
            if( err < minerr ) {
                printf("F = (%f,%f), Center = (%f,%f): err = %f\n", f[0], f[1], x0, y0, err);
                minerr = err;

                mat[0][0] = f[0];
                mat[0][1] = 0.0;
                mat[0][2] = x0;
                mat[0][3] = 0.0;
                mat[1][0] = 0.0;
                mat[1][1] = f[1];
                mat[1][2] = y0;
                mat[1][3] = 0.0;
                mat[2][0] = 0.0;
                mat[2][1] = 0.0;
                mat[2][2] = 1.0;
                mat[2][3] = 0.0;
            }
        }
    }

    free( screen );
    free( pos2d );
    free( pos3d );

    if( minerr >= 100.0 ) return -1;
    return 0;
}


static int calc_inp2 ( CALIB_PATT_T *patt, CALIB_COORD_T *screen, double *pos2d, double *pos3d,
                      double dist_factor[4], double x0, double y0, double f[2], double *err )
{
    double  x1, y1, x2, y2;
    double  p[LOOP_MAX], q[LOOP_MAX];
    double  para[3][3];
    double  rot[3][3], rot2[3][3];
    double  cpara[3][4], conv[3][4];
    double  ppos2d[4][2], ppos3d[4][2];
    double  d, werr, werr2;
    int     i, j, k, l;

    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        x1 = para[0][0] / para[2][0];
        y1 = para[1][0] / para[2][0];
        x2 = para[0][1] / para[2][1];
        y2 = para[1][1] / para[2][1];

        p[i] = (x1 - x0)*(x2 - x0);
        q[i] = (y1 - y0)*(y2 - y0);
    }
    if( get_fl(p, q, patt->loop_num, f) < 0 ) return -1;

    cpara[0][0] = f[0];
    cpara[0][1] = 0.0;
    cpara[0][2] = x0;
    cpara[0][3] = 0.0;
    cpara[1][0] = 0.0;
    cpara[1][1] = f[1];
    cpara[1][2] = y0;
    cpara[1][3] = 0.0;
    cpara[2][0] = 0.0;
    cpara[2][1] = 0.0;
    cpara[2][2] = 1.0;
    cpara[2][3] = 0.0;

    werr = 0.0;
    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        rot[0][0] = (para[0][0] - x0*para[2][0]) / f[0];
        rot[0][1] = (para[1][0] - y0*para[2][0]) / f[1];
        rot[0][2] = para[2][0];
        d = sqrt( rot[0][0]*rot[0][0] + rot[0][1]*rot[0][1] + rot[0][2]*rot[0][2] );
        rot[0][0] /= d;
        rot[0][1] /= d;
        rot[0][2] /= d;
        rot[1][0] = (para[0][1] - x0*para[2][1]) / f[0];
        rot[1][1] = (para[1][1] - y0*para[2][1]) / f[1];
        rot[1][2] = para[2][1];
        d = sqrt( rot[1][0]*rot[1][0] + rot[1][1]*rot[1][1] + rot[1][2]*rot[1][2] );
        rot[1][0] /= d;
        rot[1][1] /= d;
        rot[1][2] /= d;
        check_rotation( rot );
        rot[2][0] = rot[0][1]*rot[1][2] - rot[0][2]*rot[1][1];
        rot[2][1] = rot[0][2]*rot[1][0] - rot[0][0]*rot[1][2];
        rot[2][2] = rot[0][0]*rot[1][1] - rot[0][1]*rot[1][0];
        d = sqrt( rot[2][0]*rot[2][0] + rot[2][1]*rot[2][1] + rot[2][2]*rot[2][2] );
        rot[2][0] /= d;
        rot[2][1] /= d;
        rot[2][2] /= d;
        rot2[0][0] = rot[0][0];
        rot2[1][0] = rot[0][1];
        rot2[2][0] = rot[0][2];
        rot2[0][1] = rot[1][0];
        rot2[1][1] = rot[1][1];
        rot2[2][1] = rot[1][2];
        rot2[0][2] = rot[2][0];
        rot2[1][2] = rot[2][1];
        rot2[2][2] = rot[2][2];

        ppos2d[0][0] = pos2d[i*patt->h_num*patt->v_num*2 + 0];
        ppos2d[0][1] = pos2d[i*patt->h_num*patt->v_num*2 + 1];
        ppos2d[1][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num-1)*2 + 0];
        ppos2d[1][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num-1)*2 + 1];
        ppos2d[2][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*(patt->v_num-1))*2 + 0];
        ppos2d[2][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*(patt->v_num-1))*2 + 1];
        ppos2d[3][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*patt->v_num-1)*2 + 0];
        ppos2d[3][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*patt->v_num-1)*2 + 1];
        ppos3d[0][0] = pos3d[0];
        ppos3d[0][1] = pos3d[1];
        ppos3d[1][0] = pos3d[(patt->h_num-1)*2 + 0];
        ppos3d[1][1] = pos3d[(patt->h_num-1)*2 + 1];
        ppos3d[2][0] = pos3d[(patt->h_num*(patt->v_num-1))*2 + 0];
        ppos3d[2][1] = pos3d[(patt->h_num*(patt->v_num-1))*2 + 1];
        ppos3d[3][0] = pos3d[(patt->h_num*patt->v_num-1)*2 + 0];
        ppos3d[3][1] = pos3d[(patt->h_num*patt->v_num-1)*2 + 1];

        for( j = 0; j < 5; j++ ) {
            arFittingMode = AR_FITTING_TO_IDEAL;
            werr2 = arGetTransMat3( rot2, ppos2d, ppos3d, 4, conv, dist_factor, cpara );
            for( k = 0; k < 3; k++ ) {
            for( l = 0; l < 3; l++ ) {
                rot2[k][l] = conv[k][l];
            }}
        }
        werr += werr2;

    }
    *err = sqrt( werr / patt->loop_num );

    return 0;
}

static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, double para[3][3] )
{
    ARMat   *a, *b, *c;
    ARMat   *at, *aa, res;
    int     i;

    a = arMatrixAlloc( num*2, 8 );
    b = arMatrixAlloc( num*2, 1 );
    c = arMatrixAlloc( 8, num*2 );
    at = arMatrixAlloc( 8, num*2 );
    aa = arMatrixAlloc( 8, 8 );
    for( i = 0; i < num; i++ ) {
        a->m[i*16+0]  = world[i].x_coord;
        a->m[i*16+1]  = world[i].y_coord;
        a->m[i*16+2]  = 1.0;
        a->m[i*16+3]  = 0.0;
        a->m[i*16+4]  = 0.0;
        a->m[i*16+5]  = 0.0;
        a->m[i*16+6]  = -world[i].x_coord * screen[i].x_coord;
        a->m[i*16+7]  = -world[i].y_coord * screen[i].x_coord;
        a->m[i*16+8]  = 0.0;
        a->m[i*16+9]  = 0.0;
        a->m[i*16+10] = 0.0;
        a->m[i*16+11] = world[i].x_coord;
        a->m[i*16+12] = world[i].y_coord;
        a->m[i*16+13] = 1.0;
        a->m[i*16+14] = -world[i].x_coord * screen[i].y_coord;
        a->m[i*16+15] = -world[i].y_coord * screen[i].y_coord;
        b->m[i*2+0] = screen[i].x_coord;
        b->m[i*2+1] = screen[i].y_coord;
    }
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    res.row = 8;
    res.clm = 1;
    res.m = &(para[0][0]);
    arMatrixMul( &res, c, b );
    para[2][2] = 1.0;

    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
}

static int get_fl( double *p , double *q, int num, double f[2] )
{
    ARMat   *a, *b, *c;
    ARMat   *at, *aa, *res;
    int     i;

#if 1
    a = arMatrixAlloc( num, 2 );
    b = arMatrixAlloc( num, 1 );
    c = arMatrixAlloc( 2, num );
    at = arMatrixAlloc( 2, num );
    aa = arMatrixAlloc( 2, 2 );
    res = arMatrixAlloc( 2, 1 );
    for( i = 0; i < num; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#else
    a = arMatrixAlloc( num-1, 2 );
    b = arMatrixAlloc( num-1, 1 );
    c = arMatrixAlloc( 2, num-1 );
    at = arMatrixAlloc( 2, num-1 );
    aa = arMatrixAlloc( 2, 2 );
    res = arMatrixAlloc( 2, 1 );
    p++; q++;
    for( i = 0; i < num-1; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#endif
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    arMatrixMul( res, c, b );

    if( res->m[0] < 0 || res->m[1] < 0 ) return -1;

    f[0] = sqrt( 1.0 / res->m[0] );
    f[1] = sqrt( 1.0 / res->m[1] );

    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
    arMatrixFree( res );

    return 0;
}

static int check_rotation( double rot[2][3] )
{
    double  v1[3], v2[3], v3[3];
    double  ca, cb, k1, k2, k3, k4;
    double  a, b, c, d;
    double  p1, q1, r1;
    double  p2, q2, r2;
    double  p3, q3, r3;
    double  p4, q4, r4;
    double  w;
    double  e1, e2, e3, e4;
    int     f;

    v1[0] = rot[0][0];
    v1[1] = rot[0][1];
    v1[2] = rot[0][2];
    v2[0] = rot[1][0];
    v2[1] = rot[1][1];
    v2[2] = rot[1][2];
    v3[0] = v1[1]*v2[2] - v1[2]*v2[1];
    v3[1] = v1[2]*v2[0] - v1[0]*v2[2];
    v3[2] = v1[0]*v2[1] - v1[1]*v2[0];
    w = sqrt( v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2] );
    if( w == 0.0 ) return -1;
    v3[0] /= w;
    v3[1] /= w;
    v3[2] /= w;

    cb = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    if( cb < 0 ) cb *= -1.0;
    ca = (sqrt(cb+1.0) + sqrt(1.0-cb)) * 0.5;

    if( v3[1]*v1[0] - v1[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v1[0] - v1[2]*v3[0] != 0.0 ) {
            w = v1[1]; v1[1] = v1[2]; v1[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v1[0]; v1[0] = v1[2]; v1[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v1[0] - v1[1]*v3[0] == 0.0 ) return -1;
    k1 = (v1[1]*v3[2] - v3[1]*v1[2]) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k3 = (v1[0]*v3[2] - v3[0]*v1[2]) / (v3[0]*v1[1] - v1[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v1[1] - v1[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r1 = (-b + sqrt(d))/a;
    p1 = k1*r1 + k2;
    q1 = k3*r1 + k4;
    r2 = (-b - sqrt(d))/a;
    p2 = k1*r2 + k2;
    q2 = k3*r2 + k4;
    if( f == 1 ) {
        w = q1; q1 = r1; r1 = w;
        w = q2; q2 = r2; r2 = w;
        w = v1[1]; v1[1] = v1[2]; v1[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p1; p1 = r1; r1 = w;
        w = p2; p2 = r2; r2 = w;
        w = v1[0]; v1[0] = v1[2]; v1[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    if( v3[1]*v2[0] - v2[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v2[0] - v2[2]*v3[0] != 0.0 ) {
            w = v2[1]; v2[1] = v2[2]; v2[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v2[0]; v2[0] = v2[2]; v2[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v2[0] - v2[1]*v3[0] == 0.0 ) return -1;
    k1 = (v2[1]*v3[2] - v3[1]*v2[2]) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k3 = (v2[0]*v3[2] - v3[0]*v2[2]) / (v3[0]*v2[1] - v2[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v2[1] - v2[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r3 = (-b + sqrt(d))/a;
    p3 = k1*r3 + k2;
    q3 = k3*r3 + k4;
    r4 = (-b - sqrt(d))/a;
    p4 = k1*r4 + k2;
    q4 = k3*r4 + k4;
    if( f == 1 ) {
        w = q3; q3 = r3; r3 = w;
        w = q4; q4 = r4; r4 = w;
        w = v2[1]; v2[1] = v2[2]; v2[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p3; p3 = r3; r3 = w;
        w = p4; p4 = r4; r4 = w;
        w = v2[0]; v2[0] = v2[2]; v2[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    e1 = p1*p3+q1*q3+r1*r3; if( e1 < 0 ) e1 = -e1;
    e2 = p1*p4+q1*q4+r1*r4; if( e2 < 0 ) e2 = -e2;
    e3 = p2*p3+q2*q3+r2*r3; if( e3 < 0 ) e3 = -e3;
    e4 = p2*p4+q2*q4+r2*r4; if( e4 < 0 ) e4 = -e4;
    if( e1 < e2 ) {
        if( e1 < e3 ) {
            if( e1 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }
    else {
        if( e2 < e3 ) {
            if( e2 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }

    return 0;
}



void calc_distortion( CALIB_PATT_T *patt, int xsize, int ysize, double dist_factor[4] )
{
    int     i, j;
    double  bx, by;
    double  bf[4];
    double  error, min;
    double  factor[4];

    bx = xsize / 2;
    by = ysize / 2;
    factor[0] = bx;
    factor[1] = by;
    factor[3] = 1.0;
    min = calc_distortion2( patt, factor );
    bf[0] = factor[0];
    bf[1] = factor[1];
    bf[2] = factor[2];
    bf[3] = 1.0;
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + j*5;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + i*5;
            error = calc_distortion2( patt, factor );
            if( error < min ) { bf[0] = factor[0]; bf[1] = factor[1];
                                bf[2] = factor[2]; min = error; }
        }
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    }

    bx = bf[0];
    by = bf[1];
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + 0.5 * j;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + 0.5 * i;
            error = calc_distortion2( patt, factor );
            if( error < min ) { bf[0] = factor[0]; bf[1] = factor[1];
                                bf[2] = factor[2]; min = error; }
        }
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    }

    dist_factor[0] = bf[0];
    dist_factor[1] = bf[1];
    dist_factor[2] = bf[2];
    dist_factor[3] = get_size_factor( bf, xsize, ysize );
}

static double get_size_factor( double dist_factor[4], int xsize, int ysize )
{
    double  ox, oy, ix, iy;
    double  olen, ilen;
    double  sf, sf1;

    sf = 100.0;

    ox = 0.0;
    oy = dist_factor[1];
    olen = dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = dist_factor[0] - ix;
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = dist_factor[1];
    olen = xsize - dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = ix - dist_factor[0];
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = 0.0;
    olen = dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = dist_factor[1] - iy;
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = ysize;
    olen = ysize - dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = iy - dist_factor[1];
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    if( sf == 0.0 ) sf = 1.0;

    return sf;
}

static double calc_distortion2( CALIB_PATT_T *patt, double dist_factor[4] )
{
    double    min, err, f, fb;
    int       i;

    dist_factor[2] = 0.0;
    min = get_fitting_error( patt, dist_factor );

    f = dist_factor[2];
    for( i = -100; i < 200; i+=10 ) {
        dist_factor[2] = i;
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    fb = f;
    for( i = -10; i <= 10; i++ ) {
        dist_factor[2] = fb + i;
/*
        if( dist_factor[2] < 0 ) continue;
*/
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    fb = f;
    for( i = -10; i <= 10; i++ ) {
        dist_factor[2] = fb + 0.1 * i;
/*
        if( dist_factor[2] < 0 ) continue;
*/
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    dist_factor[2] = f;
    return min;
}

static double get_fitting_error( CALIB_PATT_T *patt, double dist_factor[4] )
{
    double   *x, *y;
    double   error;
    int      max;
    int      i, j, k, l;
    int      p, c;

    max = (patt->v_num > patt->h_num)? patt->v_num: patt->h_num;
    x = (double *)malloc( sizeof(double)*max );
    y = (double *)malloc( sizeof(double)*max );
    if( x == NULL || y == NULL ) exit(0);

    error = 0.0;
    c = 0;
    for( i = 0; i < patt->loop_num; i++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( k = 0; k < patt->h_num; k++ ) {
                x[k] = patt->point[i][j*patt->h_num+k].x_coord;
                y[k] = patt->point[i][j*patt->h_num+k].y_coord;
            }
            error += check_error( x, y, patt->h_num, dist_factor );
            c += patt->h_num;
        }

        for( j = 0; j < patt->h_num; j++ ) {
            for( k = 0; k < patt->v_num; k++ ) {
                x[k] = patt->point[i][k*patt->h_num+j].x_coord;
                y[k] = patt->point[i][k*patt->h_num+j].y_coord;
            }
            error += check_error( x, y, patt->v_num, dist_factor );
            c += patt->v_num;
        }

        for( j = 3 - patt->v_num; j < patt->h_num - 2; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j+k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor );
            c += p;
        }

        for( j = 2; j < patt->h_num + patt->v_num - 3; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j-k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor );
            c += p;
        }
    }

    free( x );
    free( y );

    return sqrt(error/c);
}

static double check_error( double *x, double *y, int num, double dist_factor[4] )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    double   a, b, c;
    double   error;
    int      i;

    ev     = arVecAlloc( 2 );
    mean   = arVecAlloc( 2 );
    evec   = arMatrixAlloc( 2, 2 );

    input  = arMatrixAlloc( num, 2 );
    for( i = 0; i < num; i++ ) {
        arParamObserv2Ideal( dist_factor, x[i], y[i],
                             &(input->m[i*2+0]), &(input->m[i*2+1]) );
    }
    if( arMatrixPCA(input, evec, ev, mean) < 0 ) exit(0);
    a =  evec->m[1];
    b = -evec->m[0];
    c = -(a*mean->v[0] + b*mean->v[1]);

    error = 0.0;
    for( i = 0; i < num; i++ ) {
        error += (a*input->m[i*2+0] + b*input->m[i*2+1] + c)
               * (a*input->m[i*2+0] + b*input->m[i*2+1] + c);
    }
    error /= (a*a + b*b);

    arMatrixFree( input );
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );

    return error;
}

//======================================================================================
int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    if (!init(argc, argv)) exit(-1);
	
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(gXsize, gYsize);
    glutInitWindowPosition(100,100);
    gWin = glutCreateWindow("Calibrate camera parameters");
	
	// Setup argl library for current context.
	// Turn off distortion compensation.. we don't know distortion yet!
	if ((gArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gArglSettings, FALSE);
	
	// Make a dummy camera parameter to supply when calling arglDispImage().
	gARTCparam.xsize = gXsize;
	gARTCparam.ysize = gYsize;
	
	g_MyKinect.Init();

	// Register GLUT event-handling callbacks.
	// NB: Idle() is registered by Visibility.
    glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
	
	// Start grabbing.
	/*if (arVideoCapStart() != 0) {
    	fprintf(stderr, "init(): Unable to begin camera data capture.\n");
		return (FALSE);		
	}*/
	g_MyKinect.StartGeneratingAll();

	point_num = 0;
    gStatus = 0;
    print_comment(0);
	
    glutMainLoop();
	
	return (0);
}

static int init(int argc, char *argv[])
{
    double  length;
    char    line[512];
    int     i, j;

    gPatt.h_num    = H_NUM;
    gPatt.v_num    = V_NUM;
    gPatt.loop_num = 0;
    if (gPatt.h_num < 3 || gPatt.v_num < 3) exit(0);

    printf("Input the distance between each marker dot, in millimeters: ");
    scanf("%lf", &length);
	while (getchar() != '\n');

    gPatt.world_coord = (CALIB_COORD_T *)malloc(sizeof(CALIB_COORD_T) * gPatt.h_num * gPatt.v_num);
    for (j = 0; j < gPatt.v_num; j++) {
        for (i = 0; i < gPatt.h_num; i++) {
            gPatt.world_coord[j*gPatt.h_num+i].x_coord = length * i;
            gPatt.world_coord[j*gPatt.h_num+i].y_coord = length * j;
        }
    }
		
	// Add command-line arguments to vconf string.
    strcpy(line, vconf);
    for (i = 1; i < argc; i++) {
        strcat(line, " ");
        strcat(line, argv[i]);
    }
	
	// Open the video path.
    /*if (arVideoOpen(line) < 0) {
    	fprintf(stderr, "init(): Unable to open connection to camera.\n");
    	return (FALSE);
	}*/
	
	// Find the size of the window.
   /* if (arVideoInqSize(&gXsize, &gYsize) < 0) return (FALSE);
    fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", gXsize, gYsize);
	*/
	// Allocate space for a clipping image (luminance only).
	arMalloc(gClipImage, unsigned char, gXsize * gYsize);

	return (TRUE);
}

static void grabImage(void) {
	ARUint8 *image;
	
	// Processing a new image.
	// Copy an image to saved image buffer.
	do {
		//image = arVideoGetImage();
		image = (ARUint8 *)g_MyKinect.GetBGRA32Image();
	} while (image == NULL);
	gPatt.loop_num++;
	if ((gPatt.arglSettings[gPatt.loop_num-1] = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "grabImage(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gPatt.arglSettings[gPatt.loop_num-1], FALSE);
	arMalloc((gPatt.savedImage)[gPatt.loop_num-1], unsigned char, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);
	memcpy((gPatt.savedImage)[gPatt.loop_num-1], image, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);
	printf("Grabbed image %d.\n", gPatt.loop_num);
	arMalloc(gPatt.point[gPatt.loop_num-1], CALIB_COORD_T, gPatt.h_num*gPatt.v_num);
}

static void ungrabImage(void) {
	if (gPatt.loop_num == 0) {printf("error!!\n"); exit(0);}
	free(gPatt.point[gPatt.loop_num-1]);
	gPatt.point[gPatt.loop_num-1] = NULL;
	free(gPatt.savedImage[gPatt.loop_num-1]);
	gPatt.savedImage[gPatt.loop_num-1] = NULL;
	arglCleanup(gPatt.arglSettings[gPatt.loop_num-1]);
	gPatt.loop_num--;			
}

void checkFit(void) {
    printf("\n-----------\n");
	if (check_num < gPatt.loop_num) {
		printf("Checking fit on image %3d of %3d.\n", check_num + 1, gPatt.loop_num);
		if (check_num + 1 < gPatt.loop_num) {
			printf("Press mouse button to check fit of next image.\n");
		} else {
			printf("Press mouse button to calculate parameter.\n");
		}
		glutPostRedisplay();
	} else {
		if (gPatt.loop_num > 1) {
			if (calc_inp(&gPatt, dist_factor, gXsize, gYsize, mat) < 0) {
				printf("Calibration failed. Exiting.\n");
				exit(0);
			} else {
				printf("Calibration succeeded. Enter filename to save camera parameter to below.\n");
				save_param();
			}
		} else {
			printf("Not enough images to proceed with calibration. Exiting.\n");
		}
		Quit();
	}				
}

static void eventCancel(void) {
	// What was cancelled?
	if (gStatus == 0) {
		// Cancelled grabbing.
		// Live video will not be needed from here on.
		arVideoCapStop();
		if (gPatt.loop_num == 0) {
			// No images with all features identified, so quit.
			Quit();
		} else {
			// At least one image with all features identified,
			// so calculate distortion.
			calc_distortion(&gPatt, gXsize, gYsize, dist_factor);
			printf("--------------\n");
			printf("Center X: %f\n", dist_factor[0]);
			printf("       Y: %f\n", dist_factor[1]);
			printf("Dist Factor: %f\n", dist_factor[2]);
			printf("Size Adjust: %f\n", dist_factor[3]);
			printf("--------------\n");
			// Distortion calculation done. Check fit.
			gStatus = 2;
			check_num = 0;
			checkFit();
		}
	} else if (gStatus == 1) {
		// Cancelled rubber-bounding.
		ungrabImage();
		// Restart grabbing.
		point_num = 0;
		arVideoCapStart();
		gStatus = 0;
		if (gPatt.loop_num == 0) print_comment(0);
		else                     print_comment(4);	
	}
}

static void Mouse(int button, int state, int x, int y)
{
    unsigned char   *p;
    int             ssx, ssy, eex, eey;
    int             i, j, k;

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		eventCancel();
	} else if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			if (gStatus == 0 && gPatt.loop_num < LOOP_MAX) {
				// End grabbing.
				// Begin waiting for drag for rubber-bounding of a feature.
				grabImage();
				gDragStartX = gDragStartY = gDragEndX = gDragEndY = -1;
				arVideoCapStop();
				gStatus = 1;
				print_comment(1);				
			} else if (gStatus == 1) {
				if (point_num < gPatt.h_num*gPatt.v_num) {
					// Drag for rubber-bounding of a feature has begun.
					gDragStartX = gDragEndX = x;
					gDragStartY = gDragEndY = y;
				} else {
					// Feature point locations have been accepted.
					printf("### Image no.%d ###\n", gPatt.loop_num);
					for( j = 0; j < gPatt.v_num; j++ ) {
						for( i = 0; i < gPatt.h_num; i++ ) {
							printf("%2d, %2d: %6.2f, %6.2f\n", i+1, j+1,
								   gPatt.point[gPatt.loop_num-1][j*gPatt.h_num+i].x_coord,
								   gPatt.point[gPatt.loop_num-1][j*gPatt.h_num+i].y_coord);
						}
					}
					printf("\n\n");
					// Restart grabbing.
					point_num = 0;
					arVideoCapStart();
					gStatus = 0;
					if (gPatt.loop_num < LOOP_MAX) print_comment(4);
					else                           print_comment(5);
				}
			} else if (gStatus == 2) {
				check_num++;
				checkFit();
			}
		} else if (state == GLUT_UP) {
			if (gStatus == 1
				 && gDragStartX != -1 && gDragStartY != -1
				 && gDragEndX != -1 && gDragEndY != -1
				 && point_num < gPatt.h_num*gPatt.v_num) {
				// Drag for rubber-bounding of a feature has finished. Begin identification
				// of center of white region in gClipImage.
				if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
				else         { ssx = gDragEndX; eex = gDragStartX; }
				if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
				else         { ssy = gDragEndY; eey = gDragStartY; }
				
				gPatt.point[gPatt.loop_num-1][point_num].x_coord = 0.0;
				gPatt.point[gPatt.loop_num-1][point_num].y_coord = 0.0;
				p = gClipImage;
				k = 0;
				for (j = 0; j < (eey-ssy+1); j++) {
					for (i = 0; i < (eex-ssx+1); i++) {
						gPatt.point[gPatt.loop_num-1][point_num].x_coord += i * *p;
						gPatt.point[gPatt.loop_num-1][point_num].y_coord += j * *p;
						k += *p;
						p++;
					}
				}
				if (k != 0) {
					gPatt.point[gPatt.loop_num-1][point_num].x_coord /= k;
					gPatt.point[gPatt.loop_num-1][point_num].y_coord /= k;
					gPatt.point[gPatt.loop_num-1][point_num].x_coord += ssx;
					gPatt.point[gPatt.loop_num-1][point_num].y_coord += ssy;
					point_num++;
					printf("Marked feature position %3d of %3d\n", point_num, gPatt.h_num*gPatt.v_num);
					if (point_num == gPatt.h_num*gPatt.v_num) print_comment(2);
				}
				gDragStartX = gDragStartY = gDragEndX = gDragEndY = -1;
				glutPostRedisplay();
			}
		}
	}
}

static void Motion(int x, int y)
{
    unsigned char   *p, *p1;
    int             ssx, ssy, eex, eey;
    int             i, j;
	
	// During mouse drag.
    if (gStatus == 1 && gDragStartX != -1 && gDragStartY != -1) {
		
		// Set x,y of end of mouse drag region.
        gDragEndX = x;
        gDragEndY = y;
		
		// Check that start is above and to left of end.
		if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
		else         { ssx = gDragEndX; eex = gDragStartX; }
		if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
		else         { ssy = gDragEndY; eey = gDragStartY; }
		
		// Threshold clipping area, copy it into gClipImage.
        p1 = gClipImage;
        for (j = ssy; j <= eey; j++) {
            p = &(gPatt.savedImage[gPatt.loop_num-1][(j*gXsize+ssx)*AR_PIX_SIZE_DEFAULT]);
            for (i = ssx; i <= eex; i++) {
#if (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_BGRA)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_ABGR)
                *p1 = (((255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_ARGB)
                *p1 = (((255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_BGR)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_RGBA)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_RGB)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_2vuy)
                *p1 = ((255 - *(p+1)) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_yuvs)
                *p1 = ((255 - *(p+0)) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO)
                *p1 = ((255 - *(p)) < gThresh ? 0 : 255);
#else
#  error Unknown default pixel format defined in config.h
#endif
                p  += AR_PIX_SIZE_DEFAULT;
                p1++;
            }
        }
		// Tell GLUT the display has changed.
		glutPostRedisplay();
    }
}

static void Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 0x1B:
			eventCancel();
			break;
		case 'T':
		case 't':
			printf("Enter new threshold value (now = %d): ", gThresh);
			scanf("%d",&gThresh); while( getchar()!='\n' );
			printf("\n");
			break;
		case '1':
			gThresh -= 5;
			if (gThresh < 0) gThresh = 0;
			break;
		case '2':
			gThresh += 5;
			if (gThresh > 255) gThresh = 255;
			break;
		default:
			break;
	}
}

static void Quit(void)
{
	if (gClipImage) {
		free(gClipImage);
		gClipImage = NULL;
	}
	if (gArglSettings) arglCleanup(gArglSettings);
	if (gWin) glutDestroyWindow(gWin);
	//arVideoClose();
	g_MyKinect.Exit();

	exit(0);
}

static void Idle(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	ARUint8 *image;
	
	// Find out how long since Idle() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Grab a video frame.
	if (gStatus == 0) {
		//if ((image = arVideoGetImage()) != NULL) {
		if ((image = (ARUint8 *)g_MyKinect.GetBGRA32Image()) != NULL) {
			gARTImage = image;
			// Tell GLUT the display has changed.
			glutPostRedisplay();
		}	
	}
}

//
//	This function is called on events when the visibility of the
//	GLUT window changes (including when it first becomes visible).
//
static void Visibility(int visible)
{
	if (visible == GLUT_VISIBLE) {
		glutIdleFunc(Idle);
	} else {
		glutIdleFunc(NULL);
	}
}

//
//	This function is called when the
//	GLUT window is resized.
//
static void Reshape(int w, int h)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Call through to anyone else who needs to know about window sizing here.
}

static void beginOrtho2D(int xsize, int ysize) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, xsize, 0.0, ysize);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();	
}

static void endOrtho2D(void) {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static void Display(void)
{
    double         x, y;
    int            ssx, eex, ssy, eey;
    int            i;

	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	beginOrtho2D(gXsize, gYsize);

    if (gStatus == 0) {
		
		arglDispImage(gARTImage, &gARTCparam, 1.0, gArglSettings);	// zoom = 1.0.
		//arVideoCapNext();
		g_MyKinect.Update();
		gARTImage = NULL;
		
    } else if (gStatus == 1) {
		
        arglDispImage(gPatt.savedImage[gPatt.loop_num-1], &gARTCparam, 1.0, gPatt.arglSettings[gPatt.loop_num-1]);
		// Draw red crosses on the points in the image.
        for (i = 0; i < point_num; i++) {
            x = gPatt.point[gPatt.loop_num-1][i].x_coord;
            y = gPatt.point[gPatt.loop_num-1][i].y_coord;
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
              glVertex2d(x-10.0, (GLdouble)(gYsize-1)-y);
              glVertex2d(x+10.0, (GLdouble)(gYsize-1)-y);
              glVertex2d(x, (GLdouble)(gYsize-1)-(y-10.0));
              glVertex2d(x, (GLdouble)(gYsize-1)-(y+10.0));
            glEnd();
        }
		
		// Draw the current mouse drag clipping area.
        if (gDragStartX != -1 && gDragStartY != -1
			&& gDragEndX != -1 && gDragEndY != -1) {
			if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
			else         { ssx = gDragEndX; eex = gDragStartX; }
			if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
			else         { ssy = gDragEndY; eey = gDragStartY; }
#if 1
			if (gClipImage) {
				glPixelZoom(1.0f, -1.0f);	// ARToolKit bitmap 0.0 is at upper-left, OpenGL bitmap 0.0 is at lower-left.
				glRasterPos2f((GLfloat)(ssx), (GLfloat)(gYsize-1-ssy));
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glDrawPixels(eex-ssx+1, eey-ssy+1, GL_LUMINANCE, GL_UNSIGNED_BYTE, gClipImage);
			}
#else
            glColor3f(0.0f, 0.0f, 1.0f);
            glBegin(GL_LINE_LOOP);
              glVertex2i(gDragStartX, (gYsize-1)-gDragStartY);
              glVertex2i(gDragEndX, (gYsize-1)-gDragStartY);
              glVertex2i(gDragEndX, (gYsize-1)-gDragEndY);
              glVertex2i(gDragStartX, (gYsize-1)-gDragEndY);
            glEnd();
#endif
        }
		
    } else if (gStatus == 2) {
		
        arglDispImage(gPatt.savedImage[check_num], &gARTCparam, 1.0, gPatt.arglSettings[check_num]);
        for (i = 0; i < gPatt.h_num*gPatt.v_num; i++) {
            x = gPatt.point[check_num][i].x_coord;
            y = gPatt.point[check_num][i].y_coord;
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
			glVertex2d(x-10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x+10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x, (GLdouble)(gYsize-1)-(y-10.0));
			glVertex2d(x, (GLdouble)(gYsize-1)-(y+10.0));
            glEnd();
        }
        draw_line();
		
    }

	endOrtho2D();
    glutSwapBuffers();
}

static void draw_line(void)
{
    double   *x, *y;
    int      max;
    // int      num; // unreferenced
    int      i, j, k, l;
    int      p;

    max = (gPatt.v_num > gPatt.h_num) ? gPatt.v_num : gPatt.h_num;
    arMalloc(x, double, max);
    arMalloc(y, double, max);

    i = check_num;

    for (j = 0; j < gPatt.v_num; j++) {
        for (k = 0; k < gPatt.h_num; k++) {
            x[k] = gPatt.point[i][j*gPatt.h_num+k].x_coord;
            y[k] = gPatt.point[i][j*gPatt.h_num+k].y_coord;
        }
        draw_line2(x, y, gPatt.h_num);
    }

    for (j = 0; j < gPatt.h_num; j++) {
        for (k = 0; k < gPatt.v_num; k++) {
            x[k] = gPatt.point[i][k*gPatt.h_num+j].x_coord;
            y[k] = gPatt.point[i][k*gPatt.h_num+j].y_coord;
        }
        draw_line2(x, y, gPatt.v_num);
    }

    for (j = 3 - gPatt.v_num; j < gPatt.h_num - 2; j++) {
        p = 0;
        for (k = 0; k < gPatt.v_num; k++) {
            l = j+k;
            if (l < 0 || l >= gPatt.h_num) continue;
            x[p] = gPatt.point[i][k*gPatt.h_num+l].x_coord;
            y[p] = gPatt.point[i][k*gPatt.h_num+l].y_coord;
            p++;
        }
        draw_line2(x, y, p);
    }

    for (j = 2; j < gPatt.h_num + gPatt.v_num - 3; j++) {
        p = 0;
        for (k = 0; k < gPatt.v_num; k++) {
            l = j-k;
            if (l < 0 || l >= gPatt.h_num) continue;
            x[p] = gPatt.point[i][k*gPatt.h_num+l].x_coord;
            y[p] = gPatt.point[i][k*gPatt.h_num+l].y_coord;
            p++;
        }
        draw_line2(x, y, p);
    }

    free(x);
    free(y);
}

static void draw_line2( double *x, double *y, int num )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    double   a, b, c;
    int      i;

    ev     = arVecAlloc(2);
    mean   = arVecAlloc(2);
    evec   = arMatrixAlloc(2, 2);

    input  = arMatrixAlloc(num, 2);
    for (i = 0; i < num; i++) {
        arParamObserv2Ideal(dist_factor, x[i], y[i],
                             &(input->m[i*2+0]), &(input->m[i*2+1]));
    }
    if (arMatrixPCA(input, evec, ev, mean) < 0) exit(0);
    a =  evec->m[1];
    b = -evec->m[0];
    c = -(a*mean->v[0] + b*mean->v[1]);

    arMatrixFree(input);
    arMatrixFree(evec);
    arVecFree(mean);
    arVecFree(ev);

    draw_warp_line(a, b, c);
}

static void draw_warp_line(double a, double b , double c)
{
    double   x, y;
    double   x1, y1;
    int      i;

    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    if (a*a >= b*b) {
        for (i = -20; i <= gYsize+20; i+=10) {
            x = -(b*i + c)/a;
            y = i;

            arParamIdeal2Observ(dist_factor, x, y, &x1, &y1);
            glVertex2f(x1, gYsize-1-y1);
        }
    } else {
        for(i = -20; i <= gXsize+20; i+=10) {
            x = i;
            y = -(a*i + c)/b;

            arParamIdeal2Observ(dist_factor, x, y, &x1, &y1);
            glVertex2f(x1, gYsize-1-y1);
        }
    }
    glEnd();
}

static void print_comment(int status)
{
    printf("\n-----------\n");
    switch(status) {
        case 0:
			printf("Press mouse button to grab first image,\n");
			printf("or press right mouse button or [esc] to quit.\n");
			break;
        case 1:
			printf("Press mouse button and drag mouse to rubber-bound features (%d x %d),\n", gPatt.h_num, gPatt.v_num);
			printf("or press right mouse button or [esc] to cancel rubber-bounding & retry grabbing.\n");
			break;
        case 2:
			printf("Press mouse button to save feature positions,\n");
			printf("or press right mouse button or [esc] to discard feature positions & retry grabbing.\n");
			break;
        case 4:
			printf("Press mouse button to grab next image,\n");
			printf("or press right mouse button or [esc] to calculate distortion parameter.\n");
			break;
        case 5:
			printf("Press right mouse button or [esc] to calculate distortion parameter.\n");
			break;
    }
}

static void save_param(void)
{
    char     name[256];
    ARParam  param;
    int      i, j;
	
    param.xsize = gXsize;
    param.ysize = gYsize;
    for (i = 0; i < 4; i++) param.dist_factor[i] = dist_factor[i];
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            param.mat[j][i] = mat[j][i];
        }
    }
    arParamDisp( &param );
	
    printf("Filename: ");
    scanf( "%s", name );
    arParamSave( name, 1, &param );
	
    return;
}

