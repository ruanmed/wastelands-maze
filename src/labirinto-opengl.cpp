//============================================================================
// Name        : labirinto-opengl.cpp
// Author      : Ricardo Valério Teixeira de Medeiros Silva, Ruan de Medeiros Bahia e Jose Adolfo de Castro Neto
// Version     :
// Copyright   : 
// Description : Jogo de labirinto feito com OpenGL
//  Universidade Federal do Vale do São Francisco
// 	Professor Jorge Luis Cavalcanti
// 	Trabalho de Computação Gráfica
// 		para compilar no linux: g++ labirinto-opengl.c -lglut -lGL -lGLU -lm
//============================================================================
/*
[x]	O objetivo é passar um objeto (circulo, triângulo ou retângulo) sem tocar nas paredes
	do labirinto.
[x]	Cada vez que ocorrer um toque na parede, o objeto volta para o início e o
	jogador perde uma “vida”.
[-]	O jogo acaba quando ele atravessar sem tocar (vitória) ou bate 4 vezes em uma
	parede (derrota).
 */

#include <GL/gl.h>
#include <GL/glut.h>
#include <FreeImage.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <effolkronium/random.hpp>
#include <conversorSR.hpp>
/*
 * #include "../include/effolkronium/random.hpp"
 * #include "../include/conversorSR.hpp"
*/
#include <locale.h>
#include <vector>
#ifdef _WIN32
    #include <windows.h>
#endif
#define PI 3.14159265
#define RESET_COLOR 0
#define CIRCLE_COLOR 1
#define MAZE_COLOR 2
#define BACK_COLOR 3
#define FLASH_COLOR 4

#define RIGHT 1
#define LEFT 2
#define UP 3
#define DOWN 4

#define GLUT_SPACEBAR_KEY 32
#define GAME_TITLE "Wastelands Maze 2.0"
#define GAME_OVER 0
#define GAME_WELCOME 1
#define GAME_START 2
#define GAME_WIN 3
#define GAME_NEWLEVEL 4
#define GAME_HELP 5

#define MENU_DIFFICULTY_INCREASE 1
#define MENU_DIFFICULTY_DECREASE 2
#define MENU_DIFFICULTY_SUPER_INCREASE 3
#define MENU_DIFFICULTY_SUPER_DECREASE 4
#define MENU_OPTIONS_RESTART 1
#define MENU_OPTIONS_RESET 2
#define MENU_OPTIONS_RESET_MAZE 3
#define MENU_OPTIONS_RESET_COLORS 4
#define MENU_OPTIONS_RESET_LEVEL 5
#define MENU_COLORS_CHANGE_CIRCLE 1
#define MENU_COLORS_CHANGE_WALLS 2
#define MENU_COLORS_CHANGE_BACKGROUND 3
#define MENU_COLORS_RESET 4
#define MENU_COLORS_FLASH_MAZE 5
#define MENU_EXIT 10
#define MENU_HELP 11

//#define MAZE_LIGHT_OFF 0
//#define MAZE_LIGHT_ON 1

#define CHAR_POSITION_UP_RIGHT 0
#define CHAR_POSITION_UP_LEFT 1
#define CHAR_POSITION_DOWN_RIGHT 2
#define CHAR_POSITION_DOWN_LEFT 3

#define OBJECT_SQUARE 1
#define OBJECT_CIRCLE 0

int GAME_STATUS = 1;
int GAME_PREVIOUS_STATUS = 1;
int GAME_LEVEL = 1;

int OBJECT_CLASS = OBJECT_SQUARE;
int CIRCLE_RADIUS = 5*GAME_LEVEL;
bool CIRCLE_FLASH = false;
double CIRCLE_RADIUS_DECREASE_TIME_CONSTANT = 30;
double CIRCLE_POINT_SIZE = 1.0f;
double CIRCLE_CENTER_SPEED = (1/4.0);
int CIRCLE_INITIAL_LIFE = 4;
int ORTHO_WIDTH = 1920;
int ORTHO_HEIGTH = 1080;
int ORTHO_LEFT = -(ORTHO_WIDTH/2);
int ORTHO_RIGHT = (ORTHO_WIDTH/2);
int ORTHO_BOTTOM  = -(ORTHO_HEIGTH/2);
int ORTHO_TOP = (ORTHO_HEIGTH/2);
double WINDOW_PROPORTION = 0.5;
double WINDOW_WIDTH = (ORTHO_WIDTH*WINDOW_PROPORTION);
double WINDOW_HEIGTH = (ORTHO_HEIGTH*WINDOW_PROPORTION);
int MAZE_STEP = (CIRCLE_RADIUS*6);

bool MAZE_FLASH = true;

bool MAZE_LIGHT_STATUS = true;
double MAZE_LIGHT_MULT = 2.0;
double MAZE_LIGHT_SIZE = MAZE_STEP*MAZE_LIGHT_MULT;

double CIRCLE_CENTER_DISPLACEMENT = CIRCLE_CENTER_SPEED*MAZE_STEP;
double MAZE_LINE_SIZE = CIRCLE_RADIUS*(1.0/4.0);
int MESH_WIDTH_PARTS = ORTHO_WIDTH/MAZE_STEP;
int MESH_HEIGTH_PARTS = ORTHO_HEIGTH/MAZE_STEP;
double MESH_WIDTH_PARTS_OPENNING_PROBABILITY = 0.5;
double MESH_HEIGTH_PARTS_OPENNING_PROBABILITY =  0.7;
double MESH_PARTS_OPPENING_DECREASE_TIME_CONSTANT = 600;

using Random = effolkronium::random_static;
typedef struct {
	char side;
	char top;
	char visited;
}mesh;

mesh **maze;

double x,y;
double xc = 0, yc = 0, xc0 = 0, yc0 = 0, raio = CIRCLE_RADIUS;
int eixoH = 0, eixoV = 0;
int vidas = CIRCLE_INITIAL_LIFE;
static GLuint texture = 0;
static GLuint texturasCarteiro[4] = {0,1,2,3}; // Estados do carteiro {UP-RIGHT, UP-LEFT, DOWN-RIGHT, DOWN-LEFT}

double corCircR,corCircG,corCircB;
double corVidaR,corVidaG,corVidaB;
double corFundR,corFundG,corFundB;
double corLabiR,corLabiG,corLabiB;
//  SISTEMA = {Xmin,Xmax,Ymin,Ymax]
double SRU[4] = {(double) ORTHO_LEFT,(double) ORTHO_RIGHT,(double) ORTHO_BOTTOM,(double) ORTHO_TOP};
double SRD[4] = {0,0,0,0};
char tituloJanela[50];

void novaDificuldade(int nivel, bool resetarCores);
void novaCor(int elemento); // CIRCLE_COLOR || MAZE_COLOR || BACK_COLOR || FLASH_COLOR


//======================================================================//
// Função auxiliar para converter de BGRA para RGBA
BYTE* FreeImage_GetBitsSwapRedBlue(FIBITMAP *dib) {
	BYTE *bits = new BYTE[FreeImage_GetWidth(dib) * FreeImage_GetHeight(dib) * 4];

	BYTE *pixels = (BYTE*) FreeImage_GetBits(dib);

	for (unsigned int pix = 0; pix< (unsigned int)(FreeImage_GetWidth(dib) * FreeImage_GetHeight(dib)); pix++)
	{
		bits[pix * 4 + 0] = pixels[pix * 4 + 2];
		bits[pix * 4 + 1] = pixels[pix * 4 + 1];
		bits[pix * 4 + 2] = pixels[pix * 4 + 0];
	}
	//printf()
	return bits;
}

//======================================================================//
void atualizarJanela(){
	sprintf(tituloJanela, "Wastelands Maze 2.0 by Ricardo, Ruan Medeiros e Jose Adolfo - Vidas: %d - Nivel: %d", vidas, GAME_LEVEL);
	glutSetWindowTitle(tituloJanela);
}
void retornarInicio() {	//	Retorna círculo para o início do labirinto
	xc = xc0;
	yc = yc0;
}
//======================================================================//
void resetMazeMesh(){
	for(int l=0;l<MESH_WIDTH_PARTS;l++)
		for(int c=0;c<MESH_HEIGTH_PARTS;c++){
			maze[l][c].side = 0;
			maze[l][c].top = 0;
			maze[l][c].visited = 0;
		}
}
bool exitPathExists(int l, int c, int whereItCameFrom){
	char left, right, up, down;
	//printf ("L: %d, C: %d, WH: %d\n", l, c, whereItCameFrom);
	if (l < 0 || l >=	MESH_WIDTH_PARTS - 1 || c < 0 || c >=	MESH_HEIGTH_PARTS - 1 )
		return true;
	else if (maze[l][c].visited){
		return false;
	}
	else {
		maze[l][c].visited = 1;
		left = maze[l][c].side;
		right = maze[l+1][c].side;
		up = maze[l][c+1].top;
		down = maze[l][c].top;

		if (left == 1 && whereItCameFrom != RIGHT){ // 	se tiver aberto para a esquerda e
													//	não veio de um movimento para a direita
			if (exitPathExists(l-1,c,LEFT))
				return true;
		}
		if (right == 1 && whereItCameFrom != LEFT){ // 	se tiver aberto para a direita e
													//	não veio de um movimento para a esquerda
			if (exitPathExists(l+1,c,RIGHT))
				return true;
		}
		if (up == 1 && whereItCameFrom != DOWN){ 	// 	se tiver aberto para cima e
													//	não veio de um movimento para baixa
			if (exitPathExists(l,c+1,UP))
				return true;
		}
		if (down == 1 && whereItCameFrom != UP){ 	// 	se tiver aberto para baixo e
													//	não veio de um movimento para cima
			if (exitPathExists(l,c-1,DOWN))
				return true;
		}
	}
	return false;
}
bool isOnMaze(double x,double y){
	if (OBJECT_CLASS == OBJECT_CIRCLE) {
		double xSRD = getXSRD(SRU, SRD, x);
		double ySRD = getYSRD(SRU, SRD, y);

		GLubyte *data = (GLubyte *) malloc( 3 * 1 * 1);
		if( data ) {
			glReadPixels(xSRD, glutGet( GLUT_WINDOW_HEIGHT ) -  ySRD, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
			//printf("SRD( %d, %d)\n", xSRD, glutGet( GLUT_WINDOW_HEIGHT ) -ySRD);
			/*
			printf("MAZE - rgb( %d, %d, %d)\nRGB( %d, %d, %d)\n SRD ( %f, %f)\n",
					data[0], data[1], data[2],
						(int) (corLabiR*255.0), (int)(corLabiG*255.0), (int)(corLabiB*255.0),
						xSRD, ySRD);
			//*/
			if (	ceil(data[0]/25.5f) == ceil(corLabiR*10) &&
					ceil(data[1]/25.5f) == ceil(corLabiG*10) &&
					ceil(data[2]/25.5f) == ceil(corLabiB*10)
					){

				return true;
			}
			else {
				return false;
			}
		}
		else {
			printf("ERROR - Can't allocated memory.\n");
			return false;
		}
	}
	else if (OBJECT_CLASS == OBJECT_SQUARE){
		return false;
	}
	return false;
}
bool isOnLimit(int x,int y)
{	if(x <= ORTHO_LEFT || x >= ORTHO_RIGHT || y<= ORTHO_BOTTOM || y >= ORTHO_TOP)
		return true;
	else
		return false;
}
bool verificarColisaoCirculo() {
	for(double theta = 0; theta < M_PI_4;theta+=0.1) {
		x = (raio*cos(theta));
		y = (raio*sin(theta));

		//	Verifica se bateu nas paredes do labirinto
		if (	isOnMaze(xc+(x),yc+(y)) ||
				isOnMaze(xc+(-x),yc+(y)) ||
				isOnMaze(xc+(-x),yc+(-y)) ||
				isOnMaze(xc+(x),yc+(-y)) ||
				isOnMaze(xc+(y),yc+(x)) ||
				isOnMaze(xc+(-y),yc+(x)) ||
				isOnMaze(xc+(-y),yc+(-x)) ||
				isOnMaze(xc+(y),yc+(-x))
				) {
			//retornarInicio();
			//vidas--;
			return true;
		}
		else
			return false;
	}
	//raio = CIRCLE_RADIUS;
	return false;
}
bool verificarColisaoQuadrado(){	//	Verifica se vai haver colisão detectando se os segmentos de reta do quadrado
									//	Passam por alguma parede do labirinto
	//printf("\n_____------->\nEHNTOREUORUIOA\n");
	int meshX1 = 1, meshY1 = 1, meshX2 = 1, meshY2 = 1;
	meshX1 = floor((xc-raio-ORTHO_LEFT)/MAZE_STEP);
	meshX2 = floor((xc+raio-ORTHO_LEFT)/MAZE_STEP);
	meshY1 = floor((yc-raio-ORTHO_BOTTOM)/MAZE_STEP);
	meshY2 = floor((yc+raio-ORTHO_BOTTOM)/MAZE_STEP);
	//printf("RAIO MESHSHSHSHSH____1> (%d, %d)\n",meshX1,meshY1);
	//printf("RAIO MESHSHSHSHSH____2> (%d, %d)\n",meshX2,meshY2);
	if (meshX2 < MESH_WIDTH_PARTS && meshY2 < MESH_HEIGTH_PARTS){
		if (meshY1 >= 0 && meshX2-meshX1 > 0){	//	Verifica se a parte de cima ou de baixo do quadrado
			if (maze[meshX2][meshY1].side == 0)	//	está colidindo com alguma parede vertical do labirinto
				return true;
			if (maze[meshX2][meshY2].side == 0)
				return true;
		}
		if (meshX1 >= 0 && meshY2-meshY1 > 0) {	//	Verifica se os lados do quadrado estão colidindo
			if (maze[meshX1][meshY2].top == 0)	//	com alguma parede horizontal do labirinto
				return true;
			if (maze[meshX2][meshY2].top == 0)
				return true;
		}
	}
	return false;
}
bool verificarColisao(){
	if (OBJECT_CLASS == OBJECT_CIRCLE)
		return verificarColisaoCirculo();
	else if (OBJECT_CLASS == OBJECT_SQUARE)
		return verificarColisaoQuadrado();
	return false;
}
void verificarVitoria()
{
	double theta;
		for(theta = 0; theta <0.8;theta+=0.01) {
			x = (int)(raio*cos(theta));
			y = (int)(raio*sin(theta));

			//	Verifica se bateu nas paredes da viewport
			if (	isOnLimit(xc+(x),yc+(y)) ||
					isOnLimit(xc+(-x),yc+(y)) ||
					isOnLimit(xc+(-x),yc+(-y)) ||
					isOnLimit(xc+(x),yc+(-y)) ||
					isOnLimit(xc+(y),yc+(x)) ||
					isOnLimit(xc+(-y),yc+(x)) ||
					isOnLimit(xc+(-y),yc+(-x)) ||
					isOnLimit(xc+(y),yc+(-x))
					) {
				novaDificuldade(GAME_LEVEL+1,false);
				GAME_STATUS = GAME_NEWLEVEL;
				retornarInicio();
				break;
			}
		}
}
bool verificarStatus()
{
	bool status; // if true -> aconteceu colisão

	status = verificarColisao();
	if (!status)
		verificarVitoria();

	return status;
}
void generateRandomMaze()
{
	resetMazeMesh();

	for(int l=0;l<MESH_WIDTH_PARTS;l++)
			for(int c=0;c<MESH_HEIGTH_PARTS;c++)
			{
				if(auto val = Random::get<bool>(MESH_WIDTH_PARTS_OPENNING_PROBABILITY))
					maze[l][c].top = 1;
				if(auto val = Random::get<bool>(MESH_HEIGTH_PARTS_OPENNING_PROBABILITY))
					maze[l][c].side = 1;
			}
}
//======================================================================//
void desenhaLabirinto(void)
{
	//glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(corLabiR, corLabiG, corLabiB);

	glLineWidth(MAZE_LINE_SIZE);
	glBegin(GL_LINES);

		for(int l=0;l<MESH_WIDTH_PARTS;l++)
				for(int c=0;c<MESH_HEIGTH_PARTS;c++)
				{
					if(!maze[l][c].top)
					{
						glVertex2f(ORTHO_LEFT+l*MAZE_STEP,ORTHO_BOTTOM+c*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(l+1)*MAZE_STEP,ORTHO_BOTTOM+c*MAZE_STEP);
					}
					if(!maze[l][c].side)
					{
						glVertex2f(ORTHO_LEFT+l*MAZE_STEP,ORTHO_BOTTOM+c*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+l*MAZE_STEP,ORTHO_BOTTOM+(c+1)*MAZE_STEP);
						//printf("%d %d\n", c*MAZE_STEP, l*MAZE_STEP);
					}
				}

	glEnd();
	//glutSwapBuffers();
	//glutPostRedisplay();
}
//======================================================================//
void desenhaLabirintoLuz(void){
	//	Definindo a cor do labirinto iluminado
	glColor3f(corLabiR, corLabiG, corLabiB);

	glLineWidth(MAZE_LINE_SIZE);
	glBegin(GL_LINES);

	int meshX = 1, meshY = 1;

	// Determinando até aonde a luz vai para cima
	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP);
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP) + c + 1;
		//printf("RAIO 5____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS) // Verifica se não passou dos limites
			break;
		else {
			if (maze[meshX][meshY].top == 0) {	//	Verifica se existe uma parede acima para definir o limite
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				break;
			}
			else if (0){
				int meshXD = meshX+1;
				// Se não houver uma parede acima então tem que ser verificadas as paredes à esquerda e à direita
				for (int l = 0;meshX >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						break;
					}
					meshX--;
				}

				for (int l = 0;meshXD < MESH_WIDTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshXD][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						break;
					}
					meshXD++;
				}
			}
		}
	}

	printf("->>>> COISAS> (%d, %d)\n",MESH_WIDTH_PARTS,MESH_HEIGTH_PARTS);
	// Determinando até aonde a luz vai para baixo
	for (int c = 0; c <  (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP);
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP) - c;
		//printf("RAIO 6____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {	// Se não houver uma parede abaixo então tem que ser verificada a parede logo ao lado direito desta
			if (maze[meshX][meshY].top == 0) {
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				break;
			}
			else if (0){
				meshY--;
				int meshXD = meshX;
				meshX++;
				for (int l = 0; meshX < MESH_WIDTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					//printf("RAIO 66666666666____> (%d, %d)\n",meshX,meshY);
					if (maze[meshX][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						break;
					}
					meshX++;
				}
				for (int l = 0;meshXD >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshXD][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						break;
					}
					meshXD--;
				}
			}
		}
	}
	// Determinando até aonde a luz vai para direita

	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP) +c + 1;
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP);
		//meshX++;
		//printf("RAIO 7____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {
			if (maze[meshX][meshY].side == 0) {	//	Limite pela direita
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
				break;
			}
			else if (0){
				// Se não houver uma parede à direita então tem que ser verificada a parede logo ao lado direito desta
				int meshYE = meshY;
				meshY++;
				for (int l = 0; meshY < MESH_HEIGTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						break;
					}
					meshY++;
				}
				for (int l = 0; meshYE >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshYE].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
						break;
					}
					meshYE--;
				}
			}
		}
	}
	// Determinando até aonde a luz vai para esquerda
	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP) - c;
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP);
		//printf("RAIO 8____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {
			if (maze[meshX][meshY].side == 0) {
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
				break;
			}
			else if (0){
				meshX--;
				if (meshX < 0)
					break;
				int meshYE = meshY+1;
				// Se não houver uma parede à direita então tem que ser verificada a parede logo ao lado direito desta
				for (int l = 0; meshY >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glColor3f(corFundR-0.1, corFundG-0.1, 0.2);
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						break;
					}
					meshY--;
				}
				for (int l = 0; meshYE < MESH_HEIGTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshYE].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glColor3f(corFundR-0.1, corFundG-0.1, 0.8);
						glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
						glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
						break;
					}
					meshYE++;
				}
				glColor3f(corLabiR, corLabiG, corLabiB);
			}
		}
	}
	glEnd();
}

//======================================================================//
void desenhaQuadrado(void){
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f,0.0f);
			glVertex2f(xc-raio,yc-raio);
		glTexCoord2f(0.0f,1.0f);
			glVertex2f(xc-raio,yc+raio);
		glTexCoord2f(1.0f,1.0f);
			glVertex2f(xc+raio,yc+raio);
		glTexCoord2f(1.0f,0.0f);
			glVertex2f(xc+raio,yc-raio);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
//======================================================================//
void desenhaLuzQuadrada(){
	glColor3f(0.0, 0.0, 0.0);
	double limiteCima = yc+MAZE_LIGHT_SIZE,
			limiteBaixo = yc-MAZE_LIGHT_SIZE,
			limiteEsquerda = xc-MAZE_LIGHT_SIZE,
			limiteDireita = xc+MAZE_LIGHT_SIZE;
	glBegin(GL_QUADS);
		glVertex2f(ORTHO_LEFT,ORTHO_TOP);
		glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
		glVertex2f(limiteEsquerda,ORTHO_BOTTOM);
		glVertex2f(limiteEsquerda,ORTHO_TOP);

		glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
		glVertex2f(limiteDireita,ORTHO_BOTTOM);
		glVertex2f(limiteDireita,ORTHO_TOP);

		glVertex2f(ORTHO_LEFT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,limiteCima);
		glVertex2f(ORTHO_LEFT,limiteCima);

		glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
		glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
		glVertex2f(ORTHO_RIGHT,limiteBaixo);
		glVertex2f(ORTHO_LEFT,limiteBaixo);
	glEnd();
}

void desenhaLuz(void){	//	Essa função na verdade é uma mentira, ela na verdade desenha a área sem luz
	int meshX = 1, meshY = 1;

	//	Definindo a cor da área sem luz
	//glColor3f(corFundR-0.1, corFundG-0.1, corFundB-0.1);
	glColor3f(0.0, 0.0, 0.0);

	// Determinando até aonde a luz vai para cima
	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP);
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP) + c + 1;
		//printf("RAIO 5____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS) // Verifica se não passou dos limites
			break;
		else {
			if (maze[meshX][meshY].top == 0) {	//	Verifica se existe uma parede acima para definir o limite
				glBegin(GL_QUADS);
					glVertex2f(ORTHO_LEFT,ORTHO_TOP);
					glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
					glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
					glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glEnd();
			}
			else {
				int meshXD = meshX+1;
				// Se não houver uma parede acima então tem que ser verificadas as paredes à esquerda e à direita
				for (int l = 0;meshX >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_LEFT,ORTHO_TOP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_TOP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glEnd();
						break;
					}
					meshX--;
				}

				for (int l = 0;meshXD < MESH_WIDTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshXD][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
							glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_TOP);
							glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
						glEnd();
						break;
					}
					meshXD++;
				}
			}
		}
	}
	//printf("->>>> COISAS> (%d, %d)\n",MESH_WIDTH_PARTS,MESH_HEIGTH_PARTS);
	// Determinando até aonde a luz vai para baixo
	for (int c = 0; c <  (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP);
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP) - c;
		//printf("RAIO 6____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {	// Se não houver uma parede abaixo então tem que ser verificada a parede logo ao lado direito desta
			if (maze[meshX][meshY].top == 0) {
				glBegin(GL_QUADS);
					glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
					glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
					glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
					glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
				glEnd();
			}
			else {
				meshY--;
				int meshXD = meshX;
				meshX++;
				for (int l = 0; meshX < MESH_WIDTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					//printf("RAIO 66666666666____> (%d, %d)\n",meshX,meshY);
					if (maze[meshX][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						glEnd();
						break;
					}
					meshX++;
				}
				for (int l = 0;meshXD >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshXD][meshY].side == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
							glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM);
							glVertex2f(ORTHO_LEFT+(meshXD)*MAZE_STEP,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshY+1)*MAZE_STEP);
						glEnd();
						break;
					}
					meshXD--;
				}
			}
		}
	}
	// Determinando até aonde a luz vai para direita

	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP) +c + 1;
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP);
		//meshX++;
		//printf("RAIO 7____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {
			if (maze[meshX][meshY].side == 0) {	//	Limite pela direita
				glBegin(GL_QUADS);
					glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
					glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
					glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM);
					glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_TOP);
				glEnd();
			}
			else {
				// Se não houver uma parede à direita então tem que ser verificada a parede logo ao lado direito desta
				int meshYE = meshY;
				meshY++;
				for (int l = 0; meshY < MESH_HEIGTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_TOP);
						glEnd();
						break;
					}
					meshY++;
				}
				for (int l = 0; meshYE >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshYE].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
							glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM);
						glEnd();
						break;
					}
					meshYE--;
				}
			}
		}
	}
	// Determinando até aonde a luz vai para esquerda
	for (int c = 0; c < (int)floor(MAZE_LIGHT_MULT); c++) {
		meshX = floor((xc-ORTHO_LEFT)/MAZE_STEP) - c;
		meshY = floor((yc-ORTHO_BOTTOM)/MAZE_STEP);
		//printf("RAIO 8____> (%d, %d)\n",meshX,meshY);
		if (meshX < 0 || meshY < 0 || meshX >= MESH_WIDTH_PARTS || meshY >= MESH_HEIGTH_PARTS)
			break;
		else {
			if (maze[meshX][meshY].side == 0) {
				glBegin(GL_QUADS);
					glVertex2f(ORTHO_LEFT,ORTHO_TOP);
					glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
					glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_BOTTOM);
					glVertex2f(ORTHO_LEFT+(meshX)*MAZE_STEP,ORTHO_TOP);
				glEnd();
			}
			else {
				meshX--;
				if (meshX < 0)
					break;
				int meshYE = meshY+1;
				// Se não houver uma parede à direita então tem que ser verificada a parede logo ao lado direito desta
				for (int l = 0; meshY >= 0 && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshY].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						//glColor3f(corFundR-0.1, corFundG-0.1, 0.2);
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshY)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM);
						glEnd();
						break;
					}
					meshY--;
				}
				for (int l = 0; meshYE < MESH_HEIGTH_PARTS && l < (int)floor(MAZE_LIGHT_MULT); l++){
					if (maze[meshX][meshYE].top == 0){ // Se exister a parede ao lado então devem ser apagadas as subsequentes que estão "atrás"dela
						//glColor3f(corFundR-0.1, corFundG-0.1, 0.8);
						glBegin(GL_QUADS);
							glVertex2f(ORTHO_LEFT,ORTHO_TOP);
							glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_BOTTOM+(meshYE)*MAZE_STEP);
							glVertex2f(ORTHO_LEFT+(meshX+1)*MAZE_STEP,ORTHO_TOP);
						glEnd();

						break;
					}
					meshYE++;
				}
			}
		}
	}

	//desenhaLabirintoLuz();
	desenhaLuzQuadrada();
}
/*
void getMeshGridCenter(int InLMesh,int InCMesh,double &OutXc,double &OutYc)//recebe a linha e a coluna do mesh e retorna seu ponto central no ortho 2d
{
	OutXc = (InCMesh+0.5)*(MAZE_STEP)+ORTHO_LEFT;
	OutYc = (InLMesh+0.5)*(MAZE_STEP)+ORTHO_BOTTOM;
}
void getCurrentPositionMesh(int &OutXchar,int &OutYchar)//retorna o mesh atual do personagem
{
	OutXchar = floor(x/MAZE_STEP);
	OutYchar = floor(y/MAZE_STEP);
}
void getCurrentPositionMeshGridCenter(double &OutXc,double &OutYc)// retorna o centro do mesh atual do personagem
{
	int xchar,ychar;
	getCurrentPositionMesh(xchar,ychar);
	getMeshGridCenter(xchar,ychar,OutXc,OutYc);
}
double getLineAnguleRADS(double x1,double y1,double x2,double y2)
{
	return atan((y2-y1)/(x2-x1));
}
double getLineAnguleGRAUS(double x1,double y1,double x2,double y2)
{
	return getLineAnguleRADS(x1,y1,x2,y2)*(180/PI);
}
void desenhaLuz2(void)//Essa função também é uma mentira
{
	double limiteCima = yc+MAZE_LIGHT_SIZE,
				limiteBaixo = yc-MAZE_LIGHT_SIZE,
				limiteEsquerda = xc-MAZE_LIGHT_SIZE,
				limiteDireita = xc+MAZE_LIGHT_SIZE;
	int meshX = 1, meshY = 1;

	//	Definindo a cor da área sem luz
	glColor3f(corFundR-0.1, corFundG-0.1, corFundB-0.1);
}
*/
//======================================================================//
void desenhaCirculo(void)//Infelizmente esta função de Call Back não pode ter parametros ou eu não sei como...
{
	double theta;

	glColor3f(corCircR, corCircG, corCircB);
	glPointSize(CIRCLE_POINT_SIZE); // Atualiza o tamanho dos pontos
	glBegin(GL_POINTS);
		glColor3f(corCircR, corCircG, corCircB);
		while(raio>0) {

			for(theta = 0; theta < M_PI_4;theta+=0.1)
			{
				x = (raio*cos(theta));
				y = (raio*sin(theta));
				glVertex2f(xc+(x),yc+(y));
				glVertex2f(xc+(-x),yc+(y));
				glVertex2f(xc+(-x),yc+(-y));
				glVertex2f(xc+(x),yc+(-y));
				glVertex2f(xc+(y),yc+(x));
				glVertex2f(xc+(-y),yc+(x));
				glVertex2f(xc+(-y),yc+(-x));
				glVertex2f(xc+(y),yc+(-x));
			}
			raio -= 0.1;
		}
		raio = CIRCLE_RADIUS;

	glEnd();
    //glutSwapBuffers();
}
//======================================================================//
void desenhaVidas(){
	atualizarJanela();
	if (vidas > 0) {
		//glColor3f(corVidaR, corVidaG, corVidaB);
		glBegin(GL_QUADS);
			for (int c = vidas; c; c--){
				glColor3f(fabs(1-corFundR), fabs(1-corFundG), fabs(1-corFundB));
				glVertex2d(ORTHO_LEFT+8+c*20,ORTHO_TOP-8);
				glVertex2d(ORTHO_LEFT+22+c*20,ORTHO_TOP-8);
				glVertex2d(ORTHO_LEFT+22+c*20,ORTHO_TOP-22);
				glVertex2d(ORTHO_LEFT+8+c*20,ORTHO_TOP-22);

				glColor3f(corVidaR, corVidaG, corVidaB);
				glVertex2d(ORTHO_LEFT+10+c*20,ORTHO_TOP-10);
				glVertex2d(ORTHO_LEFT+20+c*20,ORTHO_TOP-10);
				glVertex2d(ORTHO_LEFT+20+c*20,ORTHO_TOP-20);
				glVertex2d(ORTHO_LEFT+10+c*20,ORTHO_TOP-20);
			}
		glEnd();
	}
	else {
		GAME_STATUS = GAME_OVER;
	}
}
void desenhaTexto(const char *string) {	// Exibe caractere a caractere
	while(*string)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15,*string++);
}
void desenhaTextoStroke(void *font, char *string) {	// Exibe caractere a caractere
	while(*string)
		glutStrokeCharacter(GLUT_STROKE_ROMAN,*string++);
}
void desenhaBoasVindas(){
	// Posiciona o texto stroke usando transformações geométricas
	glPushMatrix();
	glTranslatef(ORTHO_LEFT*0.75,ORTHO_BOTTOM*0.3-100,0);
	//glScalef(0.2, 0.2, 0.2); // diminui o tamanho do fonte
	//glRotatef(15, 0,0,1); // rotaciona o texto
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,(char *) GAME_TITLE);
	glPopMatrix();

	// Posição no universo onde o texto bitmap será colocado
	glColor3f(1,1,1);
	//glScalef(1.0, 1.0, 1.0); // diminui o tamanho do fonte
	//glRotatef(15, 0,0,1); // rotaciona o texto

	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_TOP*0.8;
	glRasterPos2f(textoX,textoY);
	desenhaTexto("O OBJETIVO do jogo e levar a encomenda para fora do labirinto, para isso ache a saida,");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("ela fica pelas bordas do labirinto, assim voce avancara para o proximo nivel.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- MOVIMENTOS: Utilize as setas do teclado para mover o carteiro.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- CORES: Para mudar a cor de qualquer objeto (PAREDES DO LABIRINTO, FUNDO) ");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("         basta clicar no objeto em questao.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- DESAFIO: Se o carteiro colidir com qualquer INIMIGO entao ele");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("           volta a posicao inicial no meio do labirinto e perde uma vida.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- FIM: O jogo acaba quando suas vidas terminarem.");
	glColor3f(0,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("                   PRESSIONE BARRA DE ESPACOS ou ENTER PARA COMECAR");
	glColor3f(1,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- DICA: A barra de espacos tambem funciona para fazer o circulo piscar.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("- DICA 2: Ao subir de nivel voce fica novamente com 4 vidas.");
	glColor3f(1,0,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("BOA SORTE E BOM JOGO! - APERTE h PARA VER O MENU DE AJUDA");
	glColor3f(1,1,1);
}
void desenhaAjuda(){
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_TOP*0.8;

	glColor3f(1,1,1);
	glRasterPos2f(textoX,textoY);
	desenhaTexto("AJUDA - AS TECLAS/BOTOES DO MOUSE UTILIZADOS NESSE JOGO SAO:");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("ENTER/BARRA DE ESPACOS = Comeca o jogo");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("SETAS DIRECIONAIS DO TECLADO = Move carteiro");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("BOTAO DIREITO DO MOUSE = Abre menu de opcoes");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("BOTAO ESQUEDO DO MOUSE = Muda cor do objeto clicado");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("BARRA DE ESPACOS = Habilita o efeito de luz nas paredes do labirinto");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("ESC = Sai do jogo");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("h = Abre ou fecha essa tela de ajuda");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("r = Reseta a configuracao do labirinto no nivel atual");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("R = Reinicia o jogo voltando a tela inicial");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("n = Reseta as cores de todos os objetos para as cores padrao");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("b = Muda a cor do fundo para uma cor aleatoria");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("m = Muda a cor das paredes para uma cor aleatoria");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("FUNCIONALIDADES DE TRAPACA (CHEATS)");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("L = Liga ou desliga a luz no labirinto");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("i = Pula para o proximo nivel do labirinto");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("d = Volta para o nivel anterior do labirinto");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("I = Aumenta o nivel do labirinto atual em 10 niveis");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("D = Diminui o nivel do labirinto atual em 10 niveis");
}
void desenhaNovoNivel(){
	char nivel[10];
	sprintf(nivel, "%d", GAME_LEVEL);
	glColor3f(1,1,1);
	glPushMatrix();
	glTranslatef(ORTHO_LEFT*0.75,ORTHO_TOP*0.3,0);
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,(char *) GAME_TITLE);
	glPopMatrix();

	glColor3f(1,1,1);
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_BOTTOM*0.1;
	glRasterPos2f(textoX,textoY);
	desenhaTexto("BEM VINDO AO NIVEL");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto( nivel);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("PRESSIONE BARRA DE ESPACOS ou ENTER PARA CONTINUAR NO LABIRINTO");
}
void desenhaFimDeJogo(){
	char mensagem[50];

	glColor3f(1,1,1);
	glPushMatrix();
	glTranslatef(ORTHO_LEFT*0.5,ORTHO_TOP*0.3,0);
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,(char *) "Wastelands Maze");
	glPopMatrix();

	glColor3f(1,1,1);
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_BOTTOM*0.1;
	glColor3f(1,0,0);
	glRasterPos2f(textoX,textoY);
	desenhaTexto("FIM DE JOGO");
	glColor3f(1,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("Uma pena! Tente jogar melhor da proxima vez!");
	glRasterPos2f(textoX,(textoY-=50));
	sprintf(mensagem, "Pelo menos tu chegou ao nivel %d.", GAME_LEVEL);
	desenhaTexto( mensagem);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto("PRESSIONE BARRA DE ESPACOS PARA REINICIAR O JOGO");
}
void desenhaParabens(){

}
//==== The Textures Functions ==========================================//
void carregarTextura(GLuint texture, const char* filename){

	FIBITMAP *pImage = FreeImage_Load( FIF_PNG, filename, PNG_DEFAULT);
		int nWidth = FreeImage_GetWidth(pImage);
		int nHeight = FreeImage_GetHeight(pImage);

	//

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight,
					0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(pImage));
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap),
	//    0, GL_RGB, GL_UNSIGNED_BYTE, FreeImage_ConvertTo32Bits(bitmap));
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//FreeImage_Unload(bitmap);

	FreeImage_Unload(pImage);
}

void carregarImagens(void){
	glGenTextures(4, texturasCarteiro);
	carregarTextura(texturasCarteiro[CHAR_POSITION_DOWN_LEFT], "images/carteiro-down-left.png");
	carregarTextura(texturasCarteiro[CHAR_POSITION_DOWN_RIGHT], "images/carteiro-down-right.png");
	carregarTextura(texturasCarteiro[CHAR_POSITION_UP_LEFT], "images/carteiro-up-left.png");
	carregarTextura(texturasCarteiro[CHAR_POSITION_UP_RIGHT], "images/carteiro-up-right.png");

	//{UP-RIGHT, UP-LEFT, DOWN-RIGHT, DOWN-LEFT}
}

//======================================================================//

void novaCor(int elemento){
	switch (elemento) {
		case CIRCLE_COLOR:
			corCircR = Random::get(0,255)/255.0;
			corCircG = Random::get(0,255)/255.0;
			corCircB = Random::get(0,255)/255.0;
			break;
		case MAZE_COLOR:
			corLabiR = Random::get(0,255)/255.0;
			corLabiG = Random::get(0,255)/255.0;
			corLabiB = Random::get(0,255)/255.0;
			break;
		case BACK_COLOR:
			corFundR = Random::get(0,255)/255.0;
			corFundG = Random::get(0,255)/255.0;
			corFundB = Random::get(0,255)/255.0;
			break;
		case RESET_COLOR:
			corCircR = corCircG = corCircB = 0.9;
			corVidaR = corVidaG = 0;
			corVidaB = 1;
			corFundR = 0.4;
			corFundG = corFundB = fabs(1-corCircR);
			corLabiR = corLabiG = corLabiB = 0.08;
			break;
		case FLASH_COLOR:
			if (CIRCLE_FLASH){
				corCircR = fabs(1.0-corCircR);
				corCircG = fabs(1.0-corCircG);
				corCircB = fabs(1.0-corCircB);
			}
			if (MAZE_FLASH){
				corLabiR = fabs(1.0-corLabiR);
				corLabiG = fabs(1.0-corLabiG);
				corLabiB = fabs(1.0-corLabiB);
			}
			break;
	}
}
//======================================================================//
void piscarCirculo(int value) {
	if (CIRCLE_FLASH) {
		glutTimerFunc(150,piscarCirculo,1);
		novaCor(FLASH_COLOR);
		glutPostRedisplay();
	}
}
void piscarLabirinto(int value) {
	if (MAZE_FLASH) {
		glutTimerFunc(150,piscarLabirinto,1);
		novaCor(FLASH_COLOR);
		glutPostRedisplay();
	}
}
//======================================================================//

//==== The Menu Functions ==============================================//
void menuDificuldade(int op) {
	switch (op){
		case MENU_DIFFICULTY_INCREASE: // Aumentar nível
			novaDificuldade(GAME_LEVEL+1,false);
			break;\
		case MENU_DIFFICULTY_DECREASE: // Diminuir nível
			novaDificuldade(GAME_LEVEL-1,false);
			break;
		case MENU_DIFFICULTY_SUPER_INCREASE: // Aumentar 10 níveis
			novaDificuldade(GAME_LEVEL+10,false);
			break;
		case MENU_DIFFICULTY_SUPER_DECREASE: // Diminuir 10 níveis
			novaDificuldade(GAME_LEVEL-10,false);
			break;
	}
	glutPostRedisplay();
}
void menuOpcoes(int op) {
	switch (op){
		case MENU_OPTIONS_RESTART: // 	Reiniciar jogo (volta ao nível 1)
			novaDificuldade(1,true);
			break;
		case MENU_OPTIONS_RESET: // 	Reiniciar jogo (volta à tela inicial)
			novaDificuldade(1,true);
			GAME_STATUS = GAME_WELCOME;
			break;
		case MENU_OPTIONS_RESET_MAZE:	//	Resetar labirinto (continua no nível atual)
			generateRandomMaze();
			retornarInicio();
			break;
		case MENU_OPTIONS_RESET_COLORS:	//	Resetar cores do jogo
			novaCor(RESET_COLOR);
			break;
		case MENU_OPTIONS_RESET_LEVEL: //	Voltar ao início do level atual
			retornarInicio();
			break;
	}
	glutPostRedisplay();
}
void menuCores(int op){
	switch (op){
		case MENU_COLORS_CHANGE_CIRCLE: // Mudar cor do círculo
			novaCor(CIRCLE_COLOR);
			break;
		case MENU_COLORS_CHANGE_WALLS:	//	Mudar cor das paredes do labirinto
			novaCor(MAZE_COLOR);
			break;
		case MENU_COLORS_CHANGE_BACKGROUND: // 	Mudar cor do fundo
			novaCor(BACK_COLOR);
			break;
		case MENU_COLORS_RESET: //	Resetar cores
			novaCor(RESET_COLOR);
			break;
		case MENU_COLORS_FLASH_MAZE: // 	Piscar ou parar de piscar labirinto
			MAZE_FLASH = !MAZE_FLASH;
			if (MAZE_FLASH)
					glutTimerFunc(150,piscarLabirinto,1);
			break;
	}
	glutPostRedisplay();
}
void menuPrincipal(int op){
	switch (op){
		case MENU_HELP:
			if (GAME_STATUS == GAME_HELP){
				GAME_STATUS = GAME_PREVIOUS_STATUS;
			}
			else {
				GAME_PREVIOUS_STATUS = GAME_STATUS;
				GAME_STATUS = GAME_HELP;
			}
			break;
		case MENU_EXIT:
			exit(0);
			break;
		default:
			break;
	}
}
void exibirMenu() {
	int menu, dificuldade, opcoes, cores;
	dificuldade = glutCreateMenu(menuDificuldade);

		glutAddMenuEntry("[+] Aumentar nivel",MENU_DIFFICULTY_INCREASE);
		glutAddMenuEntry("[-] Diminuir nivel",MENU_DIFFICULTY_DECREASE);
		glutAddMenuEntry("[++] Aumentar 10 niveis",MENU_DIFFICULTY_SUPER_INCREASE);
		glutAddMenuEntry("[--] Diminuir 10 niveis",MENU_DIFFICULTY_SUPER_DECREASE);
	opcoes = glutCreateMenu(menuOpcoes);
		glutAddMenuEntry("Reiniciar jogo (volta ao nivel 1)",MENU_OPTIONS_RESTART);
		glutAddMenuEntry("Reiniciar jogo (volta a tela inicial)",MENU_OPTIONS_RESET);
		glutAddMenuEntry("Resetar labirinto (continua no nivel atual",MENU_OPTIONS_RESET_MAZE);
		glutAddMenuEntry("Resetar cores",MENU_OPTIONS_RESET_COLORS);
		glutAddMenuEntry("Voltar ao inicio do level atual",MENU_OPTIONS_RESET_LEVEL);
	cores = glutCreateMenu(menuCores);
		//glutAddMenuEntry("Mudar cor do circulo",MENU_COLORS_CHANGE_CIRCLE);
		glutAddMenuEntry("Mudar cor das paredes do labirinto",MENU_COLORS_CHANGE_WALLS);
		glutAddMenuEntry("Mudar cor do fundo",MENU_COLORS_CHANGE_BACKGROUND);
		glutAddMenuEntry("Resetar cores",MENU_COLORS_RESET);
		glutAddMenuEntry("Piscar ou parar de piscar labirinto",MENU_COLORS_FLASH_MAZE);
	menu = glutCreateMenu(menuPrincipal);
		glutAddSubMenu("DIFICULDADE",dificuldade);
		glutAddSubMenu("OPCOES",opcoes);
		glutAddSubMenu("CORES",cores);
		glutAddMenuEntry("AJUDA",MENU_HELP);
		glutAddMenuEntry("SAIR",MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//======================================================================//

//==== The Mouse Clicks Function =======================================//
void myMouseFunc(int button, int state, int x, int y){
	//y^2 + x^2 = r^2
	int xSRU = getXSRU(SRU, SRD,x);
	int ySRU = getYSRU(SRU, SRD,y);
	int paramCirculo = (sqrt((xSRU-xc)*(xSRU-xc)+(ySRU-yc)*(ySRU-yc)));

	GLubyte *data = (GLubyte *) malloc( 3 * 1 * 1);
	if( data ) {
	    glReadPixels(x, glutGet( GLUT_WINDOW_HEIGHT ) - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	switch(button) {
		case GLUT_LEFT_BUTTON:
			if (paramCirculo <= raio)	//	Identifica se a região do clique foi dentro do círculo
				novaCor(CIRCLE_COLOR);
			else if (ceil(data[0]/25.5f) == ceil(corLabiR*10) &&
					ceil(data[1]/25.5f) == ceil(corLabiG*10) &&
					ceil(data[2]/25.5f) == ceil(corLabiB*10)
					)
				novaCor(MAZE_COLOR);
			else if (ceil(data[0]/25.5f) == ceil(corFundR*10) &&
					ceil(data[1]/25.5f) == ceil(corFundG*10) &&
					ceil(data[2]/25.5f) == ceil(corFundB*10)
					) {
				novaCor(BACK_COLOR);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			exibirMenu();
			break;
		default:
			break;
	}
	//printf("m(%d, %d) c(%d, %d) -- d %d -- SRU(%d, %d)\n", x, y, xc, yc, paramCirculo, xSRU, ySRU);
	//printf("rgb( %d, %d, %d) - rgb.f( %f, %f, %f)\nRGB( %d, %d, %d) -  RGB.f( %f, %f, %f)\n",
	//		data[0], data[1], data[2], data[0]/255.0f, data[1]/255.0f, data[2]/255.0f,
	//		(int) (corLabiR*255.0), (int)(corLabiG*255.0), (int)(corLabiB*255.0), corLabiR, corLabiG, corLabiB);
	glutPostRedisplay();
}
//======================================================================//
void myMotionFunc(int x, int y){

}

//======================================================================//
void myKeyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		// TECLAS DE FUNCIONALIDADES DO JOGO
		case 27:	//	ESC - ESCAPE - GLUT_ESCAPE_KEY
			menuPrincipal(MENU_EXIT);
			break;
		case 13:	//	ENTER - GLUT_ENTER_KEY
		case GLUT_SPACEBAR_KEY:
			if (GAME_STATUS == GAME_WELCOME || GAME_STATUS == GAME_NEWLEVEL)
				GAME_STATUS = GAME_START;
			else if (GAME_STATUS == GAME_START && key == GLUT_SPACEBAR_KEY) {
				menuCores(MENU_COLORS_FLASH_MAZE);
			}
			else if (GAME_STATUS == GAME_OVER){
				novaDificuldade(1,true);
				GAME_STATUS = GAME_WELCOME;
			}
			break;
		//*
		case 'c':
			if (exitPathExists(floor((xc0-ORTHO_LEFT)/MAZE_STEP),floor((yc0-ORTHO_BOTTOM)/MAZE_STEP),0))
				printf("VERDADE - TEM UM CAMINHO\n");
			else
				printf("NÃO EXISTE\n");
			break;
		//*/
		case 'h':
			menuPrincipal(MENU_HELP);
			break;
		case 'r':
			menuOpcoes(MENU_OPTIONS_RESET_MAZE);
			break;
		case 'R':
			menuOpcoes(MENU_OPTIONS_RESET);
			break;
		case 'b':
			menuCores(MENU_COLORS_CHANGE_BACKGROUND);
			break;
		case 'n':
			menuCores(MENU_COLORS_RESET);
			break;
		case 'm':
			menuCores(MENU_COLORS_CHANGE_WALLS);
			break;
		// TECLAS DE TRAPAÇA DO JOGO - CHEATS
		case 'L':
			MAZE_LIGHT_STATUS = (!MAZE_LIGHT_STATUS);
			break;
		case 'i':
			menuDificuldade(MENU_DIFFICULTY_INCREASE);
			break;
		case 'I':
			menuDificuldade(MENU_DIFFICULTY_SUPER_INCREASE);
			break;
		case 'd':
			menuDificuldade(MENU_DIFFICULTY_DECREASE);
			break;
		case 'D':
			menuDificuldade(MENU_DIFFICULTY_SUPER_DECREASE);
			break;
		default:
			break;
	}
	glutPostRedisplay();
}
//======================================================================//
void mySpecialFunc(int key, int x, int y){
	double temp;
	if (GAME_STATUS == GAME_START) {
		switch (key) {
			case GLUT_KEY_LEFT:
				if (eixoH == 1) {
					temp = xc;
					xc -= CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						xc = temp;
				}
				else {
					eixoH = 1;
					if (eixoV == 1)
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_DOWN_LEFT]);
					else
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_UP_LEFT]);
				}
				break;
			case GLUT_KEY_UP:
				if (eixoV == 0) {
					temp = yc;
					yc += CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						yc = temp;
				}
				else {
					eixoV = 0;
					if (eixoH == 1)
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_UP_LEFT]);
					else
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_UP_RIGHT]);
				}
				break;
			case GLUT_KEY_RIGHT:
				if (eixoH == 0) {
					temp = xc;
					xc += CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						xc = temp;
				}
				else {
					eixoH = 0;
					if (eixoV == 1)
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_DOWN_RIGHT]);
					else
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_UP_RIGHT]);
				}
				break;
			case GLUT_KEY_DOWN:
				if (eixoV == 1) {
					temp = yc;
					yc -= CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						yc = temp;
				}
				else {
					eixoV = 1;
					if (eixoH == 1)
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_DOWN_LEFT]);
					else
						glBindTexture(GL_TEXTURE_2D, texturasCarteiro[CHAR_POSITION_DOWN_RIGHT]);
				}
				break;
			default:
				break;
		}
	}
	glutPostRedisplay();
}
//======================================================================//
void myDisplayFunc(){
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(corFundR, corFundG, corFundB, 0.0f);

	if (GAME_STATUS == GAME_WELCOME) {
		desenhaBoasVindas();
	}
	else if (GAME_STATUS == GAME_START) {
		desenhaLabirinto();
		if (MAZE_LIGHT_STATUS == true)
			desenhaLuz();
		if (OBJECT_CLASS == OBJECT_CIRCLE)
			desenhaCirculo();
		else if (OBJECT_CLASS == OBJECT_SQUARE)
			desenhaQuadrado();
		desenhaVidas();
	}
	else if (GAME_STATUS == GAME_NEWLEVEL){
		desenhaNovoNivel();
	}
	else if (GAME_STATUS == GAME_WIN){
		desenhaParabens();
	}
	else if (GAME_STATUS == GAME_OVER){
		desenhaFimDeJogo();
	}
	else if (GAME_STATUS == GAME_HELP){
		desenhaAjuda();
	}

	glutSwapBuffers();
	//glutPostRedisplay();
	//glutSwapBuffers();
}
//======================================================================//
void myReshapeFunc(int w, int h){
	SRD[X_MAX] = w;
	SRD[Y_MAX] = h;
	//printf("m(%d, %d)", w, h);
	glutReshapeWindow(w, h);
	glViewport(0,0, w, h);
	glutPostRedisplay();
}

//======================================================================//
void allocMaze(){
	maze = (mesh **) malloc(sizeof(mesh*) * MESH_WIDTH_PARTS  );
	if (!maze) {
		printf("ERROR - Can't allocate memory for maze.");
		exit(0);
	}
	for (int c = 0; c < MESH_WIDTH_PARTS; c++){
		maze[c] = (mesh *) malloc(sizeof(mesh) * MESH_HEIGTH_PARTS );
		if (!maze[c]) {
			printf("ERROR - Can't allocate memory for maze.");
			exit(0);
		}
	}
}
void novaDificuldade(int nivel, bool resetarCores) {
	if (nivel < 1)
		GAME_LEVEL = 1;
	else
		GAME_LEVEL = nivel;
	// 	Definindo variáveis do jogo
	//GAME_STATUS = 1;
	//CIRCLE_RADIUS = (50.0/ceil(log(nivel)));
	CIRCLE_RADIUS = ceil(50.0/exp(GAME_LEVEL/CIRCLE_RADIUS_DECREASE_TIME_CONSTANT));
	CIRCLE_POINT_SIZE = 1.0f;
	CIRCLE_CENTER_SPEED = (1/4.0);
	CIRCLE_INITIAL_LIFE = 4;

	//ORTHO_WIDTH = 1920;
	//ORTHO_HEIGTH = 1080;
	//ORTHO_LEFT = -(ORTHO_WIDTH/2);
	//ORTHO_RIGHT = (ORTHO_WIDTH/2);
	//ORTHO_BOTTOM  = -(ORTHO_HEIGTH/2);
	//ORTHO_TOP = (ORTHO_HEIGTH/2);
	//WINDOW_PROPORTION = 0.5;
	//WINDOW_WIDTH = (ORTHO_WIDTH*WINDOW_PROPORTION);
	//WINDOW_HEIGTH = (ORTHO_HEIGTH*WINDOW_PROPORTION);

	MAZE_STEP = (CIRCLE_RADIUS*6);
	CIRCLE_CENTER_DISPLACEMENT = CIRCLE_CENTER_SPEED*MAZE_STEP;
	MAZE_LINE_SIZE = CIRCLE_RADIUS*(1.0/4.0);
	MESH_WIDTH_PARTS = ORTHO_WIDTH/MAZE_STEP +1;
	MESH_HEIGTH_PARTS = ORTHO_HEIGTH/MAZE_STEP +1;
	MESH_WIDTH_PARTS_OPENNING_PROBABILITY = 0.5/exp(GAME_LEVEL/MESH_PARTS_OPPENING_DECREASE_TIME_CONSTANT);
	MESH_HEIGTH_PARTS_OPENNING_PROBABILITY =  0.7/exp(GAME_LEVEL/MESH_PARTS_OPPENING_DECREASE_TIME_CONSTANT);

	MAZE_LIGHT_SIZE = MAZE_STEP*MAZE_LIGHT_MULT;
	// MAZE_LIGHT_STATUS = true;

	raio = CIRCLE_RADIUS;
	vidas = CIRCLE_INITIAL_LIFE;
	if (resetarCores)
		novaCor(RESET_COLOR);


	SRD[X_MIN] = 0;
	SRD[X_MAX] = WINDOW_WIDTH;
	SRD[Y_MIN] = 0;
	SRD[Y_MAX] = WINDOW_HEIGTH;

	allocMaze();
	generateRandomMaze();
//	auto val = Random::get(-MESH_WIDTH_PARTS/6,MESH_WIDTH_PARTS/6);
//	yc = yc0 = -(ORTHO_HEIGTH/2)+((ORTHO_HEIGTH/2/MAZE_STEP)*MAZE_STEP) + MAZE_STEP/2 ;
//	xc = xc0 = -(ORTHO_WIDTH/2)+((ORTHO_WIDTH/MAZE_STEP/2)*MAZE_STEP) + MAZE_STEP/2;
	yc = yc0 = ORTHO_BOTTOM+(MESH_HEIGTH_PARTS/2)*MAZE_STEP + MAZE_STEP/2 ;
	xc = xc0 = ORTHO_LEFT+(MESH_WIDTH_PARTS/2)*MAZE_STEP + MAZE_STEP/2;

	while (!exitPathExists(floor((xc0-ORTHO_LEFT)/MAZE_STEP),floor((yc0-ORTHO_BOTTOM)/MAZE_STEP),0)){
		generateRandomMaze();
	}
	//printf("W: %d H: %d\n",	MESH_WIDTH_PARTS, MESH_HEIGTH_PARTS);
}
// Inicializa parâmetros de rendering
void Inicializa (void)
{
	FreeImage_Initialise(true);
	novaCor(RESET_COLOR);
	glClearColor(corFundR, corFundG, corFundB, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluOrtho2D(ORTHO_LEFT-50,ORTHO_RIGHT+50,ORTHO_BOTTOM-50,ORTHO_TOP+50);
	gluOrtho2D(ORTHO_LEFT,ORTHO_RIGHT,ORTHO_BOTTOM,ORTHO_TOP);
	glMatrixMode(GL_MODELVIEW);
	novaDificuldade(1,true);

	// Habilita transparências nas imagens
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	carregarImagens();

	glutTimerFunc(150,piscarLabirinto,1);
}

// Programa principal

int main(int argc, char** argv)
{
	printf ("Localidade corrente é: %s\n", setlocale(LC_ALL,"") );
	//char str[50];// = "Wastelands Maze 2.0 by Ricardo e Ruan Medeiros e Jose Adolfo";
	glutInit(&argc,argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
 	glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGTH);
	glutInitWindowPosition(10,10);

	sprintf(tituloJanela, "Wastelands Maze 2.0 by Ricardo, Ruan Medeiros e Jose Adolfo - Vidas: %d", vidas);
	glutCreateWindow(tituloJanela);
	OpenClipboard(NULL);
	Inicializa();
	glutDisplayFunc 	( myDisplayFunc	);
	glutMouseFunc   	( myMouseFunc   );
	glutMotionFunc  	( myMotionFunc  );
	glutKeyboardFunc	( myKeyboardFunc);
	glutSpecialFunc		( mySpecialFunc	);
	glutReshapeFunc 	( myReshapeFunc );


	glutMainLoop();

	return 0;
}
