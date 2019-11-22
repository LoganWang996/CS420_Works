/*
  CSCI 480
  Assignment 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include <vector>
#include <math.h>



/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

GLenum renderMode = GL_FILL; /* default renderMode is gl_POINT*/
int width = 640;
int height = 480;
int screenshotIndex = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};
GLuint texNames[10];
double s = 0.5, DELTA_U = 0.001;
int camPosIndex = 0;
std::vector <point> posToDraw, vts, vns, vbs;

int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf",
     &g_Splines[j].points[i].x,
     &g_Splines[j].points[i].y,
     &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}

void initTexture()
{
  char textureFile[50];
  // load textures for background and master yoda
  for(int i=1; i<=7; i++)
  {
    sprintf(textureFile, "background/%d.jpg", i);
    Pic* texture = jpeg_read(textureFile, NULL);
    if(!texture)
    {
      printf("error reading %s.\n", textureFile);
      exit(1);
    }
    glGenTextures(1, &texNames[i]);
    glBindTexture(GL_TEXTURE_2D, texNames[i]);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->nx, texture->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->pix);
  }
}

void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

void myinit()
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST);
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH);
//  glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
}

// write to matrix
void Matrix_write(double *m, int row, int col, int sizeCol, double val)
{
  *(m+row*sizeCol+col) = val;
}

// read from matrix
double Matrix_read(double *m, int row, int col, int sizeCol)
{
  return *(m+row*sizeCol+col);
}

// for debugging
void printMatrix(double *m, int r, int c)
{
  for(int i=0; i<r; i++){
    printf("[ ");
    for(int j=0; j<c; j++)
    {
      printf("%f ",Matrix_read(m, i, j, c));
      if(j<c-1)
        printf(", ");
    }
    printf("]\n");
  }
}

// multiply 2 matrices with specified dimensions
double* Matrix_mult(double *m1, int row1, int col1, double *m2, int row2, int col2, double *result)
{
  for(int i=0; i<row1; i++)
  {
    for(int j=0; j<col2; j++)
    {
      Matrix_write(result, i, j, col2, 0);
      for(int k=0; k<col1; k++)
      {
        double before = Matrix_read(result, i, j, col2);
        double toAdd = Matrix_read(m1, i, k, col1) * Matrix_read(m2, k, j, col2);
        Matrix_write(result, i, j, col2, before+toAdd);
      }
    }
  }
}

// calculate the length of the vector for normalizing
double vectorLength(double x, double y, double z)
{
  return sqrt(x*x+y*y+z*z);
}

// perform cross product of 2 3D vectors
void Vec3_Cross(double ax, double ay, double az, double bx, double by, double bz, double *result)
{
  Matrix_write(result, 0, 0, 3, ay*bz-az*by);
  Matrix_write(result, 0, 1, 3, az*bx-ax*bz);
  Matrix_write(result, 0, 2, 3, ax*by-ay*bx);
}

void calculatePosition()
{
  double mBasis [4][4] = {  {-1*s, 2-s, s-2, s}, {2*s, s-3, 3-2*s, -1*s},
                            {-1*s, 0, s, 0}, {0, 1, 0, 0} };
  for(int i=0; i<g_iNumOfSplines; i++)
  {
    for(int j=0; j<g_Splines[i].numControlPoints-3; j++)
    {
      // control matrix
      double mControl [4][3] ={
        {g_Splines[i].points[j].x, g_Splines[i].points[j].y, g_Splines[i].points[j].z},
        {g_Splines[i].points[j+1].x, g_Splines[i].points[j+1].y, g_Splines[i].points[j+1].z},
        {g_Splines[i].points[j+2].x, g_Splines[i].points[j+2].y, g_Splines[i].points[j+2].z},
        {g_Splines[i].points[j+3].x, g_Splines[i].points[j+3].y, g_Splines[i].points[j+3].z} };
      
      for(double u=0; u<=1; u += DELTA_U)
      {
        // calculate coordinate
        double mU [1][4] = {u*u*u, u*u, u, 1};
        double res12[1][4];
        Matrix_mult(&(mU[0][0]), 1, 4, &(mBasis[0][0]), 4, 4, &(res12[0][0]));
        double res123[1][3];
        Matrix_mult(&(res12[0][0]), 1, 4, &(mControl[0][0]), 4, 3, &(res123[0][0]));
        point coord = {res123[0][0], res123[0][1], res123[0][2]};
        // push to a vector of points storing the xyz coordinates of each step
        posToDraw.push_back(coord);
        
        // calculate tangent vector
        double mUderiv [1][4] = {{3*u*u, 2*u, 1, 0}};
        double tan12[1][4];
        Matrix_mult(&(mUderiv[0][0]), 1, 4, &(mBasis[0][0]), 4, 4, &(tan12[0][0]));
        double tan123[1][3];
        Matrix_mult(&(tan12[0][0]), 1, 4, &(mControl[0][0]), 4, 3, &(tan123[0][0]));
        double vLen = vectorLength(tan123[0][0], tan123[0][1], tan123[0][2]);
        point tangent = {tan123[0][0]/vLen, tan123[0][1]/vLen, tan123[0][2]/vLen};
        // push to a vector of points storing the tangent vectors
        vts.push_back(tangent);
        // calculate binormal vector
        double t0[3] = {vts.back().x, vts.back().y, vts.back().z};
        if(u==0 && i==0 && j==0){
          double n0[3], b0[3];
          // arbitrary vector V = (1, 0, 0)
          // if T0 parallel to (1, 0, 0), change to (0, 1, 0)
          double arbv[3] = {(double)1, (double)0, (double)0};
          if(t0[1] == 0 && t0[2] == 0)  {
            arbv[0] = 0;
            arbv[1] = 1;
          }
          // N0 = unit(T0×V)
          Vec3_Cross(t0[0], t0[1], t0[2], arbv[0], arbv[1], arbv[2], n0);
          double nLen = vectorLength(n0[0], n0[1], n0[2]);
          // push to a vector of points storing the normal vectors
          point normal0 = {n0[0]/nLen, n0[1]/nLen, n0[2]/nLen};
          vns.push_back(normal0);
          // B0 = unit(T0×N0)
          Vec3_Cross(t0[0], t0[1], t0[2], n0[0], n0[1], n0[2], b0);
          double bLen = vectorLength(b0[0], b0[1], b0[2]);
          point binormal = {b0[0]/bLen, b0[1]/bLen, b0[2]/bLen};
          // push to a vector of points storing the binormal vectors
          vbs.push_back(binormal);
        }
        else{
          double n1[3], b1[3];
          // N1 = unit(B0×T1)
          Vec3_Cross(vbs.back().x, vbs.back().y, vbs.back().z, vts.back().x,
                     vts.back().y, vts.back().z, n1);
          double nLen = vectorLength(n1[0], n1[1], n1[2]);
          point normal = {n1[0]/nLen, n1[1]/nLen, n1[2]/nLen};
          vns.push_back(normal);
          // B1 = unit(T1×N1)
          Vec3_Cross(vts.back().x, vts.back().y, vts.back().z, normal.x,
                     normal.y, normal.z, b1);
          double bLen = vectorLength(b1[0], b1[1], b1[2]);
          point binormal = {b1[0]/bLen, b1[1]/bLen, b1[2]/bLen};
          vbs.push_back(binormal);
        }
      }
    }
  }
}

void setCamera()
{
  if(camPosIndex>= posToDraw.size()){
    camPosIndex=0;
  }
  // coordinates for plane coord, tangent for focus, normal for up
  if(camPosIndex < posToDraw.size())
  {
    // add offset to camera position so that it's a bit above the cross section
    double sOffset = 0.2, sfront = 10;

    gluLookAt(posToDraw[camPosIndex].x+vns[camPosIndex].x*sOffset,
    posToDraw[camPosIndex].y+vns[camPosIndex].y*sOffset,
    posToDraw[camPosIndex].z+vns[camPosIndex].z*sOffset,
    posToDraw[camPosIndex].x+vts[camPosIndex].x*sfront,
    posToDraw[camPosIndex].y+vts[camPosIndex].y*sfront,
    posToDraw[camPosIndex].z+vts[camPosIndex].z*sfront,
    vns[camPosIndex].x, vns[camPosIndex].y, vns[camPosIndex].z);
    camPosIndex+=10;
    
  }
}

void drawPoint(point a)
{
  glVertex3f(a.x, a.y, a.z);
}

void drawLines()
{
  if (posToDraw.empty()) return;
  glBegin(GL_TRIANGLE_STRIP);
    glLineWidth(1);
  glColor3f(169.0/255, 166.0/255, 160.0/255);
  // draw left and right double rails
  for(int j=-1; j<2; j+=2){
  for(int i=0; i<posToDraw.size()-1; i++)
  {
//    glVertex3f(posToDraw[i].x, posToDraw[i].y, posToDraw[i].z);
//    glVertex3f(posToDraw[i+1].x, posToDraw[i+1].y, posToDraw[i+1].z);
    
    // scale of cross section
    double scc =0.03;
    // scale of cross section distance
    double scd = 0.2;
      
      point v0 = {(posToDraw[i].x+scc*(vns[i].x - vbs[i].x)-vbs[i].x*scd*j),
      (posToDraw[i].y+scc*(vns[i].y- vbs[i].y)-vbs[i].y*scd*j),
        (posToDraw[i].z+scc*(vns[i].z- vbs[i].z)-vbs[i].z*scd*j) };

      point v1 = {(posToDraw[i].x+scc*(vns[i].x + vbs[i].x)-vbs[i].x*scd*j),
      (posToDraw[i].y+scc*(vns[i].y+ vbs[i].y)-vbs[i].y*scd*j),
        (posToDraw[i].z+scc*(vns[i].z+ vbs[i].z)-vbs[i].z*scd*j) };

      point v2 = {(posToDraw[i].x+scc*(-1*vns[i].x + vbs[i].x)-vbs[i].x*scd*j),
      (posToDraw[i].y+scc*(-1*vns[i].y+ vbs[i].y)-vbs[i].y*scd*j),
      (posToDraw[i].z+scc*(-1*vns[i].z+ vbs[i].z)-vbs[i].z*scd*j) };

      point v3 = {(posToDraw[i].x+scc*(-1*vns[i].x - vbs[i].x)-vbs[i].x*scd*j),
      (posToDraw[i].y+scc*(-1*vns[i].y - vbs[i].y)-vbs[i].y*scd*j),
      (posToDraw[i].z+scc*(-1*vns[i].z- vbs[i].z)-vbs[i].z*scd*j) };


      point v4 = {(posToDraw[i+1].x+scc*(vns[i+1].x - vbs[i+1].x)-vbs[i].x*scd*j),
      (posToDraw[i+1].y+scc*(vns[i+1].y- vbs[i+1].y)-vbs[i].y*scd*j),
      (posToDraw[i+1].z+scc*(vns[i+1].z- vbs[i+1].z)-vbs[i].z*scd*j) };

      point v5 = {(posToDraw[i+1].x+scc*(vns[i+1].x + vbs[i+1].x)-vbs[i].x*scd*j),
      (posToDraw[i+1].y+scc*(vns[i+1].y+ vbs[i+1].y)-vbs[i].y*scd*j),
      (posToDraw[i+1].z+scc*(vns[i+1].z+ vbs[i+1].z)-vbs[i].z*scd*j) };

      point v6 = {(posToDraw[i+1].x+scc*(-1*vns[i+1].x + vbs[i+1].x)-vbs[i].x*scd*j),
      (posToDraw[i+1].y+scc*(-1*vns[i+1].y + vbs[i+1].y)-vbs[i].y*scd*j),
      (posToDraw[i+1].z+scc*(-1*vns[i+1].z + vbs[i+1].z)-vbs[i].z*scd*j) };

      point v7 = {(posToDraw[i+1].x+scc*(-1*vns[i+1].x - vbs[i+1].x)-vbs[i].x*scd*j),
      (posToDraw[i+1].y+scc*(-1*vns[i+1].y - vbs[i+1].y)-vbs[i].y*scd*j),
        (posToDraw[i+1].z+scc*(-1*vns[i+1].z - vbs[i+1].z)-vbs[i].z*scd*j) };

      // draw triangle strips
      // right
      drawPoint(v3);
      drawPoint(v2);
      drawPoint(v6);
      drawPoint(v7);
      // up
      drawPoint(v1);
      drawPoint(v2);
      drawPoint(v6);
      drawPoint(v5);
      // left
      drawPoint(v0);
      drawPoint(v1);
      drawPoint(v5);
      drawPoint(v4);
      // down
      drawPoint(v0);
      drawPoint(v3);
      drawPoint(v7);
      drawPoint(v4);
  }
  }
  glEnd();
}

void drawBackground()
{
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glEnable(GL_TEXTURE_2D);
  double scale = 300;
  glPolygonMode(GL_FRONT_AND_BACK, renderMode);
  // up
  glBindTexture(GL_TEXTURE_2D, texNames[1]);
  glBegin(GL_QUADS);
    glTexCoord2f(1.0,1.0);
    glVertex3f(-1*scale,1*scale,-1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(1*scale,1*scale,-1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(1*scale,1*scale,1*scale);
    glTexCoord2f(0.0,1.0);
    glVertex3f(-1*scale,1*scale,1*scale);
  glEnd();
  
  // front
  glBindTexture(GL_TEXTURE_2D, texNames[2]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0);
    glVertex3f(-1*scale,-1*scale,-1*scale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(1*scale,-1*scale,-1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(1*scale,1*scale,-1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(-1*scale,1*scale,-1*scale);
  glEnd();
  
  // right
  glBindTexture(GL_TEXTURE_2D, texNames[3]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0);
    glVertex3f(1*scale,-1*scale,-1*scale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(1*scale,-1*scale,1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(1*scale,1*scale,1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(1*scale,1*scale,-1*scale);
  glEnd();
  
  // left
  glBindTexture(GL_TEXTURE_2D, texNames[4]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0);
    glVertex3f(-1*scale,-1*scale,1*scale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(-1*scale,-1*scale,-1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(-1*scale,1*scale,-1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(-1*scale,1*scale,1*scale);
  glEnd();
  
  // back
  glBindTexture(GL_TEXTURE_2D, texNames[5]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0);
    glVertex3f(1*scale,-1*scale,1*scale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(-1*scale,-1*scale,1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(-1*scale,1*scale,1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(1*scale,1*scale,1*scale);
  glEnd();
  
  // down
  glBindTexture(GL_TEXTURE_2D, texNames[6]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0);
    glVertex3f(1*scale,-1*scale,1*scale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(1*scale,-1*scale,-1*scale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(-1*scale,-1*scale,-1*scale);
    glTexCoord2f(0.0,0.0);
    glVertex3f(-1*scale,-1*scale,1*scale);
  glEnd();
  
  // yoda
  double yodaScale = 60;
  glBindTexture(GL_TEXTURE_2D, texNames[7]);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0,0.0);
    glVertex3f(-1*yodaScale,1*scale*0.7,-1*yodaScale);
    glTexCoord2f(0.0,1.0);
    glVertex3f(1*yodaScale,1*scale*0.7,-1*yodaScale);
    glTexCoord2f(1.0,1.0);
    glVertex3f(1*yodaScale,1*scale*0.7,1*yodaScale);
    glTexCoord2f(1.0,0.0);
    glVertex3f(-1*yodaScale,1*scale*0.7,1*yodaScale);
  glEnd();
  
  glDisable(GL_TEXTURE_2D);
}

// for debugging
bool print = true;
void checkVector()
{
  if(print)
  {
    FILE * pFile = fopen ("cam.txt","w");
    fprintf(pFile, "%d\n", posToDraw.size());
    for(int i=0; i<posToDraw.size(); i++)
    {
      fprintf(pFile, "%f  %f  %f ----- ", posToDraw[i].x, posToDraw[i].y, posToDraw[i].z);
      fprintf(pFile, "%f  %f  %f ----- ", vts[i].x, vts[i].y, vts[i].z);
      fprintf(pFile, "%f  %f  %f ----- ", vns[i].x, vns[i].y, vns[i].z);
      fprintf(pFile, "%f  %f  %f\n", vbs[i].x, vbs[i].y, vbs[i].z);
      if(isnan(vns[i].x) || isnan(vbs[i].x))
        printf("theres nan");
    }
    print = false;
    printf("output to file.\n");
    fclose (pFile);
  }
}

void display()
{
  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); // reset transformation
  
  
  
  GLfloat aspect = (GLfloat) width / (GLfloat) height;
  
  gluPerspective(80, 1.0 * aspect, 0.01f, 1000.0);
//  glFrustum(-0.1, 0.1, -float(height)/(10.0*float(width)), float(height)/(10.0*float(width)), 0.1, 5000.0);
  
  
//  gluLookAt(0, 0, 2.0, 0, 0, 0, 0, 1, 0);
  
  // code for rotating the output image
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  // code for translating and scaling
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
  
  glPolygonMode(GL_FRONT_AND_BACK, renderMode);
  setCamera();
  checkVector();
  drawLines();
  drawBackground();
  //checkVector();
  glutSwapBuffers(); // double buffer flush
}

void reshape(int w, int h)
{
  width = w;
  height = h;
  GLfloat aspect = (GLfloat) w / (GLfloat) h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
//  gluLookAt(0, 0, 0, 0, 0, -0.5, 0, 1, 0);
  gluPerspective(60.0, 1.0 * aspect, 0.01, 5.0);
  
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_ALT:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void keyboard(unsigned char key, int x, int y)
{
  // user can press q to quit
  if (key=='q' || key == 'Q')
      exit(0);
  // press p for point render mode, l for line(wireframe), f for fill(solid)
  if(key=='p' || key=='P')
    renderMode = GL_POINT;
  if(key=='l' || key=='L')
    renderMode = GL_LINE;
  if(key=='f' || key=='F')
    renderMode = GL_FILL;
  
  // hold T to translate output with mouse
  if(key=='t' || key=='T'){
    g_ControlState = TRANSLATE;
  }
  
  if(key==' '){
    char myFilenm[2048];
    sprintf(myFilenm, "anim.%04d.jpg", screenshotIndex++);
    saveScreenshot(myFilenm);
  }
  
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }
  loadSplines(argv[1]);
  glutInit(&argc,argv);
  // request double buffer
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  // set window size
  glutInitWindowSize(width, height);
  // set window position
  glutInitWindowPosition(0, 0);
  // creates a window
  glutCreateWindow("Homework_2");
  //exit(0);
  /* tells glut to use a particular display function to redraw */
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit",0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  /* replace with any animate code */
  glutIdleFunc(doIdle);
  /* callback for mouse drags */
  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);
  /* do initialization */
  myinit();
  calculatePosition();
  initTexture();
//
  glutMainLoop();
//  return(0);
  return(0);
}
