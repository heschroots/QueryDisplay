
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

TIFFRGBAImage img;
uint32 *raster;
size_t npixels;
int imgwidth, imgheight;

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

  //char name[81] = "..\/QueryDisplay\/images\/1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1_1.tif";
  //filename = mfilename;
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

void
display(void)
{
  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Re-blit the image. */
	glDrawPixels(imgwidth, imgheight,
		 hasABGR ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE,
		 raster);

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
		
		std::string fullPath = filePathName(fileDir, tiffImages.at(imgCount));
		openFile(fullPath.c_str());
		imgCount++;
		if(imgCount > MAX_IMAGES-1)
		imgCount = 0;

    } else {

      /* Left mouse button released; unset "moving" since button no longer
         pressed. */
      moving = 0;
    }

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

  glutInitWindowSize(imgwidth, imgheight);

    main_window = glutCreateWindow("Query Display");

	// set callbacks
	glutDisplayFunc(display);
	GLUI_Master.set_glutReshapeFunc(reshape);
	//GLUI_Master.set_glutIdleFunc(myGlutIdle);
	//GLUI_Master.set_glutKeyboardFunc(myGlutKeyboard);
	GLUI_Master.set_glutMouseFunc(mouse);
	glutMotionFunc(motion);


#ifdef GL_EXT_abgr
  if (glutExtensionSupported("GL_EXT_abgr")) {
    hasABGR = 1;
  }
#endif
#ifdef GL_EXT_convolution
  if (glutExtensionSupported("GL_EXT_convolution")) {
    hasConvolve = 1;
  } else {
    while (glGetError() != GL_NO_ERROR);  /* Clear any OpenGL errors. */

    /* The following glDisable would be a no-op whether done on a freshly
       initialized OpenGL context whether convolution is supported or not.
       The only difference should be an OpenGL error should be reported if
       the GL_CONVOLUTION_2D_EXT is not understood (ie, convolution is not
       supported at all). */
    glDisable(GL_CONVOLUTION_2D_EXT);

    if (glGetError() == GL_NO_ERROR) {
      /* RealityEngine only partially implements the convolve extension and
         hence does not advertise the extension in its extension string (See
         MACHINE DEPENDENCIES section of the glConvolutionFilter2DEXT man
         page). We limit this program to use only the convolve functionality
         supported by RealityEngine so we test if OpenGL lets us enable
         convolution without an error (the indication that convolution is
         partially supported). */
      hasConvolve = 1;
    }
    /* Clear any further OpenGL errors (hopefully there should have only been 

       one or zero though). */
    while (glGetError() != GL_NO_ERROR);
  }
#endif
#ifdef GL_SGI_color_matrix
  if (glutExtensionSupported("GL_SGI_color_matrix")) {
    hasColorMatrix = 1;
  }
#endif
  /* If cannot directly display ABGR format, we need to reverse the component
     ordering in each pixel. :-( */
  if (!hasABGR) {
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
  }
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