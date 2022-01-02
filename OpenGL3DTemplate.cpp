#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <math.h>
#include <glut.h>
#include <iostream>
#include <string>
#include <Windows.h>
#include <mmsystem.h>


#define DEG2RAD(a) (a * 0.0174532925)

int WIDTH = 1200;
int HEIGHT = 700;
int lives = 3;
GLuint tex;
char title[] = "3D Model Loader Sample";
bool gameover = false, winner = false, winnerSound = false;
double timeLeft = 30;
int level = 1;

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
		center = Vector3f(0,0,10); // adjust camera tps
		eye = Vector3f(0,5,30);
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

Camera camera; 

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
	//glDisable(GL_LIGHTING);	// Disable lighting 

	glColor3f(0.6, 0.6, 0.6);	// Dim the ground texture a bit

	glEnable(GL_TEXTURE_2D);	// Enable 2D texturing

	glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]);	// Bind the ground texture

	glPushMatrix();
	glScalef(6.0, 6.0, 6.0);
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

	//glEnable(GL_LIGHTING);	// Enable lighting again for other entites coming throung the pipeline.

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
		position = Vector3f(0,0,20);
		front = Vector3f(0,0,10);
		rotation = Vector3f(0,0,0);
		scale = Vector3f(0.01,0.01,0.01);
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
		scale = Vector3f(5,5,5);
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
				distance(position.x, position.z, carBackCenter.x, carBackCenter.z) <= 3){
				//detected collision with the car
				visible = false;
				lives--;	
				PlaySound(TEXT("sound/hit.wav"), NULL, SND_ASYNC);
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
		scale = Vector3f(1,1,1);
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
				PlaySound(TEXT("sound/point.wav"), NULL, SND_ASYNC);
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
		position = Vector3f(15,4, -10);
		rotation = Vector3f(0, 45, 0);
		scale = Vector3f(0.03,0.03,0.03);
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
				PlaySound(TEXT("sound/point.wav"), NULL, SND_ASYNC);
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

class Tower {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;

	Tower() {
		model.Load("Models/city/buildings/3ds/001.3ds");
		position = Vector3f(10, 1, 5);
		rotation = Vector3f(0, 0, 0);
		scale = Vector3f(1, 1, 1);
	}

	Tower(char* towerNum) {
		char res[100];
		strcpy(res, "Models/city/buildings/3ds/");
		strcat(res, towerNum);
		strcat(res, ".3ds");
		model.Load(res);
		position = Vector3f(17, 0, 5);
		rotation = Vector3f(0, 0, 0);
		scale = Vector3f(1, 1, 1);
	}
	Tower(char* towerNum, Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		char res[100];
		strcpy(res, "Models/city/buildings/3ds/");
		strcat(res, towerNum);
		strcat(res, ".3ds");
		model.Load(res);
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

class Heart {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;
	bool visible;
	Heart() {
		model.Load("Models/heart.3ds");
		position = Vector3f(10, 1.3, -5);
		rotation = Vector3f(90, -90, 0);
		scale = Vector3f(0.2, 0.2, 0.2);
		visible = true;
	}
	Heart(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/heart.3ds");
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
			if (distance(position.x, position.z, car.position.x, car.position.z) <= 4 ||
				distance(position.x, position.z, carFrontCenter.x, carFrontCenter.z) <= 6 ||
				distance(position.x, position.z, carBackCenter.x, carBackCenter.z) <= 3) {
				//detected collision with the car
				visible = false;
				lives++;
				PlaySound(TEXT("sound/point.wav"), NULL, SND_ASYNC);
			}
		}
	}

};

class Rock {
public:
	Model_3DS model;
	Vector3f position, rotation, scale;
	bool visible;
	Rock() {
		model.Load("Models/rock.3ds");
		position = Vector3f(8, 0, -10);
		rotation = Vector3f(0, 0, 0);
		scale = Vector3f(0.07, 0.07, 0.07);
		visible = true;
	}
	Rock(Vector3f _position, Vector3f _rotation, Vector3f _scale) {
		model.Load("Models/rock.3ds");
		position = _position;
		rotation = _rotation;
		scale = _scale;
		visible = true;
	}
	void draw() {
		if (visible) {
			glColor3f(0.5, 0.2, 0);
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
			if (distance(position.x + 2, position.z - 4, car.position.x, car.position.z) <= 3 ||
				distance(position.x + 2, position.z - 4, carFrontCenter.x, carFrontCenter.z) <= 4 ||
				distance(position.x + 2, position.z - 4, carBackCenter.x, carBackCenter.z) <= 3) {
				//detected collision with the car
				visible = false;
				lives--;
				PlaySound(TEXT("sound/hit.wav"), NULL, SND_ASYNC);
			}
		}
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

Heart heart;
Rock rock1, rock2;
Box box, box2;
Coin coin, coin2, coin3, coin4;
Tank tank;
Road road1, road2, road3, road4, road;
Tower tower1, tower2, tower3, tower4;
Tree tree;

void declareBuildings() {
	tower1 = Tower("001");
	tower2 = Tower("002", Vector3f(45, 0, 10), Vector3f(0, 90, 0), Vector3f(1, 1, 1));
	tower3 = Tower("003", Vector3f(17, 0, -40), Vector3f(0, 90, 0), Vector3f(1, 1, 1));
	tower4 = Tower("004",Vector3f(45, 0, -40), Vector3f(0, 90, 0), Vector3f(1, 1, 1));
}

void declareRoads() {
	road1 = Road(Vector3f(0, -1, -19), Vector3f(0, 90, 0), Vector3f(1.8,1,2.95));
	road2 = Road(Vector3f(32, -1, -68), Vector3f(0, 0, 0), Vector3f(2, 1, 1.8));
	road3 = Road(Vector3f(65, -1, -24), Vector3f(0, 90, 0), Vector3f(1.8, 1, 3.1));
	road4 = Road(Vector3f(35, -1, 24), Vector3f(0, 0, 0), Vector3f(1.8, 1, 1.8));
}

//=======================================================================
// OpengGL Configuration Function
//=======================================================================
void myInit(void)
{
	camera = Camera();
	glClearColor(0.0, 0.0, 0.0, 0.0);

	InitMaterial();

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_NORMALIZE);

	if (level == 2) {
		glDisable(GL_LIGHT0);
		car = Car();
		box = Box(Vector3f(0, 0, -17), Vector3f(0, 0, 0), Vector3f(4, 4, 4));
		box2 = Box(Vector3f(64, 0, 6), Vector3f(0, 0, 0), Vector3f(4, 4, 4));
		heart = Heart(Vector3f(67, 1.3, -17), Vector3f(0, 0, 0), Vector3f(0.2, 0.2, 0.2));
		rock1 = Rock(Vector3f(62, 0, -1), Vector3f(0, 0, 0), Vector3f(0.07, 0.07, 0.07));
		rock2 = Rock(Vector3f(62, 0, -50), Vector3f(0, 0, 0), Vector3f(0.07, 0.07, 0.07));
		coin = Coin(Vector3f(0, 1, 7), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		coin2 = Coin(Vector3f(33, 1, -66), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		coin3 = Coin(Vector3f(47, 1, -66), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		coin4 = Coin(Vector3f(14, 1, 25), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		tank = Tank(Vector3f(47, 4, 13), Vector3f(0, 45, 0), Vector3f(0.03, 0.03, 0.03));
		declareBuildings();
		declareRoads();
	}


	else {
		car = Car();
		road = Road(Vector3f(0, -1, -53), Vector3f(0, 90, 0), Vector3f(2.5, 1, 4.5));
		tree = Tree();

		box = Box(Vector3f(3, 0, -20), Vector3f(0, 0, 0), Vector3f(4, 4, 4));
		box2 = Box(Vector3f(3, 0, -5), Vector3f(0, 0, 0), Vector3f(4, 4, 4));
		coin = Coin(Vector3f(3, 1, 7), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		coin2 = Coin(Vector3f(0, 1, -5), Vector3f(0, 0, 90), Vector3f(1, 1, 1));
		tank = Tank(Vector3f(4, 4, -50), Vector3f(0, 45, 0), Vector3f(0.03, 0.03, 0.03));
		heart = Heart(Vector3f(2, 1, -17), Vector3f(0, 0, 0), Vector3f(0.2, 0.2, 0.2));
		rock1 = Rock(Vector3f(1, 0, -60), Vector3f(0, 0, 0), Vector3f(0.07, 0.07, 0.07));
	}
}

void setupLights() {
	GLfloat lmodel_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	GLfloat l0Diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat l0Spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat l0Ambient[] = { 0.3, 0.3, 0.3, 1.0f };
	GLfloat l0Position[] = { 0.0f, 0.0f, 0.0f, true };
	GLfloat l0Direction[] = { 0.0, -1, 0.0 };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0Ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, l0Position);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 90.0);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, l0Direction);
}

void output(std::string string, float x, float y);

//=======================================================================
// Display Function
//=======================================================================
void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setupCamera();

	// Draw Ground
	RenderGround();

	if (level == 2) {
		heart.draw();
		rock1.draw();
		rock2.draw();
		box.draw();
		box2.draw();
		coin.draw();
		coin2.draw();
		coin3.draw();
		coin4.draw();
		tank.draw();
		car.draw();
		// building draw
		tower1.draw();
		tower2.draw();
		tower3.draw();
		tower4.draw();

		//road.draw();
		road1.draw();
		road2.draw();
		road3.draw();
		road4.draw();
	}
	else {
		setupLights();
		car.draw();
		road.draw();

		box.draw();
		box2.draw();
		coin.draw();
		coin2.draw();
		tank.draw();
		heart.draw();
		rock1.draw();

		glPushMatrix();
		glTranslated(5, 0, -10);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(5, 0, -25);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(5, 0, -40);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(5, 0, -55);
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






		//////////////////////////////////////
		//drawing the trees on the left side
		// lines with -20 value in x corresponding to first left line
		// -30 the 2nd left line

		glPushMatrix();
		glTranslated(-20, 0, -10);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(-20, 0, -20);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(-20, 0, -35);
		tree.draw();
		glPopMatrix();

		glPushMatrix();
		glTranslated(-20, 0, -50);
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
	}

	//sky box
	glPushMatrix();

	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glTranslated(50, 0, 0);
	glRotated(90, 1, 0, 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);
	gluSphere(qobj, 200, 200, 200);
	gluDeleteQuadric(qobj);


	glPopMatrix();

	if (!winner && lives > 0) {
		std::string remainingTime = "Remaining time is :  " + std::to_string((int)timeLeft);
		std::string disLives = "Your Lives :  " + std::to_string((int)lives);
		std::string gas = "The gas level is :  " + std::to_string((int)car.gas);
		std::string score = "Your Score is :" + std::to_string((int)car.score);

		output(remainingTime, 20, 650.0);

		output(gas, 20, 600.0);

		output(disLives, 400, 650.0);
		output(score, 400, 600.0);
	}
	else if (winner) {
		output("Winner!", 520, 600.0); // TODO: not working

	}
	else {
		output("Game Over!", 520, 600.0); // TODO: not working

	}



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
			car.gas = max(0, car.gas- 0.3);
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
void output(std::string string, float x, float y)
{
	glDisable(GL_TEXTURE_2D); //added this
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, WIDTH, 0.0, HEIGHT);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glColor3f(1, 0, 0);
	glLoadIdentity();
	glRasterPos2i(x, y);
	void* font = GLUT_BITMAP_TIMES_ROMAN_24;
	for (std::string::iterator i = string.begin(); i != string.end(); ++i)
	{
		char c = *i;
		glColor3d(1.0, 0.0, 0.0);
		glutBitmapCharacter(font, c);
	}
	glMatrixMode(GL_PROJECTION); //swapped this with...
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW); //...this
	glPopMatrix();
	//added this
	glEnable(GL_TEXTURE_2D);
}

void time(int val) {
	timeLeft = max(timeLeft - 0.01, 0);
	// check if winner
	if (winner) {
		if(!winnerSound)
			PlaySound(TEXT("sound/winner.wav"), NULL, SND_ASYNC);
		winnerSound = true;
	}

	// check if out of lives 
	if (lives <= 0) {
		gameover = 1;
	}

	// if out of gas, decrement lives and give some more gas to the player
	if (car.gas <= 0) {
		lives--;
		car.gas = 30;
	}

	
	if (level == 2) {
		// check if out of boundaries
		int x = car.position.x, z = car.position.z;
		if (timeLeft <= 0 || (x > 4 && x < 60 && z >= -62 && z < 20) || !(x > -3.2 && x < 71 && z > -72 && z < 28)) {
			if (lives > 0)
				PlaySound(TEXT("sound/die.wav"), NULL, SND_ASYNC);
			lives = 0; // game over if out of the track boundaries
		}

		// check if the player collected all coins
		if (lives > 0 && car.gas > 0 && car.score == 4 /*(x > 4.5 && x < 5.5 && z > 21 && z < 29)*/)
			winner = 1;
	}
	else {
		// check if out of boundaries
		int x = car.position.x, z = car.position.z;
		if (!(x > -3.5 && x < 5.8  && z < 26)) {
			lives = 0; // game over if out of the track boundaries
		}

		// check if the player reached the finish line
		if (lives > 0 && car.gas > 0 && z <= -100) {
			//winner = 1;
			level = 2;
			myInit();
			//delay 3 seconds
			float end = clock() / CLOCKS_PER_SEC + 3;
			while ((clock() / CLOCKS_PER_SEC) < end);
		}
	}

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

	if (coin3.rotation.x == 0) coin3.rotation.x = 360;
	coin3.rotation.x -= 2;

	if (coin4.rotation.x == 0) coin4.rotation.x = 360;
	coin4.rotation.x -= 2;

	if (heart.rotation.y == 0) heart.rotation.y = 360;
	heart.rotation.y -= 2;

	/*car.sirenX -= car.position.x;
	car.sirenZ -= car.position.z;*/
	float xTmp = car.sirenX, zTmp = car.sirenZ;
	car.sirenX = xTmp * 0.9961946981  - zTmp * 0.08715574275; //rotate -5 degrees
	car.sirenZ = xTmp * 0.08715574275 + zTmp * 0.9961946981;
	/*car.sirenX += car.position.x;
	car.sirenZ += car.position.z;*/

	std::cout << car.position.x << ' ' << car.position.z << ' ' << car.score << ' ' << car.gas << ' ' << timeLeft << '\n';

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
	glEnable(GL_LIGHT3);
	glEnable(GL_LIGHT4);
	glEnable(GL_LIGHT5);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);

	glutMainLoop();
}