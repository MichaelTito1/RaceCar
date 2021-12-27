#include <glut.h>

float rotAng;

void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glRotatef(rotAng, 0, 1, 0);
	glColor3f(0.0f, 0.0f, 0.0f);
	glutSolidCube(1);
	glPopMatrix();

	glPushMatrix();
	glRotatef(-rotAng, 0, 1, 0);
	glTranslatef(2, 0, 0);
	glRotatef(rotAng, 1, 0, 0);
	glColor3f(0.5f, 0.5f, 0.5f);
	glutSolidSphere(0.5, 25, 25);
	glPopMatrix();

	glFlush();
}

void Anim() {
	rotAng += 0.01;

	glutPostRedisplay();
}

void main(int argc, char** argv) {
	glutInit(&argc, argv);

	glutInitWindowSize(300, 300);
	glutInitWindowPosition(150, 150);

	glutCreateWindow("OpenGL - 3D Template");
	glutDisplayFunc(Display);
	glutIdleFunc(Anim);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, 300 / 300, 0.1f, 300.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 2.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glutMainLoop();
}
