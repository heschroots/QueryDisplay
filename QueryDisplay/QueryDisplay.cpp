
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
#include <tiffio.h>     /* Sam Leffler's libtiff library. */

using namespace std;

static const int MAX_IMAGES = 2;

//Holds the index to the main window
int main_window; 

// pointers for all of the glui controls
GLUI *glui;

//Live Variables
int numLinks = 10;
float initAngle = 0.f;
float length = 1.f;

//State variables
int drawInterpolatedSteps;
int useContraints;

const typedef enum{

	CB_GRID_BUTTON,
	CB_UPDATE_LINKS_BUTTON,

	CB_NUMLINKS_SPINNER,
	CB_INIT_ANGLE_SPINNER,
	CB_INIT_LENGTH_SPINNER,

	CB_DRAW_INTERPOLATED_STEPS_CHECK,
	CB_USE_CONSTRAINTS_CHECK,

	CB_LINK_ANGLE_SPINNERS,
	CB_LINK_WEIGHT_SPINNERS,

	CB_RESET
}Action;

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
int imgCount=0;

GLfloat rgbBlur[7][7][3];
GLfloat rgbEdgeDetect[3][3][3];
GLfloat rgbSharpen[3][3][3];

/* Find files and subdirectories recursively */
static int
find_directory(
		const char *dirname)
{
		char buffer[PATH_MAX + 2];
		char *p = buffer;
		const char *src;
		char *end = &buffer[PATH_MAX];
		int ok;

		/* Copy directory name to buffer */
		src = dirname;
		while (p < end  &&  *src != '\0') {
			*p++ = *src++;
		}
		*p = '\0';

		/* Open directory stream */
		DIR* dir = opendir (dirname);
		if (dir != NULL) {
			struct dirent *ent;

			/* Print all files and directories within the directory */
			while ((ent = readdir (dir)) != NULL) {
				char *q = p;
				char c;

				/* Get final character of directory name */
				if (buffer < q) {
						c = q[-1];
				} else {
						c = ':';
				}

				/* Append directory separator if not already there */
				if (c != ':'  &&  c != '/'  &&  c != '\\') {
						*q++ = '/';
				}

				/* Append file name */
				src = ent->d_name;
				while (q < end  &&  *src != '\0') {
						*q++ = *src++;
				}
				*q = '\0';

				/* Decide what to do with the directory entry */
				switch (ent->d_type) {
				case DT_LNK:
				case DT_REG:
						/* Output file name with directory */
						printf ("%s\n", buffer);
						break;

				case DT_DIR:
						/* Scan sub-directory recursively */
						if (strcmp (ent->d_name, ".") != 0  
								&&  strcmp (ent->d_name, "..") != 0) {
							find_directory (buffer);
						}
						break;

				default:
						/* Ignore device entries */
						/*NOP*/;
				}

			}

			closedir (dir);
			ok = 1;

		} else {
			/* Could not open directory */
			printf ("Cannot open directory %s\n", dirname);
			ok = 0;
		}

		return ok;
}

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
			}
		}
	}
	closedir (dir);
	return tiffImages.size();
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
  //if we decide to use the tiff directory option
  //that is multiple tiff images are stored in a single file
  /*
   if (tif) {
      int dircount = 0;
      do {
         dircount++;
      } while (TIFFReadDirectory(tif));
      printf("%d directories in %s\n", dircount, filename);
      //TIFFClose(tif);
    }
	 */
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
  glTranslatef(0, h - imgheight, 0);
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, ax, -ay, NULL);
}

void drawHands() //std::string filename1, std::string filename2)
{
	 /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBitmap(0, 0, 0, 0, -imgwidth/4, 0, NULL);
  /* Re-blit the image. */
	glDrawPixels(imgwidth, imgheight,
		 hasABGR ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);

	std::string fullPath = filePathName(fileDir, tiffImages.at(imgCount));
		openFile(fullPath.c_str());
		imgCount++;
		if(imgCount >= tiffImages.size())
		imgCount = 0;
   
   glRasterPos2i(imgwidth/2, 0);
  /* Re-blit the image. */
	glDrawPixels(imgwidth, imgheight,
		 hasABGR ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);
	glRasterPos2i(0, 0);
	glutSwapBuffers();
}
void
display(void)
{
  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /*glBitmap(0, 0, 0, 0, -imgwidth/4, 0, NULL);
  // Re-blit the image. 
	glDrawPixels(imgwidth, imgheight,
		 hasABGR ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);

	std::string fullPath = filePathName(fileDir, tiffImages.at(imgCount));
		openFile(fullPath.c_str());
		imgCount++;
		if(imgCount > MAX_IMAGES-1)
		imgCount = 0;
   
   glRasterPos2i(imgwidth/2, 0);
  // Re-blit the image. 
	glDrawPixels(imgwidth, imgheight,
		 hasABGR ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);
	glRasterPos2i(0, 0);*/
  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

static int moving = 0, ox, oy;

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      /* Left mouse button press.  Update last seen mouse position. And set
         "moving" true since button is pressed. */
      //ox = x;
      //oy = y;
      moving = 1;
		
		/*std::string fullPath = filePathName(fileDir, tiffImages.at(imgCount));
		openFile(fullPath.c_str());
		imgCount++;
		if(imgCount > MAX_IMAGES-1)
		imgCount = 0;*/

    } else {

      /* Left mouse button released; unset "moving" since button no longer
         pressed. */
      moving = 0;
    }

  }
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

void
motion(int x, int y)
{
  /* If there is mouse motion with the left button held down... */
  if (moving) {

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

// some controls generate a callback when they are changed
void glui_cb(int control)
{
	//TODO: redo all this
	switch(control)
	{
	case CB_GRID_BUTTON:
			//showGrid = !showGrid;
		break;
	case CB_UPDATE_LINKS_BUTTON:
			//updateLinksButton();
		break;
	case CB_NUMLINKS_SPINNER:
		break;
	case CB_INIT_ANGLE_SPINNER:
		break;
	case CB_INIT_LENGTH_SPINNER:
		break;
	case CB_LINK_ANGLE_SPINNERS:
		for(int i = 0; i < numLinks; i++)
			//desiredAngles[i] = *link_angles[i];
		break;
	case CB_LINK_WEIGHT_SPINNERS:
		for(int i = 0; i < numLinks; i++)
			//constraintWeights[i] = *link_weights[i];
		break;
	case CB_DRAW_INTERPOLATED_STEPS_CHECK:
		break;
	case CB_USE_CONSTRAINTS_CHECK:
		//updateSpinnerDisplay();
		break;
	case CB_RESET:
		//reset();
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

void initializeSubWindow()
{
	//TODO: Redo all this to have button that acutally work and all
	glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_RIGHT);
	glui->set_main_gfx_window(main_window);

	//Create a panel
	GLUI_Panel *init_params_panel = glui->add_panel("Initial Parameters");

	//Add spinner to panel
	GLUI_Spinner *link_spinner = glui->add_spinner_to_panel(init_params_panel, "Links:", GLUI_SPINNER_INT, &numLinks, CB_NUMLINKS_SPINNER, glui_cb);
	link_spinner->set_int_limits(0, 10, GLUI_LIMIT_CLAMP);

	GLUI_Spinner *angle_spinner = glui->add_spinner_to_panel(init_params_panel, "Init Angle:", GLUI_SPINNER_FLOAT, &initAngle, CB_INIT_ANGLE_SPINNER, glui_cb);
	angle_spinner->set_float_limits(0.1f, 90.f, GLUI_LIMIT_CLAMP);

	GLUI_Spinner *length_spinner = glui->add_spinner_to_panel(init_params_panel, "1st Link Length:", GLUI_SPINNER_FLOAT, &length, CB_INIT_LENGTH_SPINNER, glui_cb);
	length_spinner->set_float_limits(0.2f, 1.75f, GLUI_LIMIT_CLAMP);

	//add button to panel
	glui->add_button_to_panel(init_params_panel, "Update Links", CB_UPDATE_LINKS_BUTTON, glui_cb);

	//checkboxes
	glui->add_checkbox("Interpolate Animation", &drawInterpolatedSteps, CB_DRAW_INTERPOLATED_STEPS_CHECK, glui_cb);

	glui->add_separator();
	glui->add_button("Show Grid", CB_GRID_BUTTON, glui_cb); 
	glui->add_button("Reset", CB_RESET, glui_cb);
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

  imgwidth = img.width;
  imgheight = img.height;

  w_width = imgwidth*1.5+120;
  w_height = imgheight;

  glutInitWindowSize(w_width, w_height);

   main_window = glutCreateWindow("Query Display");

	// set callbacks
	glutDisplayFunc(display);
	GLUI_Master.set_glutReshapeFunc(reshape);
	//GLUI_Master.set_glutIdleFunc(myGlutIdle);
	//GLUI_Master.set_glutKeyboardFunc(myGlutKeyboard);
	GLUI_Master.set_glutMouseFunc(mouse);
	glutMotionFunc(motion);

	initializeSubWindow();


/*#ifdef GL_EXT_abgr
  if (glutExtensionSupported("GL_EXT_abgr")) {
    hasABGR = 1;
  }
#endif
  /* If cannot directly display ABGR format, we need to reverse the component
     ordering in each pixel. :-( */
  /*if (!hasABGR) {
    int i;

    for (i = 0; i < npixels; i++) {
      register unsigned char *cp = (unsigned char *) &raster[i];
      int t;

      t = cp[3];
      cp[3] = cp[0];
      cp[0] = t;
      t = cp[2];
      cp[2] = cp[1];
      cp[1] = t;
    }
  }*/
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glutCreateMenu(option);
  glutAddMenuEntry("Normal", 1);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  /* Use a gray background so TIFF images with black backgrounds will
     show against textiff's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);
  glEnable(GL_DEPTH_TEST);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}