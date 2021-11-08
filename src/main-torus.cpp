#include <iostream>
#include <stdlib.h>
#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
using namespace OpenMesh;

typedef OpenMesh::TriMesh_ArrayKernelT<OpenMesh::DefaultTraits> Mesh;


// You don't have to modify any of these variables
Mesh* mesh = NULL;
bool leftDown = false, rightDown = false, middleDown = false;
int lastPos[2];
float cameraPos[4] = { 0, 0, 20, 1 };
Vec3f up, pan;
int windowWidth = 640, windowHeight = 480;
float specular[] = { 1.0, 1.0, 1.0, 1.0 };
float shininess[] = { 50.0 };

#define R 5 // major radius of torus
#define r 2 // minor radius of torus

// a structure to hold a point in homogenous coordinates
struct Point {
	float x;
	float y;
	float z;
	float w;
};

// enumerated type used to refer to the particular torus quadrant
// we're constructing
enum torusQuadrant {
	posYposZ,
	posYnegZ,
	negYposZ,
	negYnegZ
};

// STUDENT CODE SECTION 1
// REPLACE CODE BELOW AND DEFINE THE CONTROL POINTS FOR THE FOUR QUADRANTS OF THE TORUS --------------------
// Replace values in the definition of 'Points'

/// 3x3 grid of control points that will define the surface patch
// we have 4 of these grids, one for each quarter of the torus
Point Points[4][3][3] = {

	//Positive y and positive z quarter of torus
	{
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		}
	},

	// Positive y and negative z quarter of torus
	{
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		}
	},

	// Negative y and positive z quarter of torus
	{
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		}
	},

	// Negative y and negative z quarter of torus
	{
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		},
		{
			{0,0,0,1},
			{0,0,0,1},
			{0,0,0,1}
		}
	}
};
// ---------------------------------------------------------------------------------------------------------


// the level of detail of the surface.
// surface patch is constructed as an LOD x LOD grid of vertices.
// minimum level of depth is 3.
unsigned int LOD = 30;

/*
 * Inputs:
 * u - float in range [0,1]
 * v - float in range [0,1]
 * idx - int in set {posYposZ, posYnegZ, negYposZ, negYnegZ} (one for each quarter of torus)
 *
 * Given u,v parameter values, interpolate bidirectionally (may want to define separate
 * functions for each interpolation) in order to evaluate the coordinates of the
 * corresponding point on the surface of the torus.
 */
Point Calculate(float u, float v, int idx) {
	Point surfacePoint;
	surfacePoint.x = surfacePoint.y = surfacePoint.z = 0;
	surfacePoint.w = 1;

	// STUDENT CODE SECTION 2
	// WRITE CODE HERE TO EVALUATE THE VERTEX POSITION OF A POINT ON THE SURFACE ------------------------------

		//u=Q, v=T
	float constant = R * (1 + u * u) + r * (1 - u * u);
	surfacePoint.w = (1 + v * v)*(1 + u * u);
	surfacePoint.x = constant * (1 - v * v);
	surfacePoint.y = constant * 2 * v;
	surfacePoint.z = 2 * u*r*(1 + v * v);

	if (idx % 2)
	{
		surfacePoint.z /= -1;

	}
	if (idx >= 2)
	{
		surfacePoint.y /= -1;
	}

	// ---------------------------------------------------------------------------------------------------------

	return surfacePoint;
}


/*
 * The data structure we will use to construct the mesh is a little hard to picture.
 * We highly recommend you draw a picture or clarify in office hours.  It is as follows:
 *
 * To store the coordinates of the surface points:
 * vhandle points to an array of 4 pointers, one for each quadrant of the torus.
 * Each of these pointers points to an array of size LOD, and each of the
 * elements of this array is another set of pointers.  These final pointers point
 * to an array of size LOD whose elements are vertex handles.  These keep track of
 * coordinates of points on the surface of the torus.
 *
 * Informally, vhandle is nothing more than a set of 4 two-dimensional arrays.  If we
 * consider just one quadrant of the torus, this corresponds to a single 2D array of
 * vertex handles.  We are mapping our rectangular parameter domain (u,v) discretely
 * into a 2D grid that resides in 3D space.
 *
 * To construct the mesh:
 * The indices of our 2D arrays store vertex handles.  Thus we can list 3 (or 4) adjacent
 * vertex handles as constituting a face.  If you choose to do the extra credit and want
 * to orient your faces correctly, you have to pay close attention to the order in which
 * you list your vertices.
 */
Mesh* generateMesh() {
	// have a minimum level of depth
	if (LOD < 3)
		LOD = 3;

	Mesh::VertexHandle*** vhandle = new VertexHandle**[4];
	for (int i = posYposZ; i <= negYnegZ; ++i) {
		vhandle[i] = new VertexHandle *[LOD];
		for (int j = 0; j < LOD; ++j) {
			vhandle[i][j] = new VertexHandle[LOD];
		}
	}

	if (mesh) {
		delete mesh;
	}

	mesh = new Mesh();

	for (int i = 0; i < LOD; ++i) {
		// calculate the parametric u value
		float u = (float)i / (LOD - 1);

		for (int j = 0; j < LOD; ++j) {
			// calculate the parametric v value
			float v = (float)j / (LOD - 1);

			for (int k = posYposZ; k <= negYnegZ; ++k) {
				// NEED TO FILL IN DEFINITION OF Calculate(float u,float v,int idx) -----------------------
				// calculate the point on the surface
				Point p = Calculate(u, v, k);
				// ----------------------------------------------------------------------------------------

				// stores point in the appropriate 2D array of vertex handles
				vhandle[k][i][j] = mesh->add_vertex(Mesh::Point(p.x / p.w, p.y / p.w, p.z / p.w));
			}
		}
	}

	//Add faces
	std::vector<Mesh::VertexHandle>  face_vhandles;
	for (int i = 0; i < LOD - 1; ++i) {
		for (int j = 0; j < LOD - 1; ++j) {
			for (int k = posYposZ; k <= negYnegZ; ++k) {

				// STUDENT CODE SECTION 3
				// NEED TO ADD FACES TO THE MESH USING YOUR VERTEX HANDLES DEFINED IN vhandle -------------

				// ----------------------------------------------------------------------------------------
				face_vhandles.push_back(vhandle[i][j][k]);
			}
		}
	}
	mesh->add_face(face_vhandles);
	/*
	 * Writes mesh as a .off file.
	 * Can use the code in ComputeCurvature to view your mesh with lighting
	 * to verify correct face orientation if you so desire (not required).
	 */
	if (!OpenMesh::IO::write_mesh(*mesh, "torus.off"))
	{
		std::cerr << "write error\n";
		exit(1);
	}

	return mesh;
}

void display() {
	// clear the screen & depth buffer
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, windowWidth, windowHeight);

	float ratio = (float)windowWidth / (float)windowHeight;
	gluPerspective(50, ratio, 1, 1000); // 50 degree vertical viewing angle, zNear = 1, zFar = 1000

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(cameraPos[0] + pan[0], cameraPos[1] + pan[1], cameraPos[2] + pan[2],
		pan[0], pan[1], pan[2], up[0], up[1], up[2]);

	glColor3f(1, 0, 1);

	// Iterates through the mesh's faces and renders them.
	// This code renders quad meshes.  Adapt to render triangle meshes.
	for (Mesh::FaceIter f_it = mesh->faces_begin(); f_it != mesh->faces_end(); ++f_it) {
		OpenMesh::Vec3f point[4];
		Mesh::ConstFaceVertexIter cfv_it;
		cfv_it = mesh->cfv_iter(*f_it);
		point[0] = mesh->point(*cfv_it);
		point[1] = mesh->point(*(++cfv_it));
		point[2] = mesh->point(*(++cfv_it));
		point[3] = mesh->point(*(++cfv_it));

		glBegin(GL_LINE_LOOP);
		glVertex3f(point[0][0], point[0][1], point[0][2]);
		glVertex3f(point[2][0], point[2][1], point[2][2]);
		glVertex3f(point[1][0], point[1][1], point[1][2]);
		glVertex3f(point[3][0], point[3][1], point[3][2]);
		glEnd();
	}

	// currently we've been drawing to the back buffer, we need
	// to swap the back buffer with the front one to make the image visible
	glutSwapBuffers();
}

void reshape(int w, int h)
{
	windowWidth = w;
	windowHeight = h;

	// flag glut to redraw the screen
	glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
	switch (key) {

		// increase the LOD
	case '+':
	case '=':
		++LOD;
		mesh = generateMesh();
		break;

		// decrease the LOD
	case '-':
	case '_':
		--LOD;
		// have a minimum level of depth
		if (LOD < 3)
			LOD = 3;

		mesh = generateMesh();
		break;
	case 'q':
		exit(0);

	default:
		break;
	}

	// flag glut to redraw the screen
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) leftDown = (state == GLUT_DOWN);
	else if (button == GLUT_RIGHT_BUTTON) rightDown = (state == GLUT_DOWN);
	else if (button == GLUT_MIDDLE_BUTTON) middleDown = (state == GLUT_DOWN);

	lastPos[0] = x;
	lastPos[1] = y;
}

// hold left mouse button and drag - rotation
// hold middle mouse button and drag - pan
// hold right mouse button and drag - zoom
void mouseMoved(int x, int y) {
	const float speed = 30.0f;

	int dx = x - lastPos[0];
	int dy = y - lastPos[1];
	Vec3f curCamera(cameraPos[0], cameraPos[1], cameraPos[2]);
	Vec3f curCameraNormalized = curCamera.normalized();
	Vec3f right = up % curCameraNormalized;

	if (middleDown || (leftDown && rightDown)) {
		pan += -speed * (float)((float)dx / (float)windowWidth) * right +
			speed * (float)((float)dy / (float)windowHeight) * up;
	}
	else if (leftDown) {
		// Assume here that up vector is (0,1,0)
		Vec3f newPos = curCamera - speed * (float)((float)dx / (float)windowWidth) * right +
			speed * (float)((float)dy / (float)windowHeight) * up;
		newPos = newPos.normalized() * curCamera.length();

		up = up - (up | newPos) * newPos / newPos.sqrnorm();
		up.normalize();

		for (int i = 0; i < 3; i++) cameraPos[i] = newPos[i];
	}
	else if (rightDown) {
		for (int i = 0; i < 3; i++) cameraPos[i] *= pow(1.1, dy * 0.1);
	}

	lastPos[0] = x;
	lastPos[1] = y;

	// flag glut to redraw the screen
	glutPostRedisplay();
}

int main(int argc, char** argv) {

	mesh = generateMesh();
	up = Vec3f(0, 1, 0);
	pan = Vec3f(0, 0, 0);

	// initialise glut
	glutInit(&argc, argv);

	// request a depth buffer, RGBA display mode, and we want double buffering
	glutInitDisplayMode(GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE);

	// set the initial window size
	glutInitWindowSize(windowWidth, windowHeight);

	// create the window
	glutCreateWindow("Tensor Product Surface: +/- to Change Level of Detail");

	// set the function to use to draw our scene
	glutDisplayFunc(display);

	// set the function to handle changes in screen size
	glutReshapeFunc(reshape);

	// set the function for the key presses
	glutKeyboardFunc(keyboard);

	// set the function for mouse click
	glutMouseFunc(mouse);

	// set the function for mouse move
	glutMotionFunc(mouseMoved);

	// this function runs a while loop to keep the program running.
	glutMainLoop();
	return 0;
}
