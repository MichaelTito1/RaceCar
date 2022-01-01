#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <math.h>
#include <glut.h>
#include <iostream>
#include <string>

#define DEG2RAD(a) (a * 0.0174532925)

int WIDTH = 1200;
int HEIGHT = 700;
int lives = 3;
GLuint tex;
char title[] = "3D Model Loader Sample";
bool gameover = false, winner = false;

// 3D Projection Options
GLdouble fovy = 45;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 1000;

class Vector3f {
public:
	float x, y, z;

	Vector3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) {
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3f operator+(Vector3f& v) {
		return Vector3f(x + v.x, y + v.y, z + v.z);
	}

	Vector3f operator-(Vector3f& v) {
		return Vector3f(x - v.x, y - v.y, z - v.z);
	}

	Vector3f operator*(float n) {
		return Vector3f(x * n, y * n, z * n);
	}

	Vector3f operator/(float n) {
		return Vector3f(x / n, y / n, z / n);
	}

	Vector3f unit() {
		return *this / sqrt(x * x + y * y + z * z);
	}

	Vector3f cross(Vector3f v) {
		return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
};

class Camera {
public:
	Vector3f eye, center, up;
	bool fps;

	Camera() {
		up = Vector3f(0, 1, 0);
		center = Vector3f(0, 0, 10); // adjust camera tps
		eye = Vector3f(0, 5, 30);
		fps = false;
	}

	void moveX(float d) {
		/*Vector3f right = up.cross(center - eye).unit();
		eye = eye + right * d;
		center = center + right * d;*/
		eye.x += d;
		center.x += d;
	}

	void moveY(float d) {
		/*eye = eye + up.unit() * d;
		center = center + up.unit() * d;*/
		eye.y += d;
		center.y += d;
	}

	void moveZ(float d) {
		/*Vector3f view = (center - eye).unit();
		eye = eye + view * d;
		center = center + view * d;*/
		eye.z += d;
		center.z += d;
	}

	void rotateX(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + up * sin(DEG2RAD(a));
		up = view.cross(right);
		center = eye + view;
	}

	void rotateY(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + right * sin(DEG2RAD(a));
		right = view.cross(up);
		center = eye + view;
	}

	void look() {
		gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z
		);
	}
};

Camera camera = Camera();

// Textures
GLTexture tex_ground;


//=======================================================================
// Material Configuration Function
//======================================================================
void InitMaterial()
{
	// Enable Material Tracking
	glEnable(GL_COLOR_MATERIAL);

	// Sich will be assigneet Material Properties whd by glColor
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// Set Material's Specular Color
	// Will be applied to all objects
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

	// Set Material's Shine value (0->128)
	GLfloat shininess[] = { 96.0f };
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

//=======================================================================
// Render Ground Function
//=======================================================================
void RenderGround()
{
	glDisable(GL_LIGHTING);	// Disable lighting 

	glColor3f(0.6, 0.6, 0.6);	// Dim the ground texture a bit

	glEnable(GL_TEXTURE_2D);	// Enable 2D texturing

	glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]);	// Bind the ground texture

	glPushMatrix();
	glScalef(5.0, 5.0, 5.0);
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);	// Set quad normal direction.
	glTexCoord2f(0, 0);		// Set tex coordinates ( Using (0,0) -> (5,5) with texture wrapping set to GL_REPEAT to simulate the ground repeated grass texture).
	glVertex3f(-20, 0, -20);
	glTexCoord2f(5, 0);
	glVertex3f(20, 0, -20);
	glTexCoord2f(5, 5);
	glVertex3f(20, 0, 20);
	glTexCoord2f(0, 5);
	glVertex3f(-20, 0, 20);
	glEnd();
	glPopMatrix();

	glEnable(GL_LIGHTING);	// Enable lighting again for other entites coming throung the pipeline.

	glColor3f(1, 1, 1);	// Set material back to white instead of grey used for the ground texture.
}

// for collision detection
float distance(float x1, float z1, float x2, float z2) {
	return (x1 - x2) * (x1 - x2) + (z1 - z2) * (z1 - z2);
}

class Car {
public:
	Model_3DS model;
	Vector3f position, rotation, scale, front, lightDir, auxAxix;
	int score = 0;
	double gas = 100; // min=0, max=100

	float speed, sirenX, sirenZ;
	Car() {
		model.Load("Models/car/car.3ds");
		position = Vector3f(0, 0, 20);
		front = Vector3f(0, 0, 10);
		rotation = Vector3f(0, 0, 0);
		scale = Vector3f(0.01, 0.01, 0.01);
		speed = 0;
		sirenX = 1;
		sirenZ = -1;
	}
	Car(Vector3f _position, Vector3f _rotation, Vector3f _scale, Vector3f _front, float _speed) {
		model.Load("Models/car/car.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
		front = _front;
		speed = _speed;
		sirenX = 1;
		sirenZ = -1;
	}
	void draw() {

		glColor3f(1, 0, 0);
		glPushMatrix();
		glTranslated(position.x, 1.7, position.z);
		glRotatef(90, 1, 0, 0);
		gluCylinder(gluNewQuadric(), 0.1, 0.2, 0.4, 20, 20);
		glPopMatrix();
		glColor3f(1, 1, 1);

		glPopMatrix();
		glPushMatrix();
		glTranslated(position.x, position.y, position.z);
		glScaled(scale.x, scale.y, scale.z);
		glRotatef(rotation.z, 0, 0, 1);
		glRotatef(180 + rotation.y, 0, 1, 0);
		glRotatef(rotation.x, 1, 0, 0);
		model.Draw();
		glPopMatrix();

		//front car lights
		lightDir = (front - position).unit();
		auxAxix = lightDir.cross(Vector3f(0, 1, 0));

		//right car light
		GLfloat l3Diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat l3Spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat l3Position[] = { position.x + 2 * auxAxix.x, 1, position.z + 2 * auxAxix.z, true };
		GLfloat l3Direction[] = { lightDir.x, 0, lightDir.z };
		glLightfv(GL_LIGHT3, GL_DIFFUSE, l3Diffuse);
		glLightfv(GL_LIGHT3, GL_SPECULAR, l3Spec);
		glLightfv(GL_LIGHT3, GL_POSITION, l3Position);
		glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 30);
		glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 120);
		glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, l3Direction);
		//try to make light fade as it 

		//left car light
		GLfloat l4Position[] = { position.x - 2 * auxAxix.x, 1, position.z - 2 * auxAxix.z, true };
		glLightfv(GL_LIGHT4, GL_DIFFUSE, l3Diffuse);
		glLightfv(GL_LIGHT4, GL_SPECULAR, l3Spec);
		glLightfv(GL_LIGHT4, GL_POSITION, l4Position);
		glLightf(GL_LIGHT4, GL_SPOT_CUTOFF, 30);
		glLightf(GL_LIGHT4, GL_SPOT_EXPONENT, 120);
		glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, l3Direction);
		//try to make light fade as it 


		//siren
		GLfloat l5Diffuse[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		GLfloat l5Position[] = { position.x, 5, position.z, true };
		GLfloat l5Direction[] = { sirenX, -20, sirenZ };
		glLightfv(GL_LIGHT5, GL_DIFFUSE, l5Diffuse);
		glLightfv(GL_LIGHT5, GL_POSITION, l5Position);
		glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, 30);
		glLightf(GL_LIGHT5, GL_SPOT_EXPONENT, 80);
		glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, l5Direction);

	}
};

Car car;

class Box {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;
	bool visible;
	Box() {
		model.Load("Models/box1.3ds");
		position = Vector3f(10, 0, 10);
		rotation = Vector3f(0, 0, 0);
		scale = Vector3f(5, 5, 5);
		visible = true;
	}
	Box(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/box1.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
		visible = true;
	}
	void draw() {
		if (visible) {
			//glColor3f(1, 1, 0);
			glPopMatrix();
			glPushMatrix();
			glTranslated(position.x, position.y, position.z);
			glScaled(scale.x, scale.y, scale.z);
			glRotatef(rotation.z, 0, 0, 1);
			glRotatef(rotation.y, 0, 1, 0);
			glRotatef(rotation.x, 1, 0, 0);
			model.Draw();
			glPopMatrix();

			Vector3f carFrontCenter = car.position + car.lightDir;
			Vector3f carBackCenter = car.position - car.lightDir;
			if (distance(position.x, position.z, car.position.x, car.position.z) <= 4 ||
				distance(position.x, position.z, carFrontCenter.x, carFrontCenter.z) <= 4 ||
				distance(position.x, position.z, carBackCenter.x, carBackCenter.z) <= 3) {
				//detected collision with the car
				visible = false;
			}
		}
	}

};

class Coin {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;
	bool visible;
	Coin() {
		model.Load("Models/coin2.3ds");
		position = Vector3f(10, 1, 5);
		rotation = Vector3f(0, 0, 90);
		scale = Vector3f(1, 1, 1);
		visible = true;
	}
	Coin(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/coin2.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
		visible = true;
	}
	void draw() {
		if (visible) {
			glColor3f(1, 1, 0);
			glPopMatrix();
			glPushMatrix();
			glTranslated(position.x, position.y, position.z);
			glScaled(scale.x, scale.y, scale.z);
			glRotatef(rotation.z, 0, 0, 1);
			glRotatef(rotation.y, 0, 1, 0);
			glRotatef(rotation.x, 1, 0, 0);
			model.Draw();
			glPopMatrix();
			glColor3f(1, 1, 1);

			Vector3f carFrontCenter = car.position + car.lightDir;
			Vector3f carBackCenter = car.position - car.lightDir;
			if (distance(position.x, position.z, car.position.x, car.position.z) <= 4 ||
				distance(position.x, position.z, carFrontCenter.x, carFrontCenter.z) <= 4 ||
				distance(position.x, position.z, carBackCenter.x, carBackCenter.z) <= 3) {
				//detected collision with the car
				visible = false;
				car.score++;
			}
		}
	}

};


class Tank {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;
	bool visible;
	Tank() {
		model.Load("Models/tank.3ds");
		position = Vector3f(15, 4, -10);
		rotation = Vector3f(0, 45, 0);
		scale = Vector3f(0.03, 0.03, 0.03);
		visible = true;
	}
	Tank(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/tank.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
		visible = true;
	}
	void draw() {
		if (visible) {
			glColor3f(1, 0, 0);
			glPopMatrix();
			glPushMatrix();
			glTranslated(position.x, position.y, position.z);
			glScaled(scale.x, scale.y, scale.z);
			glRotatef(rotation.z, 0, 0, 1);
			glRotatef(rotation.y, 0, 1, 0);
			glRotatef(rotation.x, 1, 0, 0);
			model.Draw();
			glPopMatrix();
			glColor3f(1, 1, 1);

			Vector3f carFrontCenter = car.position + car.lightDir;
			Vector3f carBackCenter = car.position - car.lightDir;
			if (distance(position.x - 5, position.z + 12, car.position.x, car.position.z) <= 2 ||
				distance(position.x - 5, position.z + 12, carFrontCenter.x, carFrontCenter.z) <= 3 ||
				distance(position.x - 5, position.z + 12, carBackCenter.x, carBackCenter.z) <= 2) {
				//detected collision with the car
				visible = false;
				car.gas = min(100, car.gas + 30);
			}
		}
	}

};


class Road {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;

	Road() {
		model.Load("Models/city/road/untitled.3ds");
		position = Vector3f(0, -1, 5);
		rotation = Vector3f(0, 90, 0);
		scale = Vector3f(1.2, 1, 1);
	}
	Road(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/city/road/untitled.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
	}
	void draw() {
		glPushMatrix();
		glTranslated(position.x, position.y, position.z);
		glScaled(scale.x, scale.y, scale.z);
		glRotatef(rotation.z, 0, 0, 1);
		glRotatef(180 + rotation.y, 0, 1, 0);
		glRotatef(rotation.x, 1, 0, 0);
		model.Draw();
		glPopMatrix();
	}

};




class Building {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;

	Building() {
		model.Load("Models/house/house.3ds");
		position = Vector3f(20, -1, 5);
		rotation = Vector3f(90, 90, 0);
		scale = Vector3f(2, 2, 2);
	}
	Building(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/house/house.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
	}
	void draw() {
		glPushMatrix();
		glTranslated(position.x, position.y, position.z);
		glScaled(scale.x, scale.y, scale.z);
		glRotatef(rotation.z, 0, 0, 1);
		glRotatef(180 + rotation.y, 0, 1, 0);
		glRotatef(rotation.x, 1, 0, 0);
		model.Draw();
		glPopMatrix();
	}
};
class Tree {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;

	Tree() {
		model.Load("Models/tree/Tree1.3ds");
		position = Vector3f(20, -1, 5);
		rotation = Vector3f(90, 90, 0);
		scale = Vector3f(2, 2, 2);
	}
	Tree(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/tree/Tree1.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
	}
	void draw() {
		glPushMatrix();
		glTranslatef(10, 0, 0);
		glScalef(0.7, 0.7, 0.7);
		model.Draw();
		glPopMatrix();
	}
};






void setupCamera() {
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	gluPerspective(fovy, aspectRatio, zNear, zFar);
	//*******************************************************************************************//
	// fovy:			Angle between the bottom and top of the projectors, in degrees.			 //
	// aspectRatio:		Ratio of width to height of the clipping plane.							 //
	// zNear and zFar:	Specify the front and back clipping planes distances from camera.		 //
	//*******************************************************************************************//

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	camera.look();
}

void setupLights() {
	GLfloat lmodel_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	GLfloat l0Diffuse[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	GLfloat l0Spec[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	GLfloat l0Ambient[] = { 0.1f, 0.0f, 0.0f, 1.0f };
	GLfloat l0Position[] = { 10.0f, 20.0f, 10, true };
	GLfloat l0Direction[] = { -1.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0Ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, l0Position);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 90.0);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, l0Direction);

	GLfloat l1Diffuse[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	GLfloat l1Ambient[] = { 0.0f, 0.1f, 0.0f, 1.0f };
	GLfloat l1Position[] = { 0.0f, 20.0f, 10, true };
	GLfloat l1Direction[] = { 0.0, -1.0, 0.0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Diffuse);
	glLightfv(GL_LIGHT1, GL_AMBIENT, l1Ambient);
	glLightfv(GL_LIGHT1, GL_POSITION, l1Position);
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 30.0);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 90.0);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, l1Direction);

	GLfloat l2Diffuse[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	GLfloat l2Ambient[] = { 0.0f, 0.0f, 0.1f, 1.0f };
	GLfloat l2Position[] = { 30.0f, 2.0f, 10, true };
	GLfloat l2Direction[] = { 0.0, 0.0, -1.0 };
	glLightfv(GL_LIGHT2, GL_DIFFUSE, l2Diffuse);
	glLightfv(GL_LIGHT2, GL_AMBIENT, l2Ambient);
	glLightfv(GL_LIGHT2, GL_POSITION, l2Position);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0);
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 90.0);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, l2Direction);
}



Box box;
Coin coin , coin2;
Tank tank;
Road road;
Building building;
Tree tree;



//=======================================================================
// OpengGL Configuration Function
//=======================================================================
void myInit(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);

	//InitLightSource();

	InitMaterial();

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_NORMALIZE);

	car = Car();
	box = Box();
	coin = Coin(Vector3f(3, 1, 7), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
	coin2 = Coin(Vector3f(0, 1, -5), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
	tank = Tank(Vector3f(8, 4, -35), Vector3f(0, 45, 0), Vector3f(0.03, 0.03, 0.03));
	road = Road(Vector3f(0, -1, -20), Vector3f(0, 90, 0), Vector3f(2.5, 1, 5));
	//building = Building();
	tree = Tree();
	
	
}

//=======================================================================
// Display Function
//=======================================================================
void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setupLights();
	setupCamera();


	GLfloat lightIntensity[] = { 0.7, 0.7, 0.7, 1.0f };
	GLfloat lightPosition[] = { 0.0f, 100.0f, 0.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

	// Draw Ground
	RenderGround();

	car.draw();
	box.draw();
	coin.draw();
	coin2.draw();
	tank.draw();
	road.draw();
	//building.draw();

	

	//drawing the trees on the right side 
	//tree.draw();

	/*glPushMatrix();
	glTranslated(5, 0, 2);
	tree.draw();
	glPopMatrix();*/

	glPushMatrix();
	glTranslated(5, 0, -10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(5, 0, -25);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, -10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, -25);
	tree.draw();
	glPopMatrix();


	glPushMatrix();
	glTranslated(5, 0, 10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(5, 0, 25);
	tree.draw();
	glPopMatrix();


	glPushMatrix();
	glTranslated(5, 0, 40);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(5, 0, 55);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, 55);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, 40);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, 25);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, 10);
	tree.draw();
	glPopMatrix();


	
	
	//////////////////////////////////////
	//drawing the trees on the left side

	glPushMatrix();
	glTranslated(-30, 0, -10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-30, 0,-20);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, -10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, -20);
	tree.draw();
	glPopMatrix();



	glPushMatrix();
	glTranslated(-30, 0, 10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-30, 0, 20);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-30, 0, 30);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-30, 0, 40);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, 10);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, 20);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, 30);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, 40);
	tree.draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-20, 0, 55);
	tree.draw();
	glPopMatrix();


	//sky box
	glPushMatrix();

	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glTranslated(50, 0, 0);
	glRotated(90, 1, 0, 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);
	gluSphere(qobj, 100, 100, 100);
	gluDeleteQuadric(qobj);


	glPopMatrix();



	glutSwapBuffers();
}

//=======================================================================
// Keyboard Function
//=======================================================================
void myKeyboard(unsigned char button, int x, int y)
{
	if (!gameover && !winner) {
		float frontX, frontZ;
		Vector3f eye, center;
		switch (button)
		{
		case 'w':
			car.gas = max(0, car.gas - 0.3);
			car.speed += 0.05;
			if (car.speed > 0.5) car.speed = 0.5;
			break;
		case 's':
			car.gas = max(0, car.gas - 0.3);
			car.speed -= 0.05;
			if (car.speed < -0.2) car.speed = -0.2;
			break;
		case 'd':
			car.rotation.y -= 5;
			car.front.x -= car.position.x; //translate to coincide the car center with origin
			car.front.z -= car.position.z;
			frontX = car.front.x; frontZ = car.front.z;
			car.front.x = frontX * 0.9961946981 - frontZ * 0.08715574275; //rotate -5 degrees
			car.front.z = frontX * 0.08715574275 + frontZ * 0.9961946981;
			car.front.x += car.position.x; //translate back
			car.front.z += car.position.z;
			if (camera.fps) {
				camera.eye = car.position + Vector3f(0, 5, 0);
				camera.center = car.front + (car.front - car.position) * 3;
			}
			else {
				camera.center = car.front; // adjust camera tps
				camera.eye = car.position * 2 - car.front + Vector3f(0, 5, 0);
			}
			break;
		case 'a':
			car.rotation.y += 5;
			car.front.x -= car.position.x; //translate to coincide the car center with origin
			car.front.z -= car.position.z;
			frontX = car.front.x; frontZ = car.front.z;
			car.front.x = frontX * 0.9961946981 + frontZ * 0.08715574275; //rotate -5 degrees
			car.front.z = -frontX * 0.08715574275 + frontZ * 0.9961946981;
			car.front.x += car.position.x; //translate back
			car.front.z += car.position.z;
			if (camera.fps) {
				camera.eye = car.position + Vector3f(0, 5, 0);
				camera.center = car.front + (car.front - car.position) * 3;
			}
			else {
				camera.center = car.front; // adjust camera tps
				camera.eye = car.position * 2 - car.front + Vector3f(0, 5, 0);
			}
			break;
		case 'v': //switch between fps and tps
			if (!camera.fps) {
				camera.eye = car.position + Vector3f(0, 5, 0);
				camera.center = car.front + (car.front - car.position) * 3;
			}
			else {
				camera.center = car.front; // adjust camera tps
				camera.eye = car.position * 2 - car.front + Vector3f(0, 5, 0);
			}
			camera.fps = !camera.fps;
			break;
		case '0':
			//camera.moveZ(2);
			//tank.position.x++;
			//coin.position.x++;
			//box.position.x++;
			//glDisable(GL_LIGHTING);
			break;
		case '1':
			//camera.moveZ(-2);
			//tank.position.x--;
			//coin.position.x--;
			//box.position.x--;
			break;
		case '2':
			//camera.moveY(2);
			//tank.position.z++;
			//coin.position.z++;
			//box.position.z++;
			break;
		case '3':
			//camera.moveY(-2);
			//tank.position.z--;
			//coin.position.z--;
			//box.position.z--;
			break;
		case 27:
			exit(0);
			break;
		default:
			break;
		}
	}
	glutPostRedisplay();
}

// prints text in 3D
void output(std::string string1, float x, float y, float z)
{
	glColor3f(0, 0, 0);
	glRasterPos3f(x, y, z);
	int len, i;
	len = string1.size();
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string1[i]);
	}
}


void time(int val) {
	// check if winner
	if (winner) {
		output("Winner!", 0, 0, 0); // TODO: not working
	}

	// check if out of lives 
	if (lives <= 0 || car.gas <= 0) {
		gameover = 1;
		output("Game Over", car.position.x, 0, car.position.z); // TODO: not working
	}

	// if out of gas, decrement lives and give some more gas to the player
	if (car.gas <= 0) {
		lives--;
		car.gas = 30;
	}

	// check if out of boundaries
	int x = car.position.x, z = car.position.z;
	if ((x > 4.5 && x < 61 && z >= -64 && z < 26) || !(x > -1.5 && x < 66 && z > -70 && z < 30)) {

		lives = 0; // game over if out of the track boundaries
	}

	// check if the player reached the finish line
	if (lives > 0 && car.gas > 0 && (x > 1.5 && x < 4 && z > 25 && z < 29))
		winner = 1;
	if (car.speed != 0) {
		Vector3f deltaD = (car.front - car.position).unit() * car.speed;
		car.position = car.position + deltaD;
		car.front = car.front + deltaD;
		camera.center = camera.center + deltaD;
		camera.eye = camera.eye + deltaD;
		if (car.speed > 0) car.speed -= 0.01;
		if (car.speed < 0) car.speed += 0.01;
	}

	if (coin.rotation.x == 0) coin.rotation.x = 360;
	coin.rotation.x -= 2;

	if (coin2.rotation.x == 0) coin2.rotation.x = 360;
	coin2.rotation.x -= 2;

	/*car.sirenX -= car.position.x;
	car.sirenZ -= car.position.z;*/
	float xTmp = car.sirenX, zTmp = car.sirenZ;
	car.sirenX = xTmp * 0.9961946981 - zTmp * 0.08715574275; //rotate -5 degrees
	car.sirenZ = xTmp * 0.08715574275 + zTmp * 0.9961946981;
	/*car.sirenX += car.position.x;
	car.sirenZ += car.position.z;*/

	std::cout << car.position.x << ' ' << car.position.z << ' ' << car.score << ' ' << car.gas << '\n';

	glutPostRedisplay();
	glutTimerFunc(10, time, 0);
}

//=======================================================================
// Main Function
//=======================================================================
void main(int argc, char** argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(WIDTH, HEIGHT);

	glutInitWindowPosition(50, 0);

	glutCreateWindow(title);

	glutDisplayFunc(myDisplay);

	glutTimerFunc(0, time, 0);

	glutKeyboardFunc(myKeyboard);

	myInit();

	tex_ground.Load("Textures/ground.bmp");
	loadBMP(&tex, "Textures/blu-sky-3.bmp", true);

	

	

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);

	glutMainLoop();
}