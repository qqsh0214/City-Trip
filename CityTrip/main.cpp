#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include "freeglut\freeglut.h"
#include<GLFW/glfw3.h>
#include<math.h>
#include<time.h>
#include<iostream>
#include<windows.h>

#include "Mesh.h"
#include <assimp/DefaultLogger.hpp>
#include <stdlib.h>


#include <GL/glaux.h>
#pragma comment(lib, "legacy_stdio_definitions.lib")

using namespace std;

const unsigned int MAP_WIDTH = 1024;
const unsigned int CELL_WIDTH = 16;
const unsigned int MAP = MAP_WIDTH * CELL_WIDTH / 2;

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

#define BITMAP_ID 0x4D42    /**< 位图文件的标志 */

/** 位图载入类 */
class CBMPLoader
{
   public:
      CBMPLoader();
      ~CBMPLoader();

      bool LoadBitmap(char *filename); /**< 装载一个bmp文件 */
      void FreeImage();                /**< 释放图像数据 */

      unsigned int ID;                 /**< 生成纹理的ID号 */
      int imageWidth;                  /**< 图像宽度 */
      int imageHeight;                 /**< 图像高度 */
      unsigned char *image;            /**< 指向图像数据的指针 */
};

/** 构造函数 */
CBMPLoader::CBMPLoader()
{
   /** 初始化成员值为0 */
    image = 0;
    imageWidth = 0;
    imageHeight = 0;
}

/** 析构函数 */
CBMPLoader::~CBMPLoader()
{
   FreeImage(); /**< 释放图像数据占据的内存 */
}

/** 装载一个位图文件 */
bool CBMPLoader::LoadBitmap(char *file)
{
    FILE *pFile = 0; /**< 文件指针 */

    /** 创建位图文件信息和位图文件头结构 */
    BITMAPINFOHEADER bitmapInfoHeader;
    BITMAPFILEHEADER header;

    unsigned char textureColors = 0;/**< 用于将图像颜色从BGR变换到RGB */

   /** 打开文件,并检查错误 */
    pFile = fopen(file, "rb");
        if(pFile == 0) return false;

    /** 读入位图文件头信息 */
    fread(&header, sizeof(BITMAPFILEHEADER), 1, pFile);

    /** 检查该文件是否为位图文件 */
    if(header.bfType != BITMAP_ID)
       {
           fclose(pFile);             /**< 若不是位图文件,则关闭文件并返回 */
           return false;
       }

    /** 读入位图文件信息 */
    fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, pFile);

    /** 保存图像的宽度和高度 */
    imageWidth = bitmapInfoHeader.biWidth;
    imageHeight = bitmapInfoHeader.biHeight;

    /** 确保读取数据的大小 */
   if(bitmapInfoHeader.biSizeImage == 0)
      bitmapInfoHeader.biSizeImage = bitmapInfoHeader.biWidth *
      bitmapInfoHeader.biHeight * 3;

    /** 将指针移到数据开始位置 */
    fseek(pFile, header.bfOffBits, SEEK_SET);

    /** 分配内存 */
    image = new unsigned char[bitmapInfoHeader.biSizeImage];

    /** 检查内存分配是否成功 */
    if(!image)                        /**< 若分配内存失败则返回 */
       {
           delete[] image;
           fclose(pFile);
           return false;
       }

    /** 读取图像数据 */
    fread(image, 1, bitmapInfoHeader.biSizeImage, pFile);

    /** 将图像颜色数据格式进行交换,由BGR转换为RGB */
    for(int index = 0; index < (int)bitmapInfoHeader.biSizeImage; index+=3)
       {
           textureColors = image[index];
           image[index] = image[index + 2];
           image[index + 2] = textureColors;
       }

    fclose(pFile);       /**< 关闭文件 */
    return true;         /**< 成功返回 */
}

/** 释放内存 */
void CBMPLoader::FreeImage()
{
   /** 释放分配的内存 */
   if(image)
      {
         delete[] image;
         image = 0;
      }
}

#define GL_CLAMP_TO_EDGE    0x812F

/** 天空盒类 */
class CSkyBox
{
public:
    /** 构造函数 */
    CSkyBox();
    ~CSkyBox();

    /** 初始化 */
    bool Init();

    /** 渲染天空 */
    void  CreateSkyBox(float x, float y, float z,
                       float width, float height,
                       float length);

private:
    CBMPLoader  m_texture[6];   /**< 天空盒纹理 */

};

CSkyBox::CSkyBox()
{
}

CSkyBox::~CSkyBox()
{
    /** 删除纹理对象及其占用的内存 */
    for(int i =0 ;i< 6; i++)
    {
        m_texture[i].FreeImage();
        glDeleteTextures(1,&m_texture[i].ID);
    }

}

/** 天空盒初始化 */
bool CSkyBox::Init()
{
    char filename[128] ;                                         /**< 用来保存文件名 */
    char *bmpName[] = { "back","front","bottom","top","left","right"};
    for(int i=0; i< 6; i++)
    {
        sprintf(filename,"skybox/%s",bmpName[i]);
        strcat(filename,".bmp");
        if(!m_texture[i].LoadBitmap(filename))                     /**< 载入位图文件 */
        {
            exit(0);
        }
        glGenTextures(1, &m_texture[i].ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, m_texture[i].ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, m_texture[i].imageWidth,
                        m_texture[i].imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        m_texture[i].image);
    }
    return true;

}

/** 构造天空盒 */
void  CSkyBox::CreateSkyBox(float x, float y, float z,
                            float box_width, float box_height,
                            float box_length)
{
    /** 获得场景中光照状态 */
    GLboolean lp;
    glGetBooleanv(GL_LIGHTING,&lp);

    /** 计算天空盒长 宽 高 */
    float width = MAP * box_width/8;
    float height = MAP * box_height/8;
    float length = MAP * box_length/8;

    /** 计算天空盒中心位置 */
    x = x+ MAP/8 - width  / 2;
    y = y+ MAP/24 - height / 2;
    z = z+ MAP/8 - length / 2;

    glDisable(GL_LIGHTING);            /**< 关闭光照 */

    /** 开始绘制 */
    glPushMatrix();
    glTranslatef(-x,-y,-z);

    /** 绘制背面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[0].ID);

    glBegin(GL_QUADS);

        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10,5,15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10,-5,15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10,-5,15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10,5,15);

    glEnd();

    /** 绘制前面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[1].ID);

    glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10,5,-15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10,-5,-15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10,-5,-15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10,5,-15);

    glEnd();

    /** 绘制底面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[2].ID);

    glBegin(GL_QUADS);

        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10,-5,-15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10,-5,15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10,-5,15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10,-5,-15);

    glEnd();

    /** 绘制顶面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[3].ID);

    glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10,5,-15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10,5,15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10,5,15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10,5,-15);

    glEnd();

    /** 绘制左面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[4].ID);

    glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10,5,-15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10,-5,-15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-10,-5,15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-10,5,15);

    glEnd();

    /** 绘制右面 */
    glBindTexture(GL_TEXTURE_2D, m_texture[5].ID);

    glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(10,5,-15);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(10,-5,-15);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10,-5,15);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10,5,15);

    glEnd();


    glPopMatrix();                 /** 绘制结束 */

    if(lp)                         /** 恢复光照状态 */
        glEnable(GL_LIGHTING);

}

class Camera {
public:
    Camera() :pleft(-1.34f),pright(1.34f),
    ptop(1.0f),pbottom(-1.0f),pnear(0.01f),pfar(2),
    cameraPosX(0),cameraPosY(0),cameraPosZ(0),
    cameraFrontX(0),cameraFrontY(0),cameraFrontZ(1),
    cameraRightX(1.0f),cameraRightY(0),cameraRightZ(0),
    cameraUpX(0),cameraUpY(-1.0f),cameraUpZ(0)
    {} ;

    void moveForward(GLfloat const distance);
    void moveBack(GLfloat const distance);
    void moveRight(GLfloat const distance);
    void moveLeft(GLfloat const distance);
    void moveTop(GLfloat const distance);
    void moveBottom(GLfloat const distance);
    void rotate(GLfloat const pitch,GLfloat const yaw);
    void setCamera(GLfloat camPosX,GLfloat camPosY,GLfloat camPosZ);

    float getCamPosX() {
        return cameraPosX;
    }
    float getCamPosY() {
        return cameraPosY;
    }
    float getCamPosZ() {
        return cameraPosZ;
    }
    float getCamFrontX() {
        return cameraFrontX;
    }
    float getCamFrontY() {
        return cameraFrontY;
    }
    float getCamFrontZ() {
        return cameraFrontZ;
    }
    float getCamUpX() {
        return cameraUpX;
    }
    float getCamUpY() {
        return cameraUpY;
    }
    float getCamUpZ() {
        return cameraUpZ;
    }

private:
    GLfloat pleft,pright,ptop,pbottom,pnear,pfar;
    GLfloat cameraPosX,cameraPosY,cameraPosZ;
    GLfloat cameraFrontX,cameraFrontY,cameraFrontZ;
    GLfloat cameraRightX,cameraRightY,cameraRightZ;
    GLfloat cameraUpX,cameraUpY,cameraUpZ;
};

void Camera::setCamera(GLfloat camPosX,GLfloat camPosY,GLfloat camPosZ) {
    cameraPosX = camPosX;
    cameraPosY = camPosY;
    cameraPosZ = camPosZ;
}
void Camera::moveTop(GLfloat const distance) {
    cameraPosX += cameraUpX*distance;
    cameraPosY += cameraUpY*distance;
    cameraPosZ += cameraUpZ*distance;

    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

    cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " <<cameraPosZ << endl;
}

void Camera::moveBottom(GLfloat const distance) {
    cameraPosX -= cameraUpX*distance;
    cameraPosY -= cameraUpY*distance;
    cameraPosZ -= cameraUpZ*distance;

    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

	cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " << cameraPosZ << endl;
}

void Camera::moveForward(GLfloat const distance) {
    cameraPosX += cameraFrontX*distance;
    cameraPosY += cameraFrontY*distance;
    cameraPosZ += cameraFrontZ*distance;

    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

	cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " << cameraPosZ << endl;
}

void Camera::moveBack(GLfloat const distance) {
    cameraPosX -= cameraFrontX*distance;
    cameraPosY -= cameraFrontY*distance;
    cameraPosZ -= cameraFrontZ*distance;


    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

	cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " << cameraPosZ << endl;
}

void Camera::moveRight(GLfloat const distance) {
    cameraPosX += cameraRightX*distance;
    cameraPosY += cameraRightY*distance;
    cameraPosZ += cameraRightZ*distance;


    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

	cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " << cameraPosZ << endl;
}

void Camera::moveLeft(GLfloat const distance) {
    cameraPosX -= cameraRightX*distance;
    cameraPosY -= cameraRightY*distance;
    cameraPosZ -= cameraRightZ*distance;


    gluLookAt(cameraPosX,cameraPosY,cameraPosZ,
              cameraPosX+cameraFrontX,cameraPosY+cameraFrontY,cameraPosZ+cameraFrontZ,
              cameraUpX,cameraUpY,cameraUpZ);

	cout << "X : " << cameraPosX << " Y : " << cameraPosY << " Z : " << cameraPosZ << endl;
}

/*pitch can adapt up and down , yaw adapt left and right*/
void Camera::rotate(GLfloat const pitch, GLfloat const yaw) {
    float angleOfPitch = -1*pitch/480;
    float angleOfYaw = yaw/640;
    float a,b,c;

    /*rotate around Up Vector*/
    float cosPitch = cos(angleOfPitch);
    float sinPitch = sin(angleOfPitch);
    float minusCosPitch = -1*cosPitch;
    float minusSinPitch = -1*sinPitch;
    float cosYaw = cos(angleOfYaw);
    float sinYaw = sin(angleOfYaw);
    float minusCosYaw = -1*cosYaw;
    float minusSinYaw = -1*sinYaw;
    float X;
    float Y;
    float Z;

    a = cameraUpX;
    b = cameraUpY;
    c = cameraUpZ;

    X = cameraFrontX;
    Y = cameraFrontY;
    Z = cameraFrontZ;
    cameraFrontX = (a*a+(1-a*a)*cosYaw)*X + (a*b*(1-cosYaw)+c*sinYaw)*Y+(a*c*(1-cosYaw)-b*sinYaw)*Z;
    cameraFrontY = (a*b*(1-cosYaw)-c*sinYaw)*X+(b*b+(1-b*b)*cosYaw)*Y+(b*c*(1-cosYaw)+a*sinYaw)*Z;
    cameraFrontZ = (a*c*(1-cosYaw)+b*sinYaw)*X+(b*c*(1-cosYaw)-a*sinYaw)*Y+(c*c+(1-c*c)*cosYaw)*Z;

    X = cameraRightX;
    Y = cameraRightY;
    Z = cameraRightZ;
    cameraRightX = (a*a+(1-a*a)*cosYaw)*X + (a*b*(1-cosYaw)+c*sinYaw)*Y+(a*c*(1-cosYaw)-b*sinYaw)*Z;
    cameraRightY = (a*b*(1-cosYaw)-c*sinYaw)*X+(b*b+(1-b*b)*cosYaw)*Y+(b*c*(1-cosYaw)+a*sinYaw)*Z;
    cameraRightZ = (a*c*(1-cosYaw)+b*sinYaw)*X+(b*c*(1-cosYaw)-a*sinYaw)*Y+(c*c+(1-c*c)*cosYaw)*Z;

    a = cameraRightX;
    b = cameraRightY;
    c = cameraRightZ;

    X = cameraFrontX;
    Y = cameraFrontY;
    Z = cameraFrontZ;
    cameraFrontX = (a*a+(1-a*a)*cosPitch)*X + (a*b*(1-cosPitch)+c*sinPitch)*Y+(a*c*(1-cosPitch)-b*sinPitch)*Z;
    cameraFrontY = (a*b*(1-cosPitch)-c*sinPitch)*X+(b*b+(1-b*b)*cosPitch)*Y+(b*c*(1-cosPitch)+a*sinPitch)*Z;
    cameraFrontZ = (a*c*(1-cosPitch)+b*sinPitch)*X+(b*c*(1-cosPitch)-a*sinPitch)*Y+(c*c+(1-c*c)*cosPitch)*Z;

    X = cameraUpX;
    Y = cameraUpY;
    Z = cameraUpZ;
    cameraUpX = (a*a+(1-a*a)*cosPitch)*X + (a*b*(1-cosPitch)+c*sinPitch)*Y+(a*c*(1-cosPitch)-b*sinPitch)*Z;
    cameraUpY = (a*b*(1-cosPitch)-c*sinPitch)*X+(b*b+(1-b*b)*cosPitch)*Y+(b*c*(1-cosPitch)+a*sinPitch)*Z;
    cameraUpZ = (a*c*(1-cosPitch)+b*sinPitch)*X+(b*c*(1-cosPitch)-a*sinPitch)*Y+(c*c+(1-c*c)*cosPitch)*Z;
}


/*****************************粒子系统******************************************/
#define MAX_PARTICLES 1000

typedef struct//structrue for particle
{
	bool active;//active or not
	float life;//last time
	float fade;//describe the decreasing of life
	float r, g, b;//color
	float x, y, z;//the position
	float xi, yi, zi;//what direction to move
	float xg, yg, zg;//gravity
}particles;

bool rainbow = true;
bool sp, rp;//space button and return button pressed or not?
float slowdown = 2.0f;
float xspeed, yspeed;
float zoom = -10.0f;
bool gkeys[256];

GLuint loop, col, delay, texture[1];

particles paticle[MAX_PARTICLES];

static GLfloat colors[12][3] =     // Rainbow Of Colors 
{
	{ 1.0f,0.5f,0.5f },{ 1.0f,0.75f,0.5f },{ 1.0f,1.0f,0.5f },{ 0.75f,1.0f,0.5f },
	{ 0.5f,1.0f,0.5f },{ 0.5f,1.0f,0.75f },{ 0.5f,1.0f,1.0f },{ 0.5f,0.75f,1.0f },
	{ 0.5f,0.5f,1.0f },{ 0.75f,0.5f,1.0f },{ 1.0f,0.5f,1.0f },{ 1.0f,0.5f,0.75f }
};


AUX_RGBImageRec *LoadBMP(char *Filename) // Loads A Bitmap Image 
{
	FILE *File = NULL;        // File Handle 
	if (!Filename)        // Make Sure A Filename Was Given 
	{
		return NULL;        // If Not Return NULL 
	}
	File = fopen(Filename, "r");      // Check To See If The File Exists 
	if (File)           // Does The File Exist? 
	{
		fclose(File);         // Close The Handle 
		return auxDIBImageLoadA(Filename);   // Load The Bitmap And Return A Pointer 
	}
	return NULL;          // If Load Failed Return NULL 
}

int LoadGLTextures()        // Load Bitmaps And Convert To Textures 
{
	int Status = FALSE;        // Status Indicator 
	AUX_RGBImageRec *TextureImage[1];    // Create Storage Space For The Texture 

	memset(TextureImage, 0, sizeof(void *) * 1); // Set The Pointer To NULL 
	if (TextureImage[0] = LoadBMP("assets/timg.bmp"))  // Load Particle Texture 
	{
		Status = TRUE;          // Set The Status To TRUE 
		glGenTextures(1, &texture[0]);    // Create One Textures 

		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3,
			TextureImage[0]->sizeX, TextureImage[0]->sizeY,
			0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);//gamma矫正
	}
	if (TextureImage[0])        // If Texture Exists 
	{
		if (TextureImage[0]->data)      // If Texture Image Exists 
		{
			free(TextureImage[0]->data);    // Free The Texture Image Memory 
		}
		free(TextureImage[0]);       // Free The Image Structure 
	}
	return Status;          // Return The Status 
}
/***************************例子系统结束******************************************/


/****************************************以下是添加的加载模型的代码*****************************************/
Mesh mesh("./city.obj");

HGLRC		hRC = NULL;			// Permanent Rendering Context
HDC			hDC = NULL;			// Private GDI Device Context
HWND		hWnd = NULL;			// Holds Window Handle
HINSTANCE	hInstance;	// Holds The Instance Of The Application

bool		keys[256];			// Array used for Keyboard Routine;
bool		active = TRUE;		// Window Active Flag Set To TRUE by Default
bool		fullscreen = TRUE;	// Fullscreen Flag Set To Fullscreen By Default

GLfloat		xrot;
GLfloat		yrot;
GLfloat		zrot;
GLfloat     yr = -7.0f;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[] = { 0.0f, 0.0f, 15.0f, 1.0f };

void ReSizeGLScene(GLsizei width, GLsizei height)				// Resize And Initialize The GL Window
{
	if (height == 0)								// Prevent A Divide By Zero By
	{
		height = 1;							// Making Height Equal One
	}

	glViewport(0, 0, width, height);					// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

												// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix
}


//display text显示列表
#define MAX_CHAR        128  

void selectFont(int size, int charset, const char* face)
{
	HFONT hFont = CreateFontA(size, 0, 0, 0, FW_MEDIUM, 0, 0, 0,
		charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
	HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), hFont);
	DeleteObject(hOldFont);
}

void drawString(const char* str)
{
	static int isFirstCall = 1;
	static GLuint lists;

	if (isFirstCall)
	{ // 如果是第一次调用，执行初始化  
	  // 为每一个ASCII字符产生一个显示列表  
		isFirstCall = 0;

		// 申请MAX_CHAR个连续的显示列表编号  
		lists = glGenLists(MAX_CHAR);


		// 把每个字符的绘制命令都装到对应的显示列表中  
		wglUseFontBitmaps(wglGetCurrentDC(), 0, MAX_CHAR, lists);
	}
	// 调用每个字符对应的显示列表，绘制每个字符 

	glPushAttrib(GL_LIST_BIT);
	for (; *str != '\0'; ++str)
	{
		glCallList(lists + *str);
	}
	glPopAttrib();
}


void DrawGLScene()				//Here's where we do all the drawing
{
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();				// Reset MV Matrix


	glTranslatef(0.0f, -1.0f, -20.0f);	// Move 40 Units And Into The Screen

	glRotatef(185, 1.0f, 0.0f, 0.0f);
	glRotatef(yrot, 0.0f, 1.0f, 0.0f);
	glRotatef(zrot, 0.0f, 0.0f, 1.0f);

	glScaled(0.01, 0.01, 0.01);

	//mesh.Render(0.5f);

	//xrot+=0.3f;
	//yrot += 0.2f;
	//zrot+=0.4f;

	//glutSwapBuffers();

}
/***********************************************************************************************************/

int main(void)
{
    Camera* cam = new Camera();

    double* lastCursorX =(double*)malloc(sizeof(double));
    double* lastCursorY =(double*)malloc(sizeof(double));
    double* currentCursorX =(double*)malloc(sizeof(double));
    double* currentCursorY =(double*)malloc(sizeof(double));
    double tempX;
    double tempY;
    bool IsFirstFrame = true;

    GLFWwindow* window;

    bool lookAtIsSetted = false;

    glfwSetErrorCallback(error_callback);

 
    if (!glfwInit())
        exit(EXIT_FAILURE);

 
    window = glfwCreateWindow(640, 480, "City Trip", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

	/***********************添加加载模型代码***********************/
	//mesh.Init("./city.obj");
	//ReSizeGLScene(640, 480);
	/*****************************************************************/
    glfwMakeContextCurrent(window);

	/***********************添加加载模型代码***********************/
	if (0 != mesh.loadasset()) {
		return -1;
	}
	bool load = mesh.LoadGLTextures();
	


    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_TEXTURE_2D);

	

    //cam->setCamera(0.0f,-4.f,-14.f);
	cam->setCamera(0.0f,-4.f,-10.f);


	/*********************粒子系统*************************/
	for (loop = 0; loop < MAX_PARTICLES; ++loop)
	{
		paticle[loop].active = true;
		paticle[loop].life = 1.0f;//full life is 1.0f
								  //life decrease rate(a random value add 0.003f : never equals 0)
		paticle[loop].fade = float(rand() % 100) / 1000.0f + 0.003f;
		paticle[loop].r = colors[loop / 100][0];
		paticle[loop].g = colors[loop / 100][1];
		paticle[loop].b = colors[loop / 100][2];

		paticle[loop].xi = float((rand() % 50) - 26.0f) * 10.0f;
		paticle[loop].yi = float((rand() % 50) - 25.0f) * 10.0f;
		paticle[loop].zi = float((rand() % 50) - 25.0f) * 10.0f;

		paticle[loop].xg = 0.0f;
		paticle[loop].yg = -0.8f;
		paticle[loop].zg = 0.8f;
	}
	/****************************************************/


	
    while (!glfwWindowShouldClose(window))
    {

  
        float ratio;
        int width,height;
        lookAtIsSetted = false;

  
        glfwGetFramebufferSize(window,&width,&height);
        ratio = width/(float) height;
   
        glViewport(0,0,width,height);
        glClear(GL_COLOR_BUFFER_BIT);

   
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        gluPerspective(60,0.75,2.0f,100.f);

        if (glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS) {
            cam->moveForward(1.f);
            lookAtIsSetted = true;
        } else if (glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS) {
            cam->moveBack(1.f);
            lookAtIsSetted = true;
        } else if (glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS) {
            cam->moveLeft(1.f);
            lookAtIsSetted = true;
        } else if (glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS) {
            cam->moveRight(1.f);
            lookAtIsSetted = true;
        } else if (glfwGetKey(window,GLFW_KEY_Q) == GLFW_PRESS) {
            cam->moveTop(1.f);
            lookAtIsSetted = true;
        } else if (glfwGetKey(window,GLFW_KEY_E) == GLFW_PRESS) {
            cam->moveBottom(1.f);
            lookAtIsSetted = true;
        }
        if (IsFirstFrame) {
            IsFirstFrame = false;
            glfwGetCursorPos(window,lastCursorX,lastCursorY);
        }
        glfwGetCursorPos(window,currentCursorX,currentCursorY);
        cam->rotate(*lastCursorY-*currentCursorY,*currentCursorX-*lastCursorX);
        *lastCursorX = *currentCursorX;
        *lastCursorY = *currentCursorY;


        if (!lookAtIsSetted) {
            gluLookAt(cam->getCamPosX(),cam->getCamPosY(),cam->getCamPosZ(),
                      cam->getCamPosX()+cam->getCamFrontX(),cam->getCamPosY()+cam->getCamFrontY(),cam->getCamPosZ()+cam->getCamFrontZ(),
                      cam->getCamUpX(),cam->getCamUpY(),cam->getCamUpZ());
        }



        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

		/**/
		glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glEnable(GL_LIGHTING); //在后面的渲染中使用光照
		glEnable(GL_NORMALIZE);

        GLfloat sun_light_position[] = {0.0f, -12.0f,-4.0f, 1.0f};
        GLfloat sun_light_ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
        GLfloat sun_light_diffuse[]  = {0.8f, 0.8f, 0.8f, 1.0f};
        GLfloat sun_light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};


        glLightfv(GL_LIGHT0, GL_POSITION, sun_light_position); //指定第0号光源的位置
        glLightfv(GL_LIGHT0, GL_AMBIENT,  sun_light_ambient); //GL_AMBIENT表示各种光线照射到该材质上，
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  sun_light_diffuse); //漫反射后~~
        glLightfv(GL_LIGHT0, GL_SPECULAR, sun_light_specular);//镜面反射后~~~
        glEnable(GL_LIGHT0); //使用第0号光照
        

        glColorMaterial(GL_FRONT_AND_BACK,GL_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);

		/*************************/
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		
		/**************************/

        //glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		/*************渲染模型代码*************/
		GLfloat LightPosition1[] = { -8.0f, -4.0f, -40.0f, 1.0f };
		GLfloat LightPosition2[] = { 7.0f, -22.0f, -32.0f, 1.0f };

		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
		glEnable(GL_LIGHT1);

		glLightfv(GL_LIGHT2, GL_AMBIENT, LightAmbient);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, LightDiffuse);
		glLightfv(GL_LIGHT2, GL_POSITION, LightPosition1);
		glEnable(GL_LIGHT2);

		glLightfv(GL_LIGHT3, GL_AMBIENT, LightAmbient);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, LightDiffuse);
		glLightfv(GL_LIGHT3, GL_POSITION, LightPosition2);
		glEnable(GL_LIGHT3);

		
		//SwapBuffers(hDC);
		/**************************************/

        glTranslatef(0.f,-0.5f,0.f);

	
        //window
        glLoadIdentity();
        glColor3f(1,1,1);
        glLoadIdentity();
        glDisable(GL_LIGHTING);

		/****************粒子系统*******************/
		glLoadIdentity();
		for (loop = 0; loop < MAX_PARTICLES; ++loop)
		{

			if (paticle[loop].active)//the particle is alive
			{
				float x = paticle[loop].x;
				float y = paticle[loop].y;
				//our scene is moved into the screen
				float z = paticle[loop].z + zoom;

				glColor4f(paticle[loop].r,
					paticle[loop].g,
					paticle[loop].r,
					//use life as alpha value:
					//as particle dies,it becomes more and more transparent
					paticle[loop].life);

				//draw particles : use triangle strip instead of quad to speed
				glBegin(GL_TRIANGLE_STRIP);
				//top right
				glTexCoord2d(1, 1);
				glVertex3f(x + 0.05f, y + 0.05f, z);
				//top left
				glTexCoord2d(0, 1);
				glVertex3f(x - 0.05f, y + 0.05f, z);
				//bottom right
				glTexCoord2d(1, 0);
				glVertex3f(x + 0.05f, y - 0.05f, z);
				//bottom left
				glTexCoord2d(0, 0);
				glVertex3f(x - 0.05f, y - 0.05f, z);
				glEnd();

				//Move On The X Axis By X Speed 
				paticle[loop].x += paticle[loop].xi / (slowdown * 1000);
				//Move On The Y Axis By Y Speed 
				paticle[loop].y += paticle[loop].yi / (slowdown * 1000);
				//Move On The Z Axis By Z Speed 
				paticle[loop].z += paticle[loop].zi / (slowdown * 1000);

				//add gravity or resistance : acceleration
				paticle[loop].xi += paticle[loop].xg;
				paticle[loop].yi += paticle[loop].yg;
				paticle[loop].zi += paticle[loop].zg;

				//decrease particles' life
				paticle[loop].life -= paticle[loop].fade;

				//if particle is dead,rejuvenate it
				if (paticle[loop].life < 0.0f)
				{
					paticle[loop].life = 1.0f;//alive again
					paticle[loop].fade = float(rand() % 100) / 1000.0f + 0.003f;

					//reset its position
					paticle[loop].x = 0.0f;
					paticle[loop].y = 0.0f;
					paticle[loop].z = 0.0f;

					//X Axis Speed And Direction 
					paticle[loop].xi = xspeed + float((rand() % 60) - 32.0f);
					// Y Axis Speed And Direction 
					paticle[loop].yi = yspeed + float((rand() % 60) - 30.0f);
					// Z Axis Speed And Direction 
					paticle[loop].zi = float((rand() % 60) - 30.0f);

					paticle[loop].r = colors[loop/100][0];			// Select Red From Color Table
					paticle[loop].g = colors[loop/100][1];			// Select Green From Color Table
					paticle[loop].b = colors[loop/100][2];			// Select Blue From Color Table
				}
			}
		}

		selectFont(48, ANSI_CHARSET, "Comic Sans MS");//选择字符样式，大小，字体等
		glRasterPos3f(0.0f, -12.0f, -5.0f);//设置文字位置
		drawString("City Trip");

		//渲染模型
		mesh.Render();

		
		//DrawGLScene();
		//glutSwapBuffers();
		/*****************************************/


		/*       ground      */
        CBMPLoader loaderGround;
        loaderGround.LoadBitmap("assets/ground.bmp");
        glGenTextures(1, &loaderGround.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderGround.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderGround.imageWidth,
                        loaderGround.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderGround.image);
        glBindTexture(GL_TEXTURE_2D, loaderGround.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-40,0,40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(40,0,40);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(40,0,-40);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-40,0,-40);

    glEnd();
/*         Night_back       */
        CBMPLoader loaderBack;
        //loaderBack.LoadBitmap("skybox/Night_back.bmp");
		loaderBack.LoadBitmap("skybox/back.bmp");
        glGenTextures(1, &loaderBack.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderBack.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderBack.imageWidth,
                        loaderBack.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderBack.image);
        glBindTexture(GL_TEXTURE_2D, loaderBack.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-40,40,40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-40,-40,40);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(40,-40,40);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(40,40,40);

    glEnd();
/*         Night_front       */
        CBMPLoader loader;
        //loader.LoadBitmap("skybox/Night_front.bmp");
		loader.LoadBitmap("skybox/front.bmp");
        glGenTextures(1, &loader.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loader.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loader.imageWidth,
                        loader.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loader.image);
        glBindTexture(GL_TEXTURE_2D, loader.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-40,40,-40);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-40,-40,-40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(40,-40,-40);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(40,40,-40);

    glEnd();
	/*         Night_left       */
        CBMPLoader loaderLeft;
        //loaderLeft.LoadBitmap("skybox/Night_left.bmp");
		loaderLeft.LoadBitmap("skybox/left.bmp");
        glGenTextures(1, &loaderLeft.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderLeft.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderLeft.imageWidth,
                        loaderLeft.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderLeft.image);
        glBindTexture(GL_TEXTURE_2D, loaderLeft.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-40,40,40);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-40,-40,40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-40,-40,-40);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-40,40,-40);

    glEnd();
	/*         Night_right      */
        CBMPLoader loaderRight;
        //loaderRight.LoadBitmap("skybox/Night_right.bmp");
		loaderRight.LoadBitmap("skybox/right.bmp");
        glGenTextures(1, &loaderRight.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderRight.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderRight.imageWidth,
                        loaderRight.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderRight.image);
        glBindTexture(GL_TEXTURE_2D, loaderRight.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(0.0f, 0.0f); glVertex3f(40,40,40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(40,-40,40);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(40,-40,-40);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(40,40,-40);

    glEnd();
	/*         Night_up       */
        CBMPLoader loaderTop;
        //loaderTop.LoadBitmap("skybox/Night_up.bmp");
		loaderTop.LoadBitmap("skybox/top.bmp");
        glGenTextures(1, &loaderTop.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderTop.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderTop.imageWidth,
                        loaderTop.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderTop.image);
        glBindTexture(GL_TEXTURE_2D, loaderTop.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-40,-40,40);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-40,-40,-40);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(40,-40,-40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(40,-40,40);


    glEnd();
	/*         Night_bottom       */
        CBMPLoader loaderBottom;
        loaderBottom.LoadBitmap("skybox/Night_bottom.bmp");
        glGenTextures(1, &loaderBottom.ID);                        /**< 生成一个纹理对象名称 */

        glBindTexture(GL_TEXTURE_2D, loaderBottom.ID);             /**< 创建纹理对象 */
        /** 控制滤波: */
        /*
            其中GL_TEXTURE_WRAP_S，GL_TEXTURE_WRAP_T通常可设置为GL_REPEAT或GL_CLAMP两种方式。
            当待填充的多边形大于纹理的时候，GL_REPEAT表示多余的部分用重复的方式填充；GL_CLAMP表示多余的部分用相连边缘的相邻像素填充。
            在实际绘制中，我们一般采用GL_CLAMP_EDGE来处理，这就消除了接缝处的细线，增强了天空盒的真实感。
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        /** 创建纹理 */
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB,loaderBottom.imageWidth,
                        loaderBottom.imageHeight, GL_RGB, GL_UNSIGNED_BYTE,
                        loaderBottom.image);
        glBindTexture(GL_TEXTURE_2D, loaderBottom.ID);

        glBegin(GL_QUADS);

        /** 指定纹理坐标和顶点坐标 */
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-40,40,40);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-40,40,-40);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(40,40,-40);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(40,40,40);


    glEnd();


        glfwSwapBuffers(window);

        glfwPollEvents();

        loader.FreeImage();
        loaderBack.FreeImage();
        loaderLeft.FreeImage();
        loaderTop.FreeImage();
        loaderRight.FreeImage();
        loaderBottom.FreeImage();
        loaderGround.FreeImage();
        glDeleteTextures(1,&loader.ID);
        glDeleteTextures(1,&loaderBack.ID);
        glDeleteTextures(1,&loaderLeft.ID);
        glDeleteTextures(1,&loaderRight.ID);
        glDeleteTextures(1,&loaderTop.ID);
        glDeleteTextures(1,&loaderBottom.ID);
        glDeleteTextures(1,&loaderGround.ID);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


