#include <iostream>
#include <string>
#include <sstream>
#include <Windows.h>
#include "stdafx.h"
#include <math.h>
#include <glm/glm.hpp>

using namespace std;
using glm::vec4;
using glm::mat4;

int main_window; //Holds the index to the main window

// pointers for all of the glui controls
GLUI *glui;

//mouse position
float gX, gY;

//Window parameters
int winWidth = 500, winHeight = 500;     // window (x,y) size

//Camera Controls
double seye[3] = {0.5,0.5,10};
double at[3]  = {0.5,0.5,0};
double up[3]  = {0.0,1.0,0.0};

//////////////////////////////////////////////////////////////////////
//
// Set all variables to their default values.
//
// *** DO NOT CHANGE ***
//
//////////////////////////////////////////////////////////////////////

void initializeWindow() {
  //reset mouse X and Y to origin
  gX = gY = 0;
}

void setCamera() {
  glViewport(0,0, winWidth,winHeight);

}

void myGlutIdle(void)
{
	// make sure the main window is active
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);

	// just keep redrawing the scene over and over
	glutPostRedisplay();
}

// mouse handling functions for the main window
// left mouse translates, middle zooms, right rotates
// keep track of which button is down and where the last position was
int cur_button = -1;
int last_x;
int last_y;

// catch mouse up/down events
void myGlutMouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
		cur_button = button;
	else
	{
		if (button == cur_button)
			cur_button = -1;
		//gX = ((float)(x))/winWidth * METERS - 3;
		//gY = (-((float)(y))/winHeight * METERS)+ 4;
	}

	last_x = x;
	last_y = y;

  // Leave the following call in place.  It tells GLUT that
  // we've done something, and that the window needs to be
  // re-drawn.  GLUT will call display().
  glutPostRedisplay();
}

// catch mouse move events
void myGlutMotion(int x, int y)
{
	// the change in mouse position
	int dx = x-last_x;
	int dy = y-last_y;

	vec4 eye;
	mat4 translate, rotate;
	switch(cur_button)
	{
		//'translate
	case GLUT_LEFT_BUTTON:
		break;
		//zoom
	case GLUT_MIDDLE_BUTTON:
			break;
		//rotate
	case GLUT_RIGHT_BUTTON:
		break;
	}
	last_x = x;
	last_y = y;

	glutPostRedisplay();
}

// you can put keyboard shortcuts in here
void myGlutKeyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	// quit
	case 27: 
	case 'q':
	case 'Q':
		exit(0);
		break;
	}

	glutPostRedisplay();
}

// the window has changed shapes, fix ourselves up
void myGlutReshape(int	x, int y)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	winWidth = tw;
	winHeight = th;
	glViewport(tx, ty, tw, th);

	//GLUI_Master.auto_set_viewport();
	glutPostRedisplay();
}

// draw the scene
void myGlutDisplay(	void )
{
  setCamera();

  // Set the background colour to dark gray.
  glClearColor(.73f,.73f, 1.f, 1.0f);

  // OK, now clear the screen with the background colour
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set the drawing colour to yellow.
  glColor3f(1.0f, 1.0f, 0.0f);

  // Execute any GL functions that are in the queue.
  glFlush();

  // Now, show the frame buffer that we just drew into.
  // (this avoids flicker).
  glutSwapBuffers();
}

// some controls generate a callback when they are changed
void glui_cb(int control)
{
	switch(control)
	{
	default:
		break;
	}

	glutPostRedisplay();
}

// entry point
int _tmain(int argc, _TCHAR* argv[])
{
  // Initialize the GLUT window.  We want a double-buffered window,
  // with R,G,B and alpha per pixel, and the depth buffer (z-buffer)
  // enabled.
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

  // Put the window at the top left corner of the screen.
  glutInitWindowPosition (0, 0);

  // Specify the window dimensions.
  glutInitWindowSize(winWidth,winHeight);

  // And now create the window.
  main_window = glutCreateWindow("Query Display");

	// set callbacks
	glutDisplayFunc(myGlutDisplay);
	GLUI_Master.set_glutReshapeFunc(myGlutReshape);
	GLUI_Master.set_glutIdleFunc(myGlutIdle);
	GLUI_Master.set_glutKeyboardFunc(myGlutKeyboard);
	GLUI_Master.set_glutMouseFunc(myGlutMouse);
	glutMotionFunc(myGlutMotion);

	// initialize gl
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);

	// give control over to glut
	glutMainLoop();
}