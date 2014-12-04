
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o showtiff showtiff.c -ltiff -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

/* showtiff is a simple TIFF file viewer using Sam Leffler's libtiff, GLUT,
   and OpenGL.  If OpenGL the image processing extensions for convolution and
   color matrix are supported by your OpenGL implementation, showtiff will let
   you blur, sharpen, edge detect, and color convert to grayscale the TIFF
   file.  You can also move around the image within the window. */

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <Windows.h>
#include <chrono>
#include "stdafx.h"
#include "glut.h"
#include "dirent.h"
#include <gl/glcorearb.h>
#include <vector>
#include <map>
#include <tiffio.h>     /* Sam Leffler's libtiff library. */
#include "QuerySet.h"

using namespace std;

static const int MAX_IMAGES = 2;

//Holds the index to the main window
int main_window; 

// pointers for all of the glui controls
GLUI *glui;
GLUI* msgGlui; 
GLUI* welcomeGlui;

//Live Variables
int numLinks = 10;
float initAngle = 0.f;
float length = 1.f;

//State variables
int drawInterpolatedSteps;
int useContraints;

const typedef enum{

	CB_READY_BUTTON,
	CB_LEFT_SIDE_WON_BUTTON,
	CB_RIGHT_SIDE_WON_BUTTON,
	CB_IT_WAS_A_TIE_BUTTON,
	CB_I_DONT_KNOW_BUTTON,
	//CB_SUBMIT_BUTTON,
	CB_NEXT_BUTTON,
	CB_OK_BUTTON,
	CB_YES_BUTTON,
	CB_NO_BUTTON
}Action;

const typedef enum{
	TIFF_ROCK,
	TIFF_PAPER,
	TIFF_SISSORS,
	TIFF_ZERO,
	TIFF_ONE,
	TIFF_TWO,
	TIFF_THREE,
	TIFF_FOUR,
	TIFF_FIVE
}HandConfig;

const typedef enum{
	ROCK_PAPER,
	ROCK_SCISSOR,
	PAPER_ROCK,
	PAPER_SCISSOR,
	SCISSOR_ROCK,
	SCISSOR_PAPER
}QuerySetType;

TIFFRGBAImage img;
uint32 *raster;
size_t npixels;
int imgwidth, imgheight;
int w_width, w_height;

int hasABGR = 0;
int hasConvolve = 0;
int hasColorMatrix = 0;
int doubleBuffer = 1;
char *filename = NULL;
std::string fileDir;
int ax = 0, ay = 0;
int luminance = 0;

std::vector<std::string> tiffImages;
std::map<std::string, int> tiffIndices;
int imgCount=0;

//QuerySet Related info
std::vector<QuerySet> querySets;

static bool firstTime = false;

std::string filePathName(const std::string mfilePath, const std::string mfileName)
{
	std::string buffer;

	buffer.append(mfilePath);
	buffer.append(mfileName);

	return buffer;
}

int getFileNames(const char *dirname)
{
	int tmpCount = 0;

	DIR *dir = opendir (dirname);
	if (dir != NULL) {
		struct dirent *ent;

		while ((ent = readdir (dir)) != NULL) 
		{
			if(ent->d_type != DT_DIR)
			{
				char buffer[PATH_MAX + 2];
				char *p = buffer;
				const char *src = ent->d_name;
				while (*src != '\0') {
						*p++ = *src++;
				}
				*p = '\0';
				std::string name(buffer);
				tiffImages.push_back(name);
				tiffIndices.insert(std::map<std::string, int>::value_type(name, tiffImages.size()-1));
			}
		}
	}
	closedir (dir);
	return tiffImages.size();
}

void rotateLeft()
{
  uint32* newRaster =  (uint32*) malloc(npixels * sizeof(uint32));
  //test rortation
  for(int row=0; row<img.height; row++) {
    for(int col=0; col<img.width; col++) {
		*(newRaster+(col*img.height+img.height-1-row))= *(raster+(img.width*row+col));
        //*(newRaster+(img.height*col+row))= *(raster+(img.width*row+col));
	}	
  }
  uint32* tmpRaster = raster;
  raster = newRaster;

  delete tmpRaster;

  int tmpwidth = img.width;
  int tmpheight = img.height;
  img.width = tmpheight;
  img.height = tmpwidth;
  imgwidth = img.width;
  imgheight = img.height;
}

void rotateRight()
{
   uint32* newRaster =  (uint32*) malloc(npixels * sizeof(uint32));
  //test rortation
  for(int row=0; row<img.height; row++) {
    for(int col=0; col<img.width; col++) {
		*(newRaster+(img.height*(img.width-1-col)+row))= *(raster+(img.width*row+col));
         //*(newRaster+(col*img.height+img.height-1-row))= *(raster+(img.width*row+col));
	}	
  }
  uint32* tmpRaster = raster;
  raster = newRaster;

  delete tmpRaster;

  int tmpwidth = img.width;
  int tmpheight = img.height;
  img.width = tmpheight;
  img.height = tmpwidth;
  imgwidth = img.width;
  imgheight = img.height;
}

int openFile(const char* mfilename)
{
  TIFF *tif;
  char emsg[1024];

  if (mfilename == NULL) {
    fprintf(stderr, "Filename is null\n");
    exit(1);
  }
  tif = TIFFOpen(mfilename, "r");
  if (tif == NULL) {
    fprintf(stderr, "Problem showing %s\n", mfilename);
    exit(1);
  }
  if (TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
    npixels = img.width * img.height;
    raster = (uint32 *) _TIFFmalloc(npixels * sizeof(uint32));
    if (raster != NULL) {
      if (TIFFRGBAImageGet(&img, raster, img.width, img.height) == 0) {
        TIFFError(filename, emsg);
        exit(1);
      }
    }
    TIFFRGBAImageEnd(&img);
  } else {
    TIFFError(mfilename, emsg);
    exit(1);
  }
  imgwidth = img.width;
  imgheight = img.height;
  return 1;
}

/* If resize is called, enable drawing into the full screen area
   (glViewport). Then setup the modelview and projection matrices to map 2D
   x,y coodinates directly onto pixels in the window (lower left origin).
   Then set the raster position (where the image would be drawn) to be offset
   from the upper left corner, and then offset by the current offset (using a
   null glBitmap). */
void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0, 0, 0);
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, ax, -ay, NULL);
}

void drawHands() //std::string filename1, std::string filename2)
{
	 /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBitmap(0, 0, 0, 0, 0, 0, NULL);

  	std::string fullPath = filePathName(fileDir, tiffImages.at((!imgCount)? 0 : imgCount-1 ));
	openFile(fullPath.c_str());
	rotateRight();

  /* Re-blit the image. */
	glDrawPixels(imgwidth, imgheight, GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);

	fullPath = filePathName(fileDir, tiffImages.at(imgCount));
		openFile(fullPath.c_str());
		imgCount++;
		if(imgCount >= tiffImages.size())
		imgCount = 0;
   
   rotateLeft();
   glBitmap(0, 0, 0, 0, 0, 0, NULL);
   glRasterPos2i(imgwidth, 0);
  /* Re-blit the image. */
	glDrawPixels(imgwidth, imgheight, GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);
	glRasterPos2i(0, 0);
	glutSwapBuffers();
}

static int showingImages = 0;

void
display(void)
{
  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
  showingImages = 0;
}

static int ox, oy;

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

    } else {

      /* Left mouse button released; unset "moving" since button no longer
         pressed. */
    }
  }
}

void
motion(int x, int y)
{
  /* If there is mouse motion with the left button held down... */
  if (!showingImages) {

    /* Figure out the offset from the last mouse position seen. */
    //ax += (x - ox);
    //ay += (y - oy);

    /* Offset the raster position based on the just calculated mouse position 

       delta.  Use a null glBitmap call to offset the raster position in
       window coordinates. */
    //glBitmap(0, 0, 0, 0, x - ox, oy - y, NULL);

    /* Request a window redraw. */
    glutPostRedisplay();

    /* Update last seen mouse position. */
    ox = x;
    oy = y;
  }
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'z':
	case 'Z':
		std::cout << "Z pressed" <<std::endl;
		break;
	case 'm':
	case 'M':
		std::cout << "M pressed" <<std::endl;
		break;
	case 't':
	case 'T':
		std::cout << "T pressed" <<std::endl;
		break;
	case 13: //enter
		std::cout << "ENTER pressed" <<std::endl;
		break;
	// quit
	case 27: 
	case 'q':
	case 'Q':
		exit(0);
		break;
	}
	glutPostRedisplay();
}

void
option(int value)
{
  switch (value) {
  case 1:
    break;
  case 666:
    exit(0);
    break;
  }
  glutPostRedisplay();
}

void addQuerySet(int num)
{
	switch(num)
	{
	case ROCK_PAPER:
		querySets.push_back(QuerySet("Rock","Paper"));
		break;
	case ROCK_SCISSOR:
		querySets.push_back(QuerySet("Rock","Scissor"));
		break;
	case PAPER_ROCK:
		querySets.push_back(QuerySet("Paper","Rock"));
		break;
	case PAPER_SCISSOR:
		querySets.push_back(QuerySet("Paper","Scissor"));
		break;
	case SCISSOR_ROCK:
		querySets.push_back(QuerySet("Scissor","Rock"));
		break;
	case SCISSOR_PAPER:
		querySets.push_back(QuerySet("Scissor","Paper"));
		break;
	}
}

void initializeQuerySets()
{
	//To esnure we don't add the same query set twice, well keep a hash
	//of which query sets we've already added
	std::map<int, bool> queryMap;

	//generate a random int. This int corresponds to the QUerySet enumType
	int numQuerySets = 6;
	srand( time(NULL) );
	int randomNum;
	while(querySets.size() < numQuerySets)
	{
		randomNum = rand() % numQuerySets; //some number between 0 and 6
		if(queryMap.count(randomNum))
		{
			//this particular Query has already been added
		} else {
			//add new querySet
			addQuerySet(randomNum);
			queryMap.insert(std::map<int, bool>::value_type(randomNum, true));
		}
	}
}

// some controls generate a callback when they are changed
void glui_cb(int control)
{
	//TODO: redo all this
	switch(control)
	{
	case CB_READY_BUTTON:
		if(!showingImages)
		{
			showingImages = 1;
			auto start = std::chrono::system_clock::now();
			drawHands();
			auto elapsed = 0.0;
			while( elapsed < 1000)
			{
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now() - start);
				elapsed = duration.count();
			}
			glutPostRedisplay();
		}
		break;
	case CB_LEFT_SIDE_WON_BUTTON:
			//showGrid = !showGrid;
		break;
	case CB_RIGHT_SIDE_WON_BUTTON:
			//updateLinksButton();
		break;
	case CB_IT_WAS_A_TIE_BUTTON:
	//updateLinksButton();
		break;
	case CB_I_DONT_KNOW_BUTTON:
	//updateLinksButton();
		break;
	case CB_OK_BUTTON:
		if(msgGlui)
			msgGlui->show();
		else
		{
			msgGlui = GLUI_Master.create_glui("Message",0,w_width/2,w_height/2);
			//tmpglui->set_main_gfx_window(main_window);
			msgGlui->add_statictext("Are you sure you understand the instructions?");
			msgGlui->add_button("Yes", CB_YES_BUTTON, glui_cb);
			msgGlui->add_column(false);
			msgGlui->add_statictext("");
			msgGlui->add_button("No", CB_NO_BUTTON, glui_cb);
		}
		break;
	case CB_NEXT_BUTTON:
		break;
	case CB_YES_BUTTON:
		if(msgGlui)
			msgGlui->hide();
		if(welcomeGlui)
		{
			welcomeGlui->disable();
			welcomeGlui->hide();
		}
		if(glui)
		{
			glui->enable();
			glui->show();
		}
	case CB_NO_BUTTON:
		if(msgGlui)
			msgGlui->hide();
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

void mainSubWindow()
{
	glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_RIGHT);
	glui->set_main_gfx_window(main_window);

	GLUI_Panel *instructions_panel = glui->add_panel("Instructions");

	//add text to panel
	glui->add_statictext_to_panel(instructions_panel, "This system will display a series of animated hand gestures.");
	glui->add_statictext_to_panel(instructions_panel, "There are two categories of hand gestures:");
	glui->add_statictext_to_panel(instructions_panel, "1. Rock-Paper-Scissors, in which two hand gestures");
	glui->add_statictext_to_panel(instructions_panel, "will be shown and you will identify the winner.");
	glui->add_statictext_to_panel(instructions_panel, "2. Finger Counting, in which a hand will be shows and");
	glui->add_statictext_to_panel(instructions_panel, "you are asked to count how many fingers were held up.");
	
	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");

	GLUI_Panel *controls_panel = glui->add_panel("Controls");
	//add button to panel
	glui->add_button_to_panel(controls_panel, "Ready. Show Images", CB_READY_BUTTON, glui_cb);

	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");
	glui->add_statictext("");

	//Create a panel
	GLUI_Panel *answers_panel = glui->add_panel("Keyboard Input Answers");

	glui->add_statictext_to_panel(answers_panel, "           'z' to indicate that the LEFT hand won.       ");
	glui->add_statictext_to_panel(answers_panel, "           'm' to indicate that the RIGHT hand won       ");
	glui->add_statictext_to_panel(answers_panel, "           't' to indicate that it was a tie      ");
	glui->add_statictext_to_panel(answers_panel, "           'ENTER' if you are not sure who won.     ");

	glui->disable();
	glui->hide();
}

void welcomeScreen()
{
	welcomeGlui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_RIGHT);
	welcomeGlui->set_main_gfx_window(main_window);

	welcomeGlui->add_statictext("");

	welcomeGlui->add_statictext("Instructions");

	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext("");

	//add text to panel
	welcomeGlui->add_statictext( "This system will display a series of animated hand gestures and ask you to identify them. There are two categories of hand gestures:");
	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext( "1. Rock-Paper-Scissors (RPS)");
	welcomeGlui->add_statictext( "       In RPS you will be shown two hands simultaneously for two seconds. Each hand will be a rock, paper or scissor.");
	welcomeGlui->add_statictext( "       You are asked to answer which side you think won. Your answer will be input using the keyboard using the following inputs"); 
	welcomeGlui->add_statictext("");
	GLUI_Panel *answers_panel = welcomeGlui->add_panel("Keyboard Input Answers");

	welcomeGlui->add_statictext_to_panel(answers_panel, "           'z'        to indicate that the LEFT hand won.       ");
	welcomeGlui->add_statictext_to_panel(answers_panel, "           'm'       to indicate that the RIGHT hand won       ");
	welcomeGlui->add_statictext_to_panel(answers_panel, "           't'        to indicate that it was a tie      ");
	welcomeGlui->add_statictext_to_panel(answers_panel, "       'ENTER'   if you are not sure who won.     ");

	welcomeGlui->add_statictext( "There will be a few practice image to give you an opportunity to get used to the system."); 
    
	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext( "2. Finger Counting, in which a hand will be shows and");
	welcomeGlui->add_statictext( "you are asked to count how many fingers were held up.");
	
	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext("");
	welcomeGlui->add_statictext("");

	//add button to panel
	welcomeGlui->add_button("Ok", CB_OK_BUTTON, glui_cb);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    }
  }
  if (doubleBuffer) {
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  } else {
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);
  }

  fileDir = "..\/QueryDisplay\/images\/\0";
  int val = getFileNames(fileDir.c_str());
  if(val)
  {
	  std::string fullPath = filePathName(fileDir, tiffImages.at(imgCount));
	  openFile(fullPath.c_str());
	  imgCount++;
  } else {
	  fprintf(stderr, "No valid files found in directory %s\n", fileDir);
	  exit(1);
  }

  w_width = imgheight*2.25+240;
  w_height = imgwidth;

  glutInitWindowSize(w_width, w_height);

   main_window = glutCreateWindow("Query Display");

	// set callbacks
	glutDisplayFunc(display);
	GLUI_Master.set_glutReshapeFunc(reshape);
	//GLUI_Master.set_glutIdleFunc(myGlutIdle);
	GLUI_Master.set_glutKeyboardFunc(keyboard);
	GLUI_Master.set_glutMouseFunc(mouse);
	glutMotionFunc(motion);

	mainSubWindow();
	welcomeScreen();

	//initializeQuerySets();	

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glClearColor(0.2, 0.2, 0.2, 1.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}