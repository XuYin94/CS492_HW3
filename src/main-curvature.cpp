#include <cmath>
#include <cstdio>
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include "curvature.h"

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
using namespace OpenMesh;
using namespace Eigen;


VPropHandleT<CurvatureInfo> curvature;
Mesh mesh;

bool leftDown = false, rightDown = false, middleDown = false;
int lastPos[2];
float cameraPos[4] = { 0, 0, 4, 1 };
Vec3f up, pan;
int windowWidth = 640, windowHeight = 480;
bool showSurface = true, showAxes = true, showCurvature = false, showNormals = false;

float specular[] = { 1.0, 1.0, 1.0, 1.0 };
float shininess[] = { 50.0 };

void renderMesh() {
  if (!showSurface) glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // render regardless to remove hidden lines

  glEnable(GL_LIGHTING);
  glLightfv(GL_LIGHT0, GL_POSITION, cameraPos);

  glDepthRange(0.001, 1);
  glEnable(GL_NORMALIZE);

  glBegin(GL_TRIANGLES);
  for (Mesh::ConstFaceIter f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it) {
    Mesh::ConstFaceVertexIter fv_it = mesh.cfv_begin(*f_it);
    for (int i = 0; fv_it != mesh.cfv_end(*f_it) && i < 3; ++fv_it, ++i) {
      const Mesh::VertexHandle n_vh = (*fv_it);
      const Vec3f p = mesh.point(n_vh);
      const Vec3f n = mesh.normal(n_vh);
      glVertex3f(p[0], p[1], p[2]);
      glNormal3f(n[0], n[1], n[2]);
    }
  }
  glEnd();

  if (!showSurface) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glDisable(GL_LIGHTING);
  glDepthRange(0, 0.999);

  if (showCurvature) {
    glBegin(GL_LINES);
    for (Mesh::ConstVertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
      const Mesh::VertexHandle vh = (*v_it);
      const CurvatureInfo c = mesh.property(curvature, vh);
      const Vec3f p = mesh.point(vh);

      for (int i = 0; i < 2; ++i) {
        if (i == 0) glColor3f(0, 0, 1);
        else glColor3f(1, 0, 0);
        const Vec3f d1 = p - c.directions[i] * 0.01;
        glVertex3f(d1[0], d1[1], d1[2]);
        const Vec3f d2 = p + c.directions[i] * 0.01;
        glVertex3f(d2[0], d2[1], d2[2]);
      }
    }
    glEnd();
  }

  if (showNormals) {
    glBegin(GL_LINES);
    glColor3f(0, 1, 0);
    for (Mesh::ConstVertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
      const Mesh::VertexHandle vh = (*v_it);
      const Vec3f n = mesh.normal(vh);
      const Vec3f p = mesh.point(vh);
      const Vec3f d = p + n * 0.01;
      glVertex3f(p[0], p[1], p[2]);
      glVertex3f(d[0], d[1], d[2]);
    }
    glEnd();
  }

  glDepthRange(0, 1);
}

void display() {
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
  glEnable(GL_LIGHT0);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, windowWidth, windowHeight);

  float ratio = (float)windowWidth / (float)windowHeight;
  gluPerspective(50, ratio, 1, 1000); // 50 degree vertical viewing angle, zNear = 1, zFar = 1000

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(cameraPos[0] + pan[0], cameraPos[1] + pan[1], cameraPos[2] + pan[2],
            pan[0], pan[1], pan[2], up[0], up[1], up[2]);

  // Draw mesh
  renderMesh();

  // Draw axes
  if (showAxes) {
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glLineWidth(1);
    glColor3f(1, 0 ,0); glVertex3f(0, 0, 0); glVertex3f(1, 0, 0); // x axis
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 1, 0); // y axis
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1); // z axis
    glEnd(/*GL_LINES*/);
  }

  glutSwapBuffers();
}

void reshape(int width, int height) {
  windowWidth = width;
  windowHeight = height;
  glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
  Vec3f actualCamPos(cameraPos[0] + pan[0], cameraPos[1] + pan[1], cameraPos[2] + pan[2]);

  if (key == 's' || key == 'S') showSurface = !showSurface;
  else if (key == 'a' || key == 'A') showAxes = !showAxes;
  else if (key == 'c' || key == 'C') showCurvature = !showCurvature;
  else if (key == 'n' || key == 'N') showNormals = !showNormals;
  else if (key == 'q' || key == 'Q') exit(0);
  glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) leftDown = (state == GLUT_DOWN);
  else if (button == GLUT_RIGHT_BUTTON) rightDown = (state == GLUT_DOWN);
  else if (button == GLUT_MIDDLE_BUTTON) middleDown = (state == GLUT_DOWN);

  lastPos[0] = x;
  lastPos[1] = y;
}

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
  } else if (leftDown) {
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

  glutPostRedisplay();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " mesh_filename\n";
    exit(0);
  }

  IO::Options opt;
  opt += IO::Options::VertexNormal;
  opt += IO::Options::FaceNormal;

  mesh.request_face_normals();
  mesh.request_vertex_normals();

  cout << "Reading from file " << argv[1] << "...\n";
  if ( !IO::read_mesh(mesh, argv[1], opt )) {
    cout << "Read failed.\n";
    exit(0);
  }

  mesh.update_normals();
  mesh.add_property(curvature);

  // Move center of mass to origin
  Vec3f center(0, 0, 0);
  for (Mesh::ConstVertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
    center += mesh.point(*v_it);
  }
  center /= mesh.n_vertices();

  for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
    mesh.point(*v_it) -= center;
  }

  // Fit in the unit sphere
  float maxLength = 0;
  for (Mesh::ConstVertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
    maxLength = max(maxLength, mesh.point(*v_it).length());
  }

  if (maxLength > 0) {
    for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
      mesh.point(*v_it) /= maxLength;
    }
  }

  computeCurvature(mesh,curvature);

  cout << "Mesh stats:\n";
  cout << '\t' << mesh.n_vertices() << " vertices.\n";
  cout << '\t' << mesh.n_edges() << " edges.\n";
  cout << '\t' << mesh.n_faces() << " faces.\n";

  up = Vec3f(0, 1, 0);
  pan = Vec3f(0, 0, 0);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow(argv[0]);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMoved);

  glutMainLoop();

  return 0;
}
