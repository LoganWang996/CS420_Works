/*
  CSCI 480 Computer Graphics
  Assignment 1: Height Fields
  C++ starter code
*/

#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

GLenum renderMode = GL_POINT; /* default renderMode is gl_POINT*/
int width = 640;
int height = 480;
int colorChoice = 3; /* default color choice is blue
                        1 - red, 2 - green, 3 - blue */
int screenshotIndex = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;

/* Write a screenshot to the specified filename */
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
}

// average 3 BPP values to get height for BPP=3 mode
float computeAverageBPP3(int i, int j)
{
  float height = PIC_PIXEL(g_pHeightData, i, j, 0)+PIC_PIXEL(g_pHeightData, i, j, 1)+PIC_PIXEL(g_pHeightData, i, j, 2);
  height = height/3.0;
  return height;
}

// set the vertex color before calling glVertex3f()
void prepareColorBPP1(int i, int j, float maxHeight)
{
  if (colorChoice == 3){
    glColor3f(PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight,
    PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight,
    1);
  }
  else if (colorChoice == 1){
    glColor3f(1,
    PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight,
    PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight);
  }
  else if (colorChoice == 2){
    glColor3f(PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight,
    1,
    PIC_PIXEL(g_pHeightData, i, j, 0)/maxHeight);
  }
}

void drawNodes()
{
  // for greyscale pictures
  if(g_pHeightData->bpp == 1){
    float maxHeight = 255.0f;
    // draw map by iterating through x, y coordinates
    glPolygonMode(GL_FRONT_AND_BACK, renderMode);
    for(int i=0; i<g_pHeightData->ny-1; i++)
    {
      glBegin(GL_TRIANGLE_STRIP);
      for(int j=0; j<g_pHeightData->nx; j++)
      {
        prepareColorBPP1(i, j, maxHeight);
        glVertex3f((i-128)/225.0, (j-128)/225.0,1*PIC_PIXEL(g_pHeightData, i, j, 0)/225.0/10.0);
        prepareColorBPP1(i+1, j, maxHeight);
        glVertex3f((i+1-128)/225.0, (j-128)/225.0,1*PIC_PIXEL(g_pHeightData, i+1, j, 0)/225.0/10.0);
      }
      glEnd();
    }
  }
  // for RGB pictures
  else if (g_pHeightData->bpp == 3){
    float maxHeight = 255.0f;
    // draw map
    glPolygonMode(GL_FRONT_AND_BACK, renderMode);
    for(int i=0; i<g_pHeightData->ny-1; i++)
    {
      glBegin(GL_TRIANGLE_STRIP);
      for(int j=0; j<g_pHeightData->nx; j++)
      {
        glColor3f(PIC_PIXEL(g_pHeightData, i, j, 0)/255.0,
            PIC_PIXEL(g_pHeightData, i, j, 1)/255.0,
            PIC_PIXEL(g_pHeightData, i, j, 2)/255.0);
        glVertex3f((i-128)/225.0, (j-128)/225.0,1*computeAverageBPP3(i, j)/225.0/10.0);
        glColor3f(PIC_PIXEL(g_pHeightData, i+1, j, 0)/255.0,
                  PIC_PIXEL(g_pHeightData, i+1, j, 1)/255.0,
                  PIC_PIXEL(g_pHeightData, i+1, j, 2)/255.0);
        glVertex3f((i+1-128)/225.0, (j-128)/225.0,1*computeAverageBPP3(i+1, j)/225.0/10.0);
      }
      glEnd();
    }
  }
}

void display()
{
  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); // reset transformation
  
  
  GLfloat aspect = (GLfloat) width / (GLfloat) height;
  
  gluPerspective(60.0, 1.0 * aspect, 0.01, 1000.0);
  gluLookAt(0, 0, 2.0, 0, 0, 0, 0, -1, 0);
  // code for rotating the output image
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  // code for translating and scaling
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
  
  glPolygonMode(GL_FRONT_AND_BACK, renderMode);
  drawNodes();
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
  gluLookAt(0, 0, 0, 0, 0, -0.5, 0, 1, 0);
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
  // press 1 for red color mode, 2 for green, 3 for blue, default is blue
  if(key=='1')
    colorChoice=1;
  if(key=='2')
    colorChoice=2;
  if(key=='3')
    colorChoice=3;
  
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
    printf ("usage: %s heightfield.jpg\n", argv[0]);
    exit(1);
  }

  g_pHeightData = jpeg_read(argv[1], NULL);
  if (!g_pHeightData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }

  glutInit(&argc,argv);
  
  // request double buffer
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  
  // set window size
  glutInitWindowSize(width, height);
  
  // set window position
  glutInitWindowPosition(0, 0);
  
  // creates a window
  glutCreateWindow("Homework_1");
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

  glutMainLoop();
  return(0);
}
