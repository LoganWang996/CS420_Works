/*
CSCI 480
Assignment 3 Raytracer

Name: Jizhong Wang
*/

#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename=0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode=MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define FOV 60.0
#define DIST_NEAR 0.01
GLfloat aspect = (GLfloat) WIDTH / (GLfloat) HEIGHT;
double SMALL_Z = -1000000;

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Vector3 {
  // coordinates
  double x;
  double y;
  double z;
  
  Vector3()
    :x(0.0f)
    ,y(0.0f)
    ,z(0.0f)
  {}
  
  Vector3(double cox, double coy, double coz)
    :x(cox)
    ,y(coy)
    ,z(coz)
  {}
  
  Vector3(Vertex ver)
    :x(ver.position[0])
    ,y(ver.position[1])
    ,z(ver.position[2])
  {}
  
  Vector3(double* array)
    :x(array[0])
    ,y(array[1])
    ,z(array[2])
  {}
  
  void Set(double cox, double coy, double coz)
  {
    x = cox;
    y = coy;
    z = coz;
  }
  
  friend Vector3 operator+(const Vector3& a, const Vector3& b)
  {
    return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
  }
  
  friend Vector3 operator-(const Vector3& a, const Vector3& b)
  {
    return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
  }
  
  friend Vector3 operator*(const Vector3& a, const Vector3& b)
  {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
  }
  
  friend Vector3 operator*(const Vector3& v, double s)
  {
    return Vector3(v.x * s, v.y * s, v.z * s);
  }
  
  friend Vector3 operator*(double s, const Vector3& v)
  {
    return Vector3(v.x * s, v.y * s, v.z * s);
  }
  
  Vector3& operator*=(double s)
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }
  
  Vector3& operator/=(double s)
  {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }
  
  Vector3& operator+=(const Vector3& v)
  {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  
  Vector3& operator-=(const Vector3& v)
  {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
  
  double VecSquared() const
  {
    return (x*x + y*y + z*z);
  }
  
  double Length() const
  {
    return (sqrt(VecSquared()));
  }
  
  void Normalize()
  {
    double length = Length();
    x /= length;
    y /= length;
    z /= length;
  }
  
  static double Dot(const Vector3& a, const Vector3& b)
  {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
  }
  
  static Vector3 Cross(const Vector3& a, const Vector3& b)
  {
    Vector3 temp;
    temp.x = a.y * b.z - a.z * b.y;
    temp.y = a.z * b.x - a.x * b.z;
    temp.z = a.x * b.y - a.y * b.x;
    return temp;
  }
};

struct Ray
{
  Vector3 origin;
  Vector3 direction;
  
  Ray()
  {}
  
  Ray(Vector3 orig, Vector3 dir)
  :origin(orig)
  {
    dir.Normalize();
    direction = dir;
  }
  
  Ray(Vector3 dir)
  : origin(Vector3())
  {
    dir.Normalize();
    direction = dir;
  }
};

typedef struct _Triangle
{
  struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  double position[3];
  double color[3];
} Light;

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles=0;
int num_spheres=0;
int num_lights=0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

double deg2rad(const float &deg)
{
  return deg * M_PI / 180.0;
}

bool nearZero(double val)
{
  double epsilon = 0.0000001;
  if (fabs(val) <= epsilon)
  {
    return true;
  }
  return false;
}

// lerp from -1 to 1
double lerpNeg1to1(double val, double max)
{
  return -1.0+(1.0-(-1.0))*(val/max);
}

Ray getRay(double x, double y)
{
  double windowX = aspect*tan(deg2rad(FOV/2.0))*lerpNeg1to1(x, WIDTH);
  double windowY = tan(deg2rad(FOV/2.0))*lerpNeg1to1(y, HEIGHT);
  Ray r = Ray(Vector3(windowX, windowY, -1));
  return r;
}

Vector3 CollideTriangle(Ray r, Triangle tri, Vector3* bary, double max_t)
{
  double u, v, t;
  Vector3 invalid(0, 0, SMALL_Z);
  // calculate normal N
  Vector3 v0v1 = Vector3(tri.v[1]) - Vector3(tri.v[0]);
  Vector3 v0v2 = Vector3(tri.v[2]) - Vector3(tri.v[0]);
  Vector3 normal = Vector3::Cross(v0v1, v0v2);
  double area2 = normal.Length();
  normal.Normalize();
  // ray & triangle parallel: see if N dot R is 0 before computing t
  double NdotR = Vector3::Dot(normal, r.direction);
  if(nearZero(NdotR))
    return invalid;
  // calculate D
  double d = Vector3::Dot(normal, Vector3(tri.v[0]));
  //calculate t
  // t = (d - Vector3::Dot(normal, r.origin)) / NdotR;
  t = (Vector3::Dot(normal, Vector3(tri.v[0])-r.origin))/NdotR;
  // max_t: used in intersection point to light source case;
  // if an intersection is beyond the light source, then it's
  // actually not an intersection (no shadow)
  if(max_t!=-1 && t>max_t)
    return invalid;
  // check if triangle behind ray
  if(t<=0)
    return invalid;
  // calculate intersection point P
  Vector3 p = r.origin + t*r.direction;
  // P inside or outside triangle?

  // C - vector perpendicular to triangle's plane
  Vector3 c;
  // edge 0
  Vector3 edge0 = Vector3(tri.v[1]) - Vector3(tri.v[0]);
  Vector3 vp0 = p - Vector3(tri.v[0]);
  c = Vector3::Cross(edge0, vp0);
  if(Vector3::Dot(normal, c)<0)
    return invalid;
  // edge 1
  Vector3 edge1 = Vector3(tri.v[2]) - Vector3(tri.v[1]);
  Vector3 vp1 = p - Vector3(tri.v[1]);
  c = Vector3::Cross(edge1, vp1);
  u = c.Length() / area2;
  if(Vector3::Dot(normal, c)<0)
    return invalid;
  // edge 2
  Vector3 edge2 = Vector3(tri.v[0]) - Vector3(tri.v[2]);
  Vector3 vp2 = p - Vector3(tri.v[2]);
  c = Vector3::Cross(edge2, vp2);
  v = c.Length() / area2;
  if(Vector3::Dot(normal, c)<0)
    return invalid;
  Vector3 v0 = Vector3(tri.v[0]);
  Vector3 v1 = Vector3(tri.v[1]);
  Vector3 v2 = Vector3(tri.v[2]);
  *bary = Vector3(u, v, 1.0-v-u);
  return p;
}

bool checkAllTriangles(Ray r, int* indexColl, Vector3* coll, Vector3* bary, double max_t = -1)
{
  bool found = false;
  *coll = Vector3(0, 0, SMALL_Z);
  Vector3 tempP;
  Vector3 tempBary;
  for(int i=0; i<num_triangles; i++){
    tempP = CollideTriangle(r, triangles[i], &tempBary, max_t);
    if(tempP.z!=SMALL_Z && tempP.z>coll->z){
      *indexColl = i;
      *coll = tempP;
      *bary = tempBary;
      found = true;
    }
  }
  
  if(found){
    return true;
  }
  return false;
}

double CollideSphere(Ray r, Sphere s)
{
  // solve for equation a*t^2 + b*t + c = 0
  double a = 1;
  double b = 2*(r.direction.x*(r.origin.x - s.position[0])
                + r.direction.y*(r.origin.y - s.position[1])
                + r.direction.z*(r.origin.z - s.position[2]));
  double c = pow(r.origin.x - s.position[0], 2)
              +pow(r.origin.y - s.position[1], 2)
              +pow(r.origin.z - s.position[2], 2) - pow(s.radius, 2);
  
  double t0 = (-1.0*b + sqrt(pow(b,2)-4.0*a*c))/2.0;
  double t1 = (-1.0*b - sqrt(pow(b,2)-4.0*a*c))/2.0;
  if(t0>0 && t1>0){
    return (fmin(t0, t1));
  }
  return -1;
}

bool checkAllSpheres(Ray r, int* indexColl, Vector3* coll)
{
  double tmin = DBL_MAX;
  int index = -1;
  bool found = false;
  double t;
  for(int i=0; i<num_spheres; i++){
    t = CollideSphere(r, spheres[i]);
    if(t != -1 && t < tmin){
      index = i;
      tmin = t;
      found = true;
    }
  }
  
  if(found){
    *indexColl = index;
    *coll = r.origin+r.direction*tmin;
    return true;
  }
  return false;
}

Vector3 reflect(Vector3 vec, Vector3 n)
{
  double projectedLen = Vector3::Dot(vec, n);
  Vector3 projectedVec = n*projectedLen;
  return 2*projectedVec-vec;
}

Vector3 clamp(Vector3 vec)
{
  if(vec.x > 1)
    vec.x = 1;
  if(vec.x < 0)
    vec.x = 0;
  if(vec.y > 1)
    vec.y = 1;
  if(vec.y < 0)
    vec.y = 0;
  if(vec.z > 1)
    vec.z = 1;
  if(vec.z < 0)
    vec.z = 0;
  return vec;
}

Vector3 determineColor(Ray r)
{
  Vector3 coll;
  int indexColl = -1;
  Vector3 bary;
  bool triangleColl = checkAllTriangles(r, &indexColl, &coll, &bary);
  bool sphereColl = checkAllSpheres(r, &indexColl, &coll);
  
  // no collision
  if(indexColl == -1){
    return Vector3(1, 1, 1);
  }
  // has collision, calculate normal and light coefficients
  Vector3 normal, dif, spe;
  double shi;
  // sphere collision
  if(sphereColl){
    normal = (coll - spheres[indexColl].position);
    normal.Normalize();
    dif = Vector3(spheres[indexColl].color_diffuse);
    spe = Vector3(spheres[indexColl].color_specular);
    shi = spheres[indexColl].shininess;
  }
  // triangle collision
  else{
    // interpolate normal using barycentric coordinates
    double interNormalX = triangles[indexColl].v[0].normal[0]*bary.x+
                          triangles[indexColl].v[1].normal[0]*bary.y+
                          triangles[indexColl].v[2].normal[0]*bary.z;
    double interNormalY = triangles[indexColl].v[0].normal[1]*bary.x+
                          triangles[indexColl].v[1].normal[1]*bary.y+
                          triangles[indexColl].v[2].normal[1]*bary.z;
    double interNormalZ = triangles[indexColl].v[0].normal[2]*bary.x+
                          triangles[indexColl].v[1].normal[2]*bary.y+
                          triangles[indexColl].v[2].normal[2]*bary.z;
    normal = Vector3(interNormalX, interNormalY, interNormalZ);
    normal.Normalize();
    
    // interpolate diffuse light
    double interDifX = triangles[indexColl].v[0].color_diffuse[0]*bary.x+
                          triangles[indexColl].v[1].color_diffuse[0]*bary.y+
                          triangles[indexColl].v[2].color_diffuse[0]*bary.z;
    double interDifY = triangles[indexColl].v[0].color_diffuse[1]*bary.x+
                          triangles[indexColl].v[1].color_diffuse[1]*bary.y+
                          triangles[indexColl].v[2].color_diffuse[1]*bary.z;
    double interDifZ = triangles[indexColl].v[0].color_diffuse[2]*bary.x+
                          triangles[indexColl].v[1].color_diffuse[2]*bary.y+
                          triangles[indexColl].v[2].color_diffuse[2]*bary.z;
    dif = Vector3(interDifX, interDifY, interDifZ);
    
    // interpolate specular light
    double interSpeX = triangles[indexColl].v[0].color_specular[0]*bary.x+
                          triangles[indexColl].v[1].color_specular[0]*bary.y+
                          triangles[indexColl].v[2].color_specular[0]*bary.z;
    double interSpeY = triangles[indexColl].v[0].color_specular[1]*bary.x+
                          triangles[indexColl].v[1].color_specular[1]*bary.y+
                          triangles[indexColl].v[2].color_specular[1]*bary.z;
    double interSpeZ = triangles[indexColl].v[0].color_specular[2]*bary.x+
                          triangles[indexColl].v[1].color_specular[2]*bary.y+
                          triangles[indexColl].v[2].color_specular[2]*bary.z;
    spe = Vector3(interSpeX, interSpeY, interSpeZ);
    
    // interpolate shininess
    shi = triangles[indexColl].v[0].shininess*bary.x+
          triangles[indexColl].v[1].shininess*bary.y+
          triangles[indexColl].v[2].shininess*bary.z;
  }
  Vector3 color(0, 0, 0);
  // for each light, check if blocked
  for(int i=0; i<num_lights; i++){
    // ray from intersection point to light
    Vector3 vecToLight = Vector3(lights[i].position)-coll;
    Ray toLight = Ray(coll+(normal*0.001), vecToLight);
    int indexCollToLight = -1;
    Vector3 collToLight, baryToLight;
    bool triCollToLight = checkAllTriangles(toLight, &indexCollToLight, &collToLight, &baryToLight, vecToLight.Length());
    bool sphCollToLight = checkAllSpheres(toLight, &indexCollToLight, &collToLight);
    
    // light not blocked
    if(!triCollToLight && !sphCollToLight){
      // check L dot N and clamp to 0 if needed
      Vector3 L = Vector3(lights[i].position)-coll;
      L.Normalize();
      double LdotN = Vector3::Dot(L, normal);
      if(LdotN < 0)
        LdotN = 0;
      // check R dot V and clamp to 0 if needed
      // R: L reflected by normal
      // V: vector intersection to camera
      Vector3 R = reflect(L, normal);
      // Vector3 V = Vector3(0, 0, 0)-coll;
      Vector3 V = r.origin-coll;
      R.Normalize();
      V.Normalize();
      double RdotV = Vector3::Dot(R, V);
      if(RdotV < 0)
        RdotV = 0;
      // determine color
      color += Vector3(lights[i].color)*(dif*LdotN + spe*pow((RdotV), shi));
    }
  }
  color += Vector3(ambient_light);
  color = clamp(color);
  // printf("illu: %f, %f, %f\n", color.x, color.y, color.z);
  return color;
}

Vector3 times255(Vector3 vec)
{
  vec.x *= 255;
  vec.y *= 255;
  vec.z *= 255;
  return vec;
}

//MODIFY THIS FUNCTION
void draw_scene()
{
  unsigned int x,y;

  for(x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    Vector3 colorRGB = Vector3();
    glBegin(GL_POINTS);
    for(y=0;y < HEIGHT;y++){
      Ray r = getRay(x, y);
      Vector3 color = times255(determineColor(r));
      plot_pixel(x, y, color.x, color.y, color.z);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n");
}

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}

void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  Pic *in = NULL;

  in = pic_alloc(640, 480, 3, NULL);
  printf("Saving JPEG file: %s\n", filename);

  memcpy(in->pix,buffer,3*WIDTH*HEIGHT);
  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);      

}

void parse_check(char *expected,char *found)
{
  if(strcasecmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}

void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i",&number_of_objects);

  printf("number of objects: %i\n",number_of_objects);
  char str[200];

  parse_doubles(file,"amb:",ambient_light);

  for(i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
      if(strcasecmp(type,"triangle")==0)
	{

	  printf("found triangle\n");
	  int j;

	  for(j=0;j < 3;j++)
	    {
	      parse_doubles(file,"pos:",t.v[j].position);
	      parse_doubles(file,"nor:",t.v[j].normal);
	      parse_doubles(file,"dif:",t.v[j].color_diffuse);
	      parse_doubles(file,"spe:",t.v[j].color_specular);
	      parse_shi(file,&t.v[j].shininess);
	    }

	  if(num_triangles == MAX_TRIANGLES)
	    {
	      printf("too many triangles, you should increase MAX_TRIANGLES!\n");
	      exit(0);
	    }
	  triangles[num_triangles++] = t;
	}
      else if(strcasecmp(type,"sphere")==0)
	{
	  printf("found sphere\n");

	  parse_doubles(file,"pos:",s.position);
	  parse_rad(file,&s.radius);
	  parse_doubles(file,"dif:",s.color_diffuse);
	  parse_doubles(file,"spe:",s.color_specular);
	  parse_shi(file,&s.shininess);

	  if(num_spheres == MAX_SPHERES)
	    {
	      printf("too many spheres, you should increase MAX_SPHERES!\n");
	      exit(0);
	    }
	  spheres[num_spheres++] = s;
	}
      else if(strcasecmp(type,"light")==0)
	{
	  printf("found light\n");
	  parse_doubles(file,"pos:",l.position);
	  parse_doubles(file,"col:",l.color);

	  if(num_lights == MAX_LIGHTS)
	    {
	      printf("too many lights, you should increase MAX_LIGHTS!\n");
	      exit(0);
	    }
	  lights[num_lights++] = l;
	}
      else
	{
	  printf("unknown type in scene description:\n%s\n",type);
	  exit(0);
	}
    }
  return 0;
}

void display()
{

}

void init()
{
  glMatrixMode(GL_PROJECTION);
  
//  gluPerspective(FOV, 1.0 * aspect, DIST_NEAR, 1000.0);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
      draw_scene();
      if(mode == MODE_JPEG)
	save_jpg();
    }
  once=1;
}

int main (int argc, char ** argv)
{
  if (argc<2 || argc > 3)
  {  
    printf ("usage: %s <scenefile> [jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
    {
      mode = MODE_JPEG;
      filename = argv[2];
    }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
