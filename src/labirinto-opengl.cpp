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
#define RESET_COLOR 0
#define CIRCLE_COLOR 1
#define MAZE_COLOR 2
#define BACK_COLOR 3
#define FLASH_COLOR 4

#define GLUT_SPACEBAR_KEY 32
#define GAME_OVER 0
#define GAME_WELCOME 1
#define GAME_START 2
#define GAME_WIN 3
#define GAME_NEWLEVEL 4

int GAME_STATUS = 1;
int GAME_LEVEL = 1;
double CIRCLE_RADIUS = 5*GAME_LEVEL;
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
double MAZE_STEP = (CIRCLE_RADIUS*6);
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
}mesh;

mesh **maze;

double x,y;
double xc = 0, yc = 0, xc0 = 0, yc0 = 0, raio = CIRCLE_RADIUS;
int eixo = 0;
int vidas = CIRCLE_INITIAL_LIFE;
static GLuint texture = 0;
static GLuint texturasCarteiro[4] = {0,1,2,3}; // { Estados do carteiro

double corCircR,corCircG,corCircB;
double corVidaR,corVidaG,corVidaB;
double corFundR,corFundG,corFundB;
double corLabiR,corLabiG,corLabiB;
//  SISTEMA = {Xmin,Xmax,Ymin,Ymax]
double SRU[4] = {ORTHO_LEFT,ORTHO_RIGHT,ORTHO_BOTTOM,ORTHO_TOP};
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
void resetMazeMesh() {
	for(int l=0;l<MESH_WIDTH_PARTS;l++)
		for(int c=0;c<MESH_HEIGTH_PARTS;c++){
			maze[l][c].side = 0;
			maze[l][c].top = 0;
		}
}
bool isOnMaze(double x,double y) {
	double xSRD = getXSRD(SRU, SRD, x);
	double ySRD = getYSRD(SRU, SRD, y);

	GLubyte *data = (GLubyte *) malloc( 3 * 1 * 1);
	if( data ) {
		glReadPixels(xSRD, glutGet( GLUT_WINDOW_HEIGHT ) -  ySRD, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
		//printf("SRD( %d, %d)\n", xSRD, glutGet( GLUT_WINDOW_HEIGHT ) -ySRD);
		printf("MAZE - rgb( %d, %d, %d)\nRGB( %d, %d, %d)\n SRD ( %f, %f)\n",
				data[0], data[1], data[2],
					(int) (corLabiR*255.0), (int)(corLabiG*255.0), (int)(corLabiB*255.0),
					xSRD, ySRD);
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
bool isOnLimit(int x,int y)
{	if(x <= ORTHO_LEFT || x >= ORTHO_RIGHT || y<= ORTHO_BOTTOM || y >= ORTHO_TOP)
		return true;
	else
		return false;
}
bool verificarColisao(){
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
void desenhaQuadrado(void){
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f,0.0f);
			glVertex2f(xc-raio,yc-raio);
		glTexCoord2f(0.0f,1.0f);
			glVertex2f(xc-raio,yc+raio);
		glTexCoord2f((eixo?-1:1)*1.0f,1.0f);
			glVertex2f(xc+raio,yc+raio);
		glTexCoord2f((eixo?-1:1)*1.0f,0.0f);
			glVertex2f(xc+raio,yc-raio);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
//======================================================================//
void desenhaLuz(void) {
	double luz=raio*5.5;
	glColor3f(corFundR, corFundG, corFundB);
	glBegin(GL_QUADS);
		glVertex3f(ORTHO_LEFT,ORTHO_TOP,0.0f);
		glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
		glVertex2f(xc-luz,ORTHO_BOTTOM);
		glVertex2f(xc-luz,ORTHO_TOP);

		glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
		glVertex2f(xc+luz,ORTHO_BOTTOM);
		glVertex2f(xc+luz,ORTHO_TOP);

		glVertex2f(ORTHO_LEFT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,ORTHO_TOP);
		glVertex2f(ORTHO_RIGHT,yc+luz);
		glVertex2f(ORTHO_LEFT,yc+luz);

		glVertex2f(ORTHO_LEFT,ORTHO_BOTTOM);
		glVertex2f(ORTHO_RIGHT,ORTHO_BOTTOM);
		glVertex2f(ORTHO_RIGHT,yc-luz);
		glVertex2f(ORTHO_LEFT,yc-luz);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
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
		glColor3f(corVidaR, corVidaG, corVidaB);
		glBegin(GL_QUADS);
			for (int c = vidas; c; c--){
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
void desenhaTexto(void *font, char *string) {	// Exibe caractere a caractere
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
	glTranslatef(ORTHO_LEFT*0.5,ORTHO_BOTTOM*0.2-100,0);
	//glScalef(0.2, 0.2, 0.2); // diminui o tamanho do fonte
	//glRotatef(15, 0,0,1); // rotaciona o texto
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,"Wastelands Maze");
	glPopMatrix();

	// Posição no universo onde o texto bitmap será colocado
	glColor3f(1,1,1);
	//glScalef(1.0, 1.0, 1.0); // diminui o tamanho do fonte
	//glRotatef(15, 0,0,1); // rotaciona o texto
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_TOP*0.8;
	glRasterPos2f(textoX,textoY);
	desenhaTexto(GLUT_BITMAP_9_BY_15,"Mova o circulo para fora do labirinto (sair pelas bordas) para subir de nivel.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- MOVIMENTOS: Utilize as setas do teclado para mover o circulo.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- CORES: Para mudar a cor de qualquer objeto (CIRCULO, PAREDES DO LABIRINTO, FUNDO) ");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"         basta clicar no objeto em questao.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- DESAFIO: Se o circulo se mover em direcao a qualquer parede do labirinto");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"           entao ele volta a posicao inicial no labirinto e perde uma vida.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- FIM: O jogo acaba quando suas vidas terminarem.");
	glColor3f(0,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"                   PRESSIONE BARRA DE ESPACOS PARA COMECAR");
	glColor3f(1,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- DICA: A barra de espacos tambem funciona para fazer o circulo piscar.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"- DICA 2: Ao subir de nivel voce fica novamente com 4 vidas.");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"BOA SORTE E BOM JOGO!");
}
void desenhaNovoNivel(){
	char nivel[10];
	sprintf(nivel, "%d", GAME_LEVEL);
	glColor3f(1,1,1);
	glPushMatrix();
	glTranslatef(ORTHO_LEFT*0.5,ORTHO_TOP*0.3,0);
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,"Wastelands Maze");
	glPopMatrix();

	glColor3f(1,1,1);
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_BOTTOM*0.1;
	glRasterPos2f(textoX,textoY);
	desenhaTexto(GLUT_BITMAP_9_BY_15,"BEM VINDO AO NIVEL");
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15, nivel);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"PRESSIONE BARRA DE ESPACOS PARA CONTINUAR NO LABIRINTO");
}
void desenhaFimDeJogo(){
	char mensagem[50];

	glColor3f(1,1,1);
	glPushMatrix();
	glTranslatef(ORTHO_LEFT*0.5,ORTHO_TOP*0.3,0);
	glLineWidth(2); // define a espessura da linha
	desenhaTextoStroke(GLUT_STROKE_ROMAN,"Wastelands Maze");
	glPopMatrix();

	glColor3f(1,1,1);
	int textoX =  ORTHO_LEFT*0.8, textoY = ORTHO_BOTTOM*0.1;
	glColor3f(1,0,0);
	glRasterPos2f(textoX,textoY);
	desenhaTexto(GLUT_BITMAP_9_BY_15,"FIM DE JOGO");
	glColor3f(1,1,1);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"Uma pena! Tente jogar melhor da proxima vez!");
	glRasterPos2f(textoX,(textoY-=50));
	sprintf(mensagem, "Pelo menos tu chegou ao nivel %d.", GAME_LEVEL);
	desenhaTexto(GLUT_BITMAP_9_BY_15, mensagem);
	glRasterPos2f(textoX,(textoY-=50));
	desenhaTexto(GLUT_BITMAP_9_BY_15,"PRESSIONE BARRA DE ESPACOS PARA REINICIAR O JOGO");
}
void desenhaParabens(){

}
//==== The Menu Functions ==============================================//
void novaCor(int elemento){
	switch (elemento) {
		case CIRCLE_COLOR:
			//corCircR = Random::get(0,255)/255.0;
			//corCircG = Random::get(0,255)/255.0;
			//corCircB = Random::get(0,255)/255.0;
			break;
		case MAZE_COLOR:
			//corLabiR = Random::get(0,255)/255.0;
			//corLabiG = Random::get(0,255)/255.0;
			//corLabiB = Random::get(0,255)/255.0;
			break;
		case BACK_COLOR:
			//corFundR = Random::get(0,255)/255.0;
			//corFundG = Random::get(0,255)/255.0;
			//corFundB = Random::get(0,255)/255.0;
			break;
		case RESET_COLOR:
			corCircR = corCircG = corCircB = 0.9;
			corVidaR = corVidaG = 0;
			corVidaB = 1;
			corFundR = 0.4;
			corFundG = corFundB = fabs(1-corCircR);
			corLabiR = corLabiG = corLabiB = 0.7;
			break;
		case FLASH_COLOR:
			corCircR = fabs(1.0-corCircR);
			corCircG = fabs(1.0-corCircG);
			corCircB = fabs(1.0-corCircB);
			break;
	}
}
void menuDificuldade(int op) {
	switch (op){
		case 1: // Aumentar nível
			novaDificuldade(GAME_LEVEL+1,false);
			break;\
		case 2: // Diminuir nível
			novaDificuldade(GAME_LEVEL-1,false);
			break;
		case 3: // Aumentar 10 níveis
			novaDificuldade(GAME_LEVEL+10,false);
			break;
		case 4: // Diminuir 10 níveis
			novaDificuldade(GAME_LEVEL-10,false);
			break;
	}
	glutPostRedisplay();
}
void menuOpcoes(int op) {
	switch (op){
		case 1: // 	Reiniciar jogo (volta ao nível 1)
			novaDificuldade(1,true);
			break;
		case 2: // 	Reiniciar jogo (volta à tela inicial)
			novaDificuldade(1,true);
			GAME_STATUS = GAME_WELCOME;
			break;
		case 3:	//	Resetar labirinto (continua no nível atual)
			generateRandomMaze();
			retornarInicio();
			break;
		case 4:	//	Resetar cores do jogo
			novaCor(RESET_COLOR);
			break;
		case 5: //	Voltar ao início do level atual
			retornarInicio();
			break;
	}
	glutPostRedisplay();
}
void menuCores(int op){
	switch (op){
		case 1: // Mudar cor do círculo
			novaCor(CIRCLE_COLOR);
			break;
		case 2:	//	Mudar cor das paredes do labirinto
			novaCor(MAZE_COLOR);
			break;
		case 3: // 	Mudar cor do fundo
			novaCor(BACK_COLOR);
			break;
		case 4: //	Resetar cores
			novaCor(RESET_COLOR);
			break;
		case 5: // 	Realçar círculo
			CIRCLE_FLASH = !CIRCLE_FLASH;
			break;
	}
	glutPostRedisplay();
}
void menuPrincipal(int op){

}
void exibirMenu() {
	int menu, dificuldade, opcoes, cores;
	dificuldade = glutCreateMenu(menuDificuldade);
		glutAddMenuEntry("[+] Aumentar nivel",1);
		glutAddMenuEntry("[-] Diminuir nivel",2);
		glutAddMenuEntry("[++] Aumentar 10 niveis",3);
		glutAddMenuEntry("[--] Diminuir 10 niveis",4);
	opcoes = glutCreateMenu(menuOpcoes);
		glutAddMenuEntry("Reiniciar jogo (volta ao nivel 1)",1);
		glutAddMenuEntry("Reiniciar jogo (volta a tela inicial)",2);
		glutAddMenuEntry("Resetar labirinto (continua no nivel atual",3);
		glutAddMenuEntry("Resetar cores",4);
		glutAddMenuEntry("Voltar ao inicio do level atual",5);
	cores = glutCreateMenu(menuCores);
		glutAddMenuEntry("Mudar cor do circulo",1);
		glutAddMenuEntry("Mudar cor das paredes do labirinto",2);
		glutAddMenuEntry("Mudar cor do fundo",3);
		glutAddMenuEntry("Resetar cores",4);
	menu = glutCreateMenu(menuPrincipal);
		glutAddSubMenu("Dificuldade",dificuldade);
		glutAddSubMenu("Opcoes",opcoes);
		glutAddSubMenu("Cores",cores);
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
void piscarCirculo(int value) {
	if (CIRCLE_FLASH) {
		glutTimerFunc(200,piscarCirculo,1);
		novaCor(FLASH_COLOR);
		glutPostRedisplay();
	}
}
//======================================================================//
void myKeyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 27:
			exit(0);
		case GLUT_SPACEBAR_KEY:
			if (GAME_STATUS == GAME_WELCOME || GAME_STATUS == GAME_NEWLEVEL)
				GAME_STATUS = GAME_START;
			else if (GAME_STATUS == GAME_START) {
				CIRCLE_FLASH = !CIRCLE_FLASH;
				glutTimerFunc(100,piscarCirculo,1);
			}
			else if (GAME_STATUS == GAME_OVER){
				novaDificuldade(1,true);
				GAME_STATUS = GAME_WELCOME;
			}
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
				if (eixo) {
					temp = xc;
					xc -= CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						xc = temp;
				}
				else
					eixo = 1;
				break;
			case GLUT_KEY_UP:
				temp = yc;
				yc += CIRCLE_CENTER_DISPLACEMENT;
				if (verificarStatus())
					yc = temp;
				break;
			case GLUT_KEY_RIGHT:
				if (!eixo) {
					temp = xc;
					xc += CIRCLE_CENTER_DISPLACEMENT;
					if (verificarStatus())
						xc = temp;
				}
				else
					eixo = 0;
				break;
			case GLUT_KEY_DOWN:
				temp = yc;
				yc -= CIRCLE_CENTER_DISPLACEMENT;
				if (verificarStatus())
					yc = temp;
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
		desenhaVidas();
		desenhaLuz();
		//desenhaCirculo();
		desenhaQuadrado();
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
	CIRCLE_RADIUS = (50.0/exp(GAME_LEVEL/CIRCLE_RADIUS_DECREASE_TIME_CONSTANT));
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
	MESH_WIDTH_PARTS = ORTHO_WIDTH/MAZE_STEP;
	MESH_HEIGTH_PARTS = ORTHO_HEIGTH/MAZE_STEP;
	MESH_WIDTH_PARTS_OPENNING_PROBABILITY = 0.5/exp(GAME_LEVEL/MESH_PARTS_OPPENING_DECREASE_TIME_CONSTANT);
	MESH_HEIGTH_PARTS_OPENNING_PROBABILITY =  0.7/exp(GAME_LEVEL/MESH_PARTS_OPPENING_DECREASE_TIME_CONSTANT);

	//
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

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (1)
	{

		FIBITMAP* bitmap = FreeImage_Load( FIF_PNG, "images/carteiro.png", PNG_DEFAULT);
		//
		//FIBITMAP* bitmap = FreeImage_Load(
		//	FreeImage_GetFileType("images/carteiro.png", 0),
		//	"images/carteiro.png");
		//RGBQUAD *palette = FreeImage_GetPalette(bitmap);
		//bitmap = FreeImage_ConvertTo24Bits(bitmap);

		FIBITMAP *pImage = FreeImage_ConvertTo32Bits(bitmap);
		//FIBITMAP *pImage = bitmap;
			int nWidth = FreeImage_GetWidth(pImage);
			int nHeight = FreeImage_GetHeight(pImage);

		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight,
						0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(pImage));
		//glTexImage2D(GL_TEXTURE_2D, 0, 3, FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap),
		//    0, GL_RGB, GL_UNSIGNED_BYTE, FreeImage_ConvertTo32Bits(bitmap));


		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		FreeImage_Unload(bitmap);

		//FreeImage_Unload(pImage);
	}
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

	sprintf(tituloJanela, "Wastelands Maze by Ricardo e Ruan Medeiros - Vidas: %d", vidas);
	glutCreateWindow(tituloJanela);

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
