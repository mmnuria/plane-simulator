/*
* 
*	Simulator.cpp
* 
*	@author		Nuria Manzano Mata <nmanmat@upv.es>
*	@date		Enero,2023
* 
*/

#include <string>
#include <GL/glut.h>
#include <math.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "FastNoise.h"
#include "Utilidades.h"

using namespace std;

//Variables globales
const int EXTENSION = 400;
const int RESOLUCION_TERRENO = 200;
const float DESPLAZAMIENTO_RUIDO = 0.7;
const float ALTURA_MAX = 100;
const float SENSIBILIDAD = 0.05;
const float MOVIMIENTO_CABINA = 1.0;
const double ACELERACION = 0.01; //Almacenar valor de aceleracion
static const int tasaFPS = 60;
const float ANGULO_GIRO = 5.0f;

//Variables GLUT
GLdouble eye[3] = { 0, 0, 0};
GLdouble look[3] = { 1, 0, 0 };
GLdouble up[3] = { 0, 0, 1 };
GLuint nieve, agua, dia, noche;
GLuint cabina, bosque2;

//Variables de estado
bool botonPresionado = false;
bool PilotoActivo = false;
bool modoDiurno = true;
bool cabinaActiva = true;
bool LuzNocturna = false;
bool LuzFocal = false;
bool modoNiebla = true;

//Variables
float desplazamientoX;
float desplazamientoY;
float desplazamientoGiro = 0.0;
static float desplazamiento_velocidad = 0.0;
static float velocidad = 0.0;
float xanterior, yanterior;
float guinyada = 0.0;
float cabeceo = 0.0;
static float tiempoTranscurrido;
float altura_autopilot;

//Objeto para la generacion de terreno
FastNoiseLite noise;

//Método que genera el ruido de perlin (usando la biblioteca FastNoiseLite)
float Elevacion(float x, float y) {
	float resultado = (noise.GetNoise(x, y) + DESPLAZAMIENTO_RUIDO) * (ALTURA_MAX / (1 + DESPLAZAMIENTO_RUIDO));
	if (resultado < 0) {
		resultado = 0;
	}
	return resultado;
}
//Método que dibuja el terreno
void DibujarTerreno() {
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	for (int i = -RESOLUCION_TERRENO+eye[0]; i <= EXTENSION+eye[0]; i++) {
		for (int j = -RESOLUCION_TERRENO+eye[1]; j <= EXTENSION+eye[1]; j++) {

			glPushMatrix();

			float media_vertices = ((Elevacion(i, j))+ (Elevacion(i + 1, j)) + (Elevacion(i + 1, j + 1))) /3;
			
			if (media_vertices > ALTURA_MAX * 0.7) {
				//Color textura nieve
				glBindTexture(GL_TEXTURE_2D, nieve);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glShadeModel(GL_SMOOTH);
				
			}
			else if (media_vertices > ALTURA_MAX * 0.2) {
				//Color de textura a las montañas
				glBindTexture(GL_TEXTURE_2D, bosque2);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glShadeModel(GL_SMOOTH);
			}
			else{
				//Color textura agua
				glBindTexture(GL_TEXTURE_2D, agua);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glShadeModel(GL_SMOOTH);
				
			}

			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(0.0, 0.0);
			glVertex3f(i, j, Elevacion(i, j));
			glTexCoord2f(0.0, 1);
			glVertex3f(i + 1, j, Elevacion(i + 1, j));
			glTexCoord2f(1, 0.0);
			glVertex3f(i + 1, j + 1, Elevacion(i + 1, j + 1));
			glEnd();

			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(0.0, 1);
			glVertex3f(i + 1, j + 1, Elevacion(i + 1, j + 1));
			glTexCoord2f(1, 1);
			glVertex3f(i, j + 1, Elevacion(i, j + 1));
			glTexCoord2f(1, 0.0);
			glVertex3f(i, j, Elevacion(i, j));
			glEnd();

			glPopMatrix();
		}
	}
	glPopAttrib();

}
void init() {
	// Inicializaciones
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	//Activamos iluminaciones distintas
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);


	// Iluminación diurna
	const GLfloat A[] = { 0.2,0.2,0.2,1 };
	GLfloat dark[] = { 0.2, 0.15, 0.2, 1.0 };
	GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat direction[] = { 0.2, 1.0, 0.5, 0.0 };

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, A);
	glLightfv(GL_LIGHT0, GL_AMBIENT, dark);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
	glLightfv(GL_LIGHT0, GL_POSITION, direction);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	glShadeModel(GL_SMOOTH);

	// Iluminación nocturna movil
	GLfloat Ambiente[] = { 0.2,0.2,0.2,1.0 };
	GLfloat Difusa[] = { 1.0,1.0,1.0,1.0 };
	GLfloat Especular[] = { 0.3,0.3,0.3,1.0 };
	GLfloat direction1[] = { 0.2, 1.0, 0.5, 1.0 };

	glLightfv(GL_LIGHT1, GL_AMBIENT, Ambiente);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, Difusa);
	glLightfv(GL_LIGHT1, GL_SPECULAR, Especular);
	glLightfv(GL_LIGHT1, GL_POSITION, direction1);
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10.0);

	//Iluminacion nocturna estatica
	GLfloat Ambiente1[] = { 0.2,0.2,0.2,1.0 };
	GLfloat Difusa1[] = { 1.0,1.0,1.0,1.0 };
	GLfloat Especular1[] = { 0.3,0.3,0.3,1.0 };
	

	glLightfv(GL_LIGHT2, GL_AMBIENT, Ambiente1);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, Difusa1);
	glLightfv(GL_LIGHT2, GL_SPECULAR, Especular1);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 45.0);
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10.0);

	//Niebla
	GLfloat color[] = { 207 / 255,239 / 255,252 / 255,0.5 };
	glEnable(GL_FOG);
	glFogfv(GL_FOG_COLOR, color);
	glFogf(GL_FOG_DENSITY, 0.0035);
	glFogi(GL_FOG_MODE, GL_EXP2);

	//Material ------------------------------
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);
	glMaterialf(GL_FRONT, GL_SHININESS, 30);

	//TEXTURAS

	glGenTextures(1, &bosque2);
	glBindTexture(GL_TEXTURE_2D, bosque2);
	loadImageFile((char*)"cesped2.jpg");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &agua);
	glBindTexture(GL_TEXTURE_2D, agua);
	loadImageFile((char*)"agua.jpg");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &nieve);
	glBindTexture(GL_TEXTURE_2D, nieve);
	loadImageFile((char*)"nieve.jpg");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &dia);
	glBindTexture(GL_TEXTURE_2D, dia);
	loadImageFile((char*)"dia.jpg");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &noche);
	glBindTexture(GL_TEXTURE_2D, noche);
	loadImageFile((char*)"noche.jpg");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &cabina);
	glBindTexture(GL_TEXTURE_2D, cabina);
	loadImageFile((char*)"cockpit.png");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	

	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	eye[2] = Elevacion(0, 0) + 10;
	look[2] = eye[2];/*
	noise.SetFrequency(0.002);
	noise.SetFractalLacunarity(0.2);*/

	// Configurar el motor de render 
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHT0);
	glEnable(GL_TEXTURE_2D);
}
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Control piloto automatico
	if (PilotoActivo) {
		eye[2] = Elevacion(eye[0], eye[1]) + altura_autopilot;
		look[2] = eye[2];
	}

	// Seleccionar la MODELVIEW
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Establecer camara
	gluLookAt(eye[0], eye[1], eye[2], look[0], look[1], look[2], up[0], up[1], up[2]);
	
	//Control de la iluminacion y texturas de dia y noche
	if (modoDiurno) {
		glColor3f(1.0, 1.0, 1.0);
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT2);
		glDisable(GL_LIGHT1);
		glBindTexture(GL_TEXTURE_2D, dia);
		texturarFondo();
	}
	else {
		glDisable(GL_LIGHT0);
		glBindTexture(GL_TEXTURE_2D, noche);
		texturarFondo();

		//foco estatico cabina
		glEnable(GL_LIGHT2);
		GLfloat direction2[] = {cos(rad(desplazamientoGiro)),sin(rad(desplazamientoGiro)),0.0,1.0 };
		GLfloat posicion[] = {eye[0],eye[1],eye[2],1.0};
		glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, direction2);
		glLightfv(GL_LIGHT2, GL_POSITION, posicion);


		//Llamada a los focos de la cabina
		if (LuzFocal) {
			glEnable(GL_LIGHT1);

		}
		else {
			glDisable(GL_LIGHT1);
		}
	}
	
	//Dibujamos el terreno
	glColor3f(1.0, 1.0, 1.0);
	DibujarTerreno();

	//Creamos la cabina
	if (cabinaActiva) {
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);

		glBindTexture(GL_TEXTURE_2D, cabina);
		GLfloat planoS[] = { 0, 0.5, 0, 0.5 };
		GLfloat planoT[] = { 0, 0, 0.5, -0.5 };
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenfv(GL_S, GL_OBJECT_PLANE, planoS);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenfv(GL_T, GL_OBJECT_PLANE, planoT);

		glPushMatrix();
		glTranslatef(eye[0], eye[1], eye[2]);
		glRotatef(desplazamientoGiro, 0, 0, 1);
		glutSolidSphere(1.5, 10, 10);
		glPopMatrix();

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glPopAttrib();
	}
	
	//Visor HUD

	std::ostringstream strs2;
	strs2 << fixed << setprecision(2) << velocidad*1200;
	std::string str2 = strs2.str();

	std::ostringstream strs3;
	strs3 << fixed << setprecision(2) << eye[2];
	std::string str3 = strs3.str();

	std::ostringstream strs4;
	strs4 << fixed << setprecision(1) << Elevacion(eye[0], eye[1]);
	std::string str4 = strs4.str();

	std::ostringstream strs5;
	strs5 << fixed << setprecision(1) << eye[0];
	std::string str5 = strs5.str();

	std::ostringstream strs51;
	strs51 << fixed << setprecision(1) << eye[1];
	std::string str51 = strs51.str(); 

	std::ostringstream strs6;
	strs6 << fixed << setprecision(1) << desplazamientoGiro;
	std::string str6 = strs6.str();

	std::ostringstream strs7;
	strs7 << fixed << setprecision(1) << (guinyada - desplazamientoGiro);
	std::string str7 = strs7.str();

	std::ostringstream strs8;
	strs8 << fixed << setprecision(1) << cabeceo;
	std::string str8 = strs8.str();

	string texto1 = "HUD: Cpt. NMANMAT";
	string texto2 = "Speed: " + str2 + "km/h";
	string texto3 = "Altitude: " + str3 + "m";
	string texto4 = "Ground: "+ str4 + "m";
	string texto5 = "Location: " + str5 + ", " + str51;
	string texto6 = "Compass: " + str6 + "degN";
	string texto7 = "LookYaw: " + str7 + "deg";
	string texto8 = "LookPitch: " + str8 + "deg";
	texto(5, 120, (char*)texto1.c_str(), BLANCO, (void*)3U, false);
	texto(5, 95, (char*)texto2.c_str(), BLANCO, (void*)3U, false);
	texto(5, 80, (char*)texto3.c_str(), BLANCO, (void*)3U, false);
	texto(5, 65, (char*)texto4.c_str(), BLANCO, (void*)3U, false);
	texto(5, 50, (char*)texto5.c_str(), BLANCO, (void*)3U, false);
	texto(5, 35, (char*)texto6.c_str(), BLANCO, (void*)3U, false);
	texto(5, 20, (char*)texto7.c_str(), BLANCO, (void*)3U, false);
	texto(5, 5, (char*)texto8.c_str(), BLANCO, (void*)3U, false);

	glutSwapBuffers();
}

//Controles de eventos
void OnKeyPress(unsigned char key, int, int) {

	switch (key) {
		//Control de acelerar
	case 'a': 
		if (velocidad < 50 && !PilotoActivo) {
		velocidad += ACELERACION;
		};
		break;
	case 'A': 
		if (velocidad < 50 && !PilotoActivo) {
		velocidad += ACELERACION;
		};
		break;
		//Control de frenar
	case 'z': if (velocidad > 0 && (velocidad - ACELERACION) >0 && !PilotoActivo) {
		velocidad -= ACELERACION;
	}else if(!PilotoActivo) {
		velocidad = 0;
	}
		break;
	case 'Z': if (velocidad > 0 && (velocidad - ACELERACION) > 0 && !PilotoActivo) {
		velocidad -= ACELERACION;
		}else if (!PilotoActivo) {
		velocidad = 0;
	}
		break;
		//Control de generacion de un nuevo terreno aleatorio
	case 'r': 
		noise.SetSeed(1400 + rand() % 1600);
		eye[2] = Elevacion(0, 0) + 10;
		if (Elevacion(eye[0], eye[1]) == eye[2]) {
			eye[2] += 10;
		}
		break;
	case 'R':  
		noise.SetSeed(1000 + rand() % 1600);
		eye[2] = Elevacion(0, 0) + 10;
		if (Elevacion(eye[0], eye[1]) == eye[2]) {
			eye[2] += 10;
		}
		break;
		//Control de iluminacion del terreno (diurno/nocturno)
	case 'l':
		if (modoDiurno) {
			modoDiurno = !modoDiurno;
		}
		else {
			modoDiurno = !modoDiurno;
		}
		break;
	case 'L':
		if (modoDiurno) {
			modoDiurno = !modoDiurno;
		}
		else {
			modoDiurno = !modoDiurno;
		}
		break;
		//Control de que aparezca y desaparezca la cabina
	case 'c':
	case 'C':
		if (cabinaActiva) { cabinaActiva = !cabinaActiva; }
		else {
			cabinaActiva = !cabinaActiva;
		}
		break;

		//Control piloto automatico
	case 'q':
		PilotoActivo = !PilotoActivo;
		altura_autopilot = eye[2] - Elevacion(eye[0], eye[1]);
		velocidad = velocidad / 2;
		break;
	case 'Q':
		PilotoActivo = !PilotoActivo;
		altura_autopilot = eye[2] - Elevacion(eye[0], eye[1]);
		velocidad = velocidad / 2;
		break;
	case 'f':
	case 'F':
		if (LuzFocal) {
			LuzFocal = !LuzFocal;
		}
		else {
			LuzFocal = !LuzFocal;
		}
		break;
	}
	glutPostRedisplay();
}

//Control de eventos especiales
void OnSpecialKeyPress(int key, int, int) {

	//Si esta en el piloto automatico: no dejamos la opcion de que suba o baje, ya que deberia de ir sola la cabina de vuelo
	if (PilotoActivo) {
		switch (key) {
		case GLUT_KEY_LEFT:
			desplazamientoGiro += ANGULO_GIRO;
			guinyada += ANGULO_GIRO;
			break;
		case GLUT_KEY_RIGHT:
			desplazamientoGiro -= ANGULO_GIRO;
			guinyada -= ANGULO_GIRO;
			break;
		}
	}
	//Si no esta activo el piloto automatico
	else {
		switch (key) {
		case GLUT_KEY_UP:
			if (velocidad > 0 && (eye[2] + MOVIMIENTO_CABINA) < ALTURA_MAX) {
				eye[2] = eye[2] + MOVIMIENTO_CABINA;
			}
			else if (eye[2] == 0) {
				eye[2] = Elevacion(eye[0], eye[1])+1;
				velocidad -= velocidad / 2;
			}
			break;
		case GLUT_KEY_DOWN:
			if (velocidad > 0 && (eye[2] - MOVIMIENTO_CABINA) > Elevacion(eye[0],eye[1])) {
				eye[2] = eye[2] - MOVIMIENTO_CABINA;
			}
			else if (eye[2] == 0) {
				eye[2] = Elevacion(eye[0], eye[1])+1;
				velocidad -= velocidad / 2;
			}
			break;
		case GLUT_KEY_LEFT:
			desplazamientoGiro += ANGULO_GIRO;
			guinyada += ANGULO_GIRO;
			break;
		case GLUT_KEY_RIGHT:
			desplazamientoGiro -= ANGULO_GIRO;
			guinyada -= ANGULO_GIRO;
			break;
		}

	}
	glutPostRedisplay();
}

//Control evento del click del raton
void onClick(int boton, int estado, int x, int y)
{
	if (boton == GLUT_LEFT_BUTTON && estado == GLUT_DOWN) {
		xanterior = x;
		yanterior = y;
		botonPresionado = true;
	}

}

//Control del desplazamiento del raton cuando esta pulsado
void onDrag(int x, int y)
{
	if (botonPresionado) {
		desplazamientoX = (x - xanterior) * SENSIBILIDAD;
		desplazamientoY = (y - yanterior) * SENSIBILIDAD;
		if (desplazamientoX >= 90) {
			desplazamientoX = 90;
		}
		if (desplazamientoX <= -90) {
			desplazamientoX = -90;
		}
		if (desplazamientoY >= 45) {
			desplazamientoY = 45;
		}
		if (desplazamientoY <= -89) {
			desplazamientoY = -89;
		}

		guinyada += desplazamientoX;
		cabeceo += desplazamientoY;

		if (cabeceo > 360) {
			cabeceo -= 360;
		}
		if (cabeceo < -360) {
			cabeceo += 360;
		}
		if (guinyada > 360) {
			guinyada -= 360;
		}
		if (guinyada < -360) {
			guinyada += 360;
		}

		look[0] = cos(rad(guinyada)) * cos(rad(cabeceo)) + eye[0];
		look[1] = sin(rad(guinyada)) * cos(rad(cabeceo)) + eye[1];
		look[2] = sin(rad(cabeceo)) + eye[2];

		xanterior = x;
		yanterior = y;

	}

	glutPostRedisplay();
}

//Control del desplazamiento de la cabina
void onIdle() {

	//Control del tiempo
	static int antes = glutGet(GLUT_ELAPSED_TIME);
	int ahora = glutGet(GLUT_ELAPSED_TIME);
	tiempoTranscurrido = ahora - antes;
	antes = ahora;

	//Control de la velocidad
	desplazamiento_velocidad = tiempoTranscurrido * velocidad;

	eye[0] += desplazamiento_velocidad * cos(rad(desplazamientoGiro));
	eye[1] += desplazamiento_velocidad * sin(rad(desplazamientoGiro));

	look[0] = cos(rad(guinyada)) * cos(rad(cabeceo)) + eye[0];
	look[1] = sin(rad(guinyada)) * cos(rad(cabeceo)) + eye[1];
	look[2] = sin(rad(cabeceo)) + eye[2];

	if (eye[2] < Elevacion(eye[0], eye[1]) && eye[2] > 0) {
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		cout << "***************** TE HAS ESTRELLADO *****************" << endl;
		exit(0);
	}
	
	glutPostRedisplay();
}

// Funcion de atencion al redimensionamiento
void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 0.05, 300.0);
	glMatrixMode(GL_MODELVIEW);
}

void onTimer(int tiempo) {
	
	glutTimerFunc(tiempo, onTimer, tiempo);

	onIdle();
}

int main(int argc, char** argv) {

	FreeImage_Initialise();

	//Inicializaciones
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(80, 80);
	glutInitWindowSize(780, 500);
	glutCreateWindow("Simulador");
	init();

	//Registro de callbacks
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(OnKeyPress);
	glutSpecialFunc(OnSpecialKeyPress);
	glutMouseFunc(onClick);
	glutMotionFunc(onDrag);

	glutTimerFunc(1000 / tasaFPS, onTimer, 1000 / tasaFPS);

	//Bucle de atencion a eventos
	glutMainLoop();

	FreeImage_DeInitialise();
}
